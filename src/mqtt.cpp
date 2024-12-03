#include "mqtt.h"
#include "MatrixRSS.h"
#include "contentcontainer.h"
#include "debug.h"
#include "secrets.h"
#include <Arduino.h>
#include <SPI.h>
#include <WiFiClient.h>
#include <algorithm>

#undef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>

#define MQTT_TEMP "home/buitenTemp"
#define MQTT_NEWS "rss/news"

extern ContentContainer container;

WiFiClient espClient;
PubSubClient client(espClient);
double mqttTemperature;
unsigned long previouspub = 0;

void MQTT_reconnect() {
  if (!client.connected()) {
    LOGINFO0("Attempting MQTT connection...");

    LOGINFO1("MQTT Client  name:", Hostname);
    if (client.connect(Hostname.c_str(), MQTT_USER, MQTT_PASS)) {
      LOGINFO("connected");
      LOGINFO1("MQTT subscribing to: ", MQTT_TEMP);
      client.subscribe(MQTT_TEMP); // We should be OK with QOS 0
      LOGINFO1("MQTT subscribing to: ", MQTT_NEWS);
      client.subscribe(MQTT_NEWS); // We should be OK with QOS 0
      LOGINFO("MQTT Subscription Passed")

    } else {
      LOGINFO1("failed, rc=", client.state());
    }
  }
  LOGDEBUG0("MQTT_reconnect Returns");
}

void MQTT_callback(char *topic, byte *payload, unsigned int length) {
  LOGDEBUG2("Message arrived [", topic, "] '");

  if (strcmp(topic, MQTT_TEMP) == 0) {
    // obvioulsy state of my red LED

    char msg[ELEMENT_LENGTH];
    for (int i = 0; i < min(length, (unsigned int)ELEMENT_LENGTH); i++) {
      LOGDEBUG0((char)payload[i]);
      msg[i] = (char)payload[i];
    }

    msg[ELEMENT_LENGTH - 1] = '\0'; // to be sure

    mqttTemperature = atof(msg);
    LOGINFO0(mqttTemperature);
  }

  if (strcmp(topic, MQTT_NEWS) == 0) {
    // obvioulsy state of my red LED

    char msg[ELEMENT_LENGTH];

    unsigned int j = 0;
    for (unsigned int i = 0; i < length && j < ELEMENT_LENGTH; i++) {

      if ((char)payload[i] == '\n') {
        continue;
      }
      if ((char)payload[i] == '\r') {
        continue;
      }
      if ((char)payload[i] == '\t') {
        continue;
      }
      msg[j] = (char)payload[i];

      LOGDEBUG((char)payload[i]);
      j++;
    }

    msg[min(j, (unsigned int)(ELEMENT_LENGTH - 1))] = '\0';
    LOGDEBUG('\n');
    container.addcontent(msg);
    LOGINFO(msg);
  }
}

void setupMQTT() {
  mqttTemperature = 150.0;
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(MQTT_callback);
}

void loopMQTT() {
  unsigned long timestamp = millis();
  for (int i = 0; i < MAX_CONNECTION_RETRIES && !client.connected(); i++) {

    LOGINFO0("MQTT Reconnection Attempt");
    MQTT_reconnect();
    delay(100);
  }

  if (((timestamp - previouspub) > 60 * 1000) || (previouspub > timestamp)) {
    char pubbuf[256];
    size_t n = serializeJson(statusObject, pubbuf);
    client.publish(Hostname.c_str(), pubbuf, n);
    previouspub = timestamp;
  }
  client.loop();
}
