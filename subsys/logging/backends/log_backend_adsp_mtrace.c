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

static void mtrace_init(void)
{
	if (ADSP_DW->descs[0].type == MTRACE_LOGGING_SLOT_TYPE(MTRACE_CORE)) {
		return;
	}

	ADSP_DW->descs[0].type = MTRACE_LOGGING_SLOT_TYPE(MTRACE_CORE);
}

static size_t mtrace_out(int8_t *str, size_t len, size_t *space_left)
{
	struct adsp_debug_slot *slot = (struct adsp_debug_slot *)(ADSP_DW->slots[0]);
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

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	size_t space_left = 0;
	size_t out;

	/*
	 * we handle the data even if mtrace notifier is not
	 * active. this ensures we can capture early boot messages.
	 */
	out = mtrace_out(data, length, &space_left);

	if (mtrace_active && mtrace_hook) {

		/* if we are in panic mode, need to flush out asap */
		if (unlikely(mtrace_panic_mode)) {
			space_left = 0;
		}

		mtrace_hook(out, space_left);
	}

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

	return flags;
}

static void panic(struct log_backend const *const backend)
{
	mtrace_panic_mode = true;
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

	mtrace_hook = hook;
	mtrace_active = true;
}

const struct log_backend *log_backend_adsp_mtrace_get(void)
{
	return &log_backend_adsp_mtrace;
}
