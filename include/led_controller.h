#pragma once

#include <FastLED.h>
#include "config.h"

void led_init();
void led_set_color(uint8_t r, uint8_t g, uint8_t b);
void led_rainbow(uint8_t speed);
void led_clear();
