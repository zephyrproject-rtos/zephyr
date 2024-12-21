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
 * @param cc1 Pointer to variable where detected CC1 voltage state will be stored
 * @param cc2 Pointer to variable where detected CC2 voltage state will be stored
 * @return -EINVAL if cc1 or cc2 pointer is NULL
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_get_cc(const struct i2c_dt_spec *bus, enum tc_cc_voltage_state *cc1,
		      enum tc_cc_voltage_state *cc2);

/**
 * @brief Function to retrieve information about the TCPCI chip.
 *
 * @param bus I2C bus
 * @param chip_info Pointer to the structure where the chip information will be stored
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_get_chip_info(const struct i2c_dt_spec *bus, struct tcpc_chip_info *chip_info);

/**
 * @brief Function to dump the standard TCPCI registers.
 *
 * @param bus I2C bus
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_dump_std_reg(const struct i2c_dt_spec *bus);

/**
 * @brief Function to enable or disable the BIST (Built-In Self-Test) mode.
 *
 * @param bus I2C bus
 * @param enable Boolean flag to enable (true) or disable (false) BIST mode
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_set_bist_test_mode(const struct i2c_dt_spec *bus, bool enable);

/**
 * @brief Function to transmit a PD (Power Delivery) message. The message is transmitted
 * with a specified number of retries in case of failure.
 *
 * @param bus I2C bus
 * @param msg Pointer to the PD message structure to be transmitted
 * @param retries Number of retries in case of transmission failure
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_transmit_data(const struct i2c_dt_spec *bus, struct pd_msg *msg,
			     const uint8_t retries);

/**
 * @brief Function to select the Rp (Pull-up Resistor) value.
 *
 * @param bus I2C bus
 * @param rp Enum representing the Rp value to be set
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_select_rp_value(const struct i2c_dt_spec *bus, enum tc_rp_value rp);

/**
 * @brief Function to get the currently selected Rp value.
 *
 * @param bus I2C bus
 * @param rp Pointer to the variable where the Rp value will be stored
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_get_rp_value(const struct i2c_dt_spec *bus, enum tc_rp_value *rp);

/**
 * @brief Function to set the CC pull resistor and set the role as either Source or Sink.
 *
 * @param bus I2C bus
 * @param pull Enum representing the CC pull resistor to be set
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_set_cc(const struct i2c_dt_spec *bus, enum tc_cc_pull pull);

/**
 * @brief Function to enable or disable TCPC auto dual role toggle.
 *
 * @param bus I2C bus
 * @param enable Boolean flag to enable (true) or disable (false) DRP toggle mode
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_set_drp_toggle(const struct i2c_dt_spec *bus, bool enable);

/**
 * @brief Function to set the power and data role of the PD message header.
 *
 * @param bus I2C bus
 * @param pd_rev Enum representing the USB−PD Specification Revision to be set
 * @param power_role Enum representing the power role to be set
 * @param data_role Enum representing the data role to be set
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_set_roles(const struct i2c_dt_spec *bus, enum pd_rev_type pd_rev,
			 enum tc_power_role power_role, enum tc_data_role data_role);

/**
 * @brief Function to set the RX type.
 *
 * @param bus I2C bus
 * @param type Value representing the RX type to be set
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_set_rx_type(const struct i2c_dt_spec *bus, uint8_t type);

/**
 * @brief Function to set the polarity of the CC lines.
 *
 * @param bus I2C bus
 * @param polarity Enum representing the CC polarity to be set
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_set_cc_polarity(const struct i2c_dt_spec *bus, enum tc_cc_polarity polarity);

/**
 * @brief Function to enable or disable VCONN.
 *
 * @param bus I2C bus
 * @param enable Boolean flag to enable (true) or disable (false) VCONN
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_set_vconn(const struct i2c_dt_spec *bus, bool enable);

/**
 * @brief Function to get the status of a specific TCPCI status register.
 *
 * @param bus I2C bus
 * @param reg Enum representing the status register to be read
 * @param status Pointer to the variable where the status will be stored
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_get_status_register(const struct i2c_dt_spec *bus, enum tcpc_status_reg reg,
				   uint16_t *status);

/**
 * @brief Function to clear specific bits in a TCPCI status register.
 *
 * @param bus I2C bus
 * @param reg Enum representing the status register to be cleared
 * @param mask Bitmask specifying which bits to clear
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_clear_status_register(const struct i2c_dt_spec *bus, enum tcpc_status_reg reg,
				     uint16_t mask);

/**
 * @brief Function to set the mask of a TCPCI status register.
 *
 * @param bus I2C bus
 * @param reg Enum representing the status register to be masked
 * @param mask Bitmask specifying which bits to mask
 * @return int Status of I2C operation, 0 in case of success
 */
int tcpci_tcpm_mask_status_register(const struct i2c_dt_spec *bus, enum tcpc_status_reg reg,
				    uint16_t mask);

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_TCPCI_PRIV_H_ */
