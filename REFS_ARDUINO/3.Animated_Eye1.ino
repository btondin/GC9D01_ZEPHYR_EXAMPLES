// An adaption of the "UncannyEyes" sketch (see eye_functions tab)
// for the TFT_eSPI library. As written the sketch is for driving
// one (240x320 minimum) TFT display, showing 2 eyes. See example
// Animated_Eyes_2 for a dual 128x128 TFT display configured sketch.

// The size of the displayed eye is determined by the screen size and
// resolution. The eye image is 128 pixels wide. In humans the palpebral
// fissure (open eye) width is about 30mm so a low resolution, large
// pixel size display works best to show a scale eye image. Note that
// display manufacturers usually quote the diagonal measurement, so a
// 128 x 128 1.7" display or 128 x 160 2" display is about right.

// Configuration settings for the eye, eye style, display count,
// chip selects and x offsets can be defined in the sketch "config.h" tab.

// Performance (frames per second = fps) can be improved by using
// DMA (for SPI displays only) on ESP32 and STM32 processors. Use
// as high a SPI clock rate as is supported by the display. 27MHz
// minimum, some displays can be operated at higher clock rates in
// the range 40-80MHz.

// Single defaultEye performance for dic:\Users\bruno\Documents\Arduino\libraries\TFT_eSPI\User_Setup.hfferent processors
//                                  No DMA   With DMA
// ESP8266 (160MHz CPU) 40MHz SPI   36 fps
// ESP32 27MHz SPI                  53 fps     85 fps
// ESP32 40MHz SPI                  67 fps    102 fps
// ESP32 80MHz SPI                  82 fps    116 fps // Note: Few displays work reliably at 80MHz
// STM32F401 55MHz SPI              44 fps     90 fps
// STM32F446 55MHz SPI              83 fps    155 fps
// STM32F767 55MHz SPI             136 fps    197 fps

// DMA can be used with RP2040, STM32 and ESP32 processors when the interface
// is SPI, uncomment the next line:
//#define USE_DMA

// Load TFT driver library
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft;           // A single instance is used for 1 or 2 displays

// A pixel buffer is used during eye rendering
#define BUFFER_SIZE 1024 // 128 to 1024 seems optimum

#ifdef USE_DMA
  #define BUFFERS 2      // 2 toggle buffers with DMA
#else
  #define BUFFERS 1      // 1 buffer for no DMA
#endif

uint16_t pbuffer[BUFFERS][BUFFER_SIZE]; // Pixel rendering buffer
bool     dmaBuf   = 0;                  // DMA buffer selection

// This struct is populated in config.h
typedef struct {        // Struct is defined before including config.h --
  int8_t  select;       // pin numbers for each eye's screen select line
  int8_t  wink;         // and wink button (or -1 if none) specified there,
  uint8_t rotation;     // also display rotation and the x offset
  int16_t xposition;    // position of eye on the screen
} eyeInfo_t;

#include "config.h"     // ****** CONFIGURATION IS DONE IN HERE ******

extern void user_setup(void); // Functions in the user*.cpp files
extern void user_loop(void);

#define SCREEN_X_START 0
#define SCREEN_X_END   SCREEN_WIDTH   // Badly named, actually the "eye" width!
#define SCREEN_Y_START 0
#define SCREEN_Y_END   SCREEN_HEIGHT  // Actually "eye" height

// A simple state machine is used to control eye blinks/winks:
#define NOBLINK 0       // Not currently engaged in a blink
#define ENBLINK 1       // Eyelid is currently closing
#define DEBLINK 2       // Eyelid is currently opening
typedef struct {
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duration of blink state (micros)
  uint32_t startTime;   // Time (micros) of last state change
} eyeBlink;

struct {                // One-per-eye structure
  int16_t   tft_cs;     // Chip select pin for each display
  eyeBlink  blink;      // Current blink/wink state
  int16_t   xposition;  // x position of eye image
} eye[NUM_EYES];

uint32_t startTime;  // For FPS indicator

