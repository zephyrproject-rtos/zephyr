/*
 * Copyright (c) 2022 IoT.bzh
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

/* CAN-FD Clock Frequency Control Register */
#define CANFDCKCR                 0x244

/* Clock stop bit */
#define CANFDCKCR_CKSTP           BIT(8)

/* CANFD Clock */
#define CANFDCKCR_PARENT_CLK_RATE 800000000
#define CANFDCKCR_DIVIDER_MASK    0x1FF

/* Peripherals Clocks */
#define S3D4_CLK_RATE             66600000	/* SCIF	*/
#define S0D12_CLK_RATE            66600000	/* PWM	*/
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
