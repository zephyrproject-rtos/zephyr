/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal/RV32M1/radio/radio.h"

#if defined(CONFIG_BT_CTLR_GPIO_PA_OFFSET)
#define HAL_RADIO_GPIO_PA_OFFSET CONFIG_BT_CTLR_GPIO_PA_OFFSET
#endif
#if defined(CONFIG_BT_CTLR_GPIO_LNA_OFFSET)
#define HAL_RADIO_GPIO_LNA_OFFSET CONFIG_BT_CTLR_GPIO_LNA_OFFSET
#endif
