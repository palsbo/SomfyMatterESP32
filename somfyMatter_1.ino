/*
  Matter gateway for Somfy
  Based on LILYGO Lora Model T2 V1.6.1
  OBS!
    Use #SP32 WROVER Module in board selector due to partition setting
    Use Partition Scheme: "Huge App.....
*/
#define DEBUG_DISABLED
#define debug_
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

#define HOSTNAME "SomfyMATTER"
#define VERSION "v-0.1"
#define QRpayload "MT:Y.K9042C00KA0648G00",2,"3497","011","2332"

#include <TimeOut.h>
#include "oled.h"
#include "somfy.h"

#define ATTRIBUTE_ID 11
#define NUM_BLINDS 3

DEVICE * devs[] = {
  new DEVICE(0x46371C, "left", "Venstre markise", 37.0, 37.0),
  new DEVICE(0x46371D, "right", "Højre markise", 37.0, 37.0),
  new DEVICE(0x46371E, "kitchen", "Køkken markise", 38.0, 38.0),
};

DEVICES dev(devs);
TimeOut timer1;
OLED oled;
FIELD ip(&oled, 23, 51, 14, 86, TEXT_ALIGN_CENTER, true);
FIELD line1(&oled, 16, 14, 14, 100, TEXT_ALIGN_LEFT);
FIELD line2(&oled, 16, 26, 14, 100, TEXT_ALIGN_LEFT);
FIELD line3(&oled, 16, 38, 14, 100, TEXT_ALIGN_LEFT);
FIELD status(&oled, 16, 26, 14, 100, TEXT_ALIGN_CENTER);

static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}

static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id,
                                   uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
  return ESP_OK;
}

static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
  if (type == attribute::PRE_UPDATE && attribute_id == 11) {
    for (int i = 0; i < NUM_BLINDS; i++) {
      if ((devs[i]->endpoint_id == endpoint_id)) {
        uint16_t value = 100 - val->val.u16 / 100;
        Serial.println("---------------------------------------------------------------------------");
        Serial.printf("Addr: %06X, Name: %s, Value: %s\n", devs[i]->devAddr, devs[i]->name, &(String(value))[0]);
        Serial.printf("Endpoint_id: %d, Cluster_id: %d, Attribue_id: %d, Value: %d\n", endpoint_id, cluster_id, attribute_id, value);
        Serial.println("---------------------------------------------------------------------------");
        if (devs[i]) devs[i]->onData(mqtt_TargetTopic, &(String(value))[0]);
      }
    }
  }
  return ESP_OK;
}

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


  node::config_t node_config;
  node_t *node = node::create(&node_config, on_attribute_update, on_identification);
  endpoint_t *endpoint[NUM_BLINDS];
  cluster_t *cluster[NUM_BLINDS];
  cluster::window_covering::feature::lift::config_t lift[NUM_BLINDS];
  cluster::window_covering::feature::position_aware_lift::config_t position_aware_lift[NUM_BLINDS];

  window_covering_device::config_t window_covering_device_config;
  for (int i = 0; i < NUM_BLINDS; i++) {
    endpoint[i] = window_covering_device::create(node, &window_covering_device_config, ENDPOINT_FLAG_NONE, NULL);
    if (endpoint[i] != NULL) {
      cluster[i] = cluster::get(endpoint[i], WindowCovering::Id);
      cluster::window_covering::feature::lift::add(cluster[i], &lift[i]);
      cluster::window_covering::feature::position_aware_lift::add(cluster[i], &position_aware_lift[i]);
      if (devs[i]) {
        devs[i]->endpoint_id = endpoint::get_id(endpoint[i]);
        devs[i]->cluster_id = cluster::get_id(cluster[i]);
      }
    }
  }

  // Setup DAC (this is good place to also set custom commission data, passcodes etc.)
  esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

  esp_matter::start(on_device_event);

  PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
}

void loop() {
  TimeOut::handler();
  dev.loop();
}
