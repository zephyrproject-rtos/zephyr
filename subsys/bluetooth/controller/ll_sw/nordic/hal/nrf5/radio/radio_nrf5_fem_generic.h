/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file contains helper macros for dealing with the devicetree
 * radio node's fem property, in the case that it has compatible
 * "generic-fem-two-ctrl-pins".
 *
 * Do not include it directly.
 *
 * For these devices:
 *
 *  Value             Property
 *  ---------         --------
 *  PA pin            ctx-gpios
 *  PA offset         ctx-settle-time-us
 *  LNA pin           crx-gpios
 *  LNA offset        crx-settle-time-us
 */

#define HAL_RADIO_GPIO_PA_PROP_NAME         "ctx-gpios"
#define HAL_RADIO_GPIO_PA_OFFSET_PROP_NAME  "ctx-settle-time-us"
#define HAL_RADIO_GPIO_LNA_PROP_NAME        "crx-gpios"
#define HAL_RADIO_GPIO_LNA_OFFSET_PROP_NAME "crx-settle-time-us"

#if FEM_HAS_PROP(ctx_gpios)
#define HAL_RADIO_GPIO_HAVE_PA_PIN         1
#define HAL_RADIO_GPIO_PA_PROP             ctx_gpios

#define HAL_RADIO_GPIO_PA_OFFSET_MISSING   (!FEM_HAS_PROP(ctx_settle_time_us))
#define HAL_RADIO_GPIO_PA_OFFSET \
	DT_PROP_OR(FEM_NODE, ctx_settle_time_us, 0)
#else  /* !FEM_HAS_PROP(ctx_gpios) */
#define HAL_RADIO_GPIO_PA_OFFSET_MISSING 0
#endif	/* FEM_HAS_PROP(ctx_gpios) */

#if FEM_HAS_PROP(crx_gpios)
#define HAL_RADIO_GPIO_HAVE_LNA_PIN         1
#define HAL_RADIO_GPIO_LNA_PROP             crx_gpios

#define HAL_RADIO_GPIO_LNA_OFFSET_MISSING   (!FEM_HAS_PROP(crx_settle_time_us))
#define HAL_RADIO_GPIO_LNA_OFFSET \
	DT_PROP_OR(FEM_NODE, crx_settle_time_us, 0)
#else  /* !FEM_HAS_PROP(crx_gpios) */
#define HAL_RADIO_GPIO_LNA_OFFSET_MISSING 0
#endif	/* FEM_HAS_PROP(crx_gpios) */
