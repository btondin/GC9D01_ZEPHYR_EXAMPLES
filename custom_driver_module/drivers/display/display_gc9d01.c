/*
 * GC9D01 display driver — Waveshare 0.71" BOE 160x160 round LCD
 *
 * Mirrors display_gc9x01x.c exactly.
 * Only difference: default_init_regs[] uses the GC9D01 BOE panel sequence
 * (source: GC9D01-BOE0.71_init_code.txt).
 * Power / gamma registers are DTS-configurable with BOE defaults in the YAML.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT galaxycore_gc9d01

#include "display_gc9d01.h"

#include <zephyr/dt-bindings/display/panel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_gc9d01, CONFIG_DISPLAY_LOG_LEVEL);

/*
 * Maximum data bytes in any single entry of default_init_regs[].
 * The GC9D01 BOE sequence has a 32-byte entry (0x6E).
 */
#define GC9D01_NUM_DEFAULT_INIT_REGS 32U

struct gc9d01_data {
	uint8_t bytes_per_pixel;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

struct gc9d01_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	uint8_t pixel_format;
	uint16_t orientation;
	uint16_t x_resolution;
	uint16_t y_resolution;
	bool inversion;
	const void *regs;
};

struct gc9d01_default_init_regs {
	uint8_t cmd;
	uint8_t len;
	uint8_t data[GC9D01_NUM_DEFAULT_INIT_REGS];
};

/*
 * GC9D01 BOE 0.71" panel default init sequence.
 * Source: GC9D01-BOE0.71_init_code.txt
 *
 * Excluded from this table (handled elsewhere):
 *   0xFE / 0xEF  — inter-register enable, sent by gc9d01_regs_init() header
 *   0x3A         — pixel format,    gc9d01_set_pixel_format()
 *   0xC3/C4/C9   — power control,   DTS-configurable (gc9d01_regs_init tail)
 *   0xF0-F3      — gamma,           DTS-configurable (gc9d01_regs_init tail)
 *   0x36         — MADCTL,          gc9d01_set_orientation()
 *   0x11         — SLPOUT,          gc9d01_exit_sleep()
 *   0x29         — DISPON,          gc9d01_display_blanking_off()
 *   0x2C         — MEMWR,           gc9d01_write()
 */
static const struct gc9d01_default_init_regs default_init_regs[] = {
	/* Internal registers 0x80-0x8F = 0xFF */
	{ .cmd = 0x80U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x81U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x82U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x83U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x84U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x85U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x86U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x87U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x88U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x89U, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x8AU, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x8BU, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x8CU, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x8DU, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x8EU, .len = 1U, .data = {0xFFU} },
	{ .cmd = 0x8FU, .len = 1U, .data = {0xFFU} },
	/* Panel control */
	{ .cmd = 0xECU, .len = 1U, .data = {0x01U} },
	{
		.cmd = 0x74U, .len = 7U,
		.data = {0x02U, 0x0EU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U},
	},
	{ .cmd = 0x98U, .len = 1U, .data = {0x3EU} },
	{ .cmd = 0x99U, .len = 1U, .data = {0x3EU} },
	{ .cmd = 0xB5U, .len = 2U, .data = {0x0DU, 0x0DU} },
	{ .cmd = 0x60U, .len = 4U, .data = {0x38U, 0x0FU, 0x79U, 0x67U} },
	{ .cmd = 0x61U, .len = 4U, .data = {0x38U, 0x11U, 0x79U, 0x67U} },
	{
		.cmd = 0x64U, .len = 6U,
		.data = {0x38U, 0x17U, 0x71U, 0x5FU, 0x79U, 0x67U},
	},
	{
		.cmd = 0x65U, .len = 6U,
		.data = {0x38U, 0x13U, 0x71U, 0x5BU, 0x79U, 0x67U},
	},
	{ .cmd = 0x6AU, .len = 2U, .data = {0x00U, 0x00U} },
	{
		.cmd = 0x6CU, .len = 7U,
		.data = {0x22U, 0x02U, 0x22U, 0x02U, 0x22U, 0x22U, 0x50U},
	},
	{
		/* GOA mux — 32 bytes, drives GC9D01_NUM_DEFAULT_INIT_REGS */
		.cmd = 0x6EU, .len = 32U,
		.data = {
			0x03U, 0x03U, 0x01U, 0x01U, 0x00U, 0x00U, 0x0FU, 0x0FU,
			0x0DU, 0x0DU, 0x0BU, 0x0BU, 0x09U, 0x09U, 0x00U, 0x00U,
			0x00U, 0x00U, 0x0AU, 0x0AU, 0x0CU, 0x0CU, 0x0EU, 0x0EU,
			0x10U, 0x10U, 0x00U, 0x00U, 0x02U, 0x02U, 0x04U, 0x04U,
		},
	},
	{ .cmd = 0xBFU, .len = 1U, .data = {0x01U} },
	{ .cmd = 0xF9U, .len = 1U, .data = {0x40U} },
	{ .cmd = 0x9BU, .len = 1U, .data = {0x3BU} },
	{ .cmd = 0x93U, .len = 3U, .data = {0x33U, 0x7FU, 0x00U} },
	{ .cmd = 0x7EU, .len = 1U, .data = {0x30U} },
	{
		.cmd = 0x70U, .len = 6U,
		.data = {0x0DU, 0x02U, 0x08U, 0x0DU, 0x02U, 0x08U},
	},
	{ .cmd = 0x71U, .len = 3U, .data = {0x0DU, 0x02U, 0x08U} },
	{ .cmd = 0x91U, .len = 2U, .data = {0x0EU, 0x09U} },
};

