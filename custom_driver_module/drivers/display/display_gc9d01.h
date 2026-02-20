/*
 * GC9D01 display driver header
 * Mirrors display_gc9x01x.h exactly — only naming and array size differ.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_GC9D01_H_
#define ZEPHYR_DRIVERS_DISPLAY_GC9D01_H_

#include <zephyr/sys/util.h>

/* Command registers (same register map as GC9X01X) */
#define GC9D01_CMD_SLPIN     0x10U
#define GC9D01_CMD_SLPOUT    0x11U
#define GC9D01_CMD_PTLON     0x12U
#define GC9D01_CMD_NORON     0x13U
#define GC9D01_CMD_INVOFF    0x20U
#define GC9D01_CMD_INVON     0x21U
#define GC9D01_CMD_DISPOFF   0x28U
#define GC9D01_CMD_DISPON    0x29U
#define GC9D01_CMD_COLSET    0x2AU
#define GC9D01_CMD_ROWSET    0x2BU
#define GC9D01_CMD_MEMWR     0x2CU
#define GC9D01_CMD_PTLAR     0x30U
#define GC9D01_CMD_VSCRDEF   0x33U
#define GC9D01_CMD_TEOFF     0x34U
#define GC9D01_CMD_TEON      0x35U
#define GC9D01_CMD_MADCTL    0x36U
#define GC9D01_CMD_VSCRSADD  0x37U
#define GC9D01_CMD_PIXFMT    0x3AU
#define GC9D01_CMD_DFUNCTR   0xB6U
#define GC9D01_CMD_PWRCTRL1  0xC1U
#define GC9D01_CMD_PWRCTRL2  0xC3U
#define GC9D01_CMD_PWRCTRL3  0xC4U
#define GC9D01_CMD_PWRCTRL4  0xC9U
#define GC9D01_CMD_READID1   0xDAU
#define GC9D01_CMD_READID2   0xDBU
#define GC9D01_CMD_READID3   0xDCU
#define GC9D01_CMD_GAMMA1    0xF0U
#define GC9D01_CMD_GAMMA2    0xF1U
#define GC9D01_CMD_GAMMA3    0xF2U
#define GC9D01_CMD_GAMMA4    0xF3U
#define GC9D01_CMD_INREGEN1  0xFEU
#define GC9D01_CMD_INREGEN2  0xEFU
#define GC9D01_CMD_FRAMERATE 0xE8U

/* GC9D01_CMD_MADCTL register fields */
#define GC9D01_MADCTL_VAL_MY  BIT(7U)
#define GC9D01_MADCTL_VAL_MX  BIT(6U)
#define GC9D01_MADCTL_VAL_MV  BIT(5U)
#define GC9D01_MADCTL_VAL_ML  BIT(4U)
#define GC9D01_MADCTL_VAL_BGR BIT(3U)
#define GC9D01_MADCTL_VAL_MH  BIT(2U)

/* GC9D01_CMD_PIXFMT register fields */
#define GC9D01_PIXFMT_VAL_RGB_18_BIT 0x60U
#define GC9D01_PIXFMT_VAL_RGB_16_BIT 0x50U
#define GC9D01_PIXFMT_VAL_MCU_18_BIT 0x06U
#define GC9D01_PIXFMT_VAL_MCU_16_BIT 0x05U

/* Sleep durations */
#define GC9D01_SLEEP_IN_OUT_DURATION_MS 120

/* DTS-configurable register lengths (same as GC9X01X) */
#define GC9D01_CMD_PWRCTRL1_LEN  1U
#define GC9D01_CMD_PWRCTRL2_LEN  1U
#define GC9D01_CMD_PWRCTRL3_LEN  1U
#define GC9D01_CMD_PWRCTRL4_LEN  1U
#define GC9D01_CMD_GAMMA1_LEN    6U
#define GC9D01_CMD_GAMMA2_LEN    6U
#define GC9D01_CMD_GAMMA3_LEN    6U
#define GC9D01_CMD_GAMMA4_LEN    6U
#define GC9D01_CMD_FRAMERATE_LEN 1U

struct gc9d01_regs {
	uint8_t pwrctrl1[GC9D01_CMD_PWRCTRL1_LEN];
	uint8_t pwrctrl2[GC9D01_CMD_PWRCTRL2_LEN];
	uint8_t pwrctrl3[GC9D01_CMD_PWRCTRL3_LEN];
	uint8_t pwrctrl4[GC9D01_CMD_PWRCTRL4_LEN];
	uint8_t gamma1[GC9D01_CMD_GAMMA1_LEN];
	uint8_t gamma2[GC9D01_CMD_GAMMA2_LEN];
	uint8_t gamma3[GC9D01_CMD_GAMMA3_LEN];
	uint8_t gamma4[GC9D01_CMD_GAMMA4_LEN];
	uint8_t framerate[GC9D01_CMD_FRAMERATE_LEN];
};

#define GC9D01_REGS_INIT(inst)                                                \
	static const struct gc9d01_regs gc9d01_regs_##inst = {               \
		.pwrctrl1 = DT_INST_PROP(inst, pwrctrl1),                     \
		.pwrctrl2 = DT_INST_PROP(inst, pwrctrl2),                     \
		.pwrctrl3 = DT_INST_PROP(inst, pwrctrl3),                     \
		.pwrctrl4 = DT_INST_PROP(inst, pwrctrl4),                     \
		.gamma1   = DT_INST_PROP(inst, gamma1),                       \
		.gamma2   = DT_INST_PROP(inst, gamma2),                       \
		.gamma3   = DT_INST_PROP(inst, gamma3),                       \
		.gamma4   = DT_INST_PROP(inst, gamma4),                       \
		.framerate = DT_INST_PROP(inst, framerate),                   \
	};

#endif /* ZEPHYR_DRIVERS_DISPLAY_GC9D01_H_ */
