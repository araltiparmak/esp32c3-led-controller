# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash Commands

```bash
# Build (compile only)
arduino-cli compile \
  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc \
  --output-dir build/ \
  firmware/

# Build with explicit version
arduino-cli compile \
  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc \
  --build-property "compiler.cpp.extra_flags=-DFIRMWARE_VERSION='\"1.0.0\"'" \
  --output-dir build/ \
  firmware/

# Upload to connected device (replace port as needed)
arduino-cli upload \
  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc \
  --port /dev/cu.usbmodem* \
  --input-dir build/

# Monitor serial output
arduino-cli monitor --port /dev/cu.usbmodem* --config baudrate=115200

# List connected boards
arduino-cli board list
```

## Architecture

All firmware lives in a single file: `firmware/firmware.ino`. There are three logical sections:

**LED** — `led_clear()`, `led_set_color()`, `led_rainbow()`. FastLED drives a WS2812B strip. All LED functions call `FastLED.show()` immediately — there is no deferred render.

**WiFi** — `wifi_connect()` checks NVS (ESP32 Preferences) for saved credentials. If empty (first boot), `wifi_wps()` is called: LEDs blink purple and the device waits up to 2 minutes for a WPS button press. On success, credentials are saved to NVS via `WiFi.SSID()` / `WiFi.psk()`. All subsequent boots read from NVS — no credentials in code or config.

**OTA** — `ota_task()` runs as a FreeRTOS task (8KB stack, priority 1) so it never blocks the LED animation in `loop()`. It waits 5s after boot, then calls `ota_check()` every `OTA_CHECK_INTERVAL_MS`. `ota_check()` hits `api.github.com/repos/araltiparmak/esp32c3-led-controller/releases/latest`, compares `tag_name` against `FIRMWARE_VERSION`, finds the `.bin` asset URL, downloads and flashes via the ESP32 `Update` library. LED colors signal state: cyan = checking, blue = downloading, green flash = success, red = error.

## Releasing

```bash
git tag v1.0.1
git push origin v1.0.1
```

GitHub Actions (`.github/workflows/release.yml`) builds the firmware and attaches `firmware-v1.0.1.bin` to the release. Running devices pick it up within one `OTA_CHECK_INTERVAL_MS` cycle.

## Config

`firmware/config.h` is gitignored. Users create it by copying `firmware/config.example.h`. It defines `OTA_CHECK_INTERVAL_MS` and LED hardware settings (`LED_PIN`, `NUM_LEDS`, etc.). No WiFi credentials — those are handled by WPS and stored in NVS.

`FIRMWARE_VERSION` is injected at compile time via `-DFIRMWARE_VERSION` build flag. It defaults to `"dev"` if not set.

## Hardware

ESP32-C3 Super Mini (DUBEUYEW), WS2812B strip on GPIO 8. Arduino IDE board: **ESP32C3 Dev Module** with **USB CDC On Boot: Enabled**.
