/*
 * Copyright 2024  NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <fsl_clock.h>
#include <soc.h>
#include <fsl_glikey.h>

static void BOARD_InitAHBSC(void);

#define SET_UP_FLEXCOMM_CLOCK(x)                                                                   \
	do {                                                                                       \
		CLOCK_AttachClk(kFCCLK0_to_FLEXCOMM##x);                                           \
		RESET_ClearPeripheralReset(kFC##x##_RST_SHIFT_RSTn);                               \
		CLOCK_EnableClock(kCLOCK_LPFlexComm##x);                                           \
	} while (0)

static int mimxrt700_evk_init(void)
{
	BOARD_InitAHBSC();

#if DT_NODE_HAS_STATUS(DT_NODELABEL(iocon0), okay)
	RESET_ClearPeripheralReset(kIOPCTL0_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Iopctl0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(iocon1), okay)
	RESET_ClearPeripheralReset(kIOPCTL1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Iopctl1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(iocon2), okay)
	RESET_ClearPeripheralReset(kIOPCTL2_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Iopctl2);
#endif

#ifdef CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CPU0
	CLOCK_AttachClk(kOSC_CLK_to_FCCLK0);
	CLOCK_SetClkDiv(kCLOCK_DivFcclk0Clk, 1U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm0), okay)
	SET_UP_FLEXCOMM_CLOCK(0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm1), okay)
	SET_UP_FLEXCOMM_CLOCK(1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm2), okay)
	SET_UP_FLEXCOMM_CLOCK(2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm3), okay)
	SET_UP_FLEXCOMM_CLOCK(3);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm4), okay)
	SET_UP_FLEXCOMM_CLOCK(4);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm5), okay)
	SET_UP_FLEXCOMM_CLOCK(5);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm6), okay)
	SET_UP_FLEXCOMM_CLOCK(6);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm7), okay)
	SET_UP_FLEXCOMM_CLOCK(7);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm8), okay)
	SET_UP_FLEXCOMM_CLOCK(8);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm9), okay)
	SET_UP_FLEXCOMM_CLOCK(9);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm10), okay)
	SET_UP_FLEXCOMM_CLOCK(10);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm11), okay)
	SET_UP_FLEXCOMM_CLOCK(11);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm12), okay)
	SET_UP_FLEXCOMM_CLOCK(12);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm13), okay)
	SET_UP_FLEXCOMM_CLOCK(13);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi14), okay)
	CLOCK_EnableClock(kCLOCK_LPSpi14);
	RESET_ClearPeripheralReset(kLPSPI14_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c15), okay)
	CLOCK_EnableClock(kCLOCK_LPI2c15);
	RESET_ClearPeripheralReset(kLPI2C15_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi16), okay)
	CLOCK_AttachClk(kFRO0_DIV1_to_LPSPI16);
	CLOCK_SetClkDiv(kCLOCK_DivLpspi16Clk, 1U);
	CLOCK_EnableClock(kCLOCK_LPSpi16);
	RESET_ClearPeripheralReset(kLPSPI16_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm17), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM17);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm17Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm18), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM18);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm18Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm19), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM19);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm19Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm20), okay)
	CLOCK_AttachClk(kSENSE_BASE_to_FLEXCOMM20);
	CLOCK_SetClkDiv(kCLOCK_DivLPFlexComm20Clk, 4U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	CLOCK_EnableClock(kCLOCK_Gpio0);
	RESET_ClearPeripheralReset(kGPIO0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	CLOCK_EnableClock(kCLOCK_Gpio1);
	RESET_ClearPeripheralReset(kGPIO1_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio2), okay)
	CLOCK_EnableClock(kCLOCK_Gpio2);
	RESET_ClearPeripheralReset(kGPIO2_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio3), okay)
	CLOCK_EnableClock(kCLOCK_Gpio3);
	RESET_ClearPeripheralReset(kGPIO3_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio4), okay)
	CLOCK_EnableClock(kCLOCK_Gpio4);
	RESET_ClearPeripheralReset(kGPIO4_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio5), okay)
	CLOCK_EnableClock(kCLOCK_Gpio5);
	RESET_ClearPeripheralReset(kGPIO5_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio6), okay)
	CLOCK_EnableClock(kCLOCK_Gpio6);
	RESET_ClearPeripheralReset(kGPIO6_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio7), okay)
	CLOCK_EnableClock(kCLOCK_Gpio7);
	RESET_ClearPeripheralReset(kGPIO7_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio8), okay)
	CLOCK_EnableClock(kCLOCK_Gpio8);
	RESET_ClearPeripheralReset(kGPIO8_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio9), okay)
	CLOCK_EnableClock(kCLOCK_Gpio9);
	RESET_ClearPeripheralReset(kGPIO9_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio10), okay)
	CLOCK_EnableClock(kCLOCK_Gpio10);
	RESET_ClearPeripheralReset(kGPIO10_RST_SHIFT_RSTn);
#endif

	return 0;
}

static void GlikeyWriteEnable(GLIKEY_Type *base, uint8_t idx)
{
	(void)GLIKEY_SyncReset(base);

	(void)GLIKEY_StartEnable(base, idx);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP1);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP2);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP3);
	(void)GLIKEY_ContinueEnable(base, GLIKEY_CODEWORD_STEP_EN);
}

static void GlikeyClearConfig(GLIKEY_Type *base)
{
	(void)GLIKEY_SyncReset(base);
}

/* Disable the secure check for AHBSC and enable periperhals/sram access for masters */
static void BOARD_InitAHBSC(void)
{
#if defined(CONFIG_SOC_MIMXRT798S_CPU0)
	GlikeyWriteEnable(GLIKEY0, 1U);
	AHBSC0->MISC_CTRL_DP_REG = 0x000086aa;
	/* AHBSC0 MISC_CTRL_REG, disable Privilege & Secure checking. */
	AHBSC0->MISC_CTRL_REG = 0x000086aa;

	GlikeyWriteEnable(GLIKEY0, 7U);
	/* Enable arbiter0 accessing SRAM */
	AHBSC0->COMPUTE_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->SENSE_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->MEDIA_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->NPU_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC0->HIFI4_ARB0RAM_ACCESS_ENABLE = 0x3FFFFFFF;
#endif

	GlikeyWriteEnable(GLIKEY1, 1U);
	AHBSC3->MISC_CTRL_DP_REG = 0x000086aa;
	/* AHBSC3 MISC_CTRL_REG, disable Privilege & Secure checking.*/
	AHBSC3->MISC_CTRL_REG = 0x000086aa;

	GlikeyWriteEnable(GLIKEY1, 9U);
	/* Enable arbiter1 accessing SRAM */
	AHBSC3->COMPUTE_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->SENSE_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->MEDIA_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->NPU_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->HIFI4_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;
	AHBSC3->HIFI1_ARB1RAM_ACCESS_ENABLE = 0x3FFFFFFF;

	GlikeyWriteEnable(GLIKEY1, 8U);
	/* Access enable for COMPUTE domain masters to common APB peripherals.*/
	AHBSC3->COMPUTE_APB_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;
	AHBSC3->SENSE_APB_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;
	GlikeyWriteEnable(GLIKEY1, 7U);
	AHBSC3->COMPUTE_AIPS_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;
	AHBSC3->SENSE_AIPS_PERIPHERAL_ACCESS_ENABLE = 0xffffffff;

	GlikeyWriteEnable(GLIKEY2, 1U);
	/*Disable secure and secure privilege checking. */
	AHBSC4->MISC_CTRL_DP_REG = 0x000086aa;
	AHBSC4->MISC_CTRL_REG = 0x000086aa;

#if defined(CONFIG_SOC_MIMXRT798S_CPU0)
	GlikeyClearConfig(GLIKEY0);
#endif
	GlikeyClearConfig(GLIKEY1);
	GlikeyClearConfig(GLIKEY2);
}

SYS_INIT(mimxrt700_evk_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
