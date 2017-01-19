/* gpio_sch.h - Descriptions of registers for Intel SCH GPIO controller */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __GPIO_SCH_H__
#define __GPIO_SCH_H__

#include <stdint.h>
#include <kernel.h>
#include <gpio.h>

#define GPIO_SCH_REG_GEN		(0x00)
#define GPIO_SCH_REG_GIO		(0x04)
#define GPIO_SCH_REG_GLVL		(0x08)
#define GPIO_SCH_REG_GTPE		(0x0C)
#define GPIO_SCH_REG_GTNE		(0x10)
#define GPIO_SCH_REG_GGPE		(0x14)
#define GPIO_SCH_REG_GSMI		(0x18)
#define GPIO_SCH_REG_GTS		(0x1C)

struct gpio_sch_config {
	uint32_t regs;
	uint8_t bits;
	uint8_t stride[3];
};

#define GPIO_SCH_POLLING_STACK_SIZE 	1024
#define GPIO_SCH_POLLING_MSEC		200

struct gpio_sch_data {
	char __stack polling_stack[GPIO_SCH_POLLING_STACK_SIZE];
	sys_slist_t callbacks;
	struct k_timer poll_timer;

	struct {
		uint32_t gtpe;
		uint32_t gtne;
	} int_regs;

	uint32_t cb_enabled;
	uint8_t poll;
	uint8_t stride[3];
};

#endif /* __GPIO_SCH_H__ */
