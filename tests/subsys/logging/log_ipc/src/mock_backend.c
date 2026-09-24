/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <string.h>

#define MOCK_ENTRY_CNT 100

static struct mock_log_entry entries[MOCK_ENTRY_CNT];
/* Total number of messages captured since reset. Only the last MOCK_ENTRY_CNT are kept. */
static size_t entry_cnt;
static size_t text_len;
static atomic_t dropped;
static struct k_spinlock lock;

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	struct mock_log_entry *entry = &entries[entry_cnt % MOCK_ENTRY_CNT];

	ARG_UNUSED(ctx);

	for (size_t i = 0; i < length; i++) {
		if (text_len < (sizeof(entry->text) - 1)) {
			entry->text[text_len++] = data[i];
		}
	}

	return length;
}

static uint8_t mock_output_buf[16];
LOG_OUTPUT_DEFINE(log_output_mock, char_out, mock_output_buf, sizeof(mock_output_buf));

static void copy_name(char *dst, const char *name)
{
	if (name == NULL) {
		dst[0] = '\0';
		return;
	}

	strncpy(dst, name, MOCK_ENTRY_NAME_LEN - 1);
	dst[MOCK_ENTRY_NAME_LEN - 1] = '\0';
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	struct log_msg *log_msg = &msg->log;
	struct mock_log_entry *entry = &entries[entry_cnt % MOCK_ENTRY_CNT];
	size_t plen, dlen;
	uint8_t *package = log_msg_get_package(log_msg, &plen);
	uint8_t *data = log_msg_get_data(log_msg, &dlen);
	k_spinlock_key_t key;

	ARG_UNUSED(backend);

	memset(entry, 0, sizeof(*entry));
	entry->domain_id = log_msg_get_domain(log_msg);
	entry->source_id = log_msg_get_source_id(log_msg);
	entry->level = log_msg_get_level(log_msg);
	copy_name(entry->domain, log_domain_name_get(entry->domain_id));
	copy_name(entry->source, entry->source_id >= 0
					 ? log_source_name_get(entry->domain_id, entry->source_id)
					 : NULL);

	/* Only the message body is rendered, prefix is captured in dedicated fields. */
	text_len = 0;
	log_output_process(&log_output_mock, 0, NULL, NULL, NULL, 0, entry->level,
			   plen > 0 ? package : NULL, data, dlen, LOG_OUTPUT_FLAG_CRLF_NONE);

	while ((text_len > 0) &&
	       ((entry->text[text_len - 1] == '\n') || (entry->text[text_len - 1] == '\r'))) {
		text_len--;
	}
	entry->text[text_len] = '\0';

	key = k_spin_lock(&lock);
	entry_cnt++;
	k_spin_unlock(&lock, key);
}

static void dropped_cb(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	atomic_add(&dropped, cnt);
}

static void panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

void mock_backend_reset(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	entry_cnt = 0;
	atomic_set(&dropped, 0);
	k_spin_unlock(&lock, key);
}

size_t mock_backend_count(void)
{
	return entry_cnt;
}

bool mock_backend_get(size_t idx, struct mock_log_entry *entry)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	bool rv = (idx < entry_cnt) && ((entry_cnt - idx) <= MOCK_ENTRY_CNT);

	if (rv) {
		*entry = entries[idx % MOCK_ENTRY_CNT];
	}
	k_spin_unlock(&lock, key);

	return rv;
}

uint32_t mock_backend_dropped(void)
{
	return (uint32_t)atomic_get(&dropped);
}

static const struct log_backend_api mock_log_backend_api = {
	.process = process,
	.dropped = dropped_cb,
	.panic = panic,
};

LOG_BACKEND_DEFINE(log_backend_mock, mock_log_backend_api, true);

const struct log_backend *mock_backend_get_instance(void)
{
	return &log_backend_mock;
}
