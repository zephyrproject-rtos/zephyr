/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#include <zephyr/autoconf.h>

/* ARRAY_SIZE causes a conflict as it is defined both by TF-M and indirectly by devicetree.h */
#undef ARRAY_SIZE
#include <zephyr/devicetree.h>

#define UART_PIN_INIT(node_id, prop, idx) \
	DT_PROP_BY_IDX(node_id, prop, idx),

/* Configuration settings for Driver_USART0. */
#if DOMAIN_NS == 1U

#define RTE_USART0 1

#define RTE_USART0_PINS \
{ \
	DT_FOREACH_CHILD_VARGS( \
		DT_PINCTRL_BY_NAME(DT_NODELABEL(uart0), default, 0), \
		DT_FOREACH_PROP_ELEM, psels, UART_PIN_INIT \
	) \
}

#endif

/* Configuration settings for Driver_USART1. */
#if DT_PINCTRL_HAS_NAME(DT_NODELABEL(uart1), default) && DOMAIN_NS != 1U

#define RTE_USART1 1

#define RTE_USART1_PINS \
{ \
	DT_FOREACH_CHILD_VARGS( \
		DT_PINCTRL_BY_NAME(DT_NODELABEL(uart1), default, 0), \
		DT_FOREACH_PROP_ELEM, psels, UART_PIN_INIT \
	) \
}

#endif

/* Configuration settings for Driver_FLASH0. */
#define RTE_FLASH0 1

#endif  /* __RTE_DEVICE_H */