// INITIALIZATION -- runs once at startup ----------------------------------
void setup(void) {
  Serial.begin(115200);
  tft.writecommand(TFT_MADCTL);
  delay(10);
  tft.writedata(TFT_MAD_MX | TFT_MAD_MV );
  //while (!Serial);
  Serial.println("Starting");

#if defined(DISPLAY_BACKLIGHT) && (DISPLAY_BACKLIGHT >= 0)
  // Enable backlight pin, initially off
  Serial.println("Backlight turned off");
  pinMode(DISPLAY_BACKLIGHT, OUTPUT);
  digitalWrite(DISPLAY_BACKLIGHT, LOW);
#endif

  // User call for additional features
  user_setup();

  // Initialise the eye(s), this will set all chip selects low for the tft.init()
  initEyes();

  // Initialise TFT
  Serial.println("Initialising displays");
  tft.init();

  

// GC9D01 custom init
tft.startWrite();

tft.writecommand(0xFE);
tft.writecommand(0xEF);

tft.writecommand(0x80); tft.writedata(0xFF);
tft.writecommand(0x81); tft.writedata(0xFF);
tft.writecommand(0x82); tft.writedata(0xFF);
tft.writecommand(0x83); tft.writedata(0xFF);
tft.writecommand(0x84); tft.writedata(0xFF);
tft.writecommand(0x85); tft.writedata(0xFF);
tft.writecommand(0x86); tft.writedata(0xFF);
tft.writecommand(0x87); tft.writedata(0xFF);
tft.writecommand(0x88); tft.writedata(0xFF);
tft.writecommand(0x89); tft.writedata(0xFF);
tft.writecommand(0x8A); tft.writedata(0xFF);
tft.writecommand(0x8B); tft.writedata(0xFF);
tft.writecommand(0x8C); tft.writedata(0xFF);
tft.writecommand(0x8D); tft.writedata(0xFF);
tft.writecommand(0x8E); tft.writedata(0xFF);
tft.writecommand(0x8F); tft.writedata(0xFF);

tft.writecommand(0x3A); tft.writedata(0x05);
tft.writecommand(0xEC); tft.writedata(0x01);

tft.writecommand(0x74);
tft.writedata(0x02); tft.writedata(0x0E); tft.writedata(0x00);
tft.writedata(0x00); tft.writedata(0x00); tft.writedata(0x00);
tft.writedata(0x00);

tft.writecommand(0x98); tft.writedata(0x3E);
tft.writecommand(0x99); tft.writedata(0x3E);

tft.writecommand(0xB5); tft.writedata(0x0D); tft.writedata(0x0D);

tft.writecommand(0x60);
tft.writedata(0x38); tft.writedata(0x0F); tft.writedata(0x79); tft.writedata(0x67);

tft.writecommand(0x61);
tft.writedata(0x38); tft.writedata(0x11); tft.writedata(0x79); tft.writedata(0x67);

tft.writecommand(0x64);
tft.writedata(0x38); tft.writedata(0x17); tft.writedata(0x71);
tft.writedata(0x5F); tft.writedata(0x79); tft.writedata(0x67);

tft.writecommand(0x65);
tft.writedata(0x38); tft.writedata(0x13); tft.writedata(0x71);
tft.writedata(0x5B); tft.writedata(0x79); tft.writedata(0x67);

tft.writecommand(0x6A); tft.writedata(0x00); tft.writedata(0x00);

tft.writecommand(0x6C);
tft.writedata(0x22); tft.writedata(0x02); tft.writedata(0x22);
tft.writedata(0x02); tft.writedata(0x22); tft.writedata(0x22);
tft.writedata(0x50);

tft.writecommand(0x6E);
tft.writedata(0x03); tft.writedata(0x03); tft.writedata(0x01);
tft.writedata(0x01); tft.writedata(0x00); tft.writedata(0x00);
tft.writedata(0x0f); tft.writedata(0x0f); tft.writedata(0x0d);
tft.writedata(0x0d); tft.writedata(0x0b); tft.writedata(0x0b);
tft.writedata(0x09); tft.writedata(0x09); tft.writedata(0x00);
tft.writedata(0x00); tft.writedata(0x00); tft.writedata(0x00);
tft.writedata(0x0a); tft.writedata(0x0a); tft.writedata(0x0c);
tft.writedata(0x0c); tft.writedata(0x0e); tft.writedata(0x0e);
tft.writedata(0x10); tft.writedata(0x10); tft.writedata(0x00);
tft.writedata(0x00); tft.writedata(0x02); tft.writedata(0x02);
tft.writedata(0x04); tft.writedata(0x04);

tft.writecommand(0xBF); tft.writedata(0x01);
tft.writecommand(0xF9); tft.writedata(0x40);
tft.writecommand(0x9B); tft.writedata(0x3B);

tft.writecommand(0x93);
tft.writedata(0x33); tft.writedata(0x7F); tft.writedata(0x00);

tft.writecommand(0x7E); tft.writedata(0x30);

tft.writecommand(0x70);
tft.writedata(0x0d); tft.writedata(0x02); tft.writedata(0x08);
tft.writedata(0x0d); tft.writedata(0x02); tft.writedata(0x08);

tft.writecommand(0x71);
tft.writedata(0x0d); tft.writedata(0x02); tft.writedata(0x08);

tft.writecommand(0x91); tft.writedata(0x0E); tft.writedata(0x09);

tft.writecommand(0xC3); tft.writedata(0x19);
tft.writecommand(0xC4); tft.writedata(0x19);
tft.writecommand(0xC9); tft.writedata(0x3C);

tft.writecommand(0xF0);
tft.writedata(0x53); tft.writedata(0x15); tft.writedata(0x0a);
tft.writedata(0x04); tft.writedata(0x00); tft.writedata(0x3e);

tft.writecommand(0xF2);
tft.writedata(0x53); tft.writedata(0x15); tft.writedata(0x0a);
tft.writedata(0x04); tft.writedata(0x00); tft.writedata(0x3a);

tft.writecommand(0xF1);
tft.writedata(0x56); tft.writedata(0xa8); tft.writedata(0x7f);
tft.writedata(0x33); tft.writedata(0x34); tft.writedata(0x5f);

tft.writecommand(0xF3);
tft.writedata(0x52); tft.writedata(0xa4); tft.writedata(0x7f);
tft.writedata(0x33); tft.writedata(0x34); tft.writedata(0xdf);

tft.writecommand(0x36); tft.writedata(0x00);
//tft.writecommand(0x36); tft.writedata(0x08); // <<<<<<<<<< bgr
tft.writecommand(0x11);

tft.endWrite();
delay(200);

tft.startWrite();
tft.writecommand(0x29); // Display ON
tft.writecommand(0x2C); // Memory write
tft.endWrite();
   //tft.setRotation(1);
  
   tft.invertDisplay(0);
#ifdef USE_DMA
  tft.initDMA();
#endif

  // Raise chip select(s) so that displays can be individually configured
  digitalWrite(eye[0].tft_cs, HIGH);
  if (NUM_EYES > 1) digitalWrite(eye[1].tft_cs, HIGH);

  for (uint8_t e = 0; e < NUM_EYES; e++) {
    digitalWrite(eye[e].tft_cs, LOW);
    tft.setRotation(eyeInfo[e].rotation);
    tft.fillScreen(TFT_BLACK);
    digitalWrite(eye[e].tft_cs, HIGH);
  }



#if defined(DISPLAY_BACKLIGHT) && (DISPLAY_BACKLIGHT >= 0)
  Serial.println("Backlight now on!");
  analogWrite(DISPLAY_BACKLIGHT, BACKLIGHT_MAX);
#endif

  startTime = millis(); // For frame-rate calculation
}

// MAIN LOOP -- runs continuously after setup() ----------------------------
void loop() {
  updateEye();
}
