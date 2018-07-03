
#include <Wire.h> // Include the I2C library (required)
#include <SoftwareSerial.h>
#include <SparkFun_Tlc5940.h>
#include <SparkFunSX1509.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Soundboard.h>
#include <And_NeutrinoWandBarGraph.h>
#include <And_RGBLed.h>

#define POWER_BTN 52
#define ACTIVATE_BTN 53
#define INTENSIFY_BTN 54
#define MODE_BTN 55
#define LEVER_BTN 56
#define STRIP_N 16
#define STRIP_PIN 22

const int SX1509_BG_LED_1 = 0;
const int SX1509_BG_LED_2 = 1;
const int SX1509_BG_LED_3 = 2;
const int SX1509_BG_LED_4 = 3;
const int SX1509_BG_LED_5 = 4;
const int SX1509_BG_LED_6 = 5;
const int SX1509_BG_LED_7 = 6;
const int SX1509_BG_LED_8 = 7;
const int SX1509_BG_LED_9 = 8;
const int SX1509_BG_LED_10 = 9;
const int SX1509_BG_LED_11 = 10;
const int SX1509_BG_LED_12 = 11;
const int SX1509_BG_LED_13 = 12;
const int SX1509_BG_LED_14 = 13;
const int SX1509_BG_LED_15 = 14;

#define ACT 57
#define SFX_RST 4
#define SFX_TX 5
#define SFX_RX 6

#define PACK_MODE_STREAM 1
#define PACK_MODE_STASIS 2
#define PACK_MODE_SLIME 3

#define CYCLOTRON_0_PIN_R 11
#define CYCLOTRON_0_PIN_G 10
#define CYCLOTRON_0_PIN_B 9

#define CYCLOTRON_1_PIN_R 1
#define CYCLOTRON_1_PIN_G 1
#define CYCLOTRON_1_PIN_B 1

#define CYCLOTRON_2_PIN_R 1
#define CYCLOTRON_2_PIN_G 1
#define CYCLOTRON_2_PIN_B 1

#define CYCLOTRON_3_PIN_R 1
#define CYCLOTRON_3_PIN_G 1
#define CYCLOTRON_3_PIN_B 1

Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_N, STRIP_PIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);

const byte SX1509_ADDRESS = 0x3E;
SX1509 bargraphPinsIO;

int bgPins[15] = {
  SX1509_BG_LED_1, SX1509_BG_LED_2, SX1509_BG_LED_3, SX1509_BG_LED_4, SX1509_BG_LED_5,
  SX1509_BG_LED_6, SX1509_BG_LED_7, SX1509_BG_LED_8, SX1509_BG_LED_9, SX1509_BG_LED_10,
  SX1509_BG_LED_11, SX1509_BG_LED_12, SX1509_BG_LED_13, SX1509_BG_LED_14, SX1509_BG_LED_15,
};

And_NeutrinoWandBarGraph bargraph = And_NeutrinoWandBarGraph();

/**
   11-character file name (8.3 without the dot).
   If the filename is shorter than 8 characters, fill the characters
   @see https://learn.adafruit.com/adafruit-audio-fx-sound-board/serial-audio-control
*/
char packBootTrack[]      = "T00     WAV";
char packHumTrack[]       = "T01     WAV";
char fireStartTrack[]     = "T02     WAV";
char fireStart2Track[]    = "T12     WAV";
char fireStart3Track[]    = "T13     WAV";
char fireLoopTrack[]      = "T03     WAV";
char fireEndTrack[]       = "T04     WAV";
char fireEndTrack2[]      = "T11     WAV";
char stasisStartTrack[]   = "T05     WAV";
char stasisLoopTrack[]    = "T06     WAV";
char stasisEndTrack[]     = "T07     WAV";
char slimeStartTrack[]    = "T08     WAV";
char slimeLoopTrack[]     = "T09     WAV";
char slimeEndTrack[]      = "T10     WAV";
char modeSwitchTrack[]    = "T14     WAV";
char maxFireStartTrack[]  = "T15     WAV";
char maxFireLoopTrack[]   = "T16     WAV";
char maxFireEndTrack[]    = "T17     WAV";
char shutdownTrack[]      = "T18     WAV";
char powerOffTrack[]      = "T19     WAV";

