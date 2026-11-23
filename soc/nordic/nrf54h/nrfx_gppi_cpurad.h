/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NORDIC_NRF54H_NRFX_GPPI_CPURAD_H_
#define SOC_NORDIC_NRF54H_NRFX_GPPI_CPURAD_H_

#include <helpers/nrfx_gppi_routes.h>

typedef enum {
	NRFX_GPPI_NODE_DPPIC020,
	NRFX_GPPI_NODE_DPPIC030,
	NRFX_GPPI_NODE_DPPI_COUNT,
	NRFX_GPPI_NODE_PPIB020_030 = NRFX_GPPI_NODE_DPPI_COUNT,
	NRFX_GPPI_NODE_COUNT
} nrfx_gppi_node_id_t;

const nrfx_gppi_route_t ***nrfx_gppi_route_map_get(void);
const nrfx_gppi_route_t *nrfx_gppi_routes_get(void);
const nrfx_gppi_node_t *nrfx_gppi_nodes_get(void);
void nrfx_gppi_channel_init(nrfx_gppi_node_id_t node_id, uint32_t ch_mask);
void nrfx_gppi_groups_init(nrfx_gppi_node_id_t node_id, uint32_t group_mask);

#endif /* SOC_NORDIC_NRF54H_NRFX_GPPI_CPURAD_H_ */
