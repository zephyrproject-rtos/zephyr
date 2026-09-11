/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MICROCHIP_BZ62_SOC_H_
#define MICROCHIP_BZ62_SOC_H_

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CX2051BZ62132)
#include <pic32cx2051bz62132.h>
#else
#error Library does not support the specified device.
#endif

#include "pic32cx_bz.h"

#endif /* MICROCHIP_BZ62_SOC_H_ */
