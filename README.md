# ESP32-C3 LED Controller

Rainbow and color effects for WS2812B LED strips, with automatic OTA updates from GitHub.

## Hardware

- ESP32-C3 Super Mini (DUBEUYEW)
- WS2812B LED strip (or compatible: WS2811, SK6812)
- USB-C cable

## Wiring

```
ESP32-C3 Super Mini     LED Strip
-------------------     ---------
GPIO 8    --------->    DATA IN
GND       --------->    GND
5V        --------->    5V (or use external power supply for long strips)
```

> For strips longer than 30 LEDs, use an external 5V power supply and connect GND to ESP32 GND.

## First-time Setup

### 1. Install Arduino IDE
Download from [arduino.cc](https://www.arduino.cc/en/software)

### 2. Add ESP32 board support
1. Open Arduino IDE → **Preferences**
2. Add this URL to "Additional boards manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**, search `esp32`, install **esp32 by Espressif**

### 3. Install FastLED library
**Sketch → Include Library → Manage Libraries** → search `FastLED` → install

### 4. Configure
```bash
cd firmware/
cp config.example.h config.h
```
Open `config.h` and fill in your WiFi credentials and GitHub info.

### 5. Select board settings
- **Tools → Board → ESP32 Arduino → ESP32C3 Dev Module**
- **Tools → USB CDC On Boot → Enabled**

### 6. Upload
Connect ESP32-C3 via USB-C, select the port, click **Upload**.

After the first upload, the device will update itself automatically from GitHub — no USB needed.

## Configuration

Edit `firmware/config.h`:

| Setting | Description |
|---|---|
| `WIFI_SSID` | Your WiFi network name |
| `WIFI_PASSWORD` | Your WiFi password |
| `GITHUB_USER` | Your GitHub username |
| `GITHUB_REPO` | This repository name |
| `LED_PIN` | GPIO pin connected to LED data line |
| `NUM_LEDS` | Number of LEDs in your strip |
| `LED_BRIGHTNESS` | Brightness 0-255 (keep ≤150 on USB power) |
| `COLOR_ORDER` | Try `RGB` if colors look wrong |

## Releasing a new version

```bash
git tag v1.0.1
git push origin v1.0.1
```

GitHub Actions automatically builds the firmware and creates a release. Any running ESP32 will download and install it within the hour.

## How OTA works

1. ESP32 connects to WiFi on boot
2. Checks GitHub releases API for a newer version
3. If found, downloads the `.bin` file and flashes itself
4. Reboots into the new firmware
5. Repeats the check every hour
