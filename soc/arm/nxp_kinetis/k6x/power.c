/*
 * Copytight (c) 2020 Nicolai Glud <nigd@prevas.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <fsl_smc.h>


#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

#ifdef CONFIG_PM
void enable_uart(void);
void disable_uart(void);

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
static ALWAYS_INLINE void set_clock_hw_cycles_per_sec(int cycles)
{
	extern int z_clock_hw_cycles_per_sec;

	z_clock_hw_cycles_per_sec = cycles;
}
#else
static ALWAYS_INLINE void set_clock_hw_cycles_per_sec(int cycles) {}
#endif

static ALWAYS_INLINE void set_clock_to_FEI_mode(void)
{
	const mcg_config_t mcgConfigStruct = {
		/* FEI - FLL with internal RTC. */
		.mcgMode = kMCG_ModeFEI,
		/* MCGIRCLK enabled, MCGIRCLK disabled in STOP mode */
		.irclkEnableMode = kMCG_IrclkEnable,
		/* Slow internal reference clock selected */
		.ircs = kMCG_IrcSlow,
		/* Fast IRC divider: divided by 1 */
		.fcrdiv = 0x0U,
		/* FLL reference clock divider: divided by 32 */
		.frdiv = 0x0U,
		/* Low frequency range */
		.drs = kMCG_DrsLow,
		/* DCO has a default range of 25% */
		.dmx32 = kMCG_Dmx32Default,
		/* Selects System Oscillator (OSCCLK) */
		.oscsel = kMCG_OscselOsc,
		.pll0Config = {
			/* MCGPLLCLK disabled */
			.enableMode = 0,
			/* PLL Reference divider: divided by 20 */
			.prdiv = 0x13U,
			/* VCO divider: multiplied by 48 */
			.vdiv = 0x18U,
		},
	};

	CLOCK_SetMcgConfig(&mcgConfigStruct);

	set_clock_hw_cycles_per_sec(25000000);
}

static ALWAYS_INLINE void set_clock_to_PEE_mode(void)
{
	const mcg_config_t mcgConfigStruct = {
		/* PEE - PLL Engaged External */
		.mcgMode = kMCG_ModePEE,
		/* MCGIRCLK enabled, MCGIRCLK disabled in STOP mode */
		.irclkEnableMode = kMCG_IrclkEnable,
		/* Slow internal reference clock selected */
		.ircs = kMCG_IrcSlow,
		/* Fast IRC divider: divided by 1 */
		.fcrdiv = 0x0U,
		/* FLL reference clock divider: divided by 32 */
		.frdiv = 0x0U,
		/* Low frequency range */
		.drs = kMCG_DrsLow,
		/* DCO has a default range of 25% */
		.dmx32 = kMCG_Dmx32Default,
		/* Selects System Oscillator (OSCCLK) */
		.oscsel = kMCG_OscselOsc,
		.pll0Config = {
			/* MCGPLLCLK disabled */
			.enableMode = 0,
			/* PLL Reference divider: divided by 20 */
			.prdiv = 0x13U,
			/* VCO divider: multiplied by 48 */
			.vdiv = 0x18U,
		},
	};

	CLOCK_SetMcgConfig(&mcgConfigStruct);

	set_clock_hw_cycles_per_sec(120000000);
}

static ALWAYS_INLINE void enter_sleep_mode_1(void)
{
	disable_uart();
	set_clock_to_FEI_mode();
	SMC_PreEnterWaitModes();
	SMC_SetPowerModeWait(SMC);
	SMC_PostExitWaitModes();
}

static ALWAYS_INLINE void exit_sleep_mode_1(void)
{
	enable_uart();
	set_clock_to_PEE_mode();
}
#endif

void pm_power_state_set(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	{
		enter_sleep_mode_1();
		break;
	}
	case PM_STATE_ACTIVE:
	case PM_STATE_RUNTIME_IDLE:
	case PM_STATE_SUSPEND_TO_RAM:
	case PM_STATE_SUSPEND_TO_DISK:
	case PM_STATE_SOFT_OFF:
	default:
		LOG_INF("Unsupported power state %u", info.state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	{
		exit_sleep_mode_1();
		break;
	}
	case PM_STATE_ACTIVE:
	case PM_STATE_RUNTIME_IDLE:
	case PM_STATE_SUSPEND_TO_RAM:
	case PM_STATE_SUSPEND_TO_DISK:
	case PM_STATE_SOFT_OFF:
	default:
		break;
	}
}
