/*
 * =============================================================================
 * SH1106 OLED Display Demo (LVGL 9.x)
 * =============================================================================
 * This program demonstrates different ways to draw on a 128x64 monochrome OLED
 * display using the LVGL graphics library on Zephyr RTOS.
 *
 * It cycles through 6 demos:
 *   1. Text      - Display text with different fonts (Montserrat 14, UNSCII 8)
 *   2. Lines     - Draw lines connecting points (triangle + X shape)
 *   3. Arc       - Draw curved arc shapes (circular progress indicators)
 *   4. Image     - Display a 1-bit bitmap image (32x32 smiley face)
 *   5. Canvas    - Draw individual pixels to create custom shapes
 *   6. MP3       - Simulates an old-school MP3 player with scrolling song
 *                  title, progress bar, and time counter (like a Foston)
 *
 * SPDX-License-Identifier: Apache-2.0
 * =============================================================================
 */

/* --- Include headers --------------------------------------------------------
 * These #include lines bring in code from external libraries so we can use
 * their functions. Think of it like importing a toolbox.
 */
#include <zephyr/device.h>        /* Zephyr device model (access hardware peripherals) */
#include <zephyr/devicetree.h>    /* Access hardware configuration from the .overlay file */
#include <zephyr/drivers/display.h> /* Display driver API (blanking on/off, write pixels) */
#include <zephyr/kernel.h>        /* Zephyr kernel (threads, sleep, timing) */
#include <lvgl.h>                 /* LVGL graphics library (widgets, drawing, fonts) */
#include <stdio.h>                /* C standard I/O (snprintf for formatting strings) */
#include <string.h>               /* C standard string functions (memcpy, strlen, etc.) */
#include <stdbool.h>              /* C standard bool type (true/false) */

/* --- Logging setup ----------------------------------------------------------
 * This creates a "log channel" named "app". We can then use LOG_INF() to print
 * informational messages and LOG_ERR() to print error messages to the serial
 * console. LOG_LEVEL_INF means we see info messages and above (warn, error).
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* --- Constants --------------------------------------------------------------
 * #define creates a named constant. Everywhere the compiler sees the name,
 * it substitutes the value. This avoids "magic numbers" scattered in the code.
 */
#define DEMO_DURATION_MS 2000    /* How long each demo is shown (2000 ms = 2 seconds) */
#define SCREEN_WIDTH     128     /* Display width in pixels */
#define SCREEN_HEIGHT    64      /* Display height in pixels */


/* =============================================================================
 * DEMO 1: TEXT
 * =============================================================================
 * Shows how to create text labels with different fonts and positions.
 * LVGL uses "objects" (widgets) - a label is one type of widget.
 */
static void demo_text(void)
{
	/* Get a pointer to the currently active screen.
	 * All widgets must be placed on a screen to be visible. */
	lv_obj_t *scr = lv_screen_active();

	/* --- Title label (large font) --- */
	/* Create a new label widget and attach it to the screen */
	lv_obj_t *title = lv_label_create(scr);
	/* Set the text content of the label */
	lv_label_set_text(title, "SH1106 Demo");
	/* Change the font to Montserrat 14px (a proportional, smooth font) */
	lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
	/* Position the label: centered horizontally, at the top, 2px down */
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

	/* --- Subtitle label (small font) --- */
	lv_obj_t *sub = lv_label_create(scr);
	lv_label_set_text(sub, "128x64 OLED");
	/* UNSCII 8px is a tiny monospace font (each character has equal width) */
	lv_obj_set_style_text_font(sub, &lv_font_unscii_8, 0);
	/* Position: centered both horizontally and vertically, shifted 4px down */
	lv_obj_align(sub, LV_ALIGN_CENTER, 0, 4);

	/* --- Footer label (small font) --- */
	lv_obj_t *footer = lv_label_create(scr);
	lv_label_set_text(footer, "Zephyr + LVGL");
	lv_obj_set_style_text_font(footer, &lv_font_unscii_8, 0);
	/* Position: centered at the bottom, 2px up from edge */
	lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -2);
}


/* =============================================================================
 * DEMO 2: LINES
 * =============================================================================
 * Draws lines by connecting a series of (x, y) coordinate points.
 * We define the points as arrays, then tell LVGL to draw lines through them.
 */

/* Triangle points: top-center, bottom-left, bottom-right, back to top.
 * Each {x, y} pair is a point on the screen. The line widget connects them
 * in order. Repeating the first point closes the triangle. */
