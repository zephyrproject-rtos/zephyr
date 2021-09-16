/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <spinlock.h>
#include <sys/slist.h>
#include <event_manager/event_manager.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(event_manager, CONFIG_EVENT_MANAGER_LOG_LEVEL);


/* Event manager uses orphan sections. Below tag will allow linker to
 * find a section usable to put the orphan sections next to.
 */
const struct {} linker_tag __attribute__((__section__("event_manager"))) __used;


static void event_processor_fn(struct k_work *work);

struct event_manager_event_display_bm _event_manager_event_display_bm;

static K_WORK_DEFINE(event_processor, event_processor_fn);
static sys_slist_t eventq = SYS_SLIST_STATIC_INIT(&eventq);
static struct k_spinlock lock;


static bool log_is_event_displayed(const struct event_type *et)
{
	return atomic_test_bit(_event_manager_event_display_bm.flags, et - __start_event_types);
}

static void log_event(const struct event_header *eh)
{
	const struct event_type *et = eh->type_id;

	if (!IS_ENABLED(CONFIG_EVENT_MANAGER_SHOW_EVENTS) ||
	    !log_is_event_displayed(et)) {
		return;
	}

	if (et->log_event) {
		char log_buf[CONFIG_EVENT_MANAGER_EVENT_LOG_BUF_LEN];

		int pos = et->log_event(eh, log_buf, sizeof(log_buf));

		if (pos < 0) {
			log_buf[0] = '\0';
		} else if (pos >= sizeof(log_buf)) {
			BUILD_ASSERT(sizeof(log_buf) >= 2,
					 "Buffer invalid");
			log_buf[sizeof(log_buf) - 2] = '~';
		}

		if (IS_ENABLED(CONFIG_EVENT_MANAGER_LOG_EVENT_TYPE)) {
			LOG_INF("e: %s %s", et->name, log_strdup(log_buf));
		} else {
			LOG_INF("%s", log_strdup(log_buf));
		}

	} else if (IS_ENABLED(CONFIG_EVENT_MANAGER_LOG_EVENT_TYPE)) {
		LOG_INF("e: %s", et->name);
	}
}

static void log_event_progress(const struct event_type *et,
			       const struct event_listener *el)
{
	if (!IS_ENABLED(CONFIG_EVENT_MANAGER_SHOW_EVENTS) ||
	    !IS_ENABLED(CONFIG_EVENT_MANAGER_SHOW_EVENT_HANDLERS) ||
	    !log_is_event_displayed(et)) {
		return;
	}

	LOG_INF("|\tnotifying %s", el->name);
}

static void log_event_consumed(const struct event_type *et)
{
	if (!IS_ENABLED(CONFIG_EVENT_MANAGER_SHOW_EVENTS) ||
	    !IS_ENABLED(CONFIG_EVENT_MANAGER_SHOW_EVENT_HANDLERS) ||
	    !log_is_event_displayed(et)) {
		return;
	}

	LOG_INF("|\tevent consumed");
}

static void log_event_init(void)
{
	if (!IS_ENABLED(CONFIG_LOG)) {
		return;
	}

	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {
		if (et->init_log_enable) {
			atomic_set_bit(_event_manager_event_display_bm.flags,
					et - __start_event_types);
		}
	}
}

void __weak event_manager_trace_event_execution(const struct event_header *eh,
				  bool is_start)
{
}

void __weak event_manager_trace_event_submission(const struct event_header *eh,
				const void *trace_info)
{
}

void * __weak event_manager_alloc(size_t size)
{
	void *event = k_malloc(size);

	if (unlikely(!event)) {
		printk("Event Manager OOM error\n");
		LOG_PANIC();
		__ASSERT_NO_MSG(false);
		k_panic();
		return NULL;
	}

	return event;
}

void __weak event_manager_free(void *addr)
{
	k_free(addr);
}

int __weak event_manager_trace_event_init(void)
{
	return 0;
}

static void event_processor_fn(struct k_work *work)
{
	sys_slist_t events = SYS_SLIST_STATIC_INIT(&events);

	/* Make current event list local. */
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (sys_slist_is_empty(&eventq)) {
		k_spin_unlock(&lock, key);
		return;
	}

	sys_slist_merge_slist(&events, &eventq);

	k_spin_unlock(&lock, key);


	/* Traverse the list of events. */
	sys_snode_t *node;

	while (NULL != (node = sys_slist_get(&events))) {
		struct event_header *eh = CONTAINER_OF(node,
						       struct event_header,
						       node);

		ASSERT_EVENT_ID(eh->type_id);

		const struct event_type *et = eh->type_id;

		event_manager_trace_event_execution(eh, true);

		log_event(eh);

		bool consumed = false;

		for (size_t prio = SUBS_PRIO_MIN;
		     (prio <= SUBS_PRIO_MAX) && !consumed;
		     prio++) {
			for (const struct event_subscriber *es =
					et->subs_start[prio];
			     (es != et->subs_stop[prio]) && !consumed;
			     es++) {

				__ASSERT_NO_MSG(es != NULL);

				const struct event_listener *el = es->listener;

				__ASSERT_NO_MSG(el != NULL);
				__ASSERT_NO_MSG(el->notification != NULL);

				log_event_progress(et, el);

				consumed = el->notification(eh);

				if (consumed) {
					log_event_consumed(et);
				}
			}
		}

		event_manager_trace_event_execution(eh, false);

		event_manager_free(eh);
	}
}

void _event_submit(struct event_header *eh)
{
	__ASSERT_NO_MSG(eh);
	ASSERT_EVENT_ID(eh->type_id);

	event_manager_trace_event_submission(eh, eh->type_id->trace_data);

	k_spinlock_key_t key = k_spin_lock(&lock);

	sys_slist_append(&eventq, &eh->node);
	k_spin_unlock(&lock, key);

	k_work_submit(&event_processor);
}

int event_manager_init(void)
{
	__ASSERT_NO_MSG(__stop_event_types - __start_event_types <=
			CONFIG_EVENT_MANAGER_MAX_EVENT_CNT);

	log_event_init();

	return event_manager_trace_event_init();
}
