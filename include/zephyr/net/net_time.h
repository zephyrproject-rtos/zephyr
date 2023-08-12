/*
 * Copyright (c) 2023 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Representation and manipulation of nanosecond resolution elapsed time
 * and timestamps in the network stack.
 *
 * Inspired by
 * https://github.com/torvalds/linux/blob/master/include/linux/ktime.h and
 * https://github.com/torvalds/linux/blob/master/[tools/]include/linux/time64.h
 *
 * @defgroup net_time Network time representation.
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_TIME_H_
#define ZEPHYR_INCLUDE_NET_NET_TIME_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Any occurrence of net_time_t specifies a concept of nanosecond
 * resolution scalar time span, future (positive) or past (negative) relative
 * time or absolute timestamp referred to some local network uptime reference
 * clock that does not wrap during uptime and is - in a certain, well-defined
 * sense - common to all local network interfaces, sometimes even to remote
 * interfaces on the same network.
 *
 * This type is EXPERIMENTAL. Usage is currently restricted to representation of
 * time within the network subsystem.
 *
 * @details Timed network protocols (PTP, TDMA, ...) usually require several
 * local or remote interfaces to share a common notion of elapsed time within
 * well-defined tolerances. Network uptime therefore differs from time
 * represented by a single hardware counter peripheral in that it will need to
 * be represented in several distinct hardware peripherals with different
 * frequencies, accuracy and precision. To co-operate, these hardware counters
 * will have to be "syntonized" or "disciplined" (i.e. frequency and phase
 * locked) with respect to a common local or remote network reference time
 * signal. Be aware that while syntonized clocks share the same frequency and
 * phase, they do not usually share the same epoch (zero-point).
 *
 * This also explains why network time, if represented as a cycle value of some
 * specific hardware counter, will never be "precise" but only can be "good
 * enough" with respect to the tolerances (resolution, drift, jitter) required
 * by a given network protocol. All counter peripherals involved in a timed
 * network protocol must comply with these tolerances.
 *
 * Please use specific @ref k_timepoint_t counter values rather than net_time_t
 * whenever possible especially when referring to the kernel system clock or
 * values of any single counter peripheral.
 *
 * net_time_t cannot represent general clocks referred to an arbitrary epoch as
 * it only covers roughly +/- ~290 years. It also cannot be used to represent
 * time according to a more complex timescale (e.g. including leap seconds, time
 * adjustments, complex calendars or time zones). In these cases you may use
 * @ref timespec (C11, POSIX.1-2001), @ref timeval (POSIX.1-2001) or broken down
 * time as in @ref tm (C90). The advantage of net_time_t over these structured
 * time representations is lower memory footprint, faster and simpler scalar
 * arithmetics and easier conversion from/to low-level hardware counter values.
 * Also net_time_t can be used in the network stack as well as in applications
 * while POSIX concepts cannot. Converting net_time_t from/to structured time
 * representations is possible in a limited way but - except for @ref timespec -
 * requires concepts that must be implemented by higher-level APIs. Utility
 * functions converting from/to @ref timespec will be provided as part of the
 * net_time_t API as and when needed.
 *
 * If you want to represent more coarse grained scalar time in network
 * applications, use @ref time_t (C99, POSIX.1-2001) which is specified to
 * represent seconds or @ref suseconds_t (POSIX.1-2001) for microsecond
 * resolution. Kernel @ref k_ticks_t and cycles (both specific to Zephyr) have
 * an unspecified resolution but are useful to represent the kernel's notion of
 * time and implement high resolution spinning.
 *
 * If you need even finer grained time resolution, you may want to look at
 * (g)PTP concepts, see @ref net_ptp_extended_time.
 *
 * The reason why we don't use int64_t directly to represent scalar nanosecond
 * resolution times in the network stack is that it has been shown in the past
 * that fields using generic type will often not be used correctly (e.g. with
 * the wrong resolution or to represent underspecified concepts of time with
 * unclear syntonization semantics).
 *
 * Any API that exposes or consumes net_time_t values SHALL ensure that it
 * maintains the specified contract including all protocol specific tolerances
 * and therefore clients can rely on common semantics of this type. This makes
 * times coming from different hardware peripherals and even from different
 * network nodes comparable within well-defined limits and therefore net_time_t
 * is the ideal intermediate building block for timed network protocols.
 */
