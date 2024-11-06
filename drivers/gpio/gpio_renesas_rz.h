/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_RENESAS_RZ_H_
#define ZEPHYR_DRIVERS_GPIO_RENESAS_RZ_H_

#include <zephyr/dt-bindings/gpio/renesas-rz-gpio.h>
#include "r_ioport.h"

#define GPIO_RZ_IOPORT_P_REG_BASE_GET  (&R_GPIO->P_20)
#define GPIO_RZ_IOPORT_PM_REG_BASE_GET (&R_GPIO->PM_20)

#define GPIO_RZ_REG_OFFSET(port, pin) (port + (pin / 4))

#define GPIO_RZ_P_VALUE_GET(value, pin)  ((value >> pin) & 1U)
#define GPIO_RZ_PM_VALUE_GET(value, pin) ((value >> (pin * 2)) & 3U)

#define GPIO_RZ_MAX_PORT_NUM 19
#define GPIO_RZ_MAX_TINT_NUM 32

#define GPIO_RZ_TINT_IRQ_OFFSET        429
#define GPIO_RZ_TINT_IRQ_GET(tint_num) (tint_num + GPIO_RZ_TINT_IRQ_OFFSET)

#define GPIO_RZ_TINT_EDGE_RISING  0x0
#define GPIO_RZ_TINT_EDGE_FALLING 0x1
#define GPIO_RZ_TINT_LEVEL_HIGH   0x2
#define GPIO_RZ_TINT_LEVEL_LOW    0x3

#define GPIO_RZ_TSSR_VAL(port, pin) (0x80 | (gpio_rz_int[port] + pin))
#define GPIO_RZ_TSSR_OFFSET(irq)    ((irq % 4) * 8)
#define GPIO_RZ_TITSR_OFFSET(irq)   ((irq % 16) * 2)

#define GPIO_RZ_PIN_CONFIGURE_GET_FILTER(flag) (((flags >> RZG3S_GPIO_FILTER_SHIFT) & 0x1F) << 19U)
#define GPIO_RZ_PIN_CONFIGURE_GET_DRIVE_ABILITY(flag)                                              \
	(((flag >> RZG3S_GPIO_IOLH_SHIFT) & 0x3) << 10U)

#define GPIO_RZ_PIN_CONFIGURE_INT_ENABLE         IOPORT_CFG_TINT_ENABLE
#define GPIO_RZ_PIN_CONFIGURE_INT_DISABLE        (~(IOPORT_CFG_TINT_ENABLE))
#define GPIO_RZ_PIN_CONFIGURE_INPUT_OUTPUT_RESET (~(0x3 << 2))

static const uint8_t gpio_rz_int[GPIO_RZ_MAX_PORT_NUM] = {0,  4,  9,  13, 17, 23, 28, 33, 38, 43,
							  47, 52, 56, 58, 63, 66, 70, 72, 76};
#endif /* ZEPHYR_DRIVERS_GPIO_RENESAS_RZ_H_ */
