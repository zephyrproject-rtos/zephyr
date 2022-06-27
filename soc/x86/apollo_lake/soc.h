/*
 * Copyright (c) 2018-2019, Intel Corporation
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the Apollo Lake SoC
 *
 * This header file is used to specify and describe soc-level aspects for
 * the 'Apollo Lake' SoC.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE
#include <zephyr/device.h>
#include <zephyr/random/rand32.h>
#endif

#ifdef CONFIG_GPIO_INTEL
#include "soc_gpio.h"
#endif

#include <zephyr/drivers/pcie/pcie.h>

/* This expands to "PCIE_BDF(0, 0x18, 0)" on this platform */
#define X86_SOC_EARLY_SERIAL_PCIDEV DT_REG_ADDR(DT_CHOSEN(zephyr_console))

#endif /* __SOC_H_ */
