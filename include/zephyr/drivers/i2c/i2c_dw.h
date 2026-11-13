/*
 * Copyright (c) 2026 Xsight Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SMBus ARP engine of the Synopsys DesignWare I2C controller (SMBus 3.3.1 section 6.6).
 *
 * The engine answers the ARP commands at the SMBus Device Default Address in hardware and loads
 * the assigned address into IC_SAR. These functions are available with CONFIG_I2C_TARGET.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_I2C_DW_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_I2C_DW_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Length of an SMBus UDID in bytes. */
#define I2C_DW_SMBUS_UDID_LEN 16

/** @brief SMBus ARP engine configuration. */
struct i2c_dw_smbus_arp_config {
	/** UDID, byte 15 (Device Capabilities) first. */
	uint8_t udid[I2C_DW_SMBUS_UDID_LEN];
	/**
	 * Persistent target address: valid from enable and kept across ARP Reset Device.
	 * The ARP host can still assign a new address.
	 */
	bool persistent;
	/** Initial 7-bit address, used when @c persistent is set. */
	uint8_t addr;
};

/**
 * @brief Program the UDID and enable the SMBus ARP engine.
 *
 * Call while no target is registered. The engine starts at the next i2c_target_register(), which
 * then ignores the address in its configuration.
 *
 * @param dev DesignWare I2C controller.
 * @param cfg Engine configuration.
 *
 * @retval 0 Success.
 * @retval -EINVAL Configuration is NULL or its address is not 7-bit.
 * @retval -EBUSY A target is registered.
 * @retval -ENOTSUP The controller has no ARP engine.
 */
int i2c_dw_smbus_arp_enable(const struct device *dev, const struct i2c_dw_smbus_arp_config *cfg);

/**
 * @brief Disable the SMBus ARP engine.
 *
 * Call while no target is registered. The next i2c_target_register() uses the address in its
 * configuration.
 *
 * @param dev DesignWare I2C controller.
 *
 * @retval 0 Success.
 * @retval -EBUSY A target is registered.
 */
int i2c_dw_smbus_arp_disable(const struct device *dev);

/**
 * @brief Read the target address held by the SMBus ARP engine.
 *
 * Reads registers only, so it can be called from any context while a target is registered.
 *
 * @param dev   DesignWare I2C controller.
 * @param addr7 Receives the 7-bit address.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP The ARP engine is not enabled.
 * @retval -EADDRNOTAVAIL No address is assigned.
 */
int i2c_dw_smbus_arp_get_addr(const struct device *dev, uint8_t *addr7);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_I2C_DW_H_ */
