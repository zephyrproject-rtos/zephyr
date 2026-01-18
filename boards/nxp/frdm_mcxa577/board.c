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

/* Core clock frequency: 12MHz in FPGA */
#define CLOCK_INIT_CORE_CLOCK 12000000U
/* System clock frequency. */
extern uint32_t SystemCoreClock;

void board_early_init_hook(void)
{
	/* Get the CPU Core frequency */
	CLOCK_SetupFRO12MClocking();
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);
	CLOCK_SetClockDiv(kCLOCK_DivFRO_HF, 1);
	CLOCK_SetClockDiv(kCLOCK_DivFRO_LF, 1);

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
	RESET_ReleasePeripheralReset(kGPIO5_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO5);
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

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;
}
