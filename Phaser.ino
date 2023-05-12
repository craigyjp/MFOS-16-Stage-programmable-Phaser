#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "Parameters.h"
#include <CircularBuffer.h>
#include "PatchMgr.h"
#include "HWControls.h"
#include "EepromMgr.h"
#include "Settings.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <MCP4922.h>



#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page

unsigned int state = PARAMETER;

#include "ST7735Display.h"

int patchNo = 1;
boolean cardStatus = false;

unsigned long buttonDebounce = 0;

//MIDI 5 Pin DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);  //RX - Pin 0

void setup() {

  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // OLED I2C Address, may need to change for different device,
  setUpSettings();

  setupHardware();
  renderBootUpPage();

  cardStatus = SD.begin(SDCARD);
  if (cardStatus) {

    //Get patch numbers and names from SD card
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
  } else {

    reinitialiseToPanel();
    showPatchPage("No SD", "conn'd / usable");
  }

  //Read MIDI Channel from EEPROM

  midiChannel = getMIDIChannel();
  if (midiChannel < 0 || midiChannel > 16) {
    storeMidiChannel(0);
  }

  //MIDI 5 Pin DIN
  MIDI.begin();
  MIDI.setHandleControlChange(myConvertControlChange);
  MIDI.setHandleProgramChange(myProgramChange);
  MIDI.setHandleClock(myMIDIclock);

  // Read ClockSource from EEPROM

  clocksource = getClockSource();
  if (clocksource < 0 || clocksource > 2) {
    storeClockSource(0);
  }
  switch (clocksource) {
    case 0:
      digitalWrite(MIDICLOCK_SELECT, LOW);
      break;

    case 1:
      digitalWrite(MIDICLOCK_SELECT, LOW);
      break;

    case 2:
      digitalWrite(MIDICLOCK_SELECT, HIGH);
      break;
  }
  oldclocksource = clocksource;

  //Read Encoder Direction from EEPROM
  encCW = getEncoderDir();
  startFirstTimer();
  recallPatch(patchNo);  //Load first patch
  updateScreen();
}

void startFirstTimer() {
  firsttimer = millis();
}

void MCP4922_write(const int &slavePin, const int &value1, const int &value2) {
  int value = 0;
  byte configByte = 0;
  byte data = 0;
  int channel = 0;

  for (channel = 0; channel < 2; channel++) {
    digitalWrite(slavePin, LOW);  //set DAC ready to accept commands
    if (channel == 0) {
      configByte = B01110000;  //channel 0, Vref buffered, Gain of 1x, Active Mode
      value = value1;
    } else {
      configByte = B11110000;  //channel 1, Vref buffered, Gain of 1x, Active Mode
      value = value2;
    }

    //write first byte
    data = highByte(value);
    data = B00001111 & data;   //clear out the 4 command bits
    data = configByte | data;  //set the first four command bits
    SPI.transfer(data);

    //write second byte
    data = lowByte(value);
    SPI.transfer(data);

    //close the transfer
    digitalWrite(slavePin, HIGH);  //set DAC ready to accept commands
  }
}

void myConvertControlChange(byte channel, byte number, byte value) {
  int newvalue = value << 3;
  myControlChange(channel, number, newvalue);
}

void myProgramChange(byte channel, byte program) {
  state = PATCH;
  patchNo = program + 1;
  recallPatch(patchNo);
  updateScreen();
  state = PARAMETER;
}

void recallPatch(int patchNo) {
  File patchFile = SD.open(String(patchNo).c_str());
  if (!patchFile) {
  } else {
    String data[NO_OF_PARAMS];  //Array of data read in
    recallPatchData(patchFile, data);
    setCurrentPatchData(data);
    patchFile.close();
  }
}

void setCurrentPatchData(String data[]) {
  patchName = data[0];
  lforate = data[1].toFloat();
  lfodepth = data[2].toFloat();
  feedback = data[3].toFloat();
  stage = data[4].toFloat();
  stage4 = data[5].toInt();
  stage8 = data[6].toInt();
  waveform = data[7].toFloat();
  stage12 = data[8].toInt();
  stage16 = data[9].toInt();

  //Switches
  updatestage(0);
  
  //Patchname
  updatePatchname();

}

String getCurrentPatchData() {
  return patchName + "," + String(lforate) + "," + String(lfodepth) + "," + String(feedback) + "," + String(stage) + "," + String(stage4) + "," + String(stage8) + "," + String(waveform) + "," + String(stage12) + "," + String(stage16);
}

void updatePatchname() {
  showPatchPage(String(patchNo), patchName);
}

