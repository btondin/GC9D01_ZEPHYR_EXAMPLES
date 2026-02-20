/*
 * LVGL usage example on Zephyr RTOS
 *
 * Copyright (c) 2023 Benjamin Cabé <benjamin@zephyrproject.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>

#include <lvgl.h>
#include "lv_font_tiny5.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define ACCEL_SCALE 100
#define G_MS2       9.81

static lv_obj_t *chart1;
static lv_chart_series_t *ser_x;
static lv_chart_series_t *ser_y;
static lv_chart_series_t *ser_z;
static lv_obj_t *lbl_x;
static lv_obj_t *lbl_y;
static lv_obj_t *lbl_z;
static lv_timer_t *sensor_timer;

const struct device *accel_sensor;

/* Timer handler: fetches sensor data and appends it to the chart */
static void sensor_timer_cb(lv_timer_t *timer)
{
	struct sensor_value accel[3];
	int rc = sensor_sample_fetch(accel_sensor);

	if (rc == 0) {
		rc = sensor_channel_get(accel_sensor, SENSOR_CHAN_ACCEL_XYZ, accel);
	}
	if (rc < 0) {
		LOG_ERR("ERROR: Update failed: %d\n", rc);
		return;
	}

	double ax = sensor_value_to_double(&accel[0]);
	double ay = sensor_value_to_double(&accel[1]);
	double az = sensor_value_to_double(&accel[2]);

	lv_chart_set_next_value(chart1, ser_x, (int32_t)(ax * ACCEL_SCALE));
	lv_chart_set_next_value(chart1, ser_y, (int32_t)(ay * ACCEL_SCALE));
	lv_chart_set_next_value(chart1, ser_z, (int32_t)(az * ACCEL_SCALE));

	/* Update legend with current values in G */
	lv_label_set_text_fmt(lbl_x, "X:%.1f", ax / G_MS2);
	lv_label_set_text_fmt(lbl_y, "Y:%.1f", ay / G_MS2);
	lv_label_set_text_fmt(lbl_z, "Z:%.1f", az / G_MS2);
}

static void create_legend(lv_obj_t *parent)
{
	lv_obj_t *legend = lv_obj_create(parent);
	lv_obj_remove_style_all(legend);
	lv_obj_set_size(legend, LV_HOR_RES, 8);
	lv_obj_align(legend, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_style_bg_color(legend, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(legend, LV_OPA_COVER, 0);
	lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(legend, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);

	static const struct {
		const char *init;
		lv_color_t color;
		lv_obj_t **lbl;
	} items[] = {
		{"X:--", LV_COLOR_MAKE(255, 0, 0),   &lbl_x},
		{"Y:--", LV_COLOR_MAKE(0, 128, 255), &lbl_y},
		{"Z:--", LV_COLOR_MAKE(0, 255, 0),   &lbl_z},
	};

	for (int i = 0; i < 3; i++) {
		lv_obj_t *lbl = lv_label_create(legend);
		lv_label_set_text(lbl, items[i].init);
		lv_obj_set_style_text_font(lbl, &lv_font_tiny5, 0);
		lv_obj_set_style_text_color(lbl, items[i].color, 0);
		*items[i].lbl = lbl;
	}
}

#define CHART_TOP_MARGIN 8


static void create_accelerometer_chart(lv_obj_t *parent)
{
	create_legend(parent);

	/* Fundo preto na tela */
	lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

	chart1 = lv_chart_create(parent);
	lv_obj_set_size(chart1, LV_HOR_RES, LV_VER_RES - CHART_TOP_MARGIN);
	lv_obj_align(chart1, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_set_style_bg_color(chart1, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(chart1, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(chart1, 0, 0);
	lv_chart_set_type(chart1, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(chart1, 5, 8);
	lv_obj_set_style_line_color(chart1, lv_color_make(40, 40, 40), LV_PART_MAIN);
	lv_chart_set_range(chart1, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000); /* -/+ 2G scaled x100 */
	lv_chart_set_update_mode(chart1, LV_CHART_UPDATE_MODE_SHIFT);

	ser_x = lv_chart_add_series(chart1, lv_color_make(255, 0, 0),
				    LV_CHART_AXIS_PRIMARY_Y);
	ser_y = lv_chart_add_series(chart1, lv_color_make(0, 128, 255),
				    LV_CHART_AXIS_PRIMARY_Y);
	ser_z = lv_chart_add_series(chart1, lv_color_make(0, 255, 0),
				    LV_CHART_AXIS_PRIMARY_Y);

	lv_chart_set_point_count(chart1, CONFIG_SAMPLE_CHART_POINTS_PER_SERIES);

	/* Do not display point markers on the data */
	lv_obj_set_style_size(chart1, 0, 0, LV_PART_INDICATOR);
}

/* Scan do barramento I2C - lista todos os enderecos que respondem */
static void i2c_scan(const struct device *i2c_dev)
{
	uint8_t dummy;

	LOG_INF("I2C scan on %s...", i2c_dev->name);
	for (uint8_t addr = 0x08; addr < 0x78; addr++) {
		int ret = i2c_read(i2c_dev, &dummy, 1, addr);
		if (ret == 0) {
			LOG_INF("  Found device at 0x%02X", addr);
		}
	}
	LOG_INF("I2C scan done.");
}

int main(void)
{
	const struct device *display_dev;

	/* I2C scan para debug - remover depois */
	const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (device_is_ready(i2c_dev)) {
		i2c_scan(i2c_dev);
	} else {
		LOG_ERR("I2C0 not ready");
	}

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return -ENODEV;
	}

	accel_sensor = DEVICE_DT_GET(DT_ALIAS(accel0));
	if (!device_is_ready(accel_sensor)) {
		LOG_ERR("Device %s is not ready\n", accel_sensor->name);
		return -ENODEV;
	}

	create_accelerometer_chart(lv_screen_active());
	sensor_timer = lv_timer_create(sensor_timer_cb,
					200/ CONFIG_SAMPLE_ACCEL_SAMPLING_RATE,
					NULL);
	lv_timer_handler();
	display_blanking_off(display_dev);

	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}
