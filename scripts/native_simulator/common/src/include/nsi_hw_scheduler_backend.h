/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_HW_SCHEDULER_BACKEND_H
#define NSI_COMMON_SRC_NSI_HW_SCHEDULER_BACKEND_H

#include "nsi_hws_models_if.h"

#ifdef __APPLE__
extern struct nsi_hw_event_st __nsi_hw_events_start[];
extern struct nsi_hw_event_st __nsi_hw_events_end[];
extern unsigned int nsi_hws_backend_event_count;

#define nsi_hws_events __nsi_hw_events_start

void nsi_hws_backend_init(void);
#else
extern struct nsi_hw_event_st __nsi_hw_events_start[];
extern struct nsi_hw_event_st __nsi_hw_events_end[];

#define nsi_hws_events __nsi_hw_events_start
#define nsi_hws_backend_event_count \
	((unsigned int)(__nsi_hw_events_end - __nsi_hw_events_start))

static inline void nsi_hws_backend_init(void)
{
}
#endif

NSI_INLINE const struct nsi_hw_event_st *nsi_hws_get_event(unsigned int index)
{
	return &nsi_hws_events[index];
}

#endif /* NSI_COMMON_SRC_NSI_HW_SCHEDULER_BACKEND_H */
