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
#include <soc_common.h>
#include "soc_espi.h"
#include <zephyr/dt-bindings/interrupt-controller/ite-intc.h>

/*
 * This define gets the total number of USBPD ports available on the
 * ITE EC chip from dtsi (include status disable). Both it81202 and
 * it81302 support two USBPD ports.
 */
#define SOC_USBPD_ITE_PHY_PORT_COUNT \
COND_CODE_1(DT_NODE_EXISTS(DT_INST(1, ite_it8xxx2_usbpd)), (2), (1))

/*
 * This define gets the number of active USB Power Delivery (USB PD)
 * ports in use on the ITE microcontroller from dts (only status okay).
 * The active port usage should follow the order of ITE TCPC port index,
 * ex. if we're active only one ITE USB PD port, then the port should be
 * 0x3700 (port0 register base), instead of 0x3800 (port1 register base).
 */
#define SOC_USBPD_ITE_ACTIVE_PORT_COUNT DT_NUM_INST_STATUS_OKAY(ite_it8xxx2_usbpd)

uint32_t chip_get_pll_freq(void)
{
	uint32_t pllfreq;

	switch (IT8XXX2_ECPM_PLLFREQR & 0x0F) {
	case 0:
		pllfreq = MHZ(8);
		break;
	case 1:
		pllfreq = MHZ(16);
		break;
	case 2:
		pllfreq = MHZ(24);
		break;
	case 3:
		pllfreq = MHZ(32);
		break;
	case 4:
		pllfreq = MHZ(48);
		break;
	case 5:
		pllfreq = MHZ(64);
		break;
	case 6:
		pllfreq = MHZ(72);
		break;
	case 7:
		pllfreq = MHZ(96);
		break;
	default:
		return -ERANGE;
	}

	return pllfreq;
}

void __soc_ram_code chip_pll_ctrl(enum chip_pll_mode mode)
{
	volatile uint8_t _pll_ctrl __unused;

	IT8XXX2_ECPM_PLLCTRL = mode;
	/*
	 * for deep doze / sleep mode
	 * This load operation will ensure PLL setting is taken into
	 * control register before wait for interrupt instruction.
	 */
	_pll_ctrl = IT8XXX2_ECPM_PLLCTRL;
}

#ifdef CONFIG_SOC_IT8XXX2_PLL_FLASH_48M
struct pll_config_t {
	uint8_t pll_freq;
	uint8_t div_fnd;
	uint8_t div_uart;
	uint8_t div_smb;
	uint8_t div_sspi;
	uint8_t div_ec;
	uint8_t div_jtag;
	uint8_t div_pwm;
	uint8_t div_usbpd;
};

static const struct pll_config_t pll_configuration[] = {
	/*
	 * PLL frequency setting = 4 (48MHz)
	 * FND   div = 0 (PLL / 1 = 48 mhz)
	 * UART  div = 1 (PLL / 2 = 24 mhz)
	 * SMB   div = 1 (PLL / 2 = 24 mhz)
	 * SSPI  div = 1 (PLL / 2 = 24 mhz)
	 * EC    div = 6 (FND / 6 =  8 mhz)
	 * JTAG  div = 1 (PLL / 2 = 24 mhz)
	 * PWM   div = 0 (PLL / 1 = 48 mhz)
	 * USBPD div = 5 (PLL / 6 =  8 mhz)
	 */
	{.pll_freq  = 4,
	 .div_fnd   = 0,
	 .div_uart  = 1,
	 .div_smb   = 1,
	 .div_sspi  = 1,
#ifdef CONFIG_SOC_IT8XXX2_EC_BUS_24MHZ
	 .div_ec    = 1,
#else
	 .div_ec    = 6,
#endif
	 .div_jtag  = 1,
	 .div_pwm   = 0,
	 .div_usbpd = 5}
};

