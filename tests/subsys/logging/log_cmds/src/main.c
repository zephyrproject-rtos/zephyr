/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_DBG);

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(msg);
}

static const struct log_backend_api backend_api = {
	.process = process,
};

LOG_BACKEND_DEFINE(test_backend, backend_api, false);

ZTEST(log_cmds, test_go_uses_runtime_default_level)
{
	const struct shell *shell = shell_backend_dummy_get_ptr();
	int source_id = log_source_id_get("test");
	uint32_t level;
	int err;

	zassert_not_null(shell);
	zassert_true(source_id >= 0, "test log source not found");

	err = shell_execute_cmd(shell, "log backend test_backend go");
	zassert_ok(err);
	zassert_true(log_backend_is_active(&test_backend));

	level = log_filter_get(&test_backend, Z_LOG_LOCAL_DOMAIN_ID, source_id, true);
	zassert_equal(level, CONFIG_LOG_RUNTIME_DEFAULT_LEVEL);

	log_backend_disable(&test_backend);
}

ZTEST_SUITE(log_cmds, NULL, NULL, NULL, NULL, NULL);
