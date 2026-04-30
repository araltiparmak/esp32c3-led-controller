#include <Arduino.h>
#include "led_controller.h"

void setup() {
    Serial.begin(115200);
    led_init();
    Serial.println("ESP32-C3 LED Controller ready");
}

void loop() {
    led_rainbow(2);
    delay(20);
}
