/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <zephyr/kernel.h>
#include <tracing_core.h>
#include <tracing_backend.h>
#include <zephyr/arch/common/semihost.h>

static int tracing_fd = -1;

static void tracing_backend_semihost_output(const struct tracing_backend *backend, uint8_t *data,
					    uint32_t length)
{
	semihost_write(tracing_fd, data, length);
}

static void tracing_backend_semihost_init(void)
{
	const char *tracing_file = "./tracing.bin";

	tracing_fd = semihost_open(tracing_file, SEMIHOST_OPEN_AB_PLUS);
	__ASSERT(tracing_fd >= 0, "semihost_open() returned %d", tracing_fd);
	if (tracing_fd < 0) {
		k_panic();
	}
}

const struct tracing_backend_api tracing_backend_semihost_api = {
	.init = tracing_backend_semihost_init, .output = tracing_backend_semihost_output};

TRACING_BACKEND_DEFINE(tracing_backend_semihost, tracing_backend_semihost_api);