void myMIDIclock() {

  if (millis() > clock_timeout + 300) clock_count = 0;  // Prevents Clock from starting in between quarter notes after clock is restarted!
  clock_timeout = millis();

  if (clock_count == 0) {
    digitalWrite(CLOCK_OUT, HIGH);  // Start clock pulse
    clock_timer = millis();
  }
  clock_count++;

  if (clock_count == 24) {  // MIDI timing clock sends 24 pulses per quarter note.  Sent pulse only once every 24 pulses
    clock_count = 0;
  }
}

void stopClockPulse() {
  if ((clock_timer > 0) && (millis() - clock_timer > 20)) {
    digitalWrite(CLOCK_OUT, LOW);  // Set clock pulse low after 20 msec
    clock_timer = 0;
  }
}

void updateLfoRate(boolean announce) {
  if (announce) {
    showCurrentParameterPage("LFO Rate", String(lforatestr) + " Hz");
  }
  //updateScreen();
}

void updateLfoDepth(boolean announce) {
  if (announce) {
    showCurrentParameterPage("LFO Depth", int(lfodepthstr));
  }
  //updateScreen();
}

void updatefeedback(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Feedback", int(feedbackstr));
  }
  //updateScreen();
}

void updatestage(boolean announce) {
  if (stage4) {
    if (announce) {
      showCurrentParameterPage("Stages", String("4 Stages"));
    }
    stage8 = 0;
    stage12 = 0;
    stage16 = 0;
    digitalWrite(MUXA0_OUT, LOW);
    digitalWrite(MUXA1_OUT, LOW);
    digitalWrite(STAGE4_LED, HIGH);
    digitalWrite(STAGE8_LED, LOW);
    digitalWrite(STAGE12_LED, LOW);
    digitalWrite(STAGE16_LED, LOW);
  }
  if (stage8) {
    if (announce) {
      showCurrentParameterPage("Stages", String("8 Stages"));
    }
    stage4 = 0;
    stage12 = 0;
    stage16 = 0;
    digitalWrite(MUXA0_OUT, HIGH);
    digitalWrite(MUXA1_OUT, LOW);
    digitalWrite(STAGE4_LED, LOW);
    digitalWrite(STAGE8_LED, HIGH);
    digitalWrite(STAGE12_LED, LOW);
    digitalWrite(STAGE16_LED, LOW);
  }
  if (stage12) {
    if (announce) {
      showCurrentParameterPage("Stages", String("12 Stages"));
    }
    stage4 = 0;
    stage8 = 0;
    stage16 = 0;
    digitalWrite(MUXA0_OUT, LOW);
    digitalWrite(MUXA1_OUT, HIGH);
    digitalWrite(STAGE4_LED, LOW);
    digitalWrite(STAGE8_LED, LOW);
    digitalWrite(STAGE12_LED, HIGH);
    digitalWrite(STAGE16_LED, LOW);
  }
  if (stage16) {
    if (announce) {
      showCurrentParameterPage("Stages", String("16 Stages"));
    }
    stage4 = 0;
    stage8 = 0;
    stage12 = 0;
    digitalWrite(MUXA0_OUT, HIGH);
    digitalWrite(MUXA1_OUT, HIGH);
    digitalWrite(STAGE4_LED, LOW);
    digitalWrite(STAGE8_LED, LOW);
    digitalWrite(STAGE12_LED, LOW);
    digitalWrite(STAGE16_LED, HIGH);
  }
  //updateScreen();
}

void updatewaveform(boolean announce) {
  switch (waveformstr) {
    case 0:
      StratusLFOWaveform = "Sawtooth Up";
      break;

    case 1:
      StratusLFOWaveform = "Sawtooth Dn";
      break;

    case 2:
      StratusLFOWaveform = "Squarewave";
      break;

    case 3:
      StratusLFOWaveform = "Triangle";
      break;

    case 4:
      StratusLFOWaveform = "Sinewave";
      break;

    case 5:
      StratusLFOWaveform = "Sweeps";
      break;

    case 6:
      StratusLFOWaveform = "Lumps";
      break;

    case 7:
      StratusLFOWaveform = "Random";
      break;
  }
  if (announce) {
    showCurrentParameterPage("LFO Wave", StratusLFOWaveform);
  }
}

