/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "nrfx_gppi_nrf54h_global.h"
#include <ironside/se/api.h>

static nrfx_atomic_t channels[NRFX_GPPI_NODE_COUNT];
static nrfx_atomic_t group_channels[NRFX_GPPI_NODE_DPPI_COUNT];
static struct periphconf_entry write_entries[1];
static size_t write_entries_count;

#define PPIB_REG(id) (NRF_PPIB_Type *)DT_REG_ADDR(DT_NODELABEL(ppib##id))
#define PPIB_OFF(id) DT_PROP_OR(DT_NODELABEL(ppib##id), offset, 0)

#define NRFX_GPPI_PPIB_EXT_NODE_DEFINE(_id1, _id2)                                  \
[NRFX_GPPI_NODE_PPIB##_id1##_##_id2] = {                                            \
		.type = NRFX_GPPI_NODE_PPIB,                                        \
		.domain_id = NRFX_GPPI_NODE_PPIB##_id1##_##_id2,                    \
		.ch_off = {PPIB_OFF(_id2), 0},                                      \
		.ppib = {                                                           \
			.p_channels = &channels[NRFX_GPPI_NODE_PPIB##_id1##_##_id2],\
			.p_reg = {PPIB_REG(_id1), PPIB_REG(_id2)}                   \
		}                                                                   \
	}

static const nrfx_gppi_node_t nodes[] = {
	NRFX_GPPI_DPPI_NODE_DEFINE(130, NRFX_GPPI_NODE_DPPIC130),
	NRFX_GPPI_DPPI_NODE_DEFINE(131, NRFX_GPPI_NODE_DPPIC131),
	NRFX_GPPI_DPPI_NODE_DEFINE(132, NRFX_GPPI_NODE_DPPIC132),
	NRFX_GPPI_DPPI_NODE_DEFINE(133, NRFX_GPPI_NODE_DPPIC133),
	NRFX_GPPI_DPPI_NODE_DEFINE(134, NRFX_GPPI_NODE_DPPIC134),
	NRFX_GPPI_DPPI_NODE_DEFINE(135, NRFX_GPPI_NODE_DPPIC135),
	NRFX_GPPI_DPPI_NODE_DEFINE(136, NRFX_GPPI_NODE_DPPIC136),
	NRFX_GPPI_DPPI_NODE_DEFINE(120, NRFX_GPPI_NODE_DPPIC120),
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 132),
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 133),
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 134),
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 135),
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(131, 136),
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(131, 137),
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(131, 121),
};

