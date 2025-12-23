/*
 * Copyright (c) 2025 Clunky Machines
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_LWM2M_SEND_SCHEDULER_H_
#define ZEPHYR_NET_LWM2M_SEND_SCHEDULER_H_

#include <stdbool.h>
#include <zephyr/net/lwm2m.h>

/**
 * @brief Register the send scheduler LwM2M objects and initialise state.
 *
 * This installs the Scheduler Control (10523) and Sampling Rules (10524)
 * objects and prepares bookkeeping needed by the cache filter helpers.
 *
 * @return 0 on success, or a negative errno if object registration fails.
 */
int lwm2m_send_sched_init(void);

/**
 * @brief Cache filter that enforces the configured scheduler rules.
 *
 * Hook this up via @ref lwm2m_set_cache_filter for each cached resource you
 * want the scheduler to control.
 *
 * @param path    LwM2M path of the cached resource.
 * @param element Candidate sample about to be cached.
 *
 * @return true to keep the sample in the cache, false to drop or defer it.
 */
bool lwm2m_send_sched_cache_filter(const struct lwm2m_obj_path *path,
				   const struct lwm2m_time_series_elem *element);

/**
 * @brief Handle a registration or registration-update completion event.
 *
 * Applications should call this when their LwM2M RD client reports a successful
 * registration or registration update so the send scheduler can apply its
 * flush-on-update policy.
 */
void lwm2m_send_sched_handle_registration_event(void);

#endif /* ZEPHYR_NET_LWM2M_SEND_SCHEDULER_H_ */
