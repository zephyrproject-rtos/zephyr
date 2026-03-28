/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_prog, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define PROG_ID_MAX		7

#define PROG_PRES(layout, pckr)	((pckr >> layout->pres_shift) & layout->pres_mask)

struct clk_programmable {
	struct device clk;
	struct device *parents[9];
	pmc_registers_t *pmc;
	uint32_t *mux_table;
	uint8_t id;
	uint8_t num_parents;
	uint8_t parent;
	const struct clk_programmable_layout *layout;
	char name[8];
};

#define to_clk_programmable(ptr) CONTAINER_OF(ptr, struct clk_programmable, clk)

static struct clk_programmable clocks_prog[8];
static uint32_t clocks_prog_idx;

static int clk_programmable_get_rate(const struct device *dev,
				     clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_programmable *prog = to_clk_programmable(dev);
	const struct clk_programmable_layout *layout = prog->layout;
	const struct device *parent = NULL;
	uint32_t pckr;
	uint32_t css;
	uint32_t i;
	int ret;

	pckr = prog->pmc->PMC_PCK[prog->id];
	css = pckr & PMC_PCK_CSS_Msk;

	for (i = 0; i < prog->num_parents; i++) {
		if (css == prog->mux_table[i]) {
			parent = prog->parents[i];
			break;
		}
	}
	if (!parent) {
		LOG_ERR("find parent clock failed.");
		return -ENXIO;
	}

	ret = clock_control_get_rate(parent, NULL, rate);
	if (ret) {
		LOG_ERR("get parent clock rate failed.");
		return ret;
	}

	if (layout->is_pres_direct) {
		*rate = *rate / (PROG_PRES(layout, pckr) + 1);
	} else {
		*rate = *rate >> PROG_PRES(layout, pckr);

	}

	return 0;
}

static DEVICE_API(clock_control, programmable_api) = {
	.get_rate = clk_programmable_get_rate,
};

int clk_register_programmable(pmc_registers_t *const pmc,
			      const struct device **parents,
			      uint8_t num_parents, uint8_t id,
			      const struct clk_programmable_layout *layout,
			      uint32_t *mux_table, struct device **clk)
{
	struct clk_programmable *prog;

	if (id > PROG_ID_MAX || !parents) {
		return -EINVAL;
	}

	prog = &clocks_prog[clocks_prog_idx++];
	if (clocks_prog_idx > ARRAY_SIZE(clocks_prog)) {
		LOG_ERR("Array for programmable clock not enough");
		return -ENOMEM;
	}
	if (num_parents > ARRAY_SIZE(prog->parents)) {
		LOG_ERR("Array for parent clock not enough, at least %d", num_parents);
		return -ENOMEM;
	}

	memcpy(prog->parents, parents, sizeof(struct device *) * num_parents);
	snprintf(prog->name, sizeof(prog->name), "prog%d", id);

	*clk = &prog->clk;
	(*clk)->name = prog->name;
	(*clk)->api = &programmable_api;
	prog->num_parents = num_parents;
	prog->id = id;
	prog->layout = layout;
	prog->pmc = pmc;
	prog->mux_table = mux_table;

	return 0;
}
