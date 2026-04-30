#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_wps.h>
#include "config.h"

#if __has_include("version.h")
  #include "version.h"
#endif
#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "dev"
#endif

// =============================================
//   LED
// =============================================

CRGB leds[NUM_LEDS];
uint8_t rainbow_hue = 0;

enum Theme { RAINBOW, BREATHING, CHASE, TWINKLE, RED, GREEN, BLUE };
volatile Theme current_theme = RAINBOW;

void led_clear() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
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

void led_breathing(CRGB color) {
  static uint8_t brightness = 0;
  static int8_t direction = 1;
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  brightness += direction * 2;
  if (brightness >= 200 || brightness <= 2) direction = -direction;
  delay(10);
}

void led_chase(CRGB color) {
  static uint8_t pos = 0;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int i = 0; i < 5; i++) {
    leds[(pos + i) % NUM_LEDS] = color;
  }
  FastLED.show();
  pos = (pos + 1) % NUM_LEDS;
  delay(30);
}

void led_twinkle() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(random8(), 200, 255);
  FastLED.show();
  delay(20);
}

// =============================================
//   WiFi + WPS
// =============================================

static volatile bool wps_success = false;
static volatile bool wps_failed  = false;
static String wps_ssid = "";
static String wps_pass = "";

void wps_event_handler(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      Serial.println("WPS event: SUCCESS");
      wps_success = true;
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      Serial.println("WPS event: FAILED");
      wps_failed = true;
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      Serial.println("WPS event: TIMEOUT");
      wps_failed = true;
      break;
    default:
      Serial.printf("WPS event: %d\n", event);
      break;
  }
}

void wifi_wps() {
  Serial.println("No WiFi saved. Press the WPS button on your router within 2 minutes.");

  WiFi.onEvent(wps_event_handler);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  esp_wps_config_t wps_config;
  memset(&wps_config, 0, sizeof(wps_config));
  wps_config.wps_type = WPS_TYPE_PBC;

  esp_err_t err = esp_wifi_wps_enable(&wps_config);
  Serial.printf("esp_wifi_wps_enable: %s\n", esp_err_to_name(err));

  err = esp_wifi_wps_start(0);
  Serial.printf("esp_wifi_wps_start: %s\n", esp_err_to_name(err));

  // Purple blink while waiting for WPS button press
  bool blink = false;
  unsigned long start = millis();
  unsigned long last_print = 0;
  while (!wps_success && !wps_failed && millis() - start < 120000) {
    led_set_color(blink ? 150 : 0, 0, blink ? 150 : 0);
    blink = !blink;
    delay(500);
    if (millis() - last_print > 5000) {
      Serial.printf("Waiting for WPS... (%ds elapsed)\n", (millis() - start) / 1000);
      last_print = millis();
    }
  }

  esp_wifi_wps_disable();

  if (!wps_success) {
    Serial.println("WPS failed or timed out");
    led_set_color(255, 0, 0);
    delay(2000);
    led_clear();
    return;
  }

  Serial.println("WPS success! Connecting...");

  // ESP32 automatically uses WPS credentials with WiFi.begin()
  WiFi.begin();

  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    led_set_color(i % 2 == 0 ? 255 : 0, i % 2 == 0 ? 80 : 0, 0);
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    // Save credentials to NVS for future boots
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.putString("ssid", WiFi.SSID());
    prefs.putString("pass", WiFi.psk());
    prefs.end();
    Serial.println("WiFi credentials saved");
  }
  led_clear();
}

void wifi_connect() {
  Preferences prefs;
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.isEmpty()) {
    wifi_wps();
    return;
  }

  Serial.printf("Connecting to %s", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());

  // Orange blink while connecting
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    led_set_color(i % 2 == 0 ? 255 : 0, i % 2 == 0 ? 80 : 0, 0);
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi failed, skipping OTA check");
  }
  led_clear();
}

// =============================================
//   OTA
// =============================================

static String extract_json_string(const String& json, const String& key) {
  String search = "\"" + key + "\":\"";
  int idx = json.indexOf(search);
  if (idx < 0) return "";
  idx += search.length();
  int end = json.indexOf("\"", idx);
  return json.substring(idx, end);
}

