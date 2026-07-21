/*
 * Copyright (c) 2022-2023 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RENESAS_RENESAS_CPG_MSSR_H_
#define ZEPHYR_DRIVERS_RENESAS_RENESAS_CPG_MSSR_H_

#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/device_mmio.h>

#define CPG_NUM_DOMAINS 2

struct cpg_clk_info_table {
	uint32_t domain;
	uint32_t module;
	mem_addr_t offset;
	uint32_t parent_id;

	int64_t in_freq;
	int64_t out_freq;

	/* TODO: add setting of this field and add function for getting status */
	enum clock_control_status status;

	struct cpg_clk_info_table *parent;
	struct cpg_clk_info_table *children_list;
	struct cpg_clk_info_table *next_sibling;
};

struct rcar_cpg_mssr_data {
	DEVICE_MMIO_RAM; /* Must be first */

	struct cpg_clk_info_table *clk_info_table[CPG_NUM_DOMAINS];
	const uint32_t clk_info_table_size[CPG_NUM_DOMAINS];

	struct k_spinlock lock;

	uint32_t (*get_div_helper)(uint32_t reg, uint32_t module);
	int (*set_rate_helper)(uint32_t module, uint32_t *div, uint32_t *div_mask);
};

#define RCAR_CPG_NONE -1
#define RCAR_CPG_KHZ(khz) ((khz) * 1000U)
#define RCAR_CPG_MHZ(mhz) (RCAR_CPG_KHZ(mhz) * 1000U)

#define RCAR_CORE_CLK_INFO_ITEM(id, off, par_id, in_frq)	\
	{							\
		.domain		= CPG_CORE,			\
		.module		= id,				\
		.offset		= off,				\
		.parent_id	= par_id,			\
		.in_freq	= in_frq,			\
		.out_freq	= RCAR_CPG_NONE,		\
		.status		= CLOCK_CONTROL_STATUS_UNKNOWN,	\
		.parent		= NULL,				\
		.children_list	= NULL,				\
		.next_sibling	= NULL,				\
	}

#define RCAR_MOD_CLK_INFO_ITEM(id, par_id)			\
	{							\
		.domain		= CPG_MOD,			\
		.module		= id,				\
		.offset		= RCAR_CPG_NONE,		\
		.parent_id	= par_id,			\
		.in_freq	= RCAR_CPG_NONE,		\
		.out_freq	= RCAR_CPG_NONE,		\
		.status		= CLOCK_CONTROL_STATUS_UNKNOWN,	\
		.parent		= NULL,				\
		.children_list	= NULL,				\
		.next_sibling	= NULL,				\
	}

#ifdef CONFIG_SOC_SERIES_RCAR_GEN3
/* Software Reset Clearing Register offsets */
#define SRSTCLR(i)      (0x940 + (i) * 4)

/* CPG write protect offset */
#define CPGWPR          0x900

/* Realtime Module Stop Control Register offsets */
static const uint16_t mstpcr[] = {
	0x110, 0x114, 0x118, 0x11c,
	0x120, 0x124, 0x128, 0x12c,
	0x980, 0x984, 0x988, 0x98c,
};

/* Software Reset Register offsets */
static const uint16_t srcr[] = {
	0x0A0, 0x0A8, 0x0B0, 0x0B8,
	0x0BC, 0x0C4, 0x1C8, 0x1CC,
	0x920, 0x924, 0x928, 0x92C,
};
#elif defined(CONFIG_SOC_SERIES_RCAR_GEN4)
/* Software Reset Clearing Register offsets */
#define SRSTCLR(i) (0x2C80 + (i) * 4)

/* CPG write protect offset */
#define CPGWPR      0x0

/* Realtime Module Stop Control Register offsets */
static const uint16_t mstpcr[] = {
	0x2D00, 0x2D04, 0x2D08, 0x2D0C,
	0x2D10, 0x2D14, 0x2D18, 0x2D1C,
	0x2D20, 0x2D24, 0x2D28, 0x2D2C,
	0x2D30, 0x2D34, 0x2D38, 0x2D3C,
	0x2D40, 0x2D44, 0x2D48, 0x2D4C,
	0x2D50, 0x2D54, 0x2D58, 0x2D5C,
	0x2D60, 0x2D64, 0x2D68, 0x2D6C,
};

/* Software Reset Register offsets */
static const uint16_t srcr[] = {
	0x2C00, 0x2C04, 0x2C08, 0x2C0C,
	0x2C10, 0x2C14, 0x2C18, 0x2C1C,
	0x2C20, 0x2C24, 0x2C28, 0x2C2C,
	0x2C30, 0x2C34, 0x2C38, 0x2C3C,
	0x2C40, 0x2C44, 0x2C48, 0x2C4C,
	0x2C50, 0x2C54, 0x2C58, 0x2C5C,
	0x2C60, 0x2C64, 0x2C68, 0x2C6C,
};
#endif /* CONFIG_SOC_SERIES_RCAR_GEN3 */

void rcar_cpg_write(uint32_t base_address, uint32_t reg, uint32_t val);

int rcar_cpg_mstp_clock_endisable(uint32_t base_address, uint32_t module, bool enable);

struct cpg_clk_info_table *rcar_cpg_find_clk_info_by_module_id(const struct device *dev,
							       uint32_t domain,
							       uint32_t id);

void rcar_cpg_build_clock_relationship(const struct device *dev);

void rcar_cpg_update_all_in_out_freq(const struct device *dev);

int rcar_cpg_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate);

int rcar_cpg_set_rate(const struct device *dev, clock_control_subsys_t sys,
		      clock_control_subsys_rate_t rate);

#endif /* ZEPHYR_DRIVERS_RENESAS_RENESAS_CPG_MSSR_H_ */
