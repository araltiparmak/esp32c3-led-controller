# ESP32-C3 LED Controller

Firmware for controlling LED strips (WS2812B/NeoPixel) using the ESP32-C3 microcontroller.

## Requirements

- [PlatformIO](https://platformio.org/)
- ESP32-C3 board
- WS2812B LED strip

## Hardware Wiring

| ESP32-C3 Pin | LED Strip |
|---|---|
| GPIO8 | Data In |
| 5V | 5V |
| GND | GND |

## Getting Started

```bash
# Clone the repo
git clone https://github.com/araltiparmak/esp32c3-led-controller.git
cd esp32c3-led-controller

# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## Configuration

Edit `include/config.h` to set:
- `LED_PIN` — data pin connected to the strip
- `NUM_LEDS` — number of LEDs in the strip
- `LED_BRIGHTNESS` — global brightness (0–255)
