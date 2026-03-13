/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NPM10XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NPM10XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys/slist.h>
#include <zephyr/sys/clock.h>
#include <zephyr/device.h>

/**
 * @file npm10xx.h
 * @brief nPM10xx PMICs MFD interface
 * @defgroup mfd_interface_npm10xx MFD NPM10XX Interface
 * @ingroup mfd_interfaces
 * @since 4.4
 * @version 0.1.0
 * @{
 */

/**
 * @name RESET reasons
 * @{
 */
/** VBAT Hibernate mode exit by VBUS connected. */
#define NPM10XX_RSTREAS_HIBERNATE     BIT(0)
/** Boot monitor reset. */
#define NPM10XX_RSTREAS_BOOTMON       BIT(1)
/** Watchdog reset. */
#define NPM10XX_RSTREAS_WATCHDOG      BIT(2)
/** Longpress reset. */
#define NPM10XX_RSTREAS_LONGPRESS     BIT(3)
/** Thermal shutdown reset. */
#define NPM10XX_RSTREAS_THERMAL       BIT(4)
/** POF reset. */
#define NPM10XX_RSTREAS_POF           BIT(5)
/** Software reset (task register). */
#define NPM10XX_RSTREAS_SW            BIT(6)
/** Ship mode entry error reset. */
#define NPM10XX_RSTREAS_SHIPERR       BIT(7)
/** Hibernate mode exit by timer. */
#define NPM10XX_RSTREAS_TIMER         BIT(8)
/** Hibernate mode exit by SHPHLD button. */
#define NPM10XX_RSTREAS_BUTTON        BIT(9)
/** VBUS Hibernate mode exit by VBUS disconnected. */
#define NPM10XX_RSTREAS_HIBERNATEVBUS BIT(10)
/** Watchdog soft reset. */
#define NPM10XX_RSTREAS_WATCHDOGSOFT  BIT(11)
/** @} */

/**
 * @name ADC events
 * @{
 */
/** ADC VBAT conversion ready. */
#define NPM10XX_EVENT_ADC_VBAT    BIT64(0)
/** ADC NTC conversion ready. */
#define NPM10XX_EVENT_ADC_NTC     BIT64(1)
/** ADC die temperature conversion ready. */
#define NPM10XX_EVENT_ADC_DIETEMP BIT64(2)
/** ADC VSYS conversion ready. */
#define NPM10XX_EVENT_ADC_VSYS    BIT64(3)
/** ADC VBUS conversion ready. */
#define NPM10XX_EVENT_ADC_VBUS    BIT64(4)
/** ADC VSET conversion ready. */
#define NPM10XX_EVENT_ADC_VSET    BIT64(5)
/** ADC OFFSET conversion ready. */
#define NPM10XX_EVENT_ADC_OFFSET  BIT64(6)
/** IBAT/VBAT battery current and/or voltage conversion ready. */
#define NPM10XX_EVENT_IBATVBAT    BIT64(7)
/** @} */

/**
 * @name CHARGER events
 * @{
 */
/** Charger started. */
#define NPM10XX_EVENT_CHARGE     BIT64(8)
/** Battery discharge started. */
#define NPM10XX_EVENT_DISCHARGE  BIT64(9)
/** Dropout loop activated. */
#define NPM10XX_EVENT_DROPOUT    BIT64(10)
/** Supplement mode activated. */
#define NPM10XX_EVENT_SUPPLEMENT BIT64(11)
/** Battery charging completed. */
#define NPM10XX_EVENT_COMPLETE   BIT64(12)
/** Battery discharge limit activated. */
#define NPM10XX_EVENT_BATILIM    BIT64(13)
/** Charging current below ITERM. */
#define NPM10XX_EVENT_ITERM      BIT64(14)
/** Battery voltage has reached VTERM. */
#define NPM10XX_EVENT_VTERM      BIT64(16)
/** Battery voltage below VRECHARGE. */
#define NPM10XX_EVENT_RECHARGE   BIT64(17)
/** Battery voltage above VTHROTTLE. */
#define NPM10XX_EVENT_VTHROTTLE  BIT64(18)
/** Battery voltage above VWEAK. */
#define NPM10XX_EVENT_VWEAK      BIT64(19)
/** Battery voltage above VTRICKLEFAST. */
#define NPM10XX_EVENT_VTRICKLE   BIT64(20)
/** Battery voltage below VBATLOW. */
#define NPM10XX_EVENT_VBATLOW    BIT64(21)
/** Battery temperature in cold region. */
#define NPM10XX_EVENT_NTCCOLD    BIT64(24)
/** Battery temperature in cool region. */
#define NPM10XX_EVENT_NTCCOOL    BIT64(25)
/** Battery temperature in warm region. */
#define NPM10XX_EVENT_NTCWARM    BIT64(26)
/** Battery temperature in hot region. */
#define NPM10XX_EVENT_NTCHOT     BIT64(27)
/** Die temperature is over TCHGREDUCE. */
#define NPM10XX_EVENT_DIETEMP    BIT64(28)
/** Charging error. */
#define NPM10XX_EVENT_ERROR      BIT64(29)
/** VSYSWRNR threshold detected. */
#define NPM10XX_EVENT_VSYSWRNPE  BIT64(30)
/** VSYSWRNF threshold detected. */
#define NPM10XX_EVENT_VSYSWRNNE  BIT64(31)
/** @} */

