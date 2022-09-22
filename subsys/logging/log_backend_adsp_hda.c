/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/spinlock.h"
#include "zephyr/sys/atomic.h"
#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_backend_adsp_hda.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>

/**
 * @brief HDA Log Backend
 *
 * The HDA log backend works as a pipeline of tasks. Each task may have any
 * single thread acting in it. To deal with formatting multiple writers first
 * format into local buffers and then copy those buffers into the dma ring in a
 * lock.
 *
 * 1. Messages are formatted into a format buffer. Threads do this into one of a
 *    pool of outputs. ISRs, panics, and drops use a shared output with a lock.
 * 2. The format buffer is copied into the dma ring buffer in log_out_lock.
 * 3. The DMA controller is notified of bytes written to the dma ring buffer
 *    in log_dma_lock.
 * 4. A hook is called to notify the other end of the line of available bytes
 *    in log_hook_lock.
 * 5. When hook is done, those bytes become available again
 *    in the DMA buffer.
 *
 * Not every log flush results in a DMA flush and Hook call,
 * as those require hardware to respond and that may take some time.
 *
 * Instead things are batched up until a watermark is met or maximum
 * latency threshold has been met. There is a cap on how often the hook
 * is called. This is because a Hook (IPC) called too often appears to cause
 * an unrecoverable hardware fault where the IPC registers no longer update.
 */

static uint32_t log_format_current = CONFIG_LOG_BACKEND_ADSP_HDA_OUTPUT_DEFAULT;
static const struct device *const hda_log_dev =
	DEVICE_DT_GET(DT_NODELABEL(hda_host_in));
static uint32_t hda_log_chan;

/* Easily enable/disable of the timer */
#define HDA_LOG_PERIODIC 1

/*
 * HDA requires 128 byte aligned data and 128 byte aligned transfers.
 */
#define ALIGNMENT DMA_BUF_ALIGNMENT(DT_NODELABEL(hda_host_in))
static __incoherent __aligned(ALIGNMENT) uint8_t hda_log_buf[CONFIG_LOG_BACKEND_ADSP_HDA_SIZE];
#ifdef HDA_LOG_PERIODIC
static struct k_timer hda_log_timer;
#endif
static adsp_hda_log_hook_t hook;

/* Pipeline locks */
static struct k_spinlock hda_log_fmt_lock;
static struct k_spinlock hda_log_out_lock;
static struct k_spinlock hda_log_dma_lock;
static struct k_spinlock hda_log_hook_lock;

/* Simple power of 2 wrapping mask */
#define HDA_LOG_BUF_MASK (CONFIG_LOG_BACKEND_ADSP_HDA_SIZE - 1)

/*  75% buffer fill max */
#define HDA_LOG_LOWMARK (CONFIG_LOG_BACKEND_ADSP_HDA_SIZE/4)

/* atomic bit flags for state */
#define HDA_LOG_DMA_READY 0
#define HDA_LOG_PANIC_MODE 1
static atomic_t hda_log_flags;

/* A forever increasing write_idx, should be masked
 * with HDA_LOG_BUF_MASK when accessing hda_log_buf
 */
static atomic_t write_idx = ATOMIC_INIT(0);

/* Available bytes that are not pending anything in the pipe */
static atomic_t available_bytes = ATOMIC_INIT(CONFIG_LOG_BACKEND_ADSP_HDA_SIZE);

/* Bytes pending DMA flush */
static atomic_t pending_dma_bytes = ATOMIC_INIT(0);

/* Bytes pending Hook call */
static atomic_t pending_hook_bytes = ATOMIC_INIT(0);

/* Last time a Hook occurred */
static int64_t notify_last_sent;

/* Minimum amount of time between dma + hook flushing calls
 *
 * While using DMA doesn't seem to cause any problems IPC tends
 * to fail in unusual ways.
 *
 * 1. IPCs don't seem to arrive while interrupts are locked.
 * 2. IPCs sent too fast seem to cause the IPC register to
 *    inexplicable fail to update ever again.
 *
 * So set a speed limit on flush+notify
 */
#define NOTIFY_MIN_DELAY 100

/**
 * @brief When padding is enabled insert '\0' characters to align the buffer up
 */
