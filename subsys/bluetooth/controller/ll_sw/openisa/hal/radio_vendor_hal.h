/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal/RV32M1/radio/radio.h"

/* The openisa vendor HAL does not have the GPIO support functions
 * required for handling radio front-end modules with PA/LNAs.
 *
 * If these are ever implemented, this file should be updated
 * appropriately.
 */
#undef HAL_RADIO_GPIO_HAVE_PA_PIN
#undef HAL_RADIO_GPIO_HAVE_LNA_PIN
