/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/nvmem.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/dt-bindings/clock/nxp_mcxw7x_clock.h>

#include <fsl_clock.h>
#include <fsl_ccm32k.h>
#include <fsl_spc.h>

#if defined(CONFIG_PM_S2RAM) && \
	(defined(CONFIG_SOC_MCXW716C) || defined(CONFIG_SOC_MCXW727C))
#include <zephyr/linker/sections.h>
#include <soc.h>
#endif

#define DT_DRV_COMPAT nxp_mcxw7x_clock

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
LOG_MODULE_REGISTER(clock_control);

#define MCXW7_SPC_ADDR ((SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc)))

extern uint32_t SystemCoreClock;

/*
 * Mapping table from device tree clock IP mux values to HAL clock IP source values.
 * Used to translate DT-defined mux selections to the corresponding FSL SDK clock source.
 */
static clock_ip_src_t ip_clk_mux_mapping[] = {
	[MCXW_CLK_IP_MUX_FRO_6M] = kCLOCK_IpSrcFro6M,
	[MCXW_CLK_IP_MUX_FRO_192M_DIV] = kCLOCK_IpSrcFro192M,
	[MCXW_CLK_IP_MUX_SOSC] = kCLOCK_IpSrcSoscClk,
	[MCXW_CLK_IP_MUX_32K] = kCLOCK_IpSrc32kClk,
#if defined(CONFIG_SOC_MCXW70AC)
	[MCXW_CLK_IP_MUX_FRO_200M_DIV] = kCLOCK_IpSrcFro200M,
	[MCXW_CLK_IP_MUX_1M] = kCLOCK_IpSrc1M,
#endif /* CONFIG_SOC_MCXW70AC */
};

struct mcxw_clock_control_subsys_info {
	uint32_t flags: 4;
	uint32_t div: 4;
	uint32_t mux: 8;
	uint32_t offset: 16;
};

struct mcxw_clock_control_config {
	/* OSC32K mode configuration */
	uint8_t osc32k_mode;
	/* OSC32K XTAL capacitance configuration */
	ccm32k_osc_xtal_cap_t xtal_cap;
	/* OSC32K EXTAL capacitance configuration */
	ccm32k_osc_extal_cap_t extal_cap;
	/* OSC32K amplifier coarse gain adjustment */
	ccm32k_osc_coarse_adjustment_value_t coarse_adjustment;

	/* FIRC mode configuration */
	uint8_t firc_mode;
	/* FIRC frequency range configuration */
	uint8_t firc_range;

#if DT_INST_PROP(0, sirc_supported)
	/* Enable SIRC in low power mode */
	bool enable_sirc_in_lp_mode;
#endif

	/* System clock source selection */
	uint8_t sys_clk_src;
#if DT_INST_NODE_HAS_PROP(0, sys_clk_div_plat)
	/* System clock divider for platform clock (MCXW70 only) */
	uint8_t sys_clk_div_plat;
#endif
	/* System clock divider for slow clock */
	uint8_t sys_clk_div_slow;
	/* System clock divider for bus clock */
	uint8_t sys_clk_div_bus;
	/* System clock divider for core clock */
	uint8_t sys_clk_div_core;

	/* System oscillator frequency in Hz */
	uint32_t sosc_freq;
	/* Enable system oscillator */
	bool enable_sosc;
	/* Expected core clock frequency in Hz */
	uint32_t core_clock_frequency;
};

static int nxp_mcxw_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return 0;
	}

	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);

	if ((subsys_info->flags & MCXW_CLK_LP_NO_STALL) != 0) {
		CLOCK_EnableClockLPMode(ip_name, (clock_ip_control_t)MRCC_CC(subsys_info->flags));
	} else {
		CLOCK_EnableClock(ip_name);
	}

	return 0;
}

static int nxp_mcxw_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return -ENOTSUP;
	}

	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);
	CLOCK_DisableClock(ip_name);

	return 0;
}

static int nxp_mcxw_clock_control_get_rate(const struct device *dev,
					   clock_control_subsys_t sub_system, uint32_t *rate)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;
	if (subsys_info == NULL || rate == NULL) {
		return -EINVAL;
	}

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return -ENOTSUP;
	}

	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);

	if ((ip_name == kCLOCK_Wdog0) || (ip_name == kCLOCK_Wdog1)) {
		switch (subsys_info->mux) {
		case MCXW_CLK_IP_MUX_SLOW_CLK: {
			*rate = CLOCK_GetFreq(kCLOCK_SlowClk);
			break;
		}
		case MCXW_CLK_IP_MUX_SOSC: {
			*rate = CLOCK_GetFreq(kCLOCK_ScgSysOscClk);
			break;
		}
		case MCXW_CLK_IP_MUX_32K: {
			*rate = CLOCK_GetFreq(kCLOCK_RtcOscClk);
			break;
		}
		case MCXW_CLK_IP_MUX_FRO_6M: {
#if DT_INST_PROP(0, sirc_supported)
			*rate = CLOCK_GetFreq(kCLOCK_ScgSircClk);
#else
			/* SIRC not present on this SoC; FRO6M runs at a fixed 6 MHz */
			*rate = 6000000U;
#endif
			break;
		}
		default: {
			LOG_ERR("Unsupported clock source for watchdog");
			return -ENOTSUP;
		}
		}
	} else {
		*rate = CLOCK_GetIpFreq(ip_name);
	}

	return 0;
}