static lv_point_precise_t line_points_triangle[] = {
	{64, 5}, {20, 58}, {108, 58}, {64, 5}
};

/* Diagonal line from top-left corner to bottom-right corner */
static lv_point_precise_t line_points_cross[] = {
	{0, 0}, {SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1}
};

/* Diagonal line from top-right corner to bottom-left corner */
static lv_point_precise_t line_points_cross2[] = {
	{SCREEN_WIDTH - 1, 0}, {0, SCREEN_HEIGHT - 1}
};

static void demo_lines(void)
{
	lv_obj_t *scr = lv_screen_active();

	/* Create a "style" object that defines how lines look.
	 * Styles are reusable - we apply the same style to multiple lines.
	 * "static" means this variable persists between function calls (LVGL needs
	 * the style to stay alive as long as the objects using it exist).
	 * IMPORTANT: Only initialize once to avoid memory leak! */
	static lv_style_t style_line;
	static bool style_initialized = false;
	if (!style_initialized) {
		lv_style_init(&style_line);                        /* Initialize the style ONCE */
		lv_style_set_line_width(&style_line, 1);           /* Line thickness: 1 pixel */
		lv_style_set_line_color(&style_line, lv_color_white()); /* Line color: white */
		style_initialized = true;
	}

	/* --- Triangle --- */
	lv_obj_t *tri = lv_line_create(scr);       /* Create a line widget */
	lv_line_set_points(tri, line_points_triangle, 4); /* Connect 4 points */
	lv_obj_add_style(tri, &style_line, 0);     /* Apply our white, 1px style */

	/* --- Diagonal line 1 (top-left to bottom-right) --- */
	lv_obj_t *d1 = lv_line_create(scr);
	lv_line_set_points(d1, line_points_cross, 2);  /* Connect 2 points */
	lv_obj_add_style(d1, &style_line, 0);

	/* --- Diagonal line 2 (top-right to bottom-left) --- */
	lv_obj_t *d2 = lv_line_create(scr);
	lv_line_set_points(d2, line_points_cross2, 2);
	lv_obj_add_style(d2, &style_line, 0);
}


/* =============================================================================
 * DEMO 3: ARC
 * =============================================================================
 * An arc is a curved line segment (part of a circle). It's commonly used for
 * circular progress indicators or gauges. You set the start/end angles and
 * a value within a range.
 */
static void demo_arc(void)
{
	lv_obj_t *scr = lv_screen_active();

	/* Title label */
	lv_obj_t *label = lv_label_create(scr);
	lv_label_set_text(label, "Arc");
	lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 2);

	/* --- Large arc (75% filled, 270 degree sweep) --- */
	lv_obj_t *arc = lv_arc_create(scr);       /* Create an arc widget */
	lv_arc_set_range(arc, 0, 100);             /* Value range: 0 to 100 */
	lv_arc_set_value(arc, 75);                 /* Current value: 75 (so 75% filled) */
	lv_arc_set_bg_angles(arc, 0, 270);         /* Background arc spans 0 to 270 degrees */
	lv_obj_set_size(arc, 50, 50);              /* Widget size: 50x50 pixels */
	lv_obj_align(arc, LV_ALIGN_CENTER, -20, 6); /* Position: left of center */
	lv_obj_remove_style(arc, NULL, LV_PART_KNOB); /* Hide the knob (drag handle) */

	/* --- Small arc (40% filled, full 360 degree circle) --- */
	lv_obj_t *arc2 = lv_arc_create(scr);
	lv_arc_set_range(arc2, 0, 100);
	lv_arc_set_value(arc2, 40);                /* 40% filled */
	lv_arc_set_bg_angles(arc2, 0, 360);        /* Full circle background */
	lv_obj_set_size(arc2, 30, 30);             /* Smaller: 30x30 pixels */
	lv_obj_align(arc2, LV_ALIGN_CENTER, 35, 6); /* Position: right of center */
	lv_obj_remove_style(arc2, NULL, LV_PART_KNOB);
}


/* =============================================================================
 * DEMO 4: BITMAP IMAGE
 * =============================================================================
 * Displays a pre-defined bitmap image. The image is stored as an array of bytes
 * where each bit represents one pixel (1 = visible, 0 = transparent).
 * This is a 32x32 pixel smiley face.
 */

