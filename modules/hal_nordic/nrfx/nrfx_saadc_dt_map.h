/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Overrides the nrfx default SAADC external AIN table with the one described
 * by the "nordic,saadc-inputs" property of the enabled SAADC node.
 */

#ifndef ZEPHYR_MODULES_HAL_NORDIC_NRFX_SAADC_DT_MAP_H_
#define ZEPHYR_MODULES_HAL_NORDIC_NRFX_SAADC_DT_MAP_H_

#include <zephyr/devicetree.h>
#include <hal/nrf_common.h>
#include <hal/nrf_saadc.h>

#define NRFX_SAADC_DT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_saadc)

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_saadc) && \
	DT_NODE_HAS_PROP(NRFX_SAADC_DT_NODE, nordic_saadc_inputs)

/* Including nrf_saadc.h above already pulled in the nrfx default via nrfx_soc_defines.h. */
#undef SAADC_EXTERNAL_AIN_PSELS

/* Highest AIN number across supported SoCs (see NRF_SAADC_AIN14). */
#define NRFX_SAADC_DT_MAX_AIN 14

#if NRF_SAADC_HAS_AIN_AS_PIN
/* Pin-based SoCs: the slot holds the GPIO pin number. */
#define NRFX_SAADC_DT_PSEL(node_id, prop, idx) \
	NRF_PIN_PORT_TO_PIN_NUMBER( \
		DT_PHA_BY_IDX(node_id, prop, idx, pin), \
		DT_PROP(DT_PHANDLE_BY_IDX(node_id, prop, idx), port))
#else
/* Legacy SoCs: the pin is fixed in silicon, so the slot holds the AIN input encoding. */
#define NRFX_SAADC_DT_PSEL(node_id, prop, idx) \
	UTIL_CAT(NRF_SAADC_INPUT_AIN, DT_PHA_BY_IDX(node_id, prop, idx, ain))
#endif

/* The "ain" cell selects the slot, so entry order in the DTS does not matter. */
#define NRFX_SAADC_DT_INPUT(node_id, prop, idx) \
	[DT_PHA_BY_IDX(node_id, prop, idx, ain)] = NRFX_SAADC_DT_PSEL(node_id, prop, idx),

/* Inputs missing from the DTS stay not present; later designators override the fill. */
#define SAADC_EXTERNAL_AIN_PSELS \
	[0 ... NRFX_SAADC_DT_MAX_AIN] = NRFX_SAADC_INPUT_NOT_PRESENT, \
	DT_FOREACH_PROP_ELEM(NRFX_SAADC_DT_NODE, nordic_saadc_inputs, NRFX_SAADC_DT_INPUT)

#endif /* SAADC node with nordic,saadc-inputs available. */

#endif /* ZEPHYR_MODULES_HAL_NORDIC_NRFX_SAADC_DT_MAP_H_ */