void __soc_ram_code chip_run_pll_sequence(const struct pll_config_t *pll)
{
	/* Enable HW timer to wakeup chip from the sleep mode */
	timer_5ms_one_shot();
	/*
	 * Configure PLL clock dividers.
	 * Writing data to these registers doesn't change the
	 * PLL frequency immediately until the status is changed
	 * into wakeup from the sleep mode.
	 * The following code is intended to make the system
	 * enter sleep mode, and wait HW timer to wakeup chip to
	 * complete PLL update.
	 */
	IT8XXX2_ECPM_PLLFREQR = pll->pll_freq;
	/* Pre-set FND clock frequency = PLL / 3 */
	IT8XXX2_ECPM_SCDCR0 = (2 << 4);
	/* JTAG and EC */
	IT8XXX2_ECPM_SCDCR3 = (pll->div_jtag << 4) | pll->div_ec;
	/* Chip sleep after wait for interrupt (wfi) instruction */
	chip_pll_ctrl(CHIP_PLL_SLEEP);
	/* Chip sleep and wait timer wake it up */
	__asm__ volatile  ("wfi");
	/* New FND clock frequency */
	IT8XXX2_ECPM_SCDCR0 = pll->div_fnd << 4;
	/* Chip doze after wfi instruction */
	chip_pll_ctrl(CHIP_PLL_DOZE);
	/* UART */
	IT8XXX2_ECPM_SCDCR1 = pll->div_uart;
	/* SSPI and SMB */
	IT8XXX2_ECPM_SCDCR2 = (pll->div_sspi << 4) | pll->div_smb;
	/* USBPD and PWM */
	IT8XXX2_ECPM_SCDCR4 = (pll->div_usbpd << 4) | pll->div_pwm;
}

static void chip_configure_pll(const struct pll_config_t *pll)
{
	/* Re-configure PLL clock or not. */
	if (((IT8XXX2_ECPM_PLLFREQR & 0xf) != pll->pll_freq) ||
		((IT8XXX2_ECPM_SCDCR0 & 0xf0) != (pll->div_fnd << 4)) ||
		((IT8XXX2_ECPM_SCDCR3 & 0xf) != pll->div_ec)) {
#ifdef CONFIG_ESPI
		/*
		 * We have to disable eSPI pad before changing
		 * PLL sequence or sequence will fail if CS# pin is low.
		 */
		espi_it8xxx2_enable_pad_ctrl(ESPI_IT8XXX2_SOC_DEV, false);
#endif
		/* Run change PLL sequence */
		chip_run_pll_sequence(pll);
#ifdef CONFIG_ESPI
		/* Enable eSPI pad after changing PLL sequence */
		espi_it8xxx2_enable_pad_ctrl(ESPI_IT8XXX2_SOC_DEV, true);
#endif
	}
}

static int chip_change_pll(void)
{

	if (IS_ENABLED(CONFIG_HAS_ITE_INTC)) {
		ite_intc_save_and_disable_interrupts();
	}
	/* configure PLL/CPU/flash clock */
	chip_configure_pll(&pll_configuration[0]);
	if (IS_ENABLED(CONFIG_HAS_ITE_INTC)) {
		ite_intc_restore_interrupts();
	}

	return 0;
}
SYS_INIT(chip_change_pll, PRE_KERNEL_1, CONFIG_IT8XXX2_PLL_SEQUENCE_PRIORITY);
BUILD_ASSERT(CONFIG_FLASH_INIT_PRIORITY < CONFIG_IT8XXX2_PLL_SEQUENCE_PRIORITY,
	"CONFIG_FLASH_INIT_PRIORITY must be less than CONFIG_IT8XXX2_PLL_SEQUENCE_PRIORITY");
#endif /* CONFIG_SOC_IT8XXX2_PLL_FLASH_48M */

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
#ifndef CONFIG_SOC_IT8XXX2_JTAG_DEBUG_INTERFACE
		/* Wait for interrupt */
		__asm__ volatile ("wfi");
#endif
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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usb0), disabled)
	struct usb_it82xx2_regs *const usb_regs = USB_IT82XX2_REGS_BASE;

	usb_regs->port0_misc_control &= ~PULL_DOWN_EN;
	usb_regs->port1_misc_control &= ~PULL_DOWN_EN;
#endif

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
	/*
	 * Setting this bit will disable EGAD pin output driving to avoid
	 * leakage when GPIO E1/E2 on it82002 are set to alternate function.
	 */
	IT8XXX2_EGPIO_EGCR |= IT8XXX2_EGPIO_EEPODD;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* UART1 board init */
	/* bit2: clocks to UART1 modules are not gated. */
	IT8XXX2_ECPM_CGCTRL3R &= ~BIT(2);
	IT8XXX2_ECPM_AUTOCG &= ~BIT(6);

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
	IT8XXX2_ECPM_CGCTRL3R &= ~BIT(2);
	IT8XXX2_ECPM_AUTOCG &= ~BIT(5);

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