static int gc9d01_transmit(const struct device *dev, uint8_t cmd,
			   const void *tx_data, size_t tx_len)
{
	const struct gc9d01_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				      cmd, tx_data, tx_len);
}

static int gc9d01_regs_init(const struct device *dev)
{
	const struct gc9d01_config *config = dev->config;
	const struct gc9d01_regs *regs = config->regs;
	int ret;

	if (!device_is_ready(config->mipi_dev)) {
		return -ENODEV;
	}

	/* Enable inter-command mode */
	ret = gc9d01_transmit(dev, GC9D01_CMD_INREGEN1, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_INREGEN2, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Apply BOE panel-specific init sequence */
	for (int i = 0; i < ARRAY_SIZE(default_init_regs); i++) {
		ret = gc9d01_transmit(dev,
				      default_init_regs[i].cmd,
				      default_init_regs[i].data,
				      default_init_regs[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	/* Apply DTS-configurable registers (power + gamma + framerate) */
	ret = gc9d01_transmit(dev, GC9D01_CMD_PWRCTRL2,
			      regs->pwrctrl2, sizeof(regs->pwrctrl2));
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_PWRCTRL3,
			      regs->pwrctrl3, sizeof(regs->pwrctrl3));
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_PWRCTRL4,
			      regs->pwrctrl4, sizeof(regs->pwrctrl4));
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_GAMMA1,
			      regs->gamma1, sizeof(regs->gamma1));
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_GAMMA2,
			      regs->gamma2, sizeof(regs->gamma2));
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_GAMMA3,
			      regs->gamma3, sizeof(regs->gamma3));
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_GAMMA4,
			      regs->gamma4, sizeof(regs->gamma4));
	if (ret < 0) {
		return ret;
	}
	ret = gc9d01_transmit(dev, GC9D01_CMD_FRAMERATE,
			      regs->framerate, sizeof(regs->framerate));
	if (ret < 0) {
		return ret;
	}

	/* Enable tearing line */
	ret = gc9d01_transmit(dev, GC9D01_CMD_TEON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int gc9d01_exit_sleep(const struct device *dev)
{
	int ret;

	ret = gc9d01_transmit(dev, GC9D01_CMD_SLPOUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_msleep(GC9D01_SLEEP_IN_OUT_DURATION_MS + 30);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int gc9d01_enter_sleep(const struct device *dev)
{
	int ret;

	ret = gc9d01_transmit(dev, GC9D01_CMD_SLPIN, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_msleep(GC9D01_SLEEP_IN_OUT_DURATION_MS + 30);

	return 0;
}
#endif

static int gc9d01_hw_reset(const struct device *dev)
{
	const struct gc9d01_config *config = dev->config;
	int ret;

	ret = mipi_dbi_reset(config->mipi_dev, 100);
	if (ret < 0) {
		return ret;
	}
	k_msleep(10);

	return 0;
}

static int gc9d01_display_blanking_off(const struct device *dev)
{
	LOG_DBG("Turning display blanking off");
	return gc9d01_transmit(dev, GC9D01_CMD_DISPON, NULL, 0);
}

static int gc9d01_display_blanking_on(const struct device *dev)
{
	LOG_DBG("Turning display blanking on");
	return gc9d01_transmit(dev, GC9D01_CMD_DISPOFF, NULL, 0);
}

static int gc9d01_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pixel_format)
{
	struct gc9d01_data *data = dev->data;
	int ret;
	uint8_t tx_data;
	uint8_t bytes_per_pixel;

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		bytes_per_pixel = 2U;
		tx_data = GC9D01_PIXFMT_VAL_MCU_16_BIT | GC9D01_PIXFMT_VAL_RGB_16_BIT;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		bytes_per_pixel = 3U;
		tx_data = GC9D01_PIXFMT_VAL_MCU_18_BIT | GC9D01_PIXFMT_VAL_RGB_18_BIT;
	} else {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	ret = gc9d01_transmit(dev, GC9D01_CMD_PIXFMT, &tx_data, 1U);
	if (ret < 0) {
		return ret;
	}

	data->pixel_format = pixel_format;
	data->bytes_per_pixel = bytes_per_pixel;

	return 0;
}

static int gc9d01_set_orientation(const struct device *dev,
				  const enum display_orientation orientation)
{
	struct gc9d01_data *data = dev->data;
	int ret;
	uint8_t tx_data = 0U; /* BOE panel: RGB order, no BGR bit */

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		/* 0° — default */
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		tx_data |= GC9D01_MADCTL_VAL_MV | GC9D01_MADCTL_VAL_MY;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		tx_data |= GC9D01_MADCTL_VAL_MY | GC9D01_MADCTL_VAL_MX
			 | GC9D01_MADCTL_VAL_MH;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		tx_data |= GC9D01_MADCTL_VAL_MV | GC9D01_MADCTL_VAL_MX;
	}

	ret = gc9d01_transmit(dev, GC9D01_CMD_MADCTL, &tx_data, 1U);
	if (ret < 0) {
		return ret;
	}

	data->orientation = orientation;

	return 0;
}

static int gc9d01_configure(const struct device *dev)
{
	const struct gc9d01_config *config = dev->config;
	int ret;

	ret = gc9d01_regs_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = gc9d01_set_pixel_format(dev, config->pixel_format);
	if (ret < 0) {
		return ret;
	}

	ret = gc9d01_set_orientation(dev, config->orientation);
	if (ret < 0) {
		return ret;
	}

	if (config->inversion) {
		ret = gc9d01_transmit(dev, GC9D01_CMD_INVON, NULL, 0);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int gc9d01_init(const struct device *dev)
{
	int ret;

	gc9d01_hw_reset(dev);

	gc9d01_display_blanking_on(dev);

	ret = gc9d01_configure(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure display (%d)", ret);
		return ret;
	}

	ret = gc9d01_exit_sleep(dev);
	if (ret < 0) {
		LOG_ERR("Could not exit sleep mode (%d)", ret);
		return ret;
	}

	return 0;
}

static int gc9d01_set_mem_area(const struct device *dev,
			       const uint16_t x, const uint16_t y,
			       const uint16_t w, const uint16_t h)
{
	int ret;
	uint16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1U);
	ret = gc9d01_transmit(dev, GC9D01_CMD_COLSET, &spi_data[0], 4U);
	if (ret < 0) {
		return ret;
	}

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1U);
	ret = gc9d01_transmit(dev, GC9D01_CMD_ROWSET, &spi_data[0], 4U);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int gc9d01_write(const struct device *dev,
			const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf)
{
	const struct gc9d01_config *config = dev->config;
	struct gc9d01_data *data = dev->data;
	int ret;
	const uint8_t *write_data_start = (const uint8_t *)buf;
	struct display_buffer_descriptor mipi_desc;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * data->bytes_per_pixel * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)",
		desc->width, desc->height, x, y);
	ret = gc9d01_set_mem_area(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
		mipi_desc.height = 1;
		mipi_desc.buf_size = desc->width * data->bytes_per_pixel;
	} else {
		write_h = desc->height;
		mipi_desc.height = desc->height;
		mipi_desc.buf_size = desc->width * data->bytes_per_pixel * write_h;
		nbr_of_writes = 1U;
	}

	mipi_desc.width = desc->width;
	mipi_desc.pitch = desc->width;
	mipi_desc.frame_incomplete = desc->frame_incomplete;

	ret = gc9d01_transmit(dev, GC9D01_CMD_MEMWR, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	for (write_cnt = 0U; write_cnt < nbr_of_writes; ++write_cnt) {
		ret = mipi_dbi_write_display(config->mipi_dev,
					     &config->dbi_config,
					     write_data_start,
					     &mipi_desc,
					     data->pixel_format);
		if (ret < 0) {
			return ret;
		}

		write_data_start += desc->pitch * data->bytes_per_pixel;
	}

	return 0;
}

static void gc9d01_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	struct gc9d01_data *data = dev->data;
	const struct gc9d01_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565
					      | PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = data->pixel_format;

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		capabilities->x_resolution = config->x_resolution;
		capabilities->y_resolution = config->y_resolution;
	} else {
		capabilities->x_resolution = config->y_resolution;
		capabilities->y_resolution = config->x_resolution;
	}

	capabilities->current_orientation = data->orientation;
}