typedef int64_t net_time_t;

/** The largest positive time value that can be represented by net_time_t */
#define NET_TIME_MAX INT64_MAX

/** The smallest negative time value that can be represented by net_time_t */
#define NET_TIME_MIN INT64_MIN

/** The largest positive number of seconds that can be safely represented by net_time_t */
#define NET_TIME_SEC_MAX (NET_TIME_MAX / NSEC_PER_SEC)

/** The smallest negative number of seconds that can be safely represented by net_time_t */
#define NET_TIME_SEC_MIN (NET_TIME_MIN / NSEC_PER_SEC)

/**
 * Defines the semantics used to convert from syntonized net_time_t values to
 * network counter timepoints.
 *
 * This is a best effort approach. The closer the last syntonization instant the
 * more accurately will the given timepoint value represent the actual reference
 * time. Some residual uncertainty cannot be avoided due to the fact that the
 * underlying network counter timepoint value will not be able to represent
 * nanosecond precision but will be several orders of magnitude less precise.
 *
 * Call get_uncertainty() to estimate the worst case offset of the time
 * represented by the rounded target counter value from the actual reference
 * clock time.
 */
enum net_time_rounding {
	/**
	 * Determine or schedule the network counter timepoint @em closest to the
	 * given nanosecond precision instant.
	 */
	NET_TIME_ROUNDING_NEAREST_TIMEPOINT,

	/**
	 * Determine or schedule the earliest network counter timepoint @em after
	 * the given nanosecond precision instant.
	 */
	NET_TIME_ROUNDING_NEXT_TIMEPOINT,

	/**
	 * Determine or schedule the latest network counter timepoint @em before
	 * the given nanosecond precision instant.
	 */
	NET_TIME_ROUNDING_PREVIOUS_TIMEPOINT,
};

struct net_if;

/**
 * @brief Monotonic network uptime counter of a given interface.
 *
 * Network uptime counter implementations SHOULD wrap two distinct hardware
 * counter peripherals. One high-resolution counter that provides best accuracy
 * of timeouts, hardware-supported scheduling and timestamping of TX and RX. And
 * one always-on, low-power, low-resolution sleep counter that will bridge long
 * waiting times.
 *
 * The timer is considered to be "suspended" when its high-resolution counter is
 * not powered. It is "active" when its high-resolution mode counter is
 * operational. The sleep counter MAY remain operational in "active" mode. The
 * timer SHALL be "suspended" initially.
 *
 * If "suspended", the counter SHALL switch to "active" mode early enough before
 * a scheduled timer fires so that the timer does not oversleep. The counter
 * SHALL NOT be suspended unless its may_sleep() function has been called. The
 * counter SHALL be suspended as quickly as possible when its may_sleep()
 * function has been called. If sleep mode is not supported, calls to
 * may_sleep() are no-ops.
 *
 * TODO: Use runtime driver power management API instead.
 *
 * Synchronization between the sleep counter and the high-resolution counter
 * SHOULD be done in hardware for lowest synchronization jitter and latency.
 *
 * Switching between the sleep counter and the high-resolution counter MUST not
 * violate the requirement of monotonic uptime, i.e. the value returned by
 * get_current_timepoint() SHALL never jump backwards. In suspended mode the
 * counter MAY jump forwards i.e. certain timepoints MAY be left out. This
 * SHALL, however, NOT lead to timeouts being lost.
 *
 * Uptime counters MAY be implemented internally building upon hardware
 * peripherals directly, existing drivers such as counter.h, system_timer.h or
 * rtc.h drivers. Typical sleep counters would be rtc.h or system_timer.h
 * drivers or counter.h drivers wrapping a low-power, always-on counter
 * peripheral. Typical high-resolution counters would be based on counter.h or
 * system_timer.h.
 *
 * As hardware synchronization of sleep and high-resolution counters requires
 * knowledge of both peripherals at hardware level, these drivers need to be
 * amended by low-level synchronization code in the network uptime counter
 * implementation. Switching between suspended and active mode will also require
 * additional code that is specific to each network uptime counter
 * implementation. This is the reason why network uptime counters require their
 * own API above existing counter APIs.
 */
