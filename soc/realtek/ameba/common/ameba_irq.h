/*
 * Copyright (c) 2025 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_AMEBA_COMMON_AMEBA_IRQ_H_
#define ZEPHYR_SOC_REALTEK_AMEBA_COMMON_AMEBA_IRQ_H_

bool irq_register(IRQ_FUN IrqFun, IRQn_Type IrqNum, u32 Data, u32 Priority);
bool irq_unregister(IRQn_Type IrqNum);

#endif
