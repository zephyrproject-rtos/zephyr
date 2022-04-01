/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <spinlock.h>
#include <sys/slist.h>
#include <app_event_manager/app_event_manager.h>
#include <logging/log.h>

#if defined(CONFIG_REBOOT)
#include <sys/reboot.h>
#endif

LOG_MODULE_REGISTER(app_event_manager, CONFIG_APP_EVENT_MANAGER_LOG_LEVEL);

static void event_processor_fn(struct k_work *work);

struct app_event_manager_event_display_bm _app_event_manager_event_display_bm;

static K_WORK_DEFINE(event_processor, event_processor_fn);
static sys_slist_t eventq = SYS_SLIST_STATIC_INIT(&eventq);
static struct k_spinlock lock;

static bool log_is_event_displayed(const struct event_type *et)
{
	size_t idx = et - _event_type_list_start;

	return atomic_test_bit(_app_event_manager_event_display_bm.flags, idx);
}

#if IS_ENABLED(CONFIG_APP_EVENT_MANAGER_USE_DEPRECATED_LOG_FUN)
static void log_event_using_buffer(const struct app_event_header *aeh,
				   const struct event_type *et)
{
	char log_buf[CONFIG_APP_EVENT_MANAGER_EVENT_LOG_BUF_LEN];

	int pos = et->log_event_func_dep(aeh, log_buf, sizeof(log_buf));

	if (pos < 0) {
		log_buf[0] = '\0';
	} else if (pos >= sizeof(log_buf)) {
		log_buf[sizeof(log_buf) - 2] = '~';
	}

	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_LOG_EVENT_TYPE)) {
		LOG_INF("e: %s %s", et->name, log_strdup(log_buf));
	} else {
		LOG_INF("%s", log_strdup(log_buf));
	}
}
#endif /*CONFIG_APP_EVENT_MANAGER_USE_DEPRECATED_LOG_FUN*/

static void log_event(const struct app_event_header *aeh)
{
	const struct event_type *et = aeh->type_id;

	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_SHOW_EVENTS) ||
	    !log_is_event_displayed(et)) {
		return;
	}


	if (et->log_event_func) {
		et->log_event_func(aeh);
	}
#ifdef CONFIG_APP_EVENT_MANAGER_USE_DEPRECATED_LOG_FUN
	else if (et->log_event_func_dep) {
		log_event_using_buffer(aeh, et);
	}
#endif

	else if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_LOG_EVENT_TYPE)) {
		LOG_INF("e: %s", et->name);
	}
}

static void log_event_progress(const struct event_type *et,
			       const struct event_listener *el)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_SHOW_EVENTS) ||
	    !IS_ENABLED(CONFIG_APP_EVENT_MANAGER_SHOW_EVENT_HANDLERS) ||
	    !log_is_event_displayed(et)) {
		return;
	}

	LOG_INF("|\tnotifying %s", el->name);
}

static void log_event_consumed(const struct event_type *et)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_SHOW_EVENTS) ||
	    !IS_ENABLED(CONFIG_APP_EVENT_MANAGER_SHOW_EVENT_HANDLERS) ||
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

	STRUCT_SECTION_FOREACH(event_type, et) {
		if (app_event_get_type_flag(et, APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE)) {
			size_t idx = et - _event_type_list_start;

			atomic_set_bit(_app_event_manager_event_display_bm.flags, idx);
		}
	}
}

void * __weak app_event_manager_alloc(size_t size)
{
	void *event = k_malloc(size);

	if (unlikely(!event)) {
		LOG_ERR("Application Event Manager OOM error\n");
		__ASSERT_NO_MSG(false);
		#if defined(CONFIG_REBOOT)
			sys_reboot(SYS_REBOOT_WARM);
		#else
			k_panic();
		#endif
		return NULL;
	}

	return event;
}

void __weak app_event_manager_free(void *addr)
{
	k_free(addr);
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
		struct app_event_header *aeh = CONTAINER_OF(node,
						       struct app_event_header,
						       node);

		APP_EVENT_ASSERT_ID(aeh->type_id);

		const struct event_type *et = aeh->type_id;

		if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PREPROCESS_HOOKS)) {
			STRUCT_SECTION_FOREACH(event_preprocess_hook, h) {
				h->hook(aeh);
			}
		}

		log_event(aeh);

		bool consumed = false;

		for (const struct event_subscriber *es = et->subs_start;
		     (es != et->subs_stop) && !consumed;
		     es++) {

			__ASSERT_NO_MSG(es != NULL);

			const struct event_listener *el = es->listener;

			__ASSERT_NO_MSG(el != NULL);
			__ASSERT_NO_MSG(el->notification != NULL);

			log_event_progress(et, el);

			consumed = el->notification(aeh);

			if (consumed) {
				log_event_consumed(et);
			}
		}

		if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_POSTPROCESS_HOOKS)) {
			STRUCT_SECTION_FOREACH(event_postprocess_hook, h) {
				h->hook(aeh);
			}
		}

		app_event_manager_free(aeh);
	}
}

void _event_submit(struct app_event_header *aeh)
{
	__ASSERT_NO_MSG(aeh);
	APP_EVENT_ASSERT_ID(aeh->type_id);

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_SUBMIT_HOOKS)) {
		STRUCT_SECTION_FOREACH(event_submit_hook, h) {
			h->hook(aeh);
		}
	}
	sys_slist_append(&eventq, &aeh->node);
	k_spin_unlock(&lock, key);

	k_work_submit(&event_processor);
}

int app_event_manager_init(void)
{
	int ret = 0;

	__ASSERT_NO_MSG(_event_type_list_end - _event_type_list_start <=
			CONFIG_APP_EVENT_MANAGER_MAX_EVENT_CNT);

	log_event_init();

	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_POSTINIT_HOOK)) {
		STRUCT_SECTION_FOREACH(app_event_manager_postinit_hook, h) {
			ret = h->hook();
			if (ret) {
				break;
			}
		}
	}

	return ret;
}
