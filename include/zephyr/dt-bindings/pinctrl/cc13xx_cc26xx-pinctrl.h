/*
 * Copyright (c) 2015 - 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CC13XX_CC26XX_PINCTRL_COMMON_H_
#define CC13XX_CC26XX_PINCTRL_COMMON_H_

/* Adapted from hal/ti/simplelink/source/ti/devices/cc13x2_cc26x2/driverlib/ioc.h */

/* IOC Peripheral Port Mapping */
#define IOC_PORT_GPIO             0x00000000  /* Default general purpose IO usage */
#define IOC_PORT_AON_CLK32K       0x00000007  /* AON External 32kHz clock */
#define IOC_PORT_AUX_IO           0x00000008  /* AUX IO Pin */
#define IOC_PORT_MCU_SSI0_RX      0x00000009  /* MCU SSI0 Receive Pin */
#define IOC_PORT_MCU_SSI0_TX      0x0000000A  /* MCU SSI0 Transmit Pin */
#define IOC_PORT_MCU_SSI0_FSS     0x0000000B  /* MCU SSI0 FSS Pin */
#define IOC_PORT_MCU_SSI0_CLK     0x0000000C  /* MCU SSI0 Clock Pin */
#define IOC_PORT_MCU_I2C_MSSDA    0x0000000D  /* MCU I2C Data Pin */
#define IOC_PORT_MCU_I2C_MSSCL    0x0000000E  /* MCU I2C Clock Pin */
#define IOC_PORT_MCU_UART0_RX     0x0000000F  /* MCU UART0 Receive Pin */
#define IOC_PORT_MCU_UART0_TX     0x00000010  /* MCU UART0 Transmit Pin */
#define IOC_PORT_MCU_UART0_CTS    0x00000011  /* MCU UART0 Clear To Send Pin */
#define IOC_PORT_MCU_UART0_RTS    0x00000012  /* MCU UART0 Request To Send Pin */
#define IOC_PORT_MCU_UART1_RX     0x00000013  /* MCU UART1 Receive Pin */
#define IOC_PORT_MCU_UART1_TX     0x00000014  /* MCU UART1 Transmit Pin */
#define IOC_PORT_MCU_UART1_CTS    0x00000015  /* MCU UART1 Clear To Send Pin */
#define IOC_PORT_MCU_UART1_RTS    0x00000016  /* MCU UART1 Request To Send Pin */
#define IOC_PORT_MCU_PORT_EVENT0  0x00000017  /* MCU PORT EVENT 0 */
#define IOC_PORT_MCU_PORT_EVENT1  0x00000018  /* MCU PORT EVENT 1 */
#define IOC_PORT_MCU_PORT_EVENT2  0x00000019  /* MCU PORT EVENT 2 */
#define IOC_PORT_MCU_PORT_EVENT3  0x0000001A  /* MCU PORT EVENT 3 */
#define IOC_PORT_MCU_PORT_EVENT4  0x0000001B  /* MCU PORT EVENT 4 */
#define IOC_PORT_MCU_PORT_EVENT5  0x0000001C  /* MCU PORT EVENT 5 */
#define IOC_PORT_MCU_PORT_EVENT6  0x0000001D  /* MCU PORT EVENT 6 */
#define IOC_PORT_MCU_PORT_EVENT7  0x0000001E  /* MCU PORT EVENT 7 */
#define IOC_PORT_MCU_SWV          0x00000020  /* Serial Wire Viewer */
#define IOC_PORT_MCU_SSI1_RX      0x00000021  /* MCU SSI1 Receive Pin */
#define IOC_PORT_MCU_SSI1_TX      0x00000022  /* MCU SSI1 Transmit Pin */
#define IOC_PORT_MCU_SSI1_FSS     0x00000023  /* MCU SSI1 FSS Pin */
#define IOC_PORT_MCU_SSI1_CLK     0x00000024  /* MCU SSI1 Clock Pin */
#define IOC_PORT_MCU_I2S_AD0      0x00000025  /* MCU I2S Data Pin 0 */
#define IOC_PORT_MCU_I2S_AD1      0x00000026  /* MCU I2S Data Pin 1 */
#define IOC_PORT_MCU_I2S_WCLK     0x00000027  /* MCU I2S Frame/Word Clock */
#define IOC_PORT_MCU_I2S_BCLK     0x00000028  /* MCU I2S Bit Clock */
#define IOC_PORT_MCU_I2S_MCLK     0x00000029  /* MCU I2S Master clock 2 */
#define IOC_PORT_RFC_TRC          0x0000002E  /* RF Core Tracer */
#define IOC_PORT_RFC_GPO0         0x0000002F  /* RC Core Data Out Pin 0 */
#define IOC_PORT_RFC_GPO1         0x00000030  /* RC Core Data Out Pin 1 */
#define IOC_PORT_RFC_GPO2         0x00000031  /* RC Core Data Out Pin 2 */
#define IOC_PORT_RFC_GPO3         0x00000032  /* RC Core Data Out Pin 3 */
#define IOC_PORT_RFC_GPI0         0x00000033  /* RC Core Data In Pin 0 */
#define IOC_PORT_RFC_GPI1         0x00000034  /* RC Core Data In Pin 1 */
#define IOC_PORT_RFC_SMI_DL_OUT   0x00000035  /* RF Core SMI Data Link Out */
#define IOC_PORT_RFC_SMI_DL_IN    0x00000036  /* RF Core SMI Data Link in */
#define IOC_PORT_RFC_SMI_CL_OUT   0x00000037  /* RF Core SMI Command Link Out */
#define IOC_PORT_RFC_SMI_CL_IN    0x00000038  /* RF Core SMI Command Link In */

#endif  /* CC13XX_CC26XX_PINCTRL_COMMON_H_ */
