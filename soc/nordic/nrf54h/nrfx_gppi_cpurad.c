/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "nrfx_gppi_cpurad.h"

static nrfx_atomic_t channels[NRFX_GPPI_NODE_COUNT];
static nrfx_atomic_t group_channels[NRFX_GPPI_NODE_DPPI_COUNT];

static const nrfx_gppi_node_t nodes[] = {
	NRFX_GPPI_DPPI_NODE_DEFINE(020, NRFX_GPPI_NODE_DPPIC020),
	NRFX_GPPI_DPPI_NODE_DEFINE(030, NRFX_GPPI_NODE_DPPIC030),
	NRFX_GPPI_PPIB_NODE_DEFINE(020, 030),
};

static const nrfx_gppi_route_t dppi_routes[] = {
	NRFX_GPPI_ROUTE_DEFINE("slow_rad", (&nodes[NRFX_GPPI_NODE_DPPIC020])),
	NRFX_GPPI_ROUTE_DEFINE("fast_rad", (&nodes[NRFX_GPPI_NODE_DPPIC030])),
	NRFX_GPPI_ROUTE_DEFINE("slow_fast_rad",
			(&nodes[NRFX_GPPI_NODE_DPPIC020],
			 &nodes[NRFX_GPPI_NODE_PPIB020_030],
			 &nodes[NRFX_GPPI_NODE_DPPIC030])),
};

static const nrfx_gppi_route_t *slow_rad_routes[] = {
	&dppi_routes[0], &dppi_routes[2]
};

static const nrfx_gppi_route_t *fast_rad_routes[] = {
	&dppi_routes[1]
};

static const nrfx_gppi_route_t **dppi_route_map[] = {
	slow_rad_routes, fast_rad_routes
};

uint32_t nrfx_gppi_domain_id_get(uint32_t addr)
{
	uint32_t domain = (addr >> 24) & BIT_MASK(3);
	uint32_t bus = (addr >> 16) & BIT_MASK(8);

	(void)domain;
	__ASSERT_NO_MSG(domain == 3);
	switch (bus) {
	case 2:
		return NRFX_GPPI_NODE_DPPIC020;
	case 3:
		return NRFX_GPPI_NODE_DPPIC030;
	default:
		__ASSERT_NO_MSG(0);
		return 0;
	}
}

const nrfx_gppi_route_t ***nrfx_gppi_route_map_get(void)
{
	return dppi_route_map;
}

const nrfx_gppi_route_t *nrfx_gppi_routes_get(void)
{
	return dppi_routes;
}

const nrfx_gppi_node_t *nrfx_gppi_nodes_get(void)
{
	return nodes;
}

void nrfx_gppi_channel_init(nrfx_gppi_node_id_t node_id, uint32_t ch_mask)
{
	NRFX_ASSERT(node_id < NRFX_GPPI_NODE_COUNT);

	*nodes[node_id].generic.p_channels = ch_mask;
}

void nrfx_gppi_groups_init(nrfx_gppi_node_id_t node_id, uint32_t group_mask)
{
	NRFX_ASSERT(node_id < NRFX_GPPI_NODE_DPPI_COUNT);

	*nodes[node_id].dppi.p_group_channels = group_mask;
}
