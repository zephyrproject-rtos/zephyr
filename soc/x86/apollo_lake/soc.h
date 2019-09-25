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

#include <sys/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <random/rand32.h>
#endif

#ifdef CONFIG_GPIO_INTEL_APL
#include "soc_gpio.h"
#endif

#endif /* __SOC_H_ */
