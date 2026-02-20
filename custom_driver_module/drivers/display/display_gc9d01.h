/*
 * GC9D01 Display Driver — Register Definitions and Configuration Structures
 *
 * The GC9D01 is a single-chip controller/driver for 160x160 round TFT LCD panels
 * manufactured by GalaxyCore. It communicates over SPI using the MIPI DBI Type-C
 * (4-wire SPI) protocol and shares its register map with the GC9X01X family.
 *
 * This header is included by the C driver (display_gc9d01.c) and must NOT be
 * included directly by application code. It provides:
 *   - MIPI DCS command opcodes for display control
 *   - MADCTL (Memory Access Control) bit field masks for rotation/mirroring
 *   - PIXFMT (Pixel Format) values for color depth selection
 *   - Timing constants (sleep in/out mandatory delay)
 *   - Register payload length constants used to size initialization arrays
 *   - gc9d01_regs struct holding DTS-supplied initialization values
 *   - GC9D01_REGS_INIT macro to populate the struct from Devicetree at build time
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_GC9D01_H_
#define ZEPHYR_DRIVERS_DISPLAY_GC9D01_H_

#include <zephyr/sys/util.h>

/* ---------------------------------------------------------------------------
 * MIPI DCS Command Opcodes
 * ---------------------------------------------------------------------------
 * These are the 8-bit command bytes sent over SPI with the D/C (Data/Command)
 * line held LOW. They follow the MIPI Display Command Set (DCS) standard where
 * applicable, with GalaxyCore vendor-specific extensions for power, gamma, and
 * frame rate registers.
 *
 * Usage: write the command byte first (D/C=LOW), then the parameter bytes
 * (D/C=HIGH) using the MIPI DBI bus controller API.
 */

/* --- Power and display mode control --- */
#define GC9D01_CMD_SLPIN     0x10U  /* Sleep In:  panel enters low-power sleep mode.
                                     * A minimum delay of GC9D01_SLEEP_IN_OUT_DURATION_MS
                                     * must be observed before issuing the next command. */
#define GC9D01_CMD_SLPOUT    0x11U  /* Sleep Out: panel exits sleep mode and powers up.
                                     * Same mandatory delay applies after this command.  */
#define GC9D01_CMD_PTLON     0x12U  /* Partial Mode ON:  only the area defined by
                                     * PTLAR (0x30) is refreshed, saving power.         */
#define GC9D01_CMD_NORON     0x13U  /* Normal Display Mode ON: exit partial mode,
                                     * the full frame memory is displayed.               */
#define GC9D01_CMD_INVOFF    0x20U  /* Display Inversion OFF: normal color output
                                     * (pixel data written as-is to the panel).          */
#define GC9D01_CMD_INVON     0x21U  /* Display Inversion ON:  each color channel is
                                     * inverted before driving the panel (useful for
                                     * panels with reversed polarity).                   */
#define GC9D01_CMD_DISPOFF   0x28U  /* Display OFF: blanks the output; the frame
                                     * memory is retained and can be updated.            */
#define GC9D01_CMD_DISPON    0x29U  /* Display ON:  enables output from the frame
                                     * memory to the panel source drivers.               */

/* --- Frame memory addressing --- */
#define GC9D01_CMD_COLSET    0x2AU  /* Column Address Set: defines the active column
                                     * window for subsequent MEMWR operations.
                                     * Parameters: SC[15:8], SC[7:0], EC[15:8], EC[7:0]
                                     * (SC = start column, EC = end column, 0-indexed). */
#define GC9D01_CMD_ROWSET    0x2BU  /* Row Address Set: defines the active row window.
                                     * Parameters: SP[15:8], SP[7:0], EP[15:8], EP[7:0]
                                     * (SP = start page/row, EP = end page/row).         */
#define GC9D01_CMD_MEMWR     0x2CU  /* Memory Write: pixel data bytes follow this
                                     * command (D/C=HIGH) until CS is deasserted or
                                     * a new command is issued.                           */

/* --- Partial area and scrolling --- */
#define GC9D01_CMD_PTLAR     0x30U  /* Partial Area: sets the top and bottom row
                                     * boundaries for PTLON mode.
                                     * Parameters: PSL[15:8], PSL[7:0], PEL[15:8],
                                     * PEL[7:0] (start/end line of the partial area).   */
#define GC9D01_CMD_VSCRDEF   0x33U  /* Vertical Scrolling Definition: defines the
                                     * top fixed, scroll, and bottom fixed areas.        */
