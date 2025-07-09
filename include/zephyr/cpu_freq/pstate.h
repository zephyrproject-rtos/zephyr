/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_FREQ_PSTATE_H_
#define ZEPHYR_SUBSYS_CPU_FREQ_PSTATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/cpu_freq/pstate.h>

/**
 * @brief Define all P-state information for the given node identifier.
 *
 * @param _node Node identifier.
 * @param _config Pointer to the SoC specific configuration for the P-state.
 */
#define PSTATE_DT_DEFINE(_node, _config)                                                          \
	const struct pstate _CONCAT(pstate_, DEVICE_DT_NAME_GET(_node)) = {                      \
		.load_threshold = DT_PROP(_node, load_threshold),                                  \
		.disabled = DT_PROP(_node, disabled),                                              \
		.config = _config,                                                                 \
	};

/**
 * @brief Get a P-state reference from a devicetree node identifier.
 *
 * To be used in DT_FOREACH_CHILD() or similar macros
 *
 * @param _node Node identifier.
 */
#define PSTATE_DT_GET(_node) &(_CONCAT(pstate_, DEVICE_DT_NAME_GET(_node))),

struct pstate {
	uint32_t load_threshold; /**< CPU load threshold at which P-state should be triggered */
	uint8_t disabled;        /**< Flag to indicate if P-state is disabled */
	const void *config;      /**< Vendor specific devicetree properties */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_PSTATE_H_ */
