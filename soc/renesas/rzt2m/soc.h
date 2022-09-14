/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

/* Do not let CMSIS to handle GIC and Timer */
#include <stdint.h>
#define __GIC_PRESENT 0
#define __TIM_PRESENT 0

/* Global system counter */
#define CNTCR_EN   BIT(0)
#define CNTCR_HDBG BIT(1)

/* Safety area protect register */
#define PRCRS_CLK       BIT(0)
#define PRCRS_LPC_RESET BIT(1)
#define PRCRS_GPIO      BIT(2)
#define PRCRS_SYS_CTRL  BIT(3)

/* Non-safety area protect register */
#define PRCRN_PRC0 BIT(0)
#define PRCRN_PRC1 BIT(1)
#define PRCRN_PRC2 BIT(2)

/* PRC Key Code - this value is required to allow any write operation
 * to the PRCRS / PRCRN registers.
 * See section 10.2 of the RZ/T2M User's Manual: Hardware.
 */
#define PRC_KEY_CODE 0xa500

void rzt2m_unlock_prcrn(uint32_t mask);
void rzt2m_lock_prcrn(uint32_t mask);
void rzt2m_unlock_prcrs(uint32_t mask);
void rzt2m_lock_prcrs(uint32_t mask);

#endif /* _SOC__H_ */
