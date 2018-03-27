/*
 * Copyright (c) 2018 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the efr32fg1p soc
 *
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <em_bus.h>
#include <em_common.h>
#include <device.h>

#include "soc_pinmap.h"
#include "../common/soc_gpio.h"

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