/**
 * @name SYSREG/VBUS events
 * @{
 */
/** VBUS overvoltage detected. */
#define NPM10XX_EVENT_VBUSOVPE      BIT64(32)
/** VBUS overvoltage removed. */
#define NPM10XX_EVENT_VBUSOVNE      BIT64(33)
/** VBUS undervoltage detected. */
#define NPM10XX_EVENT_VBUSUNDERPE   BIT64(34)
/** VBUS undervoltage removed. */
#define NPM10XX_EVENT_VBUSUNDERNE   BIT64(35)
/** VBUS detected. */
#define NPM10XX_EVENT_VBUSPRESENTPE BIT64(36)
/** VBUS removed. */
#define NPM10XX_EVENT_VBUSPRESENTNE BIT64(37)
/** VBUS within limit. */
#define NPM10XX_EVENT_VBUSOKPE      BIT64(38)
/** VBUS outside the limit. */
#define NPM10XX_EVENT_VBUSOKNE      BIT64(39)
/** @} */

/**
 * @name GPIO events
 * @{
 */
/** Rising edge on GPIO0. */
#define NPM10XX_EVENT_GPIO0PE BIT64(40)
/** Falling edge on GPIO0. */
#define NPM10XX_EVENT_GPIO0NE BIT64(41)
/** Rising edge on GPIO1. */
#define NPM10XX_EVENT_GPIO1PE BIT64(42)
/** Falling edge on GPIO1. */
#define NPM10XX_EVENT_GPIO1NE BIT64(43)
/** Rising edge on GPIO2. */
#define NPM10XX_EVENT_GPIO2PE BIT64(44)
/** Falling edge on GPIO2. */
#define NPM10XX_EVENT_GPIO2NE BIT64(45)
/** @} */

/**
 * @name SYSTEM events
 * @{
 */
/** General purpose timer expired. */
#define NPM10XX_EVENT_TIMERTIMEOUT BIT64(48)
/** Timer free event. */
#define NPM10XX_EVENT_TIMERFREE    BIT64(49)
/** Pre‑warning event before timer expires. */
#define NPM10XX_EVENT_TIMERPREWARN BIT64(50)
/** LOADSW1/LDO1 overcurrent. */
#define NPM10XX_EVENT_LDOSWOC      BIT64(51)
/** LOADSW2 overcurrent. */
#define NPM10XX_EVENT_LOADSWOC     BIT64(52)
/** Rising edge on SHPHLD. */
#define NPM10XX_EVENT_SHPHLDPE     BIT64(53)
/** Falling edge on SHPHLD. */
#define NPM10XX_EVENT_SHPHLDNE     BIT64(54)
/** ADC test finished. */
#define NPM10XX_EVENT_ADCTEST      BIT64(55)
/** @} */

/** nPM10xx Hibernate modes */
enum mfd_npm10xx_hibernate_mode {
	/** Hibernate when powered from VBAT. */
	NPM10XX_HIBERNATE_VBAT,
	/** Hibernate when powered from VBUS. Wait for charge completion. */
	NPM10XX_HIBERNATE_VBUS_WAIT,
	/** Hibernate when powered from VBUS. Do not wait for charge completion. */
	NPM10XX_HIBERNATE_VBUS_NOWAIT,
};