static int nxp_mcxw_clock_control_configure(const struct device *dev,
					    clock_control_subsys_t sub_system, void *data)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock configure. */
		LOG_INF("Clock configure not supported for this subsystem, using default settings");
		return 0;
	}

	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);

	/*
	 * Watchdog clock source is configured via its own WDOG_CS register,
	 * not through MRCC. Skip MRCC mux/div configuration for watchdog.
	 */
	if ((ip_name == kCLOCK_Wdog0) || (ip_name == kCLOCK_Wdog1)) {
		return 0;
	}

	if (subsys_info->mux != MCXW_CLK_IP_MUX_NONE) {
		CLOCK_SetIpSrc(ip_name, ip_clk_mux_mapping[subsys_info->mux]);
		CLOCK_SetIpSrcDiv(ip_name, subsys_info->div);
	}

	return 0;
}

static enum clock_control_status
nxp_mcxw_clock_control_get_status(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info == NULL) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);

	uint32_t reg = CLOCK_REG(ip_name);

	if ((reg & MRCC_CC_MASK) == 0) {
		return CLOCK_CONTROL_STATUS_OFF;
	} else {
		return CLOCK_CONTROL_STATUS_ON;
	}
}

static int nxp_mcxw_clock_control_pm(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);
	return 0;
}

#if defined(CONFIG_PM_S2RAM) && \
	(defined(CONFIG_SOC_MCXW716C) || defined(CONFIG_SOC_MCXW727C))
#define MCXW_CLOCK_POLL_LIMIT 1000000U
#define MCXW_RUN_CLOCK_FIELDS (SCG_RCCR_SCS_MASK | SCG_RCCR_DIVCORE_MASK | \
			       SCG_RCCR_DIVBUS_MASK | SCG_RCCR_DIVSLOW_MASK)
#define MCXW_CURRENT_CLOCK_FIELDS (SCG_CSR_SCS_MASK | SCG_CSR_DIVCORE_MASK | \
				   SCG_CSR_DIVBUS_MASK | SCG_CSR_DIVSLOW_MASK)
#define MCXW_FIRC_CONTROLS (SCG_FIRCCSR_FIRCEN_MASK | SCG_FIRCCSR_FIRCSTEN_MASK | \
			   SCG_FIRCCSR_FIRCTREN_MASK | SCG_FIRCCSR_FIRCTRUP_MASK | \
			   SCG_FIRCCSR_COARSE_TRIM_BYPASS_MASK)
#define MCXW_FIRC_TRIM_FIELDS (SCG_FIRCTCFG_TRIMSRC_MASK | SCG_FIRCTCFG_TRIMDIV_MASK)
#define MCXW_SOSC_CONTROLS (SCG_SOSCCSR_SOSCEN_MASK | SCG_SOSCCSR_SOSCSTEN_MASK | \
			   SCG_SOSCCSR_SOSCCM_MASK | SCG_SOSCCSR_SOSCCMRE_MASK)

