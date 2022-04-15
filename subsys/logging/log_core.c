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
#include <logging/log_internal.h>
#include <sys/mpsc_pbuf.h>
#include <sys/printk.h>
#include <sys_clock.h>
#include <init.h>
#include <sys/__assert.h>
#include <sys/atomic.h>
#include <ctype.h>
#include <logging/log_frontend.h>
#include <syscall_handler.h>
#include <logging/log_output_dict.h>
#include <linker/utils.h>

LOG_MODULE_REGISTER(log);

#ifndef CONFIG_LOG_PRINTK_MAX_STRING_LENGTH
#define CONFIG_LOG_PRINTK_MAX_STRING_LENGTH 0
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
/* Required to suppress compiler warnings related to array subscript above array bounds.
 * log_strdup explicitly accesses element with index of (sizeof(log_strdup_buf.buf) - 2).
 * Set to 2 because some compilers generate warning on strncpy(dst, src, 0).
 */
#define CONFIG_LOG_STRDUP_MAX_STRING 2
#endif

#ifndef CONFIG_LOG_STRDUP_BUF_COUNT
#define CONFIG_LOG_STRDUP_BUF_COUNT 0
#endif

#ifndef CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS
#define CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS 0
#endif

#ifndef CONFIG_LOG_BUFFER_SIZE
#define CONFIG_LOG_BUFFER_SIZE 4
#endif

#ifndef CONFIG_LOG_TAG_MAX_LEN
#define CONFIG_LOG_TAG_MAX_LEN 0
#endif

#ifndef CONFIG_LOG2_ALWAYS_RUNTIME
BUILD_ASSERT(!IS_ENABLED(CONFIG_NO_OPTIMIZATIONS),
	     "Option must be enabled when CONFIG_NO_OPTIMIZATIONS is set");
BUILD_ASSERT(!IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE),
	     "Option must be enabled when CONFIG_LOG_MODE_IMMEDIATE is set");
#endif

#ifndef CONFIG_LOG1
static const log_format_func_t format_table[] = {
	[LOG_OUTPUT_TEXT] = log_output_msg2_process,
	[LOG_OUTPUT_SYST] = IS_ENABLED(CONFIG_LOG_MIPI_SYST_ENABLE) ?
						log_output_msg2_syst_process : NULL,
	[LOG_OUTPUT_DICT] = IS_ENABLED(CONFIG_LOG_DICTIONARY_SUPPORT) ?
						log_dict_output_msg2_process : NULL
};

log_format_func_t log_format_func_t_get(uint32_t log_type)
{
	return format_table[log_type];
}

size_t log_format_table_size(void)
{
	return ARRAY_SIZE(format_table);
}
#endif

struct log_strdup_buf {
	atomic_t refcount;
	char buf[CONFIG_LOG_STRDUP_MAX_STRING + 1]; /* for termination */
} __aligned(sizeof(uintptr_t));

union log_msgs {
	struct log_msg *msg;
	union log_msg2_generic *msg2;
};

#define LOG_STRDUP_POOL_BUFFER_SIZE \
	(sizeof(struct log_strdup_buf) * CONFIG_LOG_STRDUP_BUF_COUNT)

K_SEM_DEFINE(log_process_thread_sem, 0, 1);

static const char *log_strdup_fail_msg = "<log_strdup alloc failed>";
struct k_mem_slab log_strdup_pool;
static uint8_t __noinit __aligned(sizeof(void *))
		log_strdup_pool_buf[LOG_STRDUP_POOL_BUFFER_SIZE];

static struct log_list_t list;
static atomic_t initialized;
static bool panic_mode;
static bool backend_attached;
static atomic_t buffered_cnt;
static atomic_t dropped_cnt;
static k_tid_t proc_tid;
static atomic_t log_strdup_in_use;
static uint32_t log_strdup_max;
static uint32_t log_strdup_longest;
static struct k_timer log_process_thread_timer;

static log_timestamp_t dummy_timestamp(void);
static log_timestamp_get_t timestamp_func = dummy_timestamp;

struct mpsc_pbuf_buffer log_buffer;
static uint32_t __aligned(Z_LOG_MSG2_ALIGNMENT)
	buf32[CONFIG_LOG_BUFFER_SIZE / sizeof(int)];

static void notify_drop(const struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic *item);

static const struct mpsc_pbuf_buffer_config mpsc_config = {
	.buf = (uint32_t *)buf32,
	.size = ARRAY_SIZE(buf32),
	.notify_drop = notify_drop,
	.get_wlen = log_msg2_generic_get_wlen,
	.flags = (IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW) ?
		  MPSC_PBUF_MODE_OVERWRITE : 0) |
		 (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION) ?
		  MPSC_PBUF_MAX_UTILIZATION : 0)
};

