/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

/* below are some interesting macros referenced by controller
 * which can be defined to SoC's GPIO toggle to observe/debug the
 * controller's runtime behavior.
 */
#ifdef CONFIG_BLUETOOTH_CONTROLLER_DEBUG_PINS
#define DEBUG_INIT()            do { \
				NRF_GPIO->DIRSET = 0x03FF0000; \
				NRF_GPIO->OUTCLR = 0x03FF0000; } \
				while (0)

#define DEBUG_CPU_SLEEP(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTSET = BIT(16); \
				NRF_GPIO->OUTCLR = BIT(16); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(16); \
				NRF_GPIO->OUTSET = BIT(16); } \
				} while (0)

#define DEBUG_TICKER_ISR(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(17); \
				NRF_GPIO->OUTSET = BIT(17); } \
				else { \
				NRF_GPIO->OUTSET = BIT(17); \
				NRF_GPIO->OUTCLR = BIT(17); } \
				} while (0)

#define DEBUG_TICKER_TASK(flag)  do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(17); \
				NRF_GPIO->OUTSET = BIT(17); } \
				else { \
				NRF_GPIO->OUTSET = BIT(17); \
				NRF_GPIO->OUTCLR = BIT(17); } \
				} while (0)

#define DEBUG_TICKER_JOB(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(18); \
				NRF_GPIO->OUTSET = BIT(18); } \
				else { \
				NRF_GPIO->OUTSET = BIT(18); \
				NRF_GPIO->OUTCLR = BIT(18); } \
				} while (0)

#define DEBUG_RADIO_ISR(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(23); \
				NRF_GPIO->OUTSET = BIT(23); } \
				else { \
				NRF_GPIO->OUTSET = BIT(23); \
				NRF_GPIO->OUTCLR = BIT(23); } \
				} while (0)

#define DEBUG_RADIO_XTAL(flag)  do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(24); \
				NRF_GPIO->OUTSET = BIT(24); } \
				else { \
				NRF_GPIO->OUTSET = BIT(24); \
				NRF_GPIO->OUTCLR = BIT(24); } \
				} while (0)

#define DEBUG_RADIO_ACTIVE(flag)    do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(25); \
				NRF_GPIO->OUTSET = BIT(25); } \
				else { \
				NRF_GPIO->OUTSET = BIT(25); \
				NRF_GPIO->OUTCLR = BIT(25); } \
				} while (0)

#define DEBUG_RADIO_CLOSE(flag)     do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00000000; \
				NRF_GPIO->OUTSET = 0x00000000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00780000; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_A(flag) do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(19); \
				NRF_GPIO->OUTSET = BIT(19); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(19); \
				NRF_GPIO->OUTSET = BIT(19); } \
				} while (0)

#define DEBUG_RADIO_START_A(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(19); \
				NRF_GPIO->OUTSET = BIT(19); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(19); \
				NRF_GPIO->OUTSET = BIT(19); } \
				} while (0)

#define DEBUG_RADIO_PREPARE_S(flag) do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(20); \
				NRF_GPIO->OUTSET = BIT(20); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(20); \
				NRF_GPIO->OUTSET = BIT(20); } \
				} while (0)

#define DEBUG_RADIO_START_S(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(20); \
				NRF_GPIO->OUTSET = BIT(20); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(20); \
				NRF_GPIO->OUTSET = BIT(20); } \
				} while (0)

#define DEBUG_RADIO_PREPARE_O(flag) do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(21); \
				NRF_GPIO->OUTSET = BIT(21); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(21); \
				NRF_GPIO->OUTSET = BIT(21); } \
				} while (0)

#define DEBUG_RADIO_START_O(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(21); \
				NRF_GPIO->OUTSET = BIT(21); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(21); \
				NRF_GPIO->OUTSET = BIT(21); } \
				} while (0)

#define DEBUG_RADIO_PREPARE_M(flag) do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(22); \
				NRF_GPIO->OUTSET = BIT(22); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(22); \
				NRF_GPIO->OUTSET = BIT(22); } \
				} while (0)

#define DEBUG_RADIO_START_M(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = BIT(22); \
				NRF_GPIO->OUTSET = BIT(22); } \
				else { \
				NRF_GPIO->OUTCLR = BIT(22); \
				NRF_GPIO->OUTSET = BIT(22); } \
				} while (0)

#else

#define DEBUG_INIT()

#define DEBUG_CPU_SLEEP(flag)

#define DEBUG_TICKER_ISR(flag)

#define DEBUG_TICKER_TASK(flag)

#define DEBUG_TICKER_JOB(flag)

#define DEBUG_RADIO_ISR(flag)

#define DEBUG_RADIO_HCTO(flag)

#define DEBUG_RADIO_XTAL(flag)

#define DEBUG_RADIO_ACTIVE(flag)

#define DEBUG_RADIO_CLOSE(flag)

#define DEBUG_RADIO_PREPARE_A(flag)

#define DEBUG_RADIO_START_A(flag)

#define DEBUG_RADIO_PREPARE_S(flag)

#define DEBUG_RADIO_START_S(flag)

#define DEBUG_RADIO_PREPARE_O(flag)

#define DEBUG_RADIO_START_O(flag)

#define DEBUG_RADIO_PREPARE_M(flag)

#define DEBUG_RADIO_START_M(flag)

#endif /* CONFIG_BLUETOOTH_CONTROLLER_DEBUG_PINS */

#endif /* _DEBUG_H_ */