struct net_time_counter_api {
	/**
	 * @brief Initialize the network uptime counter.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] iface pointer to the network interface to which this
	 * counter is bound
	 *
	 * @retval 0 The counter has been successfully initialized.
	 *
	 * @retval -ENOENT Initialization could not allocate some required resource.
	 */
	int (*init)(const struct net_time_counter_api *api, struct net_if *iface);

	/**
	 * @brief The interface's current overflow protected network uptime
	 * counter value represented in the counter's own resolution.
	 *
	 * @note Calling this method from the context of a timer expiry function
	 * context will provide the exact timepoint at which the timer fired.
	 *
	 * @param[in] api pointer to this api
	 * @param[out] timepoint the current timepoint value
	 *
	 * @retval 0 The counter is in high-precision mode and a precise
	 * counter timepoint has been provided.
	 *
	 * @retval -EIO The high-precision timer is currently not available,
	 * probably because the counter is in sleep mode. If a timepoint
	 * different from K_TIMEPOINT_NEVER is returned, this is a rough
	 * estimate of the actual current timepoint with reduced accuracy.
	 */
	int (*get_current_timepoint)(const struct net_time_counter_api *api,
				     k_timepoint_t *timepoint);

	/**
	 * @brief Converts the given opaque counter timepoint into an internal
	 * counter tick representation to be used by low-level drivers (e.g.
	 * network drivers).
	 *
	 * @details The value assigned to the tick parameter does not have any
	 * special semantics other than pointing to a variable that can be
	 * directly used by network interface drivers to program the underlying
	 * network counter peripheral (e.g. timed TX/RX radio timer ticks).
	 *
	 * @note Clients will usually not call this method directly. They'll
	 * program timers and scheduled TX/RX with well defined rounding
	 * semantics instead and the network driver or time reference
	 * implementation will take care of converting time to counter ticks
	 * internally.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] timepoint the timepoint value to be converted
	 * @param[out] tick a pointer to an implementation specific variable apt
	 * to represent driver-internal uptime counter ticks.
	 */
	void (*get_tick_from_timepoint)(const struct net_time_counter_api *api,
					 k_timepoint_t timepoint, void *tick);

	/**
	 * @brief Converts the given driver implementation specific tick to an
	 * opaque counter timepoint.
	 *
	 * @details The value of the tick parameter does not have any special
	 * semantics other than pointing to a variable that has been obtained
	 * directly via the underlying network counter peripheral (e.g. timed
	 * TX/RX radio timer ticks).
	 *
	 * @note Clients will usually not call this method directly. It's use is
	 * reserved for low-level drivers to interface with the time reference
	 * implementation.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] tick pointer to an implementation specific variable
	 * representing driver-internal uptime counter ticks.
	 * @param[out] timepoint a pointer to the converted timepoint value
	 */
	void (*get_timepoint_from_tick)(const struct net_time_counter_api *api, void *tick,
					k_timepoint_t *timepoint);

