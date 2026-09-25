/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME app
LOG_MODULE_REGISTER(MODULE_NAME, LOG_LEVEL_INF);

#ifndef CONFIG_LOG_RUNTIME_DEFAULT_LEVEL
#define CONFIG_LOG_RUNTIME_DEFAULT_LEVEL LOG_LEVEL_DBG
#endif

/* Must match the remote application. */
#define REMOTE_SOURCE_NAME "remote"
#define EARLY_LOG_CNT      3

#define LINK_ACTIVE_TIMEOUT_MS 5000
/* Logging thread may wait that long before processing pending messages. In the backend
 * configuration, messages are processed by logging threads on both cores (remote is assumed
 * to use the same sleep period).
 */
#define MSG_TIMEOUT_MS         (CONFIG_LOG_PROCESS_THREAD_SLEEP_MS + 100)
/* Remote and link report dropped messages with a delay (1 second each by default). */
#define DROPPED_TIMEOUT_MS     (MSG_TIMEOUT_MS + CONFIG_LOG_LINK_IPC_DROPPED_MSG_TIMEOUT)
#define POLL_MS                10

static const struct log_link *remote_link;
static uint8_t remote_domain;
static int16_t remote_source;

static int find_msg(size_t start, uint8_t domain_id, const char *text, struct mock_log_entry *entry)
{
	for (size_t i = start; i < mock_backend_count(); i++) {
		if (mock_backend_get(i, entry) && (entry->domain_id == domain_id) &&
		    (strncmp(entry->text, text, strlen(text)) == 0)) {
			return (int)i;
		}
	}

	return -1;
}

static int wait_msg(size_t start, uint8_t domain_id, const char *text, uint32_t timeout_ms,
		    struct mock_log_entry *entry)
{
	int64_t end = k_uptime_get() + timeout_ms;
	int idx;

	do {
		idx = find_msg(start, domain_id, text, entry);
		if (idx >= 0) {
			return idx;
		}
		k_msleep(POLL_MS);
	} while (k_uptime_get() < end);

	return -1;
}

static void print_captured(void)
{
	struct mock_log_entry entry;

	TC_PRINT("Captured messages (%zu):\n", mock_backend_count());
	for (size_t i = 0; i < mock_backend_count(); i++) {
		if (mock_backend_get(i, &entry)) {
			TC_PRINT("  [%zu] %s(%d)/%s(%d) lvl:%d \"%s\"\n", i, entry.domain,
				 entry.domain_id, entry.source, entry.source_id, entry.level,
				 entry.text);
		}
	}
}

/* Wait for a remote message and validate its origin and level. Returns the message index. */
static int expect_remote_msg(size_t start, const char *text, uint8_t level,
			     struct mock_log_entry *entry)
{
	struct mock_log_entry loc;
	int idx = wait_msg(start, remote_domain, text, MSG_TIMEOUT_MS, &loc);

	if (idx < 0) {
		print_captured();
	}
	zassert_true(idx >= 0, "Message \"%s\" not received", text);
	zassert_equal(loc.level, level, "Unexpected level %d for \"%s\"", loc.level, text);
	zassert_str_equal(loc.source, REMOTE_SOURCE_NAME);
	zassert_str_equal(loc.domain, remote_link->name);

	if (entry) {
		*entry = loc;
	}

	return idx;
}

/* Remote messages are processed in order so it must be called after a message which was
 * logged later on the remote has been received.
 */
static void expect_no_remote_msg(const char *text)
{
	struct mock_log_entry entry;

	zassert_true(find_msg(0, remote_domain, text, &entry) < 0, "Unexpected message \"%s\"",
		     entry.text);
}

static void remote_cmd(const char *cmd)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	char buf[64];
	const char *out;
	size_t len;
	int err;

	snprintf(buf, sizeof(buf), "remote_shell %s", cmd);
	shell_backend_dummy_clear_output(sh);

	err = shell_execute_cmd(sh, buf);
	zassert_equal(err, 0, "\"%s\" failed: %d", buf, err);

	out = shell_backend_dummy_get_output(sh, &len);
	zassert_not_null(strstr(out, "done"), "Unexpected \"%s\" output: \"%s\"", buf, out);
}

static int16_t remote_source_id_get(void)
{
	uint32_t cnt = log_src_cnt_get(remote_domain);

	for (uint32_t i = 0; i < cnt; i++) {
		const char *name = log_source_name_get(remote_domain, i);

		if ((name != NULL) && (strcmp(name, REMOTE_SOURCE_NAME) == 0)) {
			return (int16_t)i;
		}
	}

	return -1;
}

