/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEC_SOC_H
#define __MEC_SOC_H

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(48)

#ifndef _ASMLANGUAGE

#include "MEC1501hsz.h"
#include "regaccess.h"

/* common SoC API */
#include "../common/soc_dt.h"
#include "../common/soc_gpio.h"
#include "../common/soc_pcr.h"
#include "../common/soc_pins.h"
#include "../common/soc_espi_channels.h"
#include "soc_espi_saf_v1.h"

/* common peripheral register defines */
#include "../common/reg/mec_gpio.h"

#endif

#endif
