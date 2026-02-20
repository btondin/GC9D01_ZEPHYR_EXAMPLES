/*
 * Copyright (c) 2025 Bruno Rodriguez Tondin <btondin@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/* 1. Define the 'compatible' to match your binding */
#define DT_DRV_COMPAT microchip_mcp41xxx

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

/* 2. Include our public API header file */
#include "mcp41xxx.h"

/* 3. Register the log module (using the Kconfig from prj.conf) */
LOG_MODULE_REGISTER(mcp41xxx);

/* 4. Define the Configuration (read from devicetree/overlay) */
struct mcp41xxx_config {
    struct spi_dt_spec spi;
    uint32_t resistance; /* Stores 10, 50, or 100 (kOhm) */
};

/* 5. Define the Data (empty, as this driver is simple) */
struct mcp41xxx_data {
    /* This driver does not need runtime data (no threads/semaphores) */
};

/**
 * @brief Public API function to set the wiper (tap) value.
 */
int mcp41xxx_set_tap(const struct device *dev, uint8_t tap)
{
    const struct mcp41xxx_config *config = dev->config;
    int err;

    /* The MCP41XXX uses 2 bytes:
     * Byte 1: Command (0x11 = Write to "Wiper 0")
     * Byte 2: Data (0-255)
     */
    uint8_t tx_buffer[2];
    tx_buffer[0] = 0x11;
    tx_buffer[1] = tap;

    const struct spi_buf tx_buf = {
        .buf = tx_buffer,
        .len = sizeof(tx_buffer)
    };
    const struct spi_buf_set tx_set = {
        .buffers = &tx_buf,
        .count = 1
    };

    LOG_DBG("Writing tap: %d (0x%02x)", tap, tap);

    /* Send the 2 bytes. The 'cs-gpios' from the overlay will
     * be toggled automatically by the 'nrf-spim' driver.
     */
    err = spi_write_dt(&config->spi, &tx_set);
    if (err) {
        LOG_ERR("Failed to write to SPI: %d", err);
    }

    return err;
}

int mcp41xxx_set_resistance(const struct device *dev, uint32_t ohms)
{
    const struct mcp41xxx_config *config = dev->config;

    /* 1. Convert the overlay value (in kOhms) to Ohms */
    uint32_t total_res_ohms = config->resistance * 1000;

    /* 2. Check if the requested value is higher than the total */
    if (ohms > total_res_ohms) {
        LOG_WRN("Requested resistance (%d) higher than max (%d)",
                ohms, total_res_ohms);
        /* Set to the maximum value (255) */
        return mcp41xxx_set_tap(dev, 255);
    }

    /* 3. Calculate the 'tap' (0-255).
     * The MCP41XXX has 256 steps (0-255).
     * We use uint64_t to prevent overflow during multiplication.
     */
    uint32_t tap = (uint32_t)(((uint64_t)ohms * 256) / total_res_ohms);

    /* 4. Ensure the final value is within bounds (0-255) */
    if (tap > 255) {
        tap = 255;
    }

    LOG_DBG("Requested %d Ohms, Total %d Ohms, Calculated Tap: %d",
            ohms, total_res_ohms, tap);

    /* 5. Call the low-level function with the calculated tap value */
    return mcp41xxx_set_tap(dev, (uint8_t)tap);
}

/**
 * @brief Driver Initialization Function
 */
static int mcp41xxx_init(const struct device *dev)
{
    const struct mcp41xxx_config *config = dev->config;

    /* Check if the SPI bus is ready */
    if (!spi_is_ready_dt(&config->spi)) {
        LOG_ERR("SPI bus %s is not ready!", config->spi.bus->name);
        return -ENODEV;
    }

    LOG_INF("Device %s (%d kOhm) initialized.", dev->name, config->resistance);
    return 0;
}

/* 6. Define the SPI configuration (Mode 0, 8-bit, MSB first) */
#define MCP41XXX_SPI_CFG \
    (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA)

/* 7. Initialization Macro ("good practice" standard) */
#define MCP41XXX_INIT(n)                                                   \
    static struct mcp41xxx_data mcp41xxx_data_##n;                         \
                                                                           \
    static const struct mcp41xxx_config mcp41xxx_cfg_##n = {               \
        .spi = SPI_DT_SPEC_INST_GET(n, MCP41XXX_SPI_CFG, 0U),              \
        .resistance = DT_INST_PROP(n, resistance), /* Reads resistance from overlay */ \
    };                                                                     \
                                                                           \
    DEVICE_DT_INST_DEFINE(n, mcp41xxx_init, NULL, &mcp41xxx_data_##n,     \
                          &mcp41xxx_cfg_##n, POST_KERNEL,                  \
                          CONFIG_APPLICATION_INIT_PRIORITY, NULL);

/* 8. Call the macro for every "okay" instance in the devicetree */
DT_INST_FOREACH_STATUS_OKAY(MCP41XXX_INIT)