static uint32_t hda_log_pad(void)
{
	uint32_t padding;

#ifdef CONFIG_LOG_BACKEND_ADSP_HDA_PADDING
	uint32_t idx = atomic_get(&write_idx);
	uint32_t available = atomic_get(&available_bytes);

	uint32_t next128;
	uint32_t nearest128 = idx & ~((128) - 1);

	/* No need to pad if we are already 128 byte aligned
	 * or the buffer is empty
	 */
	if (nearest128 == idx || available == sizeof(hda_log_buf)) {
		return 0;
	}

	next128 = nearest128 + 128;
	padding = next128 - idx;

	/* In order to pad the buffer we need to know there's free space */
	__ASSERT(available >= padding,
		"Expected to always have bytes to pad up to the next 128");


	for (int i = 0; i < padding; i++) {
		hda_log_buf[idx & HDA_LOG_BUF_MASK] = '\0';
		idx++;
	}

	atomic_sub(&available_bytes, padding);
	atomic_add(&pending_dma_bytes, padding);
	atomic_set(&write_idx, idx);

	nearest128 = idx & ~((128) - 1);
	__ASSERT(idx == nearest128,
		"Expected after padding pending bytes to be 128 byte aligned");
#endif /* CONFIG_LOG_BACKEND_ADSP_HDA_PADDING */

	return padding;
}

/**
 * @brief Write pending_dma_bytes to the HDA stream
 */
static void hda_log_dma_flush(void)
{
	uint32_t nearest128;
	struct dma_status status;

	if (!atomic_test_bit(&hda_log_flags, HDA_LOG_DMA_READY)) {
		return;
	}

	uint32_t pending_bytes = atomic_get(&pending_dma_bytes);

	/* Nothing to do */
	if (pending_bytes == 0) {
		return;
	}

	nearest128 = pending_bytes & ~((128) - 1);

	/* Nothing to do */
	if (nearest128 == 0) {
		return;
	}

	z_xtensa_cache_flush(&hda_log_buf[0], CONFIG_LOG_BACKEND_ADSP_HDA_SIZE);

	dma_reload(hda_log_dev, hda_log_chan, 0, 0, nearest128);

	/* Wait for the DMA hardware to catch up */
	do {
		__ASSERT(dma_get_status(hda_log_dev, hda_log_chan, &status) == 0,
			"DMA get status failed unexpectedly");

	} while (status.read_position != status.write_position);


	/* The bytes now move to the next step in the pipe */
	atomic_sub(&pending_dma_bytes, nearest128);
	atomic_add(&pending_hook_bytes, nearest128);
}

/**
 * @brief Possibly write the pending_hook_bytes to the hook
 */
static void hda_log_hook(void)
{
	uint32_t pending_bytes = atomic_get(&pending_hook_bytes);

	int64_t delta = k_uptime_get() - notify_last_sent;

	if (hook == NULL || pending_bytes == 0 || delta < NOTIFY_MIN_DELAY) {
		return;
	}

	/* Hook must only return true once bytes are done being dealt with */
	bool notified = hook(pending_bytes);

	if (notified) {
		notify_last_sent = k_uptime_get();
		atomic_sub(&pending_hook_bytes, pending_bytes);
		atomic_add(&available_bytes, pending_bytes);
	}
}

static int hda_log_out(uint8_t *data, size_t length)
{
	k_spinlock_key_t out_key = k_spin_lock(&hda_log_out_lock);

	uint32_t available = atomic_get(&available_bytes);
	uint32_t start_idx = atomic_get(&write_idx);
	uint32_t idx = start_idx;
	size_t write_len = length < available ? length : available;

	/* Copy over the formatted message to the dma buffer */
	for (uint32_t i = 0; i < write_len; i++) {
		hda_log_buf[idx & HDA_LOG_BUF_MASK] = data[i];
		idx++;
	}

	atomic_set(&write_idx, idx);
	atomic_sub(&available_bytes, write_len);
	atomic_add(&pending_dma_bytes, write_len);

	bool panic_mode_set = atomic_test_bit(&hda_log_flags, HDA_LOG_PANIC_MODE);
	int64_t delta = k_uptime_get() - notify_last_sent;

	/* If we've hit the low watermark or are in panic mode push data out now */
	if (atomic_get(&available_bytes) < HDA_LOG_LOWMARK
	    || panic_mode_set
	    || delta > NOTIFY_MIN_DELAY) {
		hda_log_pad();

		k_spin_unlock(&hda_log_out_lock, out_key);

		k_spinlock_key_t dma_key = k_spin_lock(&hda_log_dma_lock);

		if (atomic_test_bit(&hda_log_flags, HDA_LOG_DMA_READY)) {
			hda_log_dma_flush();
		}

		k_spin_unlock(&hda_log_dma_lock, dma_key);

		k_spinlock_key_t hook_key = k_spin_lock(&hda_log_hook_lock);

		hda_log_hook();

		k_spin_unlock(&hda_log_hook_lock, hook_key);
	} else {
		k_spin_unlock(&hda_log_out_lock, out_key);
	}

	return write_len;
}

