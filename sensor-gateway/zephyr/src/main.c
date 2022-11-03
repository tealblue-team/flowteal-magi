/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

#define MAX_DATA_LEN 255

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(lora_receive);

lv_obj_t *count_label;
char count_str[16] = {0};	
static int cnt = 0;

struct moisture {
	int msgid;
	int adc;
	int V;
	int vwc;
	};
struct json_obj_descr moisture_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct moisture, msgid, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct moisture, adc, JSON_TOK_NUMBER), 
	JSON_OBJ_DESCR_PRIM(struct moisture, V, JSON_TOK_NUMBER), 
	JSON_OBJ_DESCR_PRIM(struct moisture, vwc, JSON_TOK_NUMBER)
	};

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(size);

	LOG_INF("Message received (RSSI:%ddBm, SNR:%ddBm)",
		rssi, snr);
	for(size_t i = 0; i < size; i++ )
	{
		printf("%c", data[i]);
	}
	printf("\n");

	struct moisture moisture_json;

	int ret = json_obj_parse(data, sizeof(data),
                moisture_descr,
                ARRAY_SIZE(moisture_descr),
                &moisture_json);

	if (ret < 0)
	{
		LOG_ERR("JSON Parse Error: %d", ret);
	}
	else
	{
		LOG_INF("json_obj_parse return code: %d", ret);
		LOG_INF("msgid: %d", moisture_json.msgid);
		LOG_INF("adc: %d", moisture_json.adc);
		LOG_INF("V: %d", moisture_json.V);
		LOG_INF("vwc: %d", moisture_json.vwc);
	}

	++cnt;
	sprintf(count_str, "Messages: %d", cnt);
	lv_label_set_text(count_label, count_str);
	lv_task_handler();
	
	/* Stop receiving after 10 packets */
	// if (cnt == 10) {
	// 	LOG_INF("Stopping packet receptions");
	// 	lora_recv_async(dev, NULL);
	// }
}

void main(void)
{
	// DISPLAY
	const struct device *display_dev;
	lv_obj_t *hello_world_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

	hello_world_label = lv_label_create(lv_scr_act());

	lv_label_set_text(hello_world_label, "LoRa receiver");
	lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

	count_label = lv_label_create(lv_scr_act());
	lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	sprintf(count_str, "Messages: %d", cnt);
	lv_label_set_text(count_label, count_str);

	lv_task_handler();
	display_blanking_off(display_dev);

	// while (1) {
	// 	if ((count % 100) == 0U) {
	// 		sprintf(count_str, "%d", count/100U);
	// 		lv_label_set_text(count_label, count_str);
	// 	}
	// 	lv_task_handler();
	// 	++count;
	// 	k_sleep(K_MSEC(10));
	// }


	// LORA
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
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


	/* Receive 4 packets synchronously */
	// LOG_INF("Synchronous reception");
	// for (int i = 0; i < 4; i++) {
	// 	/* Block until data arrives */
	// 	len = lora_recv(lora_dev, data, MAX_DATA_LEN, K_FOREVER,
	// 			&rssi, &snr);
	// 	if (len < 0) {
	// 		LOG_ERR("LoRa receive failed");
	// 		return;
	// 	}

	// 	LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)",
	// 		data, rssi, snr);
	// }

	/* Enable asynchronous reception */
	LOG_INF("Asynchronous reception");
	lora_recv_async(lora_dev, lora_receive_cb);
	k_sleep(K_FOREVER);
}
