/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>

LOG_MODULE_REGISTER(log_efi);

static volatile bool in_panic;
static uint32_t log_format_current = CONFIG_LOG_BACKEND_EFI_CON_OUTPUT_DEFAULT;

static struct k_spinlock lock;

#define LOG_BUF_SIZE 128
static uint8_t efi_output_buf[LOG_BUF_SIZE + 1];

extern int efi_console_putchar(int c);

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);

	size_t cnt = 0;
	uint8_t *ptr = data;

	while (cnt < length) {
		efi_console_putchar(*ptr);
		cnt++;
		ptr++;
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_efi, char_out, efi_output_buf, sizeof(efi_output_buf));

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	k_spinlock_key_t key = k_spin_lock(&lock);

	log_output_func(&log_output_efi, &msg->log, flags);

	k_spin_unlock(&lock, key);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void log_backend_efi_init(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

}

static void panic(struct log_backend const *const backend)
{
	in_panic = true;
	log_backend_std_panic(&log_output_efi);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_efi, cnt);
}

const struct log_backend_api log_backend_efi_api = {
	.process = process,
	.panic = panic,
	.init = log_backend_efi_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_efi, log_backend_efi_api, true);
