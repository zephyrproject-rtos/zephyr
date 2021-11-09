/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define HAL_RADIO_NS2US_CEIL(ns)  ((ns + 999)/1000)
#define HAL_RADIO_NS2US_ROUND(ns) ((ns + 500)/1000)

/* Use the timer instance ID, not NRF_TIMERx directly, so that it can be checked
 * in radio_nrf5_ppi.h by the preprocessor.
 */
#define EVENT_TIMER_ID 0
#define EVENT_TIMER    _CONCAT(NRF_TIMER, EVENT_TIMER_ID)

/* EVENTS_TIMER capture register used for sampling TIMER time-stamps. */
#define HAL_EVENT_TIMER_SAMPLE_CC_OFFSET 3
#define HAL_EVENT_TIMER_SAMPLE_TASK NRF_TIMER_TASK_CAPTURE3

#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
#include "radio_sim_nrfxx.h"
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
#include "radio_nrf5340.h"
#elif
#error "Unsupported SoC."
#endif

#if defined(CONFIG_SOC_SERIES_NRF51X)
#define HAL_RADIO_PDU_LEN_MAX (BIT(5) - 1)
#else
#define HAL_RADIO_PDU_LEN_MAX (BIT(8) - 1)
#endif

#include <nrf_peripherals.h>

#if defined(CONFIG_PWM_NRF5_SW)
#define HAL_PALNA_GPIOTE_CHAN 3
#define HAL_PDN_GPIOTE_CHAN 4
#define HAL_CSN_GPIOTE_CHAN 5
#else
#define HAL_PALNA_GPIOTE_CHAN 0
#define HAL_PDN_GPIOTE_CHAN 1
#define HAL_CSN_GPIOTE_CHAN 2
#endif

#if defined(CONFIG_BT_CTLR_GPIO_PA)
#define HAL_RADIO_GPIO_HAVE_PA_PIN 1
#endif
#if defined(CONFIG_BT_CTLR_GPIO_LNA)
#define HAL_RADIO_GPIO_HAVE_LNA_PIN 1
#endif

#if defined(CONFIG_BT_CTLR_GPIO_PA_POL_INV)
#define HAL_RADIO_GPIO_PA_POL_INV CONFIG_BT_CTLR_GPIO_PA_POL_INV
#endif
#if defined(CONFIG_BT_CTLR_GPIO_LNA_POL_INV)
#define HAL_RADIO_GPIO_LNA_POL_INV CONFIG_BT_CTLR_GPIO_LNA_POL_INV
#endif
#if defined(CONFIG_BT_CTLR_GPIO_PDN_POL_INV)
#define HAL_RADIO_GPIO_NRF21540_PDN_POL_INV CONFIG_BT_CTLR_GPIO_PDN_POL_INV
#endif
#if defined(CONFIG_BT_CTLR_GPIO_CSN_POL_INV)
#define HAL_RADIO_GPIO_NRF21540_CSN_POL_INV CONFIG_BT_CTLR_GPIO_CSN_POL_INV
#endif

#if defined(CONFIG_BT_CTLR_GPIO_PA_OFFSET)
#define HAL_RADIO_GPIO_PA_OFFSET CONFIG_BT_CTLR_GPIO_PA_OFFSET
#endif
#if defined(CONFIG_BT_CTLR_GPIO_LNA_OFFSET)
#define HAL_RADIO_GPIO_LNA_OFFSET CONFIG_BT_CTLR_GPIO_LNA_OFFSET
#endif
#if defined(CONFIG_BT_CTLR_GPIO_PDN_CSN_OFFSET)
#define HAL_RADIO_GPIO_NRF21540_PDN_OFFSET CONFIG_BT_CTLR_GPIO_PDN_CSN_OFFSET
#endif

#if defined(CONFIG_BT_CTLR_FEM_NRF21540)
#define HAL_RADIO_FEM_IS_NRF21540 1
#endif

#if defined(PPI_PRESENT)
#include "radio_nrf5_ppi.h"
#elif defined(DPPI_PRESENT)
#include "radio_nrf5_dppi.h"
#else
#error "PPI or DPPI abstractions missing."
#endif
#include "radio_nrf5_txp.h"
