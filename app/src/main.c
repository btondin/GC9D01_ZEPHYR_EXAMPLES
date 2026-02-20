/*
 * main.c — Animated Eye demo for GC9D01 160×160 on Bruno nRF52832
 *
 * Replicates Bodmer's "UncannyEyes" (Arduino / TFT_eSPI) using LVGL canvas.
 *
 * Single eye, 128×128 pixels, centred vertically on the 160×160 round display.
 * button0 (sw0, P0.25) triggers a manual blink.
 * Eyes also blink autonomously (AUTOBLINK) and the iris scales randomly.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>

#include <lvgl.h>
#include "eye_renderer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
    const struct device *display_dev =
        DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display not ready");
        return -ENODEV;
    }

    eye_renderer_init(lv_screen_active());

    /* Initial render before enabling the display */
    lv_timer_handler();

    display_blanking_off(display_dev);

    LOG_INF("Animated eye started");

    while (1) {
        eye_renderer_update();

        /*
         * Let LVGL flush the dirty canvas to the GC9D01.
         * lv_timer_handler() returns the ms until next internal task.
         * We cap the sleep at 5 ms so the animation loop stays responsive.
         */
        uint32_t sleep_ms = lv_timer_handler();
        k_msleep(MIN(sleep_ms, 5u));
    }

    return 0;
}
