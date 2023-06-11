/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _QUICKLOGIC_EOS_S3_SOC_PINMAP_H_
#define _QUICKLOGIC_EOS_S3_SOC_PINMAP_H_

#include <soc.h>

/* Set UART TX to PAD44 */
#define UART_TXD_PAD44                                                                             \
	(UART_TXD_SEL_PAD44 | PAD_CTRL_SEL_AO_REG | PAD_OEN_NORMAL | PAD_P_Z | PAD_SR_SLOW |       \
	 PAD_E_4MA | PAD_REN_DISABLE | PAD_SMT_DISABLE)

/* Set UART RX to PAD45 */
#define UART_RXD_PAD45                                                                             \
	(UART_RXD_SEL_PAD45 | PAD_CTRL_SEL_AO_REG | PAD_OEN_DISABLE | PAD_P_Z | PAD_SR_SLOW |      \
	 PAD_E_4MA | PAD_REN_ENABLE | PAD_SMT_DISABLE)

/* Set USR_BUTTON to PAD6 */
#define IO_REG_SEL           0x160
#define EXT_REG_OFFSET_SHIFT 20
#define PAD6_FUNC_SEL_GPIO_0 ((uint32_t)(0x03 | (IO_REG_SEL << EXT_REG_OFFSET_SHIFT)))
#define BUTTON_PAD_CFG6                                                                            \
	(PAD6_FUNC_SEL_GPIO_0 | PAD_CTRL_SEL_AO_REG | PAD_OEN_DISABLE | PAD_P_PULLUP |             \
	 PAD_SR_SLOW | PAD_E_4MA | PAD_REN_ENABLE | PAD_SMT_DISABLE)

#endif /* _QUICKLOGIC_EOS_S3_SOC_PINMAP_H_ */
