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
#include <init.h>
#include <assert.h>
#include <atomic.h>

#ifndef CONFIG_LOG_PRINTK_MAX_STRING_LENGTH
#define CONFIG_LOG_PRINTK_MAX_STRING_LENGTH 1
#endif

#ifndef CONFIG_LOG_PROCESS_THREAD_SLEEP_MS
#define CONFIG_LOG_PROCESS_THREAD_SLEEP_MS 0
#endif

#ifndef CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD
#define CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD 0
#endif

#ifndef CONFIG_LOG_PROCESS_THREAD_STACK_SIZE
#define CONFIG_LOG_PROCESS_THREAD_STACK_SIZE 1
#endif

#ifndef CONFIG_LOG_STRDUP_MAX_STRING
#define CONFIG_LOG_STRDUP_MAX_STRING 0
#endif

#ifndef CONFIG_LOG_STRDUP_BUF_COUNT
#define CONFIG_LOG_STRDUP_BUF_COUNT 0
#endif

struct log_strdup_buf {
	atomic_t refcount;
	char buf[CONFIG_LOG_STRDUP_MAX_STRING + 1]; /* for termination */
};

#define LOG_STRDUP_POOL_BUFFER_SIZE \
	(sizeof(struct log_strdup_buf) * CONFIG_LOG_STRDUP_BUF_COUNT)

static const char *log_strdup_fail_msg = "<log_strdup alloc failed>";
struct k_mem_slab log_strdup_pool;
static u8_t __noinit __aligned(sizeof(u32_t))
		log_strdup_pool_buf[LOG_STRDUP_POOL_BUFFER_SIZE];

static struct log_list_t list;
static atomic_t initialized;
static bool panic_mode;
static bool backend_attached;
static atomic_t buffered_cnt;
static atomic_t dropped_cnt;
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

	if (panic_mode) {
		(void)log_process(false);
	} else if (CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) {
		if ((buffered_cnt == CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) &&
		    (proc_tid != NULL)) {
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
	int length = 0;

	if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
		struct log_msg_ids src_level = {
			.level = LOG_LEVEL_INTERNAL_RAW_STRING
		};

		if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
			log_generic(src_level, fmt, ap);
		} else {
			u8_t formatted_str[CONFIG_LOG_PRINTK_MAX_STRING_LENGTH];
			struct log_msg *msg;

			length = vsnprintk(formatted_str,
					   sizeof(formatted_str), fmt, ap);
			length = MIN(length, sizeof(formatted_str));

			msg = log_msg_hexdump_create(NULL, formatted_str,
						     length);
			if (msg == NULL) {
				return 0;
			}

			msg_finalize(msg, src_level);
		}
	}

	return length;
}

/** @brief Count number of arguments in formatted string.
 *
 * Function counts number of '%' not followed by '%'.
 */
static u32_t count_args(const char *fmt)
{
	u32_t args = 0U;
	bool prev = false; /* if previous char was a modificator. */

	while (*fmt != '\0') {
		if (*fmt == '%') {
			prev = !prev;
		} else if (prev) {
			args++;
			prev = false;
		}
		fmt++;
	}

	return args;
}

void log_generic(struct log_msg_ids src_level, const char *fmt, va_list ap)
{
	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		struct log_backend const *backend;
		u32_t timestamp = timestamp_func();

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);

			if (log_backend_is_active(backend)) {
				log_backend_put_sync_string(backend, src_level,
						     timestamp, fmt, ap);
			}
		}
	} else {
		u32_t args[LOG_MAX_NARGS];
		u32_t nargs = count_args(fmt);

		for (int i = 0; i < nargs; i++) {
			args[i] = va_arg(ap, u32_t);
		}

		log_n(fmt, args, nargs, src_level);
	}
}

void log_string_sync(struct log_msg_ids src_level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	log_generic(src_level, fmt, ap);

	va_end(ap);
}

void log_hexdump_sync(struct log_msg_ids src_level, const char *metadata,
		      const u8_t *data, u32_t len)
{
	struct log_backend const *backend;
	u32_t timestamp = timestamp_func();

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);

		if (log_backend_is_active(backend)) {
			log_backend_put_sync_hexdump(backend, src_level,
						     timestamp, metadata,
						     data, len);
		}
	}
}

static u32_t timestamp_get(void)
{
	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > 1000000) {
		return k_uptime_get_32();
	} else {
		return k_cycle_get_32();
	}
}

void log_core_init(void)
{
	u32_t freq = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > 1000000) ?
			1000 : CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	if (!IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		log_msg_pool_init();
		log_list_init(&list);

		k_mem_slab_init(&log_strdup_pool, log_strdup_pool_buf,
					sizeof(struct log_strdup_buf),
					CONFIG_LOG_STRDUP_BUF_COUNT);
	}

	/* Set default timestamp. */
	timestamp_func = timestamp_get;
	log_output_timestamp_freq_set(freq);

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

void log_init(void)
{
	assert(log_backend_count_get() < LOG_FILTERS_NUM_OF_SLOTS);
	int i;

	if (atomic_inc(&initialized) != 0) {
		return;
	}

	/* Assign ids to backends. */
	for (i = 0; i < log_backend_count_get(); i++) {
		const struct log_backend *backend = log_backend_get(i);

		if (backend->autostart) {
			if (backend->api->init != NULL) {
				backend->api->init();
			}

			log_backend_enable(backend, NULL, CONFIG_LOG_MAX_LEVEL);
		}
	}
}

