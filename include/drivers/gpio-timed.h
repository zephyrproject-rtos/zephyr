/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for Timed GPIO drivers
 */
#ifndef __GPIO_TIMED_H__
#define __GPIO_TIMED_H__

/**
 * @brief Timed GPIO Interface
 * @defgroup tgpio_interface Timed GPIO Interface
 * @ingroup io_interfaces
 * @{
 */

#include <sys/__assert.h>
#include <sys/slist.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <syscall_handler.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TGPIO timer selection
 */
enum tgpio_timer {
	TGPIO_TMT_0 = 0,
	TGPIO_TMT_1,
	TGPIO_TMT_2,
	TGPIO_ART,
};

/**
 * @brief Event polarity
 */
enum tgpio_pin_polarity {
	TGPIO_RISING_EDGE = 0,
	TGPIO_FALLING_EDGE,
	TGPIO_TOGGLE_EDGE,
};

/**
 * @brief Structure representing time
 * @param nsec Time value in nanoseconds
 * @param sec Time value in seconds
 */
struct tgpio_time {
	uint32_t nsec;
	uint32_t sec;
};


struct more_args {
	uint32_t arg0;
	uint32_t arg1;
};

/**
 * @typedef tgpio_irq_config_func_t
 * @brief For configuring IRQ on each individual TGPIO device.
 *
 * @param port TGPIO device structure.
 *
 * @internal
 */
typedef void (*tgpio_irq_config_func_t)(const struct device *port);

/**
 * @typedef tgpio_pin_callback_t
 * @brief Define the application callback handler function for a TGPIO pin
 *
 * @param port TGPIO device
 * @param pin TGPIO pin
 * @param timestamp Timestamp of the last event occurred
 * @param event_counter Number of events occurred until last event
 *
 */

typedef void (*tgpio_pin_callback_t)(const struct device *port, uint32_t pin,
			struct tgpio_time timestamp, uint64_t event_counter);

/**
 * @cond INTERNAL_HIDDEN
 *
 * TGPIO driver API definition and system call entry points
 *
 * (Internal use only.)
 */

__subsystem struct tgpio_driver_api {
	int (*pin_disable)(const struct device *port, uint32_t pin);
	int (*pin_enable)(const struct device *port, uint32_t pin);
	int (*set_time)(const struct device *port, enum tgpio_timer
		timer, uint32_t sec, uint32_t ns);
	int (*get_time)(const struct device *port, enum tgpio_timer
		timer, uint32_t *sec, uint32_t *ns);
	int (*adjust_time)(const struct device *port,
		enum tgpio_timer timer, int32_t nsec);
	int (*adjust_frequency)(const struct device *port,
		enum tgpio_timer local_clock, int32_t ppb);
	int (*get_cross_timestamp)(const struct device *port,
		enum tgpio_timer local_clock, enum tgpio_timer ref_clock,
		struct tgpio_time *local_time, struct tgpio_time *ref_time);
	int (*set_perout)(const struct device *dev, uint32_t pin,
		enum tgpio_timer timer, struct tgpio_time *start_time,
		struct tgpio_time *repeat_interval, struct more_args *margs);
	int (*ext_ts)(const struct device *dev, uint32_t pin,
		enum tgpio_timer timer, uint32_t events_ceiling,
		enum tgpio_pin_polarity edge, tgpio_pin_callback_t cb);
	int (*count_events)(const struct device *dev, uint32_t pin,
		enum tgpio_timer timer, struct tgpio_time start_time,
		struct tgpio_time repeat_interval, enum tgpio_pin_polarity edge,
		tgpio_pin_callback_t cb);
};

/**
 * @endcond
 */

