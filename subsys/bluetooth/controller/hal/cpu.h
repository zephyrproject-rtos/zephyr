/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CPU_H_
#define _CPU_H_

static inline void cpu_sleep(void)
{
	__WFE();
	__SEV();
	__WFE();
}

#endif /* _CPU_H_ */
