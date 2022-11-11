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
#include <string.h>
#include <errno.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

#define MAX_DATA_LEN 255

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(lora_receive);

#define SID_UI_LEN 7
lv_obj_t *ui_label_top;
lv_obj_t *ui_label_mid;
lv_obj_t *ui_label_bottom;
char sensor_str[SID_UI_LEN] = {0};	
int cnt = 0;

struct moisture {
	char *sid;
	int msgid;
	int adc;
	int V;
	int vwc;
	};
struct json_obj_descr moisture_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct moisture, sid, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct moisture, msgid, JSON_TOK_NUMBER),
	//JSON_OBJ_DESCR_PRIM(struct moisture, adc, JSON_TOK_NUMBER), 
	//JSON_OBJ_DESCR_PRIM(struct moisture, V, JSON_TOK_NUMBER), 
	JSON_OBJ_DESCR_PRIM(struct moisture, vwc, JSON_TOK_NUMBER)
	};


int expected_ret_code = (1 << ARRAY_SIZE(moisture_descr)) - 1;

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr)
{
	ARG_UNUSED(dev);

	// Copy "data" since first 2 chars and last char are always junk

 	size_t start = 2;
	size_t end = -1;
	size_t size_proc = size-start+end;

	char data_processed[size_proc];
	for(size_t i = 0; i < size_proc; i++ )
    	strcpy(data_processed+i,(char*)data+start+i+2);
	data_processed[size_proc] = '\0';

	LOG_INF("Message received (RSSI:%ddBm, SNR:%ddBm)",
		rssi, snr);

	struct moisture moisture_json;
	int ret = json_obj_parse(data_processed, size_proc,
                moisture_descr,
                ARRAY_SIZE(moisture_descr),
                &moisture_json);

	if (ret < 0)
	{
		LOG_ERR("JSON Parse Error: %d", ret);
	}
	else if (ret != expected_ret_code)
	{
		LOG_ERR("Not all values decoded; Expected return code %d but got %d", expected_ret_code, ret);
	}
	else
	{
		LOG_INF("json_obj_parse return code: %d", ret);
		LOG_INF("sid: %s", moisture_json.sid);
		LOG_INF("msgid: %d", moisture_json.msgid);
		// LOG_INF("adc: %d", moisture_json.adc);
		// LOG_INF("V: %d", moisture_json.V);
		LOG_INF("vwc: %d", moisture_json.vwc);
	}

	++cnt;
	strncpy(sensor_str, moisture_json.sid, SID_UI_LEN);
	lv_label_set_text_fmt(ui_label_top, "sid: %s", sensor_str);
	lv_label_set_text_fmt(ui_label_mid, "msgid: %d", moisture_json.msgid);
	lv_label_set_text_fmt(ui_label_bottom, "vwc: %de-2", moisture_json.vwc);
	lv_task_handler();
}

void main(void)
{
	// DISPLAY
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

 	ui_label_top = lv_label_create(lv_scr_act());
	lv_obj_align(ui_label_top, LV_ALIGN_TOP_MID, 0, 0);
	lv_label_set_text(ui_label_top, "[latest sensor]");

	ui_label_mid = lv_label_create(lv_scr_act());
	lv_obj_align(ui_label_mid, LV_ALIGN_CENTER, 0, 0);
	lv_label_set_text(ui_label_mid, "[message id]");

	ui_label_bottom = lv_label_create(lv_scr_act());
	lv_obj_align(ui_label_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_label_set_text(ui_label_bottom, "[Vol Water Cont]");

	lv_task_handler();
	
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

	/* Enable asynchronous reception */
	LOG_INF("Asynchronous reception");
	lora_recv_async(lora_dev, lora_receive_cb);
	k_sleep(K_FOREVER);
}
