/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_EVENT_DOMAIN_H_
#define ZEPHYR_INCLUDE_PM_EVENT_DOMAIN_H_

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/event_device.h>

/** @cond INTERNAL_HIDDEN */

struct pm_event_domain;
struct pm_event_domain_event;

struct pm_event_domain_event {
	const struct pm_event_domain *const event_domain;
	struct pm_event_device_event *event_device_events;
};

struct pm_event_domain {
	const struct pm_event_device *const *event_devices;
	const uint32_t *event_latencies_us;
	const uint8_t *event_device_states;
	uint8_t event_devices_size;
	uint8_t event_latencies_us_size;
	uint16_t event_device_states_size;
};

/** @endcond */

/**
 * @brief Floor PM event domain event latency in microseconds
 *
 * @param domain PM event domain
 * @param event_latency_us Maximum event latency in microseconds
 *
 * @retval Floored PM event domain event latency
 */
uint32_t pm_event_domain_floor_event_latency_us(const struct pm_event_domain *domain,
						uint32_t event_latency_us);

/**
 * @brief Get PM event domain latencies in microseconds
 *
 * @param domain PM event domain
 *
 * @retval PM event domain latencies in microseconds
 */
const uint32_t *pm_event_domain_get_event_latencies_us(const struct pm_event_domain *domain);

/**
 * @brief Get PM event domain latencies size in microseconds
 *
 * @param domain PM event domain
 *
 * @retval PM event domain latencies size in microseconds
 */
uint8_t pm_event_domain_get_event_latencies_us_size(const struct pm_event_domain *domain);

/**
 * @brief Schedule PM event domain event
 *
 * @param event PM event domain event
 * @param event_latency_us Maximum event latency while event is active in microseconds
 * @param event_uptime_ticks Uptime ticks at which latency must be in effect
 *
 * @note Event remains active until released or rescheduled
 *
 * @see pm_event_domain_reschedule_event()
 * @see pm_event_domain_release_event()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_event_domain_schedule_event(const struct pm_event_domain_event *event,
				       uint32_t event_latency_us,
				       int64_t event_uptime_ticks);

/**
 * @brief Reschedule PM event domain event
 *
 * @param event PM event domain event
 * @param event_latency_us Maximum event latency while event is active in microseconds
 * @param event_uptime_ticks Uptime ticks at which latency must be in effect
 *
 * @note Event remains active until released
 * @note Event must already be scheduled
 *
 * @see pm_event_domain_schedule_event()
 * @see pm_event_domain_release_event()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_event_domain_reschedule_event(const struct pm_event_domain_event *event,
					 uint32_t event_latency_us,
					 int64_t event_uptime_ticks);

/**
 * @brief Request max PM event domain event latency
 *
 * @param event PM event domain event
 * @param event_latency_us Maximum event latency while event is active in microseconds
 *
 * @see pm_event_domain_release_event()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_event_domain_request_event(const struct pm_event_domain_event *event,
				      uint32_t event_latency_us);

/**
 * @brief Rerequest max PM event domain event latency
 *
 * @param event PM event domain event
 * @param event_latency_us Maximum event latency while event is active in microseconds
 *
 * @see pm_event_domain_release_event()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_event_domain_rerequest_event(const struct pm_event_domain_event *event,
					uint32_t event_latency_us);

/**
 * @brief Release PM event
 *
 * @param event PM event to release
 *
 * @see pm_event_domain_schedule_event()
 * @see pm_event_domain_reschedule_event()
 * @see pm_event_domain_request_event()
 * @see pm_event_domain_rerequest_event()
 */
void pm_event_domain_release_event(const struct pm_event_domain_event *event);

/** @cond INTERNAL_HIDDEN */

#define Z_PM_EVENT_DOMAIN_DT_SYM(_node) \
	_CONCAT(__pm_event_domain_dep_ord_, DT_DEP_ORD(_node))

#define Z_PM_EVENT_DOMAIN_DT_SYMNAME(_node, _name) \
	_CONCAT_3(Z_PM_EVENT_DOMAIN_DT_SYM(_node), _, _name)

#define Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICES_SYM(_node) \
	Z_PM_EVENT_DOMAIN_DT_SYMNAME(_node, event_devices)

#define Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICES_LISTIFY(_idx, _node) \
	PM_EVENT_DEVICE_DT_GET(DT_EVENT_DOMAIN_EVENT_DEVICE_BY_IDX(_node, _idx))

