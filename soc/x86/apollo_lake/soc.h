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

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <random/rand32.h>
#endif

#ifdef CONFIG_GPIO_INTEL_APL
#include "soc_gpio.h"
#endif

#ifdef CONFIG_PCI

/*
 * PCI definitions
 */
#define PCI_BUS_NUMBERS                         1

#define PCI_CTRL_ADDR_REG                       0xCF8
#define PCI_CTRL_DATA_REG                       0xCFC

/**
 * @brief Convert PCI interrupt PIN to IRQ
 *
 * BIOS should have assigned vectors linearly.
 * If not, override this in board configuration.
 */
#define pci_pin2irq(bus, dev, pin)              (pin)

#endif /* CONFIG_PCI */

#endif /* __SOC_H_ */