/* Check that default tag can fit in tag buffer. */
COND_CODE_0(CONFIG_LOG_TAG_MAX_LEN, (),
	(BUILD_ASSERT(sizeof(CONFIG_LOG_TAG_DEFAULT) <= CONFIG_LOG_TAG_MAX_LEN + 1,
		      "Default string longer than tag capacity")));

static char tag[CONFIG_LOG_TAG_MAX_LEN + 1] =
	COND_CODE_0(CONFIG_LOG_TAG_MAX_LEN, ({}), (CONFIG_LOG_TAG_DEFAULT));

bool log_is_strdup(const void *buf);
static void msg_process(union log_msgs msg, bool bypass);

static log_timestamp_t dummy_timestamp(void)
{
	return 0;
}

log_timestamp_t z_log_timestamp(void)
{
	return timestamp_func();
}

uint32_t z_log_get_s_mask(const char *str, uint32_t nargs)
{
	char curr;
	bool arm = false;
	uint32_t arg = 0U;
	uint32_t mask = 0U;

	__ASSERT_NO_MSG(nargs <= 8*sizeof(mask));

	while ((curr = *str++) && arg < nargs) {
		if (curr == '%') {
			arm = !arm;
		} else if (arm && isalpha((int)curr)) {
			if (curr == 's') {
				mask |= BIT(arg);
			}
			arm = false;
			arg++;
		} else {
			; /* standard character, continue walk */
		}
	}

	return mask;
}

/**
 * @brief Scan string arguments and report every address which is not in read
 *	  only memory and not yet duplicated.
 *
 * @param msg Log message.
 */
static void detect_missed_strdup(struct log_msg *msg)
{
#define ERR_MSG	"argument %d in source %s log message \"%s\" missing " \
		"log_strdup()."
	uint32_t idx;
	const char *str;
	const char *msg_str;
	uint32_t mask;

	if (!log_msg_is_std(msg)) {
		return;
	}

	msg_str = log_msg_str_get(msg);
	mask = z_log_get_s_mask(msg_str, log_msg_nargs_get(msg));

	while (mask) {
		idx = 31 - __builtin_clz(mask);
		str = (const char *)log_msg_arg_get(msg, idx);
		if (!linker_is_in_rodata(str) && !log_is_strdup(str) &&
			(str != log_strdup_fail_msg)) {
			const char *src_name =
				log_source_name_get(CONFIG_LOG_DOMAIN_ID,
						    log_msg_source_id_get(msg));

			if (IS_ENABLED(CONFIG_ASSERT)) {
				__ASSERT(0, ERR_MSG, idx, src_name, msg_str);
			} else {
				LOG_ERR(ERR_MSG, idx, src_name, msg_str);
			}
		}

		mask &= ~BIT(idx);
	}
#undef ERR_MSG
}

static void z_log_msg_post_finalize(void)
{
	atomic_val_t cnt = atomic_inc(&buffered_cnt);

	if (panic_mode) {
		unsigned int key = irq_lock();
		(void)log_process(false);
		irq_unlock(key);
	} else if (proc_tid != NULL && cnt == 0) {
		k_timer_start(&log_process_thread_timer,
			K_MSEC(CONFIG_LOG_PROCESS_THREAD_SLEEP_MS), K_NO_WAIT);
	} else if (CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) {
		if ((cnt == CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) &&
		    (proc_tid != NULL)) {
			k_timer_stop(&log_process_thread_timer);
			k_sem_give(&log_process_thread_sem);
		}
	} else {
		/* No action needed. Message processing will be triggered by the
		 * timeout or when number of upcoming messages exceeds the
		 * threshold.
		 */
		;
	}
}

static inline void msg_finalize(struct log_msg *msg,
				struct log_msg_ids src_level)
{
	unsigned int key;

	msg->hdr.ids = src_level;
	msg->hdr.timestamp = timestamp_func();

	key = irq_lock();

	log_list_add_tail(&list, msg);

	irq_unlock(key);

	z_log_msg_post_finalize();
}

void log_0(const char *str, struct log_msg_ids src_level)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_0(str, src_level);
	} else {
		struct log_msg *msg = log_msg_create_0(str);

		if (msg == NULL) {
			return;
		}
		msg_finalize(msg, src_level);
	}
}

void log_1(const char *str,
	   log_arg_t arg0,
	   struct log_msg_ids src_level)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_1(str, arg0, src_level);
	} else {
		struct log_msg *msg = log_msg_create_1(str, arg0);

		if (msg == NULL) {
			return;
		}
		msg_finalize(msg, src_level);
	}
}

