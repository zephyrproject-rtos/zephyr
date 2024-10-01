/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * nRF SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_NORDIC_NRF_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NORDIC_NRF_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/nrf-pinctrl.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** Type for nRF pin. */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to check if a function requires clockpin enable.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 * @param p_node_id Parent node identifier.
 */
#define Z_CHECK_CLOCKPIN_ENABLE(node_id, prop, idx, fun) \
	DT_PROP_BY_IDX(node_id, prop, idx) == fun ? BIT(NRF_CLOCKPIN_ENABLE_POS) :

/**
 * @brief Utility macro compute the clockpin enable bit.
 *
 * @note DT_FOREACH_PROP_ELEM_SEP_VARGS() is used instead of
 * DT_FOREACH_PROP_ELEM_VARGS() because the latter is already resolved in the
 * same run.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 * @param p_node_id Parent node identifier.
 */
#define Z_GET_CLOCKPIN_ENABLE(node_id, prop, idx, p_node_id)		            \
	COND_CODE_1(DT_NODE_HAS_PROP(p_node_id, nordic_clockpin_enable),            \
		    ((DT_FOREACH_PROP_ELEM_SEP_VARGS(			            \
			p_node_id, nordic_clockpin_enable, Z_CHECK_CLOCKPIN_ENABLE, \
			(), NRF_GET_FUN(DT_PROP_BY_IDX(node_id, prop, idx)))        \
		      0)), (0))

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 * @param p_node_id Parent node identifier.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx, p_node_id)		       \
	(DT_PROP_BY_IDX(node_id, prop, idx) |				       \
	 ((NRF_PULL_DOWN * DT_PROP(node_id, bias_pull_down)) << NRF_PULL_POS) |\
	 ((NRF_PULL_UP * DT_PROP(node_id, bias_pull_up)) << NRF_PULL_POS) |    \
	 (DT_PROP(node_id, nordic_drive_mode) << NRF_DRIVE_POS) |	       \
	 ((NRF_LP_ENABLE * DT_PROP(node_id, low_power_enable)) << NRF_LP_POS) |\
	 (DT_PROP(node_id, nordic_invert) << NRF_INVERT_POS) |		       \
	 Z_GET_CLOCKPIN_ENABLE(node_id, prop, idx, p_node_id)		       \
	),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		       \
				DT_FOREACH_PROP_ELEM_VARGS, psels,	       \
				Z_PINCTRL_STATE_PIN_INIT, node_id)}

/**
 * @brief Utility macro to obtain pin function.
 *
 * @param pincfg Pin configuration bit field.
 */
#define NRF_GET_FUN(pincfg) (((pincfg) >> NRF_FUN_POS) & NRF_FUN_MSK)

/**
 * @brief Utility macro to obtain pin clockpin enable flag.
 *
 * @param pincfg Pin configuration bit field.
 */
#define NRF_GET_CLOCKPIN_ENABLE(pincfg) \
	(((pincfg) >> NRF_CLOCKPIN_ENABLE_POS) & NRF_CLOCKPIN_ENABLE_MSK)

/**
 * @brief Utility macro to obtain pin inversion flag.
 *
 * @param pincfg Pin configuration bit field.
 */
#define NRF_GET_INVERT(pincfg) (((pincfg) >> NRF_INVERT_POS) & NRF_INVERT_MSK)

/**
 * @brief Utility macro to obtain pin low power flag.
 *
 * @param pincfg Pin configuration bit field.
 */
#define NRF_GET_LP(pincfg) (((pincfg) >> NRF_LP_POS) & NRF_LP_MSK)

/**
 * @brief Utility macro to obtain pin drive mode.
 *
 * @param pincfg Pin configuration bit field.
 */
#define NRF_GET_DRIVE(pincfg) (((pincfg) >> NRF_DRIVE_POS) & NRF_DRIVE_MSK)

/**
 * @brief Utility macro to obtain pin pull configuration.
 *
 * @param pincfg Pin configuration bit field.
 */
#define NRF_GET_PULL(pincfg) (((pincfg) >> NRF_PULL_POS) & NRF_PULL_MSK)

/**
 * @brief Utility macro to obtain port and pin combination.
 *
 * @param pincfg Pin configuration bit field.
 */
#define NRF_GET_PIN(pincfg) (((pincfg) >> NRF_PIN_POS) & NRF_PIN_MSK)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NORDIC_NRF_COMMON_PINCTRL_SOC_H_ */
