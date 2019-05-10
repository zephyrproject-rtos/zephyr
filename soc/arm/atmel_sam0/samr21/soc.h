/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ATMEL_SAMR21_SOC_H_
#define ZEPHYR_ATMEL_SAMR21_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PART_NUMBER_SAMR21E16A)
#include <samr21e16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMR21E17A)
#include <samr21e17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMR21E18A)
#include <samr21e18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMR21E19A)
#include <samr21e19a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMR21G16A)
#include <samr21g16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMR21G17A)
#include <samr21g17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMR21G18A)
#include <samr21g18a.h>
#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ATMEL_SAMR21_SOC_H_ */
