/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CAN_TI_CC35XX_
#define _CAN_TI_CC35XX_

#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/pinctrl.h>

#define CAN_TI_CC35XX_IMASK0       0x00000844
#define CAN_TI_CC35XX_IMASK0_INTL0 BIT(0)
#define CAN_TI_CC35XX_ICLR0        0x00000854
#define CAN_TI_CC35XX_ICLR0_INTL0  BIT(0)

#define CAN_TI_CC35XX_IMASK1       0x00000868
#define CAN_TI_CC35XX_IMASK1_INTL1 BIT(1)
#define CAN_TI_CC35XX_ICLR1        0x00000878
#define CAN_TI_CC35XX_ICLR1_INTL1  BIT(1)

#define CAN_TI_CC35XX_SSEOI       0x00000220
#define CAN_TI_CC35XX_SSEOI_INTL0 BIT(0)
#define CAN_TI_CC35XX_SSEOI_INTL1 BIT(1)

#define CAN_TI_CC35XX_CLKCFG                           0x00002000
#define CAN_TI_CC35XX_CLKCFG_CLKEN                     BIT(0)
#define CAN_TI_CC35XX_CLKCFG_RAMEN                     BIT(4)
#define CAN_TI_CC35XX_CLKCFG_CLKSEL_MASK               GENMASK(6, 5)
#define CAN_TI_CC35XX_CLKCFG_CLKSEL_HOST_DIV2_CLK      (0x01 << 5)
#define CAN_TI_CC35XX_CLKCFG_CLKSEL_HFXT               (0x02 << 5)
#define CAN_TI_CC35XX_CLKCFG_CLKSEL_HOST_DIV2_PSWL_CLK (0x03 << 5)

#define CAN_TI_CC35XX_CLKCFG_TIMEOUT_MS 100

struct can_ti_cc35xx_config {
	mm_reg_t mcan;
	mem_addr_t mram;
	uintptr_t mrba;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_configure)(void);
};

#endif /* _CAN_TI_CC35XX_ */
