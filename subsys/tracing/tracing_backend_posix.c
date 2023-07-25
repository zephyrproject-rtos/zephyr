/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <cmdline.h>
#include <tracing_backend.h>
#include "tracing_backend_posix_bottom.h"

static void *out_stream;
static const char *file_name;

static void tracing_backend_posix_init(void)
{
	if (file_name == NULL) {
		file_name = "channel0_0";
	}

	out_stream = tracing_backend_posix_init_bottom(file_name);
}

static void tracing_backend_posix_output(
		const struct tracing_backend *backend,
		uint8_t *data, uint32_t length)
{
	ARG_UNUSED(backend);

	tracing_backend_posix_output_bottom(data, length, out_stream);
}

const struct tracing_backend_api tracing_backend_posix_api = {
	.init = tracing_backend_posix_init,
	.output  = tracing_backend_posix_output
};

TRACING_BACKEND_DEFINE(tracing_backend_posix, tracing_backend_posix_api);

void tracing_backend_posix_option(void)
{
	static struct args_struct_t tracing_backend_option[] = {
		{
			.manual = false,
			.is_mandatory = false,
			.is_switch = false,
			.option = "trace-file",
			.name = "file_name",
			.type = 's',
			.dest = (void *)&file_name,
			.call_when_found = NULL,
			.descript = "File name for tracing output.",
		},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(tracing_backend_option);
}

NATIVE_TASK(tracing_backend_posix_option, PRE_BOOT_1, 1);
