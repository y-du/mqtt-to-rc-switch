#include "config.h"
#include <ESP8266WiFi.h>
#include <RCSwitch.h>
#include <PubSubClient.h>


unsigned long now = 0;
unsigned long last_reconnect = 0;
String client_id = "ESP8266Client-";


WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);
RCSwitch rc_switch = RCSwitch();


void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
     digitalWrite(1, LOW);
     delay(250);
     digitalWrite(1, HIGH);
     delay(250);
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  char* tok = strtok(topic, "/");
  char* lvls[2];
  int i = 0;
  while(tok) {
    tok = strtok(NULL, "/");
    lvls[i++] = tok;
  }
  if ((char)payload[0] == '1') {
    rc_switch.switchOn(lvls[0], lvls[1]);
  }
  if ((char)payload[0] == '0') {
    rc_switch.switchOff(lvls[0], lvls[1]);
  }
}


boolean reconnect() {
    digitalWrite(1, LOW);
    if (mqtt_client.connect(client_id.c_str())) {
      mqtt_client.subscribe("rc-switch/#");
    }
    digitalWrite(1, HIGH);
    return mqtt_client.connected();
  }


void setup() {
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);
  rc_switch.enableTransmit(3);
  setupWifi();
  randomSeed(micros());
  client_id += String(random(0xffff), HEX);
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(callback);
}


void loop() {
  if (!mqtt_client.connected()) {
    now = millis();
    if (now - last_reconnect > 5000) {
      last_reconnect = now;
      if (reconnect()) {
        last_reconnect = 0;
      }
    }
  } else {
    mqtt_client.loop();
  }
}
