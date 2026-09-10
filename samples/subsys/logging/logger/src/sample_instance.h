/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SAMPLE_INSTANCE_H
#define SAMPLE_INSTANCE_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log_instance.h>
#include <zephyr/logging/log.h>

#define SAMPLE_INSTANCE_NAME sample_instance

struct sample_instance {
	LOG_INSTANCE_PTR_DECLARE(log);
	uint32_t cnt;
};

#define SAMPLE_INSTANCE_DEFINE(_part, _name)				   \
	LOG_INSTANCE_REGISTER(SAMPLE_INSTANCE_NAME, _name, LOG_LEVEL_INF); \
	K_APP_DMEM(_part) struct sample_instance _name = {		   \
		LOG_INSTANCE_PTR_INIT(log, SAMPLE_INSTANCE_NAME, _name)	   \
	}

void sample_instance_call(struct sample_instance *inst);

static inline void sample_instance_inline_call(struct sample_instance *inst)
{
	LOG_LEVEL_SET(LOG_LEVEL_INF);

	LOG_INST_INF(inst->log, "Inline call.");
}

#endif /*SAMPLE_INSTANCE_H*/