void log_2(const char *str,
	   log_arg_t arg0,
	   log_arg_t arg1,
	   struct log_msg_ids src_level)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_2(str, arg0, arg1, src_level);
	} else {
		struct log_msg *msg = log_msg_create_2(str, arg0, arg1);

		if (msg == NULL) {
			return;
		}

		msg_finalize(msg, src_level);
	}
}

void log_3(const char *str,
	   log_arg_t arg0,
	   log_arg_t arg1,
	   log_arg_t arg2,
	   struct log_msg_ids src_level)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_3(str, arg0, arg1, arg2, src_level);
	} else {
		struct log_msg *msg = log_msg_create_3(str, arg0, arg1, arg2);

		if (msg == NULL) {
			return;
		}

		msg_finalize(msg, src_level);
	}
}

void log_n(const char *str,
	   log_arg_t *args,
	   uint32_t narg,
	   struct log_msg_ids src_level)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_n(str, args, narg, src_level);
	} else {
		struct log_msg *msg = log_msg_create_n(str, args, narg);

		if (msg == NULL) {
			return;
		}

		msg_finalize(msg, src_level);
	}
}

#ifndef CONFIG_LOG1
const struct log_backend *log_format_set_all_active_backends(size_t log_type)
{
	const struct log_backend *backend;
	const struct log_backend *failed_backend = NULL;

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);
		if (log_backend_is_active(backend)) {
			int retCode = log_backend_format_set(backend, log_type);

			if (retCode != 0) {
				failed_backend = backend;
			}
		}
	}
	return failed_backend;
}
#endif

void log_hexdump(const char *str, const void *data, uint32_t length,
		 struct log_msg_ids src_level)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_hexdump(str, (const uint8_t *)data, length,
				     src_level);
	} else {
		struct log_msg *msg =
			log_msg_hexdump_create(str, (const uint8_t *)data, length);

		if (msg == NULL) {
			return;
		}

		msg_finalize(msg, src_level);
	}
}

void z_log_vprintk(const char *fmt, va_list ap)
{
	if (!IS_ENABLED(CONFIG_LOG_PRINTK)) {
		return;
	}

	if (!IS_ENABLED(CONFIG_LOG1)) {
		z_log_msg2_runtime_vcreate(CONFIG_LOG_DOMAIN_ID, NULL,
					   LOG_LEVEL_INTERNAL_RAW_STRING, NULL, 0, 0,
					   fmt, ap);
		return;
	}

	union {
		struct log_msg_ids structure;
		uint32_t value;
	} src_level_union = {
		{
			.level = LOG_LEVEL_INTERNAL_RAW_STRING
		}
	};

	if (k_is_user_context()) {
		uint8_t str[CONFIG_LOG_PRINTK_MAX_STRING_LENGTH + 1];

		vsnprintk(str, sizeof(str), fmt, ap);

		z_log_string_from_user(src_level_union.value, str);
	} else if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		log_generic(src_level_union.structure, fmt, ap,
						LOG_STRDUP_SKIP);
	} else {
		uint8_t str[CONFIG_LOG_PRINTK_MAX_STRING_LENGTH + 1];
		struct log_msg *msg;
		int length;

		length = vsnprintk(str, sizeof(str), fmt, ap);
		length = MIN(length, sizeof(str));

		msg = log_msg_hexdump_create(NULL, str, length);
		if (msg == NULL) {
			return;
		}

		msg_finalize(msg, src_level_union.structure);
	}
}

/** @brief Count number of arguments in formatted string.
 *
 * Function counts number of '%' not followed by '%'.
 */
uint32_t log_count_args(const char *fmt)
{
	uint32_t args = 0U;
	bool prev = false; /* if previous char was a modificator. */

	while (*fmt != '\0') {
		if (*fmt == '%') {
			prev = !prev;
		} else if (prev) {
			args++;
			prev = false;
		} else {
			; /* standard character, continue walk */
		}
		fmt++;
	}

	return args;
}