#define MRCC_CLOCK(member) \
	{offsetof(MRCC_Type, member), MRCC_##member##_CC_MASK}
#define MRCC_RESET(member) \
	{offsetof(MRCC_Type, member), MRCC_##member##_CC_MASK | MRCC_##member##_RSTB_MASK}
#define MRCC_ROOT(member) \
	{offsetof(MRCC_Type, member), MRCC_##member##_CC_MASK | MRCC_##member##_RSTB_MASK | \
				    MRCC_##member##_MUX_MASK | MRCC_##member##_DIV_MASK}

/*
 * Module domains do not establish MRCC cell retention. Guard implemented
 * writable fields individually, including the entry-owned SECSUBSYS reset.
 * Clock-only memory/timer cells have no reset replay; RF BIST reads can fault.
 */
static const struct {
	uint16_t offset;
	uint32_t mask;
} mrcc_context_registers[] = {
	MRCC_RESET(MRCC_SECSUBSYS),
	MRCC_RESET(MRCC_EWM0), MRCC_CLOCK(MRCC_SYSPM0), MRCC_RESET(MRCC_SFA0),
	MRCC_RESET(MRCC_CRC0), MRCC_ROOT(MRCC_LPIT0),
#if defined(CONFIG_SOC_MCXW716C)
	MRCC_CLOCK(MRCC_TSTMR0),
#endif
	MRCC_ROOT(MRCC_TPM0), MRCC_ROOT(MRCC_TPM1), MRCC_ROOT(MRCC_LPI2C0),
	MRCC_ROOT(MRCC_LPI2C1), MRCC_ROOT(MRCC_I3C0), MRCC_ROOT(MRCC_LPSPI0),
	MRCC_ROOT(MRCC_LPSPI1), MRCC_ROOT(MRCC_LPUART0), MRCC_ROOT(MRCC_LPUART1),
	MRCC_ROOT(MRCC_FLEXIO0), MRCC_ROOT(MRCC_CAN0), MRCC_RESET(MRCC_SEMA0),
	MRCC_RESET(MRCC_DATA_STREAM_2P4), MRCC_RESET(MRCC_PORTA), MRCC_RESET(MRCC_PORTB),
	MRCC_RESET(MRCC_PORTC), MRCC_ROOT(MRCC_LPADC0), MRCC_RESET(MRCC_LPCMP0),
	MRCC_RESET(MRCC_LPCMP1), MRCC_RESET(MRCC_VREF0), MRCC_RESET(MRCC_GPIOA),
	MRCC_RESET(MRCC_GPIOB), MRCC_RESET(MRCC_GPIOC), MRCC_RESET(MRCC_DMA0),
	MRCC_CLOCK(MRCC_PFLEXNVM), MRCC_CLOCK(MRCC_SRAM0),
#if defined(CONFIG_SOC_MCXW716C)
	MRCC_CLOCK(MRCC_SRAM1),
#else
	MRCC_CLOCK(MRCC_SRAM1_ECC), MRCC_CLOCK(MRCC_MTR_MASTER), MRCC_ROOT(MRCC_CAN1),
	MRCC_CLOCK(MRCC_SRAM0_NOECC), MRCC_CLOCK(MRCC_SRAM1_NOECC),
	MRCC_RESET(MRCC_DSP0), MRCC_RESET(MRCC_DSP0_MUA),
	MRCC_CLOCK(MRCC_LPTMR0_IPG_CLK_ERCLK_WAKE2VSYS),
	MRCC_CLOCK(MRCC_LPTMR1_IPG_CLK_ERCLK_WAKE2VSYS),
	MRCC_CLOCK(MRCC_LPTMR2_IPG_CLK_ERCLK_WAKE2VSYS),
#endif
	MRCC_CLOCK(MRCC_SRAM2), MRCC_CLOCK(MRCC_SRAM3),
};

static struct {
	uint32_t mrcc[ARRAY_SIZE(mrcc_context_registers)];
	uint32_t rccr;
	uint32_t firccsr;
	uint32_t firccfg;
	uint32_t firctcfg;
	uint32_t fircstat;
	uint32_t sosccsr;
#if defined(CONFIG_SOC_MCXW716C)
	uint32_t sirccsr;
#endif
	uint32_t clock_out;
	uint32_t core_frequency;
	uint32_t flash_wait_states;
} clock_context __noinit;

static volatile uint32_t *mrcc_context_register(size_t index)
{
	return (volatile uint32_t *)((uintptr_t)MRCC + mrcc_context_registers[index].offset);
}

static int clock_wait_register(const volatile uint32_t *reg, uint32_t mask, uint32_t expected)
{
	for (uint32_t i = 0; i < MCXW_CLOCK_POLL_LIMIT; i++) {
		if ((*reg & mask) == expected) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int restore_scg_control(volatile uint32_t *reg, uint32_t controls,
			       uint32_t lock, uint32_t saved)
{
	uint32_t current = *reg & (controls | lock);

	if (current == saved) {
		return 0;
	}
	/* Do not copy read-only or write-one-to-clear status into CSR writes. */
	if ((current & lock) != 0) {
		*reg = current & ~lock;
		if ((*reg & lock) != 0) {
			return -EIO;
		}
	}
	*reg = saved;
	return (*reg & (controls | lock)) == saved ? 0 : -EIO;
}

void nxp_mcxw7x_clock_save_context(void)
{
	clock_context.rccr = SCG0->RCCR & MCXW_RUN_CLOCK_FIELDS;
	clock_context.firccsr = SCG0->FIRCCSR & (MCXW_FIRC_CONTROLS | SCG_FIRCCSR_LK_MASK);
	clock_context.firccfg = SCG0->FIRCCFG & SCG_FIRCCFG_RANGE_MASK;
	clock_context.firctcfg = SCG0->FIRCTCFG & MCXW_FIRC_TRIM_FIELDS;
	clock_context.fircstat = SCG0->FIRCSTAT & SCG_FIRCSTAT_TRIMFINE_MASK;
	clock_context.sosccsr = SCG0->SOSCCSR & (MCXW_SOSC_CONTROLS | SCG_SOSCCSR_LK_MASK);
#if defined(CONFIG_SOC_MCXW716C)
	clock_context.sirccsr = SCG0->SIRCCSR & (SCG_SIRCCSR_SIRCSTEN_MASK |
					      SCG_SIRCCSR_LK_MASK);
#endif
	clock_context.clock_out = SCG0->CLKOUTCNFG & SCG_CLKOUTCNFG_CLKOUTSEL_MASK;
	clock_context.core_frequency = SystemCoreClock;
	clock_context.flash_wait_states = FMU0->FCTRL & FMU_FCTRL_RWSC_MASK;
	for (size_t i = 0; i < ARRAY_SIZE(mrcc_context_registers); i++) {
		clock_context.mrcc[i] = *mrcc_context_register(i) & mrcc_context_registers[i].mask;
	}
}

static bool firc_configuration_changed(void)
{
	return (SCG0->FIRCCSR & MCXW_FIRC_CONTROLS) !=
	       (clock_context.firccsr & MCXW_FIRC_CONTROLS) ||
	       (SCG0->FIRCCFG & SCG_FIRCCFG_RANGE_MASK) != clock_context.firccfg ||
	       (SCG0->FIRCTCFG & MCXW_FIRC_TRIM_FIELDS) != clock_context.firctcfg ||
	       /* With auto-trim enabled, TRIMFINE is live calibration, not configuration. */
	       ((clock_context.firccsr & SCG_FIRCCSR_FIRCTREN_MASK) == 0 &&
		(SCG0->FIRCSTAT & SCG_FIRCSTAT_TRIMFINE_MASK) != clock_context.fircstat);
}

int nxp_mcxw7x_clock_restore_context(void)
{
	const uint32_t safe_clock = SCG_RCCR_SCS(kSCG_SysClkSrcSirc) |
				   SCG_RCCR_DIVCORE(kSCG_SysClkDivBy1) |
				   SCG_RCCR_DIVBUS(kSCG_SysClkDivBy1) |
				   SCG_RCCR_DIVSLOW(kSCG_SysClkDivBy4);
	bool firc_changed = firc_configuration_changed();
	bool sosc_changed = (SCG0->SOSCCSR & MCXW_SOSC_CONTROLS) !=
			    (clock_context.sosccsr & MCXW_SOSC_CONTROLS);
	bool clock_changed = firc_changed || sosc_changed ||
			     (SCG0->RCCR & MCXW_RUN_CLOCK_FIELDS) != clock_context.rccr ||
			     (SCG0->CSR & MCXW_CURRENT_CLOCK_FIELDS) != clock_context.rccr;
	int ret;

	if (clock_changed) {
		SCG0->RCCR = safe_clock;
		ret = clock_wait_register(&SCG0->CSR, MCXW_CURRENT_CLOCK_FIELDS, safe_clock);
		if (ret != 0) {
			return ret;
		}
	}
	if (firc_changed) {
		ret = restore_scg_control(&SCG0->FIRCCSR, MCXW_FIRC_CONTROLS,
			SCG_FIRCCSR_LK_MASK, (SCG0->FIRCCSR & MCXW_FIRC_CONTROLS) &
					   ~SCG_FIRCCSR_FIRCEN_MASK);
		if (ret != 0) {
			return ret;
		}
		ret = clock_wait_register(&SCG0->FIRCCSR, SCG_FIRCCSR_FIRCVLD_MASK, 0);
		if (ret != 0) {
			return ret;
		}
		SCG0->FIRCCFG = clock_context.firccfg;
		SCG0->FIRCTCFG = clock_context.firctcfg;
		if ((clock_context.firccsr & SCG_FIRCCSR_FIRCTREN_MASK) == 0) {
			SCG0->FIRCSTAT = clock_context.fircstat;
		}
		ret = restore_scg_control(&SCG0->FIRCCSR, MCXW_FIRC_CONTROLS,
			SCG_FIRCCSR_LK_MASK, clock_context.firccsr & ~SCG_FIRCCSR_LK_MASK);
		if (ret != 0) {
			return ret;
		}
	}
	if ((clock_context.firccsr & SCG_FIRCCSR_FIRCEN_MASK) != 0) {
		ret = clock_wait_register(&SCG0->FIRCCSR, SCG_FIRCCSR_FIRCVLD_MASK,
					  SCG_FIRCCSR_FIRCVLD_MASK);
		if (ret != 0) {
			return ret;
		}
	}
	if (sosc_changed) {
		ret = restore_scg_control(&SCG0->SOSCCSR, MCXW_SOSC_CONTROLS,
			SCG_SOSCCSR_LK_MASK, clock_context.sosccsr & ~SCG_SOSCCSR_LK_MASK);
		if (ret != 0) {
			return ret;
		}
	}
	if ((clock_context.sosccsr & SCG_SOSCCSR_SOSCEN_MASK) != 0) {
		ret = clock_wait_register(&SCG0->SOSCCSR, SCG_SOSCCSR_SOSCVLD_MASK,
					  SCG_SOSCCSR_SOSCVLD_MASK);
		if (ret != 0) {
			return ret;
		}
	}
	ret = clock_wait_register(&MCXW7_SPC_ADDR->SC, SPC_SC_BUSY_MASK, 0);
	if (ret != 0) {
		return ret;
	}
	if ((FMU0->FCTRL & FMU_FCTRL_RWSC_MASK) != clock_context.flash_wait_states) {
		FMU0->FCTRL = (FMU0->FCTRL & ~FMU_FCTRL_RWSC_MASK) |
			      clock_context.flash_wait_states;
	}
	if ((FMU0->FCTRL & FMU_FCTRL_RWSC_MASK) != clock_context.flash_wait_states) {
		return -EIO;
	}
	__DSB();
	if (clock_changed) {
		SCG0->RCCR = clock_context.rccr;
	}
	ret = clock_wait_register(&SCG0->CSR, MCXW_CURRENT_CLOCK_FIELDS, clock_context.rccr);
	if (ret != 0) {
		return ret;
	}
#if defined(CONFIG_SOC_MCXW716C)
	ret = restore_scg_control(&SCG0->SIRCCSR, SCG_SIRCCSR_SIRCSTEN_MASK,
				 SCG_SIRCCSR_LK_MASK, clock_context.sirccsr);
	if (ret != 0) {
		return ret;
	}
#endif
	ret = restore_scg_control(&SCG0->FIRCCSR, MCXW_FIRC_CONTROLS,
				 SCG_FIRCCSR_LK_MASK, clock_context.firccsr);
	if (ret != 0) {
		return ret;
	}
	ret = restore_scg_control(&SCG0->SOSCCSR, MCXW_SOSC_CONTROLS,
				 SCG_SOSCCSR_LK_MASK, clock_context.sosccsr);
	if (ret != 0) {
		return ret;
	}
	if ((SCG0->CLKOUTCNFG & SCG_CLKOUTCNFG_CLKOUTSEL_MASK) != clock_context.clock_out) {
		SCG0->CLKOUTCNFG = clock_context.clock_out;
	}
	for (size_t i = 0; i < ARRAY_SIZE(mrcc_context_registers); i++) {
		volatile uint32_t *reg = mrcc_context_register(i);
		uint32_t current = *reg & mrcc_context_registers[i].mask;
		uint32_t saved = clock_context.mrcc[i];
		uint32_t selectors = mrcc_context_registers[i].mask & (MRCC_MUX_MASK |
								     MRCC_DIV_MASK);

		if (current == saved) {
			continue;
		}
		if (((current ^ saved) & selectors) != 0) {
			/* Gate before changing mux/dividers; retain the current reset state. */
			*reg = current & ~MRCC_CC_MASK;
			*reg = (current & ~(MRCC_CC_MASK | selectors)) | (saved & selectors);
		}
		*reg = saved;
		if ((*reg & mrcc_context_registers[i].mask) != saved) {
			return -EIO;
		}
	}
	if (firc_configuration_changed() ||
	    (SCG0->CLKOUTCNFG & SCG_CLKOUTCNFG_CLKOUTSEL_MASK) != clock_context.clock_out ||
	    (SCG0->RCCR & MCXW_RUN_CLOCK_FIELDS) != clock_context.rccr) {
		return -EIO;
	}
	SystemCoreClock = CLOCK_GetCoreSysClkFreq();
	return SystemCoreClock == clock_context.core_frequency ? 0 : -EIO;
}
#endif /* supported MCXW S2RAM */

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
static int nxp_mcxw_clock_validate_sys_clk_src(const struct mcxw_clock_control_config *config)
{
#if !DT_INST_PROP(0, sirc_supported)
	if (config->sys_clk_src == MCXW_CLK_SYSTEM_CLK_SRC_SIRC) {
		LOG_ERR("SIRC is not available as system clock source on this SoC");
		return -EINVAL;
	}
#endif

	switch (config->sys_clk_src) {
	case MCXW_CLK_SYSTEM_CLK_SRC_SOSC:
		if (!config->enable_sosc) {
			LOG_ERR("System OSC selected but not enabled in config");
			return -EINVAL;
		}
		break;
	case MCXW_CLK_SYSTEM_CLK_SRC_FIRC:
		if (config->firc_mode == MCXW_CLK_FIRC_DISABLE) {
			LOG_ERR("FIRC selected but disabled in config");
			return -EINVAL;
		}
		break;
	case MCXW_CLK_SYSTEM_CLK_SRC_SIRC:
	case MCXW_CLK_SYSTEM_CLK_SRC_ROSC:
		break;
	default:
		LOG_ERR("Unsupported system clock source %u", config->sys_clk_src);
		return -EINVAL;
	}

	return 0;
}
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

static int nxp_mcxw_clock_control_init(const struct device *dev)
{
	const struct mcxw_clock_control_config *config = dev->config;

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/*
	 * TF-M non-secure build:
	 *
	 * Secure firmware owns global clock, oscillator, voltage,
	 * flash wait-state, power and security setup.
	 *
	 * Do not reconfigure SYSCLK, FIRC, SIRC, SOSC, ROSC, CCM32K,
	 * SPC or FMU from the non-secure image.
	 *
	 * The secure image must configure the clock tree before jumping
	 * to the non-secure image.
	 */

	if (config->core_clock_frequency != 0U) {
		SystemCoreClock = config->core_clock_frequency;
	} else {
		SystemCoreClock = CLOCK_GetCoreSysClkFreq();
	}

#else
	if (nxp_mcxw_clock_validate_sys_clk_src(config) != 0) {
		return -EINVAL;
	}

	/* Unlock Reference Clock Status Registers to allow writes */
	CLOCK_UnlockFircControlStatusReg();
#if DT_INST_PROP(0, sirc_supported)
	CLOCK_UnlockSircControlStatusReg();
#endif
	CLOCK_UnlockRoscControlStatusReg();
	CLOCK_UnlockSysOscControlStatusReg();

	CLOCK_SetXtal32Freq(32768U);

	/* Init OSC32K */
	CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorDisable);

	/*
	 * CCM32K registers are not reset on a warm boot. If a previous application
	 * selected OSC32K as the 32 kHz clock source and this application disables
	 * OSC32K, CCM32K_Set32kOscConfig() asserts because OSC32K cannot be
	 * disabled while it is the active source. Switch the 32 kHz source back to
	 * FRO32K first.
	 */
	if ((config->osc32k_mode == kCCM32K_Disable32kHzCrystalOsc) &&
	    (CCM32K_GetClockSource(CCM32K) == kCCM32K_ClockSource32kOsc)) {
		/* FRO32K must be enabled before it can be selected as the source. */
		CCM32K_Enable32kFro(CCM32K, true);
		CCM32K_SelectClockSource(CCM32K, kCCM32K_ClockSourceSelectFro32k);
		/* Wait until FRO32K is the active 32 kHz clock source. */
		while ((CCM32K_GetStatusFlag(CCM32K) & kCCM32K_32kFroActiveStatusFlag) == 0UL) {
		}
	}

	ccm32k_osc_config_t ccm32k_osc_config = {
		.coarseAdjustment = config->coarse_adjustment,
		.enableInternalCapBank = true,
		.xtalCap = config->xtal_cap,
		.extalCap = config->extal_cap,
	};
	CCM32K_Set32kOscConfig(CCM32K, (ccm32k_osc_mode_t)config->osc32k_mode, &ccm32k_osc_config);

	/* Switch to safe clock source (SIRC) before reconfiguring FIRC */
	scg_sys_clk_config_t sys_clk_safe_config_source = {
#if DT_INST_NODE_HAS_PROP(0, sys_clk_div_plat)
		.divCore1 = (uint32_t)kSCG_SysClkDivBy1,
#endif
		.divSlow = (uint32_t)kSCG_SysClkDivBy4,
		.divBus = (uint32_t)kSCG_SysClkDivBy1,
		.divCore = (uint32_t)kSCG_SysClkDivBy1,
		.src = (uint32_t)kSCG_SysClkSrcSirc,
	};

	CLOCK_SetRunModeSysClkConfig(&sys_clk_safe_config_source);

	scg_sys_clk_config_t cur_config;
	/* Wait for clock source switch to finish */
	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_safe_config_source.src);

	/* Configuration to set FIRC */
	scg_firc_config_t scg_firc_config = {
		.enableMode = (config->firc_mode == MCXW_CLK_FIRC_DISABLE)
				      ? 0
				      : ((config->firc_mode == MCXW_CLK_FIRC_ENABLE)
						 ? kSCG_FircEnable
						 : kSCG_FircEnableInSleep),
		.range = (scg_firc_range_t)config->firc_range,
		.trimConfig = NULL,
	};

	/* Initialize FIRC */
	(void)CLOCK_InitFirc(&scg_firc_config);

#if DT_INST_PROP(0, sirc_supported)
	if (config->enable_sirc_in_lp_mode) {
		scg_sirc_config_t scg_sirc_config = {
			.enableMode = kSCG_SircEnableInSleep,
		};
		(void)CLOCK_InitSirc(&scg_sirc_config);
	}
#endif

	if (config->enable_sosc) {
		scg_sosc_config_t scg_sosc_config = {
			.freq = config->sosc_freq,
			.enableMode = kSCG_SoscEnable,
			.monitorMode = kSCG_SysOscMonitorDisable,
		};
		(void)CLOCK_InitSysOsc(&scg_sosc_config);
		CLOCK_SetXtal0Freq(config->sosc_freq);
	}

	if (config->core_clock_frequency == 96000000U) {
		/* Raise the core voltage to allow the system to run at 96MHz */
		spc_active_mode_core_ldo_option_t ldoOption;
		/* Configure Flash to support different voltage level and frequency */
		FMU0->FCTRL =
			(FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x2U));
		/* Specifies the operating voltage for the SRAM's read/write timing margin */
		SPC_SetSRAMOperateVoltage(MCXW7_SPC_ADDR, kSPC_SRAM_OperatVoltage1P1V);
		/* Set the LDO_CORE VDD regulator level */
		ldoOption.CoreLDOVoltage = kSPC_CoreLDO_NormalVoltage;
		ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
		(void)SPC_SetActiveModeCoreLDORegulatorConfig(MCXW7_SPC_ADDR, &ldoOption);
	}

	/* Configure system clock with user-defined settings */
	scg_sys_clk_config_t sys_clk_config = {
#if DT_INST_NODE_HAS_PROP(0, sys_clk_div_plat)
		.divCore1 = (config->sys_clk_div_plat - 1),
#endif
		.divSlow = (config->sys_clk_div_slow - 1),
		.divBus = (config->sys_clk_div_bus - 1),
		.divCore = (config->sys_clk_div_core - 1),
		.src = config->sys_clk_src,
	};

	CLOCK_SetRunModeSysClkConfig(&sys_clk_config);

	/* Wait for clock source switch to finish */
	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_config.src);

	SystemCoreClock = CLOCK_GetCoreSysClkFreq();

	if (SystemCoreClock != config->core_clock_frequency) {
		LOG_ERR("Core clock frequency mismatch: expected %u Hz, got %u Hz",
			config->core_clock_frequency, SystemCoreClock);
		k_panic();
	}

	if (config->osc32k_mode != kCCM32K_Disable32kHzCrystalOsc) {
		while ((CCM32K_GetStatusFlag(CCM32K) & kCCM32K_32kOscReadyStatusFlag) == 0UL) {
		}
		CCM32K_SelectClockSource(CCM32K, kCCM32K_ClockSourceSelectOsc32k);
		while ((CCM32K_GetStatusFlag(CCM32K) & kCCM32K_32kOscActiveStatusFlag) == 0UL) {
		}
		/* Wait for RTC Oscillator to be Valid */
		while (!CLOCK_IsRoscValid()) {
		}
		CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorInt);
		/* Disable the FRO32K to save power */
		CCM32K_Enable32kFro(CCM32K, false);
#if FSL_FEATURE_CCM32K_HAS_CGC32K
		CCM32K_SelectClockSource(CCM32K, kCCM32K_ClockSourceSelectOsc32k);
#endif
	}

#if FSL_FEATURE_CCM32K_HAS_CGC32K
	/* Enable 32kHz clock output to all peripherals.  */
	CCM32K_EnableCLKOutToPeripherals(CCM32K, 0xFF);
#endif
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

	return pm_device_driver_init(dev, nxp_mcxw_clock_control_pm);
}

