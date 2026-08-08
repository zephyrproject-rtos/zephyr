/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Deere & Company
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the Infineon TLE927x System Basis Chip MFD driver.
 * @ingroup mfd_interface_tle927x
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_TLE927X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_TLE927X_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_tle927x MFD TLE927x Interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @name Control registers
 * @{
 */
#define TLE927X_REG_MS_CTRL    0x01U /**< M_S_CTRL, mode and supply control */
#define TLE927X_REG_HW_CTRL    0x02U /**< HW_CTRL, hardware control */
#define TLE927X_REG_WD_CTRL    0x03U /**< WD_CTRL, watchdog control */
#define TLE927X_REG_BUS_CTRL_1 0x04U /**< BUS_CTRL_1, HS-CAN and LIN1 control */
#define TLE927X_REG_BUS_CTRL_2 0x05U /**< BUS_CTRL_2, LIN2 control */
#define TLE927X_REG_WK_CTRL_1  0x06U /**< WK_CTRL_1, wake input control */
#define TLE927X_REG_WK_CTRL_2  0x07U /**< WK_CTRL_2, wake source control */
/** @} */

/**
 * @name Status registers
 * @{
 */
#define TLE927X_REG_SUP_STAT   0x41U /**< SUP_STAT, supply voltage fail status */
#define TLE927X_REG_THERM_STAT 0x42U /**< THERM_STAT, thermal protection status */
#define TLE927X_REG_DEV_STAT   0x43U /**< DEV_STAT, device information status */
#define TLE927X_REG_BUS_STAT_1 0x44U /**< BUS_STAT_1, HS-CAN and LIN1 failure status */
#define TLE927X_REG_BUS_STAT_2 0x45U /**< BUS_STAT_2, LIN2 failure status */
#define TLE927X_REG_WK_STAT_1  0x46U /**< WK_STAT_1, wake source status */
#define TLE927X_REG_WK_STAT_2  0x47U /**< WK_STAT_2, LIN wake source status */
#define TLE927X_REG_SMPS_STAT  0x4CU /**< SMPS_STAT, buck and boost regulator status */
/** @} */

/**
 * @name M_S_CTRL fields
 * @{
 */
#define TLE927X_MS_CTRL_MODE_MASK        GENMASK(7, 6) /**< SBC mode, see enum tle927x_mode */
#define TLE927X_MS_CTRL_VCC2_ON_MASK     GENMASK(4, 3) /**< SBC modes in which VCC2 is on */
#define TLE927X_MS_CTRL_VCC1_OV_RST_MASK BIT(2)        /**< Reset on VCC1 overvoltage */
#define TLE927X_MS_CTRL_VCC1_RT_MASK     GENMASK(1, 0) /**< VCC1 undervoltage reset threshold */
/** @} */

/**
 * @name M_S_CTRL.VCC2_ON values
 * @{
 */
#define TLE927X_VCC2_ON_OFF         0x0U /**< VCC2 off */
#define TLE927X_VCC2_ON_NORMAL      0x1U /**< VCC2 on in Normal mode */
#define TLE927X_VCC2_ON_NORMAL_STOP 0x2U /**< VCC2 on in Normal and Stop mode */
#define TLE927X_VCC2_ON_ALL_MODES   0x3U /**< VCC2 always on, Sleep mode included */
/** @} */

/**
 * @name M_S_CTRL.VCC1_RT values
 * @{
 */
#define TLE927X_VCC1_RT_VRT1     0x0U /**< Vrt1, the highest threshold */
#define TLE927X_VCC1_RT_VRT2     0x1U /**< Vrt2 */
#define TLE927X_VCC1_RT_VRT3     0x2U /**< Vrt3 */
#define TLE927X_VCC1_RT_DISABLED 0x3U /**< VCC1 undervoltage reset disabled */
/** @} */

/**
 * @name HW_CTRL fields
 * @{
 */
#define TLE927X_HW_CTRL_FSI_FO2_MASK BIT(7) /**< Fail-safe input disabled, pin used as FO2 */
#define TLE927X_HW_CTRL_FO_ON        BIT(5) /**< Failure outputs activated by software */
/** @} */

/**
 * @name WD_CTRL fields
 * @{
 */
#define TLE927X_WD_CTRL_CHECKSUM     BIT(7) /**< Checksum, bits 7 to 0 must sum to even */
#define TLE927X_WD_CTRL_WD_STM_EN_0  BIT(6) /**< Watchdog deactivation in Stop mode, bit 0 */
#define TLE927X_WD_CTRL_WD_WIN       BIT(5) /**< Window watchdog enabled */
#define TLE927X_WD_CTRL_WD_EN_WK_BUS BIT(4) /**< Watchdog restart on a bus wake in Stop mode */
#define TLE927X_WD_CTRL_MAX_3_RST    BIT(3) /**< Fail-Safe mode after 3 watchdog resets */
#define TLE927X_WD_CTRL_WD_TIMER     GENMASK(2, 0) /**< Watchdog timer period */
/** @} */

/**
 * @name BUS_CTRL_1 fields
 * @{
 */
#define TLE927X_BUS_CTRL_1_LIN1     GENMASK(4, 3) /**< LIN1 module mode */
#define TLE927X_BUS_CTRL_1_CAN_MASK GENMASK(1, 0) /**< HS-CAN module mode */
/** @} */

/**
 * @name BUS_CTRL_1.CAN values
 * @{
 */
#define TLE927X_CAN_OFF          0x0U /**< HS-CAN off */
#define TLE927X_CAN_WAKE_CAPABLE 0x1U /**< HS-CAN off, wake capable */
#define TLE927X_CAN_RECEIVE_ONLY 0x2U /**< HS-CAN receive only */
#define TLE927X_CAN_NORMAL       0x3U /**< HS-CAN normal mode */
/** @} */

/**
 * @name WK_CTRL_2 fields
 * @{
 */
#define TLE927X_WK_CTRL_2_WK_EN BIT(0) /**< WK pin enabled as wake source */
/** @} */

/**
 * @name SUP_STAT fields
 * @{
 */
#define TLE927X_SUP_STAT_POR     BIT(7) /**< Power-on reset detected */
#define TLE927X_SUP_STAT_VLIN_UV BIT(6) /**< VLIN undervoltage detected */
#define TLE927X_SUP_STAT_VCC1_OV BIT(5) /**< VCC1 overvoltage detected */
#define TLE927X_SUP_STAT_VCC2_OT BIT(4) /**< VCC2 overtemperature detected */
#define TLE927X_SUP_STAT_VCC2_UV BIT(3) /**< VCC2 undervoltage detected */
#define TLE927X_SUP_STAT_VCC1_SC BIT(2) /**< VCC1 short to GND detected */
#define TLE927X_SUP_STAT_VCC1_UV BIT(0) /**< VCC1 undervoltage detected */
/** @} */

/**
 * @name THERM_STAT fields
 * @{
 */
#define TLE927X_THERM_STAT_TSD2 BIT(2) /**< TSD2 thermal shutdown detected */
#define TLE927X_THERM_STAT_TSD1 BIT(1) /**< TSD1 thermal shutdown detected */
#define TLE927X_THERM_STAT_TPW  BIT(0) /**< Thermal prewarning detected */
/** @} */

/**
 * @name DEV_STAT fields
 * @{
 */
#define TLE927X_DEV_STAT_PREV        GENMASK(7, 6) /**< Device status before Restart mode */
#define TLE927X_DEV_STAT_RO_CL_HIGH  BIT(5)        /**< RO pin detected clamped high */
#define TLE927X_DEV_STAT_FSI_FAIL    BIT(4)        /**< Fail-safe input failure detected */
#define TLE927X_DEV_STAT_WD_FAIL     GENMASK(3, 2) /**< Number of watchdog failures */
#define TLE927X_DEV_STAT_SPI_FAIL    BIT(1)        /**< SPI failure detected */
#define TLE927X_DEV_STAT_FO_ON_STATE BIT(0)        /**< Failure outputs activated */
/** @} */

/**
 * @name DEV_STAT.PREV values, the device status before Restart mode
 * @{
 */
#define TLE927X_PREV_CLEARED         0x0U /**< Cleared, no Restart mode since the last clear */
#define TLE927X_PREV_RESTART_FAILURE 0x1U /**< Restart after a failure or a Fail-Safe wake */
#define TLE927X_PREV_WAKE_FROM_SLEEP 0x2U /**< Wake from Sleep mode */
/** @} */

/**
 * @name BUS_STAT_1 fields
 * @{
 */
#define TLE927X_BUS_STAT_1_CAN_FAIL GENMASK(2, 1) /**< HS-CAN failure status */
#define TLE927X_BUS_STAT_1_VCAN_UV  BIT(0)        /**< VCAN undervoltage detected */
/** @} */

/**
 * @name WK_STAT_1 fields
 * @{
 */
#define TLE927X_WK_STAT_1_PFM_PWM  BIT(7) /**< Automatic PFM to PWM transition detected */
#define TLE927X_WK_STAT_1_CAN_WU   BIT(5) /**< Wake up via HS-CAN */
#define TLE927X_WK_STAT_1_TIMER_WU BIT(4) /**< Wake up via timer */
#define TLE927X_WK_STAT_1_WK_WU    BIT(0) /**< Wake up via the WK pin */
/** @} */

/**
 * @name WK_STAT_2 fields
 * @{
 */
#define TLE927X_WK_STAT_2_LIN2_WU BIT(1) /**< Wake up via LIN2 */
#define TLE927X_WK_STAT_2_LIN1_WU BIT(0) /**< Wake up via LIN1 */
/** @} */

/**
 * @brief SBC operating modes selectable over SPI
 *
 * The values are the M_S_CTRL.MODE field encoding.
 */
enum tle927x_mode {
	/** All functions available, watchdog running */
	TLE927X_MODE_NORMAL = 0U,
	/** VCC1 off, wake through CAN, LIN or WK only */
	TLE927X_MODE_SLEEP = 1U,
	/** VCC1 on, configuration frozen, restricted SPI command set */
	TLE927X_MODE_STOP = 2U,
};

/** Cause of the most recent boot, decoded once during init */
enum tle927x_boot_cause {
	/** Could not be determined */
	TLE927X_BOOT_CAUSE_UNKNOWN = 0,
	/** Cold start, SUP_STAT.POR was set */
	TLE927X_BOOT_CAUSE_POWER_ON,
	/** Restart after a failure or a wake from Fail-Safe mode */
	TLE927X_BOOT_CAUSE_FAILURE,
	/** Wake from SBC Sleep mode */
	TLE927X_BOOT_CAUSE_WAKE_FROM_SLEEP,
};

/**
 * @brief Read a TLE927x register
 *
 * @param dev TLE927x MFD device instance
 * @param reg Register address
 * @param val Destination for the register value
 *
 * @retval 0 If successful.
 * @retval -EINVAL If @p val is NULL.
 * @retval -errno Negative errno from the SPI bus.
 */
int mfd_tle927x_read_reg(const struct device *dev, uint8_t reg, uint8_t *val);

/**
 * @brief Write a TLE927x register, writing a status register clears it
 *
 * Control registers are writable in Normal mode only. Status registers may be cleared in
 * any mode.
 *
 * @param dev TLE927x MFD device instance
 * @param reg Register address
 * @param val Value to write
 *
 * @retval 0 If successful.
 * @retval -EBUSY If @p reg is a control register and the SBC is not in Normal mode.
 * @retval -errno Negative errno from the SPI bus.
 */
int mfd_tle927x_write_reg(const struct device *dev, uint8_t reg, uint8_t val);

/**
 * @brief Read-modify-write a TLE927x control register, atomically
 *
 * @param dev TLE927x MFD device instance
 * @param reg Register address
 * @param mask Bits to modify
 * @param val New value for the bits selected by @p mask
 *
 * @retval 0 If successful.
 * @retval -EINVAL If @p reg is WD_CTRL or a status register.
 * @retval -EBUSY If the SBC is not in Normal mode.
 * @retval -errno Negative errno from the SPI bus.
 */
int mfd_tle927x_update_reg(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val);

/**
 * @brief Request an SBC mode transition
 *
 * Sleep is not supported: it switches VCC1 off, and on a board without a usable
 * wake source the SBC cannot be recovered without removing VS.
 *
 * @param dev TLE927x MFD device instance
 * @param mode Requested SBC mode
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If @p mode is Sleep.
 * @retval -EINVAL If @p mode is unknown.
 * @retval -EIO If the SBC rejected the transition.
 * @retval -errno Negative errno from the SPI bus.
 */
int mfd_tle927x_set_mode(const struct device *dev, enum tle927x_mode mode);

/**
 * @brief Get the current SBC mode
 *
 * @param dev TLE927x MFD device instance
 * @param mode Destination for the current SBC mode
 *
 * @retval 0 If successful.
 * @retval -EINVAL If @p mode is NULL.
 */
int mfd_tle927x_get_mode(const struct device *dev, enum tle927x_mode *mode);

/**
 * @brief Get the boot cause, sampled at init before the status registers were cleared
 *
 * @param dev TLE927x MFD device instance
 * @param cause Destination for the boot cause, may be NULL
 * @param wk_stat_1 Destination for WK_STAT_1, may be NULL
 * @param wk_stat_2 Destination for WK_STAT_2, may be NULL
 *
 * @retval 0 If successful.
 */
int mfd_tle927x_get_boot_cause(const struct device *dev, enum tle927x_boot_cause *cause,
			       uint8_t *wk_stat_1, uint8_t *wk_stat_2);

/**
 * @brief Read the global fault status, leaving the registers latched
 *
 * @param dev TLE927x MFD device instance
 * @param sup Destination for SUP_STAT, may be NULL
 * @param therm Destination for THERM_STAT, may be NULL
 * @param dev_stat Destination for DEV_STAT, may be NULL
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno from the SPI bus.
 */
int mfd_tle927x_read_faults(const struct device *dev, uint8_t *sup, uint8_t *therm,
			    uint8_t *dev_stat);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_TLE927X_H_ */