	/**
	 * @brief Multiplexed timer support on top of the network uptime counter.
	 *
	 * @details Schedule a timer to wake up the network uptime counter into
	 * "active" mode early enough. Then use scheduled RX/TX from the context
	 * of the expiry function to calculate precise offsets from the timer
	 * expiry for high-resolution timed RX/TX on hardware level. Once timed
	 * RX/TX finished, call `may_sleep()` to signal to the counter that it
	 * can now suspend().
	 *
	 * TODO: Use runtime driver power management API instead.
	 *
	 * @note If in low-power mode, the underlying counter SHALL switch to
	 * full-power/high-resolution mode early enough so that the timeout is
	 * guaranteed to not be overslept.
	 *
	 * @note Setting up a new timer from the context of another timer's
	 * expiry function will calculate any relative start timeout from the
	 * exact timepoint the timer fired, not from the current timepoint
	 * value.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] timer pointer to the timer to be started
	 * @param[in] duration initial relative timer duration or absolute
	 * timeout
	 * @param[in] period period of subsequent timeouts (NULL if a single
	 * timeout is to be programmed)
	 */
	void (*timer_start)(const struct net_time_counter_api *api, struct k_timer *timer,
			    k_timeout_t duration, k_timeout_t period);

	/**
	 * @brief Cancel a running timer.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] timer pointer to the timer that should be canceled
	 */
	void (*timer_stop)(const struct net_time_counter_api *api, struct k_timer *timer);

	/**
	 * @brief Calling this function signals to the counter that it should
	 * switch to "active" mode as quickly as possible.
	 *
	 * @details Blocks until the counter activated or until a timeout or
	 * error occurs. No-op if the counter is already "active". Call this
	 * function before accessing get_current_timepoint() to ensure that
	 * precise timepoints will be returned.
	 *
	 * TODO: Use runtime driver power management API instead.
	 *
	 * @param[in] api pointer to this api
	 *
	 * @retval 0 The counter was successfully activated (or was already
	 * "active").
	 *
	 * @retval -EBUSY The counter cannot be activated at this time.
	 *
	 * @retval -ETIMEDOUT The operation timed out.
	 */
	int (*wake_up)(const struct net_time_counter_api *api);

	/**
	 * @brief Calling this function signals to the counter that it may now
	 * suspend until the next timer fires.
	 *
	 * @note Implementations still may decide not to suspend, e.g. if
	 * another timer is due to expire soon such that suspending would risk
	 * missing the timeout or suspending and immediately re-activating would
	 * consume more energy than remaining active.
	 *
	 * TODO: Use runtime driver power management API instead.
	 *
	 * @param[in] api pointer to this api
	 *
	 * @retval 0 The counter was successfully suspended (or is programmed to
	 * suspend as soon as possible).
	 *
	 * @retval -EBUSY The counter cannot be suspended at this time.
	 */
	int (*may_sleep)(const struct net_time_counter_api *api);

	/* @brief The timeout API of this network uptime counter. */
	const struct k_timeout_api timeout_api;

	/** @brief Nominal counter frequency (in Hz) */
	const uint32_t frequency;
};

struct net_time_timer {
	struct k_timer timer;

	/**
	 * The currently programmed nominal expiry time (i.e. w/o rounding) in
	 * terms of syntonized uptime - required to adjust timer after a
	 * syntonization event and calculate error-accumulation free counter
	 * periods.
	 */
	net_time_t current_expiry_ns;

	/**
	 * Period measured in syntonized time for error-accumulation free
	 * calculation of counter periods.
	 */
	net_time_t period_ns;

	/**
	 * The rounding algorithm to be applied on this timer.
	 */
	enum net_time_rounding rounding;

	/* runs in ISR context,
	 * immutable after timer initialization
	 */
	void (*expiry_fn)(struct net_time_timer *timer);

	const struct net_time_reference_api *time_reference_api;
};

/**
 * @brief Syntonized network uptime reference of a given interface.
 */
struct net_time_reference_api {
	/**
	 * @brief Initialize the network uptime reference.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] iface pointer to the network interface to which this
	 * reference is bound
	 *
	 * @retval 0 The reference has been successfully initialized.
	 */
	int (*init)(const struct net_time_reference_api *api, struct net_if *iface);