static const nrfx_gppi_route_t routes[] = {
	/* 0 */NRFX_GPPI_ROUTE_DEFINE("apb32",
			(&nodes[NRFX_GPPI_NODE_DPPIC130])),
	/* 1 */NRFX_GPPI_ROUTE_DEFINE("apb38",
			(&nodes[NRFX_GPPI_NODE_DPPIC131])),
	/* 2 */NRFX_GPPI_ROUTE_DEFINE("apb39",
			(&nodes[NRFX_GPPI_NODE_DPPIC132])),
	/* 3 */NRFX_GPPI_ROUTE_DEFINE("apb3a",
			(&nodes[NRFX_GPPI_NODE_DPPIC133])),
	/* 4 */NRFX_GPPI_ROUTE_DEFINE("apb3b",
			(&nodes[NRFX_GPPI_NODE_DPPIC134])),
	/* 5 */NRFX_GPPI_ROUTE_DEFINE("apb3c",
			(&nodes[NRFX_GPPI_NODE_DPPIC135])),
	/* 6 */NRFX_GPPI_ROUTE_DEFINE("apb3d",
			(&nodes[NRFX_GPPI_NODE_DPPIC136])),
	/* 7 */NRFX_GPPI_ROUTE_DEFINE("apb22",
			(&nodes[NRFX_GPPI_NODE_DPPIC120])),

	/* 8 */NRFX_GPPI_ROUTE_DEFINE("apb32_apb38",
			(&nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_132],
			 &nodes[NRFX_GPPI_NODE_DPPIC131])),
	/* 9 */NRFX_GPPI_ROUTE_DEFINE("apb32_apb39",
			(&nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_133],
			 &nodes[NRFX_GPPI_NODE_DPPIC132])),
	/* 10 */NRFX_GPPI_ROUTE_DEFINE("apb32_apb3a",
			(&nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_134],
			 &nodes[NRFX_GPPI_NODE_DPPIC133])),
	/* 11 */NRFX_GPPI_ROUTE_DEFINE("apb32_apb3b",
			(&nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_135],
			 &nodes[NRFX_GPPI_NODE_DPPIC134])),
	/* 12 */NRFX_GPPI_ROUTE_DEFINE("apb32_apb3c",
			(&nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_136],
			 &nodes[NRFX_GPPI_NODE_DPPIC135])),
	/* 13 */NRFX_GPPI_ROUTE_DEFINE("apb32_apb3d",
			(&nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_137],
			 &nodes[NRFX_GPPI_NODE_DPPIC136])),
	/* 14 */NRFX_GPPI_ROUTE_DEFINE("apb32_apb22",
			(&nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_121],
			 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	/* 15 */NRFX_GPPI_ROUTE_DEFINE("apb38_apb39",
			(&nodes[NRFX_GPPI_NODE_DPPIC131],
			 &nodes[NRFX_GPPI_NODE_PPIB130_132],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_133],
			 &nodes[NRFX_GPPI_NODE_DPPIC132])),
	/* 16 */NRFX_GPPI_ROUTE_DEFINE("apb38_apb3a",
			(&nodes[NRFX_GPPI_NODE_DPPIC131],
			 &nodes[NRFX_GPPI_NODE_PPIB130_132],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_134],
			 &nodes[NRFX_GPPI_NODE_DPPIC133])),
	/* 17 */NRFX_GPPI_ROUTE_DEFINE("apb38_apb3b",
			(&nodes[NRFX_GPPI_NODE_DPPIC131],
			 &nodes[NRFX_GPPI_NODE_PPIB130_132],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_135],
			 &nodes[NRFX_GPPI_NODE_DPPIC134])),
	/* 18 */NRFX_GPPI_ROUTE_DEFINE("apb38_apb3c",
			(&nodes[NRFX_GPPI_NODE_DPPIC131],
			 &nodes[NRFX_GPPI_NODE_PPIB130_132],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_136],
			 &nodes[NRFX_GPPI_NODE_DPPIC135])),
	/* 19 */NRFX_GPPI_ROUTE_DEFINE("apb38_apb3d",
			(&nodes[NRFX_GPPI_NODE_DPPIC131],
			 &nodes[NRFX_GPPI_NODE_PPIB130_132],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_137],
			 &nodes[NRFX_GPPI_NODE_DPPIC136])),
	/* 20 */NRFX_GPPI_ROUTE_DEFINE("apb38_apb22",
			(&nodes[NRFX_GPPI_NODE_DPPIC131],
			 &nodes[NRFX_GPPI_NODE_PPIB130_132],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_121],
			 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	/* 21 */NRFX_GPPI_ROUTE_DEFINE("apb39_apb3a",
			(&nodes[NRFX_GPPI_NODE_DPPIC132],
			 &nodes[NRFX_GPPI_NODE_PPIB130_133],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_134],
			 &nodes[NRFX_GPPI_NODE_DPPIC133])),
	/* 22 */NRFX_GPPI_ROUTE_DEFINE("apb39_apb3b",
			(&nodes[NRFX_GPPI_NODE_DPPIC132],
			 &nodes[NRFX_GPPI_NODE_PPIB130_133],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_135],
			 &nodes[NRFX_GPPI_NODE_DPPIC134])),
	/* 23 */NRFX_GPPI_ROUTE_DEFINE("apb39_apb3c",
			(&nodes[NRFX_GPPI_NODE_DPPIC132],
			 &nodes[NRFX_GPPI_NODE_PPIB130_133],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_136],
			 &nodes[NRFX_GPPI_NODE_DPPIC135])),
	/* 24 */NRFX_GPPI_ROUTE_DEFINE("apb39_apb3d",
			(&nodes[NRFX_GPPI_NODE_DPPIC132],
			 &nodes[NRFX_GPPI_NODE_PPIB130_133],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_137],
			 &nodes[NRFX_GPPI_NODE_DPPIC136])),
	/* 25 */NRFX_GPPI_ROUTE_DEFINE("apb39_apb22",
			(&nodes[NRFX_GPPI_NODE_DPPIC132],
			 &nodes[NRFX_GPPI_NODE_PPIB130_133],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_121],
			 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	/* 26 */NRFX_GPPI_ROUTE_DEFINE("apb3a_apb3b",
			(&nodes[NRFX_GPPI_NODE_DPPIC133],
			 &nodes[NRFX_GPPI_NODE_PPIB130_134],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB130_135],
			 &nodes[NRFX_GPPI_NODE_DPPIC134])),
	/* 27 */NRFX_GPPI_ROUTE_DEFINE("apb3a_apb3c",
			(&nodes[NRFX_GPPI_NODE_DPPIC133],
			 &nodes[NRFX_GPPI_NODE_PPIB130_134],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_136],
			 &nodes[NRFX_GPPI_NODE_DPPIC135])),
	/* 28 */NRFX_GPPI_ROUTE_DEFINE("apb3a_apb3d",
			(&nodes[NRFX_GPPI_NODE_DPPIC133],
			 &nodes[NRFX_GPPI_NODE_PPIB130_134],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_137],
			 &nodes[NRFX_GPPI_NODE_DPPIC136])),
	/* 29 */NRFX_GPPI_ROUTE_DEFINE("apb3a_apb22",
			(&nodes[NRFX_GPPI_NODE_DPPIC133],
			 &nodes[NRFX_GPPI_NODE_PPIB130_134],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_121],
			 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	/* 30 */NRFX_GPPI_ROUTE_DEFINE("apb3b_apb3c",
			(&nodes[NRFX_GPPI_NODE_DPPIC134],
			 &nodes[NRFX_GPPI_NODE_PPIB130_135],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_136],
			 &nodes[NRFX_GPPI_NODE_DPPIC135])),
	/* 31 */NRFX_GPPI_ROUTE_DEFINE("apb3b_apb3d",
			(&nodes[NRFX_GPPI_NODE_DPPIC134],
			 &nodes[NRFX_GPPI_NODE_PPIB130_135],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_137],
			 &nodes[NRFX_GPPI_NODE_DPPIC136])),
	/* 32 */NRFX_GPPI_ROUTE_DEFINE("apb22_apb3b",
			(&nodes[NRFX_GPPI_NODE_DPPIC134],
			 &nodes[NRFX_GPPI_NODE_PPIB130_135],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_121],
			 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	/* 33 */NRFX_GPPI_ROUTE_DEFINE("apb3c_apb3d",
			(&nodes[NRFX_GPPI_NODE_DPPIC135],
			 &nodes[NRFX_GPPI_NODE_PPIB131_136],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_137],
			 &nodes[NRFX_GPPI_NODE_DPPIC136])),
	/* 34 */NRFX_GPPI_ROUTE_DEFINE("apb3c_apb22",
			(&nodes[NRFX_GPPI_NODE_DPPIC135],
			 &nodes[NRFX_GPPI_NODE_PPIB131_136],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_121],
			 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	/* 35 */NRFX_GPPI_ROUTE_DEFINE("apb3d_apb22",
			(&nodes[NRFX_GPPI_NODE_DPPIC136],
			 &nodes[NRFX_GPPI_NODE_PPIB131_137],
			 &nodes[NRFX_GPPI_NODE_DPPIC130],
			 &nodes[NRFX_GPPI_NODE_PPIB131_121],
			 &nodes[NRFX_GPPI_NODE_DPPIC120])),
};

