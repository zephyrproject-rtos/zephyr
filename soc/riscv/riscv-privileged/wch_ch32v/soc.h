/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#include <soc_common.h>

#ifndef _ASMLANGUAGE

#include <zephyr/device.h>

#define CH32V_SYS_BASE DT_REG_ADDR(DT_NODELABEL(syscon))

#if defined(CONFIG_SOC_CH56X)
#include <syscon_regs_ch56x.h>
#endif

void ch32v_sys_unlock(void);
void ch32v_sys_relock(void);

#endif /* !_ASMLANGUAGE */

#endif /* __SOC_H__ */
