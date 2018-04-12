/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 public exception handling
 *
 * ARC-specific kernel exception handling interface. Included by arc/arch.h.
 */

#ifndef _ARCH_ARC_V2_EXC_H_
#define _ARCH_ARC_V2_EXC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
#else
typedef struct  _irq_stack_frame NANO_ESF;
#endif

#ifdef __cplusplus
}
#endif


#endif /* _ARCH_ARC_V2_EXC_H_ */
