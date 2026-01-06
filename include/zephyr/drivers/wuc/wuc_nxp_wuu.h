/*
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_WUC_NXP_WUU_H_
#define ZEPHYR_INCLUDE_WUC_NXP_WUU_H_

/* Wakeup source encoding:
 * Bits [31:24] - Source type (0=PIN, 1=MODULE)
 * Bits [23:16] - Index (pin/module number)
 * Bits [15:8]  - Edge type (1=RISING, 2=FALLING, 3=ANY) - for PIN only
 * Bits [7:0]   - Event type (0=INT, 1=DMA, 2=TRIGGER)
 */

#define NXP_WUU_SOURCE_TYPE_POS  24
#define NXP_WUU_SOURCE_INDEX_POS 16
#define NXP_WUU_SOURCE_EDGE_POS  8
#define NXP_WUU_SOURCE_EVENT_POS 0

#define NXP_WUU_SOURCE_MASK 0xFF

#define NXP_WUU_SOURCE_TYPE_PIN    0
#define NXP_WUU_SOURCE_TYPE_MODULE 1

#define NXP_WUU_EDGE_RISING  1
#define NXP_WUU_EDGE_FALLING 2
#define NXP_WUU_EDGE_ANY     3

#define NXP_WUU_EVENT_INT     0
#define NXP_WUU_EVENT_DMA     1
#define NXP_WUU_EVENT_TRIGGER 2

#define NXP_WUU_WAKEUP_SOURCE_ENCODE(type, index, edge, event)                                     \
	(((type) << NXP_WUU_SOURCE_TYPE_POS) | ((index) << NXP_WUU_SOURCE_INDEX_POS) |             \
	 ((edge) << NXP_WUU_SOURCE_EDGE_POS) | ((event) << NXP_WUU_SOURCE_EVENT_POS))

#define NXP_WUU_WAKEUP_SOURCE_DECODE(source, pos) (((source) >> (pos)) & NXP_WUU_SOURCE_MASK)

#define NXP_WUU_WAKEUP_SOURCE_DECODE_TYPE(source)                                                  \
	NXP_WUU_WAKEUP_SOURCE_DECODE(source, NXP_WUU_SOURCE_TYPE_POS)

#define NXP_WUU_IS_PIN_SOURCE(source)	\
	(NXP_WUU_WAKEUP_SOURCE_DECODE_TYPE(source) == NXP_WUU_SOURCE_TYPE_PIN)

#define NXP_WUU_IS_MODULE_SOURCE(source) \
	(NXP_WUU_WAKEUP_SOURCE_DECODE_TYPE(source) == NXP_WUU_SOURCE_TYPE_MODULE)

#define NXP_WUU_WAKEUP_SOURCE_DECODE_INDEX(source)                                                 \
	NXP_WUU_WAKEUP_SOURCE_DECODE(source, NXP_WUU_SOURCE_INDEX_POS)

#define NXP_WUU_WAKEUP_SOURCE_DECODE_EDGE(source)                                                  \
	NXP_WUU_WAKEUP_SOURCE_DECODE(source, NXP_WUU_SOURCE_EDGE_POS)

#define NXP_WUU_WAKEUP_SOURCE_DECODE_EVENT(source)                                                 \
	NXP_WUU_WAKEUP_SOURCE_DECODE(source, NXP_WUU_SOURCE_EVENT_POS)

/** Helper macros for PIN wakeup sources */
#define NXP_WUU_PIN(index, edge, event)                                                            \
	NXP_WUU_WAKEUP_SOURCE_ENCODE(NXP_WUU_SOURCE_TYPE_PIN, index, edge, event)

/** Helper macros for MODULE wakeup sources */
#define NXP_WUU_MODULE(index, event)                                                               \
	NXP_WUU_WAKEUP_SOURCE_ENCODE(NXP_WUU_SOURCE_TYPE_MODULE, index, 0, event)

/**
 * WUU supported Internal Modules
 */

/** Module 0 interrupt event */
#define NXP_WUU_MODULE_0_INT NXP_WUU_MODULE(0, NXP_WUU_EVENT_INT)
/** Module 0 DMA event */
#define NXP_WUU_MODULE_0_DMA NXP_WUU_MODULE(0, NXP_WUU_EVENT_DMA)

/** Module 1 interrupt event */
#define NXP_WUU_MODULE_1_INT NXP_WUU_MODULE(1, NXP_WUU_EVENT_INT)
/** Module 1 DMA event */
#define NXP_WUU_MODULE_1_DMA NXP_WUU_MODULE(1, NXP_WUU_EVENT_DMA)

/** Module 2 interrupt event */
#define NXP_WUU_MODULE_2_INT NXP_WUU_MODULE(2, NXP_WUU_EVENT_INT)
/** Module 2 DMA event */
#define NXP_WUU_MODULE_2_DMA NXP_WUU_MODULE(2, NXP_WUU_EVENT_DMA)

/** Module 3 interrupt event */
#define NXP_WUU_MODULE_3_INT NXP_WUU_MODULE(3, NXP_WUU_EVENT_INT)
/** Module 3 DMA event */
#define NXP_WUU_MODULE_3_DMA NXP_WUU_MODULE(3, NXP_WUU_EVENT_DMA)

/** Module 4 interrupt event */
#define NXP_WUU_MODULE_4_INT NXP_WUU_MODULE(4, NXP_WUU_EVENT_INT)
/** Module 4 DMA event */
#define NXP_WUU_MODULE_4_DMA NXP_WUU_MODULE(4, NXP_WUU_EVENT_DMA)

/** Module 5 interrupt event */
#define NXP_WUU_MODULE_5_INT NXP_WUU_MODULE(5, NXP_WUU_EVENT_INT)
/** Module 5 DMA event */
#define NXP_WUU_MODULE_5_DMA NXP_WUU_MODULE(5, NXP_WUU_EVENT_DMA)

