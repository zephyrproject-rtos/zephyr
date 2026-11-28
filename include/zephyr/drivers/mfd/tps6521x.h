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
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util_macro.h>

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
#define MFD_TPS6521X_REG_TI_DEV_ID      0x00U
#define MFD_TPS6521X_REG_ENABLE_CTRL    0x02U
#define MFD_TPS6521X_REG_GENERAL_CONFIG 0x1EU
#define MFD_TPS6521X_REG_MFP_1_CONFIG   0x1FU
#define MFD_TPS6521X_REG_MFP_2_CONFIG   0x20U
#define MFD_TPS6521X_REG_MASK_CONFIG    0x25U
#define MFD_TPS6521X_REG_MFP_CTRL       0x29U
/** @endcond */

/**
 * @name Interrupt registers
 *
 * Flag registers that INT_SOURCE (0x2B) points at. Their flags are
 * write-one-to-clear.
 * @{
 */
/** TPS65215 only: LDO2 faults */
#define MFD_TPS6521X_REG_INT_LDO_2         0x2CU
/** LDO1 and LDO2 faults on TPS65214, LDO1 faults on TPS65215 */
#define MFD_TPS6521X_REG_INT_LDO_1         0x2DU
/** Buck3 faults */
#define MFD_TPS6521X_REG_INT_BUCK_3        0x2EU
/** Buck1 and Buck2 faults */
#define MFD_TPS6521X_REG_INT_BUCK_1_2      0x2FU
/** Thermal warm and hot flags */
#define MFD_TPS6521X_REG_INT_SYSTEM        0x30U
/** Residual voltage per rail */
#define MFD_TPS6521X_REG_INT_RV            0x31U
/** Residual voltage or timeout shutdown */
#define MFD_TPS6521X_REG_INT_TIMEOUT_RV_SD 0x32U
/** Push-button edges and real-time status */
#define MFD_TPS6521X_REG_INT_PB            0x33U
/** @} */

/** INT_PB: deglitched pin status, read-only, 1 = released */
#define MFD_TPS6521X_INT_PB_REAL_TIME_STATUS BIT(2)
/** INT_PB: button released */
#define MFD_TPS6521X_INT_PB_RISING_EDGE      BIT(1)
/** INT_PB: button pressed */
#define MFD_TPS6521X_INT_PB_FALLING_EDGE     BIT(0)

struct mfd_tps6521x_int_callback;

/**
 * @brief Interrupt callback handler.
 *
 * Runs in the system workqueue, after the flags have been cleared in the
 * PMIC. It may access PMIC registers. A fault that persists sets its flag
 * again, so the handler runs again every 100 ms while it lasts.
 *
 * @param dev TPS6521x MFD device.
 * @param cb Registered callback.
 * @param value Value read from @c cb->reg, all bits. At least one bit of
 *        @c cb->mask is set.
 */
typedef void (*mfd_tps6521x_int_handler_t)(const struct device *dev,
					   struct mfd_tps6521x_int_callback *cb, uint8_t value);

/** Interrupt callback registration */
struct mfd_tps6521x_int_callback {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	/** @endcond */
	/** Handler */
	mfd_tps6521x_int_handler_t handler;
	/** Interrupt register to watch, MFD_TPS6521X_REG_INT_* */
	uint8_t reg;
	/** Flags in @ref reg that call the handler */
	uint8_t mask;
};

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

/**
 * @brief Register an interrupt callback.
 *
 * Flags raised before a callback is registered are not reported: the MFD
 * clears all pending flags at init and on every interrupt, whether a
 * callback watches them or not, so that nINT is released.
 *
 * @param dev TPS6521x MFD device.
 * @param cb Callback, with handler, reg and mask set. It must stay valid
 *        until removed.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the PMIC node has no int-gpios property.
 * @retval -EINVAL if @p cb is incomplete.
 */
int mfd_tps6521x_add_int_callback(const struct device *dev, struct mfd_tps6521x_int_callback *cb);

/**
 * @brief Remove an interrupt callback.
 *
 * @param dev TPS6521x MFD device.
 * @param cb Callback passed to mfd_tps6521x_add_int_callback().
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the PMIC node has no int-gpios property.
 * @retval -EINVAL if @p cb was not registered.
 */
int mfd_tps6521x_remove_int_callback(const struct device *dev,
				     struct mfd_tps6521x_int_callback *cb);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_TPS6521X_H_ */
