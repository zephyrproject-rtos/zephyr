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

void sample_instance_call(struct sample_instance *inst)
{
	u8_t data[4] = { 1, 2, 3, 4 };

	LOG_INST_INF(inst->log, "counter_value: %d", inst->cnt++);
	LOG_INST_HEXDUMP_WRN(inst->log, data, sizeof(data),
					"Example of hexdump:");
	(void)memset(data, 0, sizeof(data));
}
