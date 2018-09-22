/*
 * Copyright (c) 2018, Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EMSK_DT_H
#define __EMSK_DT_H

#include <mem.h>

#define DT_APB_CLK_HZ		50000000

#if defined(CONFIG_SOC_EMSK_EM11D)
#define DT_ICCM_SIZE	DT_SIZE_K(64)
#define DT_DCCM_SIZE	DT_SIZE_K(64)
#elif defined(CONFIG_SOC_EMSK_EM7D)
#define DT_ICCM_SIZE	DT_SIZE_K(256)
#define DT_DCCM_SIZE	DT_SIZE_K(128)
#elif defined(CONFIG_SOC_EMSK_EM9D)
#define DT_ICCM_SIZE	DT_SIZE_K(256)
#define DT_DCCM_SIZE	DT_SIZE_K(128)
#endif

#ifdef CONFIG_BOARD_EM_STARTERKIT_R23
#define DT_GPIO0_INTNO	24
#define DT_GPIO1_INTNO	0
#define DT_GPIO2_INTNO	0
#define DT_GPIO3_INTNO	0
#define DT_I2C0_INTNO	25
#define DT_I2C1_INTNO	26
#define DT_SPI0_INTNO	27
#define DT_SPI1_INTNO	28
#define DT_UART0_INTNO	29
#define DT_UART1_INTNO	30
#define DT_UART2_INTNO	31
#else /* CONFIG_BOARD_EM_STARTERKIT_R23 */
#define DT_GPIO0_INTNO	22
#define DT_GPIO1_INTNO	0
#define DT_GPIO2_INTNO	0
#define DT_GPIO3_INTNO	0
#define DT_I2C0_INTNO	23
#define DT_I2C1_INTNO	24
#define DT_SPI0_INTNO	25
#define DT_SPI1_INTNO	26
#define DT_UART0_INTNO	27
#define DT_UART1_INTNO	28
#define DT_UART2_INTNO	29
#endif

#endif /* __EMSK_DT_H */
