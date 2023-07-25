/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Real-time clock control based on the DS3231 counter API.
 *
 * The [Maxim
 * DS3231](https://www.maximintegrated.com/en/products/analog/real-time-clocks/DS3231.html)
 * is a high-precision real-time clock with temperature-compensated
 * crystal oscillator and support for configurable alarms.
 *
 * The core Zephyr API to this device is as a counter, with the
 * following limitations:
 * * counter_read() and counter_*_alarm() cannot be invoked from
 *   interrupt context, as they require communication with the device
 *   over an I2C bus.
 * * many other counter APIs, such as start/stop/set_top_value are not
 *   supported as the clock is always running.
 * * two alarm channels are supported but are not equally capable:
 *   channel 0 supports alarms at 1 s resolution, while channel 1
 *   supports alarms at 1 minute resolution.
 *
 * Most applications for this device will need to use the extended
 * functionality exposed by this header to access the real-time-clock
 * features.  The majority of these functions must be invoked from
 * supervisor mode.
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_DS3231_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_DS3231_H_

#include <time.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/notify.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bit in ctrl or ctrl_stat associated with alarm 1. */
#define MAXIM_DS3231_ALARM1 BIT(0)

/** @brief Bit in ctrl or ctrl_stat associated with alarm 2. */
#define MAXIM_DS3231_ALARM2 BIT(1)

/* Constants corresponding to bits in the DS3231 control register at
 * 0x0E.
 *
 * See the datasheet for interpretation of these bits.
 */
/** @brief ctrl bit for alarm 1 interrupt enable. */
#define MAXIM_DS3231_REG_CTRL_A1IE MAXIM_DS3231_ALARM1

/** @brief ctrl bit for alarm 2 interrupt enable. */
#define MAXIM_DS3231_REG_CTRL_A2IE MAXIM_DS3231_ALARM2

/** @brief ctrl bit for ISQ functionality.
 *
 * When clear the ISW signal provides a square wave.  When set the ISW
 * signal indicates alarm events.
 *
 * @note The driver expects to be able to control this bit.
 */
#define MAXIM_DS3231_REG_CTRL_INTCN BIT(2)

/** @brief ctrl bit offset for square wave output frequency.
 *
 * @note The driver will control the content of this field.
 */
#define MAXIM_DS3231_REG_CTRL_RS_Pos 3

/** @brief ctrl mask to isolate RS bits. */
#define MAXIM_DS3231_REG_CTRL_RS_Msk (0x03 << MAXIM_DS3231_REG_CTRL_RS_Pos)

/** @brief ctrl RS field value for 1 Hz square wave. */
#define MAXIM_DS3231_REG_CTRL_RS_1Hz 0x00

/** @brief ctrl RS field value for 1024 Hz square wave. */
#define MAXIM_DS3231_REG_CTRL_RS_1KiHz 0x01

/** @brief ctrl RS field value for 4096 Hz square wave. */
#define MAXIM_DS3231_REG_CTRL_RS_4KiHz 0x02

/** @brief ctrl RS field value for 8192 Hz square wave. */
#define MAXIM_DS3231_REG_CTRL_RS_8KiHz 0x03

/** @brief ctrl bit to write to trigger temperature conversion. */
#define MAXIM_DS3231_REG_CTRL_CONV BIT(5)

/** @brief ctrl bit to write to enable square wave output in battery mode. */
#define MAXIM_DS3231_REG_CTRL_BBSQW BIT(6)

/** @brief ctrl bit to write to disable the oscillator. */
#define MAXIM_DS3231_REG_CTRL_EOSCn BIT(7),

/** @brief ctrl_stat bit indicating alarm1 has triggered.
 *
 * If an alarm callback handler is registered this bit is
 * cleared prior to invoking the callback with the flags
 * indicating which alarms are ready.
 */
#define MAXIM_DS3231_REG_STAT_A1F MAXIM_DS3231_ALARM1

/** @brief ctrl_stat bit indicating alarm2 has triggered.
 *
 * If an alarm callback handler is registered this bit is
 * cleared prior to invoking the callback with the flags
 * indicating which alarms are ready.
 */
#define MAXIM_DS3231_REG_STAT_A2F MAXIM_DS3231_ALARM2

/** @brief Flag indicating a temperature conversion is in progress. */
#define MAXIM_DS3231_REG_STAT_BSY BIT(2)

/** @brief Set to enable 32 KiHz open drain signal.
 *
 * @note This is a control bit, though it is positioned within the
 * ctrl_stat register which otherwise contains status bits.
 */
#define MAXIM_DS3231_REG_STAT_EN32kHz BIT(3)

/** @brief Flag indicating the oscillator has been off since last cleared. */
#define MAXIM_DS3231_REG_STAT_OSF BIT(7)

/** @brief Control alarm behavior on match in seconds field.
 *
 * If clear the alarm fires only when the RTC seconds matches the
 * alarm seconds.
 *
 * If set the alarm seconds field is ignored and an alarm will be
 * triggered every second.  The bits for IGNMN, IGNHR, and IGNDA must
 * all be set.
 *
 * This bit must be clear for the second alarm instance.
 *
 * Bit maps to A1M1 and is used in
 * maxim_ds3231_alarm_configuration::alarm_flags.
 */
#define MAXIM_DS3231_ALARM_FLAGS_IGNSE BIT(0)

/** @brief Control alarm behavior on match in minutes field.
 *
 * If clear the alarm fires only when the RTC minutes matches the
 * alarm minutes.  The bit for IGNSE must be clear.
 *
 * If set the alarm minutes field is ignored and alarms will be
 * triggered based on IGNSE. The bits for IGNHR and IGNDA must both be
 * set.
 *
 * Bit maps to A1M2 or A2M2 and is used in
 * maxim_ds3231_alarm_configuration::alarm_flags.
 */
#define MAXIM_DS3231_ALARM_FLAGS_IGNMN BIT(1)

/** @brief Control alarm behavior on match in hours field.
 *
 * If clear the alarm fires only when the RTC hours matches the
 * alarm hours.  The bits for IGNMN and IGNSE must be clear.
 *
 * If set the alarm hours field is ignored and alarms will be
 * triggered based on IGNMN and IGNSE.  The bit for IGNDA must be set.
 *
 * Bit maps to A1M3 or A2M3 and is used in
 * maxim_ds3231_alarm_configuration::alarm_flags.
 */
#define MAXIM_DS3231_ALARM_FLAGS_IGNHR BIT(2)

/** @brief Control alarm behavior on match in day/date field.
 *
 * If clear the alarm fires only when the RTC day/date matches the
 * alarm day/date, mediated by MAXIM_DS3231_ALARM_FLAGS_DAY.  The bits
 * for IGNHR, IGNMN, and IGNSE must be clear
 *
 * If set the alarm day/date field is ignored and an alarm will be
 * triggered based on IGNHR, IGNMN, and IGNSE.
 *
 * Bit maps to A1M4 or A2M4 and is used in
 * maxim_ds3231_alarm_configuration::alarm_flags.
 */
#define MAXIM_DS3231_ALARM_FLAGS_IGNDA BIT(3)

/** @brief Control match on day of week versus day of month
 *
 * Set the flag to match on day of week; clear it to match on day of
 * month.
 *
 * Bit maps to DY/DTn in corresponding
 * maxim_ds3231_alarm_configuration::alarm_flags.
 */
#define MAXIM_DS3231_ALARM_FLAGS_DOW BIT(4)

/** @brief Indicates that the alarm should be disabled once it fires.
 *
 * Set the flag in the maxim_ds3231_alarm_configuration::alarm_flags
 * field to cause the alarm to be disabled when the interrupt fires,
 * prior to invoking the corresponding handler.
 *
 * Leave false to allow the alarm to remain enabled so it will fire
 * again on the next match.
 */
#define MAXIM_DS3231_ALARM_FLAGS_AUTODISABLE BIT(7)

/**
 * @brief RTC DS3231 Driver-Specific API
 * @defgroup rtc_ds3231_interface RTC DS3231 Interface
 * @ingroup io_interfaces
 * @{
 */

/** @brief Signature for DS3231 alarm callbacks.
 *
 * The alarm callback is invoked from the system work queue thread.
 * At the point the callback is invoked the corresponding alarm flags
 * will have been cleared from the device status register.  The
 * callback is permitted to invoke operations on the device.
 *
 * @param dev the device from which the callback originated
 * @param id the alarm id
 * @param syncclock the value from maxim_ds3231_read_syncclock() at the
 * time the alarm interrupt was processed.
 * @param user_data the corresponding parameter from
 * maxim_ds3231_alarm::user_data.
 */
typedef void (*maxim_ds3231_alarm_callback_handler_t)(const struct device *dev,
						      uint8_t id,
						      uint32_t syncclock,
						      void *user_data);

/** @brief Signature used to notify a user of the DS3231 that an
 * asynchronous operation has completed.
 *
 * Functions compatible with this type are subject to all the
 * constraints of #sys_notify_generic_callback.
 *
 * @param dev the DS3231 device pointer
 *
 * @param notify the notification structure provided in the call
 *
 * @param res the result of the operation.
 */
typedef void (*maxim_ds3231_notify_callback)(const struct device *dev,
					     struct sys_notify *notify,
					     int res);

/** @brief Information defining the alarm configuration.
 *
 * DS3231 alarms can be set to fire at specific times or at the
 * rollover of minute, hour, day, or day of week.
 *
 * When an alarm is configured with a handler an interrupt will be
 * generated and the handler called from the system work queue.
 *
 * When an alarm is configured without a handler, or a persisted alarm
 * is present, alarms can be read using maxim_ds3231_check_alarms().
 */
struct maxim_ds3231_alarm {
	/** @brief Time specification for an RTC alarm.
	 *
	 * Though specified as a UNIX time, the alarm parameters are
	 * determined by converting to civil time and interpreting the
	 * component hours, minutes, seconds, day-of-week, and
	 * day-of-month fields, mediated by the corresponding #flags.
	 *
	 * The year and month are ignored, but be aware that gmtime()
	 * determines day-of-week based on calendar date.  Decoded
	 * alarm times will fall within 1978-01 since 1978-01-01
	 * (first of month) was a Sunday (first of week).
	 */
	time_t time;

	/** @brief Handler to be invoked when alarms are signalled.
	 *
	 * If this is null the alarm will not be triggered by the
	 * INTn/SQW GPIO.  This is a "persisted" alarm from its role
	 * in using the DS3231 to trigger a wake from deep sleep.  The
	 * application should use maxim_ds3231_check_alarms() to
	 * determine whether such an alarm has been triggered.
	 *
	 * If this is not null the driver will monitor the ISW GPIO
	 * for alarm signals and will invoke the handler with a
	 * parameter carrying the value returned by
	 * maxim_ds3231_check_alarms().  The corresponding status flags
	 * will be cleared in the device before the handler is
	 * invoked.
	 *
	 * The handler will be invoked from the system work queue.
	 */
	maxim_ds3231_alarm_callback_handler_t handler;

	/** @brief User-provided pointer passed to alarm callback. */
	void *user_data;

	/** @brief Flags controlling configuration of the alarm alarm.
	 *
	 * See MAXIM_DS3231_ALARM_FLAGS_IGNSE and related constants.
	 *
	 * Note that as described the alarm mask fields require that
	 * if a unit is not ignored, higher-precision units must also
	 * not be ignored.  For example, if match on hours is enabled,
	 * match on minutes and seconds must also be enabled.  Failure
	 * to comply with this requirement will cause
	 * maxim_ds3231_set_alarm() to return an error, leaving the
	 * alarm configuration unchanged.
	 */
	uint8_t flags;
};

/** @brief Register the RTC clock against system clocks.
 *
 * This captures the same instant in both the RTC time scale and a
 * stable system clock scale, allowing conversion between those
 * scales.
 */
struct maxim_ds3231_syncpoint {
	/** @brief Time from the DS3231.
	 *
	 * This maybe in UTC, TAI, or local offset depending on how
	 * the RTC is maintained.
	 */
	struct timespec rtc;

	/** @brief Value of a local clock at the same instant as #rtc.
	 *
	 * This is captured from a stable monotonic system clock
	 * running at between 1 kHz and 1 MHz, allowing for
	 * microsecond to millisecond accuracy in synchronization.
	 */
	uint32_t syncclock;
};

/** @brief Read the local synchronization clock.
 *
 * Synchronization aligns the DS3231 real-time clock with a stable
 * monotonic local clock which should have a frequency between 1 kHz
 * and 1 MHz and be itself synchronized with the primary system time
 * clock.  The accuracy of the alignment and the maximum time between
 * synchronization updates is affected by the resolution of this
 * clock.
 *
 * On some systems the hardware clock from k_cycles_get_32() is
 * suitable, but on others that clock advances too quickly.  The
 * frequency of the target-specific clock is provided by
 * maxim_ds3231_syncclock_frequency().
 *
 * At this time the value is captured from `k_uptime_get_32()`; future
 * kernel extensions may make a higher-resolution clock available.
 *
 * @note This function is *isr-ok*.
 *
 * @param dev the DS3231 device pointer
 *
 * @return the current value of the synchronization clock.
 */
static inline uint32_t maxim_ds3231_read_syncclock(const struct device *dev)
{
	return k_uptime_get_32();
}

/** @brief Get the frequency of the synchronization clock.
 *
 * Provides the frequency of the clock used in maxim_ds3231_read_syncclock().
 *
 * @param dev the DS3231 device pointer
 *
 * @return the frequency of the selected synchronization clock.
 */
static inline uint32_t maxim_ds3231_syncclock_frequency(const struct device *dev)
{
	return 1000U;
}

/**
 * @brief Set and clear specific bits in the control register.
 *
 * @note This function assumes the device register cache is valid.  It
 * will not read the register value, and it will write to the device
 * only if the value changes as a result of applying the set and clear
 * changes.
 *
 * @note Unlike maxim_ds3231_stat_update() the return value from this
 * function indicates the register value after changes were made.
 * That return value is cached for use in subsequent operations.
 *
 * @note This function is *supervisor*.
 *
 * @return the non-negative updated value of the register, or a
 * negative error code from an I2C transaction.
 */
int maxim_ds3231_ctrl_update(const struct device *dev,
			     uint8_t set_bits,
			     uint8_t clear_bits);

/**
 * @brief Read the ctrl_stat register then set and clear bits in it.
 *
 * The content of the ctrl_stat register will be read, then the set
 * and clear bits applied and the result written back to the device
 * (regardless of whether there appears to be a change in value).
 *
 * OSF, A1F, and A2F will be written with 1s if the corresponding bits
 * do not appear in either @p set_bits or @p clear_bits.  This ensures
 * that if any flag becomes set between the read and the write that
 * indicator will not be cleared.
 *
 * @note Unlike maxim_ds3231_ctrl_update() the return value from this
 * function indicates the register value before any changes were made.
 *
 * @note This function is *supervisor*.
 *
 * @param dev the DS3231 device pointer
 *
 * @param set_bits bits to be set when writing back.  Setting bits
 * other than @ref MAXIM_DS3231_REG_STAT_EN32kHz will have no effect.
 *
 * @param clear_bits bits to be cleared when writing back.  Include
 * the bits for the status flags you want to clear.
 *
 * @return the non-negative register value as originally read
 * (disregarding the effect of clears and sets), or a negative error
 * code from an I2C transaction.
 */
int maxim_ds3231_stat_update(const struct device *dev,
			     uint8_t set_bits,
			     uint8_t clear_bits);

/** @brief Read a DS3231 alarm configuration.
 *
 * The alarm configuration data is read from the device and
 * reconstructed into the output parameter.
 *
 * @note This function is *supervisor*.
 *
 * @param dev the DS3231 device pointer.
 *
 * @param id the alarm index, which must be 0 (for the 1 s resolution
 * alarm) or 1 (for the 1 min resolution alarm).
 *
 * @param cfg a pointer to a structure into which the configured alarm
 * data will be stored.
 *
 * @return a non-negative value indicating successful conversion, or a
 * negative error code from an I2C transaction or invalid parameter.
 */
int maxim_ds3231_get_alarm(const struct device *dev,
			   uint8_t id,
			   struct maxim_ds3231_alarm *cfg);

/** @brief Configure a DS3231 alarm.
 *
 * The alarm configuration is validated and stored into the device.
 *
 * To cancel an alarm use counter_cancel_channel_alarm().
 *
 * @note This function is *supervisor*.
 *
 * @param dev the DS3231 device pointer.
 *
 * @param id 0 Analog to counter index.  @c ALARM1 is 0 and has 1 s
 * resolution, @c ALARM2 is 1 and has 1 minute resolution.
 *
 * @param cfg a pointer to the desired alarm configuration.  Both
 * alarms are configured; if only one is to change the application
 * must supply the existing configuration for the other.
 *
 * @return a non-negative value on success, or a negative error code
 * from an I2C transaction or an invalid parameter.
 */
int maxim_ds3231_set_alarm(const struct device *dev,
			   uint8_t id,
			   const struct maxim_ds3231_alarm *cfg);

/** @brief Synchronize the RTC against the local clock.
 *
 * The RTC advances one tick per second with no access to sub-second
 * precision.  Synchronizing clocks at sub-second resolution requires
 * enabling a 1pps signal then capturing the system clocks in a GPIO
 * callback.  This function provides that operation.
 *
 * Synchronization is performed in asynchronously, and may take as
 * long as 1 s to complete; notification of completion is provided
 * through the @p notify parameter.
 *
 * Applications should use maxim_ds3231_get_syncpoint() to retrieve the
 * synchronization data collected by this operation.
 *
 * @note This function is *supervisor*.
 *
 * @param dev the DS3231 device pointer.
 *
 * @param notify pointer to the object used to specify asynchronous
 * function behavior and store completion information.
 *
 * @retval non-negative on success
 * @retval -EBUSY if a synchronization or set is currently in progress
 * @retval -EINVAL if notify is not provided
 * @retval -ENOTSUP if the required interrupt is not configured
 */
int maxim_ds3231_synchronize(const struct device *dev,
			     struct sys_notify *notify);

/** @brief Request to update the synchronization point.
 *
 * This is a variant of maxim_ds3231_synchronize() for use from user
 * threads.
 *
 * @param dev the DS3231 device pointer.
 *
 * @param signal pointer to a valid and ready-to-be-signalled
 * k_poll_signal.  May be NULL to request a synchronization point be
 * collected without notifying when it has been updated.
 *
 * @retval non-negative on success
 * @retval -EBUSY if a synchronization or set is currently in progress
 * @retval -ENOTSUP if the required interrupt is not configured
 */
__syscall int maxim_ds3231_req_syncpoint(const struct device *dev,
					 struct k_poll_signal *signal);

/** @brief Retrieve the most recent synchronization point.
 *
 * This function returns the synchronization data last captured using
 * maxim_ds3231_synchronize().
 *
 * @param dev the DS3231 device pointer.
 *
 * @param syncpoint where to store the synchronization data.
 *
 * @retval non-negative on success
 * @retval -ENOENT if no syncpoint has been captured
 */
__syscall int maxim_ds3231_get_syncpoint(const struct device *dev,
					 struct maxim_ds3231_syncpoint *syncpoint);

/** @brief Set the RTC to a time consistent with the provided
 * synchronization.
 *
 * The RTC advances one tick per second with no access to sub-second
 * precision, and setting the clock resets the internal countdown
 * chain.  This function implements the magic necessary to set the
 * clock while retaining as much sub-second accuracy as possible.  It
 * requires a synchronization point that pairs sub-second resolution
 * civil time with a local synchronization clock captured at the same
 * instant.  The set operation may take as long as 1 second to
 * complete; notification of completion is provided through the @p
 * notify parameter.
 *
 * @note This function is *supervisor*.
 *
 * @param dev the DS3231 device pointer.
 *
 * @param syncpoint the structure providing the synchronization point.
 *
 * @param notify pointer to the object used to specify asynchronous
 * function behavior and store completion information.
 *
 * @retval non-negative on success
 * @retval -EINVAL if syncpoint or notify are null
 * @retval -ENOTSUP if the required interrupt signal is not configured
 * @retval -EBUSY if a synchronization or set is currently in progress
 */
int maxim_ds3231_set(const struct device *dev,
		     const struct maxim_ds3231_syncpoint *syncpoint,
		     struct sys_notify *notify);

/** @brief Check for and clear flags indicating that an alarm has
 * fired.
 *
 * Returns a mask indicating alarms that are marked as having fired,
 * and clears from stat the flags that it found set.  Alarms that have
 * been configured with a callback are not represented in the return
 * value.
 *
 * This API may be used when a persistent alarm has been programmed.
 *
 * @note This function is *supervisor*.
 *
 * @param dev the DS3231 device pointer.
 *
 * @return a non-negative value that may have MAXIM_DS3231_ALARM1 and/or
 * MAXIM_DS3231_ALARM2 set, or a negative error code.
 */
int maxim_ds3231_check_alarms(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/* @todo this should be syscalls/drivers/rtc/maxim_ds3231.h */
#include <syscalls/maxim_ds3231.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_DS3231_H_ */
