/*
 * eye_renderer.c — Animated eye for GC9D01 160×160 on nRF52832 with LVGL
 *
 * Port of Bodmer's "Uncanny Eyes" (TFT_eSPI) to Zephyr + LVGL canvas.
 *
 * Key differences from the Arduino version:
 *   - PROGMEM / pgm_read_*  → direct const array access (ARM flash-mapped)
 *   - micros() / millis()   → k_uptime_ticks() / k_uptime_get_32()
 *   - random()              → sys_rand32_get()
 *   - BLINK_PIN             → sw0 GPIO (button0, active-low)
 *   - TFT pushPixels        → lv_canvas direct pixel write + finish_update
 *   - split() recursion     → simple linear iris interpolation state machine
 *
 * Config kept from config.h:
 *   EYE_1_XPOSITION = 20  (x offset of 128×128 canvas on 160-wide display)
 *   SYMMETRICAL_EYELID, AUTOBLINK, TRACKING, NUM_EYES = 1
 *   IRIS_MIN = 90, IRIS_MAX = 130 (autonomous iris scaling)
 *
 * Config removed:
 *   TFT_COUNT, TFT1_CS/TFT2_CS, DISPLAY_BACKLIGHT, LIGHT_PIN, wink pins
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eye_renderer.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <lvgl.h>

/* Eye graphics tables — const arrays go to flash (.rodata) on ARM */
#define SYMMETRICAL_EYELID
#include "eye_data/defaultEye.h"

LOG_MODULE_REGISTER(eye_renderer, CONFIG_LOG_DEFAULT_LEVEL);

/* ── Configuration (from config.h) ──────────────────────────────────── */
#define NUM_EYES          1
#define EYE_1_XPOSITION   20        /* x offset on 160-wide display      */
#define EYE_Y_OFFSET      ((160 - SCREEN_HEIGHT) / 2) /* center vertically */

//#define AUTOBLINK                   /* autonomous blinking                */
#define TRACKING                    /* upper eyelid tracks pupil          */

#define IRIS_MIN          90        /* iris size — smallest (bright light) */
#define IRIS_MAX          130       /* iris size — largest  (dark)         */

/* ── Canvas (128×128 RGB565) ─────────────────────────────────────────── */
#define CANVAS_W   SCREEN_WIDTH     /* 128 */
#define CANVAS_H   SCREEN_HEIGHT    /* 128 */

/*
 * Static pixel buffer: 128×128×2 = 32 768 bytes in BSS (not LVGL heap).
 * Aligned to 4 bytes as required by the LVGL draw-buffer API.
 */
static uint16_t __aligned(4) canvas_buf[CANVAS_W * CANVAS_H];
static lv_draw_buf_t canvas_draw_buf;
static lv_obj_t     *eye_canvas;

/* ── Blink button (sw0 = button0, GPIO_ACTIVE_LOW) ──────────────────── */
#define BLINK_BTN_NODE  DT_ALIAS(sw0)
static const struct gpio_dt_spec blink_btn =
    GPIO_DT_SPEC_GET(BLINK_BTN_NODE, gpios);

/* ── Blink state machine ─────────────────────────────────────────────── */
#define NOBLINK 0   /* not blinking            */
#define ENBLINK 1   /* eyelid closing          */
#define DEBLINK 2   /* eyelid opening          */

typedef struct {
    uint8_t  state;
    uint32_t duration;   /* duration of current blink phase (µs) */
    uint32_t startTime;  /* micros() at phase start              */
} eyeBlink;

static struct {
    eyeBlink blink;
    int16_t  xposition;
} eye[NUM_EYES];

/* ── AUTOBLINK timing ────────────────────────────────────────────────── */
#ifdef AUTOBLINK
static uint32_t timeOfLastBlink = 0;
static uint32_t timeToNextBlink = 0;
#endif

/* ── Iris animation state (replaces recursive split()) ──────────────── */
static uint16_t iris_cur  = (IRIS_MIN + IRIS_MAX) / 2;
static uint16_t iris_tgt  = (IRIS_MIN + IRIS_MAX) / 2;
static uint32_t iris_start_us   = 0;
static uint32_t iris_dur_us     = 1000000; /* 1 s initial */

/* ── Eye movement state ──────────────────────────────────────────────── */
static bool     eyeInMotion      = false;
static int16_t  eyeOldX = 512, eyeOldY = 512;
static int16_t  eyeNewX = 512, eyeNewY = 512;
static uint32_t eyeMoveStartTime = 0;
static int32_t  eyeMoveDuration  = 0;

