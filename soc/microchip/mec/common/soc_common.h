/*
 * Copyright (c) 2026 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_SOC_MICROCHIP_MEC_COMMON_SOC_COMMON_H_
#define _ZEPHYR_SOC_MICROCHIP_MEC_COMMON_SOC_COMMON_H_

#ifndef SHLU32
#define SHLU32(val, pos) ((uint32_t)(val) << ((uint8_t)(pos) & 0x1fu))
#endif

/* common peripheral register defines */
#include <reg/mec_acpi_ec.h>
#include <reg/mec_adc.h>
#include <reg/mec_espi_iom_v2.h>
#include <reg/mec_espi_saf_v2.h>
#include <reg/mec_espi_vw_v2.h>
#include <reg/mec_global_cfg.h>
#include <reg/mec_gpio.h>
#include <reg/mec_kbc.h>
#include <reg/mec_keyscan.h>
#include <reg/mec_pcr.h>
#include <reg/mec_peci.h>
#include <reg/mec_ps2.h>
#include <reg/mec_pwm.h>
#include <reg/mec_qmspi.h>
#include <reg/mec_tach.h>
#include <reg/mec_tfdp.h>
#include <reg/mec_timers.h>
#include <reg/mec_uart.h>
#include <reg/mec_vci.h>
#include <reg/mec_wdt.h>

/* common SoC API */
#include <pinctrl_soc.h>
#include <soc_dt.h>
#include <soc_ecia.h>
#include <soc_espi_channels.h>
#include <soc_espi_saf_v2.h>
#include <soc_gpio.h>
#include <soc_misc.h>
#include <soc_mmcr.h>
#include <soc_pcr.h>
#include <soc_pins.h>

#endif /* _ZEPHYR_SOC_MICROCHIP_MEC_COMMON_SOC_COMMON_H_ */
