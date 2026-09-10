/*
 * Copyright (c) 2024 ANITRA system s.r.o.
 * Copyright (c) 2026 Janez Ugovsek <janez@ugovsek.info>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mfd_interface_rv3028
 * @brief Header file for the RV3028 MFD driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_RV3028_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_RV3028_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_rv3028 MFD RV3028 Interface
 * @ingroup mfd_interfaces
 * @brief Shared interface between the RV3028 MFD parent and child drivers.
 * @since 4.5
 * @version 0.1.0
 *
 * This interface provides helper functions for accessing the RV3028
 * registers through the MFD parent driver. It includes register
 * definitions, register read/write APIs, and synchronization
 * primitives shared by the RV3028 child drivers.
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

/**
 * @name RV3028 RAM Registers
 * @{
 */
/* RV3028 RAM register addresses */
/** @brief Real time seconds register */
#define RV3028_REG_SECONDS        0x00
/** Real time minutes register */
#define RV3028_REG_MINUTES        0x01
/** Real time hours register */
#define RV3028_REG_HOURS          0x02
/** Real time weekday register */
#define RV3028_REG_WEEKDAY        0x03
/** Real time date register */
#define RV3028_REG_DATE           0x04
/** Real time month register */
#define RV3028_REG_MONTH          0x05
/** Real time year register */
#define RV3028_REG_YEAR           0x06
/** Alarm minutes register */
#define RV3028_REG_ALARM_MINUTES  0x07
/** Alarm hours register */
#define RV3028_REG_ALARM_HOURS    0x08
/** Alarm weekday register */
#define RV3028_REG_ALARM_WEEKDAY  0x09
/** Timer value0 register */
#define RV3028_REG_TIMER_VALUE_0  0x0A
/** Timer value1 register */
#define RV3028_REG_TIMER_VALUE_1  0x0B
/** Timer status0 register */
#define RV3028_REG_TIMER_STATUS_0 0x0C
/** Timer status1 register */
#define RV3028_REG_TIMER_STATUS_1 0x0D
/** RTC status register  */
#define RV3028_REG_STATUS         0x0E
/** RTC control1 register  */
#define RV3028_REG_CONTROL1       0x0F
/** RTC control2 register  */
#define RV3028_REG_CONTROL2       0x10
/** RTC event register  */
#define RV3028_REG_EVENT_CONTROL  0x13
/** Timestamp count register  */
#define RV3028_REG_TS_COUNT       0x14
/** Timestamp seconds register  */
#define RV3028_REG_TS_SECONDS     0x15
/** Timestamp minutes register  */
#define RV3028_REG_TS_MINUTES     0x16
/** Timestamp hours register  */
#define RV3028_REG_TS_HOURS       0x17
/** Timestamp date register  */
#define RV3028_REG_TS_DATE        0x18
/** Timestamp month register  */
#define RV3028_REG_TS_MONTH       0x19
/** Timestamp year register  */
#define RV3028_REG_TS_YEAR        0x1A
/** Timestamp UNIX timestamp register0  */
#define RV3028_REG_UNIXTIME0      0x1B
/** Timestamp UNIX timestamp register1  */
#define RV3028_REG_UNIXTIME1      0x1C
/** Timestamp UNIX timestamp register2  */
#define RV3028_REG_UNIXTIME2      0x1D
/** Timestamp UNIX timestamp register2  */
#define RV3028_REG_UNIXTIME3      0x1E
/** Timestamp UNIX timestamp register3  */
#define RV3028_REG_USER_RAM1      0x1F
/** User RAM1 register  */
#define RV3028_REG_USER_RAM2      0x20
/** User RAM2 register  */
#define RV3028_REG_EEPROM_ADDRESS 0x25
/** EEPROM data register  */
#define RV3028_REG_EEPROM_DATA    0x26
/** EEPROM command register  */
#define RV3028_REG_EEPROM_COMMAND 0x27
/** RTC ID register  */
#define RV3028_REG_ID             0x28
/** RTC clockout register  */
#define RV3028_REG_CLKOUT         0x35
/** RTC offset register  */
#define RV3028_REG_OFFSET         0x36
/** RTC offset register  */
#define RV3028_REG_BACKUP         0x37
/** @} */

/**
 * @name RV3028 CONTROL1 register masks and bits
 * @{
 */