/* ── Ease curve (from original) ──────────────────────────────────────── */
static const uint8_t ease[] = {
    0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  3,
    3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  9,  9, 10, 10,
   11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23,
   24, 25, 26, 27, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 38, 39,
   40, 41, 42, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 56, 57, 58,
   60, 61, 62, 63, 65, 66, 67, 69, 70, 72, 73, 74, 76, 77, 78, 80,
   81, 83, 84, 85, 87, 88, 90, 91, 93, 94, 96, 97, 98,100,101,103,
  104,106,107,109,110,112,113,115,116,118,119,121,122,124,125,127,
  128,130,131,133,134,136,137,139,140,142,143,145,146,148,149,151,
  152,154,155,157,158,159,161,162,164,165,167,168,170,171,172,174,
  175,177,178,179,181,182,183,185,186,188,189,190,192,193,194,195,
  197,198,199,201,202,203,204,205,207,208,209,210,211,213,214,215,
  216,217,218,219,220,221,222,224,225,226,227,228,228,229,230,231,
  232,233,234,235,236,237,237,238,239,240,240,241,242,243,243,244,
  245,245,246,246,247,248,248,249,249,250,250,251,251,251,252,252,
  252,253,253,253,254,254,254,254,254,255,255,255,255,255,255,255
};

/* ── Helpers ─────────────────────────────────────────────────────────── */

/** Current time in microseconds (32-bit, wraps after ~71 min). */
static uint32_t micros_z(void)
{
    return (uint32_t)k_ticks_to_us_near32((uint32_t)k_uptime_ticks());
}

/**
 * rand_range() - Random integer in [lo, hi).
 * Uses sys_rand32_get() (hardware TRNG on nRF52832).
 */
static uint32_t rand_range(uint32_t lo, uint32_t hi)
{
    if (hi <= lo) {
        return lo;
    }
    return lo + (sys_rand32_get() % (hi - lo));
}

/* ── Eye rendering ───────────────────────────────────────────────────── */

/**
 * draw_eye() - Render one frame of the eye into the LVGL canvas buffer.
 *
 * Pixel transformation mirrors the Arduino code exactly so the same
 * color tables produce the same visual output:
 *   1. R↔B channel swap  (eye data is RGB565; display pipeline = BGR)
 *   2. Byte swap          (matches CONFIG_LV_COLOR_16_SWAP=y in LVGL)
 *
 * If colors appear R↔B inverted on your panel, remove step 1.
 */
static void draw_eye(uint32_t iScale,
                     uint32_t scleraX, uint32_t scleraY,
                     uint32_t uT,      uint32_t lT)
{
    uint32_t screenX, screenY, scleraXsave;
    int32_t  irisX, irisY;
    uint32_t p, a, d;

    scleraXsave = scleraX;
    irisY       = (int32_t)scleraY - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;

    /*
     * For e=0 (single eye): eyelid image is scanned right-to-left
     * (lidX counts down from SCREEN_WIDTH-1 to 0).
     */
    for (screenY = 0; screenY < SCREEN_HEIGHT; screenY++, scleraY++, irisY++) {
        scleraX = scleraXsave;
        irisX   = (int32_t)scleraXsave - (SCLERA_WIDTH - IRIS_WIDTH) / 2;

        uint16_t lidX  = SCREEN_WIDTH - 1; /* start at 127, count down */

        for (screenX = 0; screenX < SCREEN_WIDTH;
             screenX++, scleraX++, irisX++, lidX--) {

            if ((lower[screenY * SCREEN_WIDTH + lidX] <= lT) ||
                (upper[screenY * SCREEN_WIDTH + lidX] <= uT)) {
                /* Covered by eyelid → black */
                p = 0;
            } else if ((irisY < 0) || (irisY >= IRIS_HEIGHT) ||
                       (irisX < 0) || (irisX >= IRIS_WIDTH)) {
                /* Outside iris region → sclera */
                p = sclera[scleraY * SCLERA_WIDTH + scleraX];
            } else {
                /* Inside iris region — polar coordinate lookup */
                p = polar[irisY * IRIS_WIDTH + irisX];
                d = (iScale * (p & 0x7F)) / 128;
                if (d < IRIS_MAP_HEIGHT) {
                    a = (IRIS_MAP_WIDTH * (p >> 7)) / 512;
                    p = iris[d * IRIS_MAP_WIDTH + a];
                } else {
                    p = sclera[scleraY * SCLERA_WIDTH + scleraX];
                }
            }

            /*
             * Eye data tables are RGB565 (R in bits [15:11]).
             * LVGL with LV_COLOR_FORMAT_RGB565 canvas + LV_COLOR_16_SWAP=y
             * applies the byte swap automatically during canvas→VDB
             * compositing, so the SPIM DMA sends the high byte (R+G) first
             * as the GC9D01 expects. No manual transformation needed.
             */
            canvas_buf[screenY * CANVAS_W + screenX] = (uint16_t)p;
        }
    }

    /* Mark canvas dirty so LVGL flushes it on the next lv_timer_handler() */
    lv_obj_invalidate(eye_canvas);
}