	/**
	 * @brief Monotonic syntonized uptime in nanoseconds referred to the
	 * network reference clock as represented by the interface's network
	 * uptime counter.
	 *
	 * @note The returned value will be systematically offset due to the
	 * latency introduced by involving the CPU in determining the time
	 * unless called from a timer expiry function in which case it will
	 * return the precise nanosecond resolution time of the underlying
	 * counter's timepoint at which the timer fired. Use pre-calculated
	 * timeouts from the context of a timer expiry function in combination
	 * with hardware scheduling of TX and RX for hard realtime guarantees.
	 *
	 * @param[in] api pointer to this api
	 * @param[out] uptime this interface's network reference clock uptime
	 *
	 * @retval 0 The retrieved timestamp was determined in active mode with
	 * high precision (as opposed to while suspended).
	 *
	 * @retval -EIO The high-precision timer is currently not available,
	 * probably because the counter is suspended. If a timestamp greater
	 * than zero is returned, this is a rough estimate of the current time
	 * with reduced accuracy.
	 */
	int (*get_time)(const struct net_time_reference_api *api, net_time_t *uptime);

	/**
	 * @brief Converts the given network uptime counter timepoint to the
	 * nearest syntonized network uptime value.
	 *
	 * @note Clients will usually not call this method directly. They'll
	 * program timers and scheduled TX/RX with well defined rounding
	 * semantics instead and the network driver or time reference
	 * implementation will take care of converting time to counter
	 * timepoints internally.
	 *
	 * @note Calling this before and after a call to syntonize() will
	 * usually produce different results. So be sure to always apply the
	 * latest syntonization data before calling this function.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] timepoint a pointer to the counter timeout to be converted
	 * @param[out] net_time Represents the network time corresponding to the
	 * counter timepoint under the current syntonization.
	 *
	 * @retval 0 if the given timeout was successfully converted.
	 */
	int (*get_time_from_timepoint)(const struct net_time_reference_api *api,
				       k_timepoint_t timepoint, net_time_t *net_time);

	/**
	 * @brief Converts the given syntonized network uptime value to an
	 * approximate absolute network uptime counter timepoint according to
	 * the given rounding mechanism.
	 *
	 * @note Clients will usually not call this method directly. They'll
	 * program timers and scheduled TX/RX with well defined rounding
	 * semantics instead and the network driver or time reference
	 * implementation will take care of converting time to counter
	 * timepoints internally.
	 *
	 * @note Calling this before and after a call to syntonize() will
	 * usually produce different results. So be sure to always apply the
	 * latest syntonization data before calling this function.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] net_time the network time to be converted
	 * @param[in] rounding the rounding type to be applied
	 * @param[out] timepoint a pointer to the counter timepoint
	 * corresponding to the rounded network time under the current
	 * syntonization
	 *
	 * @retval 0 if the given time was successfully converted.
	 */
	int (*get_timepoint_from_time)(const struct net_time_reference_api *api,
				       net_time_t net_time, enum net_time_rounding rounding,
				       k_timepoint_t *timepoint);

	/**
	 * @brief Multiplexed timer support on top of the network uptime
	 * reference. Uses get_timepoint_from_time() internally for well defined
	 * rounding semantics.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] timer pointer to an externally allocated timer struct
	 * @param[in] expire_at Set to a value greater than zero to program the
	 * initial timeout. This is always an absolute timeout. Relative
	 * timeouts are not supported. Call get_time() from a timer expiry
	 * function to retrieve the last precise timer expiry time and calculate
	 * offsets from this time for precise offsets.
	 * @param[in] period Set to a value greater than zero to program a
	 * periodic timer. Precise nanosecond precision expiry times will be
	 * calculated from the exact timepoint designated by expire_at such that
	 * timeouts do not accumulate rounding errors over time. Calculated
	 * expiry times will be rounded individually to a counter timepoint
	 * value. This may lead to jitter or multiple timeouts with the same
	 * actual expiry time if the given nanosecond value is not divisible by
	 * the underlying counter's tick period.
	 * @param[in] rounding the rounding type to be applied to expire_at
	 * and timestamps derived from the interval period
	 * @param[out] programmed_expiry (optional) Will be set to the rounded
	 * timestamp to which the timer was actually programmed or NULL if the
	 * timer could not be programmed. MAY be set to NULL if not required.
	 *
	 * @returns 0 if the timer was successfully scheduled. See @ref
	 * k_timer_start() for additional return values
	 */
	int (*timer_start)(const struct net_time_reference_api *api, struct net_time_timer *timer,
			   net_time_t expire_at, net_time_t period, enum net_time_rounding rounding,
			   net_time_t *programmed_expiry);

