/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc_common.h>
#include <zephyr/kernel.h>

static mm_reg_t ecpm_base = DT_REG_ADDR(DT_NODELABEL(ecpm));
/* it51xxx ECPM registers definition */
/* 0x03: PLL Control */
#define ECPM_PLLCTRL 0x03

void chip_pll_ctrl(enum chip_pll_mode mode)
{
	volatile uint8_t _pll_ctrl __unused;

	sys_write8(mode, ecpm_base + ECPM_PLLCTRL);
	/*
	 * for deep doze / sleep mode
	 * This load operation will ensure PLL setting is taken into
	 * control register before wait for interrupt instruction.
	 */
	_pll_ctrl = sys_read8(ecpm_base + ECPM_PLLCTRL);
}

#ifdef CONFIG_SOC_IT51XXX_CPU_IDLE_GATING
/* Preventing CPU going into idle mode during command queue. */
static atomic_t cpu_idle_disabled;

void chip_permit_idle(void)
{
	atomic_dec(&cpu_idle_disabled);
}

void chip_block_idle(void)
{
	atomic_inc(&cpu_idle_disabled);
}

bool cpu_idle_not_allowed(void)
{
	return atomic_get(&cpu_idle_disabled);
}
#endif

/* The routine must be called with interrupts locked */
void riscv_idle(enum chip_pll_mode mode, unsigned int key)
{
	/*
	 * The routine is called with interrupts locked (in kernel/idle()).
	 * But on kernel/context test_kernel_cpu_idle test, the routine will be
	 * called without interrupts locked. Hence we disable M-mode external
	 * interrupt here to protect the below content.
	 */
	csr_clear(mie, MIP_MEIP);
	sys_trace_idle();

	/* Chip doze after wfi instruction */
	chip_pll_ctrl(mode);

	/* Wait for interrupt */
	__asm__ volatile("wfi");

	/*
	 * Enable M-mode external interrupt
	 * An interrupt can not be fired yet until we enable global interrupt
	 */
	csr_set(mie, MIP_MEIP);

	/* Restore global interrupt lockout state */
	irq_unlock(key);
}

void arch_cpu_idle(void)
{
#ifdef CONFIG_SOC_IT51XXX_CPU_IDLE_GATING
	/*
	 * The EC processor(CPU) cannot be in the k_cpu_idle() during
	 * the transactions with the CQ mode(DMA mode). Otherwise,
	 * the EC processor would be clock gated.
	 */
	if (cpu_idle_not_allowed()) {
		/* Restore global interrupt lockout state */
		irq_unlock(MSTATUS_IEN);
	} else {
#endif
		riscv_idle(CHIP_PLL_DOZE, MSTATUS_IEN);
#ifdef CONFIG_SOC_IT51XXX_CPU_IDLE_GATING
	}
#endif
}

void arch_cpu_atomic_idle(unsigned int key)
{
	riscv_idle(CHIP_PLL_DOZE, key);
}

void soc_prep_hook(void)
{
	struct gpio_ite_ec_regs *const gpio_regs = GPIO_ITE_EC_REGS_BASE;
	struct gctrl_ite_ec_regs *const gctrl_regs = GCTRL_ITE_EC_REGS_BASE;

	/* Scratch SRAM0 uses the 4KB based form 0x801000h */
	gctrl_regs->GCTRL_SCR0BAR = IT51XXX_SEL_SRAM0_BASE_4K;

	/* bit4: wake up CPU if it is in low power mode and an interrupt is pending. */
	gctrl_regs->GCTRL_SPCTRL9 |= IT51XXX_GCTRL_ALTIE;

	/* UART1 and UART2 board init */
	/* bit3: UART1 and UART2 belong to the EC side. */
	gctrl_regs->GCTRL_RSTDMMC |= IT51XXX_GCTRL_UART1SD | IT51XXX_GCTRL_UART2SD;
	/* Reset UART before config it */
	gctrl_regs->GCTRL_RSTC4 = IT51XXX_GCTRL_RUART;
	/* Switch UART1 and UART2 on without hardware flow control */
	gpio_regs->GPIO_GCR1 |=
		IT51XXX_GPIO_U1CTRL_SIN0_SOUT0_EN | IT51XXX_GPIO_U2CTRL_SIN1_SOUT1_EN;
}
