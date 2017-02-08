/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_MGMT_EVENT)
#define SYS_LOG_DOMAIN "net/mgmt"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <toolchain.h>
#include <sections.h>

#include <misc/util.h>
#include <misc/slist.h>
#include <net/net_mgmt.h>

struct mgmt_event_entry {
	uint32_t event;
	struct net_if *iface;
};

static struct k_sem network_event;
NET_STACK_DEFINE(MGMT, mgmt_stack, CONFIG_NET_MGMT_EVENT_STACK_SIZE,
		 CONFIG_NET_MGMT_EVENT_STACK_SIZE);
static struct mgmt_event_entry events[CONFIG_NET_MGMT_EVENT_QUEUE_SIZE];
static uint32_t global_event_mask;
static sys_slist_t event_callbacks;
static uint16_t in_event;
static uint16_t out_event;

static inline void mgmt_push_event(uint32_t mgmt_event, struct net_if *iface)
{
	events[in_event].event = mgmt_event;
	events[in_event].iface = iface;

	in_event++;

	if (in_event == CONFIG_NET_MGMT_EVENT_QUEUE_SIZE) {
		in_event = 0;
	}

	if (in_event == out_event) {
		uint16_t o_idx = out_event + 1;

		if (o_idx == CONFIG_NET_MGMT_EVENT_QUEUE_SIZE) {
			o_idx = 0;
		}

		if (events[o_idx].event) {
			out_event = o_idx;
		}
	}
}

static inline struct mgmt_event_entry *mgmt_pop_event(void)
{
	uint16_t o_idx;

	if (!events[out_event].event) {
		return NULL;
	}

	o_idx = out_event;
	out_event++;

	if (out_event == CONFIG_NET_MGMT_EVENT_QUEUE_SIZE) {
		out_event = 0;
	}

	return &events[o_idx];
}

static inline void mgmt_clean_event(struct mgmt_event_entry *mgmt_event)
{
	mgmt_event->event = 0;
	mgmt_event->iface = NULL;
}

static inline void mgmt_add_event_mask(uint32_t event_mask)
{
	global_event_mask |= event_mask;
}

static inline void mgmt_rebuild_global_event_mask(void)
{
	struct net_mgmt_event_callback *cb, *tmp;

	global_event_mask = 0;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&event_callbacks, cb, tmp, node) {
		mgmt_add_event_mask(cb->event_mask);
	}
}

static inline bool mgmt_is_event_handled(uint32_t mgmt_event)
{
	return ((mgmt_event & global_event_mask) == mgmt_event);
}

static inline void mgmt_run_callbacks(struct mgmt_event_entry *mgmt_event)
{
	struct net_mgmt_event_callback *cb, *tmp;

	NET_DBG("Event layer %u code %u type %u",
		NET_MGMT_GET_LAYER(mgmt_event->event),
		NET_MGMT_GET_LAYER_CODE(mgmt_event->event),
		NET_MGMT_GET_COMMAND(mgmt_event->event));

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&event_callbacks, cb, tmp, node) {
		NET_DBG("Running callback %p : %p", cb, cb->handler);

		if ((mgmt_event->event & cb->event_mask) == mgmt_event->event) {
			cb->handler(cb, mgmt_event->event, mgmt_event->iface);
		}

#ifdef CONFIG_NET_DEBUG_MGMT_EVENT_STACK
			net_analyze_stack("Net MGMT event stack",
					  mgmt_stack,
					  CONFIG_NET_MGMT_EVENT_STACK_SIZE);
#endif
	}
}

static void mgmt_thread(void)
{
	struct mgmt_event_entry *mgmt_event;

	while (1) {
		k_sem_take(&network_event, K_FOREVER);

		NET_DBG("Handling events, forwarding it relevantly");

		mgmt_event = mgmt_pop_event();
		if (!mgmt_event) {
			/* System is over-loaded?
			 * At this point we have most probably notified
			 * more events than we could handle
			 */
			NET_DBG("Some event got probably lost (%u)",
				k_sem_count_get(&network_event));

			k_sem_init(&network_event, 0, UINT_MAX);

			continue;
		}

		mgmt_run_callbacks(mgmt_event);

		mgmt_clean_event(mgmt_event);

		k_yield();
	}
}

void net_mgmt_add_event_callback(struct net_mgmt_event_callback *cb)
{
	NET_DBG("Adding event callback %p", cb);

	sys_slist_prepend(&event_callbacks, &cb->node);

	mgmt_add_event_mask(cb->event_mask);
}

void net_mgmt_del_event_callback(struct net_mgmt_event_callback *cb)
{
	NET_DBG("Deleting event callback %p", cb);

	sys_slist_find_and_remove(&event_callbacks, &cb->node);

	mgmt_rebuild_global_event_mask();
}

void net_mgmt_event_notify(uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_is_event_handled(mgmt_event)) {
		NET_DBG("Notifying Event layer %u code %u type %u",
			NET_MGMT_GET_LAYER(mgmt_event),
			NET_MGMT_GET_LAYER_CODE(mgmt_event),
			NET_MGMT_GET_COMMAND(mgmt_event));

		mgmt_push_event(mgmt_event, iface);
		k_sem_give(&network_event);
	}
}

void net_mgmt_event_init(void)
{
	sys_slist_init(&event_callbacks);
	global_event_mask = 0;

	in_event = 0;
	out_event = 0;

	k_sem_init(&network_event, 0, UINT_MAX);

	memset(events, 0,
	       CONFIG_NET_MGMT_EVENT_QUEUE_SIZE *
	       sizeof(struct mgmt_event_entry));

	k_thread_spawn(mgmt_stack, sizeof(mgmt_stack),
		       (k_thread_entry_t)mgmt_thread, NULL, NULL, NULL,
		       K_PRIO_COOP(CONFIG_NET_MGMT_EVENT_THREAD_PRIO), 0, 0);

	NET_DBG("Net MGMT initialized: queue of %u entries, stack size of %u",
		CONFIG_NET_MGMT_EVENT_QUEUE_SIZE,
		CONFIG_NET_MGMT_EVENT_STACK_SIZE);
}
