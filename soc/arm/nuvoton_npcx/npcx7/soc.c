/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

static int soc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* CDCG register structure check */
NPCX_REG_SIZE_CHECK(cdcg_reg, 0x116);
NPCX_REG_OFFSET_CHECK(cdcg_reg, HFCBCD, 0x010);
NPCX_REG_OFFSET_CHECK(cdcg_reg, HFCBCD2, 0x014);
NPCX_REG_OFFSET_CHECK(cdcg_reg, LFCGCTL, 0x100);
NPCX_REG_OFFSET_CHECK(cdcg_reg, LFCGCTL2, 0x114);

/* PMC register structure check */
NPCX_REG_SIZE_CHECK(pmc_reg, 0x025);
NPCX_REG_OFFSET_CHECK(pmc_reg, ENIDL_CTL, 0x003);
NPCX_REG_OFFSET_CHECK(pmc_reg, PWDWN_CTL1, 0x008);
NPCX_REG_OFFSET_CHECK(pmc_reg, PWDWN_CTL7, 0x024);

/* SCFG register structure check */
NPCX_REG_SIZE_CHECK(scfg_reg, 0x02F);
NPCX_REG_OFFSET_CHECK(scfg_reg, DEV_CTL4, 0x006);
NPCX_REG_OFFSET_CHECK(scfg_reg, DEVALT0, 0x010);
NPCX_REG_OFFSET_CHECK(scfg_reg, LV_GPIO_CTL0, 0x02A);

/* UART register structure check */
NPCX_REG_SIZE_CHECK(uart_reg, 0x027);
NPCX_REG_OFFSET_CHECK(uart_reg, UPSR, 0x00E);
NPCX_REG_OFFSET_CHECK(uart_reg, UFTSTS, 0x020);
NPCX_REG_OFFSET_CHECK(uart_reg, UFRCTL, 0x026);

/* GPIO register structure check */
NPCX_REG_SIZE_CHECK(gpio_reg, 0x008);
NPCX_REG_OFFSET_CHECK(gpio_reg, PLOCK_CTL, 0x007);
