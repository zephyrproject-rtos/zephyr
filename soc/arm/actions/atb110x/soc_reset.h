/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Peripheral reset configuration macros for ATB110X
 */

#ifndef _ACTIONS_SOC_RESET_H_
#define _ACTIONS_SOC_RESET_H_

#define RESET_ID_DMA		0
#define RESET_ID_SPICACHE	1
#define RESET_ID_SPI0		2
#define RESET_ID_SPI1		3
#define RESET_ID_SPI2		4
#define RESET_ID_UART0		5
#define RESET_ID_UART1		6
#define RESET_ID_UART2		7
#define RESET_ID_I2C0		8
#define RESET_ID_I2C1		9
#define RESET_ID_PWM		10
#define RESET_ID_KEY		15
#define RESET_ID_AUDIO		16
#define RESET_ID_HRESET		24
#define RESET_ID_BLE		25
#define RESET_ID_LLCC		26
#define RESET_ID_BIST		30
#define RESET_ID_IRC		31

#define RESET_ID_MAX_ID		31

#ifndef _ASMLANGUAGE

void acts_reset_peripheral_assert(int reset_id);
void acts_reset_peripheral_deassert(int reset_id);
void acts_reset_peripheral(int reset_id);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_RESET_H_ */
