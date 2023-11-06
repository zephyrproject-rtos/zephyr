/*
 * Copyright (c) 2017-2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#include <cmsis_core_m_defaults.h>
#include <soc_registers.h>

extern void wakeup_cpu1(void);

extern uint32_t sse_200_platform_get_cpu_id(void);

#endif /* _SOC_H_ */
