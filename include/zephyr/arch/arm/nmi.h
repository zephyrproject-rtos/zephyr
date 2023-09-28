/**
 * @file
 *
 * @brief ARM AArch32 NMI routines
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_NMI_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_NMI_H_

#if !defined(_ASMLANGUAGE) && defined(CONFIG_RUNTIME_NMI)
extern void z_arm_nmi_set_handler(void (*pHandler)(void));
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_NMI_H_ */
