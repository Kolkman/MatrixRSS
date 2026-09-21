
#include "MatrixRSS.h"
#include "debug.h"
#include "mqtt.h"
#include "secrets.h"
#include <Arduino.h>
#include <NTPClient.h>
#include <SPI.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <WiFi.h>
#include <time.h>
// #include "rssRead.hpp"
#include "contentcontainer.h"
#include "webInterface.h"
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <WiFiManager.h>
#include <stdio.h>
#include <string.h>

#define DISPLAY_TIMEOUT 60
#define WIFI_CONNECT_ATTEMPTS 20
#define WIFI_CONNECT_RETRY_DELAY_MS 500
// Matrix Display params

#define DATA_PIN 12
#define CS_PIN 14
#define CLK_PIN 27
/* When ESP Prog connected use:
#define DATA_PIN 27
#define CS_PIN 26
#define CLK_PIN 25
*/
#ifndef MAX_DEVICES 
#define MAX_DEVICES 12
#endif


char ssid[] = WIFI_SSID; //  your network SSID (name) from secrets.h
char pass[] = WIFI_PASS; // your network password

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// #define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

static const char ntpServerName[] = "nl.pool.ntp.org";
const int timeZone = 1; // Central European Time
WiFiUDP ntpUDP;
unsigned int localPort = 8888; // local port to listen for UDP packets
const char firmwareversion[]=FIRMWAREVERSION;
/**
 * Input time in epoch format and return tm time format
 * by Renzo Mischianti <www.mischianti.org>
 */
static tm getDateTimeByParams(unsigned long time) {
  struct tm *newtime;
  const time_t tim = time;
  newtime = localtime(&tim);
  return *newtime;
}

/**
 * Input tm time format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org>
 */
static String
getDateTimeStringByParams(tm *newtime,
                          char *pattern = (char *)"%d/%m/%Y %H:%M:%S") {
  char buffer[30];
  strftime(buffer, 30, pattern, newtime);
  return buffer;
}

/**
 * Input time in epoch format format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org>
 */
static String
getEpochStringByParams(unsigned long time,
                       char *pattern = (char *)"%d/%m/%Y %H:%M:%S") {
  //    struct tm *newtime;
  tm newtime;
  newtime = getDateTimeByParams(time);
  return getDateTimeStringByParams(&newtime, pattern);
}

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
// NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
int GTMOffset = 0;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GTMOffset * 60 * 60,
                     60 * 60 * 1000);

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun,
                       Mar,    2,    120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun,
                      Oct,    3,    60}; // Central European Standard Time
Timezone CE(CEST, CET);

// declarations
time_t getNtpTime();
void sendNTPpacket(IPAddress &);

// Globals
MD_Parola Display =
    MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
extern double mqttOutdoorTemperature;
extern double mqttOutdoorHumidity;
extern double mqttIndoorTemperature;
extern double mqttIndoorHumidity;
unsigned long lastDownloadTime = 0;
unsigned long nowTime;
bool firstProbe = true;

ContentContainer container;
char currententry[ELEMENT_LENGTH];
String IPaddress;
JsonDocument statusObject;

webInterface web;

unsigned long ota_progress_millis = 0;
WiFiManager wifiManager;

static String portalSSID() {
  return Hostname + String("_Setup");
}

static void showWifiSetupPortal(const String &apName) {
  Display.setTextAlignment(PA_CENTER);
  Display.print("WiFi setup");
  delay(1000);
  Display.print(apName);
  delay(1000);
  Display.print("192.168.4.1");
  IPaddress = "192.168.4.1";
}

static void onConfigPortalStarted(WiFiManager *manager) {
  (void)manager;
  const String apName = portalSSID();
  LOGWARN1("WiFi fallback AP active:", apName);
  showWifiSetupPortal(apName);
}

static bool waitForWiFiConnection(unsigned long attempts,
                                  unsigned long retryDelayMs) {
  for (unsigned long attempt = 0; attempt < attempts; ++attempt) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(retryDelayMs);
    LOGINFO0("Connecting to WiFi..");
  }
  return WiFi.status() == WL_CONNECTED;
}