#define GC9D01_CMD_TEOFF     0x34U  /* Tearing Effect Line OFF: disables the TE
                                     * output signal (used for tear-free updates).       */
#define GC9D01_CMD_TEON      0x35U  /* Tearing Effect Line ON: enables the TE pin,
                                     * which pulses at the start of each blanking period
                                     * so the host can synchronize framebuffer writes.   */
#define GC9D01_CMD_VSCRSADD  0x37U  /* Vertical Scroll Start Address: sets the first
                                     * row of the scroll area to display at the top.    */

/* --- Memory access and pixel format --- */
#define GC9D01_CMD_MADCTL    0x36U  /* Memory Access Control: configures the read/write
                                     * scan direction of the frame memory. Controls
                                     * display rotation, mirroring, and RGB/BGR order.
                                     * See GC9D01_MADCTL_VAL_* masks below.              */
#define GC9D01_CMD_PIXFMT    0x3AU  /* Interface Pixel Format: selects the color depth
                                     * for the RGB (DPI) and MCU/SPI (DBI) interfaces.
                                     * See GC9D01_PIXFMT_VAL_* values below.             */

/* --- Display function control --- */
#define GC9D01_CMD_DFUNCTR   0xB6U  /* Display Function Control: configures source driver
                                     * output direction, gate driver scan direction, and
                                     * non-display area scan frequency.                  */

/* --- Power control (vendor-specific) --- */
#define GC9D01_CMD_PWRCTRL1  0xC1U  /* Power Control 1: sets the AVDD and AVCL analog
                                     * supply voltages that drive the panel.             */
#define GC9D01_CMD_PWRCTRL2  0xC3U  /* Power Control 2: sets the VGH (gate high) and
                                     * VGL (gate low) driver voltages.                   */
#define GC9D01_CMD_PWRCTRL3  0xC4U  /* Power Control 3: sets the VCOM voltage, which
                                     * controls the LCD common electrode level and
                                     * directly affects display contrast.                */
#define GC9D01_CMD_PWRCTRL4  0xC9U  /* Power Control 4: additional power and VCOM
                                     * adjustment registers.                             */

/* --- ID readback registers --- */
#define GC9D01_CMD_READID1   0xDAU  /* Read ID1: returns the manufacturer ID byte.      */
#define GC9D01_CMD_READID2   0xDBU  /* Read ID2: returns the module/driver version byte. */
#define GC9D01_CMD_READID3   0xDCU  /* Read ID3: returns the module/driver ID byte.     */

/* --- Gamma correction curves (vendor-specific) --- */
#define GC9D01_CMD_GAMMA1    0xF0U  /* Gamma Curve 1: positive polarity adjustment,
                                     * first set. Controls the brightness-to-gray-level
                                     * mapping for positive panel drive voltages.        */
#define GC9D01_CMD_GAMMA2    0xF1U  /* Gamma Curve 2: negative polarity adjustment,
                                     * first set. Complements GAMMA1 for the negative
                                     * drive side of the LCD pixels.                     */
#define GC9D01_CMD_GAMMA3    0xF2U  /* Gamma Curve 3: positive polarity adjustment,
                                     * second set. Fine-tunes the mid-tone response.    */
#define GC9D01_CMD_GAMMA4    0xF3U  /* Gamma Curve 4: negative polarity adjustment,
                                     * second set. Symmetrical to GAMMA3.               */

/* --- Inter-register enable (vendor-specific unlock sequence) --- */
#define GC9D01_CMD_INREGEN1  0xFEU  /* Inter Register Enable 1: must be sent before
                                     * accessing certain vendor-specific registers that
                                     * are locked by default at power-on.               */
#define GC9D01_CMD_INREGEN2  0xEFU  /* Inter Register Enable 2: second unlock command
                                     * in the sequence; further expands the accessible
                                     * vendor register space.                            */

/* --- Frame rate control (vendor-specific) --- */
#define GC9D01_CMD_FRAMERATE 0xE8U  /* Frame Rate Control: adjusts the panel refresh
                                     * frequency by setting the number of clocks per
                                     * display line period.                              */

/* ---------------------------------------------------------------------------
 * MADCTL (Memory Access Control) — Register 0x36
 * ---------------------------------------------------------------------------
 * Each bit controls one aspect of how the frame memory is scanned when
 * pixels are written to or read from the display. Bits can be OR-combined
 * to achieve any of the eight possible orientations.
 *
 * Example — 90° clockwise rotation: MY | MV
 * Example — horizontal flip only:   MX
 */
