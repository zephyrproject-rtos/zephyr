/*
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>

#include <fsl_clock.h>
#include <fsl_inputmux.h>

#if DT_NODE_HAS_STATUS(DT_NODELABEL(edma1), okay)
#define SYSCON_BASE DT_REG_ADDR(DT_NODELABEL(syscon0))
#define EN_NUM 4

#define EDMA_EN_OFFSET 0x420
#define EDMA_EN_REG(instance, idx) ((uint32_t *)((uint32_t)(SYSCON_BASE) + \
	(EDMA_EN_OFFSET) +  0x10U * (instance) + 4U * (idx)))
#endif

#define SET_UP_FLEXCOMM_CLOCK(x)                                                                   \
	do {                                                                                       \
		CLOCK_AttachClk(kFCCLK0_to_FLEXCOMM##x);                                           \
		RESET_ClearPeripheralReset(kFC##x##_RST_SHIFT_RSTn);                               \
		CLOCK_EnableClock(kCLOCK_LPFlexComm##x);                                           \
	} while (0)

#if CONFIG_DT_HAS_NXP_MCUX_EDMA_ENABLED
static void edma_enable_all_request(uint8_t instance)
{
	uint32_t *reg;

	for (uint8_t idx = 0; idx < EN_NUM; idx++) {
		reg = EDMA_EN_REG(instance, idx);
		*reg |= 0xFFFFFFFFU;
	}
}
#endif

__weak void mimxrt798s_hifi4_irq_init(void)
{
	/**
	 * IRQ assignments
	 *
	 * NMI:
	 * - IRQ 0 (SEL 0): Yet unassigned
	 *
	 * L1: (lowest priority)
	 * - IRQ 6  (SEL 1):  FLEXCOMM0
	 * - IRQ 7  (SEL 2):  FLEXCOMM2
	 * - IRQ 8  (SEL 3):  WWDT1
	 * - IRQ 9  (SEL 4):  Unmapped
	 * - IRQ 10 (SEL 5):  Unmapped
	 * - IRQ 11 (SEL 6):  Unmapped
	 * - IRQ 12 (SEL 7):  Unmapped
	 * - IRQ 13 (SEL 8):  Unmapped
	 * - IRQ 14 (SEL 9):  Unmapped
	 * - IRQ 15 (SEL 10): Unmapped
	 *
	 * L2:
	 * - IRQ 16 (SEL 11): Unmapped
	 * - IRQ 17 (SEL 12): LPSPI14
	 * - IRQ 18 (SEL 13): MU2
	 * - IRQ 19 (SEL 14): MU4
	 * - IRQ 20 (SEL 15): EDMA1-0
	 * - IRQ 21 (SEL 16): EDMA1-1
	 * - IRQ 22 (SEL 17): EDMA1-2
	 * - IRQ 23 (SEL 18): EDMA1-3
	 *
	 * L3: (highest priority)
	 * - IRQ 24 (SEL 19): EDMA1-4
	 * - IRQ 25 (SEL 20): EDMA1-5
	 * - IRQ 26 (SEL 21): EDMA1-6
	 * - IRQ 27 (SEL 22): EDMA1-7
	 * - IRQ 28 (SEL 23): MICFIL
	 * - IRQ 29 (SEL 24): SAI0
	 * - IRQ 30 (SEL 25): SAI1
	 * - IRQ 31 (SEL 26): SAI2
	 */
	INPUTMUX_Init(INPUTMUX0);

	INPUTMUX_AttachSignal(INPUTMUX0, 1, kINPUTMUX_Flexcomm0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 2, kINPUTMUX_Flexcomm2ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 3, kINPUTMUX_Wdt1ToDspInterrupt);

	INPUTMUX_AttachSignal(INPUTMUX0, 12, kINPUTMUX_Spi14ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 13, kINPUTMUX_Mu2AToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 14, kINPUTMUX_Mu4BToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 15, kINPUTMUX_Dma1Irq0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 16, kINPUTMUX_Dma1Irq1ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 17, kINPUTMUX_Dma1Irq2ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 18, kINPUTMUX_Dma1Irq3ToDspInterrupt);

	INPUTMUX_AttachSignal(INPUTMUX0, 19, kINPUTMUX_Dma1Irq4ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 20, kINPUTMUX_Dma1Irq5ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 21, kINPUTMUX_Dma1Irq6ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 22, kINPUTMUX_Dma1Irq7ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 23, kINPUTMUX_MicfilToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 24, kINPUTMUX_Sai0ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 25, kINPUTMUX_Sai1ToDspInterrupt);
	INPUTMUX_AttachSignal(INPUTMUX0, 26, kINPUTMUX_Sai2ToDspInterrupt);

	INPUTMUX_Deinit(INPUTMUX0);
}

__weak void mimxrt798s_hifi4_clock_init(void)
{
	CLOCK_AttachClk(kOSC_CLK_to_FCCLK0);
	CLOCK_SetClkDiv(kCLOCK_DivFcclk0Clk, 1U);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm0), okay)
	SET_UP_FLEXCOMM_CLOCK(0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm2), okay)
	SET_UP_FLEXCOMM_CLOCK(2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(edma1), okay)
	CLOCK_EnableClock(kCLOCK_Dma1);
	RESET_ClearPeripheralReset(kDMA1_RST_SHIFT_RSTn);
	edma_enable_all_request(1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sai0), okay)
	/* SAI clock 368.64 / 15 = 24.576MHz */
	CLOCK_AttachClk(kAUDIO_PLL_PFD3_to_AUDIO_VDD2);
	CLOCK_AttachClk(kAUDIO_VDD2_to_SAI012);
	CLOCK_SetClkDiv(kCLOCK_DivSai012Clk, 15U);
	RESET_ClearPeripheralReset(kSAI0_RST_SHIFT_RSTn);
#endif
}

void soc_early_init_hook(void)
{
	mimxrt798s_hifi4_irq_init();
	mimxrt798s_hifi4_clock_init();
}
