#pragma once

#include <lvgl.h>

/**
 * eye_renderer_init() - Initialize the eye canvas and GPIO blink button.
 *
 * @param screen  Active LVGL screen (lv_screen_active()).
 */
void eye_renderer_init(lv_obj_t *screen);

/**
 * eye_renderer_update() - Advance the animation by one frame and redraw.
 *
 * Call this as fast as possible from the main loop, right before
 * lv_timer_handler().
 */
void eye_renderer_update(void);
