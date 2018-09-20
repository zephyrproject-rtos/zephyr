/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_IRQ_H
#define __HAL_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

#define NVIC_INT_FIQ			               0
#define NVIC_INT_IRQ			               1
#define NVIC_BT_MASKED_PAGE_TIMEOUT_INTR       2
#define NVIC_BT_MASKED_SYNC_DET_INTR           3
#define NVIC_BT_MASKED_PKD_RX_HDR              4
#define NVIC_BT_MASKED_TIM_INTR0               5
#define NVIC_BT_MASKED_TIM_INTR1               6
#define NVIC_BT_MASKED_TIM_INTR2               7
#define NVIC_BT_MASKED_TIM_INTR3               8
#define NVIC_BT_MASKED_PKD_INTR                9
#define NVIC_BT_MASKED_PKA_INTR                10
#define NVIC_BT_MASKED_AUX_TMR_INTR            11
#define NVIC_BT_ACCELERATOR_INTR0              12
#define NVIC_BT_ACCELERATOR_INTR1              13
#define NVIC_BT_ACCELERATOR_INTR2              14
#define NVIC_BT_ACCELERATOR_INTR3              15
#define NVIC_BT_ACCELERATOR_INTR4              16
#define NVIC_INT_IPI			               19
#define NVIC_INT_AON_INTC		               20
#define NVIC_INT_REQ_WIFI_CAP                  31
#define NVIC_INT_DPD                           32
#define NVIC_INT_MAC                           33
#define NVIC_INT_UART0			               35
#define NVIC_INT_UART1			               36
#define NVIC_INT_REQ_COM_TMR                     37
#define NVIC_INT_WDG			               40
#define NVIC_INT_GNSS2BTWF_IPI	               50

#ifdef __cplusplus
}
#endif

#endif
