#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Wire.h> // Include the I2C library (required)
#include <SparkFun_Tlc5940.h>
#include <SparkFunSX1509.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Soundboard.h>
#include <And_NeutrinoWandBarGraph.h>
#include <And_RGBLed.h>

#define TIME_FIRING_BEFORE_OVERHEAT 25000

/**
   Connect these I/O pins to GND. Use the
   switchOn() method to abstract reading
   their state.
*/
#define POWER_BTN 32
#define SAFETY_BTN 31
#define ACTIVATE_BTN 33
#define INTENSIFY_BTN 54
#define MODE_BTN 55
#define LEVER_BTN 56
#define STRIP_N 16
#define STRIP_PIN 22
#define JEWEL_N 7
#define JEWEL_PIN 23

/**
   Pack Modes, inspired by the Video Game:
   Default (Proton) stream, Stasis stream, Slime blower
   and Maximum Proton stream (use for crossing the
   streams?)
*/
#define PROTON_MODE 0
#define SLIME_MODE 1
#define STASIS_MODE 2
#define MAX_PROTON_MODE 3

/**
   The N-Filter vent consists of a 12V fan,
   a 12V multi-directional LED lamp, and a
   9-12V e-cig kit. These are switched via
   a double Relay module.

   @TODO
   Should the fan + e-cig be synched, or
   the light + fan?
*/
#define NFILTER_FAN 46
#define NFILTER_VENT_EXHAUST 47

/**
   @TODO decide on I/O pins for the Neutrino wand
   lights.
   VENT_HAT       Top Hat light (WHITE LED) On / Off
   EAR_HAT        Orange Hat light (RGB LED) Orange / Green / Blue
   CLIPPARD_HAT   Hat light next to Clippard Minimatic (WHITE LED) On / Off
   TOP_LIGHT      Smaller top light (WHITE LED) On / Off
   SLOBLO_LIGHT   Small light near the SloBlo fusebox (RED LED) On / Off
*/
#define VENT_HAT 0
#define EAR_HAT 0
#define CLIPPARD_HAT 0
#define TOP_LIGHT 0
#define SLOBLO_LIGHT 0

#define TIP_EXTEND_DIR 11
#define TIP_EXTEND_STEP 12

/**
   We are using a SparkFun SX1509 breakout
   board to drive the Neutrino Wand bargraph,
   which is an array of 3 x 5 LED bargraphs
   since those are the very readily available.
*/
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

//# define ACT 57
#define SFX_ACT 57
#define SFX_RST 3
#define SFX_TX 14
#define SFX_RX 15

/**
   Since SX1509 breakouts can be chained, you need
   to set each board's address when communicating
   with it. The default for one board in 0x3E.
   We'll also load the boards I/O pins into an
   array so that we can ass it to our library class
   And_NeutrinoWandBarGraph
*/
const byte SX1509_ADDRESS = 0x3E;

SX1509 bargraphPinsIO;

int bgPins[15] = {
  SX1509_BG_LED_1, SX1509_BG_LED_2, SX1509_BG_LED_3, SX1509_BG_LED_4, SX1509_BG_LED_5,
  SX1509_BG_LED_6, SX1509_BG_LED_7, SX1509_BG_LED_8, SX1509_BG_LED_9, SX1509_BG_LED_10,
  SX1509_BG_LED_11, SX1509_BG_LED_12, SX1509_BG_LED_13, SX1509_BG_LED_14, SX1509_BG_LED_15,
};

And_NeutrinoWandBarGraph bargraph = And_NeutrinoWandBarGraph();

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

/**
   The power strip will be made from two Adafruit
   NeoPixel strips chained together.
*/
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_N, STRIP_PIN, NEO_GRB + NEO_KHZ800);

/**
   Use a 7 LED NeoPixel Jewel to light the tip of the
   Neutrino Wand.
*/
Adafruit_NeoPixel jewel = Adafruit_NeoPixel(JEWEL_N, JEWEL_PIN, NEO_GRB + NEO_KHZ800);

/**
   Since we are using a Mega, we will use the hardware
   Serial1 on pins 18 & 19 to control the Adafruit
   Audio FX Soundboard. If your Arduino doesn't have
   hardware serial avaiable, you'll have to use the
   SoftwareSerial alternative:

   #include <SoftwareSerial.h>
   SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
*/
Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

AccelStepper stepper = AccelStepper(AccelStepper::DRIVER, TIP_EXTEND_STEP, TIP_EXTEND_DIR);

