/*
 * Copyright (c) 2019,2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend_adsp_mtrace.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <adsp_memory.h>
#include <adsp_debug_window.h>

/*
 * A lock is needed as log_process() and log_panic() have no internal locks
 * to prevent concurrency. Meaning if log_process is called after
 * log_panic was called previously, log_process may happen from another
 * CPU and calling context than the log processing thread running in
 * the background. On an SMP system this is a race.
 *
 * This caused a race on the output trace such that the logging output
 * was garbled and useless.
 */
static struct k_spinlock mtrace_lock;

static uint32_t log_format_current = CONFIG_LOG_BACKEND_ADSP_MTRACE_OUTPUT_DEFAULT;

static adsp_mtrace_log_hook_t mtrace_hook;

static bool mtrace_active;

static bool mtrace_panic_mode;

/*
 * SRAM window for debug info is organized to equal size slots.
 * The descriptors on first page describe the layout of slots.
 * One type of debug info slot is ADSP_DBG_WIN_SLOT_DEBUG_LOG.
 * These slots (if defined) can be used for mtrace output.
 *
 * The log buffer slots have the following layout:
 *
 * u32 host_read_ptr;
 * u32 dsp_write_ptr;
 * u8 buffer[];
 *
 * The two pointers are offsets within the buffer. Buffer is empty
 * when the two pointers are equal, and full when host read pointer
 * is one ahead of the DSP writer pointer.
 */

#define MTRACE_LOG_BUF_SIZE		(ADSP_DW_SLOT_SIZE - 2 * sizeof(uint32_t))

#define MTRACE_LOGGING_SLOT_TYPE(n)	(ADSP_DW_SLOT_DEBUG_LOG | ((n) & ADSP_DW_SLOT_CORE_MASK))

#define MTRACE_CORE		0

struct adsp_debug_slot {
	uint32_t host_ptr;
	uint32_t dsp_ptr;
	uint8_t data[ADSP_DW_SLOT_SIZE - sizeof(uint32_t) * 2];
} __packed;

#ifdef CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER
#ifdef CONFIG_LOG_MULTI_SLOT
static struct adsp_debug_slot *slots[CONFIG_MP_MAX_NUM_CPUS];
#else
static struct adsp_debug_slot *slots[1];
#endif
#endif

static void mtrace_init(void)
{
#ifdef CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER
#ifdef CONFIG_LOG_MULTI_SLOT
	for (unsigned int core = 0; core < CONFIG_MP_MAX_NUM_CPUS; core++) {
		if (!slots[core]) {
			struct adsp_dw_desc desc = {
				.type = MTRACE_LOGGING_SLOT_TYPE(core),
			};

			slots[core] = adsp_dw_request_slot(&desc, NULL);
			if (slots[core]) {
				slots[core]->host_ptr = 0;
				slots[core]->dsp_ptr = 0;
			}
		}
	}
#else
	struct adsp_dw_desc slot_desc = { .type = MTRACE_LOGGING_SLOT_TYPE(MTRACE_CORE), };

	if (slots[0]) {
		return;
	}

	slots[0] = adsp_dw_request_slot(&slot_desc, NULL);
	if (slots[0]) {
		slots[0]->host_ptr = 0;
		slots[0]->dsp_ptr = 0;
	}
#endif
#else
	struct adsp_debug_slot *slot = (struct adsp_debug_slot *)
		ADSP_DW->slots[ADSP_DW_SLOT_NUM_MTRACE];

	if (ADSP_DW->descs[ADSP_DW_SLOT_NUM_MTRACE].type == MTRACE_LOGGING_SLOT_TYPE(MTRACE_CORE)) {
		return;
	}

	ADSP_DW->descs[ADSP_DW_SLOT_NUM_MTRACE].type = MTRACE_LOGGING_SLOT_TYPE(MTRACE_CORE);
	slot->host_ptr = 0;
	slot->dsp_ptr = 0;
#endif
}

