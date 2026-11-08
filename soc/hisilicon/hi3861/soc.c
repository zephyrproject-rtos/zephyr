/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <soc.h>

void soc_early_init_hook(void)
{
	/* Disable watchdog reset and interrupt outputs left enabled by ROM. */
	sys_write16(sys_read16(HI3861_GLB_WDT_RST_SEL_REG) & ~HI3861_GLB_WDT_RST_SEL_WDG,
		    HI3861_GLB_WDT_RST_SEL_REG);
	sys_write32(sys_read32(HI3861_WDG_CTRL_REG) & ~HI3861_WDG_CTRL_INTERRUPT_MODE,
		    HI3861_WDG_CTRL_REG);
#ifdef CONFIG_HI3861_TIMER
	sys_write16(sys_read16(HI3861_CLDO_CLKEN_REG) | HI3861_CLDO_CLKEN_TIMER0 |
			    HI3861_CLDO_CLKEN_TIMER1,
		    HI3861_CLDO_CLKEN_REG);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* UART2 has separate peripheral and bus clock gates. */
	sys_write16(sys_read16(HI3861_UART_MAC80M_CLKEN_REG) | HI3861_UART_MAC80M_CLKEN_UART2 |
			    HI3861_UART_MAC80M_CLKEN_BUS2,
		    HI3861_UART_MAC80M_CLKEN_REG);
#endif
}

#ifdef CONFIG_REBOOT
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	(void)irq_lock();
	/* Reset the PMU-managed power domains. */
	sys_write16(0x1d, HI3861_PMU_WLAN_SLP_EVT_CLR_REG);
	sys_write16(0x1d, HI3861_PMU_WLAN_SLP_INT_CLR_REG);
	sys_write16(0x77, HI3861_PMU_WLAN_WKUP_EVT_CLR_REG);
	sys_write16(0x77, HI3861_PMU_WLAN_WKUP_INT_CLR_REG);
	sys_write16(1, HI3861_PMU_UDSLEEP_BUTTON_CTRL_REG);
	k_busy_wait(150);
	sys_write16(2, HI3861_PMU_UDSLEEP_BUTTON_CTRL_REG);
	sys_write16(1, HI3861_PMU_WLAN_SLP_EVT_EN_REG);
	sys_write16(1, HI3861_PMU_WLAN_STA0_ALLOW_SLEEP_REG);
	sys_write16(1, HI3861_PMU_WLAN_STA1_ALLOW_SLEEP_REG);
	sys_write16(1, HI3861_PMU_WLAN_AP0_ALLOW_SLEEP_REG);
	sys_write16(1, HI3861_PMU_WLAN_PF_ALLOW_SLEEP_REG);
	sys_write16(1, HI3861_PMU_WLAN_HOST_ALLOW_SLEEP_REG);
	for (;;) {
		__asm__ volatile("wfi");
	}
}
#endif
