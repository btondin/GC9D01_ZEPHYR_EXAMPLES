/*
 * Copyright (c) 2025 Bruno Rodriguez Tondin <btondin@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_POT_MCP41XXX_H_
#define ZEPHYR_DRIVERS_POT_MCP41XXX_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/**
 * @brief Sets the potentiometer's wiper ("tap") value.
 *
 * @param dev Pointer to the MCP41XXX device instance.
 * @param tap The wiper value (0-255).
 *
 * @return 0 on success, or a negative SPI error code.
 */
int mcp41xxx_set_tap(const struct device *dev, uint8_t tap);

/**
 * @brief Sets the resistance between the Wiper (W) and terminal B (R_WB).
 *
 * This function uses the 'resistance' value (in kOhms)
 * defined in the overlay to calculate the nearest 'tap' value (0-255)
 * for the desired resistance.
 *
 * @param dev Pointer to the MCP41XXX device instance.
 * @param ohms The desired resistance in Ohms (e.g., 5000 for 5k).
 *
 * @return 0 on success, or a negative error code.
 */
int mcp41xxx_set_resistance(const struct device *dev, uint32_t ohms);

#endif /* ZEPHYR_DRIVERS_POT_MCP41XXX_H_ */