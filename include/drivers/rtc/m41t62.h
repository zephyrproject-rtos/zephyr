/*
 * Copyright (C) 2021  Ivona Loncar <ivona.loncar@greyp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Low-power serial real-time clock (RTC) m41t62 with alarm.
 *
 * The [m41t62](
     https://www.st.com/en/clocks-and-timers/m41t62.html
   )
 * is a high-precision real-time clock with support for configurable alarms.
 *
 * Most applications for this device will need to use the extended
 * functionality exposed by this header to access the real-time-clock
 * features.
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_M41T62_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_M41T62_H_

#include <stdint.h>

#include <drivers/counter.h>
#include <kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* M41T62 Register map */
#define M41T62_FRACTION_SECONDS 0x00
#define M41T62_REG_SEC  0x01
#define M41T62_REG_MIN  0x02
#define M41T62_REG_HOUR 0x03
#define M41T62_REG_WDAY 0x04
#define M41T62_REG_DAY  0x05
#define M41T62_REG_MON  0x06
#define M41T62_REG_YEAR 0x07
#define M41T62_REG_CALIBRATION 0x08
#define M41T62_REG_WATCHDOG 0x09
#define M41T62_REG_ALARM_MON 0x0A
#define M41T62_REG_ALARM_DAY 0x0B
#define M41T62_REG_ALARM_HOUR 0x0C
#define M41T62_REG_ALARM_MIN 0x0D
#define M41T62_REG_ALARM_SEC 0x0E
#define M41T62_REG_FLAGS 0x0F

#define M41T62_REGISTER_SIZE (8)
#define M41T62_DATETIME_REG_SIZE (M41T62_REG_YEAR + 1)
#define M41T62_ALARM_REG_SIZE \
	(M41T62_REG_ALARM_SEC + 1 - M41T62_REG_ALARM_MON)

/* Exported macro */
#define M41T62_BIT(x) ((uint8_t)x)
#define M41T62_assert_param(expr) ((void)0)

/** @brief Register masks */
#define M41T62_SECONDS_MASK           0x80
#define M41T62_MINUTES_MASK           0x80
#define M41T62_HOURS_MASK             0xC0
#define M41T62_WDAY_MASK              0x08
#define M41T62_DAYMONTH_MASK          0xC0
#define M41T62_MONTH_MASK             0xE0
#define M41T62_YEAR_MASK              0x00

#define M41T62_AL_MONTH_MASK          0xE0
#define M41T62_AL_DATE_MASK           0xC0
#define M41T62_AL_HOUR_MASK           0xC0
#define M41T62_AL_MIN_MASK            0x80
#define M41T62_AL_SEC_MASK            0x80

#define M41T62_REG_ZERO_BIT_ALARM_MON_MASK     0x20
#define M41T62_REG_ZERO_BIT_ALARM_HOUR_MASK    0x40
#define M41T62_REG_ZERO_BITS_FLAGS_MASK         0x3B

#define M41T62_SQW_FREQUENCY_MASK (uint8_t)0xF0
/**
 * @brief  Bitfield positioning.
 */

#define M41T62_STOP_BIT M41T62_BIT(7)

/** @brief watchdog flag bit (read only). */
#define M41T62_WDF_BIT M41T62_BIT(7)

/** @brief alarm flag (read only) bit to indicate that an alarm occurred. */
#define M41T62_AF_BIT M41T62_BIT(6)

/** @brief oscillator fail bit. */
#define M41T62_OSCILLATOR_FAIL_BIT M41T62_BIT(2)

/** @brief ctrl bit to write to enable alarm. */
#define M41T62_AFE_BIT M41T62_BIT(7)

/** @brief ctrl bit to write to enable square wave output. */
#define M41T62_SQWE_BIT M41T62_BIT(6)

struct m41t62_device {
	const struct device *i2c;
};

struct m41t62_config {
	/* Common structure first because generic API expects this here. */
	struct counter_config_info generic;
	const struct device *bus;
	const char *bus_name;
	uint16_t i2c_addr;
};
enum control_bits { SQWE_BIT, OSCILLATOR_FAIL_BIT, STOP_BIT, ALARM_FLAG, ALARM_FLAG_ENABLE };

/**
 * @brief RTC M41T62 Driver-Specific API
 * @defgroup rtc_interface Real Time Clock interfaces
 * @{
 */

/**
 * @brief Set or clear specific control bits.
 *
 * @note This function assumes the device register cache is valid.  It
 * will read the whole register content, and it will change only the value
 * of one requested bit.

 * @note The return value from this
 * function indicates the register value after changes were made.
 *
 * @return a non-negative value on success, or a
 * negative error code from an I2C transaction.
 */
int m41t62_ctrl_update(const struct device *dev, enum control_bits bit_name,
		       uint8_t val);

/**
 * @brief Get value of a specific control bit.
 *
 * The requested control bit will be read from its register.
 *
 * @param dev the M41T62 device pointer
 *
 * @return a non-negative value on success or a negative error
 * code from an I2C transaction.
 */
int m41t62_ctrl_read(const struct device *dev, enum control_bits bit_name,
		     uint8_t *val);

/** @brief Read a M41T62 alarm configuration.
 *
 * The alarm configuration data is read from the device and
 * reconstructed into the output parameter.
 *
 * @param dev the M41T62 device pointer.
 * @param time a pointer where the configured alarm data will be stored.
 *
 * @return a non-negative value indicating successful conversion, or a
 * negative error code from an I2C transaction or invalid parameter.
 */
int m41t62_get_alarm(const struct device *dev, int32_t *time);

/** @brief Configure a M41T62 alarm.
 *
 * The alarm configuration is stored into the device.
 *
 * @param dev the M41T62 device pointer.
 *
 * @param time_in_epoch the desired alarm configuration value.
 *
 * @return a non-negative value on success, or a negative error code
 * from an I2C transaction or an invalid parameter.
 */
int m41t62_set_alarm(const struct device *dev, uint32_t time_in_epoch);

/** @brief Set the RTC to the provided time value.
 *
 * @param dev the M41T62 device pointer.
 *
 * @param epoch_time time provided in epoch value.
 *
 * @retval a non-negative value on success, or a negative error code
 * from an I2C transaction or an invalid parameter.
 */
int m41t62_set_time(const struct device *dev, int32_t epoch_time);

/** @brief Restarts the oscillator.
 *
 * If the OF bit is found to be set to '1' at any time
 * other than the initial power-up, the STOP bit (ST)
 * should be written to a '1,'
 * and then immediately reset to '0.'
 *
 *
 * @param dev the M41T62 device pointer.
 *
 * @retval non-negative on success or a negative error code
 * from an I2C transaction.
 */
int m41t62_restart_oscillator(const struct device *dev);

/** @brief Sets desired square wave frequency.
 *
 * @param dev the M41T62 device pointer.
 *
 * @retval non-negative on success or a negative error code
 * from an I2C transaction.
 */
int m41t62_set_sqw_freq(const struct device *dev, uint8_t frequency);

/** @brief Reads the current frequency of the square wave.
 *
 * @param dev the M41T62 device pointer.
 *
 * @retval non-negative on success or a negative error code
 * from an I2C transaction.
 */
int m41t62_get_sqw_freq(const struct device *dev);

/** @brief	Set or clear defalut bits in the control registers.
 *
 * @param dev the M41T62 device pointer.
 *
 * @retval non-negative on success or a negative error code
 * from an I2C transaction.
 */
int m41t62_setting_default_bits(const struct device *dev);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_M41T62_H_ */
