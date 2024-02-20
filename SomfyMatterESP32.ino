/*
   Matter gateway for Somfy
   Based on LILYGO Lora Model T2 V1.6.1
   OBS!
    Use #SP32 WROVER Module in board selector due to partition setting
    Use Partition Scheme: "Huge App.....
*/
#define DEBUG_DISABLED
#define debug_

#define HOSTNAME "SomfyMATTER"
#define VERSION "v-0.1"
#define QRpayload "MT:Y.K9042C00KA0648G00",2,"3497","011","2332"
#define NUM_BLINDS 3

#include "mymatter.h"
#include <TimeOut.h>
#include "oled.h"
#include "somfy.h"

DEVICE * devs[] = {
  new DEVICE(0x46371C, "left", "Venstre markise", 37.0, 37.0),
  new DEVICE(0x46371D, "right", "Højre markise", 37.0, 37.0),
  new DEVICE(0x46371E, "kitchen", "Køkken markise", 38.0, 38.0),
};

DEVICES dev(devs);

TimeOut timer1;
OLED oled;
FIELD status(&oled, 16, 26, 14, 100, TEXT_ALIGN_CENTER);

void setup() {
  Serial.begin(115200);
  //esp_log_level_set("*", ESP_LOG_DEBUG);
  display.init();
  display.flipScreenVertically();
  display.resetDisplay();
  oled.QRcode(QRpayload);
  dev.onSendRf([](uint32_t addr, uint8_t command) {
    timer1.cancel();
    oled.Clear(ALL);
    oled.Data(command, addr);
    timer1.timeOut(3000, [] () {
      oled.Clear(ALL);
      oled.QRcode(QRpayload);
    });
  });
  dev.onPublish([](char * topic, char * payload, bool retain) {
    uint8_t i = dev.getIndexFromTopic(topic);
    String subTopic = dev.getSubTopic(topic);
    uint8_t Position = atoi((char *)payload);
    if (subTopic == mqtt_PositionTopic) {
      timer1.cancel();
      char buf[50];
      sprintf(buf, "%s Pos:%d", devs[i]->name, Position);
      oled.Clear(ALL);
      status.write(buf);
      timer1.timeOut(3000, [] () {
        oled.Clear(ALL);
        oled.QRcode(QRpayload);
      });
    }
  });
  dev.begin();
  mymatter.onMatterUpdate([](uint8_t index, uint16_t endpoint_id, uint16_t _value ) {
    if (devs[index]) {
      uint16_t value = 100 - _value / 100;
      Serial.println("---------------------------------------------------------------------------");
      Serial.printf("Addr: %06X, Name: %s, Value: %d\n", devs[index]->devAddr, devs[index]->name, value);
      Serial.println("---------------------------------------------------------------------------");
      //if (devs[index]) devs[index]->onData(mqtt_TargetTopic, &(String(value))[0]);
      devs[index]->prepareTarget(value);
    }
  });
  mymatter.begin();
}

void loop() {
  TimeOut::handler();
  dev.loop();
}
