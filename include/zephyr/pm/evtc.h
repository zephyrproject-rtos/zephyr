/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file uses the following short names:
 *
 *   uptime ticks     -> uptks
 *   ticks            -> tks
 *   latency          -> lat
 *   latencies        -> lats
 *   event            -> evt
 *   event controller -> evtc
 *   activate         -> act
 *   request          -> req
 *   runtime          -> rt
 *   update           -> upd
 *   timeout          -> to
 *   effective        -> eff
 *
 * This file uses the following terms:
 *
 *   event is active    -> event's max latency is accounted for
 *   event is effective -> event's max latency is in effect
 */

#ifndef ZEPHYR_INCLUDE_PM_EVTC_H_
#define ZEPHYR_INCLUDE_PM_EVTC_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

struct pm_evt;
struct pm_evtc;

struct pm_evt {
	sys_snode_t node;
	const struct pm_evtc *evtc;
	uint32_t lat_ns;
	int64_t uptks;
};

typedef void (*pm_evtc_request)(const struct device *dev, uint32_t latency_ns);

struct pm_evtc_rt {
	const struct pm_evtc *const evtc;
	sys_slist_t evt_list;
	struct k_spinlock lock;
	uint32_t req_lat_ns;
	int64_t req_uptks;
	struct _timeout upd_to;
};

struct pm_evtc {
	const struct device *dev;
	pm_evtc_request req;
	const uint32_t *lats_ns;
	uint8_t lats_ns_size;
	uint32_t req_lat_tks;
	struct pm_evtc_rt *rt;
};

/**
 * @brief Initialize PM event controller
 *
 * @param evtc PM event controller
 */
void pm_evtc_init(const struct pm_evtc *evtc);

/**
 * @brief Schedule PM event
 *
 * @param evtc PM event controller
 * @param evt PM event
 * @param max_latency_ns Max latency while event is active in nanoseconds
 * @param uptime_ticks Uptime ticks at which latency must be in effect
 *
 * @note Request remains active until released or rescheduled
 *
 * @see pm_evtc_reschedule_evt()
 * @see pm_evtc_release_evt()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_evtc_schedule_evt(const struct pm_evtc *evtc,
			     struct pm_evt *evt,
			     uint32_t max_latency_ns,
			     int64_t uptime_ticks);

/**
 * @brief Reschedule PM event
 *
 * @param evt PM event
 * @param uptime_ticks Uptime ticks at which latency must be in effect
 *
 * @note Request remains active until released
 * @note Request must already be scheduled
 *
 * @see pm_evtc_schedule_evt()
 * @see pm_evtc_release_evt()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_evtc_reschedule_evt(struct pm_evt *evt, int64_t uptime_ticks);

/**
 * @brief Request PM event latency
 *
 * @param evtc PM event controller
 * @param evt PM event
 * @param max_latency_ns Max latency until released in nanoseconds
 *
 * @note A max_latency_ns of 0 will request lowest supported latency
 *
 * @see pm_evtc_release_evt()
 *
 * @retval Uptime ticks at which latency will be in effect
 */
int64_t pm_evtc_request_evt(const struct pm_evtc *evtc,
			    struct pm_evt *evt,
			    uint32_t max_latency_ns);

/**
 * @brief Release PM event
 *
 * @param evt PM event to release
 *
 * @see pm_evtc_schedule_evt()
 * @see pm_evtc_reschedule_evt()
 * @see pm_evtc_request_evt()
 */
void pm_evtc_release_evt(struct pm_evt *evt);

#define PM_EVTC_DT_SYM(_node) \
	_CONCAT(__pm_evtc_dep_ord_, DT_DEP_ORD(_node))

#define PM_EVTC_DT_SYMNAME(_node, _name) \
	_CONCAT_3(PM_EVTC_DT_SYM(_node), _, _name)

#define PM_EVTC_DT_LATS_NS_SYM(_node) \
	PM_EVTC_DT_SYMNAME(_node, _lats_ns)

#define PM_EVTC_DT_LATS_NS_DEFINE(_node) \
	const uint32_t PM_EVTC_DT_LATS_NS_SYM(_node)[] = DT_PROP(_node, evtc_latencies_ns)

