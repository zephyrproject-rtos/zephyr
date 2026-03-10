/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nrfx.h>
#include <helpers/nrfx_gppi.h>
#if defined(NRFX_GPPI_MULTI_DOMAIN) && !defined(NRFX_GPPI_FIXED_CONNECTIONS)
#include <soc/interconnect/nrfx_gppi_lumos.h>
#endif

static int _gppi_init(void)
{
	static nrfx_gppi_t gppi_instance;

#if defined(PPI_PRESENT)
	gppi_instance.ch_mask = BIT_MASK(PPI_CH_NUM) & ~NRFX_PPI_CHANNELS_USED;
	gppi_instance.group_mask = BIT_MASK(PPI_GROUP_NUM) & ~NRFX_PPI_GROUPS_USED;
#elif defined(DPPIC_PRESENT) && !defined(NRFX_GPPI_MULTI_DOMAIN)
	uint32_t ch_mask = (DPPIC_CH_NUM == 32) ? UINT32_MAX : BIT_MASK(DPPIC_CH_NUM);

	gppi_instance.ch_mask = ch_mask & ~NRFX_DPPI_CHANNELS_USED;
	gppi_instance.group_mask = BIT_MASK(DPPIC_GROUP_NUM) & ~NRFX_DPPI_GROUPS_USED;
#elif defined(NRFX_GPPI_MULTI_DOMAIN) && !defined(NRFX_GPPI_FIXED_CONNECTIONS)
	gppi_instance.routes = nrfx_gppi_routes_get();
	gppi_instance.route_map = nrfx_gppi_route_map_get();
	gppi_instance.nodes = nrfx_gppi_nodes_get();

	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC00,
			NRFX_BIT_MASK(DPPIC00_CH_NUM_SIZE) & ~NRFX_DPPI00_CHANNELS_USED);
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC10,
			NRFX_BIT_MASK(DPPIC10_CH_NUM_SIZE) & ~NRFX_DPPI10_CHANNELS_USED);
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC20,
			NRFX_BIT_MASK(DPPIC20_CH_NUM_SIZE) & ~NRFX_DPPI20_CHANNELS_USED);
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC30,
			NRFX_BIT_MASK(DPPIC30_CH_NUM_SIZE) & ~NRFX_DPPI30_CHANNELS_USED);
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB00_10, NRFX_BIT_MASK(PPIB10_NTASKSEVENTS_SIZE));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB11_21, NRFX_BIT_MASK(PPIB11_NTASKSEVENTS_SIZE));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB01_20, NRFX_BIT_MASK(PPIB01_NTASKSEVENTS_SIZE));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB22_30, NRFX_BIT_MASK(PPIB22_NTASKSEVENTS_SIZE));

	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC00,
			NRFX_BIT_MASK(DPPIC00_GROUP_NUM_SIZE) & ~NRFX_DPPI00_GROUPS_USED);
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC10,
			NRFX_BIT_MASK(DPPIC10_GROUP_NUM_SIZE) & ~NRFX_DPPI10_GROUPS_USED);
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC20,
			NRFX_BIT_MASK(DPPIC20_GROUP_NUM_SIZE) & ~NRFX_DPPI20_GROUPS_USED);
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC30,
			NRFX_BIT_MASK(DPPIC30_GROUP_NUM_SIZE) & ~NRFX_DPPI30_GROUPS_USED);
#else
#error "Not supported"
#endif
	nrfx_gppi_init(&gppi_instance);
	return 0;
}

#if defined(CONFIG_NRFX_GPPI) && !defined(CONFIG_NRFX_GPPI_V1)
SYS_INIT(_gppi_init, EARLY, 0);
#endif
