/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for pin control drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_NRF_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_NRF_H_

/**
 * @brief Pin Controller Interface (nRF)
 * @defgroup pinctrl_interface_nrf Pin Controller Interface
 * @ingroup pinctrl_interface
 * @{
 */

#include <zephyr/types.h>
#include <dt-bindings/pinctrl/nrf-pinctrl.h>

#include "states.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Pin control state for nRF. */
struct pinctrl_state {
	/** State identifier. */
	enum pinctrl_state_id id;
	/** Pin configurations. */
	const uint32_t *pincfgs;
	/** Number of pin configurations. */
	size_t pincfgs_cnt;
};

/**
 * @brief Utility macro to obtain port and pin combination.
 *
 * @param pincfg Pin configuration bit field.
 */
#define PINCTRL_NRF_GET_PIN(pincfg) \
	(((pincfg) >> PINCTRL_NRF_PIN_POS) & PINCTRL_NRF_PIN_MSK)

/**
 * @brief Utility macro to obtain pin direction.
 *
 * @param pincfg Pin configuration bit field.
 */
#define PINCTRL_NRF_GET_DIR(pincfg) \
	(((pincfg) >> PINCTRL_NRF_DIR_POS) & PINCTRL_NRF_DIR_MSK)

/**
 * @brief Utility macro to obtain input buffer configuration.
 *
 * @param pincfg Pin configuration bit field.
 */
#define PINCTRL_NRF_GET_INP(pincfg) \
	(((pincfg) >> PINCTRL_NRF_INP_POS) & PINCTRL_NRF_INP_MSK)

/**
 * @brief Utility macro to obtain pin pull configuration.
 *
 * @param pincfg Pin configuration bit field.
 */
#define PINCTRL_NRF_GET_PULL(pincfg) \
	(((pincfg) >> PINCTRL_NRF_PULL_POS) & PINCTRL_NRF_PULL_MSK)

/**
 * @brief Utility macro to obtain pin function.
 *
 * @param pincfg Pin configuration bit field.
 */
#define PINCTRL_NRF_GET_FUN(pincfg) \
	(((pincfg) >> PINCTRL_NRF_FUN_POS) & PINCTRL_NRF_FUN_MSK)

/**
 * @brief Obtain the variable name storing the state information for the given
 * state index and DT node identifier.
 *
 * @param state_idx State index.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATE_NAME(state_idx, node_id) \
	_CONCAT(__pinctrl_ ## state_idx, DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Helper macro to define a pin control state.
 *
 * @param state_idx State index.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATE_DEFINE(state_idx, node_id)			       \
	COND_CODE_1(DT_PINCTRL_HAS_IDX(node_id, state_idx),		       \
	(static const struct pinctrl_state				       \
	Z_PINCTRL_STATE_NAME(state_idx, node_id) = {			       \
		.id = Z_PINCTRL_STATE_ID(state_idx, node_id),		       \
		.pincfgs = (const uint32_t[])DT_PROP(			       \
			DT_PINCTRL_BY_IDX(node_id, state_idx, 0), pincfg),     \
		.pincfgs_cnt = DT_PROP_LEN(				       \
			DT_PINCTRL_BY_IDX(node_id, state_idx, 0), pincfg),     \
	};), ())

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_NRF_H_ */
