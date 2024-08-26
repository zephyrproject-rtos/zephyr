/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>

#include <zephyr/logging/log.h>

/* Prevent LOG_MODULE_DECLARE and LOG_MODULE_REGISTER being called together */
#define SMTC_MODEM_HAL_DBG_TRACE_C
#include <smtc_modem_hal_dbg_trace.h>

LOG_MODULE_REGISTER(lorawan, CONFIG_LORA_BASICS_MODEM_LOG_LEVEL);

void smtc_str_trim_end(char *text)
{
	char *end;

	/* Trim trailing space */
	end = text + strlen(text) - 1;
	while (end > text && isspace((unsigned char)*end)) {
		end--;
	}

	/* Write new null terminator character */
	end[1] = '\0';
}
