/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AMP_TI_K3_H
#define AMP_TI_K3_H

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/dt-bindings/amp/ti-k3-amp.h>

/**
 * @brief Cluster type
 */
enum amp_ti_k3_cluster_type {
	/** A Cortex-R cluster */
	AMP_TI_K3_CLUSTER_TYPE_CORTEX_R = 0,
	/** A Cortex-M cluster */
	AMP_TI_K3_CLUSTER_TYPE_CORTEX_M = 1,
};

/**
 * @brief Boot options for K3 boot cores
 */
struct amp_ti_k3_boot_options {
	/* Config flags that should be enabled on processor boot */
	uint32_t config_flags_set;
	/* Config flags that should be disabled on processor boot */
	uint32_t config_flags_clear;
	/* Location of the boot vector */
	uint64_t boot_vec;
};

#endif /* AMP_TI_K3_H */
