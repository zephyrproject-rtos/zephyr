/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline void cpu_sleep(void)
{
	__WFE();
	__SEV();
	__WFE();
}