	/**
	 * @brief Cancel a running timer.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] timer pointer to the timer that should be canceled
	 *
	 * @returns 0 if the timer was successfully stopped. See @ref
	 * k_timer_stop() for additional return values
	 */
	int (*timer_stop)(const struct net_time_reference_api *api, struct net_time_timer *timer);

	/**
	 * @brief Provide an additional syntonization instant, i.e. a point in
	 * time with a known value of the underlying network uptime counter at a
	 * given network reference clock edge (i.e. counter timepoint).
	 *
	 * @details Syntonization MAY be based on an arbitrary number of
	 * syntonization instants. The exact behavior of the network reference
	 * clock given a certain sequence of syntonization instants is
	 * implementation specific and depends on the underlying syntonization
	 * algorithm.
	 *
	 * Syntonization SHALL NOT lead to discontinuities in the values
	 * returned by get_time(), i.e. get_time() SHALL be a monotonically
	 * increasing function over time surjectively mapping the net_time_t
	 * domain to the sets of all k_timepoint_t values even in the presence
	 * of dynamic syntonization events. Running timers SHALL be rescheduled
	 * after syntonization if the calculated timepoint values change due to
	 * additional syntonization data and rescheduling can be safely done
	 * without oversleeping the timer expiry (i.e. with appropriate guard
	 * times).
	 *
	 * @note Applying dynamic syntonization means that timepoints and time
	 * ranges represented in uptime counter values (k_timepoint_t,
	 * k_timeout) no longer represent constant measures of time. Instead,
	 * timepoints and timespans represented in net_time_t will be constant
	 * and precise measures of syntonized time. This means that arithmentic
	 * operations may be safely applied to syntonized time but not to
	 * k_timepoint_t instances, or durations/periods expressed as k_timeout.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] net_time network reference time
	 * @param[in] timepoint counter timepoint
	 */
	int (*syntonize)(const struct net_time_reference_api *api, net_time_t net_time,
			 k_timepoint_t timepoint);

	/**
	 * @brief Get the current worst case uncertainty (maximum ± deviation
	 * from the network reference clock) of the given target time as
	 * represented by the interface's syntonized network uptime counter.
	 *
	 * The value returned MAY be based on arbitrary statistical
	 * approximation models (e.g. linear or polynomial regression) derived
	 * from syntonization data. This MAY result in the uptime reference
	 * reporting better, even much better accuracy than the underlying
	 * counter's nominal accuracy and precision, as systematic frequency
	 * offset and frequency wander may have been compensated for based on
	 * empirical data.
	 *
	 * Uncertainty will usually be less the more recent the last
	 * syntonization instant. Therefore add a syntonization instant before
	 * calling this method for least uncertainty.
	 *
	 * Some residual uncertainty due to rounding errors and random jitter is
	 * unavoidable.
	 *
	 * Uncertainty is given in nanoseconds.
	 *
	 * @param[in] api pointer to this api
	 * @param[in] target_time absolute target time for which the uncertainty
	 * relative to the current time is to be calculated
	 * @param[out] uncertainty current uncertainty related to the given
	 * target time
	 */
	int (*get_uncertainty)(const struct net_time_reference_api *api, net_time_t target_time,
			       net_time_t *uncertainty);