static int remote_level_set(uint32_t level)
{
	return log_filter_set(mock_backend_get_instance(), remote_domain, remote_source, level);
}

static int remote_level_get(bool runtime)
{
	return log_filter_get(mock_backend_get_instance(), remote_domain, remote_source, runtime);
}

/* 00 in name to ensure that it runs first. */
ZTEST(log_ipc, test_00_early_logs)
{
	char text[32];
	int idx = 0;

	if (!IS_ENABLED(CONFIG_TEST_LOG_IPC_EARLY_LOGS)) {
		unsigned long dropped;
		struct mock_log_entry entry;

		snprintf(text, sizeof(text), "Remote: %s dropped messages: ", remote_link->name);
		idx = wait_msg(0, Z_LOG_LOCAL_DOMAIN_ID, text, DROPPED_TIMEOUT_MS, &entry);
		if (idx < 0) {
			print_captured();
		}
		zassert_true(idx >= 0, "No report about dropped early messages");

		dropped = strtoul(&entry.text[strlen(text)], NULL, 10);
		zassert_true(dropped >= EARLY_LOG_CNT, "Unexpected dropped count: %lu", dropped);

		zassert_true(find_msg(0, remote_domain, "early", &entry) < 0,
			     "Unexpected early message \"%s\"", entry.text);
		return;
	}

	for (int i = 0; i < EARLY_LOG_CNT; i++) {
		snprintf(text, sizeof(text), "early %d/%d", i, EARLY_LOG_CNT);
		idx = expect_remote_msg(idx, text, LOG_LEVEL_INF, NULL) + 1;
	}
}

ZTEST(log_ipc, test_local_message)
{
	struct mock_log_entry entry;
	int idx;

	mock_backend_reset();
	LOG_INF("local message");
	idx = wait_msg(0, Z_LOG_LOCAL_DOMAIN_ID, "local message", MSG_TIMEOUT_MS, &entry);

	zassert_true(idx >= 0);
	zassert_str_equal(entry.domain, CONFIG_LOG_DOMAIN_NAME);
	zassert_str_equal(entry.source, STRINGIFY(MODULE_NAME));
}

ZTEST(log_ipc, test_remote_various_msg_types)
{
	int idx = 0;
	struct mock_log_entry entry;

	mock_backend_reset();
	remote_cmd("log_gen levels");

	idx = expect_remote_msg(idx, "lvl err -1", LOG_LEVEL_ERR, NULL) + 1;
	idx = expect_remote_msg(idx, "lvl wrn ro_str", LOG_LEVEL_WRN, NULL) + 1;
	idx = expect_remote_msg(idx, "lvl inf rw_str", LOG_LEVEL_INF, NULL) + 1;
	idx = expect_remote_msg(idx, "lvl dbg 100 78187493520", LOG_LEVEL_DBG, NULL) + 1;
	idx = expect_remote_msg(idx, "lvl hexdump", LOG_LEVEL_INF, &entry) + 1;
	zassert_not_null(strstr(entry.text, "de ad be ef 01 02 03 04"),
			 "Unexpected hexdump: \"%s\"", entry.text);
	(void)expect_remote_msg(idx, "lvl end", LOG_LEVEL_ERR, NULL);
}

ZTEST(log_ipc, test_remote_runtime_filtering)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_LOG_RUNTIME_FILTERING);

	zassert_equal(remote_level_get(false), LOG_LEVEL_DBG);
	zassert_equal(remote_level_get(true), CONFIG_LOG_RUNTIME_DEFAULT_LEVEL);

	zassert_equal(remote_level_set(LOG_LEVEL_WRN), LOG_LEVEL_WRN);
	zassert_equal(remote_level_get(true), LOG_LEVEL_WRN);

	mock_backend_reset();
	remote_cmd("log_gen levels");

	expect_remote_msg(0, "lvl err", LOG_LEVEL_ERR, NULL);
	expect_remote_msg(0, "lvl wrn", LOG_LEVEL_WRN, NULL);
	expect_remote_msg(0, "lvl end", LOG_LEVEL_ERR, NULL);
	expect_no_remote_msg("lvl inf");
	expect_no_remote_msg("lvl dbg");
	expect_no_remote_msg("lvl hexdump");

	zassert_equal(remote_level_set(LOG_LEVEL_DBG), LOG_LEVEL_DBG);
	zassert_equal(remote_level_get(true), LOG_LEVEL_DBG);

	mock_backend_reset();
	remote_cmd("log_gen levels");

	expect_remote_msg(0, "lvl inf", LOG_LEVEL_INF, NULL);
	expect_remote_msg(0, "lvl dbg", LOG_LEVEL_DBG, NULL);
}

