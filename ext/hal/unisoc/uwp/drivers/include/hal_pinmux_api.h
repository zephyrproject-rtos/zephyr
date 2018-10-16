/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HAL_PINMUX_DRVAPI_UWP_H
#define _HAL_PINMUX_DRVAPI_UWP_H

#ifdef __cpluscplus
extern "C"
{
#endif

#include "uwp_hal.h"

struct pm_pinfunc_tag {
	u32_t addr;
	u32_t value;
};

int uwp_pinmux_init(struct device *dev);

#ifdef __cpluscplus
}
#endif
#endif