#ifdef HDA_LOG_PERIODIC
static void hda_log_periodic(struct k_timer *tm)
{
	ARG_UNUSED(tm);

	int64_t delta = k_uptime_get() - notify_last_sent;

	if (delta < NOTIFY_MIN_DELAY) {
		return;
	}

	k_spinlock_key_t out_key = k_spin_lock(&hda_log_out_lock);

	hda_log_pad();

	k_spin_unlock(&hda_log_out_lock, out_key);

	k_spinlock_key_t dma_key = k_spin_lock(&hda_log_dma_lock);

	hda_log_dma_flush();

	k_spin_unlock(&hda_log_dma_lock, dma_key);

	k_spinlock_key_t hook_key = k_spin_lock(&hda_log_hook_lock);

	hda_log_hook();

	k_spin_unlock(&hda_log_hook_lock, hook_key);
}
#endif

#ifdef CONFIG_LOG_MODE_IMMEDIATE

/*
 * A small formatting buffer to avoid constantly contending on the
 * spin lock for output. About a line of text worth, and a single transfer
 * for HDA.
 */
#define OUTPUT_BUF_SIZE 128
#else /* CONFIG_LOG_MODE_IMMEDIATE */

/*
 * Deferred mode wants to buffer more and flush later otherwise it drops
 * messages.
 *
 * So take what we would've used for N outputs and use it for one
 */
#define OUTPUT_BUF_SIZE 512
#endif /* CONFIG_LOG_MODE_IMMEDIATE */

#ifdef CONFIG_LOG_MODE_IMMEDIATE
#define OUTPUTS 5
#else
#define OUTPUTS 2
#endif

K_SEM_DEFINE(outputs_free, OUTPUTS-1, OUTPUTS-1);
static atomic_t output_used = ATOMIC_INIT(0);
static uint32_t output_idx[OUTPUTS];
static uint8_t output_buf[OUTPUTS][OUTPUT_BUF_SIZE];
struct log_output_control_block ctrl_block[OUTPUTS];

static inline void output_flush(uint32_t id)
{
	uint32_t write_len = output_idx[id];
	uint32_t wrote, idx = 0;

	while (write_len > 0) {
		wrote = hda_log_out(&output_buf[id][idx], write_len);

		/* output is full, but if we're in an isr..
		 * it may never flush! In order to make progress
		 * drop bytes from this output.
		 */
		if (wrote == 0 && k_is_in_isr()) {
			write_len = 0;
			output_idx[id] = 0;
			return;
		}
		idx += wrote;
		write_len -= wrote;
	}
}

static int output_log_out(uint8_t *data, size_t length, void *ctx)
{
	int wrote;

	if (length == 0) {
		return 0;
	}

#ifdef CONFIG_LOG_MODE_IMMEDIATE
	uintptr_t id = (uintptr_t)ctx;

	__ASSERT(length == 1, "Immediate mode length should be 1 always");

	output_buf[id][output_idx[id]] = *data;
	output_idx[id]++;
	if (output_idx[id] >= OUTPUT_BUF_SIZE) {
		output_flush(id);
		output_idx[id] = 0;
	}

	wrote = 1;
#else
	wrote = hda_log_out(data, length);
#endif

	return wrote;
}

#define OUTPUT_INIT(i, _) \
	{ \
		.func = output_log_out, \
		.control_block = &ctrl_block[i], \
		.buf = &output_buf[i][0], \
		.size = OUTPUT_BUF_SIZE, \
	}

static struct log_output outputs[OUTPUTS] = {
	LISTIFY(OUTPUTS, OUTPUT_INIT, (,))
};


static inline uint32_t get_output_id(k_spinlock_key_t *key)
{
	int32_t id  = -1;

	if (k_is_in_isr()) {
		*key = k_spin_lock(&hda_log_fmt_lock);
		id = 0;
	} else {
		k_sem_take(&outputs_free, K_FOREVER);

		for (int i = 1; i < OUTPUTS; i++) {
			if (!atomic_test_and_set_bit(&output_used, i)) {
				id = i;
				break;
			}
		}
		__ASSERT_NO_MSG(id >= 1);
	}

	log_output_ctx_set(&outputs[id], (void *)(uintptr_t)id);

	output_idx[id] = 0;


	return id;
}

static inline void return_output_id(uint32_t id, k_spinlock_key_t *key)
{
	output_flush(id);
	if (k_is_in_isr()) {
		k_spin_unlock(&hda_log_fmt_lock, *key);
	} else {
		atomic_clear_bit(&output_used, id);
		k_sem_give(&outputs_free);
	}
}

