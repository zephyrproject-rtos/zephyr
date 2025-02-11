/*
 * Copyright (c) 2021, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO macros for the Elkhart Lake SoC
 *
 * This header file is used to specify the GPIO macros for
 * the ELkhart Lake SoC.
 */

#ifndef __SOC_GPIO_H_
#define __SOC_GPIO_H_

#define GPIO_INTEL_NR_SUBDEVS		15

#define REG_PAD_OWNER_BASE              0x0020
#define REG_GPI_INT_STS_BASE            0x0100
#define PAD_CFG0_PMODE_MASK             (0x0F << 10)

#define REG_PAD_BASE_ADDR		0x000C
#define REG_GPI_INT_EN_BASE		0x0120
#define REG_PAD_HOST_SW_OWNER		0x0B0
#define PAD_BASE_ADDR_MASK              0xfff

#define GPIO_REG_BASE(reg_base) \
	(reg_base & ~PAD_BASE_ADDR_MASK)

#define GPIO_PAD_BASE(reg_base) \
	(reg_base & PAD_BASE_ADDR_MASK)

#define GPIO_PAD_OWNERSHIP(raw_pin, pin_offset)					\
	(pin_offset % 8) ?							\
		REG_PAD_OWNER_BASE +						\
			 ((((pin_offset / 8) + 1) + (raw_pin / 8)) * 0x4) :	\
		REG_PAD_OWNER_BASE +						\
			 (((pin_offset / 8) + (raw_pin / 8)) * 0x4);		\

#define GPIO_OWNERSHIP_BIT(raw_pin) ((raw_pin % 8) * 4)

#define GPIO_RAW_PIN(pin, pin_offset) pin

#define GPIO_INTERRUPT_BASE(cfg) \
	(cfg->group_index * 0x4)

#define GPIO_BASE(cfg) \
	(cfg->group_index * 0x4)

#define PIN_OFFSET 0x10

#endif /* __SOC_GPIO_H_ */
