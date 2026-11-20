/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Route BCM2712 MIP channels to the GIC using their devicetree maps.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/map.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/interrupt_controller/intc_rp1.h>
#include <zephyr/drivers/interrupt_controller/msi_proxy.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "soc_irq.h"

#define DT_DRV_COMPAT brcm_bcm2712_mip

LOG_MODULE_REGISTER(soc_irq, LOG_LEVEL_ERR);

enum irq_route_state {
	IRQ_ROUTE_NONE,
	IRQ_ROUTE_READY,
	IRQ_ROUTE_INVALID,
};

struct irq_route {
	const struct device *mip;
	const struct bcm_irq_ops *source;
	unsigned int channel;
	unsigned int source_line;
};

/* MIP route indexed by GIC INTID. */
struct mip_intid_route {
	/* Store the DT instance number plus one so zero marks an unmapped INTID. */
	uint8_t instance_id;
	uint8_t channel;
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= UINT8_MAX, "Too many MIP instances");

/* clang-format off */
DT_INST_FOREACH_STATUS_OKAY(MIP_ENTRY_ASSERTS)

static const struct mip_intid_route mip_routes_by_intid[] = {
	DT_INST_FOREACH_STATUS_OKAY(MIP_INTID_ROUTES)
};

static const struct device *const mip_devices_by_dt_instance[] = {
	DT_INST_FOREACH_STATUS_OKAY(MIP_INSTANCE_DEVICE)
};

static struct k_spinlock soc_irq_lock;

IF_ENABLED(CONFIG_RP1_IRQ, (
	static const struct bcm_irq_ops rp1_irq_ops = {
		.dev = DEVICE_DT_GET(BCM2712_RP1_IRQ_NODE),
		.enable = rp1_irq_enable,
		.disable = rp1_irq_disable,
		.is_enabled = rp1_irq_is_enabled,
		.eoi = rp1_irq_eoi,
	};

	BCM2712_IRQ_ROUTE_DEFINE(rp1_irq_source_route, BCM2712_RP1_IRQ_NODE,
				 BCM2712_RP1_PCIE_NODE, rp1_irq_ops);
))

static inline void __attribute__((unused)) soc_irq_validate_unique(unsigned int val)
{
	switch (val) {
	default:
		DT_INST_FOREACH_STATUS_OKAY(MIP_INTID_CASES)
		break;
	}

	DT_INST_FOREACH_STATUS_OKAY(MIP_CHANNEL_CASES)
}

/* clang-format on */

static bool irq_source_lookup(struct irq_route *route)
{
	route->source = NULL;

	STRUCT_SECTION_FOREACH(bcm2712_irq_route, source_route) {
		if (source_route->parent != route->mip) {
			continue;
		}

		for (size_t i = 0; i < source_route->count; i++) {
			if (source_route->channels[i].channel != route->channel) {
				continue;
			}

			/* A MIP channel can only belong to one local source. */
			if (route->source != NULL) {
				return false;
			}

			route->source = source_route->source;
			route->source_line = source_route->channels[i].source_line;
		}
	}

	return route->source == NULL || device_is_ready(route->source->dev);
}

static const struct mip_intid_route *mip_route_lookup(unsigned int intid)
{
	if (intid >= ARRAY_SIZE(mip_routes_by_intid) ||
	    mip_routes_by_intid[intid].instance_id == 0U) {
		return NULL;
	}

	return &mip_routes_by_intid[intid];
}

static enum irq_route_state irq_route_lookup(unsigned int intid, struct irq_route *route)
{
	const struct mip_intid_route *mip_route = mip_route_lookup(intid);

	if (mip_route == NULL) {
		return IRQ_ROUTE_NONE;
	}

	route->mip = mip_devices_by_dt_instance[mip_route->instance_id - 1U];
	route->channel = mip_route->channel;
	if (!device_is_ready(route->mip)) {
		return IRQ_ROUTE_INVALID;
	}

	if (!irq_source_lookup(route)) {
		return IRQ_ROUTE_INVALID;
	}

	return IRQ_ROUTE_READY;
}

void z_soc_irq_enable(unsigned int irq)
{
	struct irq_route route;
	enum irq_route_state state = irq_route_lookup(irq, &route);
	k_spinlock_key_t key;
	bool gic_was_enabled;
	int ret;

	if (state == IRQ_ROUTE_NONE) {
		arm_gic_irq_enable(irq);
		return;
	}

	if (state == IRQ_ROUTE_INVALID) {
		return;
	}

	/* Enable the GIC and MIP before enabling the peripheral source. */
	key = k_spin_lock(&soc_irq_lock);
	gic_was_enabled = arm_gic_irq_is_enabled(irq);
	arm_gic_irq_enable(irq);
	ret = msi_proxy_enable(route.mip, route.channel);
	if (ret == 0 && route.source != NULL) {
		route.source->enable(route.source->dev, route.source_line);
	} else if (ret != 0 && !gic_was_enabled) {
		arm_gic_irq_disable(irq);
	}
	k_spin_unlock(&soc_irq_lock, key);

	if (ret != 0) {
		LOG_ERR("Failed to enable MSI proxy channel %u: %d", route.channel, ret);
	}
}

void z_soc_irq_disable(unsigned int irq)
{
	struct irq_route route;
	enum irq_route_state state = irq_route_lookup(irq, &route);
	k_spinlock_key_t key;

	if (state == IRQ_ROUTE_NONE) {
		arm_gic_irq_disable(irq);
		return;
	}

	if (state == IRQ_ROUTE_INVALID) {
		return;
	}

	/* Disable the peripheral source before disabling the MIP and GIC. */
	key = k_spin_lock(&soc_irq_lock);
	if (route.source != NULL) {
		route.source->disable(route.source->dev, route.source_line);
	}
	msi_proxy_disable(route.mip, route.channel);
	arm_gic_irq_disable(irq);
	k_spin_unlock(&soc_irq_lock, key);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	struct irq_route route;
	enum irq_route_state state = irq_route_lookup(irq, &route);
	k_spinlock_key_t key;
	int enabled = 0;

	if (state == IRQ_ROUTE_INVALID) {
		return 0;
	}

	if (state == IRQ_ROUTE_NONE) {
		return arm_gic_irq_is_enabled(irq);
	}

	key = k_spin_lock(&soc_irq_lock);
	if ((route.source == NULL ||
	     route.source->is_enabled(route.source->dev, route.source_line) != 0) &&
	    arm_gic_irq_is_enabled(irq)) {
		enabled = 1;
	}
	k_spin_unlock(&soc_irq_lock, key);

	return enabled;
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/* The static MIP map defines the trigger type before the devices are ready. */
	if (mip_route_lookup(irq) != NULL) {
		flags = IRQ_TYPE_EDGE;
	}

	arm_gic_irq_set_priority(irq, prio, flags);
}

void z_soc_irq_eoi(unsigned int irq)
{
	unsigned int intid = irq & BCM2712_GIC_INTID_MASK;
	struct irq_route route;
	size_t source_count;

	STRUCT_SECTION_COUNT(bcm2712_irq_route, &source_count);
	if (source_count != 0U && irq_route_lookup(intid, &route) == IRQ_ROUTE_READY &&
	    route.source != NULL) {
		route.source->eoi(route.source->dev, route.source_line);
	}

	arm_gic_eoi(irq);
}
