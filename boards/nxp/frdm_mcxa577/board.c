/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_clock.h>
#include <fsl_spc.h>
#include <soc.h>

/* Core clock frequency: 200MHz from PLL */
#define CLOCK_INIT_CORE_CLOCK 200000000U
/* System clock frequency. */
extern uint32_t SystemCoreClock;

void board_early_init_hook(void)
{
	spc_active_mode_core_ldo_option_t ldoOption;
	spc_sram_voltage_config_t sramOption;

	/* Get the CPU Core frequency */
	CLOCK_SetupFRO12MClocking();
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);
	CLOCK_SetClockDiv(kCLOCK_DivFRO_HF, 1);
	CLOCK_SetClockDiv(kCLOCK_DivFRO_LF, 1);

	/* Set the LDO_CORE VDD regulator level */
	ldoOption.CoreLDOVoltage = kSPC_CoreLDO_OverDriveVoltage;
	ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
	(void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
	/* Configure Flash to support different voltage level and frequency */
	CLOCK_SetFLASHAccessCyclesForFreq(CLOCK_INIT_CORE_CLOCK, kOD_Mode);
	/* Specifies the operating voltage for the SRAM's read/write timing margin */
	sramOption.operateVoltage = kSPC_sramOperateAt1P2V;
	sramOption.requestVoltageUpdate = true;
	(void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);

	/*!< Set up system dividers */
	CLOCK_SetClockDiv(kCLOCK_DivFRO_HF, 4U); /* !< Set SYSCON.FROHFDIV divider to value 4 */
	CLOCK_SetupFROHFClocking(192000000U);    /*!< Enable FRO HF(192MHz) output */
	CLOCK_SetupFRO12MClocking();             /*!< Setup FRO12M clock */
	CLOCK_SetupExtClocking(24000000U);       /*!< Enable OSC with 24000000 HZ */
	CLOCK_SetSysOscMonitorMode(
		kSCG_SysOscMonitorDisable); /* System OSC Clock Monitor is disabled */

	/*!< Set up PLL1 */
	const pll_setup_t pll1Setup = {.pllctrl = SCG_SPLLCTRL_SOURCE(0U) | SCG_SPLLCTRL_SELI(53U) |
						  SCG_SPLLCTRL_SELP(26U) |
						  SCG_SPLLCTRL_BYPASSPOSTDIV2_MASK,
				       .pllndiv = SCG_SPLLNDIV_NDIV(6U),
				       .pllpdiv = SCG_SPLLPDIV_PDIV(2U),
				       .pllmdiv = SCG_SPLLMDIV_MDIV(100U),
				       .pllRate = 200000000U};
	CLOCK_SetPLL1Freq(&pll1Setup); /*!< Configure PLL1 to the desired values */
	CLOCK_SetPll1MonitorMode(kSCG_Pll1MonitorDisable); /* Pll1 Monitor is disabled */

	CLOCK_AttachClk(kPll1Clk_to_MAIN_CLK); /* !< Switch MAIN_CLK to kPll1Clk */

	/*!< Set up dividers */
	CLOCK_SetClockDiv(kCLOCK_DivFRO_LF, 1U);  /* !< Set SYSCON.FROLFDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivPLL1CLK, 4U); /* !< Set SYSCON.PLL1CLKDIV divider to value 4 */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(porta))
	RESET_ReleasePeripheralReset(kPORT0_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portb))
	RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portc))
	RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portd))
	RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT3);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(porte))
	RESET_ReleasePeripheralReset(kPORT4_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT4);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portf))
	CLOCK_EnableClock(kCLOCK_GatePORT5);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio0))
	RESET_ReleasePeripheralReset(kGPIO0_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio1))
	RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio2))
	RESET_ReleasePeripheralReset(kGPIO2_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio3))
	RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO3);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio4))
	RESET_ReleasePeripheralReset(kGPIO4_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO4);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio5))
	CLOCK_EnableClock(kCLOCK_GateGPIO5);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPUART0);
	RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
	CLOCK_AttachClk(kPll1ClkDiv_to_LPUART1);
	RESET_ReleasePeripheralReset(kLPUART1_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart2))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART2, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPUART2);
	RESET_ReleasePeripheralReset(kLPUART2_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart3))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART3, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPUART3);
	RESET_ReleasePeripheralReset(kLPUART3_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart4))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART4, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPUART4);
	RESET_ReleasePeripheralReset(kLPUART4_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart5))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART5, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPUART5);
	RESET_ReleasePeripheralReset(kLPUART5_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c0))
	CLOCK_SetClockDiv(kCLOCK_DivLPI2C0, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPI2C0);
	RESET_ReleasePeripheralReset(kLPI2C0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c1))
	CLOCK_SetClockDiv(kCLOCK_DivLPI2C1, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPI2C1);
	RESET_ReleasePeripheralReset(kLPI2C1_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c2))
	CLOCK_SetClockDiv(kCLOCK_DivLPI2C2, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPI2C2);
	RESET_ReleasePeripheralReset(kLPI2C2_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c3))
	CLOCK_SetClockDiv(kCLOCK_DivLPI2C3, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPI2C3);
	RESET_ReleasePeripheralReset(kLPI2C3_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c4))
	CLOCK_SetClockDiv(kCLOCK_DivLPI2C4, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPI2C4);
	RESET_ReleasePeripheralReset(kLPI2C4_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer0))
	CLOCK_SetClockDiv(kCLOCK_DivCTIMER0, 1u);
	CLOCK_AttachClk(kPll1ClkDiv_to_CTIMER0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer1))
	CLOCK_SetClockDiv(kCLOCK_DivCTIMER1, 1u);
	CLOCK_AttachClk(kPll1ClkDiv_to_CTIMER1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer2))
	CLOCK_SetClockDiv(kCLOCK_DivCTIMER2, 1u);
	CLOCK_AttachClk(kPll1ClkDiv_to_CTIMER2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer3))
	CLOCK_SetClockDiv(kCLOCK_DivCTIMER3, 1u);
	CLOCK_AttachClk(kPll1ClkDiv_to_CTIMER3);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ctimer4))
	CLOCK_SetClockDiv(kCLOCK_DivCTIMER4, 1u);
	CLOCK_AttachClk(kPll1ClkDiv_to_CTIMER4);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lptmr0))

