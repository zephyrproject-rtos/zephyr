/*
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVICE_POWER_H
#define __DEVICE_POWER_H

#ifndef _ASMLANGUAGE

void soc_lite_sleep_enable(void);

#if defined(CONFIG_PM_DEEP_SLEEP_STATES)
void soc_deep_sleep_enable(void);
void soc_deep_sleep_disable(void);
void soc_deep_sleep_periph_save(void);
void soc_deep_sleep_periph_restore(void);
void soc_deep_sleep_wait_clk_idle(void);
void soc_deep_sleep_non_wake_en(void);
void soc_deep_sleep_non_wake_dis(void);
#endif

#endif

#endif
