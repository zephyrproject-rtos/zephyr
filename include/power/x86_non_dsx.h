/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __X86_NON_DSX_H__
#define __X86_NON_DSX_H__

#include <drivers/gpio.h>

/*
 * @brief GPIO configuration structure
 */
struct gpio_config {
	/* GPIO net name */
	const char *net_name;
	/* GPIO pin port name */
	const char *port_name;
	/* GPIO pin index */
	const gpio_pin_t pin;
	/* GPIO configuration flags */
	const gpio_flags_t flags;
	/* Device structure for the driver instance */
	const struct device *port;
};

/* Power sequencing GPIOs */
#define PCH_EC_SLP_SUS_L	slpsus
#define PCH_EC_SLP_S0_L		slps0
#define VR_EC_EC_RSMRST_ODL	rsmrstin
#define VR_EC_ALL_SYS_PWRGD	allsyspwrgd
#define VR_EC_DSW_PWROK		dswpwrokin
#define EC_PCH_RSMRST_L		rsmrstout
#define EC_PCH_DSW_PWROK	dswpwrokout
#define EC_PCH_SYS_PWROK	syspwrok
#define EC_VR_PPVAR_VCCIN	vccin
#define EC_PCH_PWR_BTN_ODL	pchpwrbtn
#define EC_VR_EN_PP5000_A	enpp5p0
#define EC_VR_EN_PP3300_A	enpp3p3

/* GPIO net name as in schematics */
#define GPIO_NET_NAME(node)	DT_LABEL(DT_NODELABEL(node))

/* GPIO structure assignment */
#define POWER_SEQ_GPIO(node)	\
	.net_name = GPIO_NET_NAME(node), \
	.port_name = DT_GPIO_LABEL(DT_NODELABEL(node), gpios), \
	.pin = DT_GPIO_PIN(DT_NODELABEL(node), gpios), \
	.flags = DT_GPIO_FLAGS(DT_NODELABEL(node), gpios)

/* Check if the GPIO is present */
#define POWER_SEQ_GPIO_PRESENT(node) \
	DT_NODE_HAS_STATUS(DT_NODELABEL(node), okay)

/*
 * @brief Create power sequencing thread
 *
 * TODO: Add details about inputs
 */
void pwrseq_thread(void *p1, void *p2, void *p3);

#endif /* __X86_NON_DSX_H__ */
