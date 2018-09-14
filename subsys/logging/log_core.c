/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log_msg.h>
#include "log_list.h"
#include <logging/log.h>
#include <logging/log_backend.h>
#include <logging/log_ctrl.h>
#include <logging/log_output.h>
#include <misc/printk.h>
#include <assert.h>
#include <atomic.h>

#ifndef CONFIG_LOG_PRINTK_MAX_STRING_LENGTH
#define CONFIG_LOG_PRINTK_MAX_STRING_LENGTH 1
#endif

static struct log_list_t list;
static atomic_t initialized;
static bool panic_mode;
static atomic_t buffered_cnt;
static k_tid_t proc_tid;

static u32_t dummy_timestamp(void);
static timestamp_get_t timestamp_func = dummy_timestamp;

static u32_t dummy_timestamp(void)
{
	return 0;
}

static inline void msg_finalize(struct log_msg *msg,
				struct log_msg_ids src_level)
{
	unsigned int key;

	msg->hdr.ids = src_level;
	msg->hdr.timestamp = timestamp_func();

	atomic_inc(&buffered_cnt);

	key = irq_lock();

	log_list_add_tail(&list, msg);

	irq_unlock(key);

	if (IS_ENABLED(CONFIG_LOG_INPLACE_PROCESS) || panic_mode) {
		(void)log_process(false);
	} else if (!IS_ENABLED(CONFIG_LOG_INPLACE_PROCESS) &&
		   CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) {
		if (buffered_cnt == CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD &&
		    proc_tid) {
			k_wakeup(proc_tid);
		}
	}
}

void log_0(const char *str, struct log_msg_ids src_level)
{
	struct log_msg *msg = log_msg_create_0(str);

	if (msg == NULL) {
		return;
	}
	msg_finalize(msg, src_level);
}

void log_1(const char *str,
	   u32_t arg0,
	   struct log_msg_ids src_level)
{
	struct log_msg *msg = log_msg_create_1(str, arg0);

	if (msg == NULL) {
		return;
	}
	msg_finalize(msg, src_level);
}

void log_2(const char *str,
	   u32_t arg0,
	   u32_t arg1,
	   struct log_msg_ids src_level)
{
	struct log_msg *msg = log_msg_create_2(str, arg0, arg1);

	if (msg == NULL) {
		return;
	}

	msg_finalize(msg, src_level);
}

void log_3(const char *str,
	   u32_t arg0,
	   u32_t arg1,
	   u32_t arg2,
	   struct log_msg_ids src_level)
{
	struct log_msg *msg = log_msg_create_3(str, arg0, arg1, arg2);

	if (msg == NULL) {
		return;
	}

	msg_finalize(msg, src_level);
}

void log_n(const char *str,
	   u32_t *args,
	   u32_t narg,
	   struct log_msg_ids src_level)
{
	struct log_msg *msg = log_msg_create_n(str, args, narg);

	if (msg == NULL) {
		return;
	}

	msg_finalize(msg, src_level);
}

void log_hexdump(const char *str,
		 const u8_t *data,
		 u32_t length,
		 struct log_msg_ids src_level)
{
	struct log_msg *msg = log_msg_hexdump_create(str, data, length);

	if (msg == NULL) {
		return;
	}

	msg_finalize(msg, src_level);
}

int log_printk(const char *fmt, va_list ap)
{
	if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
		u8_t formatted_str[CONFIG_LOG_PRINTK_MAX_STRING_LENGTH];
		struct log_msg_ids empty_id = { 0 };
		struct log_msg *msg;
		int length;

		length = vsnprintk(formatted_str,
				   sizeof(formatted_str), fmt, ap);

		length = (length > sizeof(formatted_str)) ?
			 sizeof(formatted_str) : length;

		msg = log_msg_hexdump_create(NULL, formatted_str, length);
		if (!msg) {
			return 0;
		}

		msg->hdr.params.hexdump.raw_string = 1;
		msg_finalize(msg, empty_id);

		return length;
	} else {
		return 0;
	}
}