/** Timer Clock Frequency selection */
#define RV3028_CONTROL1_TD   GENMASK(1, 0)
/** Periodic Countdown Timer Enable bit. */
#define RV3028_CONTROL1_TE   BIT(2)
/** EEPROM Memory Refresh Disable bit. */
#define RV3028_CONTROL1_EERD BIT(3)
/** Update Interrupt Select bit. */
#define RV3028_CONTROL1_USEL BIT(4)
/** Weekday Alarm / Date Alarm selection bit. */
#define RV3028_CONTROL1_WADA BIT(5)
/** Timer Repeat bit. S  */
#define RV3028_CONTROL1_TRPT BIT(7)
/** @} */

/**
 * @name RV3028 CONTROL2 register masks and bits
 * @{
 */
/** Reset bit. This bit is used for a software-based time adjustment (synchronization). */
#define RV3028_CONTROL2_RESET BIT(0)
/** 12 or 24 hour mode */
#define RV3028_CONTROL2_12_24 BIT(1)
/** Event Interrupt Enable bit */
#define RV3028_CONTROL2_EIE   BIT(2)
/** Alarm Interrupt Enable bit */
#define RV3028_CONTROL2_AIE   BIT(3)
/** Periodic Countdown Timer Interrupt Enable bit */
#define RV3028_CONTROL2_TIE   BIT(4)
/** Periodic Time Update Interrupt Enable bit */
#define RV3028_CONTROL2_UIE   BIT(5)
/** Interrupt Controlled Clock Output Enable bit. */
#define RV3028_CONTROL2_CLKIE BIT(6)
/** Time Stamp Enable bit */
#define RV3028_CONTROL2_TSE   BIT(7)
/** @} */

/**
 * @name RV3028 STATUS register masks and bits
 * @{
 */
/** Power On Reset Flag */
#define RV3028_STATUS_PORF   BIT(0)
/** Event Flag */
#define RV3028_STATUS_EVF    BIT(1)
/**Alarm Flag  */
#define RV3028_STATUS_AF     BIT(2)
/** Periodic Countdown Timer Flag */
#define RV3028_STATUS_TF     BIT(3)
/** Periodic Time Update Flag */
#define RV3028_STATUS_UF     BIT(4)
/** Backup Switch Flag */
#define RV3028_STATUS_BSF    BIT(5)
/** Clock Output Interrupt Flag */
#define RV3028_STATUS_CLKF   BIT(6)
/** EEPROM Memory Busy Status Bit – (Read Only) */
#define RV3028_STATUS_EEBUSY BIT(7)
/** @} */

/**
 * @name RV3028 CLKOUT register masks, bits and values
 * @{
 */
/** CLKOUT Frequency Selection */
#define RV3028_CLKOUT_FD     GENMASK(2, 0)
/** Power On Reset Interrupt Enable bit */
#define RV3028_CLKOUT_PORIE  BIT(3)
/** CLKOUT Synchronized enable/disable */
#define RV3028_CLKOUT_CLKSY  BIT(6)
/** CLKOUT Enable bit  */
#define RV3028_CLKOUT_CLKOE  BIT(7)
/** Disable CLKOUT - set to low state */
#define RV3028_CLKOUT_FD_LOW 0x7
/** @} */

/**
 * @name RV3028 EEPROM Backup register  masks, bits and values
 * @{
 */
/** Trickle Charger Enable bit */
#define RV3028_BACKUP_TCE BIT(5)
/** Trickle Charger Series Resistance */
#define RV3028_BACKUP_TCR GENMASK(1, 0)
/** Backup Switchover Mode */
#define RV3028_BACKUP_BSM GENMASK(3, 2)

/** LSB of the EEOffset value */
#define RV3028_BACKUP_OFFSET_BIT_INDEX 7
/** Index of offset 8th bit */
#define RV3028_OFFSET_SIGN_BIT_INDEX   8

/** Enables the Level Switching Mode (LSM). */
#define RV3028_BSM_LEVEL    0x3
/** Enables the Direct Switching Mode (DSM). */
#define RV3028_BSM_DIRECT   0x1
/** Switchover Disabled.  */
#define RV3028_BSM_DISABLED 0x0
/** @} */

/**
 * @name RV3028 CONTROL1 USEL - Periodic register values
 * @{
 */
/** Second update (Auto reset time tRTN2 = 500 ms). – Default value */
#define RV3028_PERIODIC_TIME_SECONDS 0x0
/** Minute update (Auto reset time tRTN2 = 7.813 ms) */
#define RV3028_PERIODIC_TIME_MINUTES 0x1
/** @} */

