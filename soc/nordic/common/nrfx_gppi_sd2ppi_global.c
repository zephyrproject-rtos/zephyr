/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "nrfx_gppi_sd2ppi_global.h"
#include <ironside/se/api.h>

static nrfx_atomic_t channels[NRFX_GPPI_NODE_COUNT];
static nrfx_atomic_t group_channels[NRFX_GPPI_NODE_DPPI_COUNT];
static struct periphconf_entry write_entries[1];
static size_t write_entries_count;

#define PPIB_REG(id) (NRF_PPIB_Type *)DT_REG_ADDR(DT_NODELABEL(ppib##id))
#define PPIB_OFF(id) DT_PROP_OR(DT_NODELABEL(ppib##id), offset, 0)

#define _NRFX_GPPI_PPIB_EXT_NODE_DEFINE(_id1, _id2)						\
	[NRFX_GPPI_NODE_PPIB##_id1##_##_id2] = {						\
		.type = NRFX_GPPI_NODE_PPIB,							\
		.domain_id = NRFX_GPPI_NODE_PPIB##_id1##_##_id2,				\
		.ch_off = { PPIB_OFF(_id2), 0 },						\
		.ppib = {									\
			.p_channels = &channels[NRFX_GPPI_NODE_PPIB##_id1##_##_id2],		\
			.p_reg = { PPIB_REG(_id1), PPIB_REG(_id2) }				\
		}										\
	}

/** @brief Conditionally create PPIB node.
 *
 * Node is created if both ppib nodes exist. If node exists then comma is added so macro should not
 * be followed by the comma.
 *
 * @param _id1 PPIB node ID.
 * @param _id2 PPIB node ID.
 */
#define NRFX_GPPI_PPIB_EXT_NODE_DEFINE(_id1, _id2)						\
	IF_ENABLED(UTIL_AND(DT_NODE_EXISTS(DT_NODELABEL(ppib##_id1)),				\
			    DT_NODE_EXISTS(DT_NODELABEL(ppib##_id2))),				\
		    (_NRFX_GPPI_PPIB_EXT_NODE_DEFINE(_id1, _id2),))

/** @brief Conditionally create DPPI node.
 *
 * Node is created if dppic node exists. If node exists then comma is added so macro should not be
 * followed by the comma.
 */
#define DPPI_NODE_DEFINE(_node_id)								\
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic##_node_id)),				\
		    (NRFX_GPPI_DPPI_NODE_DEFINE(_node_id, NRFX_GPPI_NODE_DPPIC##_node_id),))

static const nrfx_gppi_node_t nodes[] = {
	/* DPPI nodes */
	DPPI_NODE_DEFINE(130)
	DPPI_NODE_DEFINE(131)
	DPPI_NODE_DEFINE(132)
	DPPI_NODE_DEFINE(133)
	DPPI_NODE_DEFINE(134)
	DPPI_NODE_DEFINE(135)
	DPPI_NODE_DEFINE(136)
	DPPI_NODE_DEFINE(120)

	/* PPIB nodes */
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 132)
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 133)
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 134)
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(130, 135)
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(131, 136)
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(131, 137)
	NRFX_GPPI_PPIB_EXT_NODE_DEFINE(131, 121)
};

enum nrfx_gppi_route_id {
	NRFX_GPPI_ROUTE_ID_130,
	NRFX_GPPI_ROUTE_ID_131,
	NRFX_GPPI_ROUTE_ID_132,
	NRFX_GPPI_ROUTE_ID_133,
	NRFX_GPPI_ROUTE_ID_134,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_135,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_ROUTE_ID_136,))
	NRFX_GPPI_ROUTE_ID_120,
	NRFX_GPPI_ROUTE_ID_130_131,
	NRFX_GPPI_ROUTE_ID_130_132,
	NRFX_GPPI_ROUTE_ID_130_133,
	NRFX_GPPI_ROUTE_ID_130_134,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_130_135,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_ROUTE_ID_130_136,))
	NRFX_GPPI_ROUTE_ID_130_120,
	NRFX_GPPI_ROUTE_ID_131_132,
	NRFX_GPPI_ROUTE_ID_131_133,
	NRFX_GPPI_ROUTE_ID_131_134,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_131_135,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_ROUTE_ID_131_136,))
	NRFX_GPPI_ROUTE_ID_131_120,
	NRFX_GPPI_ROUTE_ID_132_133,
	NRFX_GPPI_ROUTE_ID_132_134,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_132_135,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_ROUTE_ID_132_136,))
	NRFX_GPPI_ROUTE_ID_132_120,
	NRFX_GPPI_ROUTE_ID_133_134,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_133_135,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_ROUTE_ID_133_136,))
	NRFX_GPPI_ROUTE_ID_133_120,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_134_135,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_ROUTE_ID_134_136,))
	NRFX_GPPI_ROUTE_ID_134_120,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_135_136,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (NRFX_GPPI_ROUTE_ID_135_120,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (NRFX_GPPI_ROUTE_ID_136_120,))
	NRFX_GPPI_ROUTE_ID_COUNT,
};