static DEVICE_API(clock_control, nxp_mcxw_clock_control_api) = {
	.on = nxp_mcxw_clock_control_on,
	.off = nxp_mcxw_clock_control_off,
	.get_rate = nxp_mcxw_clock_control_get_rate,
	.get_status = nxp_mcxw_clock_control_get_status,
	.configure = nxp_mcxw_clock_control_configure,
};

static struct mcxw_clock_control_config config = {
	.osc32k_mode = DT_INST_PROP(0, osc32k_mode),
	.xtal_cap = DT_INST_PROP(0, osc32k_xtal_cap),
	.extal_cap = DT_INST_PROP(0, osc32k_extal_cap),
	.coarse_adjustment = DT_INST_PROP(0, osc32k_coarse_adjustment),
	.firc_mode = DT_INST_PROP(0, firc_mode),
	.firc_range = DT_INST_PROP(0, firc_range),
#if DT_INST_PROP(0, sirc_supported)
	.enable_sirc_in_lp_mode = DT_INST_PROP(0, enable_sirc_in_lp_mode),
#endif
	.sys_clk_src = DT_INST_PROP(0, sys_clk_src),
#if DT_INST_NODE_HAS_PROP(0, sys_clk_div_plat)
	.sys_clk_div_plat = DT_INST_PROP(0, sys_clk_div_plat),
#endif
	.sys_clk_div_bus = DT_INST_PROP(0, sys_clk_div_bus),
	.sys_clk_div_slow = DT_INST_PROP(0, sys_clk_div_slow),
	.sys_clk_div_core = DT_INST_PROP(0, sys_clk_div_core),
	.enable_sosc = DT_INST_PROP(0, enable_sosc),
	.sosc_freq = DT_INST_PROP(0, sosc_freq),
	.core_clock_frequency = DT_INST_PROP(0, core_clock_frequency),
};

