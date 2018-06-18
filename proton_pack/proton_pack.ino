#include <Adafruit_NeoPixel.h>

#define POWER_BTN 52
#define STRIP_N 16
#define STRIP_PIN 22
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_N, STRIP_PIN, NEO_GRB + NEO_KHZ800);

bool invert;
bool hasBooted;
bool justBooted;
bool isShuttingDown;
bool power_switch;

unsigned long bootStartTime;
unsigned long shutdownStartTime;

uint32_t neopixel_white;
uint32_t neopixel_black;
uint32_t neopixel_red;
uint32_t neopixel_blue;
uint32_t neopixel_green;

int powerBootStripMax;
int powerLEDIndex;
unsigned long powerNextTimeToUpdate;

void resetStripLED() {
    for (uint32_t i = 0; i < STRIP_N; i++) {
        strip.setPixelColor(i, neopixel_black);
    }
    strip.show();
}

bool bootUpSequence(long currentTime, long startTime) {

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

   for(uint32_t i = 0; i < STRIP_N; i++) {

    inverse = (STRIP_N - i) -1;
      
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

   for(uint32_t i = 0; i < STRIP_N; i++) {
      
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
  
  for(uint32_t i = 0; i < STRIP_N; i++) {
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

  pinMode(POWER_BTN, INPUT);
  digitalWrite(POWER_BTN, HIGH);

  power_switch = false;
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

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis();

  if (switchOn(POWER_BTN)) {

    if (hasBooted) {
      powerCellCycle(currentTime, justBooted);
    } else {

      if (! power_switch) {
        bootStartTime = currentTime;
        power_switch = true;
      }
      
      hasBooted = bootUpSequence(currentTime, bootStartTime);

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

