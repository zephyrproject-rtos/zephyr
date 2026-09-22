/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_MICROCHIP_MEC_MEC15XX_SOC_H
#define __SOC_MICROCHIP_MEC_MEC15XX_SOC_H

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(48)

#ifndef _ASMLANGUAGE

#include <MEC1501hsz.h>
#include <regaccess.h>

/* common peripheral register defines */
#include <reg/mec_gpio.h>
#include <reg/mec_pcr_vbr.h>
#include <reg/mec_uart.h>

/* Keyboard scan register offsets for mec15xx (legacy compatibility with new driver) */
#define XEC_KBD_KSO_SEL_OFS 0x04u
#define XEC_KBD_KSI_IN_OFS 0x08u
#define XEC_KBD_KSI_STS_OFS 0x0cu
#define XEC_KBD_KSI_IEN_OFS 0x10u
#define XEC_KBD_EXT_CTRL_OFS 0x14u

/* common SoC API */
#include <soc_dt.h>
#include <soc_ecia.h>
#include <soc_espi_channels.h>
#include <soc_gpio.h>
#include <soc_mmcr.h>
#include <soc_pcr.h>
#include <soc_pins.h>

#include "soc_espi_saf_v1.h"

#endif
#endif
