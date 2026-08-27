/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_system, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define SYSTEM_MAX_ID		31

#define SYSTEM_MAX_NAME_SZ	32

#define to_clk_system(ptr) CONTAINER_OF(ptr, struct clk_system, clk)

struct clk_system {
	struct device clk;
	const struct device *parent;
	pmc_registers_t *pmc;
	uint8_t id;
};

static struct clk_system clocks_sys[SOC_NUM_CLOCK_SYSTEM];
static uint32_t clocks_sys_idx;

static inline int is_pck(int id)
{
	return (id >= 8) && (id <= 15);
}

static inline bool clk_system_ready(pmc_registers_t *pmc, int id)
{
	uint32_t status = pmc->PMC_SR;

	return !!(status & (1 << id));
}

static int clk_system_on(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);

	struct clk_system *sys = to_clk_system(dev);

	sys->pmc->PMC_SCER = 1 << sys->id;

	if (!is_pck(sys->id)) {
		return 0;
	}

	while (!clk_system_ready(sys->pmc, sys->id)) {
		k_busy_wait(1);
	}

	return 0;
}

static int clk_system_off(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);

	struct clk_system *sys = to_clk_system(dev);

	sys->pmc->PMC_SCDR = 1 << sys->id;

	return 0;
}

static int clk_system_get_rate(const struct device *dev,
			       clock_control_subsys_t subsys, uint32_t *rate)
{
	ARG_UNUSED(subsys);

	struct clk_system *sys = to_clk_system(dev);

	return clock_control_get_rate(sys->parent, NULL, rate);
}

static enum clock_control_status clk_system_get_status(const struct device *dev,
						       clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);

	struct clk_system *sys = to_clk_system(dev);
	uint32_t status;

	status = sys->pmc->PMC_SCSR;

	if (!(status & (1 << sys->id))) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	if (!is_pck(sys->id)) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	status = sys->pmc->PMC_SR;

	if (!!(status & (1 << sys->id))) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static DEVICE_API(clock_control, system_api) = {
	.on = clk_system_on,
	.off = clk_system_off,
	.get_rate = clk_system_get_rate,
	.get_status = clk_system_get_status,
};

int clk_register_system(pmc_registers_t *const pmc, const char *name,
			const struct device *parent,
			uint8_t id, struct device **clk)
{
	struct clk_system *sys;

	if (!parent || id > SYSTEM_MAX_ID) {
		return -EINVAL;
	}

	sys = &clocks_sys[clocks_sys_idx++];
	if (clocks_sys_idx > ARRAY_SIZE(clocks_sys)) {
		LOG_ERR("Array for system clock not enough");
		return -ENOMEM;
	}

	*clk = &sys->clk;
	(*clk)->name = name;
	(*clk)->api = &system_api;
	sys->parent = parent;
	sys->id = id;
	sys->pmc = pmc;

	return 0;
}
