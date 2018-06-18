#include <Adafruit_NeoPixel.h>

#define POWER_BTN 52
#define STRIP_N 16
#define STRIP_PIN 22
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_N, STRIP_PIN, NEO_GRB + NEO_KHZ800);

bool invert;
bool hasBooted;
bool power_switch;

unsigned long bootStartTime;

uint32_t neopixel_white;
uint32_t neopixel_black;

int powerBootStripMax;


void resetStripLED() {
    for (int i = 0; i < STRIP_N; i++) {
        strip.setPixelColor(i, neopixel_black);
    }
    strip.show();
}

bool bootUpSequence(long currentTime, long startTime) {
  //bootStarted = true;

  long difference;
  int ledIndex;
  int _speed = 50;
  
  if (currentTime == startTime) {
    powerBootStripMax = STRIP_N;
    resetStripLED();
    return false;
  }

  difference = currentTime - startTime;
  while (difference > (_speed * powerBootStripMax)) {
    difference -= (_speed * powerBootStripMax);
  }
  ledIndex = (difference / _speed);

  for(int i = 0; i < STRIP_N; i++) {

    if (i == ledIndex || i >= powerBootStripMax) {
      strip.setPixelColor(i, neopixel_white);
    } else {
      strip.setPixelColor(i, neopixel_black);
    }
  
  }
  strip.show();

  if (ledIndex >= powerBootStripMax) {
    powerBootStripMax--;
  }
  return powerBootStripMax <= 0;
}

void powerCellCycle() {
  
    for(uint32_t i = 0; i < STRIP_N; i++) {
      lightStripLED(i, neopixel_white);
      delay(50);
    }
  
    for(uint32_t i = 0; i < STRIP_N; i++) {
      lightStripLED(i, neopixel_black);
    }
}

// Fill the dots one after the other with a color
void lightStripLED(uint32_t n, uint32_t color) {
    strip.setPixelColor(n, color);
    strip.show();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(POWER_BTN, INPUT);
  digitalWrite(POWER_BTN, HIGH);

  power_switch = false;
  hasBooted = false;

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  neopixel_white = strip.Color(255, 255, 255); 
  neopixel_black = strip.Color(0, 0, 0); // off
}

bool switchOn(int _switch) {
  return digitalRead(_switch) == 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis();

  if (switchOn(POWER_BTN)) {

    if (hasBooted) {
      powerCellCycle();
    } else {

      if (! power_switch) {
        bootStartTime = currentTime;
        power_switch = true;
      }
      
      hasBooted = bootUpSequence(currentTime, bootStartTime);
    }

  } else {
    power_switch = false;
    hasBooted = false;
    // @TODO
    // power down
  }
}

