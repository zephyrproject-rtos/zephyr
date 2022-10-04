/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARC_IOT_SOC_IRQ_H_
#define _ARC_IOT_SOC_IRQ_H_


#define IRQ_WATCHDOG				18
#define IRQ_GPIO_4B0				19
#define IRQ_DMA0_DONE				20
#define IRQ_DMA1_DONE				21
#define IRQ_DMA2_DONE				22
#define IRQ_DMA3_DONE				23
#define IRQ_DMA4_DONE				24
#define IRQ_DMA5_DONE				25
#define IRQ_DMA6_DONE				26
#define IRQ_DMA7_DONE				27
#define IRQ_DMA8_DONE				28
#define IRQ_DMA9_DONE				29
#define IRQ_DMA10_DONE				30
#define IRQ_DMA11_DONE				31
#define IRQ_DMA12_DONE				32
#define IRQ_DMA13_DONE				33
#define IRQ_DMA14_DONE				34
#define IRQ_DMA15_DONE				35
#define IRQ_DMA0_ERR				36
#define IRQ_DMA1_ERR				37
#define IRQ_DMA2_ERR				38
#define IRQ_DMA3_ERR				39
#define IRQ_DMA4_ERR				40
#define IRQ_DMA5_ERR				41
#define IRQ_DMA6_ERR				42
#define IRQ_DMA7_ERR				43
#define IRQ_DMA8_ERR				44
#define IRQ_DMA9_ERR				45
#define IRQ_DMA10_ERR				46
#define IRQ_DMA11_ERR				47
#define IRQ_DMA12_ERR				48
#define IRQ_DMA13_ERR				49
#define IRQ_DMA14_ERR				50
#define IRQ_DMA15_ERR				51

#define IRQ_GPIO_4B1				52
#define IRQ_GPIO_4B2				53
#define IRQ_GPIO_8B0				54
#define IRQ_GPIO_8B1				55
#define IRQ_GPIO_8B2				56
#define IRQ_GPIO_8B3				57

#define IRQ_I2CMST0_MST_ERR			58
#define IRQ_I2CMST0_MST_RX_AVAIL	59
#define IRQ_I2CMST0_MST_TX_REQ		60
#define IRQ_I2CMST0_MST_STOP_DET	61
#define IRQ_I2CMST1_MST_ERR			62
#define IRQ_I2CMST1_MST_RX_AVAIL	63
#define IRQ_I2CMST1_MST_TX_REQ		64
#define IRQ_I2CMST1_MST_STOP_DET	65
#define IRQ_I2CMST2_MST_ERR			66
#define IRQ_I2CMST2_MST_RX_AVAIL	67
#define IRQ_I2CMST2_MST_TX_REQ		68
#define IRQ_I2CMST2_MST_STOP_DET	69

/* SPI */
#define IRQ_SPIMST0_MST_ERR			70
#define IRQ_SPIMST0_MST_RX_AVAIL	71
#define IRQ_SPIMST0_MST_TX_REQ		72
#define IRQ_SPIMST0_MST_IDLE		73
#define IRQ_SPIMST1_MST_ERR			74
#define IRQ_SPIMST1_MST_RX_AVAIL	75
#define IRQ_SPIMST1_MST_TX_REQ		76
#define IRQ_SPIMST1_MST_IDLE		77
#define IRQ_SPIMST2_MST_ERR			78
#define IRQ_SPIMST2_MST_RX_AVAIL	79
#define IRQ_SPIMST2_MST_TX_REQ		80
#define IRQ_SPIMST2_MST_IDLE		81

#define IRQ_SPISLV0_SLV_ERR			82
#define IRQ_SPISLV0_SLV_RX_AVAIL	83
#define IRQ_SPISLV0_SLV_TX_REQ		84
#define IRQ_SPISLV0_SLV_IDLE		85

/* UART */
#define IRQ_UART0_UART				86
#define IRQ_UART1_UART				87
#define IRQ_UART2_UART				88
#define IRQ_UART3_UART				89

#define IRQ_EXT_WAKE_UP				90
#define IRQ_SDIO					91
/* I2S */
#define IRQ_I2S_TX_EMP_0			92
#define IRQ_I2S_TX_OR_0				93
#define IRQ_I2S_RX_DA_0				94
#define IRQ_I2S_RX_OR_0				95

#define IRQ_USB						96
#define IRQ_ADC						97

#define IRQ_DW_TIMER0				98
#define IRQ_DW_TIMER1				99
#define IRQ_DW_TIMER2				100
#define IRQ_DW_TIMER3				101
#define IRQ_DW_TIMER4				102
#define IRQ_DW_TIMER5				103
#define IRQ_DW_RTC					104
#define IRQ_DW_I3C					105

#define IRQ_RESERVED0				106
#define IRQ_RESERVED1				107
#define IRQ_RESERVED2				108
#define IRQ_RESERVED3				109
#define IRQ_RESERVED4				110

#endif /* _ARC_IOT_SOC_IRQ_H_ */