static bool connectToWiFi(const char *networkSsid, const char *networkPass,
                          const char *label, bool persistCredentials) {
  if (networkSsid == nullptr || strlen(networkSsid) == 0) {
    return false;
  }

  LOGINFO1("Trying WiFi network:", label);
  Display.setTextAlignment(PA_CENTER);
  Display.print(String("WiFi ") + label);
  WiFi.persistent(persistCredentials);
  WiFi.begin(networkSsid, networkPass);
  WiFi.persistent(false);
  return waitForWiFiConnection(WIFI_CONNECT_ATTEMPTS,
                               WIFI_CONNECT_RETRY_DELAY_MS);
}

static bool hasSavedWiFiConfig() {
  const String savedSsid = wifiManager.getWiFiSSID();
  return !savedSsid.isEmpty();
}

static bool connectToSavedWiFi() {
  const String savedSsid = wifiManager.getWiFiSSID();
  if (savedSsid.isEmpty()) {
    return false;
  }

  LOGINFO1("Trying saved WiFi network:", savedSsid);
  Display.setTextAlignment(PA_CENTER);
  Display.print(String("WiFi ") + savedSsid);

  WiFi.persistent(false);
  WiFi.begin();
  return waitForWiFiConnection(WIFI_CONNECT_ATTEMPTS,
                               WIFI_CONNECT_RETRY_DELAY_MS);
}

static void configureWiFiFallback() {
  wifiManager.setHostname(Hostname.c_str());
  wifiManager.setAPCallback(onConfigPortalStarted);
  wifiManager.setConfigPortalTimeout(0);
  wifiManager.setConnectTimeout(10);
}

void setup() {
  strcpy(currententry, "Initializing");

  Serial.begin(115200);
  delay(1000);
  container.init();
  Display.begin();
  Display.setIntensity(0);
  Display.displayClear();

  delay(DISPLAY_DELAY);
  Display.setTextAlignment(PA_CENTER);
  Display.print(FIRMWAREVERSION);
  LOGINFO0("Display initialized");
  delay(DISPLAY_DELAY);

  LOGINFO0("Setting up WIFI");

  // This check is copied from ESPAsync_WifiManager
  // Check cores/esp32/esp_arduino_version.h and cores/esp32/core_version.h
#if (defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2))
  WiFi.setHostname(Hostname.c_str());
#else
  // Still have bug in ESP32_S2 for old core. If using WiFi.setHostname() =>
  // WiFi.localIP() always = 255.255.255.255
  if (String(ARDUINO_BOARD) != "ESP32S2_DEV") {
    // See https://github.com/espressif/arduino-esp32/issues/2537
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(Hostname.c_str());
  }
#endif

  configureWiFiFallback();

  bool wifiConnected = false;
  if (hasSavedWiFiConfig()) {
    wifiConnected = connectToSavedWiFi();
  }

  if (!wifiConnected) {
    wifiConnected = connectToWiFi(ssid, pass, ssid, false);
  }

  if (!wifiConnected) {
    LOGWARN0("WiFi connection failed, starting setup AP");
    const String apName = portalSSID();
    showWifiSetupPortal(apName);
    wifiConnected = wifiManager.startConfigPortal(apName.c_str());
  }

  if (!wifiConnected) {
    LOGERROR0("WiFi setup portal exited without a connection");
    Display.print("WiFi failed");
    ESP.restart();
  }

  IPaddress = WiFi.localIP().toString();
  LOGINFO0("Connected to the WiFi network");
  LOGINFO0(IPaddress);
  LOGINFO0("Starting UDP");
  LOGINFO0("waiting for sync");
  
  timeClient.begin();

  Display.setTextAlignment(PA_CENTER);
  Display.print(WiFi.getHostname());
  delay(2000);

  bool timeUpdatePass = true;
  for (int n = 0; n < 20; n++) {
    if (timeClient.update()) {
      timeUpdatePass = true;

      break;
    }
    delay(20);
  }

  if (timeUpdatePass) {
    LOGINFO0("Adjust local clock");
    unsigned long epoch = timeClient.getEpochTime();
    setTime(epoch);
    LOGINFO(getEpochStringByParams(CE.toLocal(now()), (char *)"%H:%M"));
  } else {
    LOGINFO0("NTP Update Failed!!");
  }

  //  setSyncProvider(getNtpTime);
  // setSyncInterval(300);
  LOGINFO0("Setting UP MQTT");
  setupMQTT();
  LOGINFO0("MQTT DONE, TIME SYNC")

  LOGINFO0("setting up webserver");

  web.setupWebSrv();

  LOGINFO0("HTTP server started");
}

