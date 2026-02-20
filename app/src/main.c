/*
 * LVGL Accelerometer Chart — GC9D01 160×160 Round TFT on Bruno nRF52832
 *
 * Displays real-time X/Y/Z acceleration data from an FXOS8700CQ sensor as
 * a scrolling line chart using LVGL on a GC9D01 SPI display.
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

/*
 * Scaling factor applied to raw acceleration values (m/s²) before passing
 * them to the LVGL chart. LVGL chart values are integers, so multiplying
 * by 100 preserves two decimal places of precision in integer form.
 * Example: 9.81 m/s² → 981 chart units.
 */
#define ACCEL_SCALE 100

/*
 * Standard gravity in m/s² used to convert raw sensor readings to G-force
 * for the legend labels (e.g., 9.81 m/s² ÷ 9.81 = 1.0 G).
 */
#define G_MS2       9.81

/* --- LVGL widget handles (global so the timer callback can update them) --- */
static lv_obj_t *chart1;             /* Scrolling line chart widget          */
static lv_chart_series_t *ser_x;     /* Chart data series for the X axis     */
static lv_chart_series_t *ser_y;     /* Chart data series for the Y axis     */
static lv_chart_series_t *ser_z;     /* Chart data series for the Z axis     */
static lv_obj_t *lbl_x;             /* Legend label showing current X value  */
static lv_obj_t *lbl_y;             /* Legend label showing current Y value  */
static lv_obj_t *lbl_z;             /* Legend label showing current Z value  */
static lv_timer_t *sensor_timer;    /* Periodic LVGL timer for sensor reads  */

/* Accelerometer device handle, initialized in main() */
const struct device *accel_sensor;

/*
 * sensor_timer_cb() — LVGL periodic timer callback
 *
 * Called every (200 / CONFIG_SAMPLE_ACCEL_SAMPLING_RATE) ms by the LVGL
 * timer engine. Fetches a new sensor sample, appends the scaled X/Y/Z
 * values to the chart series, and updates the legend labels with the
 * current G-force reading.
 *
 * @param timer  Pointer to the LVGL timer that triggered this callback.
 */
static void sensor_timer_cb(lv_timer_t *timer)
{
	struct sensor_value accel[3];

	/* Trigger a new measurement on the sensor hardware */
	int rc = sensor_sample_fetch(accel_sensor);

	if (rc == 0) {
		/* Copy X, Y, Z channels from the sensor driver's internal buffer */
		rc = sensor_channel_get(accel_sensor, SENSOR_CHAN_ACCEL_XYZ, accel);
	}
	if (rc < 0) {
		LOG_ERR("ERROR: Update failed: %d\n", rc);
		return;
	}

	/* Convert Zephyr sensor_value (fixed-point) to double (m/s²) */
	double ax = sensor_value_to_double(&accel[0]);
	double ay = sensor_value_to_double(&accel[1]);
	double az = sensor_value_to_double(&accel[2]);

	/*
	 * Append scaled integer values to each chart series.
	 * The chart Y-range is ±2000 units (±2 G × ACCEL_SCALE), so values
	 * within ±2 G fill the chart vertically.
	 * LV_CHART_UPDATE_MODE_SHIFT scrolls older data left automatically.
	 */
	lv_chart_set_next_value(chart1, ser_x, (int32_t)(ax * ACCEL_SCALE));
	lv_chart_set_next_value(chart1, ser_y, (int32_t)(ay * ACCEL_SCALE));
	lv_chart_set_next_value(chart1, ser_z, (int32_t)(az * ACCEL_SCALE));

	/* Update legend labels with current G-force values (m/s² ÷ 9.81) */
	lv_label_set_text_fmt(lbl_x, "X:%.1f", ax / G_MS2);
	lv_label_set_text_fmt(lbl_y, "Y:%.1f", ay / G_MS2);
	lv_label_set_text_fmt(lbl_z, "Z:%.1f", az / G_MS2);
}

/*
 * create_legend() — Creates X/Y/Z axis labels overlaid on the chart
 *
 * Because the display is circular, labels are placed where the circle has
 * its widest horizontal span so they remain fully visible:
 *   X (red)   — left side, vertically offset from top
 *   Y (blue)  — top center
 *   Z (green) — right side, vertically offset from top
 *
 * The tiny5 font is used to minimize pixel footprint on the 160×160 screen.
 *
 * @param parent  LVGL object to attach the labels to (usually the screen).
 */
static void create_legend(lv_obj_t *parent)
{
	/* Display is round: X and Z go to left/right mid where the circle
	 * has full width; Y stays at top center as before. */
	static const struct {
		const char    *init;
		lv_color_t     color;
		lv_align_t     align;
		int32_t        x_ofs;
		int32_t        y_ofs;
		lv_obj_t     **lbl;
	} items[] = {
		{"X:--", LV_COLOR_MAKE(255, 0, 0),   LV_ALIGN_TOP_LEFT,  20, 35, &lbl_x},
		{"Y:--", LV_COLOR_MAKE(0, 128, 255), LV_ALIGN_TOP_MID,    0,  2, &lbl_y},
		{"Z:--", LV_COLOR_MAKE(0, 255, 0),   LV_ALIGN_TOP_RIGHT, -20, 35, &lbl_z},
	};

	for (int i = 0; i < 3; i++) {
		lv_obj_t *lbl = lv_label_create(parent);
		lv_label_set_text(lbl, items[i].init);
		lv_obj_set_style_text_font(lbl, &lv_font_tiny5, 0);
		lv_obj_set_style_text_color(lbl, items[i].color, 0);
		lv_obj_align(lbl, items[i].align, items[i].x_ofs, items[i].y_ofs);
		*items[i].lbl = lbl;
	}
}

