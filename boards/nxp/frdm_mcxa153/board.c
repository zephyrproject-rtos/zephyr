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

/* Core clock frequency: 96MHz */
#define CLOCK_INIT_CORE_CLOCK            960000000U
#define BOARD_BOOTCLOCKFRO96M_CORE_CLOCK 960000000U
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
	if (coreFreq <= BOARD_BOOTCLOCKFRO96M_CORE_CLOCK) {
		/* Set the LDO_CORE VDD regulator level */
		ldoOption.CoreLDOVoltage = kSPC_CoreLDO_NormalVoltage;
		ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
		(void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
		/* Configure Flash to support different voltage level and frequency */
		FMU0->FCTRL =
			(FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x2U));
		/* Specifies the operating voltage for the SRAM's read/write timing margin */
		sramOption.operateVoltage = kSPC_sramOperateAt1P1V;
		sramOption.requestVoltageUpdate = true;
		(void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
	}

	CLOCK_SetupFROHFClocking(CLOCK_INIT_CORE_CLOCK); /*!< Enable FRO HF(96MHz) output */

	CLOCK_SetupFRO12MClocking(); /*!< Setup FRO12M clock */

	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK); /* !< Switch MAIN_CLK to FRO_HF */

	/* The flow of decreasing voltage and frequency */
	if (coreFreq > BOARD_BOOTCLOCKFRO96M_CORE_CLOCK) {
		/* Configure Flash to support different voltage level and frequency */
		FMU0->FCTRL =
			(FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x2U));
		/* Specifies the operating voltage for the SRAM's read/write timing margin */
		sramOption.operateVoltage = kSPC_sramOperateAt1P1V;
		sramOption.requestVoltageUpdate = true;
		(void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
		/* Set the LDO_CORE VDD regulator level */
		ldoOption.CoreLDOVoltage = kSPC_CoreLDO_NormalVoltage;
		ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
		(void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
	}

	/*!< Set up clock selectors - Attach clocks to the peripheries */

	/*!< Set up dividers */
	CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, 1U);     /* !< Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivFRO_HF_DIV, 1U); /* !< Set FROHFDIV divider to value 1 */

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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1u);
	CLOCK_AttachClk(kFRO12M_to_LPUART0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1))
	CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
	CLOCK_AttachClk(kFRO12M_to_LPUART1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(porta))
	RESET_ReleasePeripheralReset(kPORT0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portb))
	RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portc))
	RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portd))
	RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;
}