/* The bitmap data: each row is 32 pixels = 4 bytes (8 pixels per byte).
 * A '1' bit means that pixel is drawn (white on OLED).
 * A '0' bit means that pixel is transparent (shows background).
 * The most significant bit (MSB) is the leftmost pixel in each byte. */
static const uint8_t smiley_map[] = {
	0x00, 0x03, 0xC0, 0x00,  /* Row 0:  ......####...... (top of circle) */
	0x00, 0x1F, 0xF8, 0x00,  /* Row 1:  ...##########... */
	0x00, 0x7F, 0xFE, 0x00,  /* Row 2:  .##############. */
	0x00, 0xFF, 0xFF, 0x00,  /* Row 3  */
	0x01, 0xFF, 0xFF, 0x80,  /* Row 4  */
	0x03, 0xFF, 0xFF, 0xC0,  /* Row 5  */
	0x07, 0xFF, 0xFF, 0xE0,  /* Row 6  */
	0x0F, 0xFF, 0xFF, 0xF0,  /* Row 7  */
	0x0F, 0xFF, 0xFF, 0xF0,  /* Row 8  */
	0x1F, 0x9F, 0xF9, 0xF8,  /* Row 9:  eyes start (gaps in the filled circle) */
	0x1F, 0x0F, 0xF0, 0xF8,  /* Row 10: eyes (larger gaps) */
	0x3F, 0x0F, 0xF0, 0xFC,  /* Row 11 */
	0x3F, 0x0F, 0xF0, 0xFC,  /* Row 12 */
	0x3F, 0x9F, 0xF9, 0xFC,  /* Row 13: eyes end */
	0x3F, 0xFF, 0xFF, 0xFC,  /* Row 14 */
	0x3F, 0xFF, 0xFF, 0xFC,  /* Row 15 */
	0x3F, 0xFF, 0xFF, 0xFC,  /* Row 16 */
	0x3F, 0xFF, 0xFF, 0xFC,  /* Row 17 */
	0x3F, 0xFF, 0xFF, 0xFC,  /* Row 18 */
	0x3E, 0xFF, 0xFF, 0x7C,  /* Row 19: mouth starts (gaps form a smile) */
	0x1E, 0x7F, 0xFE, 0x78,  /* Row 20 */
	0x1F, 0x3F, 0xFC, 0xF8,  /* Row 21 */
	0x0F, 0x9F, 0xF9, 0xF0,  /* Row 22 */
	0x0F, 0xC0, 0x03, 0xF0,  /* Row 23: mouth (big gap = wide smile) */
	0x07, 0xF0, 0x0F, 0xE0,  /* Row 24 */
	0x03, 0xFF, 0xFF, 0xC0,  /* Row 25 */
	0x01, 0xFF, 0xFF, 0x80,  /* Row 26 */
	0x00, 0xFF, 0xFF, 0x00,  /* Row 27 */
	0x00, 0x7F, 0xFE, 0x00,  /* Row 28 */
	0x00, 0x1F, 0xF8, 0x00,  /* Row 29 */
	0x00, 0x03, 0xC0, 0x00,  /* Row 30: bottom of circle */
	0x00, 0x00, 0x00, 0x00,  /* Row 31: empty row */
};

/* Image descriptor: tells LVGL how to interpret the raw byte array above.
 * This struct provides metadata about the image format and dimensions. */
static const lv_image_dsc_t smiley_img = {
	.header = {
		.magic = LV_IMAGE_HEADER_MAGIC, /* Magic number so LVGL knows this is a valid image */
		.cf = LV_COLOR_FORMAT_A1,       /* Color format: A1 = 1-bit alpha (each bit = 1 pixel) */
		.w = 32,                        /* Image width: 32 pixels */
		.h = 32,                        /* Image height: 32 pixels */
		.stride = 4,                    /* Bytes per row: 32 pixels / 8 bits = 4 bytes */
	},
	.data_size = sizeof(smiley_map),    /* Total size of pixel data in bytes */
	.data = smiley_map,                 /* Pointer to the actual pixel data array */
};

static void demo_image(void)
{
	lv_obj_t *scr = lv_screen_active();

	/* Title */
	lv_obj_t *label = lv_label_create(scr);
	lv_label_set_text(label, "Bitmap");
	lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 2);

	/* Create an image widget and set our smiley bitmap as its source */
	lv_obj_t *img = lv_image_create(scr);     /* Create image widget */
	lv_image_set_src(img, &smiley_img);        /* Point it to our bitmap data */
	lv_obj_align(img, LV_ALIGN_CENTER, 0, 6);  /* Center it on screen */
}


