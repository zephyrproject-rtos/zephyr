/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_WDT_H
#define ENE_KB1200_WDT_H

/**
 * brief  Structure type to access Watch Dog Timer (WDT).
 */
struct wdt_regs {
	volatile uint8_t  WDTCFG;        /*Configuration Register */
	volatile uint8_t  WDTCFG_T;      /*Configuration Reset Type Register */
	volatile uint16_t Reserved0;     /*Reserved */
	volatile uint8_t  WDTIE;         /*Interrupt Enable Register */
	volatile uint8_t  Reserved1[3];  /*Reserved */
	volatile uint8_t  WDTPF;         /*Event Pending Flag Register */
	volatile uint8_t  Reserved2[3];  /*Reserved */
	volatile uint16_t WDTM;          /*WDT Match Value Register */
	volatile uint16_t Reserved3;     /*Reserved */
	volatile uint8_t  WDTSCR[4];     /*FW Scratch(4 bytes) Register */
};

#define WDT_MIN_CNT                         3U
#define WDT_SAMPLE_TIME                     31.25

#define WDT_RESET_WHOLE_CHIP_WO_GPIO        0
#define WDT_RESET_WHOLE_CHIP                1
#define WDT_RESET_ONLY_MCU                  2

#define WDT_DISABLE_PASSWORD                0x90
#define WDT_ADCO32K                         0x00
#define WDT_PHER32K                         0x02
#define WDT_FUNCTON_ENABLE                  0x01

#define WDT_HALF_WAY_EVENT                  0x01
#define WDT_RESET_EVENT                     0x02

#endif /* ENE_KB1200_WDT_H */
