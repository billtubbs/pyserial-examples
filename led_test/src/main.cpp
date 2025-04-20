#include <Arduino.h>
#include <FastLED.h>
 
// How many leds in your strip?
#define NUM_LEDS 7

// Pin where LEDs are connected
#define DATA_PIN 6

// Define the array of leds
CRGB leds[NUM_LEDS];

void setAllLedsSameColor(CRGB color);

void setup() {
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(32);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  setAllLedsSameColor(CRGB::Green);
  FastLED.show();
  delay(700);

  digitalWrite(LED_BUILTIN, LOW);
  setAllLedsSameColor(CRGB::Black);
  FastLED.show();
  delay(300);
}

void setAllLedsSameColor(CRGB color) {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}