static size_t mtrace_out(int8_t *str, size_t len, size_t *space_left,
			 unsigned int core)
{
#ifdef CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER
	struct adsp_debug_slot *slot = slots[core];

	/* Debug slot is not allocated */
	if (!slot) {
		return 0;
	}
#else
	struct adsp_debug_slot *slot = (struct adsp_debug_slot *)
		ADSP_DW->slots[ADSP_DW_SLOT_NUM_MTRACE];
#endif

	uint8_t *data = slot->data;
	uint32_t r = slot->host_ptr;
	uint32_t w = slot->dsp_ptr;
	size_t avail, out;

	if (w > r) {
		avail = MTRACE_LOG_BUF_SIZE - w + r - 1;
	} else if (w == r) {
		avail = MTRACE_LOG_BUF_SIZE - 1;
	} else {
		avail = r - w - 1;
	}

	if (len == 0) {
		out = 0;
		goto out;
	}

	/* data that does not fit is dropped */
	out = MIN(avail, len);

	if (w + out >= MTRACE_LOG_BUF_SIZE) {
		size_t tail = MTRACE_LOG_BUF_SIZE - w;
		size_t head = out - tail;

		memcpy(data + w, str, tail);
		memcpy(data, str + tail, head);
		w = head;
	} else {
		memcpy(data + w, str, out);
		w += out;
	}

	slot->dsp_ptr = w;

out:
	if (space_left) {
		*space_left = avail > len ? avail - len : 0;
	}

	return out;
}

#if defined(CONFIG_LOG_MODE_DEFERRED) && \
	(CONFIG_MTRACE_LOG_CACHE_BUF_SIZE > ADSP_DW_SLOT_SIZE)
#define MTRACE_LOG_CACHE_ENABLE
#endif

#ifdef MTRACE_LOG_CACHE_ENABLE
/*
 * The cache buffer absorbs bursts the debug window slot cannot take because
 * the host has not consumed it yet. Data moves from the cache into the slot
 * only once the host has made room, which it does independently of DSP log
 * activity. Draining solely from char_out() would leave the tail of a burst in
 * the cache forever, hence the periodic drain below.
 */
#ifdef CONFIG_LOG_MULTI_SLOT
#define MTRACE_CACHE_COUNT	CONFIG_MP_MAX_NUM_CPUS
#else
#define MTRACE_CACHE_COUNT	1
#endif

struct mtrace_cache {
	struct k_spinlock lock;
	size_t head;
	size_t len;
	uint8_t buf[CONFIG_MTRACE_LOG_CACHE_BUF_SIZE];
};

static struct mtrace_cache mtrace_caches[MTRACE_CACHE_COUNT];
static bool mtrace_drain_armed;

/*
 * Retry delay of the drain work. It doubles on every retry the host has not
 * made room for, up to MTRACE_LOG_CACHE_DRAIN_MAX_PERIOD_MS, and returns to the default
 * as soon as the host consumes data.
 */
#define MTRACE_LOG_CACHE_DRAIN_PERIOD_MS		1
#define MTRACE_LOG_CACHE_DRAIN_MAX_PERIOD_MS	256
static uint32_t mtrace_drain_delay_ms = MTRACE_LOG_CACHE_DRAIN_PERIOD_MS;

static void mtrace_drain_handler(struct k_work *work);
static void mtrace_drain_schedule(void);

static K_WORK_DELAYABLE_DEFINE(mtrace_drain_work, mtrace_drain_handler);

static struct mtrace_cache *mtrace_cache_get(unsigned int core)
{
	if (core >= MTRACE_CACHE_COUNT) {
		core = 0;
	}

	return &mtrace_caches[core];
}

/* Discards the oldest @p count bytes. Caller holds the cache lock. */
static void mtrace_cache_drop(struct mtrace_cache *cache, size_t count)
{
	cache->head = (cache->head + count) % CONFIG_MTRACE_LOG_CACHE_BUF_SIZE;
	cache->len -= count;
}

/*
 * Moves cached bytes into the slot. Bytes the slot cannot take stay cached and
 * the drain is retried later. When @p force is set only the latest bytes that
 * fit in the slot are written and everything older is dropped.
 */