/* =============================================================================
 * DEMO 5: CANVAS (Pixel-by-pixel drawing)
 * =============================================================================
 * A canvas is a special widget where you can set individual pixels.
 * Unlike other widgets (which are pre-made shapes), the canvas gives you
 * full control to draw anything pixel by pixel.
 *
 * We use L8 format (8 bits per pixel, grayscale) because it's the simplest
 * format that works reliably with lv_canvas_set_px().
 * The buffer size is: width * height * 1 byte = 80 * 48 = 3840 bytes.
 */
#define CANVAS_W 80   /* Canvas width (smaller than screen to save RAM) */
#define CANVAS_H 48   /* Canvas height */

/* Static buffer in RAM to hold the canvas pixel data.
 * LV_CANVAS_BUF_SIZE() calculates the exact size needed including alignment.
 * Parameters: width, height, bits_per_pixel, stride_alignment */
static uint8_t canvas_buf[LV_CANVAS_BUF_SIZE(CANVAS_W, CANVAS_H, 8, 1)];

/* Helper function: draw a horizontal line from x1 to x2 at height y */
static void canvas_draw_hline(lv_obj_t *canvas, int32_t x1, int32_t x2, int32_t y)
{
	for (int32_t x = x1; x <= x2; x++) {
		/* Set one pixel to white at position (x, y) with full opacity */
		lv_canvas_set_px(canvas, x, y, lv_color_white(), LV_OPA_COVER);
	}
}

/* Helper function: draw a vertical line from y1 to y2 at column x */
static void canvas_draw_vline(lv_obj_t *canvas, int32_t x, int32_t y1, int32_t y2)
{
	for (int32_t y = y1; y <= y2; y++) {
		lv_canvas_set_px(canvas, x, y, lv_color_white(), LV_OPA_COVER);
	}
}

static void demo_canvas(void)
{
	lv_obj_t *scr = lv_screen_active();

	/* Create canvas widget and assign our RAM buffer to it */
	lv_obj_t *canvas = lv_canvas_create(scr);
	lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_W, CANVAS_H,
			     LV_COLOR_FORMAT_L8);  /* L8 = 8-bit grayscale per pixel */
	lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);

	/* Fill entire canvas with black (all pixels off) */
	lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

	/* Draw outer border (rectangle around the canvas edges) */
	canvas_draw_hline(canvas, 0, CANVAS_W - 1, 0);            /* Top edge */
	canvas_draw_hline(canvas, 0, CANVAS_W - 1, CANVAS_H - 1); /* Bottom edge */
	canvas_draw_vline(canvas, 0, 0, CANVAS_H - 1);            /* Left edge */
	canvas_draw_vline(canvas, CANVAS_W - 1, 0, CANVAS_H - 1); /* Right edge */

	/* Draw a smaller inner rectangle (8 pixels from each edge) */
	canvas_draw_hline(canvas, 8, CANVAS_W - 9, 8);
	canvas_draw_hline(canvas, 8, CANVAS_W - 9, CANVAS_H - 9);
	canvas_draw_vline(canvas, 8, 8, CANVAS_H - 9);
	canvas_draw_vline(canvas, CANVAS_W - 9, 8, CANVAS_H - 9);

	/* Draw an "X" shape inside the inner rectangle.
	 * We calculate diagonal positions mathematically:
	 * as 'i' increases (going down), 'x' moves proportionally across. */
	int32_t inner_w = CANVAS_W - 18;  /* Width of inner area */
	int32_t inner_h = CANVAS_H - 18;  /* Height of inner area */

	for (int i = 0; i < inner_h; i++) {
		/* Left-to-right diagonal */
		int32_t x = 9 + (i * inner_w) / inner_h;
		int32_t y = 9 + i;
		lv_canvas_set_px(canvas, x, y, lv_color_white(), LV_OPA_COVER);

		/* Right-to-left diagonal */
		x = CANVAS_W - 10 - (i * inner_w) / inner_h;
		lv_canvas_set_px(canvas, x, y, lv_color_white(), LV_OPA_COVER);
	}

	/* Draw a dot pattern in each corner of the canvas (decorative) */
	for (int32_t dy = 2; dy <= 6; dy += 2) {
		for (int32_t dx = 2; dx <= 6; dx += 2) {
			/* Top-left corner */
			lv_canvas_set_px(canvas, dx, dy,
					 lv_color_white(), LV_OPA_COVER);
			/* Top-right corner */
			lv_canvas_set_px(canvas, CANVAS_W - 1 - dx, dy,
					 lv_color_white(), LV_OPA_COVER);
			/* Bottom-left corner */
			lv_canvas_set_px(canvas, dx, CANVAS_H - 1 - dy,
					 lv_color_white(), LV_OPA_COVER);
			/* Bottom-right corner */
			lv_canvas_set_px(canvas, CANVAS_W - 1 - dx, CANVAS_H - 1 - dy,
					 lv_color_white(), LV_OPA_COVER);
		}
	}
}