void loop() {
  nowTime = millis();
  loopMQTT();

  if (Display.displayAnimate()) {
    container.readcontent(currententry);

    if (strlen(currententry)) {
      timeClient.update();
      LOGINFO3("HEAP:", ESP.getFreeHeap(), "/", ESP.getHeapSize());
      statusObject["freeheap"] = ESP.getFreeHeap();
      statusObject["heapsize"] = ESP.getHeapSize();
      statusObject["firmware"] = firmwareversion;
      statusObject["temperature"] = mqttOutdoorTemperature;
      statusObject["humidity"] = mqttOutdoorHumidity;
      statusObject["indoor_temperature"] = mqttIndoorTemperature;
      statusObject["indoor_humidity"] = mqttIndoorHumidity;

      char uptime[32];
      unsigned long milli = nowTime;
      long hr = milli / 3600000;
      milli = milli - 3600000 * hr;
      // 60000 milliseconds in a minute
      long min = milli / 60000;
      milli = milli - 60000 * min; // 1000 milliseconds in a second
      long sec = milli / 1000;
      milli = milli - 1000 * sec;
      sprintf(uptime, "%d:%02d:%02d", hr, min, sec);
      statusObject["uptime"] = uptime;
      statusObject["uptime_mili"] = milli;
      if (timeStatus() != timeNotSet) {

        // String timeString = String(hour()) + ":" + (minute() < 10 ? "0" : "")
        // + String(minute());
        String timeString =
            getEpochStringByParams(CE.toLocal(now()), (char *)"%H:%M");
        LOGINFO2(timeString, " Uptime: ", uptime);
        // Display.displayClear();
        Display.setTextAlignment(PA_CENTER);
        Display.print(timeString);
        delay(DISPLAY_DELAY);
        // Display.displayClear();
      }
      LOGINFO1("outdoorTemp",mqttOutdoorTemperature);
      LOGINFO1("outdoorHumidity",mqttOutdoorHumidity);
      if (mqttOutdoorTemperature < 100 && mqttOutdoorHumidity < 101) {
        Display.print("Buiten: " + String(mqttOutdoorTemperature, 1) + "\xB0 / " +
                      String(mqttOutdoorHumidity, 0) + "%");
        delay(DISPLAY_DELAY);
      }
      LOGINFO1("indoorTemp",mqttIndoorTemperature);
      LOGINFO1("indoorHumidity",mqttIndoorHumidity);
   
      if (mqttIndoorTemperature < 100 && mqttIndoorHumidity < 101) {
        Display.print("Binnen: " + String(mqttIndoorTemperature, 1) +
                      "\xB0 / " + String(mqttIndoorHumidity, 0) + "%");
        delay(DISPLAY_DELAY);
      }
      Display.print("");
      LOGINFO1("Displaying", currententry)
  
      statusObject["ip_address"] = IPaddress;;
      
      unsigned long startDisplayTime = millis();
      Display.displayText(currententry, PA_CENTER, 20, 0, PA_SCROLL_LEFT,
                          PA_SCROLL_LEFT);
      if (millis() > startDisplayTime &&
          now() - startDisplayTime > DISPLAY_TIMEOUT * 1000) {
        // Takes a very long time to display

        ESP.restart();
      }
    }
  }
}
