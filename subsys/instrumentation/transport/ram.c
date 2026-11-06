/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <instr_backend.h>

uint8_t instr_ram_buffer[CONFIG_INSTRUMENTATION_BACKEND_RAM_BUFFER_SIZE];
uint32_t instr_ram_buffer_pos;
static bool buffer_full;

__no_instrumentation__
static void instr_backend_ram_output(const struct instr_backend *backend, uint8_t *data,
				     uint32_t length)
{
	ARG_UNUSED(backend);

	if (buffer_full) {
		return;
	}

	if ((instr_ram_buffer_pos + length) > CONFIG_INSTRUMENTATION_BACKEND_RAM_BUFFER_SIZE) {
		buffer_full = true;
		return;
	}

	memcpy(instr_ram_buffer + instr_ram_buffer_pos, data, length);
	instr_ram_buffer_pos += length;
}

__no_instrumentation__
static void instr_backend_ram_init(void)
{
	memset(instr_ram_buffer, 0, sizeof(instr_ram_buffer));
	instr_ram_buffer_pos = 0;
	buffer_full = false;
}

static const struct instr_backend_api instr_backend_ram_api = {
	.init = instr_backend_ram_init,
	.output = instr_backend_ram_output,
};

INSTR_BACKEND_DEFINE(instr_backend_ram, instr_backend_ram_api);
