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

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <em_bus.h>
#include <em_common.h>

/* Add include for DTS generated information */
#include <devicetree.h>

#include "soc_pinmap.h"
#include "../common/soc_gpio.h"

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