/* ── Animation frame ─────────────────────────────────────────────────── */

static void frame(uint16_t iScale)
{
    int16_t  eyeX, eyeY;
    uint32_t t = micros_z();

    /* ── Autonomous X/Y eye movement ─────────────────────────────── */
    int32_t dt = (int32_t)(t - eyeMoveStartTime);

    if (eyeInMotion) {
        if (dt >= eyeMoveDuration) {
            eyeInMotion     = false;
            eyeMoveDuration = (int32_t)rand_range(0, 3000000);
            eyeMoveStartTime = t;
            eyeX = eyeOldX = eyeNewX;
            eyeY = eyeOldY = eyeNewY;
        } else {
            int16_t e = (int16_t)(ease[255 * dt / eyeMoveDuration] + 1);
            eyeX = eyeOldX + (int16_t)(((eyeNewX - eyeOldX) * e) / 256);
            eyeY = eyeOldY + (int16_t)(((eyeNewY - eyeOldY) * e) / 256);
        }
    } else {
        eyeX = eyeOldX;
        eyeY = eyeOldY;
        if (dt > eyeMoveDuration) {
            int16_t  dx, dy;
            uint32_t dist;
            do {
                eyeNewX = (int16_t)rand_range(0, 1024);
                eyeNewY = (int16_t)rand_range(0, 1024);
                dx = (int16_t)((eyeNewX * 2) - 1023);
                dy = (int16_t)((eyeNewY * 2) - 1023);
            } while ((dist = (uint32_t)((int32_t)dx * dx + (int32_t)dy * dy))
                     > (1023u * 1023u));
            (void)dist;
            eyeMoveDuration  = (int32_t)rand_range(72000, 144000);
            eyeMoveStartTime = t;
            eyeInMotion      = true;
        }
    }

    /* ── AUTOBLINK ────────────────────────────────────────────────── */
#ifdef AUTOBLINK
    if ((t - timeOfLastBlink) >= timeToNextBlink) {
        timeOfLastBlink = t;
        uint32_t bd = rand_range(36000, 72000);
        for (int e = 0; e < NUM_EYES; e++) {
            if (eye[e].blink.state == NOBLINK) {
                eye[e].blink.state     = ENBLINK;
                eye[e].blink.startTime = t;
                eye[e].blink.duration  = bd;
            }
        }
        timeToNextBlink = bd * 3 + rand_range(0, 4000000);
    }
#endif

    /* ── Blink state machine (eye 0) ─────────────────────────────── */
    int blink_pressed = gpio_pin_get_dt(&blink_btn); /* 1 = pressed */

    if (eye[0].blink.state) {
        if ((t - eye[0].blink.startTime) >= eye[0].blink.duration) {
            if ((eye[0].blink.state == ENBLINK) && (blink_pressed == 1)) {
                /* Hold closed while button is held */
            } else {
                if (++eye[0].blink.state > DEBLINK) {
                    eye[0].blink.state = NOBLINK;
                } else {
                    eye[0].blink.duration  *= 2;
                    eye[0].blink.startTime  = t;
                }
            }
        }
    } else {
        /* Not blinking — check manual blink button */
        if (blink_pressed == 1) {
            uint32_t bd = rand_range(36000, 72000);
            eye[0].blink.state     = ENBLINK;
            eye[0].blink.startTime = t;
            eye[0].blink.duration  = bd;
        }
    }

    /* ── Scale eye X/Y to sclera pixel units ─────────────────────── */
    eyeX = (int16_t)(((int32_t)eyeX * (SCLERA_WIDTH  - 128)) / 1023);
    eyeY = (int16_t)(((int32_t)eyeY * (SCLERA_HEIGHT - 128)) / 1023);

    if (eyeX > (SCLERA_WIDTH  - 128)) eyeX = SCLERA_WIDTH  - 128;
    if (eyeY > (SCLERA_HEIGHT - 128)) eyeY = SCLERA_HEIGHT - 128;

    /* ── Eyelid tracking ─────────────────────────────────────────── */
    static uint8_t uThreshold = 128;
    uint8_t lThreshold;

#ifdef TRACKING
    int16_t sampleX = SCLERA_WIDTH  / 2 - (eyeX / 2);
    int16_t sampleY = SCLERA_HEIGHT / 2 - (eyeY + IRIS_HEIGHT / 4);
    uint8_t n;
    if (sampleY < 0) {
        n = 0;
    } else {
        n = (uint8_t)((upper[sampleY * SCREEN_WIDTH + sampleX] +
                       upper[sampleY * SCREEN_WIDTH + (SCREEN_WIDTH - 1 - sampleX)]) / 2);
    }
    uThreshold = (uint8_t)((uThreshold * 3 + n) / 4);
    lThreshold = 254 - uThreshold;
#else
    uThreshold = lThreshold = 0;
#endif

    /* ── Apply blink to eyelid thresholds ────────────────────────── */
    uint8_t uT = uThreshold;
    if (eye[0].blink.state) {
        uint32_t s = t - eye[0].blink.startTime;
        if (s >= eye[0].blink.duration) {
            s = 255;
        } else {
            s = 255u * s / eye[0].blink.duration;
        }
        s    = (eye[0].blink.state == DEBLINK) ? 1 + s : 256 - s;
        uT         = (uint8_t)((uThreshold * s + 254u * (257u - s)) / 256u);
        lThreshold = (uint8_t)((lThreshold * s + 254u * (257u - s)) / 256u);
    }

    draw_eye(iScale, (uint32_t)eyeX, (uint32_t)eyeY, uT, lThreshold);
}