/**
 * @name RV3028 Event Control register masks, bits and values
 * @{
 */
/** Event High/Low Level (Rising/Falling Edge) selection for detection */
#define RV3028_EVENTCONTROL_EHL  BIT(6)
/** Time Stamp Reset bit */
#define RV3028_EVENTCONTROL_TSR  BIT(2)
/** Time Stamp Overwrite bit. */
#define RV3028_EVENTCONTROL_TSOW BIT(1)
/** Time Stamp Source Selection bit */
#define RV3028_EVENTCONTROL_TSS  BIT(0)

/** The falling edge or low level (is regarded as the External Event on pin EVI. */
#define RV3028_EVENT_FALLING_EDGE 0x0
/** The rising edge or high level is regarded as the External Event on pin EVI. */
#define RV3028_EVENT_RISING_EDGE  0x1

/** The time stamp of the first occurred event is recorded and remains in TS registers. */
#define RV3028_EVENT_OVERWRITE_FIRST 0x0
/** The time stamp of the last occurred event is recorded and TS registers are overwritten. */
#define RV3028_EVENT_OVERWRITE_LAST  0x1

/** Shift for Event Filtering Time set. */
#define RV3028_EVENTCONTROL_FILTER_SHIFT 4
/** No filtering. Edge detection. – Default value */
#define RV3028_EVENTCONTROL_FILTER_NO    (0x0 << RV3028_EVENTCONTROL_FILTER_SHIFT)
/** Sampling period tSP = 3.9 ms (256 Hz). */
#define RV3028_EVENTCONTROL_FILTER_256HZ (0x1 << RV3028_EVENTCONTROL_FILTER_SHIFT)
/** Sampling period tSP = 15.6 ms (64 Hz). */
#define RV3028_EVENTCONTROL_FILTER_64HZ  (0x2 << RV3028_EVENTCONTROL_FILTER_SHIFT)
/** Sampling period tSP = 125 ms (8 Hz) */
#define RV3028_EVENTCONTROL_FILTER_8HZ   (0x3 << RV3028_EVENTCONTROL_FILTER_SHIFT)
/** Mask for Event Filtering Time set */
#define RV3028_EVENTCONTROL_FILTER_MASK  GENMASK(5, 4)
/** @} */

/**
 * @name RV3028 EE command register masks and bits
 * @{
 */
/** First command must be 00h. – Default value */
#define RV3028_EEPROM_CMD_INIT    0x00
/** Update (all configuration RAM -> EEPROM). */
#define RV3028_EEPROM_CMD_UPDATE  0x11
/** Refresh (all configuration EEPROM -> RAM). */
#define RV3028_EEPROM_CMD_REFRESH 0x12
/** Write to one eeprom byte (EEDATA (RAM) -> EEPROM) */
#define RV3028_EEPROM_CMD_WRITE   0x21
/** Read one eeprom byte (EEPROM -> EEDATA (RAM)) */
#define RV3028_EEPROM_CMD_READ    0x22
/** @} */

/**
 * @name RV3028 Clock registers masks and bits
 * @{
 */
/** Seconds mask register */
#define RV3028_SECONDS_MASK   GENMASK(6, 0)
/** Minutes mask register  */
#define RV3028_MINUTES_MASK   GENMASK(6, 0)
/** Hours AM/PM refister */
#define RV3028_HOURS_AMPM     BIT(5)
/** 12h mask register  */
#define RV3028_HOURS_12H_MASK GENMASK(4, 0)
/** 24h mask register  */
#define RV3028_HOURS_24H_MASK GENMASK(5, 0)
/** date mask register  */
#define RV3028_DATE_MASK      GENMASK(5, 0)
/** weekday mask register */
#define RV3028_WEEKDAY_MASK   GENMASK(2, 0)
/** month mask register */
#define RV3028_MONTH_MASK     GENMASK(4, 0)
/** year mask register  */
#define RV3028_YEAR_MASK      GENMASK(7, 0)
/** @} */

/**
 * @name RV3028 Alarm registers masks and bits
 * @{
 */
