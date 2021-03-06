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
String client_id{STATION_ID};


WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);
RCSwitch rc_switch = RCSwitch();


void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(client_id);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
     delay(LOOP_DELAY);
  }
}


unsigned long last_msg{0};


void callback(char* topic, byte* payload, unsigned int length) {
  char* lvls[3];
  int i = 0;
  char* tok = strtok(topic, "/");
  while(tok) {
    tok = strtok(NULL, "/");
    if (i < 3) {
      lvls[i++] = tok;
    }
  }
  if ((char)payload[0] == '1') {
    rc_switch.switchOn(lvls[1], lvls[2]);
  }
  if ((char)payload[0] == '0') {
    rc_switch.switchOff(lvls[1], lvls[2]);
  }
  last_msg = millis();
}


String topic{String() + MQTT_TOPIC + "/" + STATION_ID + "/+/+"};


boolean reconnect() {
    if (mqtt_client.connect(client_id.c_str())) {
      mqtt_client.subscribe(topic.c_str());
    }
    return mqtt_client.connected();
  }


void setup() {
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  rc_switch.enableTransmit(rc_pin);
  randomSeed(micros());
  client_id += "-" + String(random(0xffff), HEX);
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(callback);
  setupWifi();
  digitalWrite(led_pin, HIGH);
}


unsigned long now{0};
unsigned long last_reconnect{0};


void loop() {
  if (!mqtt_client.connected()) {
    now = millis();
    if (now - last_reconnect > MQTT_RECONNECT) {
      last_reconnect = now;
      if (reconnect()) {
        last_reconnect = 0;
        digitalWrite(led_pin, LOW);
        delay(100);
        digitalWrite(led_pin, HIGH);
      }
    }
  } else {
    mqtt_client.loop();
  }
  if (millis() - last_msg >= MSG_WINDOW) {
    if (last_msg > 0) {
      last_msg = 0;
    }
    delay(LOOP_DELAY);
  } else {
    delay(1);
  }
}
