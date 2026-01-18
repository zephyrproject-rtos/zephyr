/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_MICROCHIP_MEC_MEC174X_SOC_H
#define __SOC_MICROCHIP_MEC_MEC174X_SOC_H

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(96)

#ifndef _ASMLANGUAGE

#define MCHP_HAS_UART_LSR2

#include <device_mec5.h>

/* common peripheral register defines */
#include <reg/mec_acpi_ec.h>
#include <reg/mec_adc.h>
#include <reg/mec_global_cfg.h>
#include <reg/mec_gpio.h>
#include <reg/mec_kbc.h>
#include <reg/mec_keyscan.h>
#include <reg/mec_peci.h>
#include <reg/mec_ps2.h>
#include <reg/mec_pwm.h>
#include <reg/mec_tach.h>
#include <reg/mec_tfdp.h>
#include <reg/mec_timers.h>
#include <reg/mec_uart.h>
#include <reg/mec_vci.h>
#include <reg/mec_wdt.h>

/* common SoC API */
#include <soc_dt.h>
#include <soc_ecia.h>
#include <soc_espi_channels.h>
#include <soc_gpio.h>
#include <soc_mmcr.h>
#include <soc_pcr.h>
#include <soc_pins.h>

#endif
#endif