static bool mtrace_cache_drain(unsigned int core, bool force, bool *progress)
{
	struct mtrace_cache *cache = mtrace_cache_get(core);
	size_t space_left = 0;
	size_t written = 0;
	bool blocked = false;
	k_spinlock_key_t key;

	key = k_spin_lock(&cache->lock);

	/* A zero length write only reports the free space of the slot. */
	(void)mtrace_out(NULL, 0, &space_left, core);

	if (force && cache->len > space_left) {
		/* Keep the latest bytes: discard the oldest ones that do not fit. */
		mtrace_cache_drop(cache, cache->len - space_left);
	}

	if (cache->len > 0) {
		size_t len = MIN(cache->len, space_left);
		size_t first = MIN(len, CONFIG_MTRACE_LOG_CACHE_BUF_SIZE - cache->head);

		written = mtrace_out((int8_t *)&cache->buf[cache->head], first, &space_left,
				     core);
		if (written == first && first < len) {
			written += mtrace_out((int8_t *)cache->buf, len - first, &space_left,
					      core);
		}

		cache->head = (cache->head + written) % CONFIG_MTRACE_LOG_CACHE_BUF_SIZE;
		cache->len -= written;
	}

	if (cache->len > 0) {
		blocked = true;
		space_left = 0;
	}

	k_spin_unlock(&cache->lock, key);

	if (mtrace_hook != NULL && (written > 0 || blocked || force)) {
		mtrace_hook(written, mtrace_panic_mode ? 0 : space_left);
	}

	if (progress != NULL) {
		*progress = written > 0;
	}

	return blocked;
}

/* Stores @p data in the cache, silently dropping the oldest bytes if it is full. */
static void mtrace_cache_out(const uint8_t *data, size_t length, unsigned int core)
{
	struct mtrace_cache *cache = mtrace_cache_get(core);
	k_spinlock_key_t key = k_spin_lock(&cache->lock);
	size_t room;
	size_t w;
	size_t first;
	bool progress;
	bool blocked;

	if (length > CONFIG_MTRACE_LOG_CACHE_BUF_SIZE) {
		data += length - CONFIG_MTRACE_LOG_CACHE_BUF_SIZE;
		length = CONFIG_MTRACE_LOG_CACHE_BUF_SIZE;
	}

	room = CONFIG_MTRACE_LOG_CACHE_BUF_SIZE - cache->len;
	if (length > room) {
		mtrace_cache_drop(cache, length - room);
	}

	w = (cache->head + cache->len) % CONFIG_MTRACE_LOG_CACHE_BUF_SIZE;
	first = MIN(length, CONFIG_MTRACE_LOG_CACHE_BUF_SIZE - w);

	memcpy(&cache->buf[w], data, first);
	memcpy(&cache->buf[0], data + first, length - first);
	cache->len += length;

	k_spin_unlock(&cache->lock, key);

	/* In panic mode there is no deferred drain: write what fits, drop the rest. */
	blocked = mtrace_cache_drain(core, mtrace_panic_mode, &progress);
	if (progress || !blocked) {
		mtrace_drain_delay_ms = MTRACE_LOG_CACHE_DRAIN_PERIOD_MS;
	}

	if (blocked) {
		mtrace_drain_schedule();
	}
}

static void mtrace_drain_schedule(void)
{
	/* A retry already waiting must not be pushed back by every log write. */
	if (!mtrace_drain_armed || k_is_pre_kernel() ||
	    (k_work_delayable_busy_get(&mtrace_drain_work) &
	     (K_WORK_DELAYED | K_WORK_QUEUED)) != 0) {
		return;
	}

	(void)k_work_reschedule(&mtrace_drain_work,
				K_MSEC(mtrace_drain_delay_ms));
}

/* Periodic drain, moves cached bytes to the slots without waiting for new logs. */
static void mtrace_drain_handler(struct k_work *work)
{
	bool blocked = false;
	bool progress = false;

	ARG_UNUSED(work);

	for (unsigned int core = 0; core < MTRACE_CACHE_COUNT; core++) {
		bool core_progress;

		blocked |= mtrace_cache_drain(core, false, &core_progress);
		progress |= core_progress;
	}

	if (progress || !blocked) {
		mtrace_drain_delay_ms = MTRACE_LOG_CACHE_DRAIN_PERIOD_MS;
	} else {
		mtrace_drain_delay_ms = MIN(mtrace_drain_delay_ms * 2U,
					    MTRACE_LOG_CACHE_DRAIN_MAX_PERIOD_MS);
	}

	if (blocked) {
		mtrace_drain_schedule();
	}
}
#endif /* MTRACE_LOG_CACHE_ENABLE */

