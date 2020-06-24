/*
   Copyright 2020 Yann Dumont

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "config.h"
#include <ESP8266WiFi.h>
#include <RCSwitch.h>
#include <PubSubClient.h>


const byte rc_pin{3};
const byte led_pin{1};
String client_id{MQTT_CLIENT_ID};


WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);
RCSwitch rc_switch = RCSwitch();


void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
     digitalWrite(led_pin, LOW);
     delay(250);
     digitalWrite(led_pin, HIGH);
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
    digitalWrite(led_pin, LOW);
    if (mqtt_client.connect(client_id.c_str())) {
      mqtt_client.subscribe(MQTT_TOPIC);
    }
    digitalWrite(led_pin, HIGH);
    return mqtt_client.connected();
  }


void setup() {
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);
  rc_switch.enableTransmit(rc_pin);
  setupWifi();
  randomSeed(micros());
  client_id += String(random(0xffff), HEX);
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(callback);
}


unsigned long now = 0;
unsigned long last_reconnect = 0;


void loop() {
  if (!mqtt_client.connected()) {
    now = millis();
    if (now - last_reconnect > MQTT_RECONNECT) {
      last_reconnect = now;
      if (reconnect()) {
        last_reconnect = 0;
      }
    }
  } else {
    mqtt_client.loop();
  }
}
