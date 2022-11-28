#include <Arduino.h>
#include "secrets.h"
#include "debug.h"
#include <SPI.h>
#include <WiFi.h>
#include <TimeLib.h>

#include "rssRead.hpp"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#define FIRMWARE_VERSION "v0.0.1-b"
// Matrix Display params




#define DATA_PIN 12
#define CS_PIN 14
#define CLK_PIN 27
/* When ESP Prog connected use:
#define DATA_PIN 27
#define CS_PIN 26
#define CLK_PIN 25
*/
#define MAX_DEVICES 6

char ssid[] = WIFI_SSID; //  your network SSID (name)
char pass[] = WIFI_PASS; // your network password

#define RSSENTRIES 6
#define MAX_RSS_STRING_LENGTH 128 // MAX length of the RSS input to be considdered
#define RSS_REFRESH 600           // RSS REfresh in seconds
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

// Globals
MD_Parola Display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

unsigned long lastDownloadTime = 0;
unsigned long nowTime;
bool firstProbe = true;
static const char *url = " https://www.nrc.nl/rss/";
// const char *url = "https://feeds.nos.nl/nosnieuwsalgemeen";
// const char *url  = "https://news.yahoo.co.jp/rss/topics/top-picks.xml";
// const char *url = "https://social.secret-wg.org/@olaf.rss";
//
//  Time keeping stuff
//  Code from
//  https://github.com/PaulStoffregen/Time/blob/master/examples/TimeNTP_ESP8266WiFi/TimeNTP_ESP8266WiFi.ino
static const char ntpServerName[] = "nl.pool.ntp.org";
const int timeZone = 1; // Central European Time
WiFiUDP Udp;
unsigned int localPort = 8888; // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);


int currentEntry = 0;

// Array to store news items in.
String Entries[RSSENTRIES];

void setup()
{
  currentEntry = 0;
  for (int i; i < RSSENTRIES; i++)
  {

    Entries[i] = "-";
  }
  Serial.begin(115200);
  delay(1000);

  Display.begin();
  Display.setIntensity(0);
  Display.displayClear();

  delay(2000);
  Display.print(FIRMWARE_VERSION);
  delay(2000);

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
  Udp.begin(localPort);
  LOGINFO("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

void loop()
{
  nowTime = millis();

  if (firstProbe or (max(nowTime, lastDownloadTime) - min(nowTime, lastDownloadTime)) >= RSS_REFRESH * 1000 or lastDownloadTime > nowTime)
  {
    if ((WiFi.status() == WL_CONNECTED))
    { // Check the current connection status
      static rssRead rss;
      unsigned int RSSentryCount = 0;
      String dst;
      firstProbe = false;
      LOGINFO("Start rssRead ==>");
      rss.begin();
      lastDownloadTime = millis();
      rss.axs(url);
      // rss.dumpXml();
      // Move to the first item.
      dst = rss.finds(String("item"));
      LOGINFO(dst.c_str());
      while ((RSSentryCount < RSSENTRIES))
      {
        dst = rss.finds(String("title"));

        if (!dst.length())
          break;

        String rawtitle = dst;
        rawtitle.replace("<![CDATA[", "");
        rawtitle.replace("]]>", "");
        rawtitle.replace("'", "\"");
        rawtitle.replace("`", "\"");
        rawtitle.replace("’", "'");
        rawtitle.replace("‘", "'");
        rawtitle.replace("é", "e");
        rawtitle.replace("ë", "e");
        rawtitle.replace("ö", "o");
        rawtitle.replace("í", "i");

        LOGINFO1("TITLE:", rawtitle);
        Entries[RSSentryCount] = rawtitle;
        RSSentryCount++;

        LOGINFO1("currentEntry: ", currentEntry);
      }
      LOGINFO1("currentEntry: ", currentEntry);
      LOGINFO1("<== End rssRead:", rss.tagCnt());
    }
    LOGINFO("ClearingDisplay");
    // Display.displayClear();
    Display.displayText("Reloading News", PA_CENTER, 60, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  }

  if (Display.displayAnimate())
  {
    LOGINFO("Display Reset");
    if (timeStatus() != timeNotSet)
    {

      String timeString = String(hour()) + ":" + (minute() < 10 ? "0" : "") + String(minute());
      LOGINFO(timeString);
      // Display.displayClear();
      Display.setTextAlignment(PA_CENTER);
      Display.print(timeString);
      delay(2000);
      // Display.displayClear();
    }
    LOGINFO(Entries[currentEntry]);
    Display.displayText(Entries[currentEntry].c_str(), PA_CENTER, 40, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    currentEntry++;
    if (currentEntry >= RSSENTRIES)
      currentEntry = 0;
  }
}
/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48;     // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ; // discard any previously received packets
  LOGINFO("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  LOGINFO2(ntpServerName, ": ", ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      LOGINFO("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  LOGINFO("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}