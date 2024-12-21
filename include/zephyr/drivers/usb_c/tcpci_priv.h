/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helper functions to use by the TCPCI-compliant drivers
 *
 * This file contains generic TCPCI functions that may be used by the drivers to TCPCI-compliant
 * devices that want to implement vendor-specific functionality without the need to reimplement the
 * TCPCI generic functionality and register operations.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USBC_TCPCI_PRIV_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBC_TCPCI_PRIV_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/usb_c/usbc.h>

/**
 * @brief Structure used to bind the register address to name in registers dump
 */
struct tcpci_reg_dump_map {
	/** Address of I2C device register */
	uint8_t addr;
	/** Human readable name of register */
	const char *name;
	/** Size in bytes of the register */
	uint8_t size;
};

/** Size of the array containing the standard registers used by tcpci dump command */
#define TCPCI_STD_REGS_SIZE 38
/**
 * @brief Array containing the standard TCPCI registers list.
 * If the TCPC driver contain any vendor-specific registers, it may override the TCPCI dump_std_reg
 * function tp dump them and should also dump the standard registers using this array.
 *
 */
extern const struct tcpci_reg_dump_map tcpci_std_regs[TCPCI_STD_REGS_SIZE];

/**
 * @brief Function to read the 8-bit register of TCPCI device
 *
 * @param bus I2C bus
 * @param reg Address of TCPCI register
 * @param value Pointer to variable that will store the register value
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_read_reg8(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t *value);

/**
 * @brief Function to write a value to the 8-bit register of TCPCI device
 *
 * @param bus I2C bus
 * @param reg Address of TCPCI register
 * @param value Value that will be written to the device register
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_write_reg8(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t value);

/**
 * @brief Function to read and update part of the 8-bit register of TCPCI device
 * The function is NOT performing this operation atomically.
 *
 * @param bus I2C bus
 * @param reg Address of TCPCI register
 * @param mask Bitmask specifying which bits of the device register will be modified
 * @param value Value that will be written to the device register after being ANDed with mask
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_update_reg8(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t mask, uint8_t value);

/**
 * @brief Function to read the 16-bit register of TCPCI device
 *
 * @param bus I2C bus
 * @param reg Address of TCPCI register
 * @param value Pointer to variable that will store the register value
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_read_reg16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *value);

/**
 * @brief Function to write a value to the 16-bit register of TCPCI device
 *
 * @param bus I2C bus
 * @param reg Address of TCPCI register
 * @param value Value that will be written to the device register
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_write_reg16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t value);

/**
 * @brief Function that converts the TCPCI alert register to the tcpc_alert enum
 * The hard reset value takes priority, where the rest are returned in the bit order from least
 * significant to most significant.
 *
 * @param reg Value of the TCPCI alert register. This parameter must have value other than zero.
 * @return enum tcpc_alert Value of one of the flags being set in the alert register
 */
enum tcpc_alert tcpci_alert_reg_to_enum(uint16_t reg);

/**
 * @brief Function that reads the CC status registers and converts read values to enums
 * representing voltages state and partner detection status.
 *
 * @param bus I2C bus
 * @param cc1 pointer to variable where detected CC1 voltage state will be stored
 * @param cc2 pointer to variable where detected CC2 voltage state will be stored
 * @return -EINVAL if cc1 or cc2 pointer is NULL
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_get_cc(const struct i2c_dt_spec *bus, enum tc_cc_voltage_state *cc1,
		      enum tc_cc_voltage_state *cc2);

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_TCPCI_PRIV_H_ */