/**
 * @brief Set time in a timer
 *
 * @param port TGPIO device
 * @param timer TGPIO timer
 * @param sec Timer value in seconds
 * @param ns Timer value in nanoseconds
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_port_set_time(const struct device *port,
			enum tgpio_timer timer, uint32_t sec, uint32_t ns);

static inline int z_impl_tgpio_port_set_time(const struct device *port,
		enum tgpio_timer timer, uint32_t sec, uint32_t ns)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->set_time(port, timer, sec, ns);
}

/**
 * @brief Get time from a timer
 *
 * @param port TGPIO device
 * @param timer TGPIO timer
 * @param sec Pointer to store timer value in seconds
 * @param ns Pointer to store timer value in nanoseconds
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_port_get_time(const struct device *port,
		enum tgpio_timer timer, uint32_t *sec, uint32_t *ns);

static inline int z_impl_tgpio_port_get_time(const struct device *port,
		enum tgpio_timer timer, uint32_t *sec, uint32_t *ns)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->get_time(port, timer, sec, ns);
}

/**
 * @brief Adjusts time dynamically in a timer up to 1(999,999,986 ns max) second
 *
 * @param port TGPIO device
 * @param timer TGPIO timer
 * @param nsec Time value in nanoseconds, sign indicates subtraction or
 *				addition
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_port_adjust_time(const struct device *port,
				enum tgpio_timer timer, int32_t nsec);

static inline int z_impl_tgpio_port_adjust_time(const struct device *port,
				enum tgpio_timer timer, int32_t nsec)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->adjust_time(port, timer, nsec);
}

/**
 * @brief  Adjusts the rate of a running timer.
 *
 * @param port TGPIO device
 * @param timer TGPIO timer
 * @param ppb Parts per billion (nanoseconds)
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_port_adjust_frequency(const struct device *port,
					enum tgpio_timer timer, int32_t ppb);

static inline int z_impl_tgpio_port_adjust_frequency(const struct device *port,
					enum tgpio_timer timer, int32_t ppb)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->adjust_frequency(port, timer, ppb);
}

/**
 * @brief Captures cross timestamps for given timers
 *
 * @param port TGPIO device
 * @param local_clock Local timer to be snapshot-ed
 * @param ref_clock Reference clock to be snapshot-ed
 * @param local_time Pointer to store snapshot-ed local time
 * @param ref_time Pointer to store snapshot-ed reference time
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_get_cross_timestamp(const struct device *port,
		enum tgpio_timer local_clock, enum tgpio_timer ref_clock,
		struct tgpio_time *local_time, struct tgpio_time *ref_time);

static inline int z_impl_tgpio_get_cross_timestamp(const struct device *port,
		  enum tgpio_timer local_clock, enum tgpio_timer ref_clock,
		  struct tgpio_time *local_time, struct tgpio_time *ref_time)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->get_cross_timestamp(port, local_clock, ref_clock,
					local_time, ref_time);
}

/**
 * @brief Disable operation on pin
 *
 * @param port TGPIO device
 * @param pin TGPIO pin
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_pin_disable(const struct device *port, uint32_t pin);

static inline int z_impl_tgpio_pin_disable(const struct device *port,
					uint32_t pin)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->pin_disable(port, pin);
}

/**
 * @brief Enable/Continue operation on pin
 *
 * @param port TGPIO device
 * @param pin TGPIO pin
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_pin_enable(const struct device *port, uint32_t pin);

static inline int z_impl_tgpio_pin_enable(const struct device *port,
					uint32_t pin)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->pin_enable(port, pin);
}

/** @cond INTERNAL_HIDDEN */
__syscall int tgpio_pin_periodic_output(const struct device *port, uint32_t pin,
		enum tgpio_timer timer, struct tgpio_time *start_time,
		struct tgpio_time *repeat_interval, uint32_t repeat_count,
		uint32_t pws);

static inline int z_impl_tgpio_pin_periodic_output(const struct device *port,
		uint32_t pin, enum tgpio_timer timer,
		struct tgpio_time *start_time,
		struct tgpio_time *repeat_interval,
		uint32_t repeat_count, uint32_t pws)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;
	struct more_args margs;

	margs.arg0 = repeat_count;
	margs.arg1 = pws;
	return api->set_perout(port, pin, timer, start_time, repeat_interval,
			       &margs);
}

/** @endcond */

/**
 * @brief Generates pulses on a gpio pin.
 *
 * @param port TGPIO device
 * @param pin TGPIO pin
 * @param timer TGPIO timer
 * @param start_time Time to start first pulse
 * @param repeat_interval Interval between two subsequent pulses
 * @param repeat_count Repeatation count. 0 for infinite.
 *		Any out of range value will be restricted to max or min values
 * @param pws Pulse width stretch in h/w cycles. Possible values [2-17].
 *		Any out of range value will be restricted to max or min values
 *
 * @return 0 if successful, negative errno code on failure.
 */

static inline int tgpio_pin_periodic_out(const struct device *port,
	uint32_t pin, enum tgpio_timer timer, struct tgpio_time start_time,
	struct tgpio_time repeat_interval,
	uint32_t repeat_count, uint32_t pws)
{

	return tgpio_pin_periodic_output(port, pin, timer, &start_time,
					&repeat_interval, repeat_count, pws);

}

/**
 * @brief Timestamp an event on a pin. Timestamping can be captured for every
 *		event or once in 'n' number of events.
 *
 * @param port TGPIO device
 * @param pin TGPIO pin
 * @param timer TGPIO timer
 * @param events_ceiling Capture timestamp and fire callback after
 * 'events_ceiling' times.
 * @param edge Edge polarity.
 * @param cb Callback to be called after 'events_ceiling' times
 *
 * @return 0 if successful, negative errno code on failure.
 */
static inline int tgpio_pin_timestamp_event(const struct device *port,
		uint32_t pin, enum tgpio_timer timer, uint32_t events_ceiling,
		enum tgpio_pin_polarity edge, tgpio_pin_callback_t cb)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->ext_ts(port, pin, timer, events_ceiling, edge, cb);
}

/**
 * @brief Count number of events in a span of time
 *
 * @param port TGPIO device
 * @param pin TGPIO pin
 * @param timer TGPIO timer
 * @param start_time Time for first interrupt/callback
 * @param repeat_interval Interval between subsequent interrupt/callback
 * @param edge Edge polarity.
 * @param cb Callback to be called after every 'repeat_interval'
 *
 * @return 0 if successful, negative errno code on failure.
 */
static inline int tgpio_pin_count_events(const struct device *port,
	uint32_t pin, enum tgpio_timer timer, struct tgpio_time start_time,
	struct tgpio_time repeat_interval,
	enum tgpio_pin_polarity edge, tgpio_pin_callback_t cb)
{
	const struct tgpio_driver_api *api =
		(const struct tgpio_driver_api *)port->api;

	return api->count_events(port, pin, timer, start_time, repeat_interval,
				edge, cb);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/gpio-timed.h>

#endif /* __GPIO_TIMED_H__ */
