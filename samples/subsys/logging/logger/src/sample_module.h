/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SAMPLE_MODULE_H
#define SAMPLE_MODULE_H

#include <logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODULE_NAME sample_module

const char *sample_module_name_get(void);
void sample_module_func(void);

static inline void inline_func(void)
{
	LOG_MODULE_DECLARE(MODULE_NAME, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

	LOG_INF("Inline function.");
}

#ifdef __cplusplus
}
#endif

#endif /* SAMPLE_MODULE_H */
