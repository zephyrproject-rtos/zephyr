/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys_clock.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/iterable_sections.h>
#include <ctype.h>
#include <zephyr/logging/log_frontend.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/logging/log_output_custom.h>
#include <zephyr/linker/utils.h>

#ifdef CONFIG_LOG_TIMESTAMP_USE_REALTIME
#include <zephyr/posix/time.h>
#endif

LOG_MODULE_REGISTER(log);

#ifndef CONFIG_LOG_PROCESS_THREAD_SLEEP_MS
#define CONFIG_LOG_PROCESS_THREAD_SLEEP_MS 0
#endif

#ifndef CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD
#define CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD 0
#endif

#ifndef CONFIG_LOG_PROCESS_THREAD_STACK_SIZE
#define CONFIG_LOG_PROCESS_THREAD_STACK_SIZE 1
#endif

#ifndef CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS
#define CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS 0
#endif

#ifndef CONFIG_LOG_PROCESSING_LATENCY_US
#define CONFIG_LOG_PROCESSING_LATENCY_US 0
#endif

#ifndef CONFIG_LOG_BUFFER_SIZE
#define CONFIG_LOG_BUFFER_SIZE 4
#endif

#ifdef CONFIG_LOG_PROCESS_THREAD_CUSTOM_PRIORITY
#define LOG_PROCESS_THREAD_PRIORITY CONFIG_LOG_PROCESS_THREAD_PRIORITY
#else
#define LOG_PROCESS_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO
#endif

#ifndef CONFIG_LOG_TAG_MAX_LEN
#define CONFIG_LOG_TAG_MAX_LEN 0
#endif

#ifndef CONFIG_LOG_FAILURE_REPORT_PERIOD
#define CONFIG_LOG_FAILURE_REPORT_PERIOD 0
#endif

#ifndef CONFIG_LOG_ALWAYS_RUNTIME
BUILD_ASSERT(!IS_ENABLED(CONFIG_NO_OPTIMIZATIONS),
	     "CONFIG_LOG_ALWAYS_RUNTIME must be enabled when "
	     "CONFIG_NO_OPTIMIZATIONS is set");
BUILD_ASSERT(!IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE),
	     "CONFIG_LOG_ALWAYS_RUNTIME must be enabled when "
	     "CONFIG_LOG_MODE_IMMEDIATE is set");
#endif

static const log_format_func_t format_table[] = {
	[LOG_OUTPUT_TEXT] = IS_ENABLED(CONFIG_LOG_OUTPUT) ?
						log_output_msg_process : NULL,
	[LOG_OUTPUT_SYST] = IS_ENABLED(CONFIG_LOG_MIPI_SYST_ENABLE) ?
						log_output_msg_syst_process : NULL,
	[LOG_OUTPUT_DICT] = IS_ENABLED(CONFIG_LOG_DICTIONARY_SUPPORT) ?
						log_dict_output_msg_process : NULL,
	[LOG_OUTPUT_CUSTOM] = IS_ENABLED(CONFIG_LOG_CUSTOM_FORMAT_SUPPORT) ?
						log_custom_output_msg_process : NULL,
};

log_format_func_t log_format_func_t_get(uint32_t log_type)
{
	return format_table[log_type];
}

size_t log_format_table_size(void)
{
	return ARRAY_SIZE(format_table);
}

K_SEM_DEFINE(log_process_thread_sem, 0, 1);

static atomic_t initialized;
static bool panic_mode;
static bool backend_attached;
static atomic_t buffered_cnt;
static atomic_t dropped_cnt;
static k_tid_t proc_tid;
static struct k_timer log_process_thread_timer;

static log_timestamp_t dummy_timestamp(void);
static log_timestamp_get_t timestamp_func = dummy_timestamp;
static uint32_t timestamp_freq;
static log_timestamp_t proc_latency;
static log_timestamp_t prev_timestamp;
static atomic_t unordered_cnt;
static uint64_t last_failure_report;

static STRUCT_SECTION_ITERABLE(log_msg_ptr, log_msg_ptr);
static STRUCT_SECTION_ITERABLE_ALTERNATE(log_mpsc_pbuf, mpsc_pbuf_buffer, log_buffer);
static struct mpsc_pbuf_buffer *curr_log_buffer;