#ifdef CONFIG_PM_DEVICE
static int gc9d01_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = gc9d01_exit_sleep(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gc9d01_enter_sleep(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(display, gc9d01_api) = {
	.blanking_on      = gc9d01_display_blanking_on,
	.blanking_off     = gc9d01_display_blanking_off,
	.write            = gc9d01_write,
	.get_capabilities = gc9d01_get_capabilities,
	.set_pixel_format = gc9d01_set_pixel_format,
	.set_orientation  = gc9d01_set_orientation,
};

#define GC9D01_INIT(inst)                                                      \
	GC9D01_REGS_INIT(inst);                                                \
	static const struct gc9d01_config gc9d01_config_##inst = {             \
		.mipi_dev     = DEVICE_DT_GET(DT_INST_PARENT(inst)),           \
		.dbi_config   = {                                              \
			.mode   = MIPI_DBI_MODE_SPI_4WIRE,                     \
			.config = MIPI_DBI_SPI_CONFIG_DT_INST(                 \
					inst,                                  \
					SPI_OP_MODE_MASTER | SPI_WORD_SET(8),  \
					0),                                    \
		},                                                             \
		.pixel_format = DT_INST_PROP(inst, pixel_format),             \
		.orientation  = DT_INST_ENUM_IDX(inst, orientation),          \
		.x_resolution = DT_INST_PROP(inst, width),                    \
		.y_resolution = DT_INST_PROP(inst, height),                   \
		.inversion    = DT_INST_PROP(inst, display_inversion),        \
		.regs         = &gc9d01_regs_##inst,                          \
	};                                                                     \
	static struct gc9d01_data gc9d01_data_##inst;                          \
	PM_DEVICE_DT_INST_DEFINE(inst, gc9d01_pm_action);                      \
	DEVICE_DT_INST_DEFINE(inst, &gc9d01_init,                             \
			      PM_DEVICE_DT_INST_GET(inst),                     \
			      &gc9d01_data_##inst, &gc9d01_config_##inst,      \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,       \
			      &gc9d01_api);

DT_INST_FOREACH_STATUS_OKAY(GC9D01_INIT)