void log_generic(struct log_msg_ids src_level, const char *fmt, va_list ap,
		 enum log_strdup_action strdup_action)
{
	if (k_is_user_context()) {
		log_generic_from_user(src_level, fmt, ap);
	} else if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) &&
	    (!IS_ENABLED(CONFIG_LOG_FRONTEND))) {
		struct log_backend const *backend;
		uint32_t timestamp = timestamp_func();

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);
			bool runtime_ok =
				IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
				(src_level.level <= log_filter_get(backend,
								src_level.domain_id,
								src_level.source_id,
								true)) : true;

			if (log_backend_is_active(backend) && runtime_ok) {
				va_list ap_tmp;

				va_copy(ap_tmp, ap);
				log_backend_put_sync_string(backend, src_level,
						     timestamp, fmt, ap_tmp);
				va_end(ap_tmp);
			}
		}
	} else {
		log_arg_t args[LOG_MAX_NARGS];
		uint32_t nargs = log_count_args(fmt);

		__ASSERT_NO_MSG(nargs < LOG_MAX_NARGS);
		for (int i = 0; i < nargs; i++) {
			args[i] = va_arg(ap, log_arg_t);
		}

		if (strdup_action != LOG_STRDUP_SKIP) {
			uint32_t mask = z_log_get_s_mask(fmt, nargs);

			while (mask) {
				uint32_t idx = 31 - __builtin_clz(mask);
				const char *str = (const char *)args[idx];

				/* linker_is_in_rodata(str) is not checked,
				 * because log_strdup does it.
				 * Hence, we will do only optional check
				 * if already not duplicated.
				 */
				if (strdup_action == LOG_STRDUP_EXEC
				   || !log_is_strdup(str)) {
					args[idx] = (log_arg_t)log_strdup(str);
				}
				mask &= ~BIT(idx);
			}
		}
		log_n(fmt, args, nargs, src_level);
	}
}

void log_string_sync(struct log_msg_ids src_level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	log_generic(src_level, fmt, ap, LOG_STRDUP_SKIP);

	va_end(ap);
}

void log_hexdump_sync(struct log_msg_ids src_level, const char *metadata,
		      const void *data, uint32_t len)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_hexdump(metadata, (const uint8_t *)data, len,
				     src_level);
	} else {
		struct log_backend const *backend;
		log_timestamp_t timestamp = timestamp_func();

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);
			bool runtime_ok =
				IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
				(src_level.level <= log_filter_get(backend,
								src_level.domain_id,
								src_level.source_id,
								true)) : true;

			if (log_backend_is_active(backend) && runtime_ok) {
				log_backend_put_sync_hexdump(
					backend, src_level, timestamp, metadata,
					(const uint8_t *)data, len);
			}
		}
	}
}

static log_timestamp_t default_get_timestamp(void)
{
	return IS_ENABLED(CONFIG_LOG_TIMESTAMP_64BIT) ?
		sys_clock_tick_get() : k_cycle_get_32();
}

static log_timestamp_t default_lf_get_timestamp(void)
{
	return IS_ENABLED(CONFIG_LOG_TIMESTAMP_64BIT) ?
		k_uptime_get() : k_uptime_get_32();
}

void log_core_init(void)
{
	uint32_t freq;
	log_timestamp_get_t _timestamp_func;

	panic_mode = false;
	dropped_cnt = 0;

	/* Set default timestamp. */
	if (sys_clock_hw_cycles_per_sec() > 1000000) {
		_timestamp_func = default_lf_get_timestamp;
		freq = 1000U;
	} else {
		_timestamp_func = default_get_timestamp;
		freq = sys_clock_hw_cycles_per_sec();
	}

	log_set_timestamp_func(_timestamp_func, freq);

	if (IS_ENABLED(CONFIG_LOG2_DEFERRED)) {
		z_log_msg2_init();
	} else if (IS_ENABLED(CONFIG_LOG1_DEFERRED)) {
		log_msg_pool_init();
		log_list_init(&list);

		k_mem_slab_init(&log_strdup_pool, log_strdup_pool_buf,
					sizeof(struct log_strdup_buf),
					CONFIG_LOG_STRDUP_BUF_COUNT);
	}

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		z_log_runtime_filters_init();
	}

	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_init();
	}
}

void log_init(void)
{
	__ASSERT_NO_MSG(log_backend_count_get() < LOG_FILTERS_NUM_OF_SLOTS);
	int i;

	if (atomic_inc(&initialized) != 0) {
		return;
	}

	/* Assign ids to backends. */
	for (i = 0; i < log_backend_count_get(); i++) {
		const struct log_backend *backend = log_backend_get(i);

		if (backend->autostart) {
			if (backend->api->init != NULL) {
				backend->api->init(backend);
			}

			log_backend_enable(backend,
					   backend->cb->ctx,
					   CONFIG_LOG_MAX_LEVEL);
		}
	}
}

static void thread_set(k_tid_t process_tid)
{
	proc_tid = process_tid;

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		return;
	}

	if (CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD &&
	    process_tid &&
	    buffered_cnt >= CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) {
		k_sem_give(&log_process_thread_sem);
	}
}

void log_thread_set(k_tid_t process_tid)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		__ASSERT_NO_MSG(0);
	} else {
		thread_set(process_tid);
	}
}

int log_set_timestamp_func(log_timestamp_get_t timestamp_getter, uint32_t freq)
{
	if (timestamp_getter == NULL) {
		return -EINVAL;
	}

	timestamp_func = timestamp_getter;
	log_output_timestamp_freq_set(freq);

	return 0;
}

