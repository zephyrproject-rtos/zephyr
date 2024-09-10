/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/linker/section_tags.h>

extern int boot_complete(void);
extern void power_init(void);
extern void adsp_clock_init(void);

#if CONFIG_MP_MAX_NUM_CPUS > 1
extern void soc_mp_init(void);
#endif

void soc_early_init_hook(void)
{
	(void)boot_complete();
	power_init();

#ifdef CONFIG_ADSP_CLOCK
	adsp_clock_init();
#endif

#if CONFIG_MP_MAX_NUM_CPUS > 1
	soc_mp_init();
#endif
}