static inline void dropped(const struct log_backend *const backend,
			   uint32_t cnt)
{
	ARG_UNUSED(backend);

	k_spinlock_key_t key = (k_spinlock_key_t){0};

	uint32_t id = get_output_id(&key);

	if (IS_ENABLED(CONFIG_LOG_DICTIONARY_SUPPORT)) {
		log_dict_output_dropped_process(&outputs[id], cnt);
	} else {
		log_output_dropped_process(&outputs[id], cnt);
	}

	return_output_id(id, &key);
}

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

	/* will immediately flush all future writes once set
	 * no other work is necessary here as flushing already
	 * occurs automatically
	 */
	atomic_set_bit(&hda_log_flags, HDA_LOG_PANIC_MODE);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	ARG_UNUSED(backend);

	log_format_current = log_type;

	return 0;
}


static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	ARG_UNUSED(backend);

	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);
	k_spinlock_key_t key = (k_spinlock_key_t){0};
;
	uint32_t id =  get_output_id(&key);

	log_output_func(&outputs[id], &msg->log, flags);

	return_output_id(id, &key);
}

/**
 * Lazily initialized, while the DMA may not be setup we continue
 * to buffer log messages untilt he buffer is full.
 */
static void init(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

const struct log_backend_api log_backend_adsp_hda_api = {
	.process = process,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.panic = panic,
	.format_set = format_set,
	.init = init,
};

LOG_BACKEND_DEFINE(log_backend_adsp_hda, log_backend_adsp_hda_api, true);

void adsp_hda_log_init(adsp_hda_log_hook_t fn, uint32_t channel)
{
	hook = fn;

	int res;

	__ASSERT(device_is_ready(hda_log_dev), "DMA device is not ready");

	hda_log_chan = dma_request_channel(hda_log_dev, &channel);
	__ASSERT(hda_log_chan >= 0, "No valid DMA channel");
	__ASSERT(hda_log_chan == channel, "Not requested channel");

	/* configure channel */
	struct dma_block_config hda_log_dma_blk_cfg = {
		.block_size = CONFIG_LOG_BACKEND_ADSP_HDA_SIZE,
		.source_address = (uint32_t)(uintptr_t)&hda_log_buf,
	};

	struct dma_config hda_log_dma_cfg = {
		.channel_direction = MEMORY_TO_HOST,
		.block_count = 1,
		.head_block = &hda_log_dma_blk_cfg,
		.source_data_size = 4,
	};

	res = dma_config(hda_log_dev, hda_log_chan, &hda_log_dma_cfg);
	__ASSERT(res == 0, "DMA config failed");

	res = dma_start(hda_log_dev, hda_log_chan);
	__ASSERT(res == 0, "DMA start failed");

	atomic_set_bit(&hda_log_flags, HDA_LOG_DMA_READY);


#ifdef HDA_LOG_PERIODIC
	k_timer_init(&hda_log_timer, hda_log_periodic, NULL);
	k_timer_start(&hda_log_timer,
		      K_MSEC(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME),
		      K_MSEC(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME));
#endif

}

#ifdef CONFIG_LOG_BACKEND_ADSP_HDA_CAVSTOOL

#include <intel_adsp_ipc.h>
#include <cavstool.h>

#define CHANNEL 6
#define IPC_TIMEOUT K_MSEC(1500)

static inline void hda_ipc_msg(const struct device *dev, uint32_t data,
			       uint32_t ext, k_timeout_t timeout)
{
	__ASSERT(intel_adsp_ipc_send_message_sync(dev, data, ext, timeout),
		"Unexpected ipc send message failure, try increasing IPC_TIMEOUT");
}

/* Each try multiplies the delay by 2 */
#define HDA_NOTIFY_MAX_TRIES 32

bool adsp_hda_log_cavstool_hook(uint32_t hook_notify)
{
	/* Cavstool polls the dma position */
	return true;
}


int adsp_hda_log_cavstool_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_RESET, CHANNEL, IPC_TIMEOUT);
	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_CONFIG,
		    CHANNEL | ((CONFIG_LOG_BACKEND_ADSP_HDA_SIZE*8) << 8), IPC_TIMEOUT);
	adsp_hda_log_init(adsp_hda_log_cavstool_hook, CHANNEL);
	/* Inform cavstool this is the logger by setting the second byte to 1 */
	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_START, (1 << 8) | CHANNEL, IPC_TIMEOUT);

	return 0;
}

SYS_INIT(adsp_hda_log_cavstool_init, POST_KERNEL, 99);

#endif