#define Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICES_DEFINE(_node)					\
	static const struct pm_event_device *const						\
	Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICES_SYM(_node)						\
	[DT_EVENT_DOMAIN_EVENT_DEVICES_LEN(_node)] = {						\
		LISTIFY(									\
			DT_EVENT_DOMAIN_EVENT_DEVICES_LEN(_node),				\
			Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICES_LISTIFY,				\
			(,),									\
			_node									\
		)										\
	}

#define Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICE_STATES_SYM(_node) \
	Z_PM_EVENT_DOMAIN_DT_SYMNAME(_node, event_device_states)

#define Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICE_STATES_DEFINE(_node)					\
	static const uint8_t									\
	Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICE_STATES_SYM(_node)					\
	[DT_EVENT_DOMAIN_EVENT_DEVICE_STATES_LEN(_node)] =					\
		DT_EVENT_DOMAIN_EVENT_DEVICE_STATES(_node)

#define Z_PM_EVENT_DOMAIN_DT_EVENT_LATENCIES_US_SYM(_node) \
	Z_PM_EVENT_DOMAIN_DT_SYMNAME(_node, event_latencies_us)

#define Z_PM_EVENT_DOMAIN_DT_EVENT_LATENCIES_US_DEFINE(_node)					\
	static const uint32_t									\
	Z_PM_EVENT_DOMAIN_DT_EVENT_LATENCIES_US_SYM(_node)					\
	[DT_EVENT_DOMAIN_EVENT_LATENCIES_US_LEN(_node)] =					\
		DT_EVENT_DOMAIN_EVENT_LATENCIES_US(_node)

#define Z_PM_EVENT_DOMAIN_EVENT_EVENT_DEVICE_EVENTS_SYM(_name) \
	_CONCAT(_name, _event_device_events)

#define Z_PM_EVENT_DOMAIN_EVENT_EVENT_DEVICE_EVENTS_DEFINE(_name, _node)			\
	static struct pm_event_device_event							\
	Z_PM_EVENT_DOMAIN_EVENT_EVENT_DEVICE_EVENTS_SYM(_name)					\
	[DT_EVENT_DOMAIN_EVENT_DEVICES_LEN(_node)]

/** @endcond */

/** Define PM event domain from devicetree node */
#define PM_EVENT_DOMAIN_DT_DEFINE(_node)							\
	Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICES_DEFINE(_node);					\
	Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICE_STATES_DEFINE(_node);					\
	Z_PM_EVENT_DOMAIN_DT_EVENT_LATENCIES_US_DEFINE(_node);					\
	const struct pm_event_domain Z_PM_EVENT_DOMAIN_DT_SYM(_node) = {			\
		.event_devices = Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICES_SYM(_node),			\
		.event_latencies_us = Z_PM_EVENT_DOMAIN_DT_EVENT_LATENCIES_US_SYM(_node),	\
		.event_device_states = Z_PM_EVENT_DOMAIN_DT_EVENT_DEVICE_STATES_SYM(_node),	\
		.event_devices_size = DT_EVENT_DOMAIN_EVENT_DEVICES_LEN(_node),			\
		.event_device_states_size = DT_EVENT_DOMAIN_EVENT_DEVICE_STATES_LEN(_node),	\
		.event_latencies_us_size = DT_EVENT_DOMAIN_EVENT_LATENCIES_US_LEN(_node),	\
	};

/** Get PM event domain from devicetree node */
#define PM_EVENT_DOMAIN_DT_GET(_node) \
	&Z_PM_EVENT_DOMAIN_DT_SYM(_node)

/** Define PM event domain event from devicetree node */
#define PM_EVENT_DOMAIN_EVENT_DT_DEFINE(_name, _node)						\
	Z_PM_EVENT_DOMAIN_EVENT_EVENT_DEVICE_EVENTS_DEFINE(_name, _node);			\
	static struct pm_event_domain_event _name = {						\
		.event_domain = PM_EVENT_DOMAIN_DT_GET(_node),					\
		.event_device_events = Z_PM_EVENT_DOMAIN_EVENT_EVENT_DEVICE_EVENTS_SYM(_name),	\
	}

/** @cond INTERNAL_HIDDEN */

/** Declare PM event domain */
#define Z_PM_EVENT_DOMAIN_DT_DECLARE(_node) \
	extern const struct pm_event_domain Z_PM_EVENT_DOMAIN_DT_SYM(_node);

/** Declare PM event domain for each enabled node */
DT_FOREACH_STATUS_OKAY_NODE(Z_PM_EVENT_DOMAIN_DT_DECLARE)

/** @endcond */

#endif /* ZEPHYR_INCLUDE_PM_EVENT_DOMAIN_H_ */