/** Module 6 interrupt event */
#define NXP_WUU_MODULE_6_INT NXP_WUU_MODULE(6, NXP_WUU_EVENT_INT)
/** Module 6 DMA event */
#define NXP_WUU_MODULE_6_DMA NXP_WUU_MODULE(6, NXP_WUU_EVENT_DMA)

/** Module 7 interrupt event */
#define NXP_WUU_MODULE_7_INT NXP_WUU_MODULE(7, NXP_WUU_EVENT_INT)
/** Module 7 DMA event */
#define NXP_WUU_MODULE_7_DMA NXP_WUU_MODULE(7, NXP_WUU_EVENT_DMA)

/** Module 8 interrupt event */
#define NXP_WUU_MODULE_8_INT NXP_WUU_MODULE(8, NXP_WUU_EVENT_INT)
/** Module 8 DMA event */
#define NXP_WUU_MODULE_8_DMA NXP_WUU_MODULE(8, NXP_WUU_EVENT_DMA)

/**
 * WUU PIN wakeup sources
 */

/* Pin 0 wakeup sources */
/** Pin 0 rising edge interrupt */
#define NXP_WUU_PIN_0_RISING_INT      NXP_WUU_PIN(0, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 0 rising edge DMA */
#define NXP_WUU_PIN_0_RISING_DMA      NXP_WUU_PIN(0, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 0 rising edge trigger */
#define NXP_WUU_PIN_0_RISING_TRIGGER  NXP_WUU_PIN(0, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 0 falling edge interrupt */
#define NXP_WUU_PIN_0_FALLING_INT     NXP_WUU_PIN(0, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 0 falling edge DMA */
#define NXP_WUU_PIN_0_FALLING_DMA     NXP_WUU_PIN(0, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 0 falling edge trigger */
#define NXP_WUU_PIN_0_FALLING_TRIGGER NXP_WUU_PIN(0, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 0 any edge interrupt */
#define NXP_WUU_PIN_0_ANY_INT         NXP_WUU_PIN(0, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 0 any edge DMA */
#define NXP_WUU_PIN_0_ANY_DMA         NXP_WUU_PIN(0, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 0 any edge trigger */
#define NXP_WUU_PIN_0_ANY_TRIGGER     NXP_WUU_PIN(0, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 1 wakeup sources */
/** Pin 1 rising edge interrupt */
#define NXP_WUU_PIN_1_RISING_INT      NXP_WUU_PIN(1, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 1 rising edge DMA */
#define NXP_WUU_PIN_1_RISING_DMA      NXP_WUU_PIN(1, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 1 rising edge trigger */
#define NXP_WUU_PIN_1_RISING_TRIGGER  NXP_WUU_PIN(1, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 1 falling edge interrupt */
#define NXP_WUU_PIN_1_FALLING_INT     NXP_WUU_PIN(1, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 1 falling edge DMA */
#define NXP_WUU_PIN_1_FALLING_DMA     NXP_WUU_PIN(1, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 1 falling edge trigger */
#define NXP_WUU_PIN_1_FALLING_TRIGGER NXP_WUU_PIN(1, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 1 any edge interrupt */
#define NXP_WUU_PIN_1_ANY_INT         NXP_WUU_PIN(1, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 1 any edge DMA */
#define NXP_WUU_PIN_1_ANY_DMA         NXP_WUU_PIN(1, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 1 any edge trigger */
#define NXP_WUU_PIN_1_ANY_TRIGGER     NXP_WUU_PIN(1, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 2 wakeup sources */
/** Pin 2 rising edge interrupt */
#define NXP_WUU_PIN_2_RISING_INT      NXP_WUU_PIN(2, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 2 rising edge DMA */
#define NXP_WUU_PIN_2_RISING_DMA      NXP_WUU_PIN(2, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 2 rising edge trigger */
#define NXP_WUU_PIN_2_RISING_TRIGGER  NXP_WUU_PIN(2, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 2 falling edge interrupt */
#define NXP_WUU_PIN_2_FALLING_INT     NXP_WUU_PIN(2, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 2 falling edge DMA */
#define NXP_WUU_PIN_2_FALLING_DMA     NXP_WUU_PIN(2, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 2 falling edge trigger */
#define NXP_WUU_PIN_2_FALLING_TRIGGER NXP_WUU_PIN(2, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 2 any edge interrupt */
#define NXP_WUU_PIN_2_ANY_INT         NXP_WUU_PIN(2, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 2 any edge DMA */
#define NXP_WUU_PIN_2_ANY_DMA         NXP_WUU_PIN(2, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 2 any edge trigger */
#define NXP_WUU_PIN_2_ANY_TRIGGER     NXP_WUU_PIN(2, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 3 wakeup sources */
/** Pin 3 rising edge interrupt */
#define NXP_WUU_PIN_3_RISING_INT      NXP_WUU_PIN(3, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 3 rising edge DMA */
#define NXP_WUU_PIN_3_RISING_DMA      NXP_WUU_PIN(3, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 3 rising edge trigger */
#define NXP_WUU_PIN_3_RISING_TRIGGER  NXP_WUU_PIN(3, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 3 falling edge interrupt */
#define NXP_WUU_PIN_3_FALLING_INT     NXP_WUU_PIN(3, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 3 falling edge DMA */
#define NXP_WUU_PIN_3_FALLING_DMA     NXP_WUU_PIN(3, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 3 falling edge trigger */
#define NXP_WUU_PIN_3_FALLING_TRIGGER NXP_WUU_PIN(3, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 3 any edge interrupt */
#define NXP_WUU_PIN_3_ANY_INT         NXP_WUU_PIN(3, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 3 any edge DMA */
#define NXP_WUU_PIN_3_ANY_DMA         NXP_WUU_PIN(3, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 3 any edge trigger */
#define NXP_WUU_PIN_3_ANY_TRIGGER     NXP_WUU_PIN(3, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 4 wakeup sources */
/** Pin 4 rising edge interrupt */
#define NXP_WUU_PIN_4_RISING_INT      NXP_WUU_PIN(4, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 4 rising edge DMA */
#define NXP_WUU_PIN_4_RISING_DMA      NXP_WUU_PIN(4, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 4 rising edge trigger */
#define NXP_WUU_PIN_4_RISING_TRIGGER  NXP_WUU_PIN(4, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 4 falling edge interrupt */
#define NXP_WUU_PIN_4_FALLING_INT     NXP_WUU_PIN(4, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 4 falling edge DMA */
#define NXP_WUU_PIN_4_FALLING_DMA     NXP_WUU_PIN(4, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 4 falling edge trigger */
#define NXP_WUU_PIN_4_FALLING_TRIGGER NXP_WUU_PIN(4, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 4 any edge interrupt */
#define NXP_WUU_PIN_4_ANY_INT         NXP_WUU_PIN(4, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 4 any edge DMA */
#define NXP_WUU_PIN_4_ANY_DMA         NXP_WUU_PIN(4, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 4 any edge trigger */
#define NXP_WUU_PIN_4_ANY_TRIGGER     NXP_WUU_PIN(4, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 5 wakeup sources */
/** Pin 5 rising edge interrupt */
#define NXP_WUU_PIN_5_RISING_INT      NXP_WUU_PIN(5, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 5 rising edge DMA */
#define NXP_WUU_PIN_5_RISING_DMA      NXP_WUU_PIN(5, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 5 rising edge trigger */
#define NXP_WUU_PIN_5_RISING_TRIGGER  NXP_WUU_PIN(5, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 5 falling edge interrupt */
#define NXP_WUU_PIN_5_FALLING_INT     NXP_WUU_PIN(5, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 5 falling edge DMA */
#define NXP_WUU_PIN_5_FALLING_DMA     NXP_WUU_PIN(5, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 5 falling edge trigger */
#define NXP_WUU_PIN_5_FALLING_TRIGGER NXP_WUU_PIN(5, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 5 any edge interrupt */
#define NXP_WUU_PIN_5_ANY_INT         NXP_WUU_PIN(5, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 5 any edge DMA */
#define NXP_WUU_PIN_5_ANY_DMA         NXP_WUU_PIN(5, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 5 any edge trigger */
#define NXP_WUU_PIN_5_ANY_TRIGGER     NXP_WUU_PIN(5, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 6 wakeup sources */
/** Pin 6 rising edge interrupt */
#define NXP_WUU_PIN_6_RISING_INT      NXP_WUU_PIN(6, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 6 rising edge DMA */
#define NXP_WUU_PIN_6_RISING_DMA      NXP_WUU_PIN(6, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 6 rising edge trigger */
#define NXP_WUU_PIN_6_RISING_TRIGGER  NXP_WUU_PIN(6, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 6 falling edge interrupt */
#define NXP_WUU_PIN_6_FALLING_INT     NXP_WUU_PIN(6, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 6 falling edge DMA */
#define NXP_WUU_PIN_6_FALLING_DMA     NXP_WUU_PIN(6, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 6 falling edge trigger */
#define NXP_WUU_PIN_6_FALLING_TRIGGER NXP_WUU_PIN(6, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 6 any edge interrupt */
#define NXP_WUU_PIN_6_ANY_INT         NXP_WUU_PIN(6, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 6 any edge DMA */
#define NXP_WUU_PIN_6_ANY_DMA         NXP_WUU_PIN(6, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 6 any edge trigger */
#define NXP_WUU_PIN_6_ANY_TRIGGER     NXP_WUU_PIN(6, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 7 wakeup sources */
/** Pin 7 rising edge interrupt */
#define NXP_WUU_PIN_7_RISING_INT      NXP_WUU_PIN(7, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 7 rising edge DMA */
#define NXP_WUU_PIN_7_RISING_DMA      NXP_WUU_PIN(7, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 7 rising edge trigger */
#define NXP_WUU_PIN_7_RISING_TRIGGER  NXP_WUU_PIN(7, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 7 falling edge interrupt */
#define NXP_WUU_PIN_7_FALLING_INT     NXP_WUU_PIN(7, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 7 falling edge DMA */
#define NXP_WUU_PIN_7_FALLING_DMA     NXP_WUU_PIN(7, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 7 falling edge trigger */
#define NXP_WUU_PIN_7_FALLING_TRIGGER NXP_WUU_PIN(7, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 7 any edge interrupt */
#define NXP_WUU_PIN_7_ANY_INT         NXP_WUU_PIN(7, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 7 any edge DMA */
#define NXP_WUU_PIN_7_ANY_DMA         NXP_WUU_PIN(7, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 7 any edge trigger */
#define NXP_WUU_PIN_7_ANY_TRIGGER     NXP_WUU_PIN(7, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 8 wakeup sources */
/** Pin 8 rising edge interrupt */
#define NXP_WUU_PIN_8_RISING_INT      NXP_WUU_PIN(8, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 8 rising edge DMA */
#define NXP_WUU_PIN_8_RISING_DMA      NXP_WUU_PIN(8, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 8 rising edge trigger */
#define NXP_WUU_PIN_8_RISING_TRIGGER  NXP_WUU_PIN(8, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 8 falling edge interrupt */
#define NXP_WUU_PIN_8_FALLING_INT     NXP_WUU_PIN(8, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 8 falling edge DMA */
#define NXP_WUU_PIN_8_FALLING_DMA     NXP_WUU_PIN(8, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 8 falling edge trigger */
#define NXP_WUU_PIN_8_FALLING_TRIGGER NXP_WUU_PIN(8, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 8 any edge interrupt */
#define NXP_WUU_PIN_8_ANY_INT         NXP_WUU_PIN(8, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 8 any edge DMA */
#define NXP_WUU_PIN_8_ANY_DMA         NXP_WUU_PIN(8, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 8 any edge trigger */
#define NXP_WUU_PIN_8_ANY_TRIGGER     NXP_WUU_PIN(8, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 9 wakeup sources */
/** Pin 9 rising edge interrupt */
#define NXP_WUU_PIN_9_RISING_INT      NXP_WUU_PIN(9, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 9 rising edge DMA */
#define NXP_WUU_PIN_9_RISING_DMA      NXP_WUU_PIN(9, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 9 rising edge trigger */
#define NXP_WUU_PIN_9_RISING_TRIGGER  NXP_WUU_PIN(9, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 9 falling edge interrupt */
#define NXP_WUU_PIN_9_FALLING_INT     NXP_WUU_PIN(9, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 9 falling edge DMA */
#define NXP_WUU_PIN_9_FALLING_DMA     NXP_WUU_PIN(9, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 9 falling edge trigger */
#define NXP_WUU_PIN_9_FALLING_TRIGGER NXP_WUU_PIN(9, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 9 any edge interrupt */
#define NXP_WUU_PIN_9_ANY_INT         NXP_WUU_PIN(9, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 9 any edge DMA */
#define NXP_WUU_PIN_9_ANY_DMA         NXP_WUU_PIN(9, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 9 any edge trigger */
#define NXP_WUU_PIN_9_ANY_TRIGGER     NXP_WUU_PIN(9, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 10 wakeup sources */
/** Pin 10 rising edge interrupt */
#define NXP_WUU_PIN_10_RISING_INT      NXP_WUU_PIN(10, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 10 rising edge DMA */
#define NXP_WUU_PIN_10_RISING_DMA      NXP_WUU_PIN(10, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 10 rising edge trigger */
#define NXP_WUU_PIN_10_RISING_TRIGGER  NXP_WUU_PIN(10, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 10 falling edge interrupt */
#define NXP_WUU_PIN_10_FALLING_INT     NXP_WUU_PIN(10, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 10 falling edge DMA */
#define NXP_WUU_PIN_10_FALLING_DMA     NXP_WUU_PIN(10, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 10 falling edge trigger */
#define NXP_WUU_PIN_10_FALLING_TRIGGER NXP_WUU_PIN(10, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 10 any edge interrupt */
#define NXP_WUU_PIN_10_ANY_INT         NXP_WUU_PIN(10, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 10 any edge DMA */
#define NXP_WUU_PIN_10_ANY_DMA         NXP_WUU_PIN(10, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 10 any edge trigger */
#define NXP_WUU_PIN_10_ANY_TRIGGER     NXP_WUU_PIN(10, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 11 wakeup sources */
/** Pin 11 rising edge interrupt */
#define NXP_WUU_PIN_11_RISING_INT      NXP_WUU_PIN(11, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 11 rising edge DMA */
#define NXP_WUU_PIN_11_RISING_DMA      NXP_WUU_PIN(11, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 11 rising edge trigger */
#define NXP_WUU_PIN_11_RISING_TRIGGER  NXP_WUU_PIN(11, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 11 falling edge interrupt */
#define NXP_WUU_PIN_11_FALLING_INT     NXP_WUU_PIN(11, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 11 falling edge DMA */
#define NXP_WUU_PIN_11_FALLING_DMA     NXP_WUU_PIN(11, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 11 falling edge trigger */
#define NXP_WUU_PIN_11_FALLING_TRIGGER NXP_WUU_PIN(11, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 11 any edge interrupt */
#define NXP_WUU_PIN_11_ANY_INT         NXP_WUU_PIN(11, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 11 any edge DMA */
#define NXP_WUU_PIN_11_ANY_DMA         NXP_WUU_PIN(11, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 11 any edge trigger */
#define NXP_WUU_PIN_11_ANY_TRIGGER     NXP_WUU_PIN(11, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 12 wakeup sources */
/** Pin 12 rising edge interrupt */
#define NXP_WUU_PIN_12_RISING_INT      NXP_WUU_PIN(12, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 12 rising edge DMA */
#define NXP_WUU_PIN_12_RISING_DMA      NXP_WUU_PIN(12, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 12 rising edge trigger */
#define NXP_WUU_PIN_12_RISING_TRIGGER  NXP_WUU_PIN(12, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 12 falling edge interrupt */
#define NXP_WUU_PIN_12_FALLING_INT     NXP_WUU_PIN(12, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 12 falling edge DMA */
#define NXP_WUU_PIN_12_FALLING_DMA     NXP_WUU_PIN(12, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 12 falling edge trigger */
#define NXP_WUU_PIN_12_FALLING_TRIGGER NXP_WUU_PIN(12, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 12 any edge interrupt */
#define NXP_WUU_PIN_12_ANY_INT         NXP_WUU_PIN(12, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 12 any edge DMA */
#define NXP_WUU_PIN_12_ANY_DMA         NXP_WUU_PIN(12, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 12 any edge trigger */
#define NXP_WUU_PIN_12_ANY_TRIGGER     NXP_WUU_PIN(12, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 13 wakeup sources */
/** Pin 13 rising edge interrupt */
#define NXP_WUU_PIN_13_RISING_INT      NXP_WUU_PIN(13, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 13 rising edge DMA */
#define NXP_WUU_PIN_13_RISING_DMA      NXP_WUU_PIN(13, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 13 rising edge trigger */
#define NXP_WUU_PIN_13_RISING_TRIGGER  NXP_WUU_PIN(13, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 13 falling edge interrupt */
#define NXP_WUU_PIN_13_FALLING_INT     NXP_WUU_PIN(13, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 13 falling edge DMA */
#define NXP_WUU_PIN_13_FALLING_DMA     NXP_WUU_PIN(13, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 13 falling edge trigger */
#define NXP_WUU_PIN_13_FALLING_TRIGGER NXP_WUU_PIN(13, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 13 any edge interrupt */
#define NXP_WUU_PIN_13_ANY_INT         NXP_WUU_PIN(13, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 13 any edge DMA */
#define NXP_WUU_PIN_13_ANY_DMA         NXP_WUU_PIN(13, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 13 any edge trigger */
#define NXP_WUU_PIN_13_ANY_TRIGGER     NXP_WUU_PIN(13, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 14 wakeup sources */
/** Pin 14 rising edge interrupt */
#define NXP_WUU_PIN_14_RISING_INT      NXP_WUU_PIN(14, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 14 rising edge DMA */
#define NXP_WUU_PIN_14_RISING_DMA      NXP_WUU_PIN(14, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 14 rising edge trigger */
#define NXP_WUU_PIN_14_RISING_TRIGGER  NXP_WUU_PIN(14, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 14 falling edge interrupt */
#define NXP_WUU_PIN_14_FALLING_INT     NXP_WUU_PIN(14, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 14 falling edge DMA */
#define NXP_WUU_PIN_14_FALLING_DMA     NXP_WUU_PIN(14, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 14 falling edge trigger */
#define NXP_WUU_PIN_14_FALLING_TRIGGER NXP_WUU_PIN(14, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 14 any edge interrupt */
#define NXP_WUU_PIN_14_ANY_INT         NXP_WUU_PIN(14, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 14 any edge DMA */
#define NXP_WUU_PIN_14_ANY_DMA         NXP_WUU_PIN(14, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 14 any edge trigger */
#define NXP_WUU_PIN_14_ANY_TRIGGER     NXP_WUU_PIN(14, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 15 wakeup sources */
/** Pin 15 rising edge interrupt */
#define NXP_WUU_PIN_15_RISING_INT      NXP_WUU_PIN(15, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 15 rising edge DMA */
#define NXP_WUU_PIN_15_RISING_DMA      NXP_WUU_PIN(15, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 15 rising edge trigger */
#define NXP_WUU_PIN_15_RISING_TRIGGER  NXP_WUU_PIN(15, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 15 falling edge interrupt */
#define NXP_WUU_PIN_15_FALLING_INT     NXP_WUU_PIN(15, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 15 falling edge DMA */
#define NXP_WUU_PIN_15_FALLING_DMA     NXP_WUU_PIN(15, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 15 falling edge trigger */
#define NXP_WUU_PIN_15_FALLING_TRIGGER NXP_WUU_PIN(15, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 15 any edge interrupt */
#define NXP_WUU_PIN_15_ANY_INT         NXP_WUU_PIN(15, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 15 any edge DMA */
#define NXP_WUU_PIN_15_ANY_DMA         NXP_WUU_PIN(15, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 15 any edge trigger */
#define NXP_WUU_PIN_15_ANY_TRIGGER     NXP_WUU_PIN(15, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 16 wakeup sources */
/** Pin 16 rising edge interrupt */
#define NXP_WUU_PIN_16_RISING_INT      NXP_WUU_PIN(16, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 16 rising edge DMA */
#define NXP_WUU_PIN_16_RISING_DMA      NXP_WUU_PIN(16, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 16 rising edge trigger */
#define NXP_WUU_PIN_16_RISING_TRIGGER  NXP_WUU_PIN(16, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 16 falling edge interrupt */
#define NXP_WUU_PIN_16_FALLING_INT     NXP_WUU_PIN(16, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 16 falling edge DMA */
#define NXP_WUU_PIN_16_FALLING_DMA     NXP_WUU_PIN(16, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 16 falling edge trigger */
#define NXP_WUU_PIN_16_FALLING_TRIGGER NXP_WUU_PIN(16, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 16 any edge interrupt */
#define NXP_WUU_PIN_16_ANY_INT         NXP_WUU_PIN(16, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 16 any edge DMA */
#define NXP_WUU_PIN_16_ANY_DMA         NXP_WUU_PIN(16, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 16 any edge trigger */
#define NXP_WUU_PIN_16_ANY_TRIGGER     NXP_WUU_PIN(16, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 17 wakeup sources */
/** Pin 17 rising edge interrupt */
#define NXP_WUU_PIN_17_RISING_INT      NXP_WUU_PIN(17, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 17 rising edge DMA */
#define NXP_WUU_PIN_17_RISING_DMA      NXP_WUU_PIN(17, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 17 rising edge trigger */
#define NXP_WUU_PIN_17_RISING_TRIGGER  NXP_WUU_PIN(17, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 17 falling edge interrupt */
#define NXP_WUU_PIN_17_FALLING_INT     NXP_WUU_PIN(17, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 17 falling edge DMA */
#define NXP_WUU_PIN_17_FALLING_DMA     NXP_WUU_PIN(17, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 17 falling edge trigger */
#define NXP_WUU_PIN_17_FALLING_TRIGGER NXP_WUU_PIN(17, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 17 any edge interrupt */
#define NXP_WUU_PIN_17_ANY_INT         NXP_WUU_PIN(17, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 17 any edge DMA */
#define NXP_WUU_PIN_17_ANY_DMA         NXP_WUU_PIN(17, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 17 any edge trigger */
#define NXP_WUU_PIN_17_ANY_TRIGGER     NXP_WUU_PIN(17, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 18 wakeup sources */
/** Pin 18 rising edge interrupt */
#define NXP_WUU_PIN_18_RISING_INT      NXP_WUU_PIN(18, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 18 rising edge DMA */
#define NXP_WUU_PIN_18_RISING_DMA      NXP_WUU_PIN(18, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 18 rising edge trigger */
#define NXP_WUU_PIN_18_RISING_TRIGGER  NXP_WUU_PIN(18, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 18 falling edge interrupt */
#define NXP_WUU_PIN_18_FALLING_INT     NXP_WUU_PIN(18, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 18 falling edge DMA */
#define NXP_WUU_PIN_18_FALLING_DMA     NXP_WUU_PIN(18, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 18 falling edge trigger */
#define NXP_WUU_PIN_18_FALLING_TRIGGER NXP_WUU_PIN(18, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 18 any edge interrupt */
#define NXP_WUU_PIN_18_ANY_INT         NXP_WUU_PIN(18, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 18 any edge DMA */
#define NXP_WUU_PIN_18_ANY_DMA         NXP_WUU_PIN(18, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 18 any edge trigger */
#define NXP_WUU_PIN_18_ANY_TRIGGER     NXP_WUU_PIN(18, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 19 wakeup sources */
/** Pin 19 rising edge interrupt */
#define NXP_WUU_PIN_19_RISING_INT      NXP_WUU_PIN(19, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 19 rising edge DMA */
#define NXP_WUU_PIN_19_RISING_DMA      NXP_WUU_PIN(19, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 19 rising edge trigger */
#define NXP_WUU_PIN_19_RISING_TRIGGER  NXP_WUU_PIN(19, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 19 falling edge interrupt */
#define NXP_WUU_PIN_19_FALLING_INT     NXP_WUU_PIN(19, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 19 falling edge DMA */
#define NXP_WUU_PIN_19_FALLING_DMA     NXP_WUU_PIN(19, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 19 falling edge trigger */
#define NXP_WUU_PIN_19_FALLING_TRIGGER NXP_WUU_PIN(19, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 19 any edge interrupt */
#define NXP_WUU_PIN_19_ANY_INT         NXP_WUU_PIN(19, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 19 any edge DMA */
#define NXP_WUU_PIN_19_ANY_DMA         NXP_WUU_PIN(19, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 19 any edge trigger */
#define NXP_WUU_PIN_19_ANY_TRIGGER     NXP_WUU_PIN(19, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 20 wakeup sources */
/** Pin 20 rising edge interrupt */
#define NXP_WUU_PIN_20_RISING_INT      NXP_WUU_PIN(20, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 20 rising edge DMA */
#define NXP_WUU_PIN_20_RISING_DMA      NXP_WUU_PIN(20, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 20 rising edge trigger */
#define NXP_WUU_PIN_20_RISING_TRIGGER  NXP_WUU_PIN(20, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 20 falling edge interrupt */
#define NXP_WUU_PIN_20_FALLING_INT     NXP_WUU_PIN(20, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 20 falling edge DMA */
#define NXP_WUU_PIN_20_FALLING_DMA     NXP_WUU_PIN(20, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 20 falling edge trigger */
#define NXP_WUU_PIN_20_FALLING_TRIGGER NXP_WUU_PIN(20, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 20 any edge interrupt */
#define NXP_WUU_PIN_20_ANY_INT         NXP_WUU_PIN(20, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 20 any edge DMA */
#define NXP_WUU_PIN_20_ANY_DMA         NXP_WUU_PIN(20, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 20 any edge trigger */
#define NXP_WUU_PIN_20_ANY_TRIGGER     NXP_WUU_PIN(20, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 21 wakeup sources */
/** Pin 21 rising edge interrupt */
#define NXP_WUU_PIN_21_RISING_INT      NXP_WUU_PIN(21, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 21 rising edge DMA */
#define NXP_WUU_PIN_21_RISING_DMA      NXP_WUU_PIN(21, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 21 rising edge trigger */
#define NXP_WUU_PIN_21_RISING_TRIGGER  NXP_WUU_PIN(21, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 21 falling edge interrupt */
#define NXP_WUU_PIN_21_FALLING_INT     NXP_WUU_PIN(21, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 21 falling edge DMA */
#define NXP_WUU_PIN_21_FALLING_DMA     NXP_WUU_PIN(21, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 21 falling edge trigger */
#define NXP_WUU_PIN_21_FALLING_TRIGGER NXP_WUU_PIN(21, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 21 any edge interrupt */
#define NXP_WUU_PIN_21_ANY_INT         NXP_WUU_PIN(21, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 21 any edge DMA */
#define NXP_WUU_PIN_21_ANY_DMA         NXP_WUU_PIN(21, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 21 any edge trigger */
#define NXP_WUU_PIN_21_ANY_TRIGGER     NXP_WUU_PIN(21, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 22 wakeup sources */
/** Pin 22 rising edge interrupt */
#define NXP_WUU_PIN_22_RISING_INT      NXP_WUU_PIN(22, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 22 rising edge DMA */
#define NXP_WUU_PIN_22_RISING_DMA      NXP_WUU_PIN(22, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 22 rising edge trigger */
#define NXP_WUU_PIN_22_RISING_TRIGGER  NXP_WUU_PIN(22, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 22 falling edge interrupt */
#define NXP_WUU_PIN_22_FALLING_INT     NXP_WUU_PIN(22, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 22 falling edge DMA */
#define NXP_WUU_PIN_22_FALLING_DMA     NXP_WUU_PIN(22, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 22 falling edge trigger */
#define NXP_WUU_PIN_22_FALLING_TRIGGER NXP_WUU_PIN(22, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 22 any edge interrupt */
#define NXP_WUU_PIN_22_ANY_INT         NXP_WUU_PIN(22, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 22 any edge DMA */
#define NXP_WUU_PIN_22_ANY_DMA         NXP_WUU_PIN(22, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 22 any edge trigger */
#define NXP_WUU_PIN_22_ANY_TRIGGER     NXP_WUU_PIN(22, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 23 wakeup sources */
/** Pin 23 rising edge interrupt */
#define NXP_WUU_PIN_23_RISING_INT      NXP_WUU_PIN(23, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 23 rising edge DMA */
#define NXP_WUU_PIN_23_RISING_DMA      NXP_WUU_PIN(23, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 23 rising edge trigger */
#define NXP_WUU_PIN_23_RISING_TRIGGER  NXP_WUU_PIN(23, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 23 falling edge interrupt */
#define NXP_WUU_PIN_23_FALLING_INT     NXP_WUU_PIN(23, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 23 falling edge DMA */
#define NXP_WUU_PIN_23_FALLING_DMA     NXP_WUU_PIN(23, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 23 falling edge trigger */
#define NXP_WUU_PIN_23_FALLING_TRIGGER NXP_WUU_PIN(23, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 23 any edge interrupt */
#define NXP_WUU_PIN_23_ANY_INT         NXP_WUU_PIN(23, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 23 any edge DMA */
#define NXP_WUU_PIN_23_ANY_DMA         NXP_WUU_PIN(23, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 23 any edge trigger */
#define NXP_WUU_PIN_23_ANY_TRIGGER     NXP_WUU_PIN(23, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 24 wakeup sources */
/** Pin 24 rising edge interrupt */
#define NXP_WUU_PIN_24_RISING_INT      NXP_WUU_PIN(24, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 24 rising edge DMA */
#define NXP_WUU_PIN_24_RISING_DMA      NXP_WUU_PIN(24, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 24 rising edge trigger */
#define NXP_WUU_PIN_24_RISING_TRIGGER  NXP_WUU_PIN(24, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 24 falling edge interrupt */
#define NXP_WUU_PIN_24_FALLING_INT     NXP_WUU_PIN(24, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 24 falling edge DMA */
#define NXP_WUU_PIN_24_FALLING_DMA     NXP_WUU_PIN(24, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 24 falling edge trigger */
#define NXP_WUU_PIN_24_FALLING_TRIGGER NXP_WUU_PIN(24, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 24 any edge interrupt */
#define NXP_WUU_PIN_24_ANY_INT         NXP_WUU_PIN(24, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 24 any edge DMA */
#define NXP_WUU_PIN_24_ANY_DMA         NXP_WUU_PIN(24, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 24 any edge trigger */
#define NXP_WUU_PIN_24_ANY_TRIGGER     NXP_WUU_PIN(24, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 25 wakeup sources */
/** Pin 25 rising edge interrupt */
#define NXP_WUU_PIN_25_RISING_INT      NXP_WUU_PIN(25, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 25 rising edge DMA */
#define NXP_WUU_PIN_25_RISING_DMA      NXP_WUU_PIN(25, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 25 rising edge trigger */
#define NXP_WUU_PIN_25_RISING_TRIGGER  NXP_WUU_PIN(25, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 25 falling edge interrupt */
#define NXP_WUU_PIN_25_FALLING_INT     NXP_WUU_PIN(25, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 25 falling edge DMA */
#define NXP_WUU_PIN_25_FALLING_DMA     NXP_WUU_PIN(25, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 25 falling edge trigger */
#define NXP_WUU_PIN_25_FALLING_TRIGGER NXP_WUU_PIN(25, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 25 any edge interrupt */
#define NXP_WUU_PIN_25_ANY_INT         NXP_WUU_PIN(25, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 25 any edge DMA */
#define NXP_WUU_PIN_25_ANY_DMA         NXP_WUU_PIN(25, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 25 any edge trigger */
#define NXP_WUU_PIN_25_ANY_TRIGGER     NXP_WUU_PIN(25, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 26 wakeup sources */
/** Pin 26 rising edge interrupt */
#define NXP_WUU_PIN_26_RISING_INT      NXP_WUU_PIN(26, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 26 rising edge DMA */
#define NXP_WUU_PIN_26_RISING_DMA      NXP_WUU_PIN(26, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 26 rising edge trigger */
#define NXP_WUU_PIN_26_RISING_TRIGGER  NXP_WUU_PIN(26, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 26 falling edge interrupt */
#define NXP_WUU_PIN_26_FALLING_INT     NXP_WUU_PIN(26, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 26 falling edge DMA */
#define NXP_WUU_PIN_26_FALLING_DMA     NXP_WUU_PIN(26, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 26 falling edge trigger */
#define NXP_WUU_PIN_26_FALLING_TRIGGER NXP_WUU_PIN(26, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 26 any edge interrupt */
#define NXP_WUU_PIN_26_ANY_INT         NXP_WUU_PIN(26, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 26 any edge DMA */
#define NXP_WUU_PIN_26_ANY_DMA         NXP_WUU_PIN(26, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 26 any edge trigger */
#define NXP_WUU_PIN_26_ANY_TRIGGER     NXP_WUU_PIN(26, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 27 wakeup sources */
/** Pin 27 rising edge interrupt */
#define NXP_WUU_PIN_27_RISING_INT      NXP_WUU_PIN(27, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 27 rising edge DMA */
#define NXP_WUU_PIN_27_RISING_DMA      NXP_WUU_PIN(27, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 27 rising edge trigger */
#define NXP_WUU_PIN_27_RISING_TRIGGER  NXP_WUU_PIN(27, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 27 falling edge interrupt */
#define NXP_WUU_PIN_27_FALLING_INT     NXP_WUU_PIN(27, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 27 falling edge DMA */
#define NXP_WUU_PIN_27_FALLING_DMA     NXP_WUU_PIN(27, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 27 falling edge trigger */
#define NXP_WUU_PIN_27_FALLING_TRIGGER NXP_WUU_PIN(27, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 27 any edge interrupt */
#define NXP_WUU_PIN_27_ANY_INT         NXP_WUU_PIN(27, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 27 any edge DMA */
#define NXP_WUU_PIN_27_ANY_DMA         NXP_WUU_PIN(27, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 27 any edge trigger */
#define NXP_WUU_PIN_27_ANY_TRIGGER     NXP_WUU_PIN(27, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 28 wakeup sources */
/** Pin 28 rising edge interrupt */
#define NXP_WUU_PIN_28_RISING_INT      NXP_WUU_PIN(28, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 28 rising edge DMA */
#define NXP_WUU_PIN_28_RISING_DMA      NXP_WUU_PIN(28, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 28 rising edge trigger */
#define NXP_WUU_PIN_28_RISING_TRIGGER  NXP_WUU_PIN(28, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 28 falling edge interrupt */
#define NXP_WUU_PIN_28_FALLING_INT     NXP_WUU_PIN(28, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 28 falling edge DMA */
#define NXP_WUU_PIN_28_FALLING_DMA     NXP_WUU_PIN(28, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 28 falling edge trigger */
#define NXP_WUU_PIN_28_FALLING_TRIGGER NXP_WUU_PIN(28, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 28 any edge interrupt */
#define NXP_WUU_PIN_28_ANY_INT         NXP_WUU_PIN(28, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 28 any edge DMA */
#define NXP_WUU_PIN_28_ANY_DMA         NXP_WUU_PIN(28, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 28 any edge trigger */
#define NXP_WUU_PIN_28_ANY_TRIGGER     NXP_WUU_PIN(28, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 29 wakeup sources */
/** Pin 29 rising edge interrupt */
#define NXP_WUU_PIN_29_RISING_INT      NXP_WUU_PIN(29, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 29 rising edge DMA */
#define NXP_WUU_PIN_29_RISING_DMA      NXP_WUU_PIN(29, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 29 rising edge trigger */
#define NXP_WUU_PIN_29_RISING_TRIGGER  NXP_WUU_PIN(29, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 29 falling edge interrupt */
#define NXP_WUU_PIN_29_FALLING_INT     NXP_WUU_PIN(29, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 29 falling edge DMA */
#define NXP_WUU_PIN_29_FALLING_DMA     NXP_WUU_PIN(29, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 29 falling edge trigger */
#define NXP_WUU_PIN_29_FALLING_TRIGGER NXP_WUU_PIN(29, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 29 any edge interrupt */
#define NXP_WUU_PIN_29_ANY_INT         NXP_WUU_PIN(29, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 29 any edge DMA */
#define NXP_WUU_PIN_29_ANY_DMA         NXP_WUU_PIN(29, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 29 any edge trigger */
#define NXP_WUU_PIN_29_ANY_TRIGGER     NXP_WUU_PIN(29, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 30 wakeup sources */
/** Pin 30 rising edge interrupt */
#define NXP_WUU_PIN_30_RISING_INT      NXP_WUU_PIN(30, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 30 rising edge DMA */
#define NXP_WUU_PIN_30_RISING_DMA      NXP_WUU_PIN(30, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 30 rising edge trigger */
#define NXP_WUU_PIN_30_RISING_TRIGGER  NXP_WUU_PIN(30, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 30 falling edge interrupt */
#define NXP_WUU_PIN_30_FALLING_INT     NXP_WUU_PIN(30, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 30 falling edge DMA */
#define NXP_WUU_PIN_30_FALLING_DMA     NXP_WUU_PIN(30, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 30 falling edge trigger */
#define NXP_WUU_PIN_30_FALLING_TRIGGER NXP_WUU_PIN(30, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 30 any edge interrupt */
#define NXP_WUU_PIN_30_ANY_INT         NXP_WUU_PIN(30, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 30 any edge DMA */
#define NXP_WUU_PIN_30_ANY_DMA         NXP_WUU_PIN(30, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 30 any edge trigger */
#define NXP_WUU_PIN_30_ANY_TRIGGER     NXP_WUU_PIN(30, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

