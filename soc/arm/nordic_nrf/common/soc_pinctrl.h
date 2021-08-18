/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SOC_NRF_KINETIS_SOC_PINCTRL_H_
#define SOC_NRF_KINETIS_SOC_PINCTRL_H_

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t pinctrl_soc_pins_t;

#define PINCTRL_SOC_PINS_ELEM_INIT(node, prop, idx) 		\
DT_PROP_BY_IDX(node, prop, idx),

#define Z_PINCTRL_DT_SPEC_GET(node, pin_indx) \
{ DT_FOREACH_PROP_ELEM(DT_PROP_BY_IDX(node, pinctrl_##pin_indx, 0), pincfg, PINCTRL_SOC_PINS_ELEM_INIT) }

#ifdef __cplusplus
}
#endif

#endif /* SOC_NRF_KINETIS_SOC_PINCTRL_H_ */