void log_generic(struct log_msg_ids src_level, const char *fmt, va_list ap)
{
	u32_t args[LOG_MAX_NARGS];

	for (int i = 0; i < LOG_MAX_NARGS; i++) {
		args[i] = va_arg(ap, u32_t);
	}

	/* Assume maximum amount of parameters. Determining exact number would
	 * require string analysis.
	 */
	log_n(fmt, args, LOG_MAX_NARGS, src_level);
}

static u32_t timestamp_get(void)
{
	return k_cycle_get_32();
}

void log_core_init(void)
{
	log_msg_pool_init();
	log_list_init(&list);

	/*
	 * Initialize aggregated runtime filter levels (no backends are
	 * attached yet, so leave backend slots in each dynamic filter set
	 * alone for now).
	 *
	 * Each log source's aggregated runtime level is set to match its
	 * compile-time level. When backends are attached later on in
	 * log_init(), they'll be initialized to the same value.
	 */
	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		for (int i = 0; i < log_sources_count(); i++) {
			u32_t *filters = log_dynamic_filters_get(i);
			u8_t level = log_compiled_level_get(i);

			LOG_FILTER_SLOT_SET(filters,
					    LOG_FILTER_AGGR_SLOT_IDX,
					    level);
		}
	}
}

/*
 * Initialize a backend's runtime filters to match the compile-time
 * settings.
 *
 * (Aggregated filters were already set up in log_core_init().
 */
static void backend_filter_init(struct log_backend const *const backend)
{
	u8_t level;
	int i;

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		for (i = 0; i < log_sources_count(); i++) {
			level = log_compiled_level_get(i);

			log_filter_set(backend,
				       CONFIG_LOG_DOMAIN_ID,
				       i,
				       level);
		}
	}
}

void log_init(void)
{
	assert(log_backend_count_get() < LOG_FILTERS_NUM_OF_SLOTS);
	int i;

	if (atomic_inc(&initialized)) {
		return;
	}

	/* Set default timestamp. */
	timestamp_func = timestamp_get;
	log_output_timestamp_freq_set(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* Assign ids to backends. */
	for (i = 0; i < log_backend_count_get(); i++) {
		const struct log_backend *backend = log_backend_get(i);

		log_backend_id_set(backend,
				   i + LOG_FILTER_FIRST_BACKEND_SLOT_IDX);

		backend_filter_init(backend);
		if (backend->api->init) {
			backend->api->init();
		}

		log_backend_activate(backend, NULL);
	}
}

static void thread_set(k_tid_t process_tid)
{
	proc_tid = process_tid;

	if (!IS_ENABLED(CONFIG_LOG_INPLACE_PROCESS) &&
	    CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD &&
	    process_tid &&
	    buffered_cnt >= CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) {
		k_wakeup(proc_tid);
	}
}

void log_thread_set(k_tid_t process_tid)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		assert(0);
	} else {
		thread_set(process_tid);
	}
}

int log_set_timestamp_func(timestamp_get_t timestamp_getter, u32_t freq)
{
	if (!timestamp_getter) {
		return -EINVAL;
	}

	timestamp_func = timestamp_getter;
	log_output_timestamp_freq_set(freq);

	return 0;
}

void log_panic(void)
{
	struct log_backend const *backend;

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);

		if (log_backend_is_active(backend)) {
			log_backend_panic(backend);
		}
	}

	panic_mode = true;

	/* Flush */
	while (log_process(false) == true) {
	}
}

static bool msg_filter_check(struct log_backend const *backend,
			     struct log_msg *msg)
{
	u32_t backend_level;
	u32_t msg_level;

	backend_level = log_filter_get(backend,
				       log_msg_domain_id_get(msg),
				       log_msg_source_id_get(msg),
				       true /*enum RUNTIME, COMPILETIME*/);
	msg_level = log_msg_level_get(msg);

	return (msg_level <= backend_level);
}

static void msg_process(struct log_msg *msg, bool bypass)
{
	struct log_backend const *backend;

	if (!bypass) {
		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);

			if (log_backend_is_active(backend) &&
			    msg_filter_check(backend, msg)) {
				log_backend_put(backend, msg);
			}
		}
	}

	log_msg_put(msg);
}