/* Pin 31 wakeup sources */
/** Pin 31 rising edge interrupt */
#define NXP_WUU_PIN_31_RISING_INT      NXP_WUU_PIN(31, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_INT)
/** Pin 31 rising edge DMA */
#define NXP_WUU_PIN_31_RISING_DMA      NXP_WUU_PIN(31, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_DMA)
/** Pin 31 rising edge trigger */
#define NXP_WUU_PIN_31_RISING_TRIGGER  NXP_WUU_PIN(31, NXP_WUU_EDGE_RISING, NXP_WUU_EVENT_TRIGGER)
/** Pin 31 falling edge interrupt */
#define NXP_WUU_PIN_31_FALLING_INT     NXP_WUU_PIN(31, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_INT)
/** Pin 31 falling edge DMA */
#define NXP_WUU_PIN_31_FALLING_DMA     NXP_WUU_PIN(31, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_DMA)
/** Pin 31 falling edge trigger */
#define NXP_WUU_PIN_31_FALLING_TRIGGER NXP_WUU_PIN(31, NXP_WUU_EDGE_FALLING, NXP_WUU_EVENT_TRIGGER)
/** Pin 31 any edge interrupt */
#define NXP_WUU_PIN_31_ANY_INT         NXP_WUU_PIN(31, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_INT)
/** Pin 31 any edge DMA */
#define NXP_WUU_PIN_31_ANY_DMA         NXP_WUU_PIN(31, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_DMA)
/** Pin 31 any edge trigger */
#define NXP_WUU_PIN_31_ANY_TRIGGER     NXP_WUU_PIN(31, NXP_WUU_EDGE_ANY, NXP_WUU_EVENT_TRIGGER)

#endif /* ZEPHYR_INCLUDE_WUC_NXP_WUU_H_ */