#if DT_INST_NODE_HAS_PROP(0, nvmem_cells) && defined(CONFIG_NVMEM_FLASH) &&                        \
	!defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
/*
 * The factory OSC32K trim lives in a non-volatile memory which is only readable once its
 * provider is initialized. The capacitance values from the devicetree are applied at
 * PRE_KERNEL_1 and refined here, at POST_KERNEL, when a valid trim is available. This
 * requires the flash driver, hence CONFIG_FLASH, to be enabled by the application.
 */
/* Bit 7 of the stored byte flags the trim as valid, the capacitance value is in the low bits */
#define MCXW_OSC32K_TRIM_VALID 0x80U
#define MCXW_OSC32K_XTAL_TRIM_MAX                                                                  \
	(CCM32K_OSC32K_CTRL_XTAL_CAP_SEL_MASK >> CCM32K_OSC32K_CTRL_XTAL_CAP_SEL_SHIFT)
#define MCXW_OSC32K_EXTAL_TRIM_MAX                                                                 \
	(CCM32K_OSC32K_CTRL_EXTAL_CAP_SEL_MASK >> CCM32K_OSC32K_CTRL_EXTAL_CAP_SEL_SHIFT)

BUILD_ASSERT(MCXW_OSC32K_XTAL_TRIM_MAX == MCXW_OSC32K_EXTAL_TRIM_MAX,
	     "XTAL and EXTAL capacitance fields must have the same width");
