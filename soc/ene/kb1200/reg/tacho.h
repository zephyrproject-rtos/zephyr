/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_TACHO_H
#define ENE_KB1200_TACHO_H

/**
 * brief Structure type to access TACHO.
 */
struct tacho_regs {
	volatile uint16_t TACHOCFG;      /*Configuration Register */
	volatile uint16_t Reserved0;     /*Reserved */
	volatile uint8_t  TACHOIE;       /*Interrupt Enable Register */
	volatile uint8_t  Reserved1[3];  /*Reserved */
	volatile uint8_t  TACHOPF;       /*Event Pending Flag Register */
	volatile uint8_t  Reserved2[3];  /*Reserved */
	volatile uint16_t TACHOCV;       /*TACHO0 Counter Value Register */
	volatile uint16_t Reserved3;     /*Reserved */
};

#define TACHO_CNT_MAX_VALUE                 0x7FFF

#define TACHO_TIMEOUT_EVENT                 0x02
#define TACHO_UPDATE_EVENT                  0x01

#define TACHO_MONITOR_CLK_64US              0
#define TACHO_MONITOR_CLK_16US              1
#define TACHO_MONITOR_CLK_8US               2
#define TACHO_MONITOR_CLK_2US               3

#define TACHO_FUNCTION_ENABLE               0x0001
#define TACHO_RING_EDGE_SAMPLE              0x0000
#define TACHO_EDGE_CHANGE_SAMPLE            0x0080
#define TACHO_FILTER_ENABLE                 0x8000

#endif /* ENE_KB1200_TACHO_H */