static int char_out(uint8_t *data, size_t length, void *ctx)
{
#ifdef CONFIG_LOG_MULTI_SLOT
	unsigned int core = (unsigned int)(uintptr_t)ctx;
#else
	unsigned int core = 0;
#endif

	/*
	 * we handle the data even if mtrace notifier is not
	 * active. this ensures we can capture early boot messages.
	 */
#ifdef MTRACE_LOG_CACHE_ENABLE
	/* The cache notifies the host itself once it has moved data to the slot. */
	mtrace_cache_out(data, length, core);
#else
	size_t space_left = 0;
	size_t out;

	out = mtrace_out(data, length, &space_left, core);

	if (mtrace_active && mtrace_hook) {

		/* if we are in panic mode, need to flush out asap */
		if (unlikely(mtrace_panic_mode)) {
			space_left = 0;
		}

		mtrace_hook(out, space_left);
	}
#endif

	return length;
}

/**
 * 80 bytes seems to catch most sensibly sized log message lines
 * in one go letting the trace out call output whole complete
 * lines. This avoids the overhead of a spin lock in the trace_out
 * more often as well as avoiding entwined characters from printk if
 * LOG_PRINTK=n.
 */
#define LOG_BUF_SIZE 80
static uint8_t log_buf[LOG_BUF_SIZE];

LOG_OUTPUT_DEFINE(log_output_adsp_mtrace, char_out, log_buf, sizeof(log_buf));

static uint32_t format_flags(void)
{
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	if (IS_ENABLED(CONFIG_LOG_CORE_ID_PREFIX)) {
		flags |= LOG_OUTPUT_FLAG_CORE;
	}

	if (IS_ENABLED(CONFIG_LOG_THREAD_ID_PREFIX)) {
		flags |= LOG_OUTPUT_FLAG_THREAD;
	}

	return flags;
}

static void panic(struct log_backend const *const backend)
{
	mtrace_panic_mode = true;

#ifdef MTRACE_LOG_CACHE_ENABLE
	/* Push whatever is cached to the slot while the host can still read it. */
	for (unsigned int core = 0; core < MTRACE_CACHE_COUNT; core++) {
		(void)mtrace_cache_drain(core, true, NULL);
	}
#endif
}

static void dropped(const struct log_backend *const backend,
		    uint32_t cnt)
{
	log_output_dropped_process(&log_output_adsp_mtrace, cnt);
}

static void process(const struct log_backend *const backend,
		    union log_msg_generic *msg)
{
	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	k_spinlock_key_t key = k_spin_lock(&mtrace_lock);

#ifdef CONFIG_LOG_MULTI_SLOT
	log_output_ctx_set(&log_output_adsp_mtrace,
			   (void *)(uintptr_t)log_msg_get_core_id(&msg->log));
#endif
	log_output_func(&log_output_adsp_mtrace, &msg->log, format_flags());

	k_spin_unlock(&mtrace_lock, key);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

/**
 * Lazily initialized, while the DMA may not be setup we continue
 * to buffer log messages untilt he buffer is full.
 */
static void init(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);

	mtrace_init();

#ifdef MTRACE_LOG_CACHE_ENABLE
	/* The drain work is statically defined; only enable scheduling now that
	 * the kernel is up and the slot is known.
	 */
	mtrace_drain_armed = true;
#endif
}

const struct log_backend_api log_backend_adsp_mtrace_api = {
	.process = process,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.panic = panic,
	.format_set = format_set,
	.init = init,
};

LOG_BACKEND_DEFINE(log_backend_adsp_mtrace, log_backend_adsp_mtrace_api, true);

void adsp_mtrace_log_init(adsp_mtrace_log_hook_t hook)
{
	mtrace_init();

#ifdef MTRACE_LOG_CACHE_ENABLE
	if (hook == NULL) {
		/* Nothing will drain the cache once the host stops reading it. */
		mtrace_drain_armed = false;
		(void)k_work_cancel_delayable(&mtrace_drain_work);
	} else {
		mtrace_drain_armed = true;
	}
#endif

	mtrace_hook = hook;
	mtrace_active = !!hook;
}

const struct log_backend *log_backend_adsp_mtrace_get(void)
{
	return &log_backend_adsp_mtrace;
}