bool invert;
bool hasBooted;
bool justBooted;
bool isShuttingDown;

bool power_switch;
bool activate_switch;
bool intensify_switch;
bool mode_switch;
bool lever_switch;

unsigned long bootStartTime;
unsigned long shutdownStartTime;

uint32_t neopixel_white;
uint32_t neopixel_black;
uint32_t neopixel_red;
uint32_t neopixel_blue;
uint32_t neopixel_green;

And_RGBLed cyclotron0 = And_RGBLed(AND_COMMON_ANODE, CYCLOTRON_0_PIN_R, CYCLOTRON_0_PIN_G, CYCLOTRON_0_PIN_B);
And_RGBLed cyclotron1 = And_RGBLed(AND_COMMON_ANODE, CYCLOTRON_1_PIN_R, CYCLOTRON_1_PIN_G, CYCLOTRON_1_PIN_B);
And_RGBLed cyclotron2 = And_RGBLed(AND_COMMON_ANODE, CYCLOTRON_2_PIN_R, CYCLOTRON_2_PIN_G, CYCLOTRON_2_PIN_B);
And_RGBLed cyclotron3 = And_RGBLed(AND_COMMON_ANODE, CYCLOTRON_3_PIN_R, CYCLOTRON_3_PIN_G, CYCLOTRON_3_PIN_B);

int currentCyclotronLight = 0;

int packMode;
int powerBootStripMax;
int powerLEDIndex;
unsigned long powerNextTimeToUpdate;

unsigned long timeActivateStart;

void resetStripLED() {
  for (uint32_t i = 0; i < STRIP_N; i++) {
    strip.setPixelColor(i, neopixel_black);
  }
  strip.show();
}

bool powerBootUpSequence(long currentTime, long startTime) {

  int _speed = 30.0;
  int inverse;

  if (currentTime == startTime) {
    powerBootStripMax = STRIP_N;
    resetStripLED();
    powerLEDIndex = 0;
    powerNextTimeToUpdate = currentTime + _speed;

    playSoundEffect(packBootTrack, true);

    return false;
  }

  if (currentTime >= powerNextTimeToUpdate) {
    powerLEDIndex++;
    if (powerLEDIndex > powerBootStripMax) {
      powerLEDIndex = 0;
    }
    powerNextTimeToUpdate = currentTime + _speed;
  }

  for (uint32_t i = 0; i < STRIP_N; i++) {

    inverse = (STRIP_N - i) - 1;

    if (i == powerLEDIndex || i >= powerBootStripMax) {
      strip.setPixelColor(inverse, neopixel_white);
    } else {
      strip.setPixelColor(inverse, neopixel_black);
    }

  }
  strip.show();

  if (powerLEDIndex == (powerBootStripMax - 1)) {
    powerBootStripMax--;
  }

  return powerBootStripMax <= 1;
}

void powerCellCycle(long currentTime, bool _init) {

  int _speed = 30.0;

  if (_init) {
    powerLEDIndex = STRIP_N;
    powerNextTimeToUpdate = currentTime + _speed;
    justBooted = false;
  }

  if (currentTime >= powerNextTimeToUpdate) {
    powerLEDIndex++;
    if (powerLEDIndex > STRIP_N) {
      powerLEDIndex = 0;
    }
    powerNextTimeToUpdate = currentTime + _speed;
  }

  for (uint32_t i = 0; i < STRIP_N; i++) {

    if (i <= powerLEDIndex) {
      strip.setPixelColor(i, neopixel_white);
    } else {
      strip.setPixelColor(i, neopixel_black);
    }

  }
  strip.show();
}

