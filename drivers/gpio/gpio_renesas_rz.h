/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_RENESAS_RZ_H_
#define ZEPHYR_DRIVERS_GPIO_RENESAS_RZ_H_

#include "r_ioport.h"

#define GPIO_RZ_INT_UNSUPPORTED 0xF

#if defined(CONFIG_SOC_SERIES_RZG3S) || defined(CONFIG_SOC_SERIES_RZA3UL) ||                       \
	defined(CONFIG_SOC_SERIES_RZV2L) || defined(CONFIG_SOC_SERIES_RZG2L)
#include <zephyr/dt-bindings/gpio/renesas-rz-gpio.h>

#if defined(CONFIG_SOC_SERIES_RZG3S)
#define GPIO_RZ_IOPORT_P_REG_BASE_GET   (&R_GPIO->P_20)
#define GPIO_RZ_IOPORT_PM_REG_BASE_GET  (&R_GPIO->PM_20)
#define GPIO_RZ_IOPORT_PFC_REG_BASE_GET (&R_GPIO->PFC_20)
#define GPIO_RZ_MAX_PORT_NUM            19
#define GPIO_RZ_TINT_IRQ_OFFSET         429
#define R_INTC                          R_INTC_IM33
static const uint8_t gpio_rz_int[GPIO_RZ_MAX_PORT_NUM] = {0,  4,  9,  13, 17, 23, 28, 33, 38, 43,
							  47, 52, 56, 58, 63, 66, 70, 72, 76};

#elif defined(CONFIG_SOC_SERIES_RZA3UL)
#define GPIO_RZ_IOPORT_P_REG_BASE_GET   (&R_GPIO->P10)
#define GPIO_RZ_IOPORT_PM_REG_BASE_GET  (&R_GPIO->PM10)
#define GPIO_RZ_IOPORT_PFC_REG_BASE_GET (&R_GPIO->PFC10)
#define GPIO_RZ_MAX_PORT_NUM            19
#define GPIO_RZ_TINT_IRQ_OFFSET         476
#define R_INTC                          R_INTC_IA55
static const uint8_t gpio_rz_int[GPIO_RZ_MAX_PORT_NUM] = {0,  4,  9,  13, 17, 23, 28, 33, 38, 43,
							  47, 52, 56, 58, 63, 66, 70, 72, 76};

#elif defined(CONFIG_SOC_SERIES_RZV2L) || defined(CONFIG_SOC_SERIES_RZG2L)
#define GPIO_RZ_IOPORT_P_REG_BASE_GET   (&R_GPIO->P10)
#define GPIO_RZ_IOPORT_PM_REG_BASE_GET  (&R_GPIO->PM10)
#define GPIO_RZ_IOPORT_PFC_REG_BASE_GET (&R_GPIO->PFC10)
#define GPIO_RZ_MAX_PORT_NUM            49
#define GPIO_RZ_TINT_IRQ_OFFSET         444
#define R_INTC                          R_INTC_IM33
static const uint8_t gpio_rz_int[GPIO_RZ_MAX_PORT_NUM] = {
	0,  2,  4,  6,  8,  10, 13, 15, 18, 21, 24,  25,  27,  29,  32, 34, 36,
	38, 41, 43, 45, 48, 50, 52, 54, 56, 58, 60,  62,  64,  66,  68, 70, 72,
	74, 76, 78, 80, 83, 85, 88, 91, 93, 98, 102, 106, 110, 114, 118};
#endif

#define GPIO_RZ_IOPORT_P_REG_GET(port, pin)   (&GPIO_RZ_IOPORT_P_REG_BASE_GET[port])
#define GPIO_RZ_IOPORT_PM_REG_GET(port, pin)  (&GPIO_RZ_IOPORT_PM_REG_BASE_GET[port])
#define GPIO_RZ_IOPORT_PFC_REG_GET(port, pin) (&GPIO_RZ_IOPORT_PFC_REG_BASE_GET[port])

#define GPIO_RZ_P_VALUE_GET(value, pin)   ((value >> pin) & 1U)
#define GPIO_RZ_PM_VALUE_GET(value, pin)  ((value >> (pin * 2)) & 3U)
#define GPIO_RZ_PFC_VALUE_GET(value, pin) ((value >> (pin * 4)) & 0xF)

#define GPIO_RZ_IOPORT_PFC_SET(value) (value << 24)

#define GPIO_RZ_PIN_DISCONNECT(port, pin) /* do nothing */

#define GPIO_RZ_MAX_INT_NUM 32

#define GPIO_RZ_TINT_IRQ_GET(tint_num) (tint_num + GPIO_RZ_TINT_IRQ_OFFSET)

#define GPIO_RZ_INT_EDGE_RISING  0x0
#define GPIO_RZ_INT_EDGE_FALLING 0x1
#define GPIO_RZ_INT_LEVEL_HIGH   0x2
#define GPIO_RZ_INT_LEVEL_LOW    0x3
#define GPIO_RZ_INT_BOTH_EDGE    GPIO_RZ_INT_UNSUPPORTED

