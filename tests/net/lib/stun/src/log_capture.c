/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/sys/cbprintf.h>
#include <zephyr/sys/printk.h>

#include "log_capture.h"

#define SOURCE_NAME "net_stun"

/* More records than any one test makes; the ones beyond are counted only. */
#define RECORDS_MAX 32
#define TEXT_SIZE   160

struct record {
	uint8_t level;
	char text[TEXT_SIZE];
};

static struct record records[RECORDS_MAX];
static size_t count;

struct text_out {
	char *at;
	char *end;
};

static int text_out_char(int c, void *ctx)
{
	struct text_out *out = ctx;

	if (out->at < out->end) {
		*out->at++ = (char)c;
	}
	return c;
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	const char *source = log_source_name_get(log_msg_get_domain(&msg->log),
						 log_msg_get_source_id(&msg->log));
	size_t len;

	ARG_UNUSED(backend);

	if (source == NULL || strcmp(source, SOURCE_NAME) != 0) {
		return;
	}
	if (count < RECORDS_MAX) {
		struct record *r = &records[count];
		struct text_out out = {.at = r->text, .end = r->text + sizeof(r->text) - 1};

		r->level = log_msg_get_level(&msg->log);
		(void)cbpprintf(text_out_char, &out, log_msg_get_package(&msg->log, &len));
		*out.at = '\0';
	}
	count++;
}

static void panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api capture_api = {
	.process = process,
	.panic = panic,
};

LOG_BACKEND_DEFINE(stun_test_log_capture, capture_api, false);

void log_capture_start(void)
{
	static bool started;

	if (!started) {
		started = true;
		log_backend_enable(&stun_test_log_capture, NULL, LOG_LEVEL_DBG);
	}
	log_capture_clear();
}

void log_capture_clear(void)
{
	count = 0;
	memset(records, 0, sizeof(records));
}

size_t log_capture_count(void)
{
	return count;
}

size_t log_capture_count_level(uint8_t level)
{
	size_t n = 0;

	for (size_t i = 0; i < MIN(count, RECORDS_MAX); i++) {
		if (records[i].level == level) {
			n++;
		}
	}
	return n;
}

bool log_capture_contains(const char *needle)
{
	for (size_t i = 0; i < MIN(count, RECORDS_MAX); i++) {
		if (strstr(records[i].text, needle) != NULL) {
			return true;
		}
	}
	return false;
}

void log_capture_dump(void)
{
	for (size_t i = 0; i < MIN(count, RECORDS_MAX); i++) {
		printk("  log record %u, level %u: %s\n", (unsigned int)i, records[i].level,
		       records[i].text);
	}
}
