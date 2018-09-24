/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SAMPLE_MODULE_H
#define SAMPLE_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <logging/log.h>

#define LOG_MODULE_NAME sample_module
#define LOG_LEVEL CONFIG_SAMPLE_MODULE_LOG_LEVEL
#define LOG_IN_HEADER 1
LOG_MODULE_IN_HEADER_DECLARE();

const char *sample_module_name_get(void);
void sample_module_func(void);

static inline void sample_module_inline_func(void)
{
	LOG_INF("inline logging");
}

#undef LOG_IN_HEADER /* Undef to indicate end of logging in the header. */
#undef LOG_LEVEL

#ifdef __cplusplus
}
#endif

#endif /* SAMPLE_MODULE_H */
