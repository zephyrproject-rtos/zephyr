/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for EM Starter kit board
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>

#if defined(CONFIG_BOARD_EM_STARTERKIT_R23) && defined(CONFIG_SOC_EMSK_EM7D)
#define IRQ_CORE_DMA_COMPLETE			22
#define IRQ_CORE_DMA_ERROR			23
#else /* CONFIG_BOARD_EM_STARTERKIT_R23 && CONFIG_SOC_EMSK_EM7D */
#define IRQ_CORE_DMA_COMPLETE			20
#define IRQ_CORE_DMA_ERROR			21
#endif /* !(CONFIG_BOARD_EM_STARTERKIT_R23 && CONFIG_SOC_EMSK_EM7D) */


#ifndef _ASMLANGUAGE

#include <zephyr/sys/util.h>
#include <zephyr/random/rand32.h>

#define INT_ENABLE_ARC				~(0x00000001 << 8)
#define INT_ENABLE_ARC_BIT_POS			(8)

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
