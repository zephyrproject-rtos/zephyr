/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>

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
static struct k_spinlock lock;

static uint32_t log_format_current = CONFIG_LOG_BACKEND_ADSP_OUTPUT_DEFAULT;
void winstream_console_trace_out(int8_t *str, size_t len);

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	winstream_console_trace_out(data, length);

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

LOG_OUTPUT_DEFINE(log_output_adsp, char_out, log_buf, sizeof(log_buf));

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
	k_spinlock_key_t key = k_spin_lock(&lock);

	log_backend_std_panic(&log_output_adsp);

	k_spin_unlock(&lock, key);
}

static inline void dropped(const struct log_backend *const backend,
			   uint32_t cnt)
{
	log_output_dropped_process(&log_output_adsp, cnt);
}

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	k_spinlock_key_t key = k_spin_lock(&lock);

	log_output_func(&log_output_adsp, &msg->log, format_flags());

	k_spin_unlock(&lock, key);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

const struct log_backend_api log_backend_adsp_api = {
	.process = process,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.panic = panic,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_adsp, log_backend_adsp_api, true);
