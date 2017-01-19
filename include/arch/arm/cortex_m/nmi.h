/**
 * @file
 *
 * @brief NMI routines for ARM Cortex M series
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CORTEX_M_NMI_H
#define __CORTEX_M_NMI_H

#ifndef _ASMLANGUAGE
#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif
#endif

#endif /* __CORTEX_M_NMI_H */
