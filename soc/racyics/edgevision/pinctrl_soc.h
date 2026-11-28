/*
 * SPDX-FileCopyrightText: Copyright 2026 sensry.io
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RACYICS_EDGEVISION_EV1X_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RACYICS_EDGEVISION_EV1X_COMMON_PINCTRL_SOC_H_

#include <stdint.h>

/*
 * Racyics edgeVision UART pins are fixed and do not need pinctrl state tables.
 * Zephyr still includes <pinctrl_soc.h> when PINCTRL is enabled, so provide
 * the minimal type definition expected by the generic pinctrl API.
 */
typedef uint32_t pinctrl_soc_pin_t;

#endif /* ZEPHYR_SOC_RACYICS_EDGEVISION_EV1X_COMMON_PINCTRL_SOC_H_ */