#define PM_EVTC_DT_LATS_NS_GET(_node) \
	PM_EVTC_DT_LATS_NS_SYM(_node)

#define PM_EVTC_DT_LATS_NS_SIZE(_node) \
	ARRAY_SIZE(PM_EVTC_DT_LATS_NS_GET(_node))

#define PM_EVTC_DT_LAT_NS_BY_IDX(_node, _idx) \
	DT_PROP_BY_IDX(_node, evtc_latencies_ns, _idx)

#define PM_EVTC_DT_LAT_NS_MAX(_node) \
	PM_EVTC_DT_LAT_NS_BY_IDX(_node, 0)

#define PM_EVTC_DT_LAT_NS_MIN(_node)								\
	DT_PROP_BY_IDX(										\
		_node,										\
		evtc_latencies_ns,								\
		UTIL_DEC(DT_PROP_LEN(_node, evtc_latencies_ns))					\
	)

#define PM_EVTC_DT_RT_SYM(_node) \
	PM_EVTC_DT_SYMNAME(_node, _rt)

#define PM_EVTC_DT_RT_DEFINE(_node)								\
	struct pm_evtc_rt PM_EVTC_DT_RT_SYM(_node) = {						\
		.evtc = &PM_EVTC_DT_SYM(_node),							\
		.req_uptks = -1,								\
	}

#define PM_EVTC_DT_RT_GET(_node) \
	&PM_EVTC_DT_RT_SYM(_node)

#define PM_EVTC_DT_REQ_LAT_NS(_node) \
	DT_PROP(_node, evtc_request_latency_ns)

#define PM_EVTC_DT_REQ_LAT_TICKS(_node) \
	k_ns_to_ticks_ceil32(PM_EVTC_DT_REQ_LAT_NS(_node))

/**
 * @brief Define PM event controller from devicetree node
 *
 * @param _node Devicetree node
 * @param _request Request API implementation
 */
#define PM_EVTC_DT_DEFINE(_node, _request)							\
	PM_EVTC_DT_LATS_NS_DEFINE(_node);							\
	PM_EVTC_DT_RT_DEFINE(_node);								\
	const struct pm_evtc PM_EVTC_DT_SYM(_node) = {						\
		.req = _request,								\
		.dev = DEVICE_DT_GET(_node),							\
		.req_lat_tks = PM_EVTC_DT_REQ_LAT_TICKS(_node),					\
		.lats_ns = PM_EVTC_DT_LATS_NS_GET(_node),					\
		.lats_ns_size = PM_EVTC_DT_LATS_NS_SIZE(_node),					\
		.rt = PM_EVTC_DT_RT_GET(_node),							\
	}

#define PM_EVTC_DT_GET(_node) \
	&PM_EVTC_DT_SYM(_node)

#define PM_EVTC_DT_INST_LATS_NS_SIZE(_inst) \
	PM_EVTC_DT_LATS_NS_SIZE(DT_DRV_INST(_inst))

#define PM_EVTC_DT_INST_LAT_NS_BY_IDX(_inst, _idx) \
	PM_EVTC_DT_LAT_NS_BY_IDX(DT_DRV_INST(_inst), _idx)

#define PM_EVTC_DT_INST_LAT_NS_MAX(_inst) \
	PM_EVTC_DT_LAT_NS_MAX(DT_DRV_INST(_inst))

#define PM_EVTC_DT_INST_LAT_NS_MIN(_inst) \
	PM_EVTC_DT_LAT_NS_MAX(DT_DRV_INST(_inst))

#define PM_EVTC_DT_INST_DEFINE(_inst, _request) \
	PM_EVTC_DT_DEFINE(DT_DRV_INST(_inst), _request)

#define PM_EVTC_DT_INST_GET(_inst) \
	PM_EVTC_DT_GET(DT_DRV_INST(_inst))

#define PM_EVTC_DT_DECLARE(_node) \
	extern const struct pm_evtc PM_EVTC_DT_SYM(_node);

/** Declare PM event controller for each enabled node */
DT_FOREACH_STATUS_OKAY_NODE(PM_EVTC_DT_DECLARE)

#endif /* ZEPHYR_INCLUDE_PM_EVTC_H_ */
