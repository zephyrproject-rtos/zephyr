/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_EVENT_DEVICE_H_
#define ZEPHYR_INCLUDE_PM_EVENT_DEVICE_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

/** @cond INTERNAL_HIDDEN */

struct pm_event_device;
struct pm_event_device_event;

struct pm_event_device_event {
	sys_snode_t node;
	const struct pm_event_device *event_device;
	uint8_t event_state;
	int64_t uptime_ticks;
};

typedef void (*pm_event_device_event_state_request)(const struct device *dev, uint8_t event_state);

struct pm_event_device_runtime {
	const struct pm_event_device *const event_device;
	sys_slist_t event_list;
	struct k_spinlock lock;
	uint32_t requested_event_state;
	int64_t request_uptime_ticks;
	struct _timeout update_timeout;
};

#if defined(CONFIG_PM_EVENT_DEVICE)

struct pm_event_device {
	const struct device *dev;
	pm_event_device_event_state_request event_state_request;
	uint8_t event_state_count;
	uint32_t event_state_request_latency_ticks;
	struct pm_event_device_runtime *runtime;
};

#else

struct pm_event_device {
	const struct device *dev;
	pm_event_device_event_state_request event_state_request;
	uint8_t event_state_count;
};

#endif

/** @endcond */

/**
 * @brief Get PM event device device
 *
 * @param event_device PM event device
 *
 * @retval PM event device device
 */
const struct device *pm_event_device_get_dev(const struct pm_event_device *event_device);

/**
 * @brief Get number of PM event device event states
 *
 * @param event_device PM event device
 *
 * @retval Number of PM event device event states
 */
uint8_t pm_event_device_get_event_state_count(const struct pm_event_device *event_device);

/**
 * @brief Get max PM event device event state
 *
 * @param event_device PM event device
 *
 * @retval Max PM event device event state
 */
uint8_t pm_event_device_get_max_event_state(const struct pm_event_device *event_device);

/**
 * @brief Initialize PM event device
 *
 * @param event_device PM event device
 */
#if defined(CONFIG_PM_EVENT_DEVICE)

void pm_event_device_init(const struct pm_event_device *event_device);

#else

static inline void pm_event_device_init(const struct pm_event_device *event_device)
{
	event_device->event_state_request(event_device->dev, event_device->event_state_count - 1);
}

#endif

/**
 * @brief Schedule PM event
 *
 * @param event_device PM event device
 * @param event PM event device event
 * @param event_state Minimum event state while event is active
 * @param uptime_ticks Uptime ticks at which latency must be in effect
 *
 * @note Request remains active until released or rescheduled
 *
 * @see pm_event_device_reschedule_event()
 * @see pm_event_device_release_event()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_event_device_schedule_event(const struct pm_event_device *event_device,
				       struct pm_event_device_event *event,
				       uint8_t event_state,
				       int64_t uptime_ticks);

/**
 * @brief Reschedule PM event
 *
 * @param event PM event device event
 * @param event_state Minimum event state while event is active
 * @param uptime_ticks Uptime ticks at which latency must be in effect
 *
 * @note Request remains active until released
 * @note Request must already be scheduled
 *
 * @see pm_event_device_schedule_event()
 * @see pm_event_device_release_event()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_event_device_reschedule_event(struct pm_event_device_event *event,
					 uint8_t event_state,
					 int64_t uptime_ticks);

/**
 * @brief Request PM event device event state
 *
 * @param event_device PM event device
 * @param event PM event device event
 * @param event_state Minimum event state while event is active
 *
 * @see pm_event_device_release_event()
 *
 * @retval Uptime ticks at which event device event state will be in effect
 */
int64_t pm_event_device_request_event(const struct pm_event_device *event_device,
				      struct pm_event_device_event *event,
				      uint8_t event_state);

/**
 * @brief Rerequest PM event device state
 *
 * @param event PM event device event
 * @param event_state Minimum event device event state while event is active
 *
 * @note Request remains active until released
 * @note Request must already be requested
 *
 * @see pm_event_device_request_event()
 * @see pm_event_device_release_event()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_event_device_rerequest_event(struct pm_event_device_event *event,
					uint8_t event_state);

/**
 * @brief Release PM event device event
 *
 * @param event PM event to release
 *
 * @see pm_event_device_schedule_event()
 * @see pm_event_device_reschedule_event()
 * @see pm_event_device_request_event()
 * @see pm_event_device_rerequest_event()
 */