static const nrfx_gppi_route_t *apb32_routes[] = {
	&routes[0], &routes[8], &routes[9], &routes[10], &routes[11],
	&routes[12], &routes[13], &routes[14]
};
static const nrfx_gppi_route_t *apb38_routes[] = {
	&routes[1], &routes[15], &routes[16], &routes[17], &routes[18], &routes[19], &routes[20]
};
static const nrfx_gppi_route_t *apb39_routes[] = {
	&routes[2], &routes[21], &routes[22], &routes[23], &routes[24], &routes[25]
};

static const nrfx_gppi_route_t *apb3a_routes[] = {
	&routes[3], &routes[26], &routes[27], &routes[28], &routes[29]
};

static const nrfx_gppi_route_t *apb3b_routes[] = {
	&routes[4], &routes[30], &routes[31], &routes[32]
};

static const nrfx_gppi_route_t *apb3c_routes[] = {
	&routes[5], &routes[33], &routes[34]
};

static const nrfx_gppi_route_t *apb3d_routes[] = {
	&routes[6], &routes[35]
};

static const nrfx_gppi_route_t *apb22_routes[] = {
	&routes[7]
};

static const nrfx_gppi_route_t **dppi_route_map[] = {
	apb32_routes, apb38_routes, apb39_routes, apb3a_routes,
	apb3b_routes, apb3c_routes, apb3d_routes, apb22_routes
};

uint32_t nrfx_gppi_domain_id_get(uint32_t addr)
{
	uint32_t apb = (addr >> 16) & 0xff;
	uint32_t domain = (addr >> 24) & 0xf;

	(void)domain;
	__ASSERT_NO_MSG(domain == 0xf);

	if (apb < 0x92) {
		return NRFX_GPPI_NODE_DPPIC120;
	} else if (apb <= 0x93) {
		return NRFX_GPPI_NODE_DPPIC130;
	}
	return apb - 0x98 + NRFX_GPPI_NODE_DPPIC131;
}

