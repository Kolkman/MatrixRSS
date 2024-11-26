#include <Arduino.h>
#include <SPI.h>
#include "mqtt.h"
#include "debug.h"
#include "secrets.h"
#include <WiFiClient.h>

#undef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>

#define MQTT_TOPIC "home/buitenTemp"

WiFiClient espClient;
PubSubClient client(espClient);
double mqttTemperature;

void MQTT_reconnect()
{
    if (!client.connected())
    {
        LOGINFO("Attempting MQTT connection...");
        static String MQTTClient = String("MatrixRSS_") + String(esp_random(), HEX);
        LOGINFO1("MQTT Client  name:", MQTTClient);
        if (client.connect(MQTTClient.c_str(), MQTT_USER, MQTT_PASS))
        {
            LOGINFO("connected");
            LOGINFO1("MQTT subscribing to: ", MQTT_TOPIC);
            client.subscribe(MQTT_TOPIC); // We should be OK with QOS 0
            LOGINFO("MQTT Subscription Passed")
        }
        else
        {
            LOGINFO1("failed, rc=", client.state());
        }
    }
    LOGDEBUG("MQTT_reconnect Returns");
}

void MQTT_callback(char *topic, byte *payload, unsigned int length)
{
    LOGDEBUG2("Message arrived [", topic, "] '");

    String msg = "";
    for (int i = 0; i < length; i++)
    {
        LOGDEBUG0((char)payload[i]);

        msg += (char)payload[i];
    }
    LOGDEBUG("'");

    mqttTemperature = msg.toFloat();
    LOGINFO(mqttTemperature);
}

void setupMQTT()
{
    mqttTemperature = 0.0;
    client.setServer(MQTT_HOST, MQTT_PORT);
    client.setCallback(MQTT_callback);
}

void loopMQTT()
{
    for (int i = 0; i < MAX_CONNECTION_RETRIES && !client.connected(); i++)
    {

        LOGINFO("MQTT Reconnection Attempt");
        MQTT_reconnect();
        delay(100);
    }
    client.loop();
}