void myControlChange(byte channel, byte control, int value) {

  switch (control) {

    case CCLfoRate:
      lforatestr = LFOTEMPO[value / 8];  // for display
      lforate = value;
      updateLfoRate(1);
      break;

    case CCLfoDepth:
      lfodepthstr = value / 8;  // for display
      lfodepth = value;
      updateLfoDepth(1);
      break;

    case CCfeedback:
      feedbackstr = value / 8;  // for display
      feedback = value;
      updatefeedback(1);
      break;

    case CCstage4:
      updatestage(1);
      break;

    case CCstage8:
      updatestage(1);
      break;

    case CCstage12:
      updatestage(1);
      break;

    case CCstage16:
      updatestage(1);
      break;

    case CCwaveform:
      waveformstr = value >> 7;  // for display
      waveform = value;
      updatewaveform(1);
      break;
  }
}

void checkMux() {

  mux1Read = analogRead(MUX1_S);
  if (mux1Read > (mux1ValuesPrev + QUANTISE_FACTOR) || mux1Read < (mux1ValuesPrev - QUANTISE_FACTOR)) {

    mux1ValuesPrev = mux1Read;
    myControlChange(midiChannel, CCLfoRate, mux1Read);
  }

  mux2Read = analogRead(MUX2_S);
  if (mux2Read > (mux2ValuesPrev + QUANTISE_FACTOR) || mux2Read < (mux2ValuesPrev - QUANTISE_FACTOR)) {

    mux2ValuesPrev = mux2Read;
    myControlChange(midiChannel, CCLfoDepth, mux2Read);
  }

  mux3Read = analogRead(MUX3_S);
  if (mux3Read > (mux3ValuesPrev + QUANTISE_FACTOR) || mux3Read < (mux3ValuesPrev - QUANTISE_FACTOR)) {

    mux3ValuesPrev = mux3Read;
    myControlChange(midiChannel, CCfeedback, mux3Read);
  }

  mux4Read = analogRead(MUX4_S);
  if (mux4Read > (mux4ValuesPrev + QUANTISE_FACTOR) || mux4Read < (mux4ValuesPrev - QUANTISE_FACTOR)) {

    mux4ValuesPrev = mux4Read;
    myControlChange(midiChannel, CCwaveform, mux4Read);
  }
}

void writeDemux() {
  MCP4922_write(DAC_CS1, (int(lforate * 4)), (int(lfodepth * 3)));
  MCP4922_write(DAC_CS2, (int(feedback * 1.6)), (int(waveform * 4)));
}

void reinitialiseToPanel() {
  //This sets the current patch to be the same as the current hardware panel state - all the pots
  //The four button controls stay the same state
  //This reinialises the previous hardware values to force a re-read
  startFirstTimer();
  mux1ValuesPrev = RE_READ;
  mux2ValuesPrev = RE_READ;
  mux3ValuesPrev = RE_READ;
  mux4ValuesPrev = RE_READ;

  patchName = INITPATCHNAME;
  showPatchPage("Initial", "Panel Settings");
}