static void thread_set(k_tid_t process_tid)
{
	proc_tid = process_tid;

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		return;
	}

	if (CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD &&
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

	if (panic_mode) {
		return;
	}

	/* If panic happend early logger might not be initialized.
	 * Forcing initialization of the logger and auto-starting backends.
	 */
	log_init();

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);

		if (log_backend_is_active(backend)) {
			log_backend_panic(backend);
		}
	}

	if (!IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		/* Flush */
		while (log_process(false) == true) {
		}
	}

	panic_mode = true;
}

static bool msg_filter_check(struct log_backend const *backend,
			     struct log_msg *msg)
{
	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		u32_t backend_level;
		u32_t msg_level;

		backend_level = log_filter_get(backend,
					       log_msg_domain_id_get(msg),
					       log_msg_source_id_get(msg),
					       true /*enum RUNTIME, COMPILETIME*/);
		msg_level = log_msg_level_get(msg);

		return (msg_level <= backend_level);
	} else {
		return true;
	}
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

void dropped_notify(void)
{
	u32_t dropped = atomic_set(&dropped_cnt, 0);

	for (int i = 0; i < log_backend_count_get(); i++) {
		struct log_backend const *backend = log_backend_get(i);

		if (log_backend_is_active(backend)) {
			log_backend_dropped(backend, dropped);
		}
	}
}

bool log_process(bool bypass)
{
	struct log_msg *msg;

	if (!backend_attached && !bypass) {
		return false;
	}
	unsigned int key = irq_lock();

	msg = log_list_head_get(&list);
	irq_unlock(key);

	if (msg != NULL) {
		atomic_dec(&buffered_cnt);
		msg_process(msg, bypass);
	}

	if (!bypass && dropped_cnt) {
		dropped_notify();
	}

	return (log_list_head_peek(&list) != NULL);
}

u32_t log_buffered_cnt(void)
{
	return buffered_cnt;
}

void log_dropped(void)
{
	atomic_inc(&dropped_cnt);
}

u32_t log_src_cnt_get(u32_t domain_id)
{
	return log_sources_count();
}

const char *log_source_name_get(u32_t domain_id, u32_t src_id)
{
	return src_id < log_sources_count() ? log_name_get(src_id) : NULL;
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

u32_t log_filter_set(struct log_backend const *const backend,
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
			u32_t max = 0U;
			u32_t current;

			for (int i = 0; i < log_backend_count_get(); i++) {
				backend = log_backend_get(i);
				current = log_filter_set(backend, domain_id,
							 src_id, level);
				max = MAX(current, max);
			}

			level = max;
		} else {
			u32_t max = log_filter_get(backend, domain_id,
						   src_id, false);

			level = MIN(level, max);

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

	return level;
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
	/* As first slot in filtering mask is reserved, backend ID has offset.*/
	u32_t id = LOG_FILTER_FIRST_BACKEND_SLOT_IDX;

	id += backend - log_backend_get(0);

	log_backend_id_set(backend, id);
	backend_filter_set(backend, level);
	log_backend_activate(backend, ctx);
	backend_attached = true;
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

char *log_strdup(const char *str)
{
	struct log_strdup_buf *dup;
	int err;

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		return (char *)str;
	}

	err = k_mem_slab_alloc(&log_strdup_pool, (void **)&dup, K_NO_WAIT);
	if (err != 0) {
		/* failed to allocate */
		return (char *)log_strdup_fail_msg;
	}

	/* Set 'allocated' flag. */
	(void)atomic_set(&dup->refcount, 1);

	strncpy(dup->buf, str, sizeof(dup->buf) - 2);
	dup->buf[sizeof(dup->buf) - 2] = '~';
	dup->buf[sizeof(dup->buf) - 1] = '\0';

	return dup->buf;
}

bool log_is_strdup(void *buf)
{
	struct log_strdup_buf *pool_first, *pool_last;

	pool_first = (struct log_strdup_buf *)log_strdup_pool_buf;
	pool_last = pool_first + CONFIG_LOG_STRDUP_BUF_COUNT - 1;

	return ((char *)buf >= pool_first->buf) &&
	       ((char *)buf <= pool_last->buf);

}

void log_free(void *str)
{
	struct log_strdup_buf *dup = CONTAINER_OF(str, struct log_strdup_buf,
						  buf);

	if (atomic_dec(&dup->refcount) == 1) {
		k_mem_slab_free(&log_strdup_pool, (void **)&dup);
	}
}

static void log_process_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	__ASSERT_NO_MSG(log_backend_count_get() > 0);

	log_init();
	thread_set(k_current_get());

	while (true) {
		if (log_process(false) == false) {
			k_sleep(CONFIG_LOG_PROCESS_THREAD_SLEEP_MS);
		}
	}
}

K_THREAD_STACK_DEFINE(logging_stack, CONFIG_LOG_PROCESS_THREAD_STACK_SIZE);
struct k_thread logging_thread;

static int enable_logger(struct device *arg)
{
	ARG_UNUSED(arg);

	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		/* start logging thread */
		k_thread_create(&logging_thread, logging_stack,
				K_THREAD_STACK_SIZEOF(logging_stack),
				log_process_thread_func, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
		k_thread_name_set(&logging_thread, "logging");
	} else {
		log_init();
	}

	return 0;
}

SYS_INIT(enable_logger, POST_KERNEL, 0);
