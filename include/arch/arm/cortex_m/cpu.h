/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CORTEX_M_CPU_H
#define _CORTEX_M_CPU_H

#ifdef _ASMLANGUAGE

#define _SCS_BASE_ADDR _PPB_INT_SCS
#define _SCS_ICSR (_SCS_BASE_ADDR + 0xd04)
#define _SCS_ICSR_PENDSV (1 << 28)
#define _SCS_ICSR_UNPENDSV (1 << 27)
#define _SCS_ICSR_RETTOBASE (1 << 11)

#define _SCS_MPU_CTRL (_SCS_BASE_ADDR + 0xd94)
#endif

#endif
