/*
 * Copyright (c) 2018-2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#ifndef _ASMLANGUAGE
#include "system_cmsdk_musca.h"
#include <generated_dts_board.h>
#include <sys/util.h>
#endif

extern void wakeup_cpu1(void);

extern u32_t sse_200_platform_get_cpu_id(void);

#endif /* _SOC_H_ */