BUILD_ASSERT(MCXW_OSC32K_XTAL_TRIM_MAX < MCXW_OSC32K_TRIM_VALID,
	     "The capacitance fields must not overlap the trim valid flag");

#if defined(CONFIG_FLASH_INIT_PRIORITY)
BUILD_ASSERT(CONFIG_CLOCK_CONTROL_NXP_MCXW7X_IFR_TRIM_INIT_PRIORITY > CONFIG_FLASH_INIT_PRIORITY,
	     "The OSC32K trim must be applied after the flash driver is initialized");
#endif

static int nxp_mcxw_clock_osc32k_ifr_trim(void)
{
	static const struct nvmem_cell trim_cell = NVMEM_CELL_GET_BY_IDX(DT_DRV_INST(0), 0);
	uint8_t raw;
	uint32_t trim;
	uint32_t reg;
	int ret;

	if (config.osc32k_mode != kCCM32K_Enable32kHzCrystalOsc) {
		return 0;
	}

	ret = nvmem_cell_read(&trim_cell, &raw, 0, sizeof(raw));
	if (ret < 0) {
		LOG_DBG("OSC32K trim not readable (%d), keeping devicetree values", ret);
		return 0;
	}

	if (raw == 0xFFU) {
		LOG_DBG("OSC32K trim not provisioned, keeping devicetree values");
		return 0;
	}

	if ((raw & MCXW_OSC32K_TRIM_VALID) == 0U) {
		LOG_DBG("OSC32K trim 0x%02x not valid, keeping devicetree values", raw);
		return 0;
	}

	/* 0 is a valid capacitance selection */
	trim = (uint32_t)raw & ~MCXW_OSC32K_TRIM_VALID;
	if (trim > MCXW_OSC32K_XTAL_TRIM_MAX) {
		LOG_DBG("OSC32K trim 0x%02x out of range, keeping devicetree values", raw);
		return 0;
	}

	/* OSC32K stays enabled, the capacitance selection can be changed on the fly */
	reg = CCM32K->OSC32K_CTRL;
	reg &= ~(CCM32K_OSC32K_CTRL_XTAL_CAP_SEL_MASK | CCM32K_OSC32K_CTRL_EXTAL_CAP_SEL_MASK);
	reg |= CCM32K_OSC32K_CTRL_XTAL_CAP_SEL(trim) | CCM32K_OSC32K_CTRL_EXTAL_CAP_SEL(trim);
	CCM32K->OSC32K_CTRL = reg;
	__DMB();

	LOG_DBG("OSC32K trim 0x%02x applied (cap sel %u)", raw, trim);

	return 0;
}

SYS_INIT(nxp_mcxw_clock_osc32k_ifr_trim, POST_KERNEL,
	 CONFIG_CLOCK_CONTROL_NXP_MCXW7X_IFR_TRIM_INIT_PRIORITY);
#endif /* nvmem-cells && CONFIG_NVMEM_FLASH && !CONFIG_TRUSTED_EXECUTION_NONSECURE */

DEVICE_DT_INST_DEFINE(0, nxp_mcxw_clock_control_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &nxp_mcxw_clock_control_api);
