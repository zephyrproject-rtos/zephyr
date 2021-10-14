/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PLATFORM_PLATFORM_H__
#define __PLATFORM_PLATFORM_H__

#define PLATFORM_PRIMARY_CORE_ID		0

#define MAX_CORE_COUNT				1

#if PLATFORM_CORE_COUNT > MAX_CORE_COUNT
#error "Invalid core count - exceeding core limit"
/* IPC Interrupt */
#define PLATFORM_IPC_INTERRUPT		IRQ_NUM_MU
#define PLATFORM_IPC_INTERRUPT_NAME	NULL
#endif

#endif /* __PLATFORM_PLATFORM_H__ */
