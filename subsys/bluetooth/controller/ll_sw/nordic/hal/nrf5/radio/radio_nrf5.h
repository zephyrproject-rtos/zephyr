/*
 * Copyright (c) 2018-2022 Nordic Semiconductor ASA
 * Copyright (c) 2018 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HAL header files for nRF5x SoCs.
 * These has to come before the radio_*.h include below.
 */
#include <hal/nrf_radio.h>

/* Common radio resources */
#include "radio_nrf5_resources.h"

/* Helpers for radio timing conversions.
 * These has to come before the radio_*.h include below.
 */
#define HAL_RADIO_NS2US_CEIL(ns)  ((ns + 999)/1000)
#define HAL_RADIO_NS2US_ROUND(ns) ((ns + 500)/1000)

/* SoC specific defines */
#if defined(CONFIG_SOC_SERIES_NRF51X)
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
#elif defined(CONFIG_SOC_SERIES_NRF54LX)
#include "radio_nrf54lx.h"
#elif defined(CONFIG_BOARD_NRF52_BSIM)
#include "radio_sim_nrf52.h"
#elif defined(CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUNET)
#include <hal/nrf_vreqctrl.h>
#include "radio_sim_nrf5340.h"
#elif defined(CONFIG_BOARD_NRF54L15BSIM_NRF54L15_CPUAPP)
#include "radio_sim_nrf54l.h"
#else
#error "Unsupported SoC."
#endif

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
#include <hal/nrf_grtc.h>
#include <hal/nrf_ppib.h>
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
#include <hal/nrf_rtc.h>
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */

#include <hal/nrf_timer.h>

#if defined(CONFIG_BT_CTLR_LE_ENC) || defined(CONFIG_BT_CTLR_BROADCAST_ISO_ENC)
#include <hal/nrf_ccm.h>
#endif /* CONFIG_BT_CTLR_LE_ENC || CONFIG_BT_CTLR_BROADCAST_ISO_ENC */

#if defined(CONFIG_BT_CTLR_PRIVACY)
#include <hal/nrf_aar.h>
#endif /* CONFIG_BT_CTLR_PRIVACY */

/* Define to reset PPI registration.
 * This has to come before the ppi/dppi includes below.
 */
#define NRF_PPI_NONE 0

/* This has to come before the ppi/dppi includes below. */
#include "radio_nrf5_fem.h"

/* Include RTC/GRTC Compare Index used to Trigger Radio TXEN/RXEN */
#include "hal/cntr.h"

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#include <hal/nrf_ppi.h>
#include "radio_nrf5_ppi_resources.h"
#include "radio_nrf5_ppi.h"
#else
#include <hal/nrf_dppi.h>
#include "radio_nrf5_dppi_resources.h"
#include "radio_nrf5_dppi.h"
#endif

#include "radio_nrf5_txp.h"

/* Common NRF_RADIO power-on reset value. Refer to Product Specification,
 * RADIO Registers section for the documented reset values.
 *
 * NOTE: Only implementation used values defined here.
 *       In the future if MDK or nRFx header include these, use them instead.
 */
#define HAL_RADIO_RESET_VALUE_PCNF1 0x00000000UL

/* SoC specific Radio PDU length field maximum value */
#if defined(CONFIG_SOC_SERIES_NRF51X)
#define HAL_RADIO_PDU_LEN_MAX (BIT(5) - 1)
#else
#define HAL_RADIO_PDU_LEN_MAX (BIT(8) - 1)
#endif

/* This is delay between PPI task START and timer actual start counting. */
#if !defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
#define HAL_RADIO_TMR_START_DELAY_US 1U
#else /* For simulated targets there is no delay for the PPI task -> TIMER start */
#define HAL_RADIO_TMR_START_DELAY_US 0U
#endif

/* This is the minimum prepare duration required to setup radio for deferred transmission */
#define HAL_RADIO_TMR_DEFERRED_TX_DELAY_US 50U