#define GPIO_RZ_TSSR_VAL(port, pin) (0x80 | (gpio_rz_int[port] + pin))
#define GPIO_RZ_TSSR_OFFSET(irq)    ((irq % 4) * 8)
#define GPIO_RZ_TITSR_OFFSET(irq)   ((irq % 16) * 2)

#define GPIO_RZ_PIN_CONFIGURE_GET_FILTER(flag) (((flags >> RZ_GPIO_FILTER_SHIFT) & 0x1F) << 19U)
#define GPIO_RZ_PIN_CONFIGURE_GET(flag)        (((flag >> RZ_GPIO_IOLH_SHIFT) & 0x3) << 10U)

#define GPIO_RZ_PIN_CONFIGURE_INT_ENABLE         IOPORT_CFG_TINT_ENABLE
#define GPIO_RZ_PIN_CONFIGURE_INT_DISABLE        (~(IOPORT_CFG_TINT_ENABLE))
#define GPIO_RZ_PIN_CONFIGURE_INPUT_OUTPUT_RESET (~(0x3 << 2))
#define GPIO_RZ_PIN_SPECIAL_FLAG_GET(flag)       GPIO_RZ_PIN_CONFIGURE_GET_FILTER(flag)

#elif defined(CONFIG_SOC_SERIES_RZN2L) || defined(CONFIG_SOC_SERIES_RZT2L) ||                      \
	defined(CONFIG_SOC_SERIES_RZT2M)
#include <zephyr/dt-bindings/gpio/renesas-rztn-gpio.h>
#define GPIO_RZ_IOPORT_REG_REGION_GET(p) (R_BSP_IoRegionGet(p) == BSP_IO_REGION_NOT_SAFE ? 1 : 0)

#define GPIO_RZ_IOPORT_P_REG_BASE_GET(port, pin)                                                   \
	(GPIO_RZ_IOPORT_REG_REGION_GET((port << 8U) | pin) == 1 ? &R_PORT_NSR->P[port]             \
								: &R_PORT_SR->P[port])

#define GPIO_RZ_IOPORT_PM_REG_BASE_GET(port, pin)                                                  \
	(GPIO_RZ_IOPORT_REG_REGION_GET((port << 8U) | pin) == 1 ? &R_PORT_NSR->PM[port]            \
								: &R_PORT_SR->PM[port])

#define GPIO_RZ_IOPORT_PFC_REG_BASE_GET(port, pin)                                                 \
	(GPIO_RZ_IOPORT_REG_REGION_GET((port << 8U) | pin) == 1 ? &R_PORT_NSR->PFC[port]           \
								: &R_PORT_SR->PFC[port])

#define GPIO_RZ_IOPORT_P_REG_GET(port, pin)   (GPIO_RZ_IOPORT_P_REG_BASE_GET(port, pin))
#define GPIO_RZ_IOPORT_PM_REG_GET(port, pin)  (GPIO_RZ_IOPORT_PM_REG_BASE_GET(port, pin))
#define GPIO_RZ_IOPORT_PFC_REG_GET(port, pin) (GPIO_RZ_IOPORT_PFC_REG_BASE_GET(port, pin))

#define GPIO_RZ_P_VALUE_GET(value, pin)   ((value >> pin) & 1U)
#define GPIO_RZ_PM_VALUE_GET(value, pin)  ((value >> (pin * 2)) & 3U)
#define GPIO_RZ_PFC_VALUE_GET(value, pin) ((value >> (pin * 4)) & 0xF)

#define GPIO_RZ_IOPORT_PFC_SET(value) (value << 4)

#define GPIO_RZ_PIN_DISCONNECT(port, pin)                                                          \
	*GPIO_RZ_IOPORT_PM_REG_GET((port >> 8U), pin) &= ~(3U << (pin * 2))

#define GPIO_RZ_MAX_INT_NUM 16

#define GPIO_RZ_INT_EDGE_FALLING 0x0
#define GPIO_RZ_INT_EDGE_RISING  0x1
#define GPIO_RZ_INT_BOTH_EDGE    0x2
#define GPIO_RZ_INT_LEVEL_LOW    0x3
#define GPIO_RZ_INT_LEVEL_HIGH   GPIO_RZ_INT_UNSUPPORTED

#define GPIO_RZ_PIN_CONFIGURE_GET(flag) (((flag >> RZTN_GPIO_DRCTL_SHIFT) & 0x33) << 8U)

#define GPIO_RZ_PIN_CONFIGURE_INT_ENABLE         (1U << 3)
#define GPIO_RZ_PIN_CONFIGURE_INT_DISABLE        (~(1U << 3))
#define GPIO_RZ_PIN_CONFIGURE_INPUT_OUTPUT_RESET (~(0x3 << 2))
#define GPIO_RZ_PIN_SPECIAL_FLAG_GET(flag)       IOPORT_CFG_REGION_NSAFETY

#endif /* CONFIG_SOC_* */

#endif /* ZEPHYR_DRIVERS_GPIO_RENESAS_RZ_H_ */
