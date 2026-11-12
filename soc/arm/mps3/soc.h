/*
 * Copyright (c) 2017-2021 Linaro Limited
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#include <cmsis_core_m_defaults.h>

#ifdef CONFIG_SOC_AN524
void wakeup_cpu1(void);
uint32_t sse_200_platform_get_cpu_id(void);
#endif /* CONFIG_SOC_AN524 */

#endif /* _SOC_H_ */
