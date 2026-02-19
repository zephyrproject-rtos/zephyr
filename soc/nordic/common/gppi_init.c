/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nrfx.h>
#include <helpers/nrfx_gppi.h>
#if defined(NRFX_GPPI_MULTI_DOMAIN) && !defined(NRFX_GPPI_FIXED_CONNECTIONS)
#include <soc/interconnect/nrfx_gppi_lumos.h>
#elif defined(CONFIG_SOC_NRF54H20_CPUAPP)
#include <nrfx_gppi_nrf54h_global.h>
#elif defined(CONFIG_SOC_NRF54H20_CPURAD)
#include <nrfx_gppi_cpurad.h>
#endif

static int gppi_init(void)
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
#elif defined(CONFIG_SOC_NRF54H20_CPUAPP)
	gppi_instance.routes = nrfx_gppi_routes_get();
	gppi_instance.route_map = nrfx_gppi_route_map_get();
	gppi_instance.nodes = nrfx_gppi_nodes_get();

	/* PPI channels init. */
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC130,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic130), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC131,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic131), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC132,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic132), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC133,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic133), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC134,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic134), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC135,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic135), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC136,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic136), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC120,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic120), channels)));

	/* PPI groups init. */
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC130,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic130), groups)));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC131,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic131), groups)));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC132,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic132), groups)));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC133,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic133), groups)));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC134,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic134), groups)));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC135,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic135), groups)));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC136,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic136), groups)));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC120,
		BIT_MASK(DT_PROP(DT_NODELABEL(dppic120), groups)));

	/* PPIB channels init. */
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB130_132,
			BIT_MASK(DT_PROP(DT_NODELABEL(ppib132), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB130_133,
			BIT_MASK(DT_PROP(DT_NODELABEL(ppib133), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB130_134,
			BIT_MASK(DT_PROP(DT_NODELABEL(ppib134), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB130_135,
			BIT_MASK(DT_PROP(DT_NODELABEL(ppib135), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB131_136,
			BIT_MASK(DT_PROP(DT_NODELABEL(ppib136), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB131_137,
			BIT_MASK(DT_PROP(DT_NODELABEL(ppib137), channels)));
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB131_121,
			BIT_MASK(DT_PROP(DT_NODELABEL(ppib121), channels)));
#elif defined(CONFIG_SOC_NRF54H20_CPURAD)
	gppi_instance.routes = nrfx_gppi_routes_get();
	gppi_instance.route_map = nrfx_gppi_route_map_get();
	gppi_instance.nodes = nrfx_gppi_nodes_get();

	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC020,
			NRFX_BIT_MASK(DPPIC020_CH_NUM_SIZE) & ~NRFX_DPPI020_CHANNELS_USED);
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_DPPIC030,
			NRFX_BIT_MASK(DPPIC030_CH_NUM_SIZE) & ~NRFX_DPPI030_CHANNELS_USED);
	nrfx_gppi_channel_init(NRFX_GPPI_NODE_PPIB020_030,
			NRFX_BIT_MASK(PPIB020_NTASKSEVENTS_SIZE));
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC020,
			NRFX_BIT_MASK(DPPIC020_GROUP_NUM_SIZE) & ~NRFX_DPPI020_GROUPS_USED);
	nrfx_gppi_groups_init(NRFX_GPPI_NODE_DPPIC030,
			NRFX_BIT_MASK(DPPIC030_GROUP_NUM_SIZE) & ~NRFX_DPPI030_GROUPS_USED);
#else
#error "Not supported"
#endif
	nrfx_gppi_init(&gppi_instance);
	return 0;
}

#if defined(CONFIG_NRFX_GPPI) && !defined(CONFIG_NRFX_GPPI_V1)

/* For nrf54h20 GPPI requires that ironside communication is up so delay the initialization. */
#define GPPI_INIT_STATE COND_CODE_1(IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP), (POST_KERNEL), (EARLY))

#define GPPI_INIT_PRIO                                                          \
	COND_CODE_1(IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP),                     \
		    (UTIL_INC(CONFIG_IRONSIDE_SE_CALL_INIT_PRIORITY)), (0))

SYS_INIT(gppi_init, GPPI_INIT_STATE, GPPI_INIT_PRIO);
#endif
