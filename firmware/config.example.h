#pragma once

// =============================================
//   CONFIGURATION - Fill in your values!
//   Copy this file to config.h and edit it.
// =============================================

// --- OTA ---
#define OTA_CHECK_INTERVAL_MS 60 * 1000  // How often to check for updates (ms). 60 * 1000 = 1 min

// --- LED Strip ---
#define LED_PIN        8       // GPIO pin connected to DATA line
#define NUM_LEDS       45      // Number of LEDs in your strip
#define LED_BRIGHTNESS 80      // Brightness 0-255 (keep ≤150 on USB power)
#define LED_TYPE       WS2812B // WS2812B, WS2811, SK6812
#define COLOR_ORDER    GRB     // Try RGB if colors look wrong