/* ── Public API ──────────────────────────────────────────────────────── */

void eye_renderer_init(lv_obj_t *screen)
{
    /* Configure the blink button (GPIO_PULL_UP + GPIO_ACTIVE_LOW in DTS) */
    if (!gpio_is_ready_dt(&blink_btn)) {
        LOG_ERR("Blink button GPIO not ready");
    } else if (gpio_pin_configure_dt(&blink_btn, GPIO_INPUT) < 0) {
        LOG_ERR("Failed to configure blink button");
    }

    /* Black background so the border around the 128×128 eye is invisible */
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    /*
     * Initialise draw buffer with the static pixel array.
     * RGB565 format, LV_STRIDE_AUTO = width * 2 bytes = 256 bytes/row.
     */
    lv_draw_buf_init(&canvas_draw_buf,
                     CANVAS_W, CANVAS_H,
                     LV_COLOR_FORMAT_RGB565,
                     LV_STRIDE_AUTO,
                     canvas_buf,
                     sizeof(canvas_buf));

    /* Create canvas widget and attach the draw buffer */
    eye_canvas = lv_canvas_create(screen);
    lv_canvas_set_draw_buf(eye_canvas, &canvas_draw_buf);
    lv_obj_set_pos(eye_canvas, EYE_1_XPOSITION, EYE_Y_OFFSET);

    /*
     * Zero the pixel buffer directly (black = 0x0000 in any RGB565 byte order).
     * Avoids using lv_canvas_fill_bg() which invokes the full LVGL draw
     * pipeline and can exhaust the LVGL heap during initialisation.
     */
    memset(canvas_buf, 0, sizeof(canvas_buf));

    /* Initialise eye state */
    eye[0].blink.state = NOBLINK;
    eye[0].xposition   = EYE_1_XPOSITION;

    /* Seed iris animation */
    iris_start_us = micros_z();
    iris_dur_us   = 1000000;

    LOG_INF("Eye renderer initialised (canvas at x=%d, y=%d, %dx%d)",
            EYE_1_XPOSITION, EYE_Y_OFFSET, CANVAS_W, CANVAS_H);
}

void eye_renderer_update(void)
{
    uint32_t now = micros_z();
    uint32_t elapsed = now - iris_start_us;

    /* Advance iris interpolation */
    uint16_t iris_scale;

    if (elapsed >= iris_dur_us) {
        iris_cur      = iris_tgt;
        iris_tgt      = (uint16_t)rand_range(IRIS_MIN, IRIS_MAX + 1);
        iris_start_us = now;
        iris_dur_us   = rand_range(300000, 1500000); /* 0.3 – 1.5 s */
        iris_scale    = iris_cur;
    } else {
        int32_t delta = (int32_t)iris_tgt - (int32_t)iris_cur;
        iris_scale = (uint16_t)((int32_t)iris_cur +
                                delta * (int32_t)elapsed / (int32_t)iris_dur_us);
    }

    /* Clamp to valid range */
    if (iris_scale < IRIS_MIN) iris_scale = IRIS_MIN;
    if (iris_scale > IRIS_MAX) iris_scale = IRIS_MAX;

    frame(iris_scale);
}
