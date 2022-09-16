/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CORTEX_M_CPU_H
#define _CORTEX_M_CPU_H

#ifdef _ASMLANGUAGE

#define _SCS_BASE_ADDR _PPB_INT_SCS

/* ICSR defines */
#define _SCS_ICSR (_SCS_BASE_ADDR + 0xd04)
#define _SCS_ICSR_PENDSV (1 << 28)
#define _SCS_ICSR_UNPENDSV (1 << 27)
#define _SCS_ICSR_RETTOBASE (1 << 11)

#define _SCS_MPU_CTRL (_SCS_BASE_ADDR + 0xd94)

/* CONTROL defines */
#define _CONTROL_FPCA_Msk (1 << 2)

/* EXC_RETURN defines */
#define _EXC_RETURN_SPSEL_Msk (1 << 2)
#define _EXC_RETURN_FTYPE_Msk (1 << 4)
#endif

#endif