void ota_check() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.printf("Checking for updates (current: %s)...\n", FIRMWARE_VERSION);

  // Cyan while checking GitHub
  led_set_color(0, 200, 200);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  http.begin(client, "https://api.github.com/repos/araltiparmak/esp32c3-led-controller/releases/latest");
  http.addHeader("User-Agent", "ESP32-OTA");
  int code = http.GET();

  if (code != 200) {
    Serial.printf("GitHub API error: %d\n", code);
    http.end();
    led_clear();
    return;
  }

  String payload = http.getString();
  http.end();

  String tag = extract_json_string(payload, "tag_name");
  Serial.printf("Latest tag: '%s', Current: '%s'\n", tag.c_str(), FIRMWARE_VERSION);
  if (tag.startsWith("v")) tag = tag.substring(1);
  Serial.printf("Comparing: '%s' == '%s' ? %d\n", tag.c_str(), FIRMWARE_VERSION, tag == FIRMWARE_VERSION);

  if (tag.isEmpty() || tag == FIRMWARE_VERSION) {
    Serial.println("Already up to date");
    led_clear();
    return;
  }

  Serial.printf("New version available: %s\n", tag.c_str());

  // Find .bin download URL
  String bin_url = "";
  int search_pos = 0;
  while (true) {
    int idx = payload.indexOf("browser_download_url", search_pos);
    if (idx < 0) break;
    int start = payload.indexOf("\"", idx + 22) + 1;
    int end = payload.indexOf("\"", start);
    String url = payload.substring(start, end);
    if (url.endsWith(".bin")) {
      bin_url = url;
      break;
    }
    search_pos = end;
  }

  if (bin_url.isEmpty()) {
    Serial.println("No .bin file found in release");
    led_clear();
    return;
  }

  Serial.printf("Downloading %s\n", bin_url.c_str());

  // Blue while downloading
  led_set_color(0, 0, 255);

  WiFiClientSecure dl_client;
  dl_client.setInsecure();
  HTTPClient dl_http;
  dl_http.begin(dl_client, bin_url);
  dl_http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int dl_code = dl_http.GET();
  if (dl_code != 200) {
    Serial.printf("Download failed: %d\n", dl_code);
    dl_http.end();
    led_set_color(255, 0, 0);
    delay(2000);
    led_clear();
    return;
  }

  int bin_size = dl_http.getSize();
  if (!Update.begin(bin_size)) {
    Serial.println("Not enough space for update");
    dl_http.end();
    led_set_color(255, 0, 0);
    delay(2000);
    led_clear();
    return;
  }

  size_t written = Update.writeStream(*dl_http.getStreamPtr());
  dl_http.end();

  if (written != (size_t)bin_size || !Update.end() || !Update.isFinished()) {
    Serial.printf("Update failed (written %d / %d)\n", written, bin_size);
    led_set_color(255, 0, 0);
    delay(2000);
    led_clear();
    return;
  }

  // Green flash on success
  Serial.println("Update complete! Rebooting...");
  for (int i = 0; i < 3; i++) {
    led_set_color(0, 255, 0);
    delay(300);
    led_clear();
    delay(200);
  }
  ESP.restart();
}

// =============================================
//   Web Server
// =============================================

WebServer server(80);

const char* html =
  "<!DOCTYPE html><html><head>"
  "<meta charset='utf-8'>"
  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
  "<title>LED Controller</title>"
  "<style>"
  "body{font-family:sans-serif;background:#111;color:#fff;text-align:center;padding:20px}"
  "h1{font-size:1.4em;margin-bottom:30px}"
  ".btn{display:block;width:80%;margin:12px auto;padding:18px;font-size:1.2em;font-weight:bold;border:none;border-radius:14px;cursor:pointer;color:#fff}"
  "</style></head><body>"
  "<h1>LED Controller</h1>"
  "<button class='btn' style='background:#cc0000' onclick=\"set('red')\">Red</button>"
  "<button class='btn' style='background:#00aa00' onclick=\"set('green')\">Green</button>"
  "<button class='btn' style='background:#0055cc' onclick=\"set('blue')\">Blue</button>"
  "<hr style='border-color:#333;margin:20px 0'>"
  "<button class='btn' style='background:#444' onclick=\"set('rainbow')\">Rainbow</button>"
  "<button class='btn' style='background:#444' onclick=\"set('breathing')\">Breathing</button>"
  "<button class='btn' style='background:#444' onclick=\"set('chase')\">Chase</button>"
  "<button class='btn' style='background:#444' onclick=\"set('twinkle')\">Twinkle</button>"
  "<script>function set(t){fetch('/theme?name='+t)}</script>"
  "</body></html>";

void web_task(void* arg) {
  server.on("/", []() {
    server.send(200, "text/html", html);
  });

  server.on("/theme", []() {
    String name = server.arg("name");
    if      (name == "rainbow")   current_theme = RAINBOW;
    else if (name == "breathing") current_theme = BREATHING;
    else if (name == "chase")     current_theme = CHASE;
    else if (name == "twinkle")   current_theme = TWINKLE;
    else if (name == "red")       current_theme = RED;
    else if (name == "green")     current_theme = GREEN;
    else if (name == "blue")      current_theme = BLUE;
    server.send(200, "text/plain", "ok");
  });

  server.begin();
  Serial.printf("Web UI: http://%s\n", WiFi.localIP().toString().c_str());

  while (true) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// =============================================
//   Main
// =============================================

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  led_clear();

  wifi_connect();

  xTaskCreate(ota_task, "ota", 8192, NULL, 1, NULL);
  xTaskCreate(web_task, "web", 8192, NULL, 1, NULL);

  Serial.printf("Ready! Firmware v%s\n", FIRMWARE_VERSION);
}

void ota_task(void* arg) {
  vTaskDelay(pdMS_TO_TICKS(5000));
  while (true) {
    ota_check();
    vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
  }
}

void loop() {
  static unsigned long last_status = 0;
  if (millis() - last_status > 10000) {
    last_status = millis();
    Serial.printf("IP: %s | Theme: %d | v%s\n", WiFi.localIP().toString().c_str(), current_theme, FIRMWARE_VERSION);
  }

  static Theme last_theme = current_theme;
  if (current_theme != last_theme) {
    FastLED.setBrightness(LED_BRIGHTNESS);
    last_theme = current_theme;
  }

  switch (current_theme) {
    case RAINBOW:   led_rainbow(2);            break;
    case BREATHING: led_breathing(CRGB::Blue); break;
    case CHASE:     led_chase(CRGB::White);    break;
    case TWINKLE:   led_twinkle();             break;
    case RED:       led_set_color(255, 0, 0);  delay(20); break;
    case GREEN:     led_set_color(0, 255, 0);  delay(20); break;
    case BLUE:      led_set_color(0, 0, 255);  delay(20); break;
  }
}
