/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <platform/argus_print.h>
#include <api/argus_status.h>
#include <zephyr/kernel.h>

status_t print(const char *fmt_s, ...)
{
	va_list args;

	va_start(args, fmt_s);
	vprintk(fmt_s, args);
	va_end(args);

	return STATUS_OK;
}
