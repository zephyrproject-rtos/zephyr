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
#include <zephyr/devicetree.h>
#include <zephyr/cpu_freq/pstate.h>

/**
 * @brief CPU Frequency Scaling Performance State (pstate)
 * @defgroup subsys_cpu_freq_pstate CPU Frequency pstate
 * @since 4.3
 * @version 0.1.0
 * @ingroup subsys_cpu_freq
 * @{
 */

/** Synthesize symbol of pstate from devicetree dependency ordinal */
#define PSTATE_DT_SYM(_node) _CONCAT(__pstate_, DT_DEP_ORD(_node))

/**
 * @brief Define all pstate information for the given node identifier.
 *
 * @param _node Node identifier.
 * @param _config Pointer to the SoC specific configuration for the pstate.
 */
#define PSTATE_DT_DEFINE(_node, _config)                                                           \
	const struct pstate PSTATE_DT_SYM(_node) = {                                               \
		.load_threshold = DT_PROP(_node, load_threshold),                                  \
		.config = _config,                                                                 \
	};

/**
 * @brief Get a pstate reference from a devicetree node identifier.
 *
 * To be used in DT_FOREACH_CHILD() or similar macros
 *
 * @param _node Node identifier.
 */
#define PSTATE_DT_GET(_node) &PSTATE_DT_SYM(_node)


/**
 * @brief CPU performance state (pstate) struct.
 *
 * This struct holds information about a CPU pstate as well as a reference to vendor-specific
 * devicetree properties.
 */
struct pstate {
	uint8_t load_threshold; /**< CPU load threshold at which P-state should be triggered */
	const void *config;     /**< Vendor specific devicetree properties */
};

/* Define performance-states as extern to be picked up by CPU Freq policy */
#define Z_DECLARE_PSTATE_EXTERN(_node) \
	extern const struct pstate PSTATE_DT_SYM(_node);

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states), Z_DECLARE_PSTATE_EXTERN)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_PSTATE_H_ */
