/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys_io.h>
#include <pinmux.h>
#include <pinmux/pinmux.h>

#include "pinmux_rda5981a.h"

/* pin assignments for 96boards Bluesky board */
static const struct pin_config pinconf[] = {
/*
	{PB0, 0},
	{PB1, 0},
	{PB2, 0},
	{PB3, 0},
	{PB4, 0},
	{PB5, 0},
	{PB6, 0},
	{PB7, 0},
	{PB8, 0},
	{PB9, 0},
	{PA8, 0},
	{PA9, 0},
	{PC0, 0},
	{PC1, 0},
	{PC2, 0},
	{PC3, 0},
	{PC4, 0},
	{PC5, 0},
	{PC6, 0},
	{PC7, 0},
	{PC8, 0},
	{PC9, 0},
	{PD0, 0},
	{PD1, 0},
	{PD2, 0},
	{PD3, 0},
	{PA0, 1},
	{PA1, 1},
	{PA2, 1},
	{PA3, 1},
	{PA4, 1},
	{PA5, 1},
	{PA6, 1},
	{PA7, 1},
*/
};

static int pinmux_rda5981a_init(struct device *port)
{
	rda5981a_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_rda5981a_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
