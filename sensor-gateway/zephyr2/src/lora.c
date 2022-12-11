#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(flowteal_magi_sensors_gateway, CONFIG_SPRINKLER_HTTP_POST_LOG_LEVEL);

#include "lora.h"

#define MAX_DATA_LEN 255

void lora_setup(const struct device *lora_dev) {
    // LORA
	//const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	struct lora_modem_config config;
	int ret, len;
	uint8_t data[MAX_DATA_LEN] = {0};
	int16_t rssi;
	int8_t snr;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return;
	}

	config.frequency = 868000000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_9;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 14;
	config.tx = false;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return;
	}
}