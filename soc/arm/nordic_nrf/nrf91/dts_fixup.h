/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_ADC_0_NAME			DT_LABEL(DT_NODELABEL(adc))

#define DT_UART_0_NAME			DT_LABEL(DT_NODELABEL(uart0))
#define DT_UART_1_NAME			DT_LABEL(DT_NODELABEL(uart1))
#define DT_UART_2_NAME			DT_LABEL(DT_NODELABEL(uart2))
#define DT_UART_3_NAME			DT_LABEL(DT_NODELABEL(uart3))

#define DT_RTC_0_NAME			DT_LABEL(DT_NODELABEL(rtc0))
#define DT_RTC_1_NAME			DT_LABEL(DT_NODELABEL(rtc1))

/* End of SoC Level DTS fixup file */
