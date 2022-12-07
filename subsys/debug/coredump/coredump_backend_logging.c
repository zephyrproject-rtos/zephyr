/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/debug/coredump.h>
#include "coredump_internal.h"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(coredump, CONFIG_KERNEL_LOG_LEVEL);

/* Length of buffer of printable size */
#define LOG_BUF_SZ		64

/* Length of buffer of printable size plus null character */
#define LOG_BUF_SZ_RAW		(LOG_BUF_SZ + 1)

/* Log Buffer */
static char log_buf[LOG_BUF_SZ_RAW];

static int error;

static void coredump_logging_backend_start(void)
{
	/* Reset error */
	error = 0;

	while (LOG_PROCESS()) {
		;
	}

	LOG_PANIC();
	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_BEGIN_STR);
}

static void coredump_logging_backend_end(void)
{
	if (error != 0) {
		LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_ERROR_STR);
	}

	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_END_STR);
}

static void coredump_logging_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	uint8_t log_ptr = 0;
	size_t remaining = buflen;
	size_t i = 0;

	if ((buf == NULL) || (buflen == 0)) {
		error = -EINVAL;
		remaining = 0;
	}

	while (remaining > 0) {
		if (hex2char(buf[i] >> 4, &log_buf[log_ptr]) < 0) {
			error = -EINVAL;
			break;
		}
		log_ptr++;

		if (hex2char(buf[i] & 0xf, &log_buf[log_ptr]) < 0) {
			error = -EINVAL;
			break;
		}
		log_ptr++;

		i++;
		remaining--;

		if ((log_ptr >= LOG_BUF_SZ) || (remaining == 0)) {
			log_buf[log_ptr] = '\0';
			LOG_ERR(COREDUMP_PREFIX_STR "%s", log_buf);
			log_ptr = 0;
		}
	}
}

static int coredump_logging_backend_query(enum coredump_query_id query_id,
					  void *arg)
{
	int ret;

	switch (query_id) {
	case COREDUMP_QUERY_GET_ERROR:
		ret = error;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int coredump_logging_backend_cmd(enum coredump_cmd_id cmd_id,
					void *arg)
{
	int ret;

	switch (cmd_id) {
	case COREDUMP_CMD_CLEAR_ERROR:
		ret = 0;
		error = 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}


struct coredump_backend_api coredump_backend_logging = {
	.start = coredump_logging_backend_start,
	.end = coredump_logging_backend_end,
	.buffer_output = coredump_logging_backend_buffer_output,
	.query = coredump_logging_backend_query,
	.cmd = coredump_logging_backend_cmd,
};


#ifdef CONFIG_DEBUG_COREDUMP_SHELL
#include <zephyr/shell/shell.h>

static int cmd_coredump_error_get(const struct shell *shell,
				  size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (error == 0) {
		shell_print(shell, "No error.");
	} else {
		shell_print(shell, "Error: %d", error);
	}

	return 0;
}

static int cmd_coredump_error_clear(const struct shell *shell,
				    size_t argc, char **argv)
{
	error = 0;

	shell_print(shell, "Error cleared.");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_coredump_error,
	SHELL_CMD(get, NULL, "Get Coredump error", cmd_coredump_error_get),
	SHELL_CMD(clear, NULL, "Clear Coredump error",
		  cmd_coredump_error_clear),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_coredump,
	SHELL_CMD(error, &sub_coredump_error,
		  "Get/clear backend error.", NULL),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(coredump, &sub_coredump,
		   "Coredump commands (logging backend)", NULL);

#endif /* CONFIG_DEBUG_COREDUMP_SHELL */
