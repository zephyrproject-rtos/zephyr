/**
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <fsl_inputmux.h>

#define BOARD_XTAL_SYS_CLK_HZ 24000000U

__weak void mimxrt685s_hifi4_irq_init(void)
{
	/*
	 * IRQ assignments
	 *
	 * L1: (lowest priority)
	 * - IRQ 5  (SEL 0):  Flexcomm 0 (UART)
	 * - IRQ 6  (SEL 1):  I3C
	 * - IRQ 7  (SEL 2):  GPIO_INT0_IRQ0
	 * - IRQ 8  (SEL 3):  GPIO_INT0_IRQ1
	 * - IRQ 9  (SEL 4):  GPIO_INT0_IRQ2
	 * - IRQ 10 (SEL 5):  GPIO_INT0_IRQ3
	 * - IRQ 11 (SEL 6):  GPIO_INT0_IRQ4
	 * - IRQ 12 (SEL 7):  GPIO_INT0_IRQ5
	 * - IRQ 13 (SEL 8):  GPIO_INT0_IRQ6
	 * - IRQ 14 (SEL 9):  GPIO_INT0_IRQ7
	 * - IRQ 15 (SEL 10): HSGPIO_INT0
	 *
	 * L2:
	 * - IRQ 16 (SEL 11): MRT0
	 * - IRQ 17 (SEL 12): WDT1
	 * - IRQ 18 (SEL 13): RTC
	 * - IRQ 19 (SEL 14): CT32BIT0
	 * - IRQ 20 (SEL 15): CT32BIT1
	 * - IRQ 21 (SEL 16): CT32BIT2
	 * - IRQ 22 (SEL 17): CT32BIT3
	 * - IRQ 23 (SEL 18): CT32BIT4
	 *
	 * L3: (highest priority)
	 * - IRQ 24 (SEL 19): UTICK0
	 * - IRQ 25 (SEL 20): HWVAD0
	 * - IRQ 26 (SEL 21): Flexcomm 5 (SPI)
	 * - IRQ 27 (SEL 22): MU
	 * - IRQ 28 (SEL 23): DMIC
	 * - IRQ 29 (SEL 24): DMA1
	 * - IRQ 30 (SEL 25): Flexcomm 3 (I2S TX)
	 * - IRQ 31 (SEL 26): Flexcomm 1 (I2S RX)
	 */

	INPUTMUX_Init(INPUTMUX);

	INPUTMUX_AttachSignal(INPUTMUX, 0, kINPUTMUX_Flexcomm0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 1, kINPUTMUX_I3c0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 2, kINPUTMUX_GpioInt0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 3, kINPUTMUX_GpioInt1ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 4, kINPUTMUX_GpioInt2ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 5, kINPUTMUX_GpioInt3ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 6, kINPUTMUX_GpioInt4ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 7, kINPUTMUX_GpioInt5ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 8, kINPUTMUX_GpioInt6ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 9, kINPUTMUX_GpioInt7ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 10, kINPUTMUX_NsHsGpioInt0ToDspInterrupt);

	INPUTMUX_AttachSignal(INPUTMUX, 11, kINPUTMUX_Mrt0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 12, kINPUTMUX_Wdt1ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 13, kINPUTMUX_RtcToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 14, kINPUTMUX_Ctimer0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 15, kINPUTMUX_Ctimer1ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 16, kINPUTMUX_Ctimer2ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 17, kINPUTMUX_Ctimer3ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 18, kINPUTMUX_Ctimer4ToDspInterrupt);

	INPUTMUX_AttachSignal(INPUTMUX, 19, kINPUTMUX_Utick0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 20, kINPUTMUX_Hwvad0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 21, kINPUTMUX_Flexcomm5ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 22, kINPUTMUX_MuBToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 23, kINPUTMUX_Dmic0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 24, kINPUTMUX_Dmac1ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 25, kINPUTMUX_Flexcomm3ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX, 26, kINPUTMUX_Flexcomm1ToDspInterrupt);

	INPUTMUX_Deinit(INPUTMUX);
}

void soc_early_init_hook(void)
{
	CLOCK_SetXtalFreq(BOARD_XTAL_SYS_CLK_HZ);
	CLOCK_EnableClock(kCLOCK_Dmac1);

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2s, okay) && CONFIG_I2S)
	/* attach AUDIO PLL clock to FLEXCOMM1 (I2S_PDM) */
	CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2s, okay) && CONFIG_I2S)
	/* attach AUDIO PLL clock to FLEXCOMM1 (I2S_PDM) */
	CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);
#endif

#if CONFIG_AUDIO_CODEC_WM8904
	/* attach AUDIO PLL clock to MCLK */
	CLOCK_AttachClk(kAUDIO_PLL_to_MCLK_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivMclkClk, 1);
	SYSCTL1->MCLKPINDIR = SYSCTL1_MCLKPINDIR_MCLKPINDIR_MASK;
#endif

	mimxrt685s_hifi4_irq_init();
}
