/*
 * Copyright (c) 2018-2022 Nordic Semiconductor ASA
 * Copyright (c) 2018 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal/nrf_radio.h>

/* Common radio resources */
#include "radio_nrf5_resources.h"

/* Helpers for radio timing conversions */
#define HAL_RADIO_NS2US_CEIL(ns)  ((ns + 999)/1000)
#define HAL_RADIO_NS2US_ROUND(ns) ((ns + 500)/1000)

/* SoC specific defines */
#if defined(CONFIG_BOARD_NRF52_BSIM)
#include "radio_sim_nrf52.h"
#elif defined(CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUNET)
#include "radio_sim_nrf5340.h"
#elif defined(CONFIG_SOC_SERIES_NRF51X)
#include "radio_nrf51.h"
#elif defined(CONFIG_SOC_NRF52805)
#include "radio_nrf52805.h"
#elif defined(CONFIG_SOC_NRF52810)
#include "radio_nrf52810.h"
#elif defined(CONFIG_SOC_NRF52811)
#include "radio_nrf52811.h"
#elif defined(CONFIG_SOC_NRF52820)
#include "radio_nrf52820.h"
#elif defined(CONFIG_SOC_NRF52832)
#include "radio_nrf52832.h"
#elif defined(CONFIG_SOC_NRF52833)
#include "radio_nrf52833.h"
#elif defined(CONFIG_SOC_NRF52840)
#include "radio_nrf52840.h"
#elif defined(CONFIG_SOC_NRF5340_CPUNET)
#include <hal/nrf_vreqctrl.h>
#include "radio_nrf5340.h"
#elif
#error "Unsupported SoC."
#endif

/* Define to reset PPI registration */
#define NRF_PPI_NONE 0

/* This has to come before the ppi/dppi includes below. */
#include "radio_nrf5_fem.h"

#if defined(PPI_PRESENT)
#include <hal/nrf_ppi.h>
#include "radio_nrf5_ppi_resources.h"
#include "radio_nrf5_ppi.h"
#elif defined(DPPI_PRESENT)
#include <hal/nrf_timer.h>
#include <hal/nrf_rtc.h>
#include <hal/nrf_aar.h>
#include <hal/nrf_ccm.h>
#include <hal/nrf_dppi.h>
#include "radio_nrf5_dppi_resources.h"
#include "radio_nrf5_dppi.h"
#else
#error "PPI or DPPI abstractions missing."
#endif

#include "radio_nrf5_txp.h"

/* SoC specific Radio PDU length field maximum value */
#if defined(CONFIG_SOC_SERIES_NRF51X)
#define HAL_RADIO_PDU_LEN_MAX (BIT(5) - 1)
#else
#define HAL_RADIO_PDU_LEN_MAX (BIT(8) - 1)
#endif

/* Common NRF_RADIO power-on reset value. Refer to Product Specification,
 * RADIO Registers section for the documented reset values.
 *
 * NOTE: Only implementation used values defined here.
 *       In the future if MDK or nRFx header include these, use them instead.
 */
#define HAL_RADIO_RESET_VALUE_PCNF1 0x00000000UL