/** Minutes Alarm Enable bit */
#define RV3028_ALARM_MINUTES_AE_M   BIT(7)
/** Minutes Alarm Mask bit */
#define RV3028_ALARM_MINUTES_MASK   GENMASK(6, 0)
/** Hours Alarm Enable bit */
#define RV3028_ALARM_HOURS_AE_H     BIT(7)
/** AM/PM Alarm bit */
#define RV3028_ALARM_HOURS_AMPM     BIT(5)
/** 12h Alarm mask */
#define RV3028_ALARM_HOURS_12H_MASK GENMASK(4, 0)
/** 24h Alarm mask */
#define RV3028_ALARM_HOURS_24H_MASK GENMASK(5, 0)
/** Weekday/Date Alarm Enable bit. */
#define RV3028_ALARM_DATE_AE_WD     BIT(7)
/** Alarm date mask */
#define RV3028_ALARM_DATE_MASK      GENMASK(5, 0)
/** @} */

/**
 * @name RV3028 Offsets
 * @{
 */
/** Year offset - The RV3028 only supports two-digit years.
 * Leap years are handled from 2000 to 2099.
 */
#define RV3028_YEAR_OFFSET (2000 - 1900)

/** Month offset - The RV3028 enumerates months 1 to 12 */
#define RV3028_MONTH_OFFSET 1
/** @} */

/**
 * @name RV3028 EEPROM EEBUSY pool times
 * @{
 */
/** Read pool in ms (miliseconds) */
#define RV3028_EEBUSY_READ_POLL_MS  1
/** Write pool in ms (miliseconds) */
#define RV3028_EEBUSY_WRITE_POLL_MS 10
/** Timeout pool in ms (miliseconds) */
#define RV3028_EEBUSY_TIMEOUT_MS    100
/** @} */

/** Size of user EEPROM */
#define RV3028_EEPROM_SIZE 43
/** Size of user RAM */
#define RV3028_RAM_SIZE    2

/**
 * @brief Callback function type for RV3028 child device interrupts.
 *
 * This callback is registered by child drivers with the RV3028 MFD
 * parent driver and is called when an interrupt event belonging to the
 * child device occurs.
 *
 * @param dev Pointer to the child device instance.
 */
typedef void (*child_isr_t)(const struct device *dev);

/**
 * @brief RV3028 child device interrupt sources.
 *
 * Identifies the child device that registers an interrupt handler with the
 * MFD parent driver.
 */
enum child_dev {
	RV3028_DEV_REG = 0,    /**< Register access interface. */
	RV3028_DEV_RTC_ALARM,  /**< RTC alarm interrupt source. */
	RV3028_DEV_RTC_UPDATE, /**< RTC update interrupt source. */
	RV3028_DEV_COUNTER,    /**< Counter interrupt source. */
	RV3028_DEV_SENSOR,     /**< Temperature sensor interrupt source. */
	RV3028_DEV_EVI,        /**< Event input interrupt source. */
	RV3028_DEV_MAX,        /**< Maximum number of child devices. */
};

/**
 * @brief Acquire exclusive access to the RV3028 device.
 *
 * Locks the internal synchronization primitive to provide exclusive
 * access to the RV3028 device. Child drivers should call this function
 * before performing a sequence of register accesses that must not be
 * interrupted by other users of the device.
 *
 * @param dev Pointer to the RV3028 MFD device.
 */
void mfd_rv3028_lock_sem(const struct device *dev);

/**
 * @brief Release exclusive access to the RV3028 device.
 *
 * Releases the internal synchronization primitive previously acquired
 * with @ref mfd_rv3028_lock_sem().
 *
 * @param dev Pointer to the RV3028 MFD device.
 */
void mfd_rv3028_unlock_sem(const struct device *dev);

