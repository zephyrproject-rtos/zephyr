/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/util.h>

#include <debug/coredump.h>
#include "coredump_internal.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(coredump, CONFIG_KERNEL_LOG_LEVEL);

#define COREDUMP_BEGIN_STR	"BEGIN#"
#define COREDUMP_END_STR	"END#"
#define COREDUMP_ERROR_STR	"ERROR CANNOT DUMP#"

/*
 * Need to prefix coredump strings to make it easier to parse
 * as log module adds its own prefixes.
 */
#define COREDUMP_PREFIX_STR	"#CD:"

/* Length of buffer of printable size */
#define LOG_BUF_SZ		64

/* Length of buffer of printable size plus null character */
#define LOG_BUF_SZ_RAW		(LOG_BUF_SZ + 1)

/* Log Buffer */
char log_buf[LOG_BUF_SZ_RAW];

static void coredump_logging_backend_start(void)
{
	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_BEGIN_STR);
}

static void coredump_logging_backend_end(void)
{
	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_END_STR);
}

static void coredump_logging_backend_error(void)
{
	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_ERROR_STR);
}

static int coredump_logging_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	uint8_t log_ptr = 0;
	size_t remaining = buflen;
	size_t i = 0;
	int ret = 0;

	if ((buf == NULL) || (buflen == 0)) {
		ret = -EINVAL;
		remaining = 0;
	}

	while (remaining > 0) {
		if (hex2char(buf[i] >> 4, &log_buf[log_ptr]) < 0) {
			ret = -EINVAL;
			break;
		}
		log_ptr++;

		if (hex2char(buf[i] & 0xf, &log_buf[log_ptr]) < 0) {
			ret = -EINVAL;
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

	return ret;
}

struct z_coredump_backend_api z_coredump_backend_logging = {
	.start = coredump_logging_backend_start,
	.end = coredump_logging_backend_end,
	.error = coredump_logging_backend_error,
	.buffer_output = coredump_logging_backend_buffer_output,
};
