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


/**
 * @brief System power states for Non Deep Sleep Well
 * EC is an always on device in a Non Deep Sx system except when the EC
 * is hibernated or all the VRs are turned off.
 */
enum power_states_ndsx {
	/*
	 * Actual power states
	 */
	/* AP is off & EC is on */
	SYS_POWER_STATE_G3,
	/* AP is in soft off state */
	SYS_POWER_STATE_S5,
	/* AP is suspended to Non-volatile disk */
	SYS_POWER_STATE_S4,
	/* AP is suspended to RAM */
	SYS_POWER_STATE_S3,
	/* AP is in active state */
	SYS_POWER_STATE_S0,

	/*
	 * Intermediate power up states
	 */
	/*  Determine if the AP's power rails are turned on */
	SYS_POWER_STATE_G3S5,
	/*  Determine if AP is suspended from sleep */
	SYS_POWER_STATE_S5S4,
	/* Determine if Suspend to Disk is de-asserted*/
	SYS_POWER_STATE_S4S3,
	/* Determine if Suspend to RAM is de-asserted*/
	SYS_POWER_STATE_S3S0,

	/*
	 * Intermediate power down states
	 */
	/*  Determine if the AP's power rails are turned off */
	SYS_POWER_STATE_S5G3,
	/*  Determine if AP is suspended to sleep */
	SYS_POWER_STATE_S4S5,
	/* Determine if Suspend to Disk is asserted*/
	SYS_POWER_STATE_S3S4,
	/* Determine if Suspend to RAM is asserted*/
	SYS_POWER_STATE_S0S3,
};

/*
 * @brief Create power sequencing thread
 *
 * @param p1 Thread sleep time in ms
 * TODO: Add details about inputs
 */
void pwrseq_thread(void *p1, void *p2, void *p3);

#endif /* __X86_NON_DSX_H__ */
