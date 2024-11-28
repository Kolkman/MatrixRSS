
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
#include <AsyncTCP.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <stdio.h>
#include <string.h>

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#define FIRMWARE_VERSION "v1.0.0-MQTT"

#define DISPLAY_TIMEOUT 60
// Matrix Display params

#define DATA_PIN 12
#define CS_PIN 14
#define CLK_PIN 27
/* When ESP Prog connected use:
#define DATA_PIN 27
#define CS_PIN 26
#define CLK_PIN 25
*/
#define MAX_DEVICES 12

char ssid[] = WIFI_SSID; //  your network SSID (name) from secrets.h
char pass[] = WIFI_PASS; // your network password

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// #define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

static const char ntpServerName[] = "nl.pool.ntp.org";
const int timeZone = 1; // Central European Time
WiFiUDP ntpUDP;
unsigned int localPort = 8888; // local port to listen for UDP packets

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
extern double mqttTemperature;
unsigned long lastDownloadTime = 0;
unsigned long nowTime;
bool firstProbe = true;

ContentContainer container;
char currententry[ELEMENT_LENGTH];

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  LOGINFO0("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    char info[64];
    sprintf(info,"OTA Progress Current: %u bytes, Final: %u bytes\n", current,   final) ;
    LOGINFO0(info);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    LOGINFO0("OTA update finished successfully!");
  } else {
    LOGERROR0("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void setup() {
  strcpy(currententry, "Initializing");

  Serial.begin(115200);
  delay(1000);
  container.init();
  Display.begin();
  Display.setIntensity(0);
  Display.displayClear();

  delay(2000);
  Display.setTextAlignment(PA_CENTER);
  Display.print(FIRMWARE_VERSION);
  delay(2000);

  // Display.print(FIRMWARE_VERSION);
  LOGINFO0("Setting up WIFI");
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  static String WiFiClient = String("MatrixRSS_") + String(ESP.getEfuseMac(), HEX);
  WiFi.setHostname(WiFiClient.c_str()); // define hostname
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    LOGINFO0("Connecting to WiFi..");
  }

  LOGINFO0("Connected to the WiFi network");
  LOGINFO0(WiFi.localIP().toString());
  LOGINFO0("Starting UDP");
  LOGINFO0("waiting for sync");

  timeClient.begin();
  delay(1000);
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
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });

  ElegantOTA.begin(&server); // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
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

      if (timeStatus() != timeNotSet) {

        // String timeString = String(hour()) + ":" + (minute() < 10 ? "0" : "")
        // + String(minute());
        String timeString =
            getEpochStringByParams(CE.toLocal(now()), (char *)"%H:%M");
        LOGINFO2(timeString, " Uptime: ", uptime);
        // Display.displayClear();
        Display.setTextAlignment(PA_CENTER);
        Display.print(timeString);
        delay(2000);
        // Display.displayClear();
      }
      Display.print(String(mqttTemperature, 1) + " \xB0"
                                                 "C"); // mqtt.h
      delay(2000);
      LOGINFO1("Displaying", currententry)
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