/*
 * Clock Select Decides what input source the lptmr will clock from
 *
 * 0 <- Reserved
 * 1 <- 16K FRO
 * 2 <- Reserved
 * 3 <- Combination of clocks configured in MRCC_LPTMR0_CLKSEL[MUX] field
 */
#if DT_PROP(DT_NODELABEL(lptmr0), clk_source) == 0x1
	CLOCK_SetupFRO16KClocking(kCLKE_16K_SYSTEM | kCLKE_16K_COREMAIN | kCLKE_16K_VBAT);
#elif DT_PROP(DT_NODELABEL(lptmr0), clk_source) == 0x3
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPTMR0);
	CLOCK_SetClockDiv(kCLOCK_DivLPTMR0, 1u);
#endif /* DT_PROP(DT_NODELABEL(lptmr0), clk_source) */

#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(edma0))
	RESET_ReleasePeripheralReset(kDMA0_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateDMA0);
	/* Release DMA0 request */
	for (uint8_t i = 0; i < AHBSC_SEC_GP_REG_COUNT/2; i++) {
		AHBSC->SEC_GP_REG[i] = 0xFFFFFFFF;
	}
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(edma1))
	RESET_ReleasePeripheralReset(kDMA1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateDMA1);
	/* Release DMA1 request */
	for (uint8_t i = AHBSC_SEC_GP_REG_COUNT/2; i < AHBSC_SEC_GP_REG_COUNT; i++) {
		AHBSC->SEC_GP_REG[i] = 0xFFFFFFFF;
	}
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wwdt0))
	CLOCK_SetClockDiv(kCLOCK_DivWWDT0, 1u);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wwdt1))
	CLOCK_SetClockDiv(kCLOCK_DivWWDT1, 1u);
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;
}