static const nrfx_gppi_route_t routes[] = {
	[NRFX_GPPI_ROUTE_ID_130] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130", (&nodes[NRFX_GPPI_NODE_DPPIC130])),
	[NRFX_GPPI_ROUTE_ID_131] =
		NRFX_GPPI_ROUTE_DEFINE("dppi131", (&nodes[NRFX_GPPI_NODE_DPPIC131])),
	[NRFX_GPPI_ROUTE_ID_132] =
		NRFX_GPPI_ROUTE_DEFINE("dppi132", (&nodes[NRFX_GPPI_NODE_DPPIC132])),
	[NRFX_GPPI_ROUTE_ID_133] =
		NRFX_GPPI_ROUTE_DEFINE("dppi133", (&nodes[NRFX_GPPI_NODE_DPPIC133])),
	[NRFX_GPPI_ROUTE_ID_134] =
		NRFX_GPPI_ROUTE_DEFINE("dppi134", (&nodes[NRFX_GPPI_NODE_DPPIC134])),
#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_135] =
		NRFX_GPPI_ROUTE_DEFINE("dppi135", (&nodes[NRFX_GPPI_NODE_DPPIC135])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
	[NRFX_GPPI_ROUTE_ID_136] =
		NRFX_GPPI_ROUTE_DEFINE("dppi136", (&nodes[NRFX_GPPI_NODE_DPPIC136])),
#endif
	[NRFX_GPPI_ROUTE_ID_120] =
		NRFX_GPPI_ROUTE_DEFINE("dppi120", (&nodes[NRFX_GPPI_NODE_DPPIC120])),
	[NRFX_GPPI_ROUTE_ID_130_131] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130_dppi131", (&nodes[NRFX_GPPI_NODE_DPPIC130],
							   &nodes[NRFX_GPPI_NODE_PPIB130_132],
							   &nodes[NRFX_GPPI_NODE_DPPIC131])),
	[NRFX_GPPI_ROUTE_ID_130_132] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130_dppi132", (&nodes[NRFX_GPPI_NODE_DPPIC130],
							   &nodes[NRFX_GPPI_NODE_PPIB130_133],
							   &nodes[NRFX_GPPI_NODE_DPPIC132])),
	[NRFX_GPPI_ROUTE_ID_130_133] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130_dppi133", (&nodes[NRFX_GPPI_NODE_DPPIC130],
							   &nodes[NRFX_GPPI_NODE_PPIB130_134],
							   &nodes[NRFX_GPPI_NODE_DPPIC133])),
	[NRFX_GPPI_ROUTE_ID_130_134] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130_dppi134", (&nodes[NRFX_GPPI_NODE_DPPIC130],
							   &nodes[NRFX_GPPI_NODE_PPIB130_135],
							   &nodes[NRFX_GPPI_NODE_DPPIC134])),
#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_130_135] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130_dppi135", (&nodes[NRFX_GPPI_NODE_DPPIC130],
							   &nodes[NRFX_GPPI_NODE_PPIB131_136],
							   &nodes[NRFX_GPPI_NODE_DPPIC135])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
	[NRFX_GPPI_ROUTE_ID_130_136] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130_dppi136", (&nodes[NRFX_GPPI_NODE_DPPIC130],
							   &nodes[NRFX_GPPI_NODE_PPIB131_137],
							   &nodes[NRFX_GPPI_NODE_DPPIC136])),
#endif
	[NRFX_GPPI_ROUTE_ID_130_120] =
		NRFX_GPPI_ROUTE_DEFINE("dppi130_dppi120", (&nodes[NRFX_GPPI_NODE_DPPIC130],
							   &nodes[NRFX_GPPI_NODE_PPIB131_121],
							   &nodes[NRFX_GPPI_NODE_DPPIC120])),

	[NRFX_GPPI_ROUTE_ID_131_132] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi131_dppi132",
		(&nodes[NRFX_GPPI_NODE_DPPIC131], &nodes[NRFX_GPPI_NODE_PPIB130_132],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB130_133],
		 &nodes[NRFX_GPPI_NODE_DPPIC132])),
	[NRFX_GPPI_ROUTE_ID_131_133] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi131_dppi133",
		(&nodes[NRFX_GPPI_NODE_DPPIC131], &nodes[NRFX_GPPI_NODE_PPIB130_132],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB130_134],
		 &nodes[NRFX_GPPI_NODE_DPPIC133])),
	[NRFX_GPPI_ROUTE_ID_131_134] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi131_dppi134",
		(&nodes[NRFX_GPPI_NODE_DPPIC131], &nodes[NRFX_GPPI_NODE_PPIB130_132],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB130_135],
		 &nodes[NRFX_GPPI_NODE_DPPIC134])),
