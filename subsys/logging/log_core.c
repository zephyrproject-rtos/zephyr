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
#include <sys/printk.h>
#include <init.h>
#include <sys/__assert.h>
#include <sys/atomic.h>
#include <ctype.h>
#include <logging/log_frontend.h>
#include <syscall_handler.h>

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
static uint32_t log_strdup_in_use;
static uint32_t log_strdup_max;
static uint32_t log_strdup_longest;
static struct k_timer log_process_thread_timer;

static uint32_t dummy_timestamp(void);
static timestamp_get_t timestamp_func = dummy_timestamp;


bool log_is_strdup(const void *buf);

static uint32_t dummy_timestamp(void)
{
	return 0;
}

uint32_t z_log_get_s_mask(const char *str, uint32_t nargs)
{
	char curr;
	bool arm = false;
	uint32_t arg = 0;
	uint32_t mask = 0;

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
		}
	}

	return mask;
}

/**
 * @brief Check if address is in read only section.
 *
 * @param addr Address.
 *
 * @return True if address identified within read only section.
 */
static bool is_rodata(const void *addr)
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARC) || defined(CONFIG_X86)
	extern const char *_image_rodata_start[];
	extern const char *_image_rodata_end[];
	#define RO_START _image_rodata_start
	#define RO_END _image_rodata_end
#elif defined(CONFIG_NIOS2) || defined(CONFIG_RISCV) || defined(CONFIG_SPARC)
	extern const char *_image_rom_start[];
	extern const char *_image_rom_end[];
	#define RO_START _image_rom_start
	#define RO_END _image_rom_end
#elif defined(CONFIG_XTENSA)
	extern const char *_rodata_start[];
	extern const char *_rodata_end[];
	#define RO_START _rodata_start
	#define RO_END _rodata_end
#else
	#define RO_START 0
	#define RO_END 0
#endif

	return (((const char *)addr >= (const char *)RO_START) &&
		((const char *)addr < (const char *)RO_END));
}

/**
 * @brief Scan string arguments and report every address which is not in read
 *	  only memory and not yet duplicated.
 *
 * @param msg Log message.
 */
static void detect_missed_strdup(struct log_msg *msg)
{
#define ERR_MSG	"argument %d in source %s log message \"%s\" missing" \
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
		if (!is_rodata(str) && !log_is_strdup(str) &&
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
	atomic_inc(&buffered_cnt);
	if (panic_mode) {
		unsigned int key = irq_lock();
		(void)log_process(false);
		irq_unlock(key);
	} else if (proc_tid != NULL && buffered_cnt == 1) {
		k_timer_start(&log_process_thread_timer,
			K_MSEC(CONFIG_LOG_PROCESS_THREAD_SLEEP_MS), K_NO_WAIT);
	} else if (CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) {
		if ((buffered_cnt == CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) &&
		    (proc_tid != NULL)) {
			k_timer_stop(&log_process_thread_timer);
			k_sem_give(&log_process_thread_sem);
		}
	} else {
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

void log_printk(const char *fmt, va_list ap)
{
	if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
		union {
			struct log_msg_ids structure;
			uint32_t value;
		} src_level_union = {
			{
				.level = LOG_LEVEL_INTERNAL_RAW_STRING
			}
		};

		if (_is_user_context()) {
			uint8_t str[CONFIG_LOG_PRINTK_MAX_STRING_LENGTH + 1];

			vsnprintk(str, sizeof(str), fmt, ap);

			z_log_string_from_user(src_level_union.value, str);
		} else if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
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
		}
		fmt++;
	}

	return args;
}

void log_generic(struct log_msg_ids src_level, const char *fmt, va_list ap,
		 enum log_strdup_action strdup_action)
{
	if (_is_user_context()) {
		log_generic_from_user(src_level, fmt, ap);
	} else if (IS_ENABLED(CONFIG_LOG_IMMEDIATE) &&
	    (!IS_ENABLED(CONFIG_LOG_FRONTEND))) {
		struct log_backend const *backend;
		uint32_t timestamp = timestamp_func();

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);

			if (log_backend_is_active(backend)) {
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

				/* is_rodata(str) is not checked,
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
		uint32_t timestamp = timestamp_func();

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);

			if (log_backend_is_active(backend)) {
				log_backend_put_sync_hexdump(
					backend, src_level, timestamp, metadata,
					(const uint8_t *)data, len);
			}
		}
	}
}

static uint32_t k_cycle_get_32_wrapper(void)
{
	/*
	 * The k_cycle_get_32() is a define which cannot be referenced
	 * by timestamp_func. Instead, this wrapper is used.
	 */
	return k_cycle_get_32();
}

void log_core_init(void)
{
	uint32_t freq;

	if (!IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		log_msg_pool_init();
		log_list_init(&list);

		k_mem_slab_init(&log_strdup_pool, log_strdup_pool_buf,
					sizeof(struct log_strdup_buf),
					CONFIG_LOG_STRDUP_BUF_COUNT);
	}

	/* Set default timestamp. */
	if (sys_clock_hw_cycles_per_sec() > 1000000) {
		timestamp_func = k_uptime_get_32;
		freq = 1000;
	} else {
		timestamp_func = k_cycle_get_32_wrapper;
		freq = sys_clock_hw_cycles_per_sec();
	}

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
			uint32_t *filters = log_dynamic_filters_get(i);
			uint8_t level = log_compiled_level_get(i);

			LOG_FILTER_SLOT_SET(filters,
					    LOG_FILTER_AGGR_SLOT_IDX,
					    level);
		}
	}
}

