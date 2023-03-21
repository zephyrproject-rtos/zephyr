/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/arch/riscv/csr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include "ilm.h"
#include <soc.h>
#include "soc_espi.h"
#include <zephyr/dt-bindings/interrupt-controller/ite-intc.h>
#include <zephyr/drivers/clock_control/it8xxx2_clock_control.h>

#ifdef CONFIG_SOC_IT8XXX2_CPU_IDLE_GATING
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
	return !!(atomic_get(&cpu_idle_disabled));
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
#ifdef CONFIG_ESPI
	/*
	 * H2RAM feature requires RAM clock to be active. Since the below doze
	 * mode will disable CPU and RAM clocks, enable eSPI transaction
	 * interrupt to restore clocks. With this interrupt, EC will not defer
	 * eSPI bus while transaction is accepted.
	 */
	espi_it8xxx2_enable_trans_irq(ESPI_IT8XXX2_SOC_DEV, true);
#endif
	/* Chip doze after wfi instruction */
	chip_pll_ctrl(mode);

	do {
		/* Wait for interrupt */
		__asm__ volatile ("wfi");
		/*
		 * Sometimes wfi instruction may fail due to CPU's MTIP@mip
		 * register is non-zero.
		 * If the ite_intc_no_irq() is true at this point,
		 * it means that EC waked-up by the above issue not an
		 * interrupt. Hence we loop running wfi instruction here until
		 * wfi success.
		 */
	} while (ite_intc_no_irq());

#ifdef CONFIG_ESPI
	/* CPU has been woken up, the interrupt is no longer needed */
	espi_it8xxx2_enable_trans_irq(ESPI_IT8XXX2_SOC_DEV, false);
#endif
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
#ifdef CONFIG_SOC_IT8XXX2_CPU_IDLE_GATING
	/*
	 * The EC processor(CPU) cannot be in the k_cpu_idle() during
	 * the transactions with the CQ mode(DMA mode). Otherwise,
	 * the EC processor would be clock gated.
	 */
	if (cpu_idle_not_allowed()) {
		/* Restore global interrupt lockout state */
		irq_unlock(MSTATUS_IEN);
	} else
#endif
	{
		riscv_idle(CHIP_PLL_DOZE, MSTATUS_IEN);
	}
}

void arch_cpu_atomic_idle(unsigned int key)
{
	riscv_idle(CHIP_PLL_DOZE, key);
}

static int ite_it8xxx2_init(void)
{
	struct gpio_it8xxx2_regs *const gpio_regs = GPIO_IT8XXX2_REG_BASE;
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;

	struct ecpm_it8xxx2_regs *const ecpm_base = ECPM_IT8XXX2_REG_BASE;

	/*
	 * bit7: wake up CPU if it is in low power mode and
	 * an interrupt is pending.
	 */
	gctrl_regs->GCTRL_WMCR |= BIT(7);

	/*
	 * Disable this feature that can detect pre-define hardware
	 * target A through I2C0. This is for debugging use, so it
	 * can be disabled to avoid illegal access.
	 */
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
	IT8XXX2_SMB_SFFCTL &= ~IT8XXX2_SMB_HSAPE;
#elif CONFIG_SOC_IT8XXX2_REG_SET_V2
	IT8XXX2_SMB_SCLKTS_BRGS &= ~IT8XXX2_SMB_PREDEN;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* UART1 board init */
	/* bit2: clocks to UART1 modules are not gated. */
	ecpm_base->CGCTRL3R &= ~BIT(2);
	ecpm_base->AUTOCG &= ~BIT(6);

	/* bit3: UART1 belongs to the EC side. */
	gctrl_regs->GCTRL_RSTDMMC |= IT8XXX2_GCTRL_UART1SD;
	/* reset UART before config it */
	gctrl_regs->GCTRL_RSTC4 = IT8XXX2_GCTRL_RUART1;

	/* switch UART1 on without hardware flow control */
	gpio_regs->GPIO_GCR1 |= IT8XXX2_GPIO_U1CTRL_SIN0_SOUT0_EN;

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* UART2 board init */
	/* setting voltage 3.3v */
	gpio_regs->GPIO_GCR21 &= ~(IT8XXX2_GPIO_GPH1VS | IT8XXX2_GPIO_GPH2VS);
	/* bit2: clocks to UART2 modules are not gated. */
	ecpm_base->CGCTRL3R &= ~BIT(2);
	ecpm_base->AUTOCG &= ~BIT(5);

	/* bit3: UART2 belongs to the EC side. */
	gctrl_regs->GCTRL_RSTDMMC |= IT8XXX2_GCTRL_UART2SD;
	/* reset UART before config it */
	gctrl_regs->GCTRL_RSTC4 = IT8XXX2_GCTRL_RUART2;

	/* switch UART2 on without hardware flow control */
	gpio_regs->GPIO_GCR1 |= IT8XXX2_GPIO_U2CTRL_SIN1_SOUT1_EN;

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */

#if (SOC_USBPD_ITE_PHY_PORT_COUNT > 0)
	int port;

	/*
	 * To prevent cc pins leakage, we disable board not active ITE
	 * TCPC port cc modules, then cc pins can be used as gpio if needed.
	 */
	for (port = SOC_USBPD_ITE_ACTIVE_PORT_COUNT;
	     port < SOC_USBPD_ITE_PHY_PORT_COUNT; port++) {
		struct usbpd_it8xxx2_regs *base;

		if (port == 0) {
			base = (struct usbpd_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(usbpd0));
		} else if (port == 1) {
			base = (struct usbpd_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(usbpd1));
		} else {
			/* Currently all ITE embedded pd chip support max two ports */
			break;
		}

		/* Power down all CC, and disable CC voltage detector */
		base->CCGCR |= (IT8XXX2_USBPD_DISABLE_CC |
				IT8XXX2_USBPD_DISABLE_CC_VOL_DETECTOR);
		/*
		 * Disconnect CC analog module (ex.UP/RD/DET/TX/RX), and
		 * disconnect CC 5.1K to GND
		 */
		base->CCCSR |= (IT8XXX2_USBPD_CC2_DISCONNECT |
				IT8XXX2_USBPD_CC2_DISCONNECT_5_1K_TO_GND |
				IT8XXX2_USBPD_CC1_DISCONNECT |
				IT8XXX2_USBPD_CC1_DISCONNECT_5_1K_TO_GND);
		/* Disconnect CC 5V tolerant */
		base->CCPSR |= (IT8XXX2_USBPD_DISCONNECT_POWER_CC2 |
				IT8XXX2_USBPD_DISCONNECT_POWER_CC1);
		/* Dis-connect 5.1K dead battery resistor to CC */
		base->CCPSR |= (IT8XXX2_USBPD_DISCONNECT_5_1K_CC2_DB |
				IT8XXX2_USBPD_DISCONNECT_5_1K_CC1_DB);
	}
#endif /* (SOC_USBPD_ITE_PHY_PORT_COUNT > 0) */

	return 0;
}
SYS_INIT(ite_it8xxx2_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