void z_impl_log_panic(void)
{
	struct log_backend const *backend;

	if (panic_mode) {
		return;
	}

	/* If panic happened early logger might not be initialized.
	 * Forcing initialization of the logger and auto-starting backends.
	 */
	log_init();

	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_panic();
	}

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);

		if (log_backend_is_active(backend)) {
			log_backend_panic(backend);
		}
	}

	if (!IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		/* Flush */
		while (log_process(false) == true) {
		}
	}

	panic_mode = true;
}

#ifdef CONFIG_USERSPACE
void z_vrfy_log_panic(void)
{
	z_impl_log_panic();
}
#include <syscalls/log_panic_mrsh.c>
#endif

static bool msg_filter_check(struct log_backend const *backend,
			     union log_msgs msg)
{
	if (IS_ENABLED(CONFIG_LOG2) && !z_log_item_is_msg(msg.msg2)) {
		return true;
	}

	if (!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		return true;
	}

	uint32_t backend_level;
	uint8_t level;
	uint8_t domain_id;
	int16_t source_id;

	if (IS_ENABLED(CONFIG_LOG2)) {
		struct log_msg2 *msg2 = &msg.msg2->log;
		struct log_source_dynamic_data *source =
			(struct log_source_dynamic_data *)
			log_msg2_get_source(msg2);

		level = log_msg2_get_level(msg2);
		domain_id = log_msg2_get_domain(msg2);
		source_id = source ? log_dynamic_source_id(source) : -1;
	} else {
		level = log_msg_level_get(msg.msg);
		domain_id = log_msg_domain_id_get(msg.msg);
		source_id = log_msg_source_id_get(msg.msg);
	}

	backend_level = log_filter_get(backend, domain_id,
				       source_id, true);

	return (level <= backend_level);
}

static void msg_process(union log_msgs msg, bool bypass)
{
	struct log_backend const *backend;

	if (!bypass) {
		if (!IS_ENABLED(CONFIG_LOG2) &&
		    IS_ENABLED(CONFIG_LOG_DETECT_MISSED_STRDUP) &&
		    !panic_mode) {
			detect_missed_strdup(msg.msg);
		}

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);
			if (log_backend_is_active(backend) &&
			    msg_filter_check(backend, msg)) {
				if (IS_ENABLED(CONFIG_LOG2)) {
					log_backend_msg2_process(backend,
								 msg.msg2);
				} else {
					log_backend_put(backend, msg.msg);
				}
			}
		}
	}

	if (IS_ENABLED(CONFIG_LOG2_DEFERRED)) {
		z_log_msg2_free(msg.msg2);
	} else if (IS_ENABLED(CONFIG_LOG1_DEFERRED)) {
		log_msg_put(msg.msg);
	}
}

void dropped_notify(void)
{
	uint32_t dropped = z_log_dropped_read_and_clear();

	for (int i = 0; i < log_backend_count_get(); i++) {
		struct log_backend const *backend = log_backend_get(i);

		if (log_backend_is_active(backend)) {
			log_backend_dropped(backend, dropped);
		}
	}
}

union log_msgs get_msg(void)
{
	union log_msgs msg;

	if (IS_ENABLED(CONFIG_LOG2)) {
		msg.msg2 = z_log_msg2_claim();

		return msg;
	}

	int key = irq_lock();

	msg.msg = log_list_head_get(&list);
	irq_unlock(key);

	return msg;
}

static bool next_pending(void)
{
	if (IS_ENABLED(CONFIG_LOG2)) {
		return z_log_msg2_pending();
	}

	return (log_list_head_peek(&list) != NULL);
}

void z_log_notify_backend_enabled(void)
{
	/* Wakeup logger thread after attaching first backend. It might be
	 * blocked with log messages pending.
	 */
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD) && !backend_attached) {
		k_sem_give(&log_process_thread_sem);
	}

	backend_attached = true;
}

bool z_impl_log_process(bool bypass)
{
	union log_msgs msg;

	if (!backend_attached && !bypass) {
		return false;
	}

	msg = get_msg();
	if (msg.msg) {
		if (!bypass) {
			atomic_dec(&buffered_cnt);
		}
		msg_process(msg, bypass);
	}

	if (!bypass && z_log_dropped_pending()) {
		dropped_notify();
	}

	return next_pending();
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_log_process(bool bypass)
{
	return z_impl_log_process(bypass);
}
#include <syscalls/log_process_mrsh.c>
#endif

uint32_t z_impl_log_buffered_cnt(void)
{
	return buffered_cnt;
}

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_log_buffered_cnt(void)
{
	return z_impl_log_buffered_cnt();
}
#include <syscalls/log_buffered_cnt_mrsh.c>
#endif

void z_log_dropped(bool buffered)
{
	atomic_inc(&dropped_cnt);
	if (buffered) {
		atomic_dec(&buffered_cnt);
	}
}

uint32_t z_log_dropped_read_and_clear(void)
{
	return atomic_set(&dropped_cnt, 0);
}

bool z_log_dropped_pending(void)
{
	return dropped_cnt > 0;
}

static void notify_drop(const struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic *item)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(item);

	z_log_dropped(true);
}