void log_init(void)
{
	__ASSERT_NO_MSG(log_backend_count_get() < LOG_FILTERS_NUM_OF_SLOTS);
	int i;

	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_init();
	}

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

int log_set_timestamp_func(timestamp_get_t timestamp_getter, uint32_t freq)
{
	if (!timestamp_getter) {
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

#ifdef CONFIG_USERSPACE
void z_vrfy_log_panic(void)
{
	z_impl_log_panic();
}
#include <syscalls/log_panic_mrsh.c>
#endif

static bool msg_filter_check(struct log_backend const *backend,
			     struct log_msg *msg)
{
	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		uint32_t backend_level;
		uint32_t msg_level;

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
		if (IS_ENABLED(CONFIG_LOG_DETECT_MISSED_STRDUP) &&
		    !panic_mode) {
			detect_missed_strdup(msg);
		}

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
	uint32_t dropped = atomic_set(&dropped_cnt, 0);

	for (int i = 0; i < log_backend_count_get(); i++) {
		struct log_backend const *backend = log_backend_get(i);

		if (log_backend_is_active(backend)) {
			log_backend_dropped(backend, dropped);
		}
	}
}

bool z_impl_log_process(bool bypass)
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

void log_dropped(void)
{
	atomic_inc(&dropped_cnt);
}

uint32_t log_src_cnt_get(uint32_t domain_id)
{
	return log_sources_count();
}

const char *log_source_name_get(uint32_t domain_id, uint32_t src_id)
{
	return src_id < log_sources_count() ? log_name_get(src_id) : NULL;
}

static uint32_t max_filter_get(uint32_t filters)
{
	uint32_t max_filter = LOG_LEVEL_NONE;
	int first_slot = LOG_FILTER_FIRST_BACKEND_SLOT_IDX;
	int i;

	for (i = first_slot; i < LOG_FILTERS_NUM_OF_SLOTS; i++) {
		uint32_t tmp_filter = LOG_FILTER_SLOT_GET(&filters, i);

		if (tmp_filter > max_filter) {
			max_filter = tmp_filter;
		}
	}

	return max_filter;
}

uint32_t z_impl_log_filter_set(struct log_backend const *const backend,
			    uint32_t domain_id,
			    uint32_t src_id,
			    uint32_t level)
{
	__ASSERT_NO_MSG(src_id < log_sources_count());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		uint32_t new_aggr_filter;

		uint32_t *filters = log_dynamic_filters_get(src_id);

		if (backend == NULL) {
			struct log_backend const *iter_backend;
			uint32_t max = 0U;
			uint32_t current;

			for (int i = 0; i < log_backend_count_get(); i++) {
				iter_backend = log_backend_get(i);
				current = log_filter_set(iter_backend,
							 domain_id,
							 src_id, level);
				max = MAX(current, max);
			}

			level = max;
		} else {
			uint32_t max = log_filter_get(backend, domain_id,
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

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_log_filter_set(struct log_backend const *const backend,
			    uint32_t domain_id,
			    uint32_t src_id,
			    uint32_t level)
{
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(backend == 0,
		"Setting per-backend filters from user mode is not supported"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(domain_id == CONFIG_LOG_DOMAIN_ID,
		"Invalid log domain_id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(src_id < log_sources_count(),
		"Invalid log source id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		(level <= LOG_LEVEL_DBG) && (level >= LOG_LEVEL_NONE),
		"Invalid log level"));

	return z_impl_log_filter_set(NULL, domain_id, src_id, level);
}
#include <syscalls/log_filter_set_mrsh.c>
#endif

static void backend_filter_set(struct log_backend const *const backend,
			       uint32_t level)
{
	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		for (int i = 0; i < log_sources_count(); i++) {
			log_filter_set(backend, CONFIG_LOG_DOMAIN_ID, i, level);
		}
	}
}

void log_backend_enable(struct log_backend const *const backend,
			void *ctx,
			uint32_t level)
{
	/* As first slot in filtering mask is reserved, backend ID has offset.*/
	uint32_t id = LOG_FILTER_FIRST_BACKEND_SLOT_IDX;

	id += backend - log_backend_get(0);

	log_backend_id_set(backend, id);
	backend_filter_set(backend, level);
	log_backend_activate(backend, ctx);

	/* Wakeup logger thread after attaching first backend. It might be
	 * blocked with log messages pending.
	 */
	if (!backend_attached) {
		k_sem_give(&log_process_thread_sem);
	}

	backend_attached = true;
}

void log_backend_disable(struct log_backend const *const backend)
{
	log_backend_deactivate(backend);
	backend_filter_set(backend, LOG_LEVEL_NONE);
}

uint32_t log_filter_get(struct log_backend const *const backend,
		     uint32_t domain_id,
		     uint32_t src_id,
		     bool runtime)
{
	__ASSERT_NO_MSG(src_id < log_sources_count());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) && runtime) {
		uint32_t *filters = log_dynamic_filters_get(src_id);

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

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE) ||
	    is_rodata(str) || _is_user_context()) {
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

void log_free(void *str)
{
	struct log_strdup_buf *dup = CONTAINER_OF(str, struct log_strdup_buf,
						  buf);

	if (atomic_dec(&dup->refcount) == 1) {
		k_mem_slab_free(&log_strdup_pool, (void **)&dup);
		if (IS_ENABLED(CONFIG_LOG_STRDUP_POOL_PROFILING)) {
			atomic_dec((atomic_t *)&log_strdup_in_use);
		}
	}
}

#if defined(CONFIG_USERSPACE)
void z_impl_z_log_string_from_user(uint32_t src_level_val, const char *str)
{
	ARG_UNUSED(src_level_val);
	ARG_UNUSED(str);

	__ASSERT(false, "This function can be called from user mode only.");
}

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
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(source_id < log_sources_count(),
		"Invalid log source id"));

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) &&
	    (level != LOG_LEVEL_INTERNAL_RAW_STRING) &&
	    (level > LOG_FILTER_SLOT_GET(log_dynamic_filters_get(source_id),
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

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
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

void z_impl_z_log_hexdump_from_user(uint32_t src_level_val, const char *metadata,
				    const uint8_t *data, uint32_t len)
{
	ARG_UNUSED(src_level_val);
	ARG_UNUSED(metadata);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	__ASSERT(false, "This function can be called from user mode only.");
}

void z_vrfy_z_log_hexdump_from_user(uint32_t src_level_val, const char *metadata,
				    const uint8_t *data, uint32_t len)
{
	union {
		struct log_msg_ids structure;
		uint32_t value;
	} src_level_union;
	size_t mlen;
	int err;

	src_level_union.value = src_level_val;

	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		(src_level_union.structure.level <= LOG_LEVEL_DBG) &&
		(src_level_union.structure.level >= LOG_LEVEL_ERR),
		"Invalid log level"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		src_level_union.structure.domain_id == CONFIG_LOG_DOMAIN_ID,
		"Invalid log domain_id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		src_level_union.structure.source_id < log_sources_count(),
		"Invalid log source id"));

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) &&
	    (src_level_union.structure.level > LOG_FILTER_SLOT_GET(
	     log_dynamic_filters_get(src_level_union.structure.source_id),
	     LOG_FILTER_AGGR_SLOT_IDX))) {
		/* Skip filtered out messages. */
		return;
	}

	/*
	 * Validate and make a copy of the metadata string. Because we
	 * need the log subsystem to eventually free it, we're going
	 * to use log_strdup().
	 */
	mlen = z_user_string_nlen(metadata, CONFIG_LOG_STRDUP_MAX_STRING, &err);
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(err == 0, "invalid string passed in"));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(metadata, mlen));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		log_hexdump_sync(src_level_union.structure,
				 metadata, data, len);
	} else {
		metadata = log_strdup(metadata);
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
#endif /* !defined(CONFIG_USERSPACE) */

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
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
		k_thread_name_set(&logging_thread, "logging");
	} else {
		log_init();
	}

	return 0;
}

SYS_INIT(enable_logger, POST_KERNEL, 0);