/**
 * @brief Read one or more registers from the RV3028 device.
 *
 * Reads @p len bytes starting at register address @p addr and stores
 * the data in @p buf.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param addr Register address to read from.
 * @param buf Pointer to the destination buffer.
 * @param len Number of bytes to read.
 *
 * @retval 0 If successful.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len);
/**
 * @brief Read an 8-bit register from the RV3028 device.
 *
 * Reads a single byte from register @p addr.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param addr Register address to read from.
 * @param val Pointer where the register value will be stored.
 *
 * @retval 0 If successful.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val);

/**
 * @brief Write one or more registers to the RV3028 device.
 *
 * Writes @p len bytes from @p buf starting at register address @p addr.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param addr Register address to write to.
 * @param buf Pointer to the source buffer.
 * @param len Number of bytes to write.
 *
 * @retval 0 If successful.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_write_regs(const struct device *dev, uint8_t addr, const void *buf, size_t len);

/**
 * @brief Write an 8-bit register to the RV3028 device.
 *
 * Writes a single byte to register @p addr.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param addr Register address to write to.
 * @param val Value to write.
 *
 * @retval 0 If successful.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_write_reg8(const struct device *dev, uint8_t addr, uint8_t val);

/**
 * @brief Update selected bits in an 8-bit register.
 *
 * Performs a read-modify-write operation on the register at @p addr.
 * Only the bits specified by @p mask are updated with the corresponding
 * bits from @p val. All other bits remain unchanged.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param addr Register address to update.
 * @param mask Bit mask selecting the bits to update.
 * @param val New value for the bits specified by @p mask.
 *
 * @retval 0 If successful.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val);

/**
 * @brief Wait until the EEPROM busy flag is cleared.
 *
 * Polls the RV3028 EEBUSY status bit until the current EEPROM operation
 * completes or the operation times out.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param poll_ms Polling interval in milliseconds.
 *
 * @retval 0 If the EEPROM is ready.
 * @retval -ETIME If the operation timed out.
 * @retval Negative errno code on I2C or communication failure.
 */
int mfd_rv3028_eeprom_wait_busy(const struct device *dev, int poll_ms);

/**
 * @brief Exit the EEPROM refresh mode.
 *
 * Disables the EEPROM refresh mode (EERD) on the RV3028 device.
 *
 * @param dev Pointer to the RV3028 MFD device.
 *
 * @retval 0 If the EEPROM refresh mode was disabled successfully.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_exit_eerd(const struct device *dev);

/**
 * @brief Enter the EEPROM refresh mode.
 *
 * Enables the EEPROM refresh mode (EERD) on the RV3028 device.
 * If the mode is already enabled, no operation is performed.
 *
 * The function waits until the EEPROM operation has completed before
 * returning.
 *
 * @param dev Pointer to the RV3028 MFD device.
 *
 * @retval 0 If the EEPROM refresh mode was enabled successfully.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_enter_eerd(const struct device *dev);

/**
 * @brief Execute an EEPROM command on the RV3028 device.
 *
 * Sends an EEPROM command sequence to the RV3028 EEPROM command
 * register. The command sequence consists of an initialization command
 * followed by the requested command.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param command EEPROM command to execute.
 *
 * @retval 0 If the command was sent successfully.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_eeprom_command(const struct device *dev, uint8_t command);

/**
 * @brief Update the RV3028 EEPROM contents.
 *
 * Starts an EEPROM update operation and waits until the operation
 * completes. The EEPROM refresh mode is exited before returning.
 *
 * @param dev Pointer to the RV3028 MFD device.
 *
 * @retval 0 If the EEPROM update completed successfully.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_update(const struct device *dev);

/**
 * @brief Refresh the RV3028 registers from EEPROM.
 *
 * Starts an EEPROM refresh operation, which restores the register
 * values from the EEPROM. The function waits until the EEPROM operation
 * completes and exits the EEPROM refresh mode before returning.
 *
 * @param dev Pointer to the RV3028 MFD device.
 *
 * @retval 0 If the EEPROM refresh completed successfully.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_refresh(const struct device *dev);

/**
 * @brief Update an RV3028 configuration register and store the change.
 *
 * Updates selected bits in an RV3028 configuration register and stores
 * the modified value in EEPROM. If the selected bits already contain the
 * requested value, no EEPROM operation is performed.
 *
 * The function enters EEPROM refresh mode before modifying the register
 * and performs the required EEPROM update operation afterwards.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param addr Register address to update.
 * @param mask Bit mask selecting the bits to modify.
 * @param val New value for the selected bits.
 *
 * @retval 0 If the configuration was updated successfully.
 * @retval Negative errno code on failure.
 */
int mfd_rv3028_update_cfg(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val);

/**
 * @brief Register an interrupt handler for an RV3028 child device.
 *
 * Registers an interrupt callback provided by a child driver with the
 * RV3028 MFD parent driver. The callback is invoked when the corresponding
 * child interrupt event is detected by the MFD interrupt handler.
 *
 * @param dev Pointer to the RV3028 MFD device.
 * @param child_dev Pointer to the child device instance.
 * @param child_idx Child device interrupt index.
 * @param handler Interrupt callback function.
 */
void mfd_rv3028_set_irq_handler(const struct device *dev, const struct device *child_dev,
				enum child_dev child_idx, child_isr_t handler);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_RV3028_H_ */
