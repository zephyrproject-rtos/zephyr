/*
 * Copyright (c) 2018-2020, Intel Corporation
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the Elkhart Lake SoC
 *
 * This header file is used to specify and describe soc-level aspects for
 * the 'Elkhart Lake' SoC.
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

#if DT_ON_BUS(DT_CHOSEN(zephyr_console), pcie)
#include <zephyr/drivers/pcie/pcie.h>
#define X86_SOC_EARLY_SERIAL_PCIDEV PCIE_BDF(0, 0x19, 2) /* uart2 */
#else
#define X86_SOC_EARLY_SERIAL_MMIO8_ADDR DT_REG_ADDR(DT_CHOSEN(zephyr_console))
#endif

#endif /* __SOC_H_ */