#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_131_135] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi131_dppi135",
		(&nodes[NRFX_GPPI_NODE_DPPIC131], &nodes[NRFX_GPPI_NODE_PPIB130_132],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_136],
		 &nodes[NRFX_GPPI_NODE_DPPIC135])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
	[NRFX_GPPI_ROUTE_ID_131_136] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi131_dppi136",
		(&nodes[NRFX_GPPI_NODE_DPPIC131], &nodes[NRFX_GPPI_NODE_PPIB130_132],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_137],
		 &nodes[NRFX_GPPI_NODE_DPPIC136])),
#endif
	[NRFX_GPPI_ROUTE_ID_131_120] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi131_dppi120",
		(&nodes[NRFX_GPPI_NODE_DPPIC131], &nodes[NRFX_GPPI_NODE_PPIB130_132],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_121],
		 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	[NRFX_GPPI_ROUTE_ID_132_133] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi132_dppi133",
		(&nodes[NRFX_GPPI_NODE_DPPIC132], &nodes[NRFX_GPPI_NODE_PPIB130_133],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB130_134],
		 &nodes[NRFX_GPPI_NODE_DPPIC133])),
	[NRFX_GPPI_ROUTE_ID_132_134] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi132_dppi134",
		(&nodes[NRFX_GPPI_NODE_DPPIC132], &nodes[NRFX_GPPI_NODE_PPIB130_133],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB130_135],
		 &nodes[NRFX_GPPI_NODE_DPPIC134])),
#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_132_135] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi132_dppi135",
		(&nodes[NRFX_GPPI_NODE_DPPIC132], &nodes[NRFX_GPPI_NODE_PPIB130_133],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_136],
		 &nodes[NRFX_GPPI_NODE_DPPIC135])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
	[NRFX_GPPI_ROUTE_ID_132_136] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi132_dppi136",
		(&nodes[NRFX_GPPI_NODE_DPPIC132], &nodes[NRFX_GPPI_NODE_PPIB130_133],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_137],
		 &nodes[NRFX_GPPI_NODE_DPPIC136])),
#endif
	[NRFX_GPPI_ROUTE_ID_132_120] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi132_dppi120",
		(&nodes[NRFX_GPPI_NODE_DPPIC132], &nodes[NRFX_GPPI_NODE_PPIB130_133],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_121],
		 &nodes[NRFX_GPPI_NODE_DPPIC120])),

	[NRFX_GPPI_ROUTE_ID_133_134] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi133_dppi134",
		(&nodes[NRFX_GPPI_NODE_DPPIC133], &nodes[NRFX_GPPI_NODE_PPIB130_134],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB130_135],
		 &nodes[NRFX_GPPI_NODE_DPPIC134])),
#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_133_135] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi133_dppi135",
		(&nodes[NRFX_GPPI_NODE_DPPIC133], &nodes[NRFX_GPPI_NODE_PPIB130_134],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_136],
		 &nodes[NRFX_GPPI_NODE_DPPIC135])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
	[NRFX_GPPI_ROUTE_ID_133_136] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi133_dppi136",
		(&nodes[NRFX_GPPI_NODE_DPPIC133], &nodes[NRFX_GPPI_NODE_PPIB130_134],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_137],
		 &nodes[NRFX_GPPI_NODE_DPPIC136])),
#endif
	[NRFX_GPPI_ROUTE_ID_133_120] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi133_dppi120",
		(&nodes[NRFX_GPPI_NODE_DPPIC133], &nodes[NRFX_GPPI_NODE_PPIB130_134],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_121],
		 &nodes[NRFX_GPPI_NODE_DPPIC120])),
#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_134_135] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi134_dppi135",
		(&nodes[NRFX_GPPI_NODE_DPPIC134], &nodes[NRFX_GPPI_NODE_PPIB130_135],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_136],
		 &nodes[NRFX_GPPI_NODE_DPPIC135])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
	[NRFX_GPPI_ROUTE_ID_134_136] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi134_dppi136",
		(&nodes[NRFX_GPPI_NODE_DPPIC134], &nodes[NRFX_GPPI_NODE_PPIB130_135],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_137],
		 &nodes[NRFX_GPPI_NODE_DPPIC136])),
