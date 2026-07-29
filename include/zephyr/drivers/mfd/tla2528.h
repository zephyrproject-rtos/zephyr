/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the TLA2528 MFD driver.
 * @ingroup mfd_interface_tla2528
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_TLA2528_H
#define ZEPHYR_INCLUDE_DRIVERS_MFD_TLA2528_H

#include <zephyr/device.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/sys/util_macro.h>

#include <stdint.h>

/**
 * @defgroup mfd_interface_tla2528 MFD TLA2528 Interface
 * @ingroup mfd_interfaces
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/**
 * @brief  TLA2528 Register mapping
 */
enum tla2528_reg {
	TLA2528_SYSTEM_STATUS = 0x0,    /**< System status register */
	TLA2528_GENERAL_CFG = 0x1,      /**< General configuration register */
	TLA2528_DATA_CFG = 0x2,         /**< Data configuration register */
	TLA2528_OSR_CFG = 0x3,          /**< Oversampling configuration register */
	TLA2528_OPMODE_CFG = 0x4,       /**< Operation mode configuration register */
	TLA2528_PIN_CFG = 0x5,          /**< Pin configuration register */
	TLA2528_GPIO_CFG = 0x7,         /**< GPIO configuration register */
	TLA2528_GPO_DRIVE_CFG = 0x9,    /**< GPIO output drive configuration register */
	TLA2528_GPO_VALUE = 0xb,        /**< GPIO output value register */
	TLA2528_GPI_VALUE = 0xd,        /**< GPIO input value register */
	TLA2528_SEQUENCE_CFG = 0x10,    /**< ADC Sequence configuration register */
	TLA2528_CHANNEL_SEL = 0x11,     /**< ADC Channel selection register */
	TLA2528_AUTO_SEQ_CH_SEL = 0x12, /**< ADC sequence channel selection register */
};

/**
 * @brief Bit mapping for the GENERAL_CFG register of the TLA2528
 */
enum tla2528_general_cfg {
	TLA2528_CNVST = BIT(3),  /**< Start conversion */
	TLA2528_CH_RST = BIT(2), /**< Reset all channels to analog mode */
	TLA2528_CAL = BIT(1),    /**< Calibrate ADC offset */
	TLA2528_RST = BIT(0)     /**< Soft-reset all registers */
};

/**
 * @brief write a specific register of the TLA2528
 *
 * @param dev instance of TLA2528
 * @param reg register to write to
 * @param data data to write
 *
 * @retval 0 on success, negative errno code otherwise
 */
int tla2528_register_write(const struct device *dev, enum tla2528_reg reg, uint8_t data);

/**
 * @brief read a specific register of the TLA2528
 *
 * @param dev instance of TLA2528
 * @param reg register to read from
 * @param data pointer to store the read data
 *
 * @retval 0 on success, negative errno code otherwise
 */
int tla2528_register_read(const struct device *dev, enum tla2528_reg reg, uint8_t *data);

/**
 * @brief set specific bits in a register of the TLA2528
 *
 * Bits are set "atomically" (in a single I2C transaction), without read-modify-write cycle
 *
 * @param dev instance of TLA2528
 * @param reg register to modify
 * @param bits bits to set
 *
 * @retval 0 on success, negative errno code otherwise
 */
int tla2528_register_set_bits(const struct device *dev, enum tla2528_reg reg, uint8_t bits);

/**
 * @brief clear specific bits in a register of the TLA2528
 *
 * Bits are cleared "atomically" (in a single I2C transaction), without read-modify-write cycle
 *
 * @param dev instance of TLA2528
 * @param reg register to modify
 * @param bits bits to clear
 *
 * @retval 0 on success, negative errno code otherwise
 */
int tla2528_register_clear_bits(const struct device *dev, enum tla2528_reg reg, uint8_t bits);

/**
 * @brief read a sample of ADC data from the TLA2528
 *
 * @param dev instance of TLA2528
 * @param data pointer to store the read ADC data
 *
 * @retval 0 on success, negative errno code otherwise
 */
int tla2528_read_adc_data(const struct device *dev, uint16_t *data);

/**
 * @brief get the mutex lock for the TLA2528
 *
 * @param dev instance of TLA2528
 *
 * @return pointer to the mutex lock
 */
struct k_mutex *tla2528_get_lock(const struct device *dev);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_TLA2528_H */