bool powerCellShutdown(long currentTime, long startTime) {

  long duration = 1500.0;
  int difference = currentTime - startTime;
  float ratio = (float) difference / duration;
  int n = 255.0 - (255.0 * ratio);
  uint32_t color = strip.Color(n, n, n);

  for (uint32_t i = 0; i < STRIP_N; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();

  return n > 0;
}




// Fill the dots one after the other with a color
void lightStripLED(uint32_t n, uint32_t color) {
  strip.setPixelColor(n, color);
  strip.show();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(POWER_BTN, INPUT_PULLUP);
  digitalWrite(POWER_BTN, HIGH);

  pinMode(ACTIVATE_BTN, INPUT_PULLUP);
  digitalWrite(ACTIVATE_BTN, HIGH);

  pinMode(INTENSIFY_BTN, INPUT_PULLUP);
  digitalWrite(INTENSIFY_BTN, HIGH);

  pinMode(MODE_BTN, INPUT_PULLUP);
  digitalWrite(MODE_BTN, HIGH);

  pinMode(ACT, INPUT);

  if (! bargraphPinsIO.begin(SX1509_ADDRESS)) {
    //    // If we failed to communicate, turn the pin 13 LED on
    //    //while (1)
    //    //  ; // And loop forever.
  }

  bargraph.init(bargraphPinsIO, bgPins, sizeof(bgPins) / sizeof(int));
  bargraph.setPowerLevel(3);
  bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_NOMINAL);

  power_switch = false;
  activate_switch = false;
  intensify_switch = false;
  mode_switch = false;
  lever_switch = false;

  hasBooted = false;

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  neopixel_red = strip.Color(255, 0, 0);
  neopixel_green = strip.Color(0, 255, 0);
  neopixel_blue = strip.Color(0, 0, 255);
  neopixel_white = strip.Color(255, 255, 255);
  neopixel_black = strip.Color(0, 0, 0); // off
}

bool switchOn(int _switch) {
  return digitalRead(_switch) == 0;
}

void playSoundEffect(char* track, bool _stop) {

  if (_stop) {
    sfx.stop();
  }

  sfx.playTrack(track);
}

void cyclotronAnimate(int in, int out) {

  //  switch (in) {
  //    case 0:
  //    break;
  //  }

}

void cyclotronLight(int i, int r, int g, int b) {

}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis();
  unsigned long firingPeriod;

  // And_NeutrinoWandBarGraph is smart enough to know that it only
  // needs to begin() once, so it is safe here in loop().
  bargraph.begin(currentTime);

  if (switchOn(ACTIVATE_BTN)) {
    if (! activate_switch) {
      timeActivateStart = currentTime;
      activate_switch = true;
    }

    firingPeriod = (currentTime - timeActivateStart);

    /**
       @todo
       Add a "venting" sequence to the bargraph?
    */

    if (firingPeriod > 5000 && firingPeriod < 10999) {
      bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_NOMINAL);
    } else if (firingPeriod > 11000 && firingPeriod < 19999) {
      bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_SEMINAL);
    } else if (firingPeriod > 20000) {
      bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_EXTREME);
    } else {
      bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_MINIMAL);
    }

    bargraph.activate(currentTime);
  } else {
    activate_switch = false;
    bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_IDLE);
    bargraph.idle(currentTime);
  }

  if (switchOn(POWER_BTN)) {

    if (hasBooted) {
      powerCellCycle(currentTime, justBooted);

      switch (currentCyclotronLight) {
        case 0:
          cyclotron0.setColor(0, 255, 255);
          break;
        case 1:
          cyclotron1.setColor(255, 0, 0);
          break;
        case 2:
          cyclotron2.setColor(255, 0, 0);
          break;
        case 3:
          cyclotron3.setColor(255, 0, 0);
          break;
      }

    } else {

      if (! power_switch) {
        bootStartTime = currentTime;
        power_switch = true;
      }

      hasBooted = powerBootUpSequence(currentTime, bootStartTime);

      if (hasBooted) {
        justBooted = true;
      }
    }

  } else {
    if (power_switch) {
      shutdownStartTime = currentTime;
      isShuttingDown = true;
      power_switch = false;
      hasBooted = false;
    }

    if (isShuttingDown) {
      isShuttingDown = powerCellShutdown(currentTime, shutdownStartTime);
    }
    // @TODO
    // power down
  }
}

