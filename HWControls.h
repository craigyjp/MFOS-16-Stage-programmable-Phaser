#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Bounce.h>
#include "TButton.h"

// #include <ADC.h>
// #include <ADC_util.h>

// ADC *adc = new ADC();

#define MUX1_S A0
#define MUX2_S A1
#define MUX3_S A2
#define MUX4_S A3

#define MUXA0_OUT 13
#define MUXA1_OUT 14

#define RECALL_SW 10
#define ENCODER_PINA 4
#define ENCODER_PINB 5

#define SDCARD 20
#define DAC_CS1 11
#define DAC_CS2 12

#define SAVE_SW 15
#define SETTINGS_SW 16
#define BACK_SW 17

#define STAGE4_SW 27
#define STAGE8_SW 26
#define STAGE12_SW 25
#define STAGE16_SW 24

#define MIDICLOCK_SELECT 7
#define CLOCK_OUT 8

#define STAGE4_LED 45
#define STAGE8_LED 44
#define STAGE12_LED 43
#define STAGE16_LED 42


#define DEBOUNCE 30
#define DEMUXCHANNELS 2

#define QUANTISE_FACTOR 19

static int mux1ValuesPrev;
static int mux2ValuesPrev;
static int mux3ValuesPrev;
static int mux4ValuesPrev;
static int mux1Read = 0;
static int mux2Read = 0;
static int mux3Read = 0;
static int mux4Read = 0;

static long encPrevious = 0;

//These are pushbuttons and require debouncing
TButton Stage4_Switch{STAGE4_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton Stage8_Switch{STAGE8_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton Stage12_Switch{STAGE12_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton Stage16_Switch{STAGE16_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};

TButton saveButton{SAVE_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton settingsButton{SETTINGS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton backButton{BACK_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton recallButton{RECALL_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION}; //On encoder
                        
Encoder encoder(ENCODER_PINA, ENCODER_PINB);        //This often needs the pins swapping depending on the encoder

void setupHardware() {

  // adc->adc0->setAveraging(16);                                          // set number of averages 0, 4, 8, 16 or 32.
  // adc->adc0->setResolution(10);                                         // set bits of resolution  8, 10, 12 or 16 bits.
  // adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED);  // change the conversion speed
  // adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  // //MUXs on ADC1
  // adc->adc1->setAveraging(16);                                          // set number of averages 0, 4, 8, 16 or 32.
  // adc->adc1->setResolution(10);                                         // set bits of resolution  8, 10, 12 or 16 bits.
  // adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED);  // change the conversion speed
  // adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  //analogWriteResolution(10);
  //analogReadResolution(10);

  //Switches

  pinMode(DAC_CS1, OUTPUT);
  pinMode(DAC_CS2, OUTPUT);

  pinMode(MUXA0_OUT, OUTPUT);
  pinMode(MUXA1_OUT, OUTPUT);

  digitalWrite(DAC_CS1, HIGH);
  digitalWrite(DAC_CS2, HIGH);

  digitalWrite(MUXA0_OUT, LOW);
  digitalWrite(MUXA1_OUT, LOW);

  pinMode(RECALL_SW, INPUT_PULLUP);  //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);

  pinMode(STAGE4_SW, INPUT_PULLUP);
  pinMode(STAGE8_SW, INPUT_PULLUP);
  pinMode(STAGE12_SW, INPUT_PULLUP);
  pinMode(STAGE16_SW, INPUT_PULLUP);

  pinMode(STAGE4_LED, OUTPUT);
  pinMode(STAGE8_LED, OUTPUT);
  pinMode(STAGE12_LED, OUTPUT);
  pinMode(STAGE16_LED, OUTPUT);

  digitalWrite(STAGE4_LED, LOW);
  digitalWrite(STAGE8_LED, LOW);
  digitalWrite(STAGE12_LED, LOW);
  digitalWrite(STAGE16_LED, LOW);

  pinMode(MIDICLOCK_SELECT, OUTPUT);
  pinMode(CLOCK_OUT, OUTPUT);
  digitalWrite(MIDICLOCK_SELECT, LOW);
  digitalWrite(CLOCK_OUT, LOW);

}
