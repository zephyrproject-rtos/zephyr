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

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_NMI_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_NMI_H_

#ifndef _ASMLANGUAGE
#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_NMI_H_ */