#ifdef CONFIG_MPSC_PBUF
static uint32_t __aligned(Z_LOG_MSG_ALIGNMENT)
	buf32[CONFIG_LOG_BUFFER_SIZE / sizeof(int)];

static void z_log_notify_drop(const struct mpsc_pbuf_buffer *buffer,
			      const union mpsc_pbuf_generic *item);

static const struct mpsc_pbuf_buffer_config mpsc_config = {
	.buf = (uint32_t *)buf32,
	.size = ARRAY_SIZE(buf32),
	.notify_drop = z_log_notify_drop,
	.get_wlen = log_msg_generic_get_wlen,
	.flags = (IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW) ?
		  MPSC_PBUF_MODE_OVERWRITE : 0) |
		 (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION) ?
		  MPSC_PBUF_MAX_UTILIZATION : 0)
};
#endif

/* Check that default tag can fit in tag buffer. */
COND_CODE_0(CONFIG_LOG_TAG_MAX_LEN, (),
	(BUILD_ASSERT(sizeof(CONFIG_LOG_TAG_DEFAULT) <= CONFIG_LOG_TAG_MAX_LEN + 1,
		      "Default string longer than tag capacity")));

static char tag[CONFIG_LOG_TAG_MAX_LEN + 1] =
	COND_CODE_0(CONFIG_LOG_TAG_MAX_LEN, ({}), (CONFIG_LOG_TAG_DEFAULT));

static void msg_process(union log_msg_generic *msg);

static log_timestamp_t dummy_timestamp(void)
{
	return 0;
}

log_timestamp_t z_log_timestamp(void)
{
	return timestamp_func();
}

static void z_log_msg_post_finalize(void)
{
	atomic_val_t cnt = atomic_inc(&buffered_cnt);

	if (panic_mode) {
		static struct k_spinlock process_lock;
		k_spinlock_key_t key = k_spin_lock(&process_lock);
		(void)log_process();

		k_spin_unlock(&process_lock, key);
	} else if (proc_tid != NULL) {
		/*
		 * If CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD == 1,
		 * timer is never needed. We release the processing
		 * thread after every message is posted.
		 */
		if (CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD == 1) {
			if (cnt == 0) {
				k_sem_give(&log_process_thread_sem);
			}
		} else {
			if (cnt == 0) {
				k_timer_start(&log_process_thread_timer,
					      K_MSEC(CONFIG_LOG_PROCESS_THREAD_SLEEP_MS),
					      K_NO_WAIT);
			} else if (CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD &&
				   (cnt + 1) == CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD) {
				k_timer_stop(&log_process_thread_timer);
				k_sem_give(&log_process_thread_sem);
			} else {
				/* No action needed. Message processing will be triggered by the
				 * timeout or when number of upcoming messages exceeds the
				 * threshold.
				 */
			}
		}
	}
}

const struct log_backend *log_format_set_all_active_backends(size_t log_type)
{
	const struct log_backend *failed_backend = NULL;

	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (log_backend_is_active(backend)) {
			int retCode = log_backend_format_set(backend, log_type);

			if (retCode != 0) {
				failed_backend = backend;
			}
		}
	}
	return failed_backend;
}

void z_log_vprintk(const char *fmt, va_list ap)
{
	if (!IS_ENABLED(CONFIG_LOG_PRINTK)) {
		return;
	}

	z_log_msg_runtime_vcreate(Z_LOG_LOCAL_DOMAIN_ID, NULL,
				   LOG_LEVEL_INTERNAL_RAW_STRING, NULL, 0,
				   Z_LOG_MSG_CBPRINTF_FLAGS(0),
				   fmt, ap);
}

#ifndef CONFIG_LOG_TIMESTAMP_USE_REALTIME
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
#else
static log_timestamp_t default_rt_get_timestamp(void)
{
	struct timespec tspec;

	clock_gettime(CLOCK_REALTIME, &tspec);

	return ((uint64_t)tspec.tv_sec * MSEC_PER_SEC) + (tspec.tv_nsec / NSEC_PER_MSEC);
}
#endif /* CONFIG_LOG_TIMESTAMP_USE_REALTIME */

