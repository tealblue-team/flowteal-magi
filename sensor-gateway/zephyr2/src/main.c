#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flowteal_magi_sensors_gateway, CONFIG_SPRINKLER_HTTP_POST_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/random/rand32.h>
#include <stdio.h>

#include "wifi.h"
#include "sockets.h"
//#include "display.h" // TODO: app crashes at runtime when adding display confs (Heltec)
#include "lora.h"
#include "moisture_data.h"

static struct sprinkler_context ctx;

static int cnt = 0;

static struct moisture moisture_json;

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

static void response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	if (final_data == HTTP_DATA_MORE) {
		LOG_DBG("Partial data received (%zd bytes)", rsp->data_len);
	} else if (final_data == HTTP_DATA_FINAL) {
		LOG_DBG("All the data received (%zd bytes)", rsp->data_len);
	}

	LOG_DBG("Response status %s", rsp->http_status);
}

static int collect_data(void)
{
	#define lower 20000
	#define upper 100000
	#define base  1000.00f

	float temp;

	/* Generate a temperature between 20 and 100 celsius degree */
	temp = ((sys_rand32_get() % (upper - lower + 1)) + lower);
	temp /= base;

	(void)snprintf(ctx.payload, sizeof(ctx.payload),
		       "{\"variable\": \"temperature\","
		       "\"unit\": \"c\",\"value\": %f}",
		       (double)temp);

	/* LOG doesn't print float #18351 */
	LOG_INF("Temp: %d", (int) temp);

	return 0;
}

static void next_turn(void)
{
	if (collect_data() < 0) {
		LOG_INF("Error collecting data.");
		return;
	}

	if (sprinkler_connect(&ctx) < 0) {
		LOG_INF("No connection available.");
		return;
	}

	if (sprinkler_http_push(&ctx, response_cb) < 0) {
		LOG_INF("Error pushing data.");
		return;
	}
}

void decode_moisture_info(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr)
{
	ARG_UNUSED(dev);
	LOG_INF("Message received (RSSI:%ddBm, SNR:%ddBm)",
		rssi, snr);

	// Copy "data" since first 2 chars and last char are always junk
 	size_t start = 2;
	size_t end = -1;
	size_t size_proc = size-start+end;
	char data_processed[size_proc];
	for(size_t i = 0; i < size_proc; i++ )
    	strcpy(data_processed+i,(char*)data+start+i+2);
	data_processed[size_proc] = '\0';

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
	//strncpy(sensor_str, moisture_json.sid, SID_UI_LEN);
	//lv_label_set_text_fmt(ui_label_top, "sid: %s", sensor_str);
	//lv_label_set_text_fmt(ui_label_mid, "msgid: %d", moisture_json.msgid);
	//lv_label_set_text_fmt(ui_label_bottom, "vwc: %de-2", moisture_json.vwc);
	//lv_task_handler();
	sprinkler_http_push(&ctx, response_cb);
}

void main(void)
{
	LOG_INF("Flowteal Magi - Sensors gateway");

	//display_init();  // TODO: app crashes at runtime when adding display confs (Heltec)

	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	lora_setup(lora_dev);

	wifi_connect();

	// while (true) {
	// 	next_turn();

	// 	k_sleep(K_SECONDS(CONFIG_SPRINKLER_HTTP_PUSH_INTERVAL));
	// }

	LOG_INF("Asynchronous reception");
	lora_recv_async(lora_dev, decode_moisture_info);
	k_sleep(K_FOREVER);
}
