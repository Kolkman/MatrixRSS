#include <Arduino.h>
#include "secrets.h"
#include "debug.h"
#include <SPI.h>
#include <WiFi.h>
#include <TimeLib.h>
#include "mqtt.h"
#include <NTPClient.h>
#include <time.h>
#include <Timezone.h>
// #include "rssRead.hpp"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include "esp32rss.h"
#include <SafeString.h>
#define FIRMWARE_VERSION "v0.0.2-d"
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

#define RSS_REFRESH 1500 // RSS REfresh in seconds
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
static tm getDateTimeByParams(unsigned long time)
{
  struct tm *newtime;
  const time_t tim = time;
  newtime = localtime(&tim);
  return *newtime;
}

/**
 * Input tm time format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org>
 */
static String getDateTimeStringByParams(tm *newtime, char *pattern = (char *)"%d/%m/%Y %H:%M:%S")
{
  char buffer[30];
  strftime(buffer, 30, pattern, newtime);
  return buffer;
}

/**
 * Input time in epoch format format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org>
 */
static String getEpochStringByParams(unsigned long time, char *pattern = (char *)"%d/%m/%Y %H:%M:%S")
{
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
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GTMOffset * 60 * 60, 60 * 60 * 1000);

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone CE(CEST, CET);

// declarations
time_t getNtpTime();
void sendNTPpacket(IPAddress &);

// Globals
MD_Parola Display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
extern double mqttTemperature;
unsigned long lastDownloadTime = 0;
unsigned long nowTime;
bool firstProbe = true;

elementArray ruleset1 = {"rss", "channel", "item", "description"};



esp32RSS rssfeed("https://www.nrc.nl/rss/");
// esp32RSS rssfeed("https://social.secret-wg.org/@olaf.rss");
// esp32RSS rssfeed("https://www.xolx.nl/Various/test.rss");

//  "https://mastodon.social/tags/elonmusk";
//  "https://www.nrc.nl/rss/";
//  "https://nieuws.nl/feed/";
//  "https://feeds.nos.nl/nosnieuwsalgemeen";
//  "https://news.yahoo.co.jp/rss/topics/top-picks.xml";
//  "https://social.secret-wg.org/@olaf.rss";
//
//   Time keeping stuff
//   Code from
//   https://www.mischianti.org/2020/08/08/network-time-protocol-ntp-timezone-and-daylight-saving-time-dst-with-esp8266-esp32-or-arduino/

contentArray content;
int currentEntry = 0;


void setup()
{

  currentEntry = 0;

  Serial.begin(115200);
  delay(1000);

  Display.begin();
  Display.setIntensity(0);
  Display.displayClear();

  delay(2000);
  Display.setTextAlignment(PA_CENTER);
  Display.print(FIRMWARE_VERSION);
  delay(2000);

  rssfeed.addRuleSet(ruleset1);
  rssfeed.logRuleSets();

  // Display.print(FIRMWARE_VERSION);
  LOGINFO("Setting up WIFI");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    LOGINFO("Connecting to WiFi..");
  }

  LOGINFO("Connected to the WiFi network");

  LOGINFO("Starting UDP");
  LOGINFO("waiting for sync");

  timeClient.begin();
  delay(1000);
  bool timeUpdatePass = true;
  for (int n = 0; n < 20; n++)
  {
    if (timeClient.update())
    {
      timeUpdatePass = true;

      break;
    }
    delay(20);
  }

  if (timeUpdatePass)
  {
    LOGINFO("Adjust local clock");
    unsigned long epoch = timeClient.getEpochTime();
    setTime(epoch);
    LOGINFO(getEpochStringByParams(CE.toLocal(now()), (char *)"%H:%M"));
  }
  else
  {
    LOGINFO("NTP Update Failed!!");
  }

  //  setSyncProvider(getNtpTime);
  // setSyncInterval(300);
  LOGINFO("Setting UP MQTT");
  setupMQTT();
  LOGINFO("MQTT DONE, TIME SYNC")
}

void loop()
{
  nowTime = millis();
  timeClient.update();
  loopMQTT();

  if (firstProbe or (max(nowTime, lastDownloadTime) - min(nowTime, lastDownloadTime)) >= RSS_REFRESH * 1000 or lastDownloadTime > nowTime)

  {
    if ((WiFi.status() == WL_CONNECTED))
    { // Check the current connection status
      // initiate buffer first

      firstProbe = false;
      int count = rssfeed.getData();
      LOGINFO1("getData content retunred", count);
      Display.displayText("Reloading News", PA_CENTER, 60, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
   
      rssfeed.rssContent(content);
      LOGINFO("ClearingDisplay");
      Display.displayClear();

      lastDownloadTime = millis();
    }
  }
  if (Display.displayAnimate())
  {
    if (content[currentEntry] != "")
    {

      LOGINFO("Display Reset");
      LOGINFO3("HEAP:", ESP.getFreeHeap(), "/", ESP.getHeapSize());

      if (timeStatus() != timeNotSet)
      {

        // String timeString = String(hour()) + ":" + (minute() < 10 ? "0" : "") + String(minute());
        String timeString = getEpochStringByParams(CE.toLocal(now()), (char *)"%H:%M");
        LOGINFO(timeString);
        // Display.displayClear();
        Display.setTextAlignment(PA_CENTER);
        Display.print(timeString);
        delay(2000);
        // Display.displayClear();
      }
      Display.print(String(mqttTemperature, 1) + " \xB0"
                                                 "C"); // mqtt.h
      delay(2000);
      LOGINFO(content[currentEntry]);
      unsigned long startDisplayTime=millis();
      Display.displayText(content[currentEntry], PA_CENTER, 20, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      if (millis() > startDisplayTime && now() - startDisplayTime > DISPLAY_TIMEOUT*1000){
        // Takes a very long time to display
        	
      ESP.restart();
      }
    }

    currentEntry++;
    if (currentEntry >= content.max_size())
      currentEntry = 0;
  }
}