const nrfx_gppi_route_t ***nrfx_gppi_route_map_get(void)
{
	return dppi_route_map;
}

const nrfx_gppi_route_t *nrfx_gppi_routes_get(void)
{
	return routes;
}

const nrfx_gppi_node_t *nrfx_gppi_nodes_get(void)
{
	return nodes;
}

BUILD_ASSERT(SPU_FEATURE_DPPIC_CH_LOCK_Msk == SPU_FEATURE_DPPIC_CHG_LOCK_Msk);
BUILD_ASSERT(SPU_FEATURE_DPPIC_CH_OWNERID_Pos == SPU_FEATURE_DPPIC_CHG_OWNERID_Pos);

static bool check_spu_value(uint32_t val)
{
	if ((val >> SPU_FEATURE_DPPIC_CH_OWNERID_Pos) != NRF_OWNER) {
		return false;
	}

	return true;
}

static int actual_channel_mask(nrfx_gppi_node_id_t node_id, uint32_t *ch_mask, bool group)
{
	static const NRF_SPU_Type * spu_addr[] = {
		NRF_SPU131,
		NRF_SPU132,
		NRF_SPU133,
		NRF_SPU134,
		NRF_SPU135,
		NRF_SPU136,
		NRF_SPU137,
		NRF_SPU122,
	};
	uint32_t out_mask = 0;
	uint32_t in_mask = *ch_mask;
	struct periphconf_entry entries[IRONSIDE_SE_PERIPHCONF_INLINE_READ_MAX_COUNT];
	uint8_t entries_idx[IRONSIDE_SE_PERIPHCONF_INLINE_READ_MAX_COUNT];
	struct ironside_se_periphconf_status status;
	size_t entries_count = 0;

	while (in_mask) {
		uint32_t idx = 31 - __builtin_clz(in_mask);

		in_mask &= ~BIT(idx);
		/* Collect batch of requests. */
		entries_idx[entries_count] = idx;
		entries[entries_count].regptr = group ?
			PERIPHCONF_SPU_FEATURE_DPPIC_CHG_REGPTR(spu_addr[node_id], idx) :
			PERIPHCONF_SPU_FEATURE_DPPIC_CH_REGPTR(spu_addr[node_id], idx);
		entries_count++;
		if ((entries_count == ARRAY_SIZE(entries)) || (in_mask == 0)) {
			/* Read batch of registers. */
			status = ironside_se_periphconf_read(entries, entries_count);
			if (status.status == 0) {
				for (uint32_t i = 0; i < entries_count; i++) {
					/* If SPU register has default value assume that channel
					 * is unused and can be in the pool.
					 */
					if (check_spu_value(entries[i].value)) {
						out_mask |= BIT(entries_idx[i]);
					}
				}
				entries_count = 0;
			} else {
				return status.status;
			}
		}
	}

	*ch_mask = out_mask;

	return 0;
}

void nrfx_gppi_channel_init(nrfx_gppi_node_id_t node_id, uint32_t ch_mask)
{
	NRFX_ASSERT(node_id < NRFX_GPPI_NODE_COUNT);
	int rv;

	if (node_id < NRFX_GPPI_NODE_DPPI_COUNT) {
		rv = actual_channel_mask(node_id, &ch_mask, false);
		__ASSERT_NO_MSG(rv == 0);
		if (rv < 0) {
			return;
		}
	}

	*nodes[node_id].generic.p_channels = ch_mask;
}

void nrfx_gppi_groups_init(nrfx_gppi_node_id_t node_id, uint32_t group_mask)
{
	NRFX_ASSERT(node_id < NRFX_GPPI_NODE_DPPI_COUNT);
	int rv;

	rv = actual_channel_mask(node_id, &group_mask, true);
	__ASSERT_NO_MSG(rv == 0);
	if (rv < 0) {
		return;
	}

	*nodes[node_id].dppi.p_group_channels = group_mask;
}

int nrfx_gppi_ext_ppib_write(volatile uint32_t *p_addr, uint32_t value)
{
	struct ironside_se_periphconf_status status;

	if (p_addr != NULL) {
		write_entries[write_entries_count].regptr = (uint32_t)p_addr;
		write_entries[write_entries_count].value = value;
		write_entries_count++;
		if (write_entries_count < ARRAY_SIZE(write_entries)) {
			return 0;
		}
	}

	if (write_entries_count == 0) {
		return 0;
	}

	status = ironside_se_periphconf_write(write_entries, write_entries_count);
	write_entries_count = 0;

	return (status.status < 0) ? -EIO : 0;
}
