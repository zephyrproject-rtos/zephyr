/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEC5_SOC_H
#define __MEC5_SOC_H

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(96)

#ifndef _ASMLANGUAGE

#include "device_mec5.h"

/* common SoC API */
#include "soc_dt.h"
#include "soc_espi_channels.h"
#include "soc_gpio.h"
#include "soc_pcr.h"
#include "soc_pins.h"

#endif

#endif