void log_core_init(void)
{
	panic_mode = false;
	dropped_cnt = 0;
	buffered_cnt = 0;

	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_init();

		for (uint16_t s = 0; s < log_src_cnt_get(0); s++) {
			log_frontend_filter_set(s, CONFIG_LOG_MAX_LEVEL);
		}

		if (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY)) {
			return;
		}
	}

	/* Set default timestamp. */
#ifdef CONFIG_LOG_TIMESTAMP_USE_REALTIME
	log_set_timestamp_func(default_rt_get_timestamp, 1000U);
#else
	if (sys_clock_hw_cycles_per_sec() > 1000000) {
		log_set_timestamp_func(default_lf_get_timestamp, 1000U);
	} else {
		uint32_t freq = IS_ENABLED(CONFIG_LOG_TIMESTAMP_64BIT) ?
			CONFIG_SYS_CLOCK_TICKS_PER_SEC : sys_clock_hw_cycles_per_sec();
		log_set_timestamp_func(default_get_timestamp, freq);
	}
#endif /* CONFIG_LOG_TIMESTAMP_USE_REALTIME */

	if (IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		z_log_msg_init();
	}

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		z_log_runtime_filters_init();
	}
}

static uint32_t activate_foreach_backend(uint32_t mask)
{
	uint32_t mask_cpy = mask;

	while (mask_cpy) {
		uint32_t i = __builtin_ctz(mask_cpy);
		const struct log_backend *backend = log_backend_get(i);

		mask_cpy &= ~BIT(i);
		if (backend->autostart && (log_backend_is_ready(backend) == 0)) {
			mask &= ~BIT(i);
			log_backend_enable(backend,
					   backend->cb->ctx,
					   CONFIG_LOG_MAX_LEVEL);
		}
	}

	return mask;
}

static uint32_t z_log_init(bool blocking, bool can_sleep)
{
	uint32_t mask = 0;

	if (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY)) {
		return 0;
	}

	__ASSERT_NO_MSG(log_backend_count_get() < LOG_FILTERS_MAX_BACKENDS);

	if (atomic_inc(&initialized) != 0) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_LOG_MULTIDOMAIN)) {
		z_log_links_initiate();
	}

	int backend_index = 0;

	/* Activate autostart backends */
	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (backend->autostart) {
			log_backend_init(backend);

			/* If backend has activation function then backend is
			 * not ready until activated.
			 */
			if (log_backend_is_ready(backend) == 0) {
				log_backend_enable(backend,
						   backend->cb->ctx,
						   CONFIG_LOG_MAX_LEVEL);
			} else {
				mask |= BIT(backend_index);
			}
		}

		++backend_index;
	}

	/* If blocking init, wait until all backends are activated. */
	if (blocking) {
		while (mask) {
			mask = activate_foreach_backend(mask);
			if (IS_ENABLED(CONFIG_MULTITHREADING) && can_sleep) {
				k_msleep(10);
			}
		}
	}

	return mask;
}

void log_init(void)
{
	(void)z_log_init(true, true);
}

void log_thread_trigger(void)
{
	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		return;
	}

	k_timer_stop(&log_process_thread_timer);
	k_sem_give(&log_process_thread_sem);
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
	timestamp_freq = freq;
	if (CONFIG_LOG_PROCESSING_LATENCY_US) {
		proc_latency = (freq * CONFIG_LOG_PROCESSING_LATENCY_US) / 1000000;
	}

	if (IS_ENABLED(CONFIG_LOG_OUTPUT)) {
		log_output_timestamp_freq_set(freq);
	}

	return 0;
}

void z_impl_log_panic(void)
{
	if (panic_mode) {
		return;
	}

	/* If panic happened early logger might not be initialized.
	 * Forcing initialization of the logger and auto-starting backends.
	 */
	(void)z_log_init(true, false);

	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_panic();
		if (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY)) {
			goto out;
		}
	}

	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (log_backend_is_active(backend)) {
			log_backend_panic(backend);
		}
	}

	if (!IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		/* Flush */
		while (log_process() == true) {
		}
	}

out:
	panic_mode = true;
}

#ifdef CONFIG_USERSPACE
void z_vrfy_log_panic(void)
{
	z_impl_log_panic();
}
#include <zephyr/syscalls/log_panic_mrsh.c>
#endif

static bool msg_filter_check(struct log_backend const *backend,
			     union log_msg_generic *msg)
{
	if (!z_log_item_is_msg(msg)) {
		return true;
	}