/**
   11-character file name (8.3 without the dot).
   If the filename is shorter than 8 characters,
   fill the characters.
   @see https://learn.adafruit.com/adafruit-audio-fx-sound-board/serial-audio-control

   Here are the Sound Effect files required,
   you'll need a 16Mb Adafruit Audio FX to fit
   these many sounds. The SFX themselves can
   be sourced from the Video Game. Use .WAV to
   minimise the delay from decoding OGG.
*/
char packBootTrack[]      = "T00     WAV";
char packHumTrack[]       = "T01     WAV";
char fireStartTrack[]     = "T02     WAV";
char fireStart2Track[]    = "T12     WAV";
char fireStart3Track[]    = "T13     WAV";
char fireLoopTrack[]      = "T03     WAV";
char fireEndTrack[]       = "T04     WAV";
char fireEnd2Track[]      = "T11     WAV";
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
char fullOverheatTrack[]  = "T20     WAV";

int packFiringMode;

/**
   @desc Return a randomised named Stream Start track
         from the list available.
*/
char* getRandomStreamStartTrack() {
  int r = random(0, 3);
  switch (r) {
    case 1:
      return fireStart2Track;
    case 2:
      return fireStart3Track;
    default: // 0, because random()'s MAX is exclusive
      return fireStartTrack;
  }
}

/**
   @desc Return a randomised named Stream Loop track
         from the list available.
*/
char* getRandomStreamTrack() {
  // @TODO:
  // Currently only using one fireLoop SFX
  return fireLoopTrack;
}

/**
   @desc Return a randomised named Stream End track
         from the list available.
*/
char* getRandomStreamEndTrack() {
  int r = random(0, 2);
  switch (r) {
    case 1:
      return fireEnd2Track;
    default: // 0, because random()'s MAX is exclusive
      return fireEndTrack;
  }
}

/**
   @desc Function to call to open a firing stream. Will
         always take into consideration the pack's
         current Firing Mode and play the appropriate
         sound effect.
*/
void openFiringStream(long timeSinceStart) {

  long now = millis();
  uint32_t remain, total;

  if (millis == now) {
    switch (packFiringMode) {
      case SLIME_MODE:
        playSoundEffect(slimeStartTrack, true);
        break;
      case STASIS_MODE:
        playSoundEffect(stasisStartTrack, true);
        break;
      case MAX_PROTON_MODE:
        playSoundEffect(maxFireStartTrack, true);
        break;
      default: // PROTON MODE
        /**
           If we've only just started firing, play a randomised
           Stream start up sound effect.
        */
        playSoundEffect(getRandomStreamStartTrack(), true);
    }
  } else {

    /**
       If we are unable to query the play duration, OR
       the effect is about to stop, queue up the next
       sound effect.
    */
    if (! sfx.trackSize(&remain, &total) || total - remain <= 3) {
      switch (packFiringMode) {
        case STASIS_MODE:
          playSoundEffect(stasisLoopTrack, true);
          break;
        case SLIME_MODE:
          playSoundEffect(stasisLoopTrack, true);
          break;
        case MAX_PROTON_MODE:
          playSoundEffect(maxFireLoopTrack, true);
          break;
        default:
          /**
             Any of the stream loops should seamlessly loop
             with any other, so we don't need to keep track
             of which track is randomly picked. This should
             make for a more organic stream effect.
          */
          playSoundEffect(getRandomStreamTrack(), false);
      }
    }
  }

}


bool invert;
bool hasBooted;
bool justBooted;
bool isShuttingDown;
bool isOverheating;

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
int powerBootStripMax;
int powerLEDIndex;
unsigned long powerNextTimeToUpdate;

unsigned long timeActivateStart;

int soundTest = 1;

void resetStripLED() {
  for (uint32_t i = 0; i < STRIP_N; i++) {
    strip.setPixelColor(i, neopixel_black);
  }
  strip.show();
}

