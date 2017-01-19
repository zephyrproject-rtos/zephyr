/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef CONFIG_BLUETOOTH_CONTROLLER_ASSERT_HANDLER
void bt_controller_assert_handle(char *file, uint32_t line);
#define LL_ASSERT(cond) if (!(cond)) { \
				bt_controller_assert_handle(__FILE__, \
							    __LINE__); \
			}
#else
#define LL_ASSERT(cond) BT_ASSERT(cond)
#endif

/* below are some interesting macros referenced by controller
 * which can be defined to SoC's GPIO toggle to observe/debug the
 * controller's runtime behavior.
 */
#if (DEBUG == 1)
#define DEBUG_INIT()            do { \
				NRF_GPIO->DIRSET = 0x03FF0000; \
				NRF_GPIO->OUTCLR = 0x03FF0000; } \
				while (0)

#define DEBUG_CPU_SLEEP(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTSET = 0x00010000; \
				NRF_GPIO->OUTCLR = 0x00010000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00010000; \
				NRF_GPIO->OUTSET = 0x00010000; } \
				} while (0)

#define DEBUG_TICKER_ISR(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00020000; \
				NRF_GPIO->OUTSET = 0x00020000; } \
				else { \
				NRF_GPIO->OUTSET = 0x00020000; \
				NRF_GPIO->OUTCLR = 0x00020000; } \
				} while (0)

#define DEBUG_TICKER_TASK(flag)  do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00020000; \
				NRF_GPIO->OUTSET = 0x00020000; } \
				else { \
				NRF_GPIO->OUTSET = 0x00020000; \
				NRF_GPIO->OUTCLR = 0x00020000; } \
				} while (0)

#define DEBUG_TICKER_JOB(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00040000; \
				NRF_GPIO->OUTSET = 0x00040000; } \
				else { \
				NRF_GPIO->OUTSET = 0x00040000; \
				NRF_GPIO->OUTCLR = 0x00040000; } \
				} while (0)

#define DEBUG_RADIO_ISR(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00800000; \
				NRF_GPIO->OUTSET = 0x00800000; } \
				else { \
				NRF_GPIO->OUTSET = 0x00800000; \
				NRF_GPIO->OUTCLR = 0x00800000; } \
				} while (0)

#define DEBUG_RADIO_XTAL(flag)  do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x01000000; \
				NRF_GPIO->OUTSET = 0x01000000; } \
				else { \
				NRF_GPIO->OUTSET = 0x01000000; \
				NRF_GPIO->OUTCLR = 0x01000000; } \
				} while (0)

#define DEBUG_RADIO_ACTIVE(flag)    do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x02000000; \
				NRF_GPIO->OUTSET = 0x02000000; } \
				else { \
				NRF_GPIO->OUTSET = 0x02000000; \
				NRF_GPIO->OUTCLR = 0x02000000; } \
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
				NRF_GPIO->OUTCLR = 0x00080000; \
				NRF_GPIO->OUTSET = 0x00080000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00080000; \
				NRF_GPIO->OUTSET = 0x00080000; } \
				} while (0)

#define DEBUG_RADIO_START_A(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00080000; \
				NRF_GPIO->OUTSET = 0x00080000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00080000; \
				NRF_GPIO->OUTSET = 0x00080000; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_S(flag) do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00100000; \
				NRF_GPIO->OUTSET = 0x00100000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00100000; \
				NRF_GPIO->OUTSET = 0x00100000; } \
				} while (0)

#define DEBUG_RADIO_START_S(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00100000; \
				NRF_GPIO->OUTSET = 0x00100000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00100000; \
				NRF_GPIO->OUTSET = 0x00100000; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_O(flag) do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00200000; \
				NRF_GPIO->OUTSET = 0x00200000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00200000; \
				NRF_GPIO->OUTSET = 0x00200000; } \
				} while (0)

#define DEBUG_RADIO_START_O(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00200000; \
				NRF_GPIO->OUTSET = 0x00200000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00200000; \
				NRF_GPIO->OUTSET = 0x00200000; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_M(flag) do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00400000; \
				NRF_GPIO->OUTSET = 0x00400000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00400000; \
				NRF_GPIO->OUTSET = 0x00400000; } \
				} while (0)

#define DEBUG_RADIO_START_M(flag)   do { \
				if (flag) { \
				NRF_GPIO->OUTCLR = 0x00400000; \
				NRF_GPIO->OUTSET = 0x00400000; } \
				else { \
				NRF_GPIO->OUTCLR = 0x00400000; \
				NRF_GPIO->OUTSET = 0x00400000; } \
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

#endif /* DEBUG */

#endif /* _DEBUG_H_ */