	if (!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		return true;
	}

	uint32_t backend_level;
	uint8_t level;
	uint8_t domain_id;
	int16_t source_id;

	level = log_msg_get_level(&msg->log);
	domain_id = log_msg_get_domain(&msg->log);
	source_id = log_msg_get_source_id(&msg->log);

	/* Accept all non-logging messages. */
	if (level == LOG_LEVEL_NONE) {
		return true;
	}
	if (source_id >= 0) {
		backend_level = log_filter_get(backend, domain_id, source_id, true);

		return (level <= backend_level);
	} else {
		return true;
	}
}

static void msg_process(union log_msg_generic *msg)
{
	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (log_backend_is_active(backend) &&
		    msg_filter_check(backend, msg)) {
			log_backend_msg_process(backend, msg);
		}
	}
}

void dropped_notify(void)
{
	uint32_t dropped = z_log_dropped_read_and_clear();

	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (log_backend_is_active(backend)) {
			log_backend_dropped(backend, dropped);
		}
	}
}

void unordered_notify(void)
{
	uint32_t unordered = atomic_set(&unordered_cnt, 0);

	LOG_WRN("%d unordered messages since last report", unordered);
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

static inline bool z_log_unordered_pending(void)
{
	return IS_ENABLED(CONFIG_LOG_MULTIDOMAIN) && unordered_cnt;
}

bool z_impl_log_process(void)
{
	if (!IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		return false;
	}

	k_timeout_t backoff = K_NO_WAIT;
	union log_msg_generic *msg;

	if (!backend_attached) {
		return false;
	}

	msg = z_log_msg_claim(&backoff);

	if (msg) {
		atomic_dec(&buffered_cnt);
		msg_process(msg);
		z_log_msg_free(msg);
	} else if (CONFIG_LOG_PROCESSING_LATENCY_US > 0 && !K_TIMEOUT_EQ(backoff, K_NO_WAIT)) {
		/* If backoff is requested, it means that there are pending
		 * messages but they are too new and processing shall back off
		 * to allow arrival of newer messages from remote domains.
		 */
		k_timer_start(&log_process_thread_timer, backoff, K_NO_WAIT);

		return false;
	}

	if (IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		bool dropped_pend = z_log_dropped_pending();
		bool unordered_pend = z_log_unordered_pending();

		if ((dropped_pend || unordered_pend) &&
		   (k_uptime_get() - last_failure_report) > CONFIG_LOG_FAILURE_REPORT_PERIOD) {
			if (dropped_pend) {
				dropped_notify();
			}

			if (unordered_pend) {
				unordered_notify();
			}
		}

		last_failure_report += CONFIG_LOG_FAILURE_REPORT_PERIOD;
	}

	return z_log_msg_pending();
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_log_process(void)
{
	return z_impl_log_process();
}
#include <zephyr/syscalls/log_process_mrsh.c>
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
#include <zephyr/syscalls/log_buffered_cnt_mrsh.c>
#endif

void z_log_dropped(bool buffered)
{
	atomic_inc(&dropped_cnt);
	if (buffered) {
		atomic_dec(&buffered_cnt);
	}

	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		k_timer_stop(&log_process_thread_timer);
		k_sem_give(&log_process_thread_sem);
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

void z_log_msg_init(void)
{
#ifdef CONFIG_MPSC_PBUF
	mpsc_pbuf_init(&log_buffer, &mpsc_config);
	curr_log_buffer = &log_buffer;
#endif
}

static struct log_msg *msg_alloc(struct mpsc_pbuf_buffer *buffer, uint32_t wlen)
{
	if (!IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		return NULL;
	}

	return (struct log_msg *)mpsc_pbuf_alloc(
		buffer, wlen,
		(CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS == -1)
			? K_FOREVER
			: K_MSEC(CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS));
}

struct log_msg *z_log_msg_alloc(uint32_t wlen)
{
	return msg_alloc(&log_buffer, wlen);
}

static void msg_commit(struct mpsc_pbuf_buffer *buffer, struct log_msg *msg)
{
	union log_msg_generic *m = (union log_msg_generic *)msg;

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		msg_process(m);

		return;
	}

#ifdef CONFIG_MPSC_PBUF
	mpsc_pbuf_commit(buffer, &m->buf);
#endif
	z_log_msg_post_finalize();
}

void z_log_msg_commit(struct log_msg *msg)
{
	msg->hdr.timestamp = timestamp_func();
	msg_commit(&log_buffer, msg);
}

union log_msg_generic *z_log_msg_local_claim(void)
{
#ifdef CONFIG_MPSC_PBUF
	return (union log_msg_generic *)mpsc_pbuf_claim(&log_buffer);
#else
	return NULL;
#endif

}

/* If there are buffers dedicated for each link, claim the oldest message (lowest timestamp). */
union log_msg_generic *z_log_msg_claim_oldest(k_timeout_t *backoff)
{
	union log_msg_generic *msg = NULL;
	struct log_msg_ptr *chosen;
	log_timestamp_t t_min = sizeof(log_timestamp_t) > sizeof(uint32_t) ?
				UINT64_MAX : UINT32_MAX;
	int i = 0;

	/* Else iterate on all available buffers and get the oldest message. */
	STRUCT_SECTION_FOREACH(log_msg_ptr, msg_ptr) {
		struct log_mpsc_pbuf *buf;

		STRUCT_SECTION_GET(log_mpsc_pbuf, i, &buf);

#ifdef CONFIG_MPSC_PBUF
		if (msg_ptr->msg == NULL) {
			msg_ptr->msg = (union log_msg_generic *)mpsc_pbuf_claim(&buf->buf);
		}
#endif

		if (msg_ptr->msg) {
			log_timestamp_t t = log_msg_get_timestamp(&msg_ptr->msg->log);

			if (t < t_min) {
				t_min = t;
				msg = msg_ptr->msg;
				chosen = msg_ptr;
				curr_log_buffer = &buf->buf;
			}
		}
		i++;
	}

	if (msg) {
		if (CONFIG_LOG_PROCESSING_LATENCY_US > 0) {
			int32_t diff = t_min - (timestamp_func() - proc_latency);

			if (diff > 0) {
			       /* Entry is too new. Back off for sometime to allow new
				* remote messages to arrive which may have been captured
				* earlier (but on other platform). Calculate for how
				* long processing shall back off.
				*/
				if (timestamp_freq == sys_clock_hw_cycles_per_sec()) {
					*backoff = K_TICKS(diff);
				} else {
					*backoff = K_TICKS((diff * sys_clock_hw_cycles_per_sec()) /
							timestamp_freq);
				}

				return NULL;
			}
		}

		(*chosen).msg = NULL;
	}

	if (t_min < prev_timestamp) {
		atomic_inc(&unordered_cnt);
	}

	prev_timestamp = t_min;

	return msg;
}

union log_msg_generic *z_log_msg_claim(k_timeout_t *backoff)
{
	size_t len;

	STRUCT_SECTION_COUNT(log_mpsc_pbuf, &len);

	/* Use only one buffer if others are not registered. */
	if (IS_ENABLED(CONFIG_LOG_MULTIDOMAIN) && len > 1) {
		return z_log_msg_claim_oldest(backoff);
	}

	return z_log_msg_local_claim();
}

static void msg_free(struct mpsc_pbuf_buffer *buffer, const union log_msg_generic *msg)
{
#ifdef CONFIG_MPSC_PBUF
	mpsc_pbuf_free(buffer, &msg->buf);
#endif
}

void z_log_msg_free(union log_msg_generic *msg)
{
	msg_free(curr_log_buffer, msg);
}

static bool msg_pending(struct mpsc_pbuf_buffer *buffer)
{
#ifdef CONFIG_MPSC_PBUF
	return mpsc_pbuf_is_pending(buffer);
#else
	return false;
#endif
}

bool z_log_msg_pending(void)
{
	size_t len;
	int i = 0;

	STRUCT_SECTION_COUNT(log_mpsc_pbuf, &len);

	if (!IS_ENABLED(CONFIG_LOG_MULTIDOMAIN) || (len == 1)) {
		return msg_pending(&log_buffer);
	}

	STRUCT_SECTION_FOREACH(log_msg_ptr, msg_ptr) {
		struct log_mpsc_pbuf *buf;

		if (msg_ptr->msg) {
			return true;
		}

		STRUCT_SECTION_GET(log_mpsc_pbuf, i, &buf);

		if (msg_pending(&buf->buf)) {
			return true;
		}

		i++;
	}

	return false;
}

void z_log_msg_enqueue(const struct log_link *link, const void *data, size_t len)
{
	struct log_msg *log_msg = (struct log_msg *)data;
	size_t wlen = DIV_ROUND_UP(ROUND_UP(len, Z_LOG_MSG_ALIGNMENT), sizeof(int));
	struct mpsc_pbuf_buffer *mpsc_pbuffer = link->mpsc_pbuf ? link->mpsc_pbuf : &log_buffer;
	struct log_msg *local_msg = msg_alloc(mpsc_pbuffer, wlen);

	if (!local_msg) {
		z_log_dropped(false);
		return;
	}

	log_msg->hdr.desc.valid = 0;
	log_msg->hdr.desc.busy = 0;
	log_msg->hdr.desc.domain += link->ctrl_blk->domain_offset;
	memcpy((void *)local_msg, data, len);
	msg_commit(mpsc_pbuffer, local_msg);
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

	mpsc_pbuf_get_utilization(&log_buffer, buf_size, usage);

	return 0;
}

int log_mem_get_max_usage(uint32_t *max)
{
	__ASSERT_NO_MSG(max != NULL);

	if (!IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		return -EINVAL;
	}

	return mpsc_pbuf_get_max_utilization(&log_buffer, max);
}

static void log_backend_notify_all(enum log_backend_evt event,
				   union log_backend_evt_arg *arg)
{
	STRUCT_SECTION_FOREACH(log_backend, backend) {
		log_backend_notify(backend, event, arg);
	}
}

static void log_process_thread_timer_expiry_fn(struct k_timer *timer)
{
	k_sem_give(&log_process_thread_sem);
}

static void log_process_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	__ASSERT_NO_MSG(log_backend_count_get() > 0);
	uint32_t links_active_mask = 0xFFFFFFFF;
	uint8_t domain_offset = 0;
	uint32_t activate_mask = z_log_init(false, false);
	/* If some backends are not activated yet set periodical thread wake up
	 * to poll backends for readiness. Period is set arbitrary.
	 * If all backends are ready periodic wake up is not needed.
	 */
	k_timeout_t timeout = (activate_mask != 0) ? K_MSEC(50) : K_FOREVER;
	bool processed_any = false;
	thread_set(k_current_get());

	/* Logging thread is periodically waken up until all backends that
	 * should be autostarted are ready.
	 */
	while (true) {
		if (activate_mask) {
			activate_mask = activate_foreach_backend(activate_mask);
			if (!activate_mask) {
				/* Periodic wake up no longer needed since all
				 * backends are ready.
				 */
				timeout = K_FOREVER;
			}
		}

		/* Keep trying to activate links until all links are active. */
		if (IS_ENABLED(CONFIG_LOG_MULTIDOMAIN) && links_active_mask) {
			links_active_mask =
				z_log_links_activate(links_active_mask, &domain_offset);
		}


		if (log_process() == false) {
			if (processed_any) {
				processed_any = false;
				log_backend_notify_all(LOG_BACKEND_EVT_PROCESS_THREAD_DONE, NULL);
			}
			(void)k_sem_take(&log_process_thread_sem, timeout);
		} else {
			processed_any = true;
		}
	}
}

K_KERNEL_STACK_DEFINE(logging_stack, CONFIG_LOG_PROCESS_THREAD_STACK_SIZE);
struct k_thread logging_thread;

static int enable_logger(void)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		k_timer_init(&log_process_thread_timer,
				log_process_thread_timer_expiry_fn, NULL);
		/* start logging thread */
		k_thread_create(&logging_thread, logging_stack,
				K_KERNEL_STACK_SIZEOF(logging_stack),
				log_process_thread_func, NULL, NULL, NULL,
				LOG_PROCESS_THREAD_PRIORITY, 0,
				COND_CODE_1(CONFIG_LOG_PROCESS_THREAD,
					K_MSEC(CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS),
					K_NO_WAIT));
		k_thread_name_set(&logging_thread, "logging");
	} else {
		(void)z_log_init(false, false);
	}

	return 0;
}

SYS_INIT(enable_logger, POST_KERNEL, CONFIG_LOG_CORE_INIT_PRIORITY);