/** nPM10xx Standby operations */
enum mfd_npm10xx_standby_op {
	/** Enter VBUS Standby1 mode (VBUS max 12V). Wait for charge completion. */
	NPM10XX_STANDBY1_ENTER_WAIT,
	/** Enter VBUS Standby1 mode (VBUS max 12V). Do not wait for charge completion. */
	NPM10XX_STANDBY1_ENTER_NOWAIT,
	/** Enter VBUS Standby2 mode (VBUS max 5V5). Wait for charge completion. */
	NPM10XX_STANDBY2_ENTER_WAIT,
	/** Enter VBUS Standby2 mode (VBUS max 5V5). Do not wait for charge completion. */
	NPM10XX_STANDBY2_ENTER_NOWAIT,
	/** Exit VBUS Standby mode. */
	NPM10XX_STANDBY_EXIT,
};

/** @brief Identifies a set of events for the event callback */
typedef uint64_t npm10xx_event_t;

/**
 * @brief Identifies a reset reason
 *
 * Intended to be checked against the NPM10XX_RSTREAS_* macros once retrieved.
 */
typedef uint16_t npm10xx_rstreas_t;

struct mfd_npm10xx_event_callback;

/**
 * @typedef npm10xx_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param mfd_npm10xx_dev nPM10xx MFD device
 * @param cb Original struct mfd_npm10xx_event_callback owning this handler
 * @param events Mask of events that triggers the callback handler
 *
 * Note: cb pointer can be used to retrieve private data through CONTAINER_OF() if original struct
 * mfd_npm10xx_event_callback is stored in another private structure.
 */
typedef void (*npm10xx_callback_handler_t)(const struct device *mfd_npm10xx_dev,
					   struct mfd_npm10xx_event_callback *cb,
					   npm10xx_event_t events);

/**
 * @brief nPM10xx MFD callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct mfd_npm10xx_event_callback.
 */
struct mfd_npm10xx_event_callback {
	/** Intended for internal use by the driver. Do not use in application. */
	sys_snode_t node;
	/**
	 * A set of events the callback is intrested in. May be constructed by a bitwise-OR of any
	 * number of the NPM10XX_EVENT_* macros.
	 */
	npm10xx_event_t event_mask;
	/** Function to be called when any of the provided events occur. */
	npm10xx_callback_handler_t handler;
};

/**
 * @brief Reset the nPM10xx PMIC device
 *
 * Turn off all supplies and apply internal reset. Clear all registers except reset registers.
 *
 * @param dev nPM10xx MFD device
 *
 * @return 0 on success, negative errno otherwise
 */
int mfd_npm10xx_reset(const struct device *dev);

/**
 * @brief Retrieve and clear the reset reason of the nPM10xx PMIC device
 *
 * @param dev nPM10xx MFD device
 * @param reason Pointer to the variable to write the reset reason into
 *
 * @return 0 on success, negative errno otherwise
 */
int mfd_npm10xx_get_reset_reason(const struct device *dev, npm10xx_rstreas_t *reason);

/**
 * @brief Trigger a Hibernate mode on an nPM10xx PMIC device
 *
 * This will turn off all supplies and any devices powered from them. The VBAT Hibernate mode is
 * exited automatically when VBUS becomes present. The VBUS Hibernate mode is exited when VBUS is
 * removed. The SHPHLD button can be configured to exit Hibernate modes as well.
 * NOTE: currently only K_FOREVER timeout is supported.
 *
 * @param dev nPM10xx MFD device
 * @param mode The Hibernate mode to trigger
 * @param time Timeout of the Hibernate mode. Use K_FOREVER to hibernate without a timeout.
 *
 * @return 0 on success, negative errno otherwise
 */
int mfd_npm10xx_hibernate(const struct device *dev, enum mfd_npm10xx_hibernate_mode mode,
			  k_timeout_t time);

/**
 * @brief Enter or exit a VBUS Standby mode on an nPM10xx PMIC device
 *
 * VBUS Standby modes reduce quiescent current drawn from VBUS. The device exits these modes
 * automatically when VBUS is no longer present.
 *
 * @param dev nPM10xx MFD device
 * @param operation The Standby operation to execute
 *
 * @return 0 on success, negative errno otherwise
 */
int mfd_npm10xx_standby(const struct device *dev, enum mfd_npm10xx_standby_op operation);

/**
 * @brief Manage nPM10xx event callbacks
 *
 * On removal, when no callbacks handling a given event are left, the interrupts from this event
 * will be disabled.
 *
 * @param dev nPM10xx MFD device
 * @param callback A pointer to the callback to be added or removed from the list
 * @param add Boolean indicating insertion or removal of the callback
 *
 * @return 0 on success, negative errno otherwise
 */
int mfd_npm10xx_manage_callback(const struct device *dev,
				struct mfd_npm10xx_event_callback *callback, bool add);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NPM10XX_H_ */
