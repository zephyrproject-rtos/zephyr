/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_MICROCHIP_MEC_MEC15XX_SOC_H
#define __SOC_MICROCHIP_MEC_MEC15XX_SOC_H

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(48)

#ifndef _ASMLANGUAGE

#include "MEC1501hsz.h"
#include "regaccess.h"

/* common SoC API */
#include <soc_dt.h>
#include <soc_ecia.h>
#include <soc_espi_channels.h>
#include <soc_gpio.h>
#include <soc_mmcr.h>
#include <soc_pcr.h>
#include <soc_pins.h>

/* common peripheral register defines */
#include <reg/mec_gpio.h>

#include "soc_espi_saf_v1.h"

#endif
#endif
