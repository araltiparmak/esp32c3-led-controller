#include "led_controller.h"

static CRGB leds[NUM_LEDS];
static uint8_t rainbow_hue = 0;

void led_init() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    led_clear();
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    FastLED.show();
}

void led_rainbow(uint8_t speed) {
    fill_rainbow(leds, NUM_LEDS, rainbow_hue, 7);
    FastLED.show();
    rainbow_hue += speed;
}

void led_clear() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}