char *z_log_strdup(const char *str)
{
	struct log_strdup_buf *dup;
	int err;

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ||
	    linker_is_in_rodata(str) || k_is_user_context()) {
		return (char *)str;
	}

	err = k_mem_slab_alloc(&log_strdup_pool, (void **)&dup, K_NO_WAIT);
	if (err != 0) {
		/* failed to allocate */
		return (char *)log_strdup_fail_msg;
	}

	if (IS_ENABLED(CONFIG_LOG_STRDUP_POOL_PROFILING)) {
		size_t slen = strlen(str);
		static struct k_spinlock lock;
		k_spinlock_key_t key;

		key = k_spin_lock(&lock);
		log_strdup_in_use++;
		log_strdup_max = MAX(log_strdup_in_use, log_strdup_max);
		log_strdup_longest = MAX(slen, log_strdup_longest);
		k_spin_unlock(&lock, key);
	}

	/* Set 'allocated' flag. */
	(void)atomic_set(&dup->refcount, 1);

	strncpy(dup->buf, str, sizeof(dup->buf) - 2);
	dup->buf[sizeof(dup->buf) - 2] = '~';
	dup->buf[sizeof(dup->buf) - 1] = '\0';

	return dup->buf;
}

uint32_t log_get_strdup_pool_current_utilization(void)
{
	return IS_ENABLED(CONFIG_LOG_STRDUP_POOL_PROFILING) ?
			log_strdup_in_use : 0;

}

uint32_t log_get_strdup_pool_utilization(void)
{
	return IS_ENABLED(CONFIG_LOG_STRDUP_POOL_PROFILING) ?
			log_strdup_max : 0;
}

uint32_t log_get_strdup_longest_string(void)
{
	return IS_ENABLED(CONFIG_LOG_STRDUP_POOL_PROFILING) ?
			log_strdup_longest : 0;
}

bool log_is_strdup(const void *buf)
{
	return PART_OF_ARRAY(log_strdup_pool_buf, (uint8_t *)buf);

}

void z_log_free(void *str)
{
	struct log_strdup_buf *dup = CONTAINER_OF(str, struct log_strdup_buf,
						  buf);

	if (atomic_dec(&dup->refcount) == 1) {
		k_mem_slab_free(&log_strdup_pool, (void **)&dup);
		if (IS_ENABLED(CONFIG_LOG_STRDUP_POOL_PROFILING)) {
			atomic_dec(&log_strdup_in_use);
		}
	}
}

#if defined(CONFIG_USERSPACE)
/* LCOV_EXCL_START */
void z_impl_z_log_string_from_user(uint32_t src_level_val, const char *str)
{
	ARG_UNUSED(src_level_val);
	ARG_UNUSED(str);

	__ASSERT(false, "This function can be called from user mode only.");
}
/* LCOV_EXCL_STOP */

void z_vrfy_z_log_string_from_user(uint32_t src_level_val, const char *str)
{
	uint8_t level, domain_id, source_id;
	union {
		struct log_msg_ids structure;
		uint32_t value;
	} src_level_union;
	size_t len;
	int err;

	src_level_union.value = src_level_val;
	level = src_level_union.structure.level;
	domain_id = src_level_union.structure.domain_id;
	source_id = src_level_union.structure.source_id;

	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		(IS_ENABLED(CONFIG_LOG_PRINTK) || (level >= LOG_LEVEL_ERR)) &&
		(level <= LOG_LEVEL_DBG),
		"Invalid log level"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(domain_id == CONFIG_LOG_DOMAIN_ID,
		"Invalid log domain_id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(source_id < z_log_sources_count(),
		"Invalid log source id"));

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) &&
	    (level != LOG_LEVEL_INTERNAL_RAW_STRING) &&
	    (level > LOG_FILTER_SLOT_GET(z_log_dynamic_filters_get(source_id),
					LOG_FILTER_AGGR_SLOT_IDX))) {
		/* Skip filtered out messages. */
		return;
	}

	/*
	 * Validate and make a copy of the source string. Because we need
	 * the log subsystem to eventually free it, we're going to use
	 * log_strdup().
	 */
	len = z_user_string_nlen(str, (level == LOG_LEVEL_INTERNAL_RAW_STRING) ?
				 CONFIG_LOG_PRINTK_MAX_STRING_LENGTH :
				 CONFIG_LOG_STRDUP_MAX_STRING, &err);

	Z_OOPS(Z_SYSCALL_VERIFY_MSG(err == 0, "invalid string passed in"));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(str, len));

	if (IS_ENABLED(CONFIG_LOG1_IMMEDIATE)) {
		log_string_sync(src_level_union.structure, "%s", str);
	} else if (IS_ENABLED(CONFIG_LOG_PRINTK) &&
		   (level == LOG_LEVEL_INTERNAL_RAW_STRING)) {
		struct log_msg *msg;

		msg = log_msg_hexdump_create(NULL, str, len);
		if (msg != NULL) {
			msg_finalize(msg, src_level_union.structure);
		}
	} else {
		str = log_strdup(str);
		log_1("%s", (log_arg_t)str, src_level_union.structure);
	}
}
#include <syscalls/z_log_string_from_user_mrsh.c>