void checkSwitches() {

  Stage4_Switch.update();
  if (Stage4_Switch.numClicks() == 1) {
    if (!stage4) {
      stage4 = 1;
      stage8 = 0;
      stage12 = 0;
      stage16 = 0;
      myControlChange(midiChannel, CCstage4, stage4);
    }
  }

  Stage8_Switch.update();
  if (Stage8_Switch.numClicks() == 1) {
    if (!stage8) {
      stage4 = 0;
      stage8 = 1;
      stage12 = 0;
      stage16 = 0;
      myControlChange(midiChannel, CCstage8, stage8);
    }
  }

  Stage12_Switch.update();
  if (Stage12_Switch.numClicks() == 1) {
    if (!stage12) {
      stage4 = 0;
      stage8 = 0;
      stage12 = 1;
      stage16 = 0;
      myControlChange(midiChannel, CCstage12, stage12);
    }
  }

  Stage16_Switch.update();
  if (Stage16_Switch.numClicks() == 1) {
    if (!stage16) {
      stage4 = 0;
      stage8 = 0;
      stage12 = 0;
      stage16 = 1;
      myControlChange(midiChannel, CCstage16, stage16);
    }
  }

  saveButton.update();
  if (saveButton.held()) {
    switch (state) {
      case PARAMETER:
      case PATCH:
        state = DELETE;
        updateScreen();
        break;
    }
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        if (patches.size() < PATCHES_LIMIT) {
          resetPatchesOrdering();  //Reset order of patches from first patch
          patches.push({ patches.size() + 1, INITPATCHNAME });
          state = SAVE;
        }
        updateScreen();
        break;
      case SAVE:
        //Save as new patch with INITIALPATCH name or overwrite existing keeping name - bypassing patch renaming
        patchName = patches.last().patchName;
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patches.last().patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        updateScreen();
        break;
      case PATCHNAMING:
        if (renamedPatch.length() > 0) patchName = renamedPatch;  //Prevent empty strings
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        updateScreen();
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
    updateScreen();  //Hack
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        settingsValueIndex = getCurrentIndex(settingsOptions.first().currentIndex);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGS);
        state = SETTINGS;
        updateScreen();
        break;
      case SETTINGS:
        settingsOptions.push(settingsOptions.shift());
        settingsValueIndex = getCurrentIndex(settingsOptions.first().currentIndex);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGS);
        //updateScreen();
      case SETTINGSVALUE:
        //Same as pushing Recall - store current settings item and go back to options
        settingsHandler(settingsOptions.first().value[settingsValueIndex], settingsOptions.first().handler);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGS);
        state = SETTINGS;
        updateScreen();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
    updateScreen();  //Hack
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        updateScreen();
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        updateScreen();
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGS:
        state = PARAMETER;
        updateScreen();
        break;

      case SETTINGSVALUE:
        settingsValueIndex = getCurrentIndex(settingsOptions.first().currentIndex);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGS);
        state = SETTINGS;
        updateScreen();
        break;
    }
  }

  //Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    //If Recall button held, return to current patch setting
    //which clears any changes made
    state = PATCH;
    //Recall the current patch
    patchNo = patches.first().patchNo;
    recallPatch(patchNo);
    state = PARAMETER;
    updateScreen();  //Hack
  } else if (recallButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = RECALL;  //show patch list
        updateScreen();
        break;
      case RECALL:
        state = PATCH;
        //Recall the current patch
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        updateScreen();
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 12)  //actually 12 chars
        {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        updateScreen();
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGS:
        //Choose this option and allow value choice
        settingsValueIndex = getCurrentIndex(settingsOptions.first().currentIndex);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGSVALUE);
        state = SETTINGSVALUE;
        updateScreen();
        break;
      case SETTINGSVALUE:
        //Store current settings item and go back to options
        settingsHandler(settingsOptions.first().value[settingsValueIndex], settingsOptions.first().handler);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGS);
        state = SETTINGS;
        updateScreen();
        break;
    }
  }
}

void checkEncoder() {
  //Encoder works with relative inc and dec values
  //Detent encoder goes up in 4 steps, hence +/-3

  long encRead = encoder.read();
  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.push(patches.shift());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case RECALL:
        patches.push(patches.shift());
        updateScreen();
        break;
      case SAVE:
        patches.push(patches.shift());
        updateScreen();
        break;
      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;  //Wrap around
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        updateScreen();
        break;
      case DELETE:
        patches.push(patches.shift());
        updateScreen();
        break;
      case SETTINGS:
        settingsOptions.push(settingsOptions.shift());
        settingsValueIndex = getCurrentIndex(settingsOptions.first().currentIndex);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGS);
        updateScreen();
        break;
      case SETTINGSVALUE:
        if (settingsOptions.first().value[settingsValueIndex + 1] != '\0')
          showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[++settingsValueIndex], SETTINGSVALUE);
        updateScreen();
        break;
    }
    encPrevious = encRead;
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.unshift(patches.pop());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case RECALL:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case SAVE:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case PATCHNAMING:
        if (charIndex == -1)
          charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        updateScreen();
        break;
      case DELETE:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case SETTINGS:
        settingsOptions.unshift(settingsOptions.pop());
        settingsValueIndex = getCurrentIndex(settingsOptions.first().currentIndex);
        showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[settingsValueIndex], SETTINGS);
        updateScreen();
        break;
      case SETTINGSVALUE:
        if (settingsValueIndex > 0)
          showSettingsPage(settingsOptions.first().option, settingsOptions.first().value[--settingsValueIndex], SETTINGSVALUE);
        updateScreen();
        break;
    }
    encPrevious = encRead;
  }
}

void loop() {
  updateScreen();
  stopClockPulse();
  checkSwitches();
  checkEncoder();
  checkForChanges();
  checkMux();
  writeDemux();
  MIDI.read();
}

void checkForChanges() {

  if (clocksource != oldclocksource) {
    switch (clocksource) {
      case 0:
        digitalWrite(MIDICLOCK_SELECT, LOW);
        break;

      case 1:
        digitalWrite(MIDICLOCK_SELECT, LOW);
        break;

      case 2:
        digitalWrite(MIDICLOCK_SELECT, HIGH);
        break;
    }
    oldclocksource = clocksource;
  }
}
