/*
 * Copyright (c) 2018 Sean Nyekjaer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAMD_SOC_H_
#define _ATMEL_SAMD_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PART_NUMBER_SAMD20E14)
#include <samd20e14.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20E15)
#include <samd20e15.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20E16)
#include <samd20e16.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20E17)
#include <samd20e17.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20E18)
#include <samd20e18.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20G14)
#include <samd20g14.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20G15)
#include <samd20g15.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20G16)
#include <samd20g16.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20G17)
#include <samd20g17.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20G17U)
#include <samd20g17u.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20G18)
#include <samd20g18.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20G18U)
#include <samd20g18u.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20J14)
#include <samd20j14.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20J15)
#include <samd20j15.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20J16)
#include <samd20j16.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20J17)
#include <samd20j17.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD20J18)
#include <samd20j18.h>
#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#endif /* _ATMEL_SAMD_SOC_H_ */