#define GC9D01_MADCTL_VAL_MY  BIT(7U) /* Row Address Order:
                                        *   0 = top-to-bottom (default)
                                        *   1 = bottom-to-top (vertical flip)           */
#define GC9D01_MADCTL_VAL_MX  BIT(6U) /* Column Address Order:
                                        *   0 = left-to-right (default)
                                        *   1 = right-to-left (horizontal flip)          */
#define GC9D01_MADCTL_VAL_MV  BIT(5U) /* Row/Column Exchange:
                                        *   0 = normal (rows are rows, columns are cols)
                                        *   1 = swap rows and columns (transpose)        */
#define GC9D01_MADCTL_VAL_ML  BIT(4U) /* Vertical Refresh Order:
                                        *   0 = refresh top-to-bottom
                                        *   1 = refresh bottom-to-top                    */
#define GC9D01_MADCTL_VAL_BGR BIT(3U) /* Color Component Order:
                                        *   0 = RGB (red first in memory)
                                        *   1 = BGR (blue first — needed when the panel
                                        *       sub-pixel order is B-G-R)                */
#define GC9D01_MADCTL_VAL_MH  BIT(2U) /* Horizontal Refresh Order:
                                        *   0 = refresh left-to-right
                                        *   1 = refresh right-to-left                    */

/* ---------------------------------------------------------------------------
 * PIXFMT (Interface Pixel Format) — Register 0x3A
 * ---------------------------------------------------------------------------
 * The register byte is split into two nibbles:
 *   Upper nibble (bits [6:4]): selects the DPI (RGB parallel) interface depth.
 *   Lower nibble (bits [2:0]): selects the DBI (MCU/SPI) interface depth.
 *
 * For SPI-only use (MIPI DBI Type-C), only the lower nibble matters.
 * The values below can be OR-combined for a full register byte, e.g.:
 *   GC9D01_PIXFMT_VAL_RGB_16_BIT | GC9D01_PIXFMT_VAL_MCU_16_BIT = 0x55
 */
#define GC9D01_PIXFMT_VAL_RGB_18_BIT 0x60U /* DPI interface: 18-bit color (RGB666, 262K colors) */
#define GC9D01_PIXFMT_VAL_RGB_16_BIT 0x50U /* DPI interface: 16-bit color (RGB565,  65K colors) */
#define GC9D01_PIXFMT_VAL_MCU_18_BIT 0x06U /* DBI/SPI interface: 18-bit color (3 bytes/pixel)   */
#define GC9D01_PIXFMT_VAL_MCU_16_BIT 0x05U /* DBI/SPI interface: 16-bit color (2 bytes/pixel,
                                             * RGB565 — used in this project for memory efficiency) */

/* ---------------------------------------------------------------------------
 * Timing Constants
 * ---------------------------------------------------------------------------
 * The MIPI DCS specification mandates a minimum wait after Sleep In (0x10)
 * and Sleep Out (0x11) commands before any further display commands may be
 * issued. Violating this delay can cause the panel to enter an undefined state.
 */
#define GC9D01_SLEEP_IN_OUT_DURATION_MS 120 /* Mandatory delay (ms) after SLPIN or SLPOUT */

/* ---------------------------------------------------------------------------
 * Register Payload Length Constants
 * ---------------------------------------------------------------------------
 * Each vendor-specific initialization register (power control, gamma curves,
 * frame rate) requires a fixed number of parameter bytes to be sent after the
 * command byte. These constants:
 *   1. Size the byte arrays inside gc9d01_regs (see struct below).
 *   2. Allow the Devicetree binding (galaxycore,gc9d01.yaml) to validate that
 *      the property arrays declared in the .overlay file have the correct length
 *      at build time, catching mismatches before they cause silent corruption.
 */
#define GC9D01_CMD_PWRCTRL1_LEN  1U /* Power Control 1: 1 parameter byte  */
#define GC9D01_CMD_PWRCTRL2_LEN  1U /* Power Control 2: 1 parameter byte  */
#define GC9D01_CMD_PWRCTRL3_LEN  1U /* Power Control 3: 1 parameter byte  */
#define GC9D01_CMD_PWRCTRL4_LEN  1U /* Power Control 4: 1 parameter byte  */
#define GC9D01_CMD_GAMMA1_LEN    6U /* Gamma Curve 1:   6 parameter bytes */
#define GC9D01_CMD_GAMMA2_LEN    6U /* Gamma Curve 2:   6 parameter bytes */
#define GC9D01_CMD_GAMMA3_LEN    6U /* Gamma Curve 3:   6 parameter bytes */
#define GC9D01_CMD_GAMMA4_LEN    6U /* Gamma Curve 4:   6 parameter bytes */
#define GC9D01_CMD_FRAMERATE_LEN 1U /* Frame Rate Ctrl: 1 parameter byte  */

