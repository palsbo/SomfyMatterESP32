#ifndef MYMATTER_H
#define MYMATTER_H

#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

namespace mymatterspace {

typedef void (*cb_onmatterupdate) (uint8_t index, uint16_t endpoint_id, uint16_t value );

cb_onmatterupdate onmatterupdate;

uint16_t endpointId[NUM_BLINDS];

static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}

static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id,
                                   uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
  return ESP_OK;
}

static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
  if (type == attribute::PRE_UPDATE && attribute_id == 11) {
    for (int i = 0; i < NUM_BLINDS; i++) {
      if ((endpointId[i] == endpoint_id)) {
        if (onmatterupdate) onmatterupdate(i, endpoint_id, val->val.u16);
      }
    }
  }
  return ESP_OK;
}

void initEndpoints() {
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
      endpointId[i] = endpoint::get_id(endpoint[i]);
    }
  }
  // Setup DAC (this is good place to also set custom commission data, passcodes etc.)
  esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());
  esp_matter::start(on_device_event);
  //PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
}
} //  end of mymatterspace

class MYMATTER {
  private:
  public:
    void begin() {
      mymatterspace::initEndpoints();
    }

    void onMatterUpdate(mymatterspace::cb_onmatterupdate cb) {
      mymatterspace::onmatterupdate = cb;
    }
} mymatter;

#endif