void log_generic_from_user(struct log_msg_ids src_level,
			   const char *fmt, va_list ap)
{
	char buffer[CONFIG_LOG_STRDUP_MAX_STRING + 1];
	union {
		struct log_msg_ids structure;
		uint32_t value;
	} src_level_union;

	vsnprintk(buffer, sizeof(buffer), fmt, ap);

	__ASSERT_NO_MSG(sizeof(src_level) <= sizeof(uint32_t));
	src_level_union.structure = src_level;
	z_log_string_from_user(src_level_union.value, buffer);
}

void log_from_user(struct log_msg_ids src_level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_generic_from_user(src_level, fmt, ap);
	va_end(ap);
}

/* LCOV_EXCL_START */
void z_impl_z_log_hexdump_from_user(uint32_t src_level_val, const char *metadata,
				    const uint8_t *data, uint32_t len)
{
	ARG_UNUSED(src_level_val);
	ARG_UNUSED(metadata);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	__ASSERT(false, "This function can be called from user mode only.");
}
/* LCOV_EXCL_STOP */

void z_vrfy_z_log_hexdump_from_user(uint32_t src_level_val, const char *metadata,
				    const uint8_t *data, uint32_t len)
{
	union {
		struct log_msg_ids structure;
		uint32_t value;
	} src_level_union;
	int err;
	char kmeta[CONFIG_LOG_STRDUP_MAX_STRING];

	src_level_union.value = src_level_val;

	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		(src_level_union.structure.level <= LOG_LEVEL_DBG) &&
		(src_level_union.structure.level >= LOG_LEVEL_ERR),
		"Invalid log level"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		src_level_union.structure.domain_id == CONFIG_LOG_DOMAIN_ID,
		"Invalid log domain_id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		src_level_union.structure.source_id < z_log_sources_count(),
		"Invalid log source id"));

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) &&
	    (src_level_union.structure.level > LOG_FILTER_SLOT_GET(
	     z_log_dynamic_filters_get(src_level_union.structure.source_id),
	     LOG_FILTER_AGGR_SLOT_IDX))) {
		/* Skip filtered out messages. */
		return;
	}

	/*
	 * Validate and make a copy of the metadata string. Because we
	 * need the log subsystem to eventually free it, we're going
	 * to use log_strdup().
	 */
	err = z_user_string_copy(kmeta, metadata, sizeof(kmeta));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(err == 0, "invalid meta passed in"));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));

	if (IS_ENABLED(CONFIG_LOG1_IMMEDIATE)) {
		log_hexdump_sync(src_level_union.structure,
				 kmeta, data, len);
	} else {
		metadata = log_strdup(kmeta);
		log_hexdump(metadata, data, len, src_level_union.structure);
	}
}
#include <syscalls/z_log_hexdump_from_user_mrsh.c>

void log_hexdump_from_user(struct log_msg_ids src_level, const char *metadata,
			   const void *data, uint32_t len)
{
	union {
		struct log_msg_ids structure;
		uint32_t value;
	} src_level_union;

	__ASSERT_NO_MSG(sizeof(src_level) <= sizeof(uint32_t));
	src_level_union.structure = src_level;
	z_log_hexdump_from_user(src_level_union.value, metadata,
				(const uint8_t *)data, len);
}
#else
/* LCOV_EXCL_START */
void z_impl_z_log_string_from_user(uint32_t src_level_val, const char *str)
{
	ARG_UNUSED(src_level_val);
	ARG_UNUSED(str);

	__ASSERT_NO_MSG(false);
}

void z_vrfy_z_log_hexdump_from_user(uint32_t src_level_val, const char *metadata,
				    const uint8_t *data, uint32_t len)
{
	ARG_UNUSED(src_level_val);
	ARG_UNUSED(metadata);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	__ASSERT_NO_MSG(false);
}