bool log_process(bool bypass)
{
	struct log_msg *msg;

	unsigned int key = irq_lock();

	msg = log_list_head_get(&list);
	irq_unlock(key);

	if (msg != NULL) {
		atomic_dec(&buffered_cnt);
		msg_process(msg, bypass);
	}

	return (log_list_head_peek(&list) != NULL);
}

u32_t log_buffered_cnt(void)
{
	return buffered_cnt;
}

u32_t log_src_cnt_get(u32_t domain_id)
{
	return log_sources_count();
}

const char *log_source_name_get(u32_t domain_id, u32_t src_id)
{
	assert(src_id < log_sources_count());

	return log_name_get(src_id);
}

static u32_t max_filter_get(u32_t filters)
{
	u32_t max_filter = LOG_LEVEL_NONE;
	int first_slot = LOG_FILTER_FIRST_BACKEND_SLOT_IDX;
	int i;

	for (i = first_slot; i < LOG_FILTERS_NUM_OF_SLOTS; i++) {
		u32_t tmp_filter = LOG_FILTER_SLOT_GET(&filters, i);

		if (tmp_filter > max_filter) {
			max_filter = tmp_filter;
		}
	}

	return max_filter;
}

void log_filter_set(struct log_backend const *const backend,
		    u32_t domain_id,
		    u32_t src_id,
		    u32_t level)
{
	assert(src_id < log_sources_count());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		u32_t new_aggr_filter;

		u32_t *filters = log_dynamic_filters_get(src_id);

		if (backend == NULL) {
			struct log_backend const *backend;

			for (int i = 0; i < log_backend_count_get(); i++) {
				backend = log_backend_get(i);
				log_filter_set(backend, domain_id,
					       src_id, level);
			}
		} else {
			LOG_FILTER_SLOT_SET(filters,
					    log_backend_id_get(backend),
					    level);

			/* Once current backend filter is updated recalculate
			 * aggregated maximal level
			 */
			new_aggr_filter = max_filter_get(*filters);

			LOG_FILTER_SLOT_SET(filters,
					    LOG_FILTER_AGGR_SLOT_IDX,
					    new_aggr_filter);
		}
	}
}

static void backend_filter_set(struct log_backend const *const backend,
			       u32_t level)
{
	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		for (int i = 0; i < log_sources_count(); i++) {
			log_filter_set(backend,
				       CONFIG_LOG_DOMAIN_ID,
				       i,
				       level);

		}
	}
}

void log_backend_enable(struct log_backend const *const backend,
			void *ctx,
			u32_t level)
{
	backend_filter_set(backend, level);
	log_backend_activate(backend, ctx);
}

void log_backend_disable(struct log_backend const *const backend)
{
	log_backend_deactivate(backend);
	backend_filter_set(backend, LOG_LEVEL_NONE);
}

u32_t log_filter_get(struct log_backend const *const backend,
		     u32_t domain_id,
		     u32_t src_id,
		     bool runtime)
{
	assert(src_id < log_sources_count());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) && runtime) {
		u32_t *filters = log_dynamic_filters_get(src_id);

		return LOG_FILTER_SLOT_GET(filters,
					   log_backend_id_get(backend));
	} else {
		return log_compiled_level_get(src_id);
	}
}

#ifdef CONFIG_LOG_PROCESS_THREAD
static void log_process_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	log_init();
	thread_set(k_current_get());

	while (1) {
		if (log_process(false) == false) {
			k_sleep(CONFIG_LOG_PROCESS_THREAD_SLEEP_MS);
		}
	}
}

K_THREAD_DEFINE(log_process_thread, CONFIG_LOG_PROCESS_THREAD_STACK_SIZE,
		log_process_thread_func, NULL, NULL, NULL,
		CONFIG_LOG_PROCESS_THREAD_PRIO, 0, K_NO_WAIT);
#else
#include <init.h>
static int enable_logger(struct device *arg)
{
	ARG_UNUSED(arg);
	log_init();
	return 0;
}
SYS_INIT(enable_logger, POST_KERNEL, 0);
#endif /* CONFIG_LOG_PROCESS_THREAD */