/* ---------------------------------------------------------------------------
 * gc9d01_regs — Panel Initialization Register Value Storage
 * ---------------------------------------------------------------------------
 * This struct holds the byte arrays for all configurable initialization
 * registers. The values come from Devicetree properties (defined in the board
 * .overlay file under the gc9d01 node) and are validated against the *_LEN
 * constants above at build time.
 *
 * The C driver sends each array as the parameter bytes of its corresponding
 * command during the panel initialization sequence (after SLPOUT and before
 * DISPON). The order and values of these bytes are panel-specific and must
 * match the manufacturer's recommended initialization sequence.
 */
struct gc9d01_regs {
	uint8_t pwrctrl1[GC9D01_CMD_PWRCTRL1_LEN]; /* AVDD/AVCL supply voltage settings         */
	uint8_t pwrctrl2[GC9D01_CMD_PWRCTRL2_LEN]; /* VGH/VGL gate driver voltage settings      */
	uint8_t pwrctrl3[GC9D01_CMD_PWRCTRL3_LEN]; /* VCOM voltage level (affects contrast)     */
	uint8_t pwrctrl4[GC9D01_CMD_PWRCTRL4_LEN]; /* Additional power / VCOM fine-tuning       */
	uint8_t gamma1[GC9D01_CMD_GAMMA1_LEN];      /* Gamma curve 1: positive polarity, set 1  */
	uint8_t gamma2[GC9D01_CMD_GAMMA2_LEN];      /* Gamma curve 2: negative polarity, set 1  */
	uint8_t gamma3[GC9D01_CMD_GAMMA3_LEN];      /* Gamma curve 3: positive polarity, set 2  */
	uint8_t gamma4[GC9D01_CMD_GAMMA4_LEN];      /* Gamma curve 4: negative polarity, set 2  */
	uint8_t framerate[GC9D01_CMD_FRAMERATE_LEN];/* Panel refresh rate control byte           */
};

/* ---------------------------------------------------------------------------
 * GC9D01_REGS_INIT(inst) — Devicetree-driven register initialization macro
 * ---------------------------------------------------------------------------
 * Declares and zero-initializes a const gc9d01_regs struct named
 * gc9d01_regs_<inst>, populated from the Devicetree instance properties
 * using DT_INST_PROP(). This approach is idiomatic in Zephyr drivers:
 * all panel-specific tuning lives in the .overlay, not in the driver source.
 *
 * This macro is invoked once per compiled driver instance inside
 * DEVICE_DT_INST_DEFINE(), ensuring the register arrays are resolved at
 * compile time with zero runtime overhead (all values end up in .rodata).
 *
 * @param inst  Devicetree instance index, supplied automatically by
 *              DT_INST_FOREACH_STATUS_OKAY() in the C driver.
 *
 * Example expansion for inst=0:
 *   static const struct gc9d01_regs gc9d01_regs_0 = {
 *       .pwrctrl1  = { 0x14 },
 *       .pwrctrl2  = { 0x0C },
 *       ...
 *   };
 */
#define GC9D01_REGS_INIT(inst)                                                \
	static const struct gc9d01_regs gc9d01_regs_##inst = {               \
		.pwrctrl1  = DT_INST_PROP(inst, pwrctrl1),                    \
		.pwrctrl2  = DT_INST_PROP(inst, pwrctrl2),                    \
		.pwrctrl3  = DT_INST_PROP(inst, pwrctrl3),                    \
		.pwrctrl4  = DT_INST_PROP(inst, pwrctrl4),                    \
		.gamma1    = DT_INST_PROP(inst, gamma1),                      \
		.gamma2    = DT_INST_PROP(inst, gamma2),                      \
		.gamma3    = DT_INST_PROP(inst, gamma3),                      \
		.gamma4    = DT_INST_PROP(inst, gamma4),                      \
		.framerate = DT_INST_PROP(inst, framerate),                   \
	};

#endif /* ZEPHYR_DRIVERS_DISPLAY_GC9D01_H_ */