/* =============================================================================
 * DEMO 6: MP3 PLAYER (scrolling text + progress bar)
 * =============================================================================
 * Simulates an old-school MP3 player (like a Foston):
 * - Song title scrolls horizontally across the screen
 * - A progress bar fills up over time (like a playback timer)
 * - Current time / total time is displayed
 *
 * This demo is self-contained: it runs its own animation loop for 6 seconds.
 */
#define MP3_DEMO_DURATION_MS 6000  /* Total playback simulation: 6 seconds */
#define MP3_SONG_TOTAL_SEC   210   /* Fake song length: 3:30 = 210 seconds */

static void demo_mp3(void)
{
	lv_obj_t *scr = lv_screen_active();

	/* --- "Now Playing" title at the top --- */
	lv_obj_t *title = lv_label_create(scr);
	lv_label_set_text(title, "> Now Playing");
	lv_obj_set_style_text_font(title, &lv_font_unscii_8, 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 2, 2);

	/* --- Song name (scrolling text) ---
	 * The label is given a fixed width smaller than the text.
	 * LV_LABEL_LONG_MODE_SCROLL makes it scroll back and forth automatically.
	 * LVGL handles the animation as long as lv_task_handler() is called. */
	lv_obj_t *song = lv_label_create(scr);
	lv_label_set_text(song, "Linkin Park - In The End (Hybrid Theory 2000)");
	lv_obj_set_style_text_font(song, &lv_font_unscii_8, 0);
	lv_obj_set_width(song, SCREEN_WIDTH - 4);  /* Constrain width to force scroll */
	lv_label_set_long_mode(song, LV_LABEL_LONG_MODE_SCROLL);
	/* Scroll speed in pixels per second. Lower = smoother on small screens.
	 * lv_anim_speed(20) encodes "20 pixels/sec" into the duration property.
	 * Default is 40px/s which is too jumpy for a 128px wide display. */
	lv_obj_set_style_anim_duration(song, lv_anim_speed(20), 0);
	lv_obj_align(song, LV_ALIGN_TOP_LEFT, 2, 16);

	/* --- Progress bar ---
	 * A bar widget that we fill from 0% to 100% during the demo. */
	lv_obj_t *bar = lv_bar_create(scr);
	lv_obj_set_size(bar, SCREEN_WIDTH - 10, 8);   /* Almost full width, 8px tall */
	lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -16);
	lv_bar_set_range(bar, 0, 100);                 /* Range: 0 to 100% */
	lv_bar_set_value(bar, 0, LV_ANIM_OFF);         /* Start at 0% */

	/* --- Time label (e.g., "0:00 / 3:30") --- */
	lv_obj_t *time_label = lv_label_create(scr);
	lv_label_set_text(time_label, "0:00 / 3:30");
	lv_obj_set_style_text_font(time_label, &lv_font_unscii_8, 0);
	lv_obj_align(time_label, LV_ALIGN_BOTTOM_MID, 0, -4);

	/* --- Animation loop ---
	 * We manually update the bar value and time text every 100ms.
	 * Over 6 seconds, the bar goes from 0% to 100% and the time counts up
	 * proportionally to the fake 3:30 song duration. */
	char time_str[16];
	int elapsed_ms = 0;

	while (elapsed_ms < MP3_DEMO_DURATION_MS) {
		/* Calculate progress percentage (0-100) */
		int progress = (elapsed_ms * 100) / MP3_DEMO_DURATION_MS;
		lv_bar_set_value(bar, progress, LV_ANIM_OFF);

		/* Calculate fake song time based on progress.
		 * If song is 3:30 (210s), map elapsed demo time to song time. */
		int song_sec = (elapsed_ms * MP3_SONG_TOTAL_SEC) / MP3_DEMO_DURATION_MS;
		int min = song_sec / 60;
		int sec = song_sec % 60;
		snprintf(time_str, sizeof(time_str), "%d:%02d / 3:30", min, sec);
		lv_label_set_text(time_label, time_str);

		/* Let LVGL process rendering and scroll animation.
		 * 30ms interval = ~33 FPS, smooth enough for scrolling text. */
		lv_task_handler();
		k_sleep(K_MSEC(30));
		elapsed_ms += 30;
	}

	/* Final state: bar full, time at 3:30 */
	lv_bar_set_value(bar, 100, LV_ANIM_OFF);
	lv_label_set_text(time_label, "3:30 / 3:30");
	lv_task_handler();
}


