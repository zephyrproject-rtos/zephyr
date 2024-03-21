/*
 * Copyright (c) 2021, Teslabs Engineering S.L.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ARM_GIGADEVICE_GD32F4XX_SOC_H_
#define _SOC_ARM_GIGADEVICE_GD32F4XX_SOC_H_

#ifndef _ASMLANGUAGE

#include <gd32f4xx.h>

/* The GigaDevice HAL headers define this, but it conflicts with the Zephyr can.h */
#undef CAN_MODE_NORMAL

#endif /* _ASMLANGUAGE */

#endif /* _SOC_ARM_GIGADEVICE_GD32F4XX_SOC_H_ */
