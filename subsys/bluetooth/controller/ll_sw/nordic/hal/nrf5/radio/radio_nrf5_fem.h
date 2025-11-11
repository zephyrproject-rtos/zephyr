/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Generic helper macros for getting radio front-end module (FEM)
 * settings from devicetree. The main task here is to check the
 * devicetree compatible of the node the fem property points at, and
 * pull in a subheader that translates from that compatible's specific
 * properties to the generic macros required by the nRF5 radio HAL.
 */

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/gpio.h>

#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), fem)
#define FEM_NODE             DT_PHANDLE(DT_NODELABEL(radio), fem)
#if DT_NODE_HAS_STATUS_OKAY(FEM_NODE)
#define HAL_RADIO_HAVE_FEM
#endif	/* DT_NODE_HAS_STATUS_OKAY(FEM_NODE) */
#endif	/* DT_NODE_HAS_PROP(DT_NODELABEL(radio), fem)) */

/* Does FEM_NODE have a particular DT compatible? */
#define FEM_HAS_COMPAT(compat) DT_NODE_HAS_COMPAT(FEM_NODE, compat)

/* Does FEM_NODE have a particular DT property defined? */
#define FEM_HAS_PROP(prop) DT_NODE_HAS_PROP(FEM_NODE, prop)

/*
 * Device-specific settings are pulled in based FEM_NODE's compatible
 * property.
 */

#ifdef HAL_RADIO_HAVE_FEM
#if FEM_HAS_COMPAT(generic_fem_two_ctrl_pins)
#include "radio_nrf5_fem_generic.h"
#elif FEM_HAS_COMPAT(nordic_nrf21540_fem)
#include "radio_nrf5_fem_nrf21540.h"
#else
#error "radio node fem property has an unsupported compatible"
#endif	/* FEM_HAS_COMPAT(generic_fem_two_ctrl_pins) */
#endif	/* HAL_RADIO_HAVE_FEM */

/*
 * Define POL_INV macros expected by radio_nrf5_dppi as needed.
 */

#ifdef HAL_RADIO_GPIO_HAVE_PA_PIN
#if DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_PA_PROP) & GPIO_ACTIVE_LOW
#define HAL_RADIO_GPIO_PA_POL_INV 1
#endif
#endif	/* HAL_RADIO_GPIO_HAVE_PA_PIN */

#ifdef HAL_RADIO_GPIO_HAVE_LNA_PIN
#if DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP) & GPIO_ACTIVE_LOW
#define HAL_RADIO_GPIO_LNA_POL_INV 1
#endif
#endif	/* HAL_RADIO_GPIO_HAVE_LNA_PIN */