ZTEST(log_ipc, test_remote_burst)
{
	struct mock_log_entry entry;
	char text[32];
	int idx = 0;

	mock_backend_reset();
	remote_cmd("log_gen burst " STRINGIFY(CONFIG_TEST_LOG_IPC_BURST_CNT));

	for (int i = 0; i < CONFIG_TEST_LOG_IPC_BURST_CNT; i++) {
		snprintf(text, sizeof(text), "burst %d", i);
		idx = expect_remote_msg(idx, text, LOG_LEVEL_INF, &entry) + 1;
	}

	zassert_equal(mock_backend_dropped(), 0);
	zassert_true(find_msg(0, Z_LOG_LOCAL_DOMAIN_ID, "Remote:", &entry) < 0,
		     "Unexpected message \"%s\"", entry.text);
}

ZTEST(log_ipc, test_remote_burst_dropped)
{
#define TOTAL_MSGS 100
	struct mock_log_entry entry;
	uint32_t received_msgs = 0;
	uint32_t dropped_msgs;
	int idx = 0;

	mock_backend_reset();
	remote_cmd("log_gen burst " STRINGIFY(TOTAL_MSGS));

	/* Wait for all remote messages to arrive. */
	k_msleep(DROPPED_TIMEOUT_MS + 1000);

	do {
		idx = find_msg(idx, remote_domain, "burst", &entry);
		if (idx < 0) {
			break;
		}
		received_msgs++;
		idx++;
	} while (received_msgs < TOTAL_MSGS);

	if (received_msgs < 10) {
		print_captured();
	}
	dropped_msgs = mock_backend_dropped();
	zassert_equal(received_msgs + dropped_msgs, TOTAL_MSGS, "Unexpected total count: %d",
		      received_msgs + dropped_msgs);

	if (!IS_ENABLED(CONFIG_LOG_LINK_IPC_RX_HOLD)) {
		/* If RX hold is not used, then local mpsc packet buffer is used so messages are
		 * dropped when the local buffer is full and there is no additionalwarning.
		 */
		return;
	}

	if (dropped_msgs > 0) {
		/* Find dropped message warning. */
		char text[64];

		snprintf(text, sizeof(text), "Remote: %s dropped messages: ", remote_link->name);
		zassert_true(find_msg(0, Z_LOG_LOCAL_DOMAIN_ID, text, &entry) >= 0,
			     "Unexpected message \"%s\"", text);
	}
}

static void *log_ipc_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int64_t end = k_uptime_get() + LINK_ACTIVE_TIMEOUT_MS;

	STRUCT_SECTION_GET(log_link, 0, &remote_link);

	/* Domain ID is assigned by the logging thread when the link is activated. */
	while ((remote_link->ctrl_blk->domain_offset == 0) && (k_uptime_get() < end)) {
		k_msleep(POLL_MS);
	}
	zassert_not_equal(remote_link->ctrl_blk->domain_offset, 0, "Link not activated");
	remote_domain = remote_link->ctrl_blk->domain_offset;
	zassert_not_equal(remote_domain, Z_LOG_LOCAL_DOMAIN_ID);
	zassert_str_equal(log_domain_name_get(remote_domain), remote_link->name);
	remote_source = remote_source_id_get();
	zassert_true(remote_source >= 0, "Remote source \"%s\" not found", REMOTE_SOURCE_NAME);

#if defined(CONFIG_LOG_RUNTIME_FILTERING)
	/* After activation, the logging thread sets runtime levels of all remote sources. */
	while ((remote_level_get(true) != CONFIG_LOG_RUNTIME_DEFAULT_LEVEL) &&
	       (k_uptime_get() < end)) {
		k_msleep(POLL_MS);
	}
#endif

	while (!shell_ready(sh) && (k_uptime_get() < end)) {
		k_msleep(POLL_MS);
	}
	zassert_true(shell_ready(sh), "Dummy shell not ready");

	TC_PRINT("Remote domain %s (id:%d), sources:%d\n", remote_link->name, remote_domain,
		 log_src_cnt_get(remote_domain));

	return NULL;
}

ZTEST_SUITE(log_ipc, NULL, log_ipc_setup, NULL, NULL, NULL);
