/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mfd_interface_tps6521x
 * @brief Header file for the TI TPS65214 and TPS65215 MFD driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_TPS6521X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_TPS6521X_H_

/**
 * @defgroup mfd_interface_tps6521x MFD TPS6521X Interface
 * @ingroup mfd_interfaces
 * @brief TI TPS65214 and TPS65215 PMIC interface.
 * @since 4.6
 * @version 0.1.0
 * @{
 */

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Register map variant, selected by the devicetree compatible */
enum mfd_tps6521x_variant {
	/** TPS65214: protected registers need REG_LOCK unlocking */
	MFD_TPS6521X_VARIANT_TPS65214,
	/** TPS65215: no REG_LOCK, other rail registers */
	MFD_TPS6521X_VARIANT_TPS65215,
};

/** @cond INTERNAL_HIDDEN */
#define MFD_TPS6521X_REG_TI_DEV_ID   0x00U
#define MFD_TPS6521X_REG_ENABLE_CTRL 0x02U
#define MFD_TPS6521X_REG_MFP_CTRL    0x29U
/** @endcond */

/**
 * @brief Read a PMIC register.
 *
 * @param dev TPS6521x MFD device.
 * @param reg Register address.
 * @param val Where to store the value.
 *
 * @retval 0 on success.
 * @retval -errno on I2C failure.
 */
int mfd_tps6521x_reg_read(const struct device *dev, uint8_t reg, uint8_t *val);

/**
 * @brief Read-modify-write a PMIC register.
 *
 * Nothing is written when the value would not change. On TPS65214 the
 * protected registers are unlocked through REG_LOCK for the write and locked
 * again afterwards. Calls are serialised against each other.
 *
 * @param dev TPS6521x MFD device.
 * @param reg Register address.
 * @param mask Bits to change.
 * @param val New value of the bits in @p mask.
 *
 * @retval 0 on success.
 * @retval -errno on I2C failure.
 */
int mfd_tps6521x_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val);

/**
 * @brief Get the register map variant.
 *
 * @param dev TPS6521x MFD device.
 *
 * @return The variant selected by the devicetree compatible.
 */
enum mfd_tps6521x_variant mfd_tps6521x_get_variant(const struct device *dev);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_TPS6521X_H_ */
