/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#ifndef _ASMLANGUAGE
#include "system_cmsdk_musca_b1.h"
#include <zephyr/sys/util.h>
#endif

extern void wakeup_cpu1(void);

extern uint32_t sse_200_platform_get_cpu_id(void);

#endif /* _SOC_H_ */