	/**
	 * @brief A pointer to the uptime counter underlying this network uptime
	 * reference. The counter is loosely coupled as network uptime
	 * references can be freely associated with any independently defined
	 * network uptime counter.
	 */
	const struct net_time_counter_api *counter_api;
};

/**
 * @cond INTERNAL_HIDDEN
 */

void net_time_timer_expiry_fn(struct k_timer *timer);

#define Z_NET_TIME_TIMER_INITIALIZER(obj, expiry, stop)                                            \
	{                                                                                          \
		.timer = Z_TIMER_INITIALIZER(obj.timer, net_time_timer_expiry_fn, stop),           \
		.expiry_fn = expiry,                                                               \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Statically define and initialize a network uptime reference timer.
 *
 * The timer can be accessed outside the module where it is defined using:
 *
 * @code extern struct net_time_timer <name>; @endcode
 *
 * @param name Name of the timer variable.
 * @param expiry_fn Function to invoke each time the timer expires.
 * @param stop_fn   Function to invoke if the timer is stopped while running.
 */
#define K_NET_TIME_TIMER_DEFINE(name, expiry_fn, stop_fn)                                          \
	STRUCT_SECTION_ITERABLE(net_time_timer, name) =                                            \
		Z_NET_TIME_TIMER_INITIALIZER(name, expiry_fn, stop_fn)

/**
 * @brief Convert a network counter timepoint to a driver-internal
 * representation.
 *
 * @note This is meant for internal network driver usage and should not be
 * called directly.
 *
 * See @ref net_time_counter_api.get_tick_from_timepoint() for details.
 */
static inline void
net_time_counter_get_tick_from_timepoint(const struct net_time_reference_api *api,
					 k_timepoint_t timepoint, void *tick)
{
	const struct net_time_counter_api *counter_api = api->counter_api;

	return counter_api->get_tick_from_timepoint(counter_api, timepoint, tick);
}

/**
 * @brief Convert a driver-internal tick representation to a network counter
 * timepoint.
 *
 * @note This is meant for internal network driver usage and should not be
 * called directly.
 *
 * See @ref net_time_counter_api.get_timepoint_from_tick() for details.
 */
static inline void
net_time_counter_get_timepoint_from_tick(const struct net_time_reference_api *api, void *tick,
					 k_timepoint_t *timepoint)
{
	const struct net_time_counter_api *counter_api = api->counter_api;

	return counter_api->get_timepoint_from_tick(counter_api, tick, timepoint);
}

/**
 * @brief Leave network uptime counter sleep mode and enter "active" mode.
 *
 * See @ref net_time_counter_api.wake_up() for details.
 */
static inline int net_time_counter_wake_up(const struct net_time_reference_api *api)
{
	const struct net_time_counter_api *counter_api = api->counter_api;

	return counter_api->wake_up(counter_api);
}

/**
 * @brief Signal to the network uptime counter that it may suspend into
 * low-power mode.
 *
 * See @ref net_time_counter_api.may_sleep() for details.
 */
static inline int net_time_counter_may_sleep(const struct net_time_reference_api *api)
{
	const struct net_time_counter_api *counter_api = api->counter_api;

	return counter_api->may_sleep(counter_api);
}

/**
 * @brief Retrieve the current time from the network uptime reference.
 *
 * See @ref net_time_reference_api.get_time() for details.
 */
static inline int net_time_reference_get_time(const struct net_time_reference_api *api,
					      net_time_t *net_time)
{
	return api->get_time(api, net_time);
}

/**
 * @brief Set a timer relative to the network uptime reference.
 *
 * See @ref net_time_reference_api.timer_start() for details.
 */
static inline int net_time_reference_timer_start(const struct net_time_reference_api *api,
						 struct net_time_timer *timer, net_time_t expire_at,
						 net_time_t period, enum net_time_rounding rounding,
						 net_time_t *programmed_expiry)
{
	return api->timer_start(api, timer, expire_at, period, rounding, programmed_expiry);
}

/**
 * @brief Convert a counter timepoint to a corresponding syntonized network
 * uptime reference time.
 *
 * @note This is meant for internal network driver usage and should not be
 * called directly.
 *
 * See @ref net_time_reference_api.get_time_from_timepoint() for details.
 */
static inline int
net_time_reference_get_time_from_timepoint(const struct net_time_reference_api *api,
					   k_timepoint_t timepoint, net_time_t *net_time)
{
	return api->get_time_from_timepoint(api, timepoint, net_time);
}

/**
 * @brief Convert a syntonized network uptime reference time to a corresponding
 * counter timepoint.
 *
 * @note This is meant for internal network driver usage and should not be
 * called directly.
 *
 * See @ref net_time_reference_api.get_timepoint_from_time() for details.
 */
static inline int
net_time_reference_get_timepoint_from_time(const struct net_time_reference_api *api,
					   net_time_t net_time, enum net_time_rounding rounding,
					   k_timepoint_t *timepoint)
{
	return api->get_timepoint_from_time(api, net_time, rounding, timepoint);
}

/**
 * @brief Convenience function to convert a driver-internal tick representation
 * directly into a syntonized network uptime reference time.
 *
 * @note This is meant for internal network driver usage and should not be
 * called directly.
 *
 * See @ref net_time_reference_api.get_time_from_timepoint() and @ref
 * net_time_counter_api.get_timepoint_from_tick() for details.
 *
 * @param[in] api pointer to the net time reference API
 * @param[in] tick pointer to a variable that contains the driver-internal
 * representation of the tick to be converted
 * @param[out] net_time pointer to a variable that will receive the converted
 * syntonized network uptime
 *
 * @return 0 if the operation was successful, otherwise the error returned by
 * @ref net_time_reference_api.get_time_from_timepoint(), see there.
 */
static inline int net_time_reference_get_time_from_tick(const struct net_time_reference_api *api,
							void *tick, net_time_t *net_time)
{
	k_timepoint_t tp;

	net_time_counter_get_timepoint_from_tick(api, tick, &tp);
	return net_time_reference_get_time_from_timepoint(api, tp, net_time);
}

/**
 * @brief Convenience function to convert a syntonized network uptime reference
 * time directly into a driver-internal tick representation.
 *
 * @note This is meant for internal network driver usage and should not be
 * called directly.
 *
 * See @ref net_time_reference_api.get_timepoint_from_time() and @ref
 * net_time_counter_api.get_tick_from_timepoint() for details.
 *
 * @param[in] api pointer to the net time reference API
 * @param[in] net_time syntonized network uptime to be converted
 * @param[in] rounding rounding algorithm to be applied when converting from ns
 * resolution uptime to a tick-based counter timepoint
 * @param[out] tick pointer to a variable that receives the driver-internal
 * representation of the converted time
 *
 * @return 0 if the operation was successful, otherwise the error returned by
 * @ref net_time_reference_api.get_timepoint_from_time(), see there.
 */
static inline int net_time_reference_get_tick_from_time(const struct net_time_reference_api *api,
							net_time_t net_time,
							enum net_time_rounding rounding, void *tick)
{
	k_timepoint_t tp;
	int ret;

	ret = net_time_reference_get_timepoint_from_time(api, net_time, rounding, &tp);
	if (ret) {
		return ret;
	}

	net_time_counter_get_tick_from_timepoint(api, tp, tick);
	return 0;
}

/**
 * @brief Initialize the given network uptime reference and its underlying
 * uptime counter.
 *
 * See @ref net_time_reference_api.init() for details.
 */
static inline int net_time_reference_init(const struct net_time_reference_api *api,
					  struct net_if *iface)
{
	int ret;

	ret = api->counter_api->init(api->counter_api, iface);
	if (ret) {
		return ret;
	}

	return api->init(api, iface);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_TIME_H_ */
