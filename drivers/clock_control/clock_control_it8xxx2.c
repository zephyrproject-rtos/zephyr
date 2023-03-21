/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ite_it8xxx2_ecpm

#include <soc.h>
#include <zephyr/logging/log.h>
#include "soc_espi.h"
#include <zephyr/drivers/clock_control.h>
#include <ilm.h>
#include <zephyr/dt-bindings/clock/it8xxx2_clock.h>
#include <zephyr/drivers/clock_control/it8xxx2_clock_control.h>
#include <zephyr/arch/riscv/arch.h>

LOG_MODULE_REGISTER(clock_control_it8xxx2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define PLLFREQ_MASK BIT_MASK(4)
#define MCUFREQ_MASK BIT_MASK(3)
#define EC_FREQ_MASK BIT_MASK(4)
#define SMBFREQ_MASK BIT_MASK(4)
#define FNDFREQ_MASK GENMASK(7, 4)

struct clock_control_it8xxx2_config {
	struct ecpm_it8xxx2_regs *reg_base;
	int pll_freq;
};

const struct clock_control_it8xxx2_config *clock_config;

/* ITE IT8XXX2 ECPM Functions */
static inline int it8xxx2_clock_control_on(const struct device *dev,
					clock_control_subsys_t sub_system)
{
	const struct clock_control_it8xxx2_config *cfg = dev->config;
	struct ecpm_it8xxx2_regs *const ecpm_regs = cfg->reg_base;
	struct it8xxx2_clock_control_cells *clk_cfg =
		(struct it8xxx2_clock_control_cells *)(sub_system);

	uint8_t offset = clk_cfg->offset;
	uint8_t code = clk_cfg->bitmask;
	uint8_t always = clk_cfg->always;

	volatile uint8_t *reg =
		(volatile uint8_t *)((uint32_t)ecpm_regs + (uint32_t)offset);

	*reg = (sys_read8((mem_addr_t)reg) & ~code) | always;

	return 0;
}

static inline int it8xxx2_clock_control_off(const struct device *dev,
					clock_control_subsys_t sub_system)
{
	const struct clock_control_it8xxx2_config *cfg = dev->config;
	struct ecpm_it8xxx2_regs *const ecpm_regs = cfg->reg_base;
	struct it8xxx2_clock_control_cells *clk_cfg =
		(struct it8xxx2_clock_control_cells *)(sub_system);

	uint8_t offset = clk_cfg->offset;
	uint8_t code = clk_cfg->bitmask;
	uint8_t always = clk_cfg->always;

	volatile uint8_t *reg =
		(volatile uint8_t *)((uint32_t)ecpm_regs + (uint32_t)offset);

	*reg = sys_read8((mem_addr_t)reg) | (always | code);

	return 0;
}

const uint32_t pll_reg_to_freq[8] = {
		MHZ(8),  MHZ(16), MHZ(24), MHZ(32),
		MHZ(48), MHZ(64), MHZ(72), MHZ(96) };

const struct it8xxx2_clkctrl_freq mcu_clk_freq_tbl[] = {
		{.divisor = 1, .frequency = 0 },
		{.divisor = 2, .frequency = 0 },
		{.divisor = 3, .frequency = 0 },
		{.divisor = 4, .frequency = 0 },
		{.divisor = 5, .frequency = 0 },
		{.divisor = 6, .frequency = 0 },
		{.divisor = 0, .frequency = MHZ(2) },
		{.divisor = 0, .frequency = KHZ(32) }
};

static int it8xxx2_clock_control_get_rate(const struct device *dev,
					  clock_control_subsys_t sub_system,
					  uint32_t *rate)
{
	const struct clock_control_it8xxx2_config *const config = dev->config;
	struct ecpm_it8xxx2_regs *const ecpm_regs = config->reg_base;
	struct it8xxx2_clkctrl_subsys *subsys =
		(struct it8xxx2_clkctrl_subsys *)(sub_system);

	uint8_t get_rate_opt = subsys->clk_opt;

	uint8_t pll_clkfreq_sel_curr = ecpm_regs->PLLFREQR & PLLFREQ_MASK;
	uint8_t mcu_clkfreq_sel_curr;
	uint8_t smb_clkfreq_sel_curr;

	uint8_t mcu_current_div;
	uint8_t smb_current_div;

	uint32_t pll_freq = pll_reg_to_freq[pll_clkfreq_sel_curr];

	if (get_rate_opt >= IT8XXX2_COUNT) {
		return -ENOTSUP;
	}

	switch (get_rate_opt) {
	case IT8XXX2_NULL:
		LOG_ERR("Error: Please configure the subsystem\r\n");
		return -ENOTSUP;

	case IT8XXX2_PLL:
		*rate = pll_freq;
		break;

	case IT8XXX2_CPU:
		mcu_clkfreq_sel_curr = ecpm_regs->SCDCR0 & MCUFREQ_MASK;
		mcu_current_div = mcu_clk_freq_tbl[mcu_clkfreq_sel_curr].divisor;

		*rate = mcu_current_div ?
			(pll_freq / (uint32_t)mcu_current_div) :
			mcu_clk_freq_tbl[mcu_clkfreq_sel_curr].frequency;
		break;

	case IT8XXX2_SMB:
		smb_clkfreq_sel_curr = ecpm_regs->SCDCR2 & SMBFREQ_MASK;
		smb_current_div = smb_clkfreq_sel_curr + 1;
		*rate = pll_freq / (uint32_t)smb_current_div;
		break;

	default:
		LOG_ERR("Error: Specified Rate Not Supported\r\n");
		return -ENOTSUP;
	}

	return 0;
}


/* The options of the clock_control_get_rate() and clock_control_set_rate()
 *
 * 1. CPU Frequency:
 * The Clock Frequency of the IT8XXX2 processor can be determined
 * according to the PLL Frequency and the divisor which is defined by
 * the MCU Clock Frequency Select field in the register SCDCR0 of ECPM.
 *
 * 2. PLL Frequency:
 * We strongly suggest that the PLL frequency NOT be configured
 * in real-time for the PLL frequency change requires some specific
 * flows including making the EC enter the sleep mode, that is, during
 * the period when EC be in sleep mode, the system could be unpredictable.
 * Therefore, we only configure the PLL during the EC initialization in
 * the very beginning.
 *
 * 3. I2C/SMBus Source Clock Frequency:
 * We also strongly suggest that the I2C/SMBus Source Clock Frequency
 * NOT be configured.
 *
 * So far the it8xxx2_clock_control_set_rate() only supports the MCU clock
 * configuration but the source clock frequency of the I2C/SMBus. For there
 * are further configurations in I2C/SMBus registers which would put the
 * source clock frequency into division again accordingly, so it's worth
 * noticing that the source clock frequency of the I2C/SMBus is different
 * from the final Speed/Frequency of the I2C/SMBus.
 *
 * The final Sspeed/Frequency can only be obtained through related I2C/SMBus
 * registers. Besides, the change of the I2C/SMBus Source Clock Frequency
 * could lead that the frequency won't match the definitions in the register
 * SCLKTS_A, that is, the speed/frequency will be wrong in such a scenario.
 * Hence, we will still not let the configurations on I2C/SMBus Source Clock
 * Frequency be available in options.
 *
 * To sum up, the it8xxx2_clock_control_set_rate() is extended to have the
 * options in the sub_system but unfortunately it only supports the CPU
 * clock frequency adjustment in realtime so far. However, the modifications
 * about the sub_system are made for the possible extensions in the future.
 *
 */
static int it8xxx2_clock_control_set_rate(const struct device *dev,
					  clock_control_subsys_t sub_system,
					  clock_control_subsys_rate_t rate)
{

	const struct clock_control_it8xxx2_config *const config = dev->config;
	struct ecpm_it8xxx2_regs *const ecpm_regs = config->reg_base;
	struct it8xxx2_clkctrl_subsys *subsys =
		(struct it8xxx2_clkctrl_subsys *)(sub_system);

	struct it8xxx2_clkctrl_subsys_rate *subsys_rate =
		(struct it8xxx2_clkctrl_subsys_rate *)(rate);

	uint8_t get_rate_opt = subsys->clk_opt;

	uint8_t pll_clkfreq_sel_curr = ecpm_regs->PLLFREQR & PLLFREQ_MASK;
	uint32_t pll_freq = pll_reg_to_freq[pll_clkfreq_sel_curr];
	uint8_t mcu_current_div, divisor;
	uint32_t s_rate = subsys_rate->clk_rate;
	int mod;

	if (get_rate_opt != IT8XXX2_CPU) {
		LOG_ERR("Error: Specified Rate Option %d Not Supported\r\n",
			get_rate_opt);
		return -ENOTSUP;
	}

	if ((uint32_t)s_rate == 0) {
		LOG_ERR("Error: Specified Rate %d Not Supported\r\n", s_rate);
		return -ENOTSUP;
	}

	mcu_current_div = (ecpm_regs->SCDCR0 & MCUFREQ_MASK) + 1;
	divisor = (uint8_t)(pll_freq / s_rate);
	mod = (pll_freq % s_rate);

	if ((mod != 0) || (divisor > 6) || (divisor < 1)) {
		LOG_ERR("Error: Specified Rate Not Supported\r\n");
		return -ENOTSUP;
	}

	if (divisor == mcu_current_div) {
		LOG_ERR("Error: The current rate is already set.\r\n");
		return -EALREADY;
	}
	ecpm_regs->SCDCR0 |= ((divisor - 1) & MCUFREQ_MASK);

	return 0;
}

#ifdef CONFIG_SOC_IT8XXX2_PLL_FLASH_48M

struct pll_config_t {
	int pll_freq;
	uint8_t pll_setting;
	uint8_t div_fnd;
	uint8_t div_uart;
	uint8_t div_smb;
	uint8_t div_sspi;
	uint8_t div_ec;
	uint8_t div_jtag;
	uint8_t div_pwm;
	uint8_t div_usbpd;
};

const struct pll_config_t pll_configuration[] = {
	/* PLL:48MHz, MCU:48MHz, Fnd:48MHz */
	[PLL_48_MHZ] = {
		.pll_freq = 48000000,
		.pll_setting = 0x04,	/* PLL frequency setting = 4 (48MHz) */
		.div_fnd = 0x00,	/* FND   = 48 MHz (PLL / 1) */
		.div_uart = 0x01,	/* UART  = 24 MHz (PLL / 2) */
		.div_smb = 0x01,	/* SMB   = 24 MHz (PLL / 2) */
		.div_sspi = 0x00,	/* SSPI  = 48 MHz (PLL / 1) */
		.div_ec = 0x06,		/* EC    =  8 MHz (PLL / 6) */
		.div_jtag = 0x01,	/* JTAG  = 24 MHz (PLL / 2) */
		.div_pwm = 0x00,	/* PWM   = 48 MHz (PLL / 1) */
		.div_usbpd = 0x05	/* USBPD =  8 MHz (PLL / 6) */
	},
	/* PLL:96MHz, MCU:96MHz, Fnd:48MHz */
	[PLL_96_MHZ] = {
		.pll_freq = 96000000,
		.pll_setting = 0x07,	/* PLL frequency setting = 7 (96MHz) */
		.div_fnd = 0x01,	/* FND   = 48 MHz (PLL / 2) */
		.div_uart = 0x03,	/* UART  = 24 MHz (PLL / 4) */
		.div_smb = 0x03,	/* SMB   = 24 MHz (PLL / 4) */
		.div_sspi = 0x01,	/* SSPI  = 48 MHz (PLL / 2) */
		.div_ec = 0x06,		/* EC    = 16 MHz (PLL / 6) */
		.div_jtag = 0x03,	/* JTAG  = 24 MHz (PLL / 4) */
		.div_pwm = 0x01,	/* PWM   = 48 MHz (PLL / 2) */
		.div_usbpd = 0x0B	/* USBPD =  8 MHz (PLL / 12) */
	},
	/* IC Default:  */
};

void __soc_ram_code chip_pll_ctrl(enum chip_pll_mode mode)
{
	struct ecpm_it8xxx2_regs *const ecpm_regs = clock_config->reg_base;

	volatile uint8_t _pll_ctrl __unused;

	ecpm_regs->PLLCTRL = mode;
	/*
	 * for deep doze / sleep mode
	 * This load operation will ensure PLL setting is taken into
	 * control register before wait for interrupt instruction.
	 */
	_pll_ctrl = ecpm_regs->PLLCTRL;
}

void __soc_ram_code chip_run_pll_sequence(const struct pll_config_t *pll)
{
	struct ecpm_it8xxx2_regs *const ecpm_regs = clock_config->reg_base;

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
	ecpm_regs->PLLFREQR = pll->pll_setting;
	/* Pre-set FND clock frequency = PLL / 3 */
	ecpm_regs->SCDCR0 = (2 << 4);
	/* JTAG and EC */
	ecpm_regs->SCDCR3 = (pll->div_jtag << 4) | pll->div_ec;
	/* Chip sleep after wait for interrupt (wfi) instruction */
	chip_pll_ctrl(CHIP_PLL_SLEEP);
	/* Chip sleep and wait timer wake it up */
	__asm__ volatile  ("wfi");
	/* New FND clock frequency */
	ecpm_regs->SCDCR0 = pll->div_fnd << 4;
	/* Chip doze after wfi instruction */
	chip_pll_ctrl(CHIP_PLL_DOZE);
	/* UART */
	ecpm_regs->SCDCR1 = pll->div_uart;
	/* SSPI and SMB */
	ecpm_regs->SCDCR2 = (pll->div_sspi << 4) | pll->div_smb;
	/* USBPD and PWM */
	ecpm_regs->SCDCR4 = (pll->div_usbpd << 4) | pll->div_pwm;
}

static void chip_configure_pll(const struct pll_config_t *pll)
{
	struct ecpm_it8xxx2_regs *const ecpm_regs = clock_config->reg_base;

	/* Re-configure PLL clock or not. */
	if (((ecpm_regs->PLLFREQR & PLLFREQ_MASK) != pll->pll_setting) ||
	    ((ecpm_regs->SCDCR0 & FNDFREQ_MASK) != (pll->div_fnd << 4)) ||
	    ((ecpm_regs->SCDCR3 & EC_FREQ_MASK) != pll->div_ec)) {
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

#endif /* CONFIG_SOC_IT8XXX2_PLL_FLASH_48M */

static int it8xxx2_clock_control_init(const struct device *dev)
{
	clock_config = dev->config;
	int pllfreq = clock_config->pll_freq;
	struct ecpm_it8xxx2_regs *const ecpm_regs = clock_config->reg_base;

	/* SWUC Clock Gating */
	ecpm_regs->CGCTRL2R |= SWUC_CLOCK_GATING;
	/* PECI, SSPI, DBGR Clock Gating */
	ecpm_regs->CGCTRL3R |= PECI_SSPI_DBGR_CLOCK_GATING;
	/* SMB/I2C Clock Gating */
	ecpm_regs->CGCTRL4R |= SMB_ALL_CHANNELS_CLOCK_GATING;


	if (DT_NODE_HAS_STATUS(DT_NODELABEL(usbpd0), disabled)) {
		/* PD0 Clock Gating */
		ecpm_regs->CGCTRL5R |= PD0_CLOCK_GATING;
	}

	if (DT_NODE_HAS_STATUS(DT_NODELABEL(usbpd1), disabled)) {
		/* PD1 Clock Gating */
		ecpm_regs->CGCTRL5R |= PD1_CLOCK_GATING;
	}

#ifdef CONFIG_ESPI_IT8XXX2
	/* SPI SLAVE Clock Gating */
	ecpm_regs->CGCTRL5R |= SPI_SLAVE_CLOCK_GATING;
#endif /* #ifdef CONFIG_ESPI_IT8XXX2 */

#ifndef CONFIG_SOC_IT8XXX2_JTAG_ENABLE
	/* JTAG Clock Gating */
	ecpm_regs->CGCTRL6R |= JTAG_CHB_CLOCK_GATING;
#endif /* #ifdef CONFIG_SOC_IT8XXX2_JTAG_ENABLE */

#ifndef CONFIG_SOC_IT8XXX2_ENABLE_FPU
	/* FPU Clock Gating */
	ecpm_regs->CGCTRL6R |= FPU_CHA_CLOCK_GATING;
#endif

	/* PLL Config */
	if (pllfreq == PLL_DEFAULT) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_ITE_IT8XXX2_INTC)) {
		ite_intc_save_and_disable_interrupts();
	}
	/* configure PLL/CPU/flash clock */
	chip_configure_pll(&pll_configuration[pllfreq]);
	if (IS_ENABLED(CONFIG_ITE_IT8XXX2_INTC)) {
		ite_intc_restore_interrupts();
	}

	return 0;
}

/* Clock controller driver registration */
static struct clock_control_driver_api clock_control_it8xxx2_api = {
	.on = it8xxx2_clock_control_on,
	.off = it8xxx2_clock_control_off,
	.get_rate = it8xxx2_clock_control_get_rate,
	.set_rate = it8xxx2_clock_control_set_rate,
};

const struct clock_control_it8xxx2_config clock_control_it8xxx2_cfg = {
	.reg_base = (struct ecpm_it8xxx2_regs *)DT_INST_REG_ADDR(0),
	.pll_freq = DT_INST_PROP(0, pll_frequency),
};

DEVICE_DT_INST_DEFINE(0,
		      it8xxx2_clock_control_init,
		      NULL,
		      NULL,
		      &clock_control_it8xxx2_cfg,
		      PRE_KERNEL_1,
		      CONFIG_IT8XXX2_PLL_SEQUENCE_PRIORITY,
		      &clock_control_it8xxx2_api);
BUILD_ASSERT(CONFIG_FLASH_INIT_PRIORITY < CONFIG_IT8XXX2_PLL_SEQUENCE_PRIORITY,
	"CONFIG_FLASH_INIT_PRIORITY must be less than CONFIG_IT8XXX2_PLL_SEQUENCE_PRIORITY");
