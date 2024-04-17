/*
 * Copyright 2024  NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_clock.h>
#include <fsl_spc.h>
#include <soc.h>

/* Board xtal frequency in Hz */
#define BOARD_XTAL0_CLK_HZ                        24000000U
/* Core clock frequency: 150MHz */
#define CLOCK_INIT_CORE_CLOCK                     150000000U
/* System clock frequency. */
extern uint32_t SystemCoreClock;

__ramfunc static void enable_lpcac(void)
{
	SYSCON->LPCAC_CTRL |= SYSCON_LPCAC_CTRL_CLR_LPCAC_MASK;
	SYSCON->LPCAC_CTRL &= ~(SYSCON_LPCAC_CTRL_CLR_LPCAC_MASK |
				SYSCON_LPCAC_CTRL_DIS_LPCAC_MASK);
}

/* Update Active mode voltage for OverDrive mode. */
void power_mode_od(void)
{
	/* Set the DCDC VDD regulator to 1.2 V voltage level */
	spc_active_mode_dcdc_option_t opt = {
		.DCDCVoltage       = kSPC_DCDC_OverdriveVoltage,
		.DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength,
	};
	SPC_SetActiveModeDCDCRegulatorConfig(SPC0, &opt);

	/* Set the LDO_CORE VDD regulator to 1.2 V voltage level */
	spc_active_mode_core_ldo_option_t ldo_opt = {
		.CoreLDOVoltage       = kSPC_CoreLDO_OverDriveVoltage,
		.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength,
	};
	SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldo_opt);

	/* Specifies the 1.2V operating voltage for the SRAM's read/write timing margin */
	spc_sram_voltage_config_t cfg = {
		.operateVoltage       = kSPC_sramOperateAt1P2V,
		.requestVoltageUpdate = true,
	};
	SPC_SetSRAMOperateVoltage(SPC0, &cfg);
}

static int frdm_mcxn947_init(void)
{
	enable_lpcac();

	power_mode_od();

	/* Enable SCG clock */
	CLOCK_EnableClock(kCLOCK_Scg);

	/* FRO OSC setup - begin, enable the FRO for safety switching */

	/* Switch to FRO 12M first to ensure we can change the clock setting */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Configure Flash wait-states to support 1.2V voltage level and 150000000Hz frequency */
	FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x3U));

	/* Enable FRO HF(48MHz) output */
	CLOCK_SetupFROHFClocking(48000000U);

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
	/* Call function flexspi_clock_safe_config() to move FleXSPI clock to a stable
	 * clock source when updating the PLL if in XIP (execute code from FlexSPI memory
	 */
	flexspi_clock_safe_config();
#endif

	/* Set up PLL0 */
	const pll_setup_t pll0Setup = {
		.pllctrl = SCG_APLLCTRL_SOURCE(1U) | SCG_APLLCTRL_SELI(27U) |
			   SCG_APLLCTRL_SELP(13U),
		.pllndiv = SCG_APLLNDIV_NDIV(8U),
		.pllpdiv = SCG_APLLPDIV_PDIV(1U),
		.pllmdiv = SCG_APLLMDIV_MDIV(50U),
		.pllRate = 150000000U
	};
	/* Configure PLL0 to the desired values */
	CLOCK_SetPLL0Freq(&pll0Setup);
	/* PLL0 Monitor is disabled */
	CLOCK_SetPll0MonitorMode(kSCG_Pll0MonitorDisable);

	/* Switch MAIN_CLK to PLL0 */
	CLOCK_AttachClk(kPLL0_to_MAIN_CLK);

	/* Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm1), okay)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom1Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm2), okay)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm4), okay)
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(os_timer), okay)
	CLOCK_AttachClk(kCLK_1M_to_OSTIMER);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	CLOCK_EnableClock(kCLOCK_Gpio0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	CLOCK_EnableClock(kCLOCK_Gpio1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio2), okay)
	CLOCK_EnableClock(kCLOCK_Gpio2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio3), okay)
	CLOCK_EnableClock(kCLOCK_Gpio3);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio4), okay)
	CLOCK_EnableClock(kCLOCK_Gpio4);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio5), okay)
	CLOCK_EnableClock(kCLOCK_Gpio5);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(dac0), okay)
	SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlDac0);
	CLOCK_SetClkDiv(kCLOCK_DivDac0Clk, 1u);
	CLOCK_AttachClk(kFRO_HF_to_DAC0);

	CLOCK_EnableClock(kCLOCK_Dac0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(dac1), okay)
	SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlDac1);
	CLOCK_SetClkDiv(kCLOCK_DivDac1Clk, 1u);
	CLOCK_AttachClk(kFRO_HF_to_DAC1);

	CLOCK_EnableClock(kCLOCK_Dac1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay)
	CLOCK_AttachClk(kNONE_to_ENETRMII);
	CLOCK_EnableClock(kCLOCK_Enet);
	SYSCON0->PRESETCTRL2 = SYSCON_PRESETCTRL2_ENET_RST_MASK;
	SYSCON0->PRESETCTRL2 &= ~SYSCON_PRESETCTRL2_ENET_RST_MASK;
	/* rmii selection for this board */
	SYSCON->ENET_PHY_INTF_SEL = SYSCON_ENET_PHY_INTF_SEL_PHY_SEL(1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(wwdt0), okay)
	CLOCK_SetClkDiv(kCLOCK_DivWdt0Clk, 1u);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ctimer0), okay)
	CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ctimer1), okay)
	CLOCK_SetClkDiv(kCLOCK_DivCtimer1Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ctimer2), okay)
	CLOCK_SetClkDiv(kCLOCK_DivCtimer2Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ctimer3), okay)
	CLOCK_SetClkDiv(kCLOCK_DivCtimer3Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER3);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ctimer4), okay)
	CLOCK_SetClkDiv(kCLOCK_DivCtimer4Clk, 1U);
	CLOCK_AttachClk(kPLL0_to_CTIMER4);
#endif

#if CONFIG_FLASH_MCUX_FLEXSPI_NOR
	/* We downclock the FlexSPI to 50MHz, it will be set to the
	 * optimum speed supported by the Flash device during FLEXSPI
	 * Init
	 */
	flexspi_clock_set_freq(MCUX_FLEXSPI_CLK, MHZ(50));
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;

	return 0;
}

SYS_INIT(frdm_mcxn947_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