void pm_event_device_release_event(struct pm_event_device_event *event);

/** @cond INTERNAL_HIDDEN */

#define Z_PM_EVENT_DEVICE_DT_SYM(_node) \
	_CONCAT(__pm_event_device_dep_ord_, DT_DEP_ORD(_node))

#define Z_PM_EVENT_DEVICE_DT_SYMNAME(_node, _name) \
	_CONCAT_3(Z_PM_EVENT_DEVICE_DT_SYM(_node), _, _name)

#define Z_PM_EVENT_DEVICE_DT_RUNTIME_SYM(_node) \
	Z_PM_EVENT_DEVICE_DT_SYMNAME(_node, _runtime)

#define Z_PM_EVENT_DEVICE_DT_RUNTIME_DEFINE(_node)						\
	static struct pm_event_device_runtime Z_PM_EVENT_DEVICE_DT_RUNTIME_SYM(_node) = {	\
		.event_device = &Z_PM_EVENT_DEVICE_DT_SYM(_node),				\
		.request_uptime_ticks = -1,							\
	}

#define Z_PM_EVENT_DEVICE_DT_RUNTIME_GET(_node) \
	&Z_PM_EVENT_DEVICE_DT_RUNTIME_SYM(_node)

/** @endcond */

/**
 * @brief Define PM event device from devicetree node
 *
 * @param _node Devicetree node
 * @param _event_state_request Event state request API implementation
 * @param _event_state_request_latency_us Event state request latency in microseconds
 * @param _event_state_count Number of supported event states.
 */
#if defined(CONFIG_PM_EVENT_DEVICE)

#define PM_EVENT_DEVICE_DT_DEFINE(_node,							\
				  _event_state_request,						\
				  _event_state_request_latency_us,				\
				  _event_state_count)						\
	Z_PM_EVENT_DEVICE_DT_RUNTIME_DEFINE(_node);						\
	const struct pm_event_device Z_PM_EVENT_DEVICE_DT_SYM(_node) = {			\
		.dev = DEVICE_DT_GET(_node),							\
		.event_state_request = _event_state_request,					\
		.event_state_request_latency_ticks =						\
			k_us_to_ticks_ceil32(_event_state_request_latency_us),			\
		.event_state_count = _event_state_count,					\
		.runtime = Z_PM_EVENT_DEVICE_DT_RUNTIME_GET(_node),				\
	}

#else

#define PM_EVENT_DEVICE_DT_DEFINE(_node,							\
				  _event_state_request,						\
				  _event_state_request_latency_us,				\
				  _event_state_count)						\
	const struct pm_event_device Z_PM_EVENT_DEVICE_DT_SYM(_node) = {			\
		.dev = DEVICE_DT_GET(_node),							\
		.event_state_request = _event_state_request,					\
		.event_state_count = _event_state_count,					\
	}

#endif

/** Get PM event device from devicetree node */
#define PM_EVENT_DEVICE_DT_GET(_node) \
	&Z_PM_EVENT_DEVICE_DT_SYM(_node)

/**
 * @brief Device driver instance variants of PM_EVENT_DEVICE_ macros
 * @{
 */

#define PM_EVENT_DEVICE_DT_INST_DEFINE(_inst,							\
				       _event_state_request,					\
				       _event_state_request_latency_us,				\
				       _event_state_count)					\
	PM_EVENT_DEVICE_DT_DEFINE(DT_DRV_INST(_inst),						\
				  _event_state_request,						\
				  _event_state_request_latency_us,				\
				  _event_state_count)

#define PM_EVENT_DEVICE_DT_INST_GET(_inst) \
	PM_EVENT_DEVICE_DT_GET(DT_DRV_INST(_inst))

/** @} */

/** @cond INTERNAL_HIDDEN */

/** Declare PM event device */
#define Z_PM_EVENT_DEVICE_DT_DECLARE(_node) \
	extern const struct pm_event_device Z_PM_EVENT_DEVICE_DT_SYM(_node);

/** Declare PM event device for each enabled node */
DT_FOREACH_STATUS_OKAY_NODE(Z_PM_EVENT_DEVICE_DT_DECLARE)

/** @endcond */

#endif /* ZEPHYR_INCLUDE_PM_EVENT_DEVICE_H_ */
