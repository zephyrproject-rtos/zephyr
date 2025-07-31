/*
 * Copyright (c) 2020-2024 NXP
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCUX_LPC_SYSCON_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCUX_LPC_SYSCON_CLOCK_H_

/* LPC54S018 clock identifiers */

/* System clocks */
#define MCUX_FRO_CLK		0
#define MCUX_MAIN_CLK		1
#define MCUX_SYS_PLL_CLK	2
#define MCUX_AUDIO_PLL_CLK	3
#define MCUX_USB_PLL_CLK	4
#define MCUX_RTC_CLK		5
#define MCUX_XTAL_CLK		6

/* Peripheral clocks */
#define MCUX_FLEXCOMM0_CLK	10
#define MCUX_FLEXCOMM1_CLK	11
#define MCUX_FLEXCOMM2_CLK	12
#define MCUX_FLEXCOMM3_CLK	13
#define MCUX_FLEXCOMM4_CLK	14
#define MCUX_FLEXCOMM5_CLK	15
#define MCUX_FLEXCOMM6_CLK	16
#define MCUX_FLEXCOMM7_CLK	17
#define MCUX_FLEXCOMM8_CLK	18
#define MCUX_FLEXCOMM9_CLK	19
#define MCUX_FLEXCOMM10_CLK	20

/* Timer clocks */
#define MCUX_CTIMER0_CLK	30
#define MCUX_CTIMER1_CLK	31
#define MCUX_CTIMER2_CLK	32
#define MCUX_CTIMER3_CLK	33
#define MCUX_CTIMER4_CLK	34
#define MCUX_SCT0_CLK		35

/* Other peripheral clocks */
#define MCUX_ADC0_CLK		40
#define MCUX_DMA0_CLK		41
#define MCUX_GPIO0_CLK		42
#define MCUX_GPIO1_CLK		43
#define MCUX_GPIO2_CLK		44
#define MCUX_GPIO3_CLK		45
#define MCUX_GPIO4_CLK		46
#define MCUX_USB0_CLK		47
#define MCUX_USB1_CLK		48
#define MCUX_FLEXSPI_CLK	49
#define MCUX_SPIFI_CLK		49  /* Alias for FLEXSPI */
#define MCUX_ENET_CLK		50
#define MCUX_MCAN0_CLK		51
#define MCUX_MCAN1_CLK		52
#define MCUX_LCD_CLK		53
#define MCUX_SDIO_CLK		54
#define MCUX_EMC_CLK		55
#define MCUX_CRC_CLK		56

/* Clock attach points for clock source selection */
#define MCUX_ATTACH_ID(mux, sel) \
	((((mux) & 0xFF) << 8) | ((sel) & 0xFF))

/* Main clock attach points */
#define kFRO12M_to_MAIN_CLK		MCUX_ATTACH_ID(0, 0)
#define kCLKIN_to_MAIN_CLK		MCUX_ATTACH_ID(0, 1)
#define kWDT_OSC_to_MAIN_CLK		MCUX_ATTACH_ID(0, 2)
#define kFRO_HF_to_MAIN_CLK		MCUX_ATTACH_ID(0, 3)
#define kSYS_PLL_to_MAIN_CLK		MCUX_ATTACH_ID(0, 4)
#define kMCLK_IN_to_MAIN_CLK		MCUX_ATTACH_ID(0, 5)
#define kRTC_to_MAIN_CLK		MCUX_ATTACH_ID(0, 6)

/* System PLL clock attach points */
#define kFRO12M_to_SYS_PLL		MCUX_ATTACH_ID(1, 0)
#define kCLKIN_to_SYS_PLL		MCUX_ATTACH_ID(1, 1)
#define kWDT_OSC_to_SYS_PLL		MCUX_ATTACH_ID(1, 2)
#define kRTC_to_SYS_PLL		MCUX_ATTACH_ID(1, 3)
#define kNONE_to_SYS_PLL		MCUX_ATTACH_ID(1, 7)

/* FLEXCOMM clock attach points */
#define kFRO12M_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 0)
#define kFRO_HF_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 1)
#define kAUDIO_PLL_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 2)
#define kMCLK_IN_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 3)
#define kFRG_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 4)
#define kMAIN_CLK_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 5)
#define kSYS_PLL_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 6)
#define kNONE_to_FLEXCOMM0		MCUX_ATTACH_ID(8, 7)

/* Audio PLL clock attach points */
#define kFRO12M_to_AUDIO_PLL		MCUX_ATTACH_ID(2, 0)
#define kCLKIN_to_AUDIO_PLL		MCUX_ATTACH_ID(2, 1)
#define kNONE_to_AUDIO_PLL		MCUX_ATTACH_ID(2, 7)

/* USB clock attach points */
#define kFRO_HF_to_USB_CLK		MCUX_ATTACH_ID(30, 0)
#define kSYS_PLL_to_USB_CLK		MCUX_ATTACH_ID(30, 1)
#define kMAIN_CLK_to_USB_CLK		MCUX_ATTACH_ID(30, 2)
#define kUSB_PLL_to_USB_CLK		MCUX_ATTACH_ID(30, 3)
#define kNONE_to_USB_CLK		MCUX_ATTACH_ID(30, 7)

/* ADC clock attach points */
#define kFRO_HF_to_ADC_CLK		MCUX_ATTACH_ID(31, 0)
#define kSYS_PLL_to_ADC_CLK		MCUX_ATTACH_ID(31, 1)
#define kUSB_PLL_to_ADC_CLK		MCUX_ATTACH_ID(31, 2)
#define kAUDIO_PLL_to_ADC_CLK		MCUX_ATTACH_ID(31, 3)
#define kNONE_to_ADC_CLK		MCUX_ATTACH_ID(31, 7)

/* MCAN clock attach points */
#define kMAIN_CLK_to_MCAN_CLK		MCUX_ATTACH_ID(32, 0)
#define kSYS_PLL_to_MCAN_CLK		MCUX_ATTACH_ID(32, 1)
#define kFRO_HF_to_MCAN_CLK		MCUX_ATTACH_ID(32, 2)
#define kAUDIO_PLL_to_MCAN_CLK		MCUX_ATTACH_ID(32, 3)
#define kNONE_to_MCAN_CLK		MCUX_ATTACH_ID(32, 7)

/* Clock divider IDs */
#define kCLOCK_DivSystickClk		0
#define kCLOCK_DivArmTrClkDiv		1
#define kCLOCK_DivCan0Clk		2
#define kCLOCK_DivCan1Clk		3
#define kCLOCK_DivAhbClk		4
#define kCLOCK_DivClkOut		5
#define kCLOCK_DivFrg			6
#define kCLOCK_DivSpifiClk		7
#define kCLOCK_DivAdcAsyncClk		8
#define kCLOCK_DivUsb0Clk		9
#define kCLOCK_DivUsb1Clk		10
#define kCLOCK_DivLcdClk		11
#define kCLOCK_DivSdioClk		12
#define kCLOCK_DivEmcClk		13

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCUX_LPC_SYSCON_CLOCK_H_ */