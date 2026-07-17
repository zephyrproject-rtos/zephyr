/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/tc_capture.h>
#include <string.h>

#if defined(CONFIG_PRINTK)
#include <zephyr/sys/printk-hooks.h>
#endif

/*
 * Full logging (a real backend mode, not minimal) delivers output to backends
 * rather than through printk, so it needs a dedicated capture backend. Minimal
 * logging and plain printk are both caught by the printk hook below.
 */
#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_MODE_MINIMAL)
#define TC_CAPTURE_LOG_BACKEND 1
#include <zephyr/sys/cbprintf.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#endif

/*
 * Ring buffer retaining the most recent CONFIG_TEST_CAPTURE_BUFFER_SIZE bytes of
 * output. Captured output is unbounded while the buffer is fixed, so on overflow
 * the oldest bytes are dropped rather than the newest. This keeps matching
 * independent of the total volume of output as long as the region of interest
 * fits in the buffer; enlarge CONFIG_TEST_CAPTURE_BUFFER_SIZE if more history is
 * required.
 */
static struct k_spinlock lock;
static char capture_buf[CONFIG_TEST_CAPTURE_BUFFER_SIZE];
static size_t capture_head;
static bool capture_wrapped;
static bool capturing;

static void capture_append(char c)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (capturing) {
		capture_buf[capture_head] = c;
		capture_head++;
		if (capture_head == sizeof(capture_buf)) {
			capture_head = 0;
			capture_wrapped = true;
		}
	}

	k_spin_unlock(&lock, key);
}

/*
 * Copy the captured bytes in order into dst (up to dst_size) and return the
 * length. Caller holds the lock.
 */
static size_t capture_linearize(char *dst, size_t dst_size)
{
	size_t len = 0;

	if (dst_size == 0) {
		return 0;
	}

	if (capture_wrapped) {
		for (size_t i = capture_head; (i < sizeof(capture_buf)) &&
		     (len < dst_size); i++) {
			dst[len++] = capture_buf[i];
		}
	}

	for (size_t i = 0; (i < capture_head) && (len < dst_size); i++) {
		dst[len++] = capture_buf[i];
	}

	return len;
}

#if defined(CONFIG_PRINTK)
static printk_hook_fn_t prev_printk_hook;

static int capture_char_out(int c)
{
	capture_append((char)c);

	if (prev_printk_hook != NULL) {
		(void)prev_printk_hook(c);
	}

	return c;
}
#endif /* CONFIG_PRINTK */

#if defined(TC_CAPTURE_LOG_BACKEND)
static int log_render_out(int c, void *ctx)
{
	ARG_UNUSED(ctx);

	capture_append((char)c);

	return c;
}

static void log_capture_process(const struct log_backend *const backend,
				union log_msg_generic *msg)
{
	ARG_UNUSED(backend);

	size_t plen;
	uint8_t *package;
	bool active;
	k_spinlock_key_t key = k_spin_lock(&lock);

	active = capturing;
	k_spin_unlock(&lock, key);

	if (!active) {
		return;
	}

	package = log_msg_get_package(&msg->log, &plen);
	if ((package == NULL) || (plen == 0)) {
		return;
	}

	(void)cbpprintf(log_render_out, NULL, package);
	capture_append('\n');
}

static void log_capture_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api log_capture_api = {
	.process = log_capture_process,
	.panic = log_capture_panic,
};

LOG_BACKEND_DEFINE(tc_capture_backend, log_capture_api, true);
#endif /* TC_CAPTURE_LOG_BACKEND */

void tc_capture_start(void)
{
	tc_capture_clear();

	k_spinlock_key_t key = k_spin_lock(&lock);

	capturing = true;
	k_spin_unlock(&lock, key);

#if defined(CONFIG_PRINTK)
	prev_printk_hook = __printk_get_hook();
	__printk_hook_install(capture_char_out);
#endif
}

void tc_capture_stop(void)
{
#if defined(CONFIG_PRINTK)
	__printk_hook_install(prev_printk_hook);
#endif

	k_spinlock_key_t key = k_spin_lock(&lock);

	capturing = false;
	k_spin_unlock(&lock, key);
}

void tc_capture_clear(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	capture_head = 0;
	capture_wrapped = false;
	k_spin_unlock(&lock, key);
}

bool tc_capture_contains(const char *substr)
{
	static char linear[CONFIG_TEST_CAPTURE_BUFFER_SIZE + 1];
	bool found;
	size_t len;
	k_spinlock_key_t key = k_spin_lock(&lock);

	len = capture_linearize(linear, sizeof(linear) - 1);
	linear[len] = '\0';
	found = strstr(linear, substr) != NULL;
	k_spin_unlock(&lock, key);

	return found;
}

size_t tc_capture_get(char *dst, size_t size)
{
	size_t copied;
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (size == 0) {
		copied = 0;
	} else {
		copied = capture_linearize(dst, size - 1);
		dst[copied] = '\0';
	}

	k_spin_unlock(&lock, key);

	return copied;
}
