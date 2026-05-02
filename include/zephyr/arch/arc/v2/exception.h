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

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_EXCEPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/* ARCv2 Exception vector numbers */
#define ARC_EV_RESET			0x0
#define ARC_EV_MEM_ERROR		0x1
#define ARC_EV_INS_ERROR		0x2
#define ARC_EV_MACHINE_CHECK		0x3
#define ARC_EV_TLB_MISS_I		0x4
#define ARC_EV_TLB_MISS_D		0x5
#define ARC_EV_PROT_V			0x6
#define ARC_EV_PRIVILEGE_V		0x7
#define ARC_EV_SWI			0x8
#define ARC_EV_TRAP			0x9
#define ARC_EV_EXTENSION		0xA
#define ARC_EV_DIV_ZERO			0xB
#define ARC_EV_DC_ERROR			0xC
#define ARC_EV_MISALIGNED		0xD
#define ARC_EV_VEC_UNIT			0xE

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_EXCEPTION_H_ */
