/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_CTLR_DEBUG_PINS
#if defined(CONFIG_BOARD_NRF52840_PCA10056)
#define DEBUG_PORT       NRF_P1
#define DEBUG_PIN0       BIT(1)
#define DEBUG_PIN1       BIT(2)
#define DEBUG_PIN2       BIT(3)
#define DEBUG_PIN3       BIT(4)
#define DEBUG_PIN4       BIT(5)
#define DEBUG_PIN5       BIT(6)
#define DEBUG_PIN6       BIT(7)
#define DEBUG_PIN7       BIT(8)
#define DEBUG_PIN8       BIT(10)
#define DEBUG_PIN9       BIT(11)
#elif defined(CONFIG_BOARD_NRF52_PCA10040) || \
      defined(CONFIG_BOARD_NRF52810_PCA10040)
#define DEBUG_PORT       NRF_GPIO
#define DEBUG_PIN0       BIT(11)
#define DEBUG_PIN1       BIT(12)
#define DEBUG_PIN2       BIT(13)
#define DEBUG_PIN3       BIT(14)
#define DEBUG_PIN4       BIT(15)
#define DEBUG_PIN5       BIT(16)
#define DEBUG_PIN6       BIT(17)
#define DEBUG_PIN7       BIT(18)
#define DEBUG_PIN8       BIT(19)
#define DEBUG_PIN9       BIT(20)
#elif defined(CONFIG_BOARD_NRF51_PCA10028)
#define DEBUG_PORT       NRF_GPIO
#define DEBUG_PIN0       BIT(12)
#define DEBUG_PIN1       BIT(13)
#define DEBUG_PIN2       BIT(14)
#define DEBUG_PIN3       BIT(15)
#define DEBUG_PIN4       BIT(16)
#define DEBUG_PIN5       BIT(17)
#define DEBUG_PIN6       BIT(18)
#define DEBUG_PIN7       BIT(19)
#define DEBUG_PIN8       BIT(20)
#define DEBUG_PIN9       BIT(23)
#else
#error BT_CTLR_DEBUG_PINS not supported on this board.
#endif

#define DEBUG_PIN_MASK   (DEBUG_PIN0 | DEBUG_PIN1 | DEBUG_PIN2 | DEBUG_PIN3 | \
			  DEBUG_PIN4 | DEBUG_PIN5 | DEBUG_PIN6 | DEBUG_PIN7 | \
			  DEBUG_PIN8 | DEBUG_PIN9)
#define DEBUG_CLOSE_MASK (DEBUG_PIN3 | DEBUG_PIN4 | DEBUG_PIN5 | DEBUG_PIN6)

/* below are some interesting macros referenced by controller
 * which can be defined to SoC's GPIO toggle to observe/debug the
 * controller's runtime behavior.
 */
#define DEBUG_INIT()            do { \
				DEBUG_PORT->DIRSET = DEBUG_PIN_MASK; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN_MASK; } \
				while (0)

#define DEBUG_CPU_SLEEP(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTSET = DEBUG_PIN0; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN0; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN0; \
				DEBUG_PORT->OUTSET = DEBUG_PIN0; } \
				} while (0)

#define DEBUG_TICKER_ISR(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN1; \
				DEBUG_PORT->OUTSET = DEBUG_PIN1; } \
				else { \
				DEBUG_PORT->OUTSET = DEBUG_PIN1; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN1; } \
				} while (0)

#define DEBUG_TICKER_TASK(flag)  do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN1; \
				DEBUG_PORT->OUTSET = DEBUG_PIN1; } \
				else { \
				DEBUG_PORT->OUTSET = DEBUG_PIN1; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN1; } \
				} while (0)

#define DEBUG_TICKER_JOB(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN2; \
				DEBUG_PORT->OUTSET = DEBUG_PIN2; } \
				else { \
				DEBUG_PORT->OUTSET = DEBUG_PIN2; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN2; } \
				} while (0)

#define DEBUG_RADIO_ISR(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN7; \
				DEBUG_PORT->OUTSET = DEBUG_PIN7; } \
				else { \
				DEBUG_PORT->OUTSET = DEBUG_PIN7; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN7; } \
				} while (0)

#define DEBUG_RADIO_XTAL(flag)  do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN8; \
				DEBUG_PORT->OUTSET = DEBUG_PIN8; } \
				else { \
				DEBUG_PORT->OUTSET = DEBUG_PIN8; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN8; } \
				} while (0)

#define DEBUG_RADIO_ACTIVE(flag)    do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN9; \
				DEBUG_PORT->OUTSET = DEBUG_PIN9; } \
				else { \
				DEBUG_PORT->OUTSET = DEBUG_PIN9; \
				DEBUG_PORT->OUTCLR = DEBUG_PIN9; } \
				} while (0)

#define DEBUG_RADIO_CLOSE(flag)     do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = 0x00000000; \
				DEBUG_PORT->OUTSET = 0x00000000; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_CLOSE_MASK; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_A(flag) do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN3; \
				DEBUG_PORT->OUTSET = DEBUG_PIN3; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN3; \
				DEBUG_PORT->OUTSET = DEBUG_PIN3; } \
				} while (0)

#define DEBUG_RADIO_START_A(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN3; \
				DEBUG_PORT->OUTSET = DEBUG_PIN3; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN3; \
				DEBUG_PORT->OUTSET = DEBUG_PIN3; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_S(flag) do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN4; \
				DEBUG_PORT->OUTSET = DEBUG_PIN4; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN4; \
				DEBUG_PORT->OUTSET = DEBUG_PIN4; } \
				} while (0)

#define DEBUG_RADIO_START_S(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN4; \
				DEBUG_PORT->OUTSET = DEBUG_PIN4; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN4; \
				DEBUG_PORT->OUTSET = DEBUG_PIN4; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_O(flag) do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN5; \
				DEBUG_PORT->OUTSET = DEBUG_PIN5; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN5; \
				DEBUG_PORT->OUTSET = DEBUG_PIN5; } \
				} while (0)

#define DEBUG_RADIO_START_O(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN5; \
				DEBUG_PORT->OUTSET = DEBUG_PIN5; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN5; \
				DEBUG_PORT->OUTSET = DEBUG_PIN5; } \
				} while (0)

#define DEBUG_RADIO_PREPARE_M(flag) do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN6; \
				DEBUG_PORT->OUTSET = DEBUG_PIN6; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN6; \
				DEBUG_PORT->OUTSET = DEBUG_PIN6; } \
				} while (0)

#define DEBUG_RADIO_START_M(flag)   do { \
				if (flag) { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN6; \
				DEBUG_PORT->OUTSET = DEBUG_PIN6; } \
				else { \
				DEBUG_PORT->OUTCLR = DEBUG_PIN6; \
				DEBUG_PORT->OUTSET = DEBUG_PIN6; } \
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

#endif /* CONFIG_BT_CTLR_DEBUG_PINS */
