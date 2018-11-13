/*
 * Copyright (c) 2018, Cypress
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#if defined(CONFIG_SOC_PSOC6_M0)
#define DT_NUM_IRQ_PRIO_BITS		DT_ARM_V6M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS
#else
#define DT_NUM_IRQ_PRIO_BITS		DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS
#endif

#define DT_UART_PSOC6_UART_5_NAME         "uart_5"
#define DT_UART_PSOC6_UART_5_BASE_ADDRESS SCB5
#define DT_UART_PSOC6_UART_5_PORT         P5_0_PORT
#define DT_UART_PSOC6_UART_5_RX_NUM       P5_0_NUM
#define DT_UART_PSOC6_UART_5_TX_NUM       P5_1_NUM
#define DT_UART_PSOC6_UART_5_RX_VAL       P5_0_SCB5_UART_RX
#define DT_UART_PSOC6_UART_5_TX_VAL       P5_1_SCB5_UART_TX
#define DT_UART_PSOC6_UART_5_CLOCK        PCLK_SCB5_CLOCK

#define DT_UART_PSOC6_UART_6_NAME         "uart_6"
#define DT_UART_PSOC6_UART_6_BASE_ADDRESS SCB6
#define DT_UART_PSOC6_UART_6_PORT         P12_0_PORT
#define DT_UART_PSOC6_UART_6_RX_NUM       P12_0_NUM
#define DT_UART_PSOC6_UART_6_TX_NUM       P12_1_NUM
#define DT_UART_PSOC6_UART_6_RX_VAL       P12_0_SCB6_UART_RX
#define DT_UART_PSOC6_UART_6_TX_VAL       P12_1_SCB6_UART_TX
#define DT_UART_PSOC6_UART_6_CLOCK        PCLK_SCB6_CLOCK

/* UART desired baud rate is 115200 bps (Standard mode).
* The UART baud rate = (SCB clock frequency / Oversample).
* For PeriClk = 50 MHz, select divider value 36 and get SCB clock = (50 MHz / 36) = 1,389 MHz.
* Select Oversample = 12. These setting results UART data rate = 1,389 MHz / 12 = 115750 bps.
*/
#define DT_UART_PSOC6_CONFIG_OVERSAMPLE      (12UL)
#define DT_UART_PSOC6_CONFIG_BREAKWIDTH      (11UL)
#define DT_UART_PSOC6_CONFIG_DATAWIDTH       (8UL)

/* Assign divider type and number for UART */
#define DT_UART_PSOC6_UART_CLK_DIV_TYPE     (CY_SYSCLK_DIV_8_BIT)
#define DT_UART_PSOC6_UART_CLK_DIV_NUMBER   (PERI_DIV_8_NR - 1u)
#define DT_UART_PSOC6_UART_CLK_DIV_VAL      (35UL)


/* End of SoC Level DTS fixup file */