/* =============================================================================
 * HELPER: Clear the screen
 * =============================================================================
 * Removes all widgets from the current screen so the next demo starts fresh.
 * lv_obj_clean() deletes all child objects of the given parent.
 */
static void clear_screen(void)
{
	lv_obj_clean(lv_screen_active());
}


/* =============================================================================
 * MAIN FUNCTION - Program entry point
 * =============================================================================
 * This is where the program starts executing. It:
 * 1. Gets a reference to the display hardware
 * 2. Turns the display on
 * 3. Loops forever, showing each demo for 2 seconds
 */
int main(void)
{
	/* Declare a pointer to a "device" structure.
	 * In Zephyr, all hardware peripherals are represented as "devices".
	 * This pointer will hold the reference to our display hardware. */
	const struct device *display_dev;

	/* Get the display device that was configured in the .overlay file.
	 * DT_CHOSEN(zephyr_display) looks up "zephyr,display" in the devicetree,
	 * which points to our SH1106 OLED. DEVICE_DT_GET() returns a pointer
	 * to the device structure for that hardware. */
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	/* Check if the display driver initialized successfully.
	 * If not (e.g., wrong wiring, driver not enabled), log an error and stop. */
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready");
		return 0;
	}

	LOG_INF("SH1106 Display Demo started (LVGL 9.x)");

	/* Call the LVGL task handler once to process any pending initialization.
	 * lv_task_handler() is LVGL's main "do work" function - it processes
	 * events, redraws dirty areas, and handles animations. */
	lv_task_handler();

	/* Turn off display blanking (= turn the display ON).
	 * "Blanking" means the display shows nothing (all black). */
	display_blanking_off(display_dev);

	/* --- Demo function table ---
	 * We store pointers to all demo functions in an array.
	 * This lets us cycle through them using an index number.
	 * "typedef void (*demo_fn)(void)" creates a type for "pointer to a
	 * function that takes no arguments and returns nothing". */
	typedef void (*demo_fn)(void);
	static const demo_fn demos[] = {
		demo_text,      /* Demo 1 */
		demo_lines,     /* Demo 2 */
		demo_arc,       /* Demo 3 */
		demo_image,     /* Demo 4 */
		demo_canvas,    /* Demo 5 */
		demo_mp3,       /* Demo 6: MP3 player (has its own 6-second loop) */
	};
	static const char *demo_names[] = {
		"Text", "Lines", "Arc", "Image", "Canvas", "MP3"
	};
	/* ARRAY_SIZE() is a macro that calculates how many elements are in an array */
	const int num_demos = ARRAY_SIZE(demos);

	int current = 0;  /* Index of the currently showing demo */

	/* --- Main loop (runs forever) ---
	 * Each iteration: clear screen, run a demo, wait 2 seconds, next demo. */
	while (1) {
		/* Log which demo is about to be shown */
		LOG_INF("Demo %d/%d: %s", current + 1, num_demos,
			demo_names[current]);

		/* Remove all widgets from previous demo */
		clear_screen();

		/* Call the current demo function (creates new widgets on screen) */
		demos[current]();

		/* Tell LVGL to render the new widgets to the display buffer */
		lv_task_handler();

		/* Keep the demo visible for DEMO_DURATION_MS milliseconds.
		 * We call lv_task_handler() every 30ms to keep LVGL responsive
		 * (handles animations, redraws, etc.) */
		for (int t = 0; t < DEMO_DURATION_MS; t += 30) {
			lv_task_handler();
			k_sleep(K_MSEC(30));  /* Sleep 30ms (saves CPU power) */
		}

		/* Move to the next demo. The % (modulo) operator wraps around:
		 * after the last demo (index 4), it goes back to 0. */
		current = (current + 1) % num_demos;
	}

	return 0;  /* Never reached, but required by the int main() signature */
}