/*
 * create_accelerometer_chart() — Builds the full-screen scrolling chart UI
 *
 * Sets up a black-background LVGL chart sized to fill the entire 160×160
 * display. Three line series (X=red, Y=blue, Z=green) represent each
 * acceleration axis. The Y-axis range covers ±2 G (scaled to ±2000).
 * Division lines are rendered in dark gray for a subtle grid effect.
 * Point markers are hidden to keep the lines clean at small resolutions.
 *
 * @param parent  LVGL screen object (pass lv_screen_active()).
 */
static void create_accelerometer_chart(lv_obj_t *parent)
{
	/* Set black background on the screen */
	lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

	/* Create chart widget sized to fill the entire display */
	chart1 = lv_chart_create(parent);
	lv_obj_set_size(chart1, LV_HOR_RES, LV_VER_RES);
	lv_obj_align(chart1, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_color(chart1, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(chart1, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(chart1, 0, 0);

	/* Line chart with 5 horizontal and 8 vertical dark-gray grid lines */
	lv_chart_set_type(chart1, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(chart1, 5, 8);
	lv_obj_set_style_line_color(chart1, lv_color_make(40, 40, 40), LV_PART_MAIN);

	/*
	 * Y-axis range: ±2000 units = ±2 G (after ACCEL_SCALE × 100 factor).
	 * Data outside this range is clipped at the chart edges.
	 */
	lv_chart_set_range(chart1, LV_CHART_AXIS_PRIMARY_Y, -2000, 2000);

	/*
	 * SHIFT mode: each new value pushes older values one position to the
	 * left, creating a scrolling oscilloscope-style display.
	 */
	lv_chart_set_update_mode(chart1, LV_CHART_UPDATE_MODE_SHIFT);

	/* Add one data series per axis with its assigned color */
	ser_x = lv_chart_add_series(chart1, lv_color_make(255, 0, 0),
				    LV_CHART_AXIS_PRIMARY_Y);   /* X axis: red   */
	ser_y = lv_chart_add_series(chart1, lv_color_make(0, 128, 255),
				    LV_CHART_AXIS_PRIMARY_Y);   /* Y axis: blue  */
	ser_z = lv_chart_add_series(chart1, lv_color_make(0, 255, 0),
				    LV_CHART_AXIS_PRIMARY_Y);   /* Z axis: green */

	/* Total number of visible data points per series (set via Kconfig) */
	lv_chart_set_point_count(chart1, CONFIG_SAMPLE_CHART_POINTS_PER_SERIES);

	/* Hide circular point markers — lines only for a clean look */
	lv_obj_set_style_size(chart1, 0, 0, LV_PART_INDICATOR);

	/* Labels are created after the chart so they render on top */
	create_legend(parent);
}

/*
 * i2c_scan() — Scans the I2C bus and logs all responding device addresses
 *
 * Iterates through the standard 7-bit I2C address space (0x08–0x77) and
 * attempts a 1-byte read at each address. A successful read (ret == 0)
 * indicates a device is present at that address. Useful for verifying
 * sensor wiring during development.
 * Remove or disable in production firmware.
 *
 * @param i2c_dev  Pointer to the Zephyr I2C device to scan.
 */
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

	/*
	 * I2C bus scan for debug — verifies the FXOS8700CQ is visible at 0x1E.
	 * Remove in production to save boot time and log traffic.
	 */
	const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (device_is_ready(i2c_dev)) {
		i2c_scan(i2c_dev);
	} else {
		LOG_ERR("I2C0 not ready");
	}

	/* Obtain the display device handle from the chosen zephyr,display node */
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return -ENODEV;
	}

	/* Obtain the accelerometer device handle from the accel0 alias */
	accel_sensor = DEVICE_DT_GET(DT_ALIAS(accel0));
	if (!device_is_ready(accel_sensor)) {
		LOG_ERR("Device %s is not ready\n", accel_sensor->name);
		return -ENODEV;
	}

	/*
	 * Build the chart UI on the active LVGL screen.
	 * All LVGL widget creation must happen before the first
	 * lv_timer_handler() call so the initial frame renders correctly.
	 */
	create_accelerometer_chart(lv_screen_active());

	/*
	 * Create a periodic LVGL timer to read sensor data.
	 * Period = 200 / CONFIG_SAMPLE_ACCEL_SAMPLING_RATE ms.
	 * Example: sampling rate = 10 → period = 20 ms → 50 Hz update rate.
	 */
	sensor_timer = lv_timer_create(sensor_timer_cb,
					200 / CONFIG_SAMPLE_ACCEL_SAMPLING_RATE,
					NULL);

	/* Process LVGL tasks once to render the initial empty chart frame */
	lv_timer_handler();

	/* Turn on the display — disable the blanking that is active at boot */
	display_blanking_off(display_dev);

	/*
	 * Main loop: delegate timing entirely to LVGL's internal task scheduler.
	 * lv_timer_handler() returns the number of ms until the next task is due.
	 * Sleeping for that duration prevents busy-waiting while still waking up
	 * in time for the next sensor read or screen redraw.
	 */
	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}