#endif
	[NRFX_GPPI_ROUTE_ID_134_120] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi134_dppi120",
		(&nodes[NRFX_GPPI_NODE_DPPIC134], &nodes[NRFX_GPPI_NODE_PPIB130_135],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_121],
		 &nodes[NRFX_GPPI_NODE_DPPIC120])),

#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_135_136] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi135_dppi136",
		(&nodes[NRFX_GPPI_NODE_DPPIC135], &nodes[NRFX_GPPI_NODE_PPIB131_136],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_137],
		 &nodes[NRFX_GPPI_NODE_DPPIC136])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
	[NRFX_GPPI_ROUTE_ID_135_120] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi135_dppi120",
		(&nodes[NRFX_GPPI_NODE_DPPIC135], &nodes[NRFX_GPPI_NODE_PPIB131_136],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_121],
		 &nodes[NRFX_GPPI_NODE_DPPIC120])),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
	[NRFX_GPPI_ROUTE_ID_136_120] = NRFX_GPPI_ROUTE_DEFINE(
		"dppi136_dppi120",
		(&nodes[NRFX_GPPI_NODE_DPPIC136], &nodes[NRFX_GPPI_NODE_PPIB131_137],
		 &nodes[NRFX_GPPI_NODE_DPPIC130], &nodes[NRFX_GPPI_NODE_PPIB131_121],
		 &nodes[NRFX_GPPI_NODE_DPPIC120])),
#endif
};

static const nrfx_gppi_route_t *dppi130_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_130],
	&routes[NRFX_GPPI_ROUTE_ID_130_131],
	&routes[NRFX_GPPI_ROUTE_ID_130_132],
	&routes[NRFX_GPPI_ROUTE_ID_130_133],
	&routes[NRFX_GPPI_ROUTE_ID_130_134],
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (&routes[NRFX_GPPI_ROUTE_ID_130_135],))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (&routes[NRFX_GPPI_ROUTE_ID_130_136],))
	&routes[NRFX_GPPI_ROUTE_ID_130_120]
};

static const nrfx_gppi_route_t *dppi131_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_131],
	&routes[NRFX_GPPI_ROUTE_ID_131_132],
	&routes[NRFX_GPPI_ROUTE_ID_131_133],
	&routes[NRFX_GPPI_ROUTE_ID_131_134],
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (&routes[NRFX_GPPI_ROUTE_ID_131_135],))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (&routes[NRFX_GPPI_ROUTE_ID_131_136],))
	&routes[NRFX_GPPI_ROUTE_ID_131_120]
};

static const nrfx_gppi_route_t *dppi132_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_132],
	&routes[NRFX_GPPI_ROUTE_ID_132_133],
	&routes[NRFX_GPPI_ROUTE_ID_132_134],
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (&routes[NRFX_GPPI_ROUTE_ID_132_135],))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (&routes[NRFX_GPPI_ROUTE_ID_132_136],))
	&routes[NRFX_GPPI_ROUTE_ID_132_120]
};

static const nrfx_gppi_route_t *dppi133_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_133],
	&routes[NRFX_GPPI_ROUTE_ID_133_134],
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (&routes[NRFX_GPPI_ROUTE_ID_133_135],))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (&routes[NRFX_GPPI_ROUTE_ID_133_136],))
	&routes[NRFX_GPPI_ROUTE_ID_133_120]
};

static const nrfx_gppi_route_t *dppi134_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_134],
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (&routes[NRFX_GPPI_ROUTE_ID_134_135],))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (&routes[NRFX_GPPI_ROUTE_ID_134_136],))
	&routes[NRFX_GPPI_ROUTE_ID_134_120]
};

#if DT_NODE_EXISTS(DT_NODELABEL(dppic135))
static const nrfx_gppi_route_t *dppi135_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_135],
	&routes[NRFX_GPPI_ROUTE_ID_135_136],
	&routes[NRFX_GPPI_ROUTE_ID_135_120]
};
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(dppic136))
static const nrfx_gppi_route_t *dppi136_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_136],
	&routes[NRFX_GPPI_ROUTE_ID_136_120]
};
#endif

static const nrfx_gppi_route_t *dppi120_routes[] = {
	&routes[NRFX_GPPI_ROUTE_ID_120]
};

static const nrfx_gppi_route_t **dppi_route_map[] = {
	dppi130_routes,
	dppi131_routes,
	dppi132_routes,
	dppi133_routes,
	dppi134_routes,
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic135)), (dppi135_routes,))
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dppic136)), (dppi136_routes,))
	dppi120_routes
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
		IF_ENABLED(NRFX_INSTANCE_PRESENT(SPU136), (NRF_SPU136,))
		IF_ENABLED(NRFX_INSTANCE_PRESENT(SPU137), (NRF_SPU137,))
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
