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

**WiFi** — `wifi_connect()` reads credentials from NVS (ESP32 Preferences) on every boot. On the very first boot, credentials are read from `config.h` and written to NVS. This means OTA-downloaded binaries (built by CI with empty config.h) can still connect to WiFi using credentials stored from the original user-built flash.

**OTA** — `ota_check()` hits the GitHub Releases API (`api.github.com/repos/araltiparmak/esp32c3-led-controller/releases/latest`), compares `tag_name` against `FIRMWARE_VERSION`, downloads the `.bin` asset, and flashes via the ESP32 `Update` library. Called once on boot and then every `OTA_CHECK_INTERVAL_MS`. LED colors signal state: cyan = checking, blue = downloading, green flash = success, red = error.

## Releasing

```bash
git tag v1.0.1
git push origin v1.0.1
```

GitHub Actions (`.github/workflows/release.yml`) builds the firmware and attaches `firmware-v1.0.1.bin` to the release. Running devices pick it up within one `OTA_CHECK_INTERVAL_MS` cycle.

## Config

`firmware/config.h` is gitignored. Users create it by copying `firmware/config.example.h`. It defines WiFi credentials, `OTA_CHECK_INTERVAL_MS`, and LED hardware settings (`LED_PIN`, `NUM_LEDS`, etc.).

`FIRMWARE_VERSION` is injected at compile time via `-DFIRMWARE_VERSION` build flag. It defaults to `"dev"` if not set.

## Hardware

ESP32-C3 Super Mini (DUBEUYEW), WS2812B strip on GPIO 8. Arduino IDE board: **ESP32C3 Dev Module** with **USB CDC On Boot: Enabled**.
