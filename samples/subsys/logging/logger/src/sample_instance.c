/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sample_instance.h"
#include <string.h>

/* Note: no name is defined because only instance logging is used.
 * Instances are registered as logging sources not module.
 */
#include <logging/log.h>

LOG_LEVEL_SET(LOG_LEVEL_INF);

void sample_instance_call(struct sample_instance *inst)
{
	uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8,
			9, 10, 11, 12, 13, 14, 15, 16,
			17, 18, 19, 20, 21, 22, 23, 24,
			25, 26, 27, 28, 29, 30, 31, 32,
			33, 34, };

	LOG_INST_INF(inst->log, "counter_value: %d", inst->cnt++);
	LOG_INST_HEXDUMP_WRN(inst->log, data, sizeof(data),
					"Example of hexdump:");
	(void)memset(data, 0, sizeof(data));
}
