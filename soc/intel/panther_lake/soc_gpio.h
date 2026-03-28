/*
 * Copyright (c) 2025, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO macros for the Panther Lake SoC
 *
 * This header file is used to specify the GPIO macros for
 * the Panther Lake SoC.
 */

#ifndef __SOC_GPIO_H_
#define __SOC_GPIO_H_

#define GPIO_OWNERSHIP_BIT(raw_pin)	(0x0)
#define GPIO_RAW_PIN(pin, pin_offset)	pin
#define PAD_CFG0_PMODE_MASK		(0x07 << 10)
#define PIN_OFFSET			(0x10)

#endif /* __SOC_GPIO_H_ */
