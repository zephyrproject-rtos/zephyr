/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32WB family processors.
 *
 */


#ifndef _STM32WBX_SOC_H_
#define _STM32WBX_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32wbxx.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#endif /* !_ASMLANGUAGE */

#endif /* _STM32WBX_SOC_H_ */
