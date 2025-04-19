/*
 * Copyright 2024-2025  NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_clock.h>
#include <fsl_spc.h>
#include <soc.h>

/* Core clock frequency: 180MHz */
#define CLOCK_INIT_CORE_CLOCK               180000000U
#define BOARD_BOOTCLOCKFROHF180M_CORE_CLOCK 180000000U
/* System clock frequency. */
extern uint32_t SystemCoreClock;

void board_early_init_hook(void)
{
	uint32_t coreFreq;
	spc_active_mode_core_ldo_option_t ldoOption;
	spc_sram_voltage_config_t sramOption;

	/* Get the CPU Core frequency */
	coreFreq = CLOCK_GetCoreSysClkFreq();

	/* The flow of increasing voltage and frequency */
	if (coreFreq <= BOARD_BOOTCLOCKFROHF180M_CORE_CLOCK) {
		/* Set the LDO_CORE VDD regulator level */
		ldoOption.CoreLDOVoltage = kSPC_CoreLDO_OverDriveVoltage;
		ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
		(void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
		/* Configure Flash to support different voltage level and frequency */
		FMU0->FCTRL =
			(FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x4U));
		/* Specifies the operating voltage for the SRAM's read/write timing margin */
		sramOption.operateVoltage = kSPC_sramOperateAt1P2V;
		sramOption.requestVoltageUpdate = true;
		(void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
	}

	/*!< Set up system dividers */
	CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, 1U); /* !< Set SYSCON.AHBCLKDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivFRO_HF, 1U); /* !< Set SYSCON.FROHFDIV divider to value 1 */
	CLOCK_SetupFROHFClocking(BOARD_BOOTCLOCKFROHF180M_CORE_CLOCK); /*!< Enable FRO HF */
	CLOCK_SetupFRO12MClocking();             /*!< Setup FRO12M clock */

	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK); /* !< Switch MAIN_CLK to kFRO_HF */

	/* The flow of decreasing voltage and frequency */
	if (coreFreq > BOARD_BOOTCLOCKFROHF180M_CORE_CLOCK) {
		/* Configure Flash to support different voltage level and frequency */
		FMU0->FCTRL =
			(FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x4U));
		/* Specifies the operating voltage for the SRAM's read/write timing margin */
		sramOption.operateVoltage = kSPC_sramOperateAt1P2V;
		sramOption.requestVoltageUpdate = true;
		(void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
		/* Set the LDO_CORE VDD regulator level */
		ldoOption.CoreLDOVoltage = kSPC_CoreLDO_OverDriveVoltage;
		ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
		(void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
	}

	/*!< Set up clock selectors - Attach clocks to the peripheries */
	CLOCK_AttachClk(kCPU_CLK_to_TRACE); /* !< Switch TRACE to CPU_CLK */

	/*!< Set up dividers */
	CLOCK_SetClockDiv(kCLOCK_DivFRO_LF, 1U); /* !< Set SYSCON.FROLFDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivWWDT0, 1U);  /* !< Set MRCC.WWDT0_CLKDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivTRACE, 2U);  /* !< Set MRCC.TRACE_CLKDIV divider to value 2 */

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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPUART0);
	RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
	CLOCK_AttachClk(kFRO_LF_DIV_to_LPUART1);
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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(edma0))
	RESET_ReleasePeripheralReset(kDMA0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wwdt0))
	CLOCK_SetClockDiv(kCLOCK_DivWWDT0, 1u);
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;
}
