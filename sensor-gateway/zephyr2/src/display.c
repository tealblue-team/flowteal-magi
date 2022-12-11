#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(flowteal_magi_sensors_gateway, CONFIG_SPRINKLER_HTTP_POST_LOG_LEVEL);

#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "display.h"

lv_obj_t *ui_label_top;
lv_obj_t *ui_label_mid;
lv_obj_t *ui_label_bottom;

void display_init(void) {
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
}