/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MICROCHIP_BZ6_SOC_H_
#define MICROCHIP_BZ6_SOC_H_

#include <zephyr/types.h>
#include <mchp_dt_helper.h>

#if defined(CONFIG_SOC_PIC32CX2051BZ62132)
#include <pic32cx2051bz62132.h>
#elif defined(CONFIG_SOC_PIC32CX2051BZ62064)
#include <pic32cx2051bz62064.h>
#elif defined(CONFIG_SOC_PIC32CX2051BZ66048)
#include <pic32cx2051bz66048.h>
#else
#error Library does not support the specified device.
#endif

#include <pic32cx_bz.h>

#endif /* MICROCHIP_BZ6_SOC_H_ */
