/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NORDIC_COMMON_NRFX_GPPI_SD2PPI_GLOBAL_H_
#define SOC_NORDIC_COMMON_NRFX_GPPI_SD2PPI_GLOBAL_H_

#include <nrfx.h>
#include <helpers/nrfx_gppi.h>

typedef enum {
	NRFX_GPPI_NODE_DPPIC130,
	NRFX_GPPI_NODE_DPPIC131,
	NRFX_GPPI_NODE_DPPIC132,
	NRFX_GPPI_NODE_DPPIC133,
	NRFX_GPPI_NODE_DPPIC134,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_NODE_DPPIC135,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_NODE_DPPIC136,))
	NRFX_GPPI_NODE_DPPIC120,
	NRFX_GPPI_NODE_DPPI_COUNT,

	NRFX_GPPI_NODE_PPIB130_132 = NRFX_GPPI_NODE_DPPI_COUNT,
	NRFX_GPPI_NODE_PPIB130_133,
	NRFX_GPPI_NODE_PPIB130_134,
	NRFX_GPPI_NODE_PPIB130_135,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_NODE_PPIB131_136,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_NODE_PPIB131_137,))
	NRFX_GPPI_NODE_PPIB131_121,
	NRFX_GPPI_NODE_COUNT,
} nrfx_gppi_node_id_t;

const nrfx_gppi_route_t ***nrfx_gppi_route_map_get(void);
const nrfx_gppi_route_t *nrfx_gppi_routes_get(void);
const nrfx_gppi_node_t *nrfx_gppi_nodes_get(void);
void nrfx_gppi_channel_init(nrfx_gppi_node_id_t node_id, uint32_t ch_mask);
void nrfx_gppi_groups_init(nrfx_gppi_node_id_t node_id, uint32_t group_mask);

#endif /* SOC_NORDIC_COMMON_NRFX_GPPI_SD2PPI_GLOBAL_H_ */
