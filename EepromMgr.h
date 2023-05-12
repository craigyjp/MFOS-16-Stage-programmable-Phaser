#include <EEPROM.h>

#define EEPROM_MIDI_CH 0
#define EEPROM_ENCODER_DIR 1
#define EEPROM_CLOCKSOURCE 2

int getMIDIChannel() {
  byte midiChannel = EEPROM.read(EEPROM_MIDI_CH);
  if (midiChannel < 0 || midiChannel > 16) midiChannel = MIDI_CHANNEL_OMNI;//If EEPROM has no MIDI channel stored
  return midiChannel;
}

void storeMidiChannel(byte channel)
{
  EEPROM.update(EEPROM_MIDI_CH, channel);
}

boolean getEncoderDir() {
  byte ed = EEPROM.read(EEPROM_ENCODER_DIR); 
  if (ed < 0 || ed > 1)return true; //If EEPROM has no encoder direction stored
  return ed == 1 ? true : false;
}

void storeEncoderDir(byte encoderDir)
{
  EEPROM.update(EEPROM_ENCODER_DIR, encoderDir);
}

float getClockSource() {
  byte clocksource = EEPROM.read(EEPROM_CLOCKSOURCE);
  if (clocksource < 0  || clocksource > 2) return 0;
  return clocksource;
}

void storeClockSource(byte clocksource)
{
  EEPROM.update(EEPROM_CLOCKSOURCE, clocksource);
}

