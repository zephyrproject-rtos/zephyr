/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SAMPLE_INSTANCE_H
#define SAMPLE_INSTANCE_H

#include <kernel.h>
#include <logging/log_instance.h>

#define SAMPLE_INSTANCE_NAME sample_instance

struct sample_instance {
	LOG_INSTANCE_PTR_DECLARE(log);
	u32_t cnt;
};

#define SAMPLE_INSTANCE_DEFINE(_name)					\
	LOG_INSTANCE_REGISTER(SAMPLE_INSTANCE_NAME, _name, 3);		\
	struct sample_instance _name = {				\
		LOG_INSTANCE_PTR_INIT(log, SAMPLE_INSTANCE_NAME, _name)	\
	}

void sample_instance_call(struct sample_instance *inst);

#endif /*SAMPLE_INSTANCE_H*/
