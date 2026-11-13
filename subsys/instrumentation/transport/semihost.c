/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/common/semihost.h>
#include <instr_backend.h>

static int instr_fd = -1;

__no_instrumentation__
static void instr_backend_semihost_output(const struct instr_backend *backend, uint8_t *data,
					  uint32_t length)
{
	ARG_UNUSED(backend);

	if (instr_fd < 0) {
		return;
	}

	semihost_write(instr_fd, data, length);
}

__no_instrumentation__
static void instr_backend_semihost_init(void)
{
	const char *instr_file = "./instrumentation.bin";

	instr_fd = semihost_open(instr_file, SEMIHOST_OPEN_AB_PLUS);
	__ASSERT(instr_fd >= 0, "semihost_open() returned %d", instr_fd);
	if (instr_fd < 0) {
		k_panic();
	}
}

static const struct instr_backend_api instr_backend_semihost_api = {
	.init = instr_backend_semihost_init,
	.output = instr_backend_semihost_output,
};

INSTR_BACKEND_DEFINE(instr_backend_semihost, instr_backend_semihost_api);