void log_from_user(struct log_msg_ids src_level, const char *fmt, ...)
{
	ARG_UNUSED(src_level);
	ARG_UNUSED(fmt);

	__ASSERT_NO_MSG(false);
}

void log_generic_from_user(struct log_msg_ids src_level,
			   const char *fmt, va_list ap)
{
	ARG_UNUSED(src_level);
	ARG_UNUSED(fmt);
	ARG_UNUSED(ap);

	__ASSERT_NO_MSG(false);
}

void log_hexdump_from_user(struct log_msg_ids src_level, const char *metadata,
			   const void *data, uint32_t len)
{
	ARG_UNUSED(src_level);
	ARG_UNUSED(metadata);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	__ASSERT_NO_MSG(false);
}
/* LCOV_EXCL_STOP */
#endif /* !defined(CONFIG_USERSPACE) */

void z_log_msg2_init(void)
{
	mpsc_pbuf_init(&log_buffer, &mpsc_config);
}

struct log_msg2 *z_log_msg2_alloc(uint32_t wlen)
{
	return (struct log_msg2 *)mpsc_pbuf_alloc(&log_buffer, wlen,
				K_MSEC(CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS));
}

void z_log_msg2_commit(struct log_msg2 *msg)
{
	msg->hdr.timestamp = timestamp_func();

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		union log_msgs msgs = {
			.msg2 = (union log_msg2_generic *)msg
		};

		msg_process(msgs, false);

		return;
	}

	mpsc_pbuf_commit(&log_buffer, (union mpsc_pbuf_generic *)msg);
	z_log_msg_post_finalize();
}

union log_msg2_generic *z_log_msg2_claim(void)
{
	return (union log_msg2_generic *)mpsc_pbuf_claim(&log_buffer);
}

void z_log_msg2_free(union log_msg2_generic *msg)
{
	mpsc_pbuf_free(&log_buffer, (union mpsc_pbuf_generic *)msg);
}


bool z_log_msg2_pending(void)
{
	return mpsc_pbuf_is_pending(&log_buffer);
}

const char *z_log_get_tag(void)
{
	return CONFIG_LOG_TAG_MAX_LEN > 0 ? tag : NULL;
}

int log_set_tag(const char *str)
{
#if CONFIG_LOG_TAG_MAX_LEN > 0
	if (str == NULL) {
		return -EINVAL;
	}

	size_t len = strlen(str);
	size_t cpy_len = MIN(len, CONFIG_LOG_TAG_MAX_LEN);

	memcpy(tag, str, cpy_len);
	tag[cpy_len] = '\0';

	if (cpy_len < len) {
		tag[cpy_len - 1] = '~';
		return -ENOMEM;
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}

int log_mem_get_usage(uint32_t *buf_size, uint32_t *usage)
{
	__ASSERT_NO_MSG(buf_size != NULL);
	__ASSERT_NO_MSG(usage != NULL);

	if (!IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_LOG1)) {
		*buf_size = CONFIG_LOG_BUFFER_SIZE;
		*usage = log_msg_mem_get_used() * log_msg_get_slab_size();
		return 0;
	}

	mpsc_pbuf_get_utilization(&log_buffer, buf_size, usage);

	return 0;
}

int log_mem_get_max_usage(uint32_t *max)
{
	__ASSERT_NO_MSG(max != NULL);

	if (!IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_LOG1)) {
		*max = log_msg_mem_get_max_used() * log_msg_get_slab_size();
		return 0;
	}

	return mpsc_pbuf_get_max_utilization(&log_buffer, max);
}

static void log_process_thread_timer_expiry_fn(struct k_timer *timer)
{
	k_sem_give(&log_process_thread_sem);
}

static void log_process_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	__ASSERT_NO_MSG(log_backend_count_get() > 0);

	log_init();
	thread_set(k_current_get());

	while (true) {
		if (log_process(false) == false) {
			k_sem_take(&log_process_thread_sem, K_FOREVER);
		}
	}
}

K_KERNEL_STACK_DEFINE(logging_stack, CONFIG_LOG_PROCESS_THREAD_STACK_SIZE);
struct k_thread logging_thread;

static int enable_logger(const struct device *arg)
{
	ARG_UNUSED(arg);

	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		k_timer_init(&log_process_thread_timer,
				log_process_thread_timer_expiry_fn, NULL);
		/* start logging thread */
		k_thread_create(&logging_thread, logging_stack,
				K_KERNEL_STACK_SIZEOF(logging_stack),
				log_process_thread_func, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0,
				COND_CODE_1(CONFIG_LOG_PROCESS_THREAD,
					K_MSEC(CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS),
					K_NO_WAIT));
		k_thread_name_set(&logging_thread, "logging");
	} else {
		log_init();
	}

	return 0;
}

SYS_INIT(enable_logger, POST_KERNEL, 0);