/**
   @desc During the boot sequence the animation of the
         power strip is different. Each LED will "fall"
         from one end to the other, producing a filling
         stack effect.
   @return bool Returns TRUE once the full animation is
         complete, otherwise FALSE.
*/
bool powerBootUpSequence(long currentTime, long startTime) {

  int _speed = 30.0;
  int inverse;

  if (currentTime == startTime) {
    powerBootStripMax = STRIP_N;
    resetStripLED();
    powerLEDIndex = 0;
    powerNextTimeToUpdate = currentTime + _speed;

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

/**
   @desc During a power cell cycle, each light will light
         in a chase until all LEDs are lit, then they
         are immediately all reset to OFF.
*/
void powerCellCycle(long currentTime, bool _init) {
  \

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

/**
   @desc On power down, the animation on the 16 LED
         NeoPixel strip is thus:
         All lights are lit at once, and then all
         fade to off over the duration variable.
   @return bool Returns TRUE once the animation is complete,
         otherwise FALSE.
*/
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




/**
   @desc Fill the dots one after the other with a color
   @deprecated?
*/
void lightStripLED(uint32_t n, uint32_t color) {
  strip.setPixelColor(n, color);
  strip.show();
}

/**
   @desc Reset the states of the N-Filter I/O
*/
void NFilterReset() {
  digitalWrite(NFILTER_FAN, HIGH);
  digitalWrite(NFILTER_VENT_EXHAUST, HIGH);
}

/**
   @desc Reset the states of the N-Filter I/O
*/
void NFilterVent() {
  digitalWrite(NFILTER_FAN, LOW);
  digitalWrite(NFILTER_VENT_EXHAUST, LOW);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // This is essential in communicating with the
  // Adafruit Audio FX board.
  Serial1.begin(9600);

  if (!sfx.reset()) {
    //  Serial.println("SFX board Not found");
    while (1);
  }

  soundTest = 1;

  pinMode(NFILTER_FAN, OUTPUT);
  pinMode(NFILTER_VENT_EXHAUST, OUTPUT);
  NFilterReset();

  /**
     Initialize the I/O pins for switches and
     controls on the Neutrino Wand.
  */
  pinMode(POWER_BTN, INPUT_PULLUP);
  digitalWrite(POWER_BTN, HIGH);

  pinMode(ACTIVATE_BTN, INPUT_PULLUP);
  digitalWrite(ACTIVATE_BTN, HIGH);

  pinMode(INTENSIFY_BTN, INPUT_PULLUP);
  digitalWrite(INTENSIFY_BTN, HIGH);

  pinMode(MODE_BTN, INPUT_PULLUP);
  digitalWrite(MODE_BTN, HIGH);

  /**
     Initialize the I/O pins we'll be using for the
     Neutrino Wand LEDs / hat lights
  */
  pinMode(EAR_HAT, OUTPUT);
  digitalWrite(EAR_HAT, LOW);

  pinMode(TOP_LIGHT, OUTPUT);
  digitalWrite(TOP_LIGHT, LOW);

  pinMode(VENT_HAT, OUTPUT);
  digitalWrite(VENT_HAT, LOW);

  pinMode(CLIPPARD_HAT, OUTPUT);
  digitalWrite(CLIPPARD_HAT, LOW);

  /**
     SLOBLO Light will always be lit, so it can be used
     to tell if power is being supplied to the arduino
     even if the in-universe power button is OFF
  */
  pinMode(SLOBLO_LIGHT, OUTPUT);
  digitalWrite(SLOBLO_LIGHT, HIGH);

  pinMode(TIP_EXTEND_STEP, OUTPUT);
  pinMode(TIP_EXTEND_DIR, OUTPUT);
  //digitalWrite(TIP_EXTEND_DIR, LOW);
  digitalWrite(TIP_EXTEND_DIR, HIGH);

  Tlc.init();

  /*
    stepper.setMaxSpeed(1000);
    stepper.setSpeed(50);
    //stepper.moveTo(5000);
  */
  //  pinMode(ACT, INPUT);

  //pinMode(SFX_RST, OUTPUT);
  //digitalWrite(SFX_RST, LOW);

  if (! bargraphPinsIO.begin(SX1509_ADDRESS)) {
    //    // If we failed to communicate, turn the pin 13 LED on
    //    //while (1)
    //    //  ; // And loop forever.
  }

  bargraph.init(bargraphPinsIO, bgPins, sizeof(bgPins) / sizeof(int));
  bargraph.setPowerLevel(3);
  bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_NOMINAL);

  /**
     Variables to track whether an event has just happened
     this loop() cycle.
  */
  power_switch = false;
  activate_switch = false;
  intensify_switch = false;
  mode_switch = false;
  lever_switch = false;

  hasBooted = false;
  isOverheating = false;

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

  if (sfx.playTrack(track)) {
    sfx.unpause();
  }
}

void animateStreamTip(long currentTime) {

}

void animateStasisTip(long currentTime) {

}

void animateSlimeTip(long currentTime) {

}

void cyclotronAnimate(int in, int out) {
  // @TODO
}

void cyclotronLight(int i, int r, int g, int b) {
  // @TODO
}

int extend = 0;

/**
   @desc Begin the overheat warning routine. Return
         TRUE if proton pack has overheated and
         rebooted, otherwise FALSE.
   @param long firingPeriod
   @return bool
*/
bool overheatWarning(long firingPeriod) {
  // @TODO

  uint32_t remain, total, progress;
  long warningPeriod = TIME_FIRING_BEFORE_OVERHEAT - firingPeriod;

  if (! isOverheating) {
    NFilterReset();
    playSoundEffect(fullOverheatTrack, true);
    progress = 0;
  } else if (sfx.trackSize(&remain, &total)) {
    progress = total - remain;
  }

  isOverheating = true;

  /**
     In warning cycle, flash the hat light next to the
     vent on and off rappidly every 500 ms
  */
  if (warningPeriod % 1000 >= 500) {
    // Vent Hat light OFF
    digitalWrite(VENT_HAT, LOW);
  } else {
    // Vent Hat light ON
    digitalWrite(VENT_HAT, HIGH);
  }


  if (progress > 10000) {
    // 10 Seconds in to the soundeffect...
    // If SFX has reached reset point, start NFilter Vent
    NFilterVent();
  }


  if (remain <= 1) {
    // Overheat and Vent cycle complete, return TRUE to
    // signal reset
    NFilterReset();
    isOverheating = false;
    return true;
  }

  return false;
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis();
  unsigned long firingPeriod;

  // if (xxx >= 4095) {
  //  xxx = 0;
  // }
  //xxx = 4095;
  // Tlc.clear();
  // Tlc.set(0s, xxx);
  // Tlc.update();
  // xxx++;
  //delay(5000);
  /**
  */

  /*
    //stepper.run();
    //stepper.runSpeed();

    //if (extend > 2000) {
    //  digitalWrite(TIP_EXTEND_DIR, HIGH);
    //} else {
      digitalWrite(TIP_EXTEND_DIR, LOW);
    //}
    if (extend < 3300) {
      digitalWrite(TIP_EXTEND_STEP, HIGH);
      delayMicroseconds(1);
      digitalWrite(TIP_EXTEND_STEP, LOW);
      delayMicroseconds(1);
      extend++;
    }
  */

  /**
     And_NeutrinoWandBarGraph is smart enough to know that
     it only needs to begin() once, so it is safe here
     in loop().
  */
  bargraph.begin(currentTime);


  /**
     Check the switch position of the Activate Button to
     see if the Neutrino Wand needs to fire a stream
  */
  if (switchOn(ACTIVATE_BTN)) {
    if (! activate_switch) {
      timeActivateStart = currentTime;
      activate_switch = true;
    }

    firingPeriod = (currentTime - timeActivateStart);

    openFiringStream(firingPeriod);

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

    /**
       We are going to trigger the overheat warning routine
    */
    if (isOverheating || firingPeriod > TIME_FIRING_BEFORE_OVERHEAT) {

      if (overheatWarning(firingPeriod)) {

        activate_switch = false;

      }

    }

  } else {
    activate_switch = false;
    bargraph.setSpeed(And_NeutrinoWandBarGraph::SPEED_IDLE);
    bargraph.idle(currentTime);
  }

  //playSoundEffect(packHumTrack, false);
  //delay(5000);

  /**
     Check the position of the Power Button to see if
     the Proton Pack is powered
  */
  if (switchOn(POWER_BTN)) {

    digitalWrite(CLIPPARD_HAT, HIGH);
    digitalWrite(TOP_LIGHT, HIGH);
    digitalWrite(VENT_HAT, HIGH);
    digitalWrite(EAR_HAT, HIGH);

    if (hasBooted) {
      powerCellCycle(currentTime, justBooted);

      playSoundEffect(packHumTrack, false);

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
        Serial.println("Power Switch ON");
        playSoundEffect(packBootTrack, false);
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

      digitalWrite(CLIPPARD_HAT, LOW);
      digitalWrite(TOP_LIGHT, LOW);
      digitalWrite(VENT_HAT, LOW);
      digitalWrite(EAR_HAT, LOW);
    }

    if (isShuttingDown) {
      isShuttingDown = powerCellShutdown(currentTime, shutdownStartTime);
    }
    // @TODO
    // power down
  }
}

