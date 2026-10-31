/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2712 SoC IRQ glue. Interrupts are routed through the device tree
 * maps of the MSI proxy and the RP1 controller. The GIC remains the
 * root controller; its INTID encoding and EOI rules stay here.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/map.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/interrupt_controller/intc_rp1.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc_irq, LOG_LEVEL_ERR);

/* GICv2 interrupt ID field of GICC_IAR. */
#define GIC_INTID_MASK 0x3ffU

/* GIC INTIDs 0..31 are SGIs and PPIs, SPIs start at 32. */
#define GIC_SPI_INTID_BASE 32U

#define RP1_NODE DT_INST(0, raspberrypi_rp1_intc)
#define MIP_NODE DT_PHANDLE(RP1_NODE, msi_parent)

/* Result of resolving a GIC INTID back to its RP1 interrupt route. */
enum rp1_route_state {
	RP1_ROUTE_NONE,
	RP1_ROUTE_OK,
	RP1_ROUTE_INVALID,
};

/* The RP1 controller that serves every route. */
struct rp1_controller {
	const struct device *dev;
	const struct irq_route_ops *ops;
};

/* RP1 local source -> MIP channel. */
struct rp1_map_route {
	unsigned int local;
	unsigned int channel;
};

/* clang-format off */
#define RP1_PARENT_ASSERT(node_id, prop, i) \
	BUILD_ASSERT(DT_SAME_NODE(DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, i), MIP_NODE), \
		     "RP1 interrupt-map uses an unsupported MSI parent");

DT_FOREACH_MAP_ENTRY(RP1_NODE, interrupt_map, RP1_PARENT_ASSERT)

/* The binding maps vector N to brcm,msi-base-spi + brcm,msi-offset + N. */
#define MIP_ROUTE_LINEAR(node_id, prop, i) \
	BUILD_ASSERT(DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0) == (i), \
		     "MIP interrupt-map vectors must be consecutive"); \
	BUILD_ASSERT(DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 1) == \
			     DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, 0, 1) + (i), \
		     "MIP interrupt-map SPIs must be consecutive");

DT_FOREACH_MAP_ENTRY(MIP_NODE, interrupt_map, MIP_ROUTE_LINEAR)

/* Per-vector GIC trigger type, from the MIP interrupt map. */
#define MIP_FLAGS_ENTRY(node_id, prop, i) \
	DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 2),

#define RP1_ROUTE_ENTRY(node_id, prop, i) \
	{ \
		.local = DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0), \
		.channel = DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 0), \
	},

static const unsigned int mip_flags[] = {
	DT_FOREACH_MAP_ENTRY(MIP_NODE, interrupt_map, MIP_FLAGS_ENTRY)
};

static const struct rp1_map_route rp1_routes[] = {
	DT_FOREACH_MAP_ENTRY(RP1_NODE, interrupt_map, RP1_ROUTE_ENTRY)
};
/* clang-format on */

static const struct rp1_controller rp1_ctrl = {
	.dev = DEVICE_DT_GET(RP1_NODE),
	.ops = &rp1_intc_route_ops,
};

static const unsigned int mip_intid_base =
	DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(MIP_NODE, interrupt_map, 0, 1) + GIC_SPI_INTID_BASE;

/*
 * BCM2712 MIP vectors map to consecutive GIC SPIs. MIP_ROUTE_LINEAR verifies
 * the DT ordering, allowing constant-time INTID lookup on the IRQ path.
 * RP1 source-to-MIP vector mappings are resolved independently.
 */
static bool mip_channel_from_intid(unsigned int intid, unsigned int *channel)
{
	unsigned int index;

	if (intid < mip_intid_base) {
		return false;
	}

	index = intid - mip_intid_base;

	if (index >= ARRAY_SIZE(mip_flags)) {
		return false;
	}

	*channel = index;

	return true;
}

static const struct rp1_map_route *rp1_route_find_channel(unsigned int channel)
{
	for (size_t i = 0; i < ARRAY_SIZE(rp1_routes); i++) {
		if (rp1_routes[i].channel == channel) {
			return &rp1_routes[i];
		}
	}

	return NULL;
}

static enum rp1_route_state rp1_route_lookup(unsigned int intid, const struct rp1_map_route **out)
{
	const struct rp1_map_route *route;
	unsigned int channel;

	if (!mip_channel_from_intid(intid, &channel)) {
		return RP1_ROUTE_NONE;
	}

	route = rp1_route_find_channel(channel);
	if (route == NULL) {
		return RP1_ROUTE_NONE;
	}

	*out = route;

	/*
	 * The RP1 driver validates its routing during initialization. A route
	 * that reaches an uninitialized controller must not fall back to the
	 * plain GIC path, so report it as invalid rather than absent.
	 */
	if (!rp1_ctrl.ops->is_ready(rp1_ctrl.dev)) {
		return RP1_ROUTE_INVALID;
	}

	return RP1_ROUTE_OK;
}

void z_soc_irq_init(void)
{
	/*
	 * Routing is validated by the RP1 driver during its own initialization
	 * and consulted through rp1_route_lookup() at run time.
	 */
}

void z_soc_irq_enable(unsigned int irq)
{
	const struct rp1_map_route *route;
	enum rp1_route_state state = rp1_route_lookup(irq, &route);

	if (state == RP1_ROUTE_NONE) {
		arm_gic_irq_enable(irq);
		return;
	}

	if (state == RP1_ROUTE_INVALID) {
		/* Belongs to the RP1, but its routing is not usable. */
		return;
	}

	/* Root first, then the parent path and finally the RP1 source. */
	arm_gic_irq_enable(irq);
	rp1_ctrl.ops->enable(rp1_ctrl.dev, route->local);
}

void z_soc_irq_disable(unsigned int irq)
{
	const struct rp1_map_route *route;
	enum rp1_route_state state = rp1_route_lookup(irq, &route);

	if (state == RP1_ROUTE_NONE) {
		arm_gic_irq_disable(irq);
		return;
	}

	if (state == RP1_ROUTE_INVALID) {
		return;
	}

	/* RP1 source first, then the parent path and the root. */
	rp1_ctrl.ops->disable(rp1_ctrl.dev, route->local);
	arm_gic_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	const struct rp1_map_route *route;
	enum rp1_route_state state = rp1_route_lookup(irq, &route);

	if (state == RP1_ROUTE_INVALID) {
		return 0;
	}

	if (state == RP1_ROUTE_NONE) {
		return arm_gic_irq_is_enabled(irq);
	}

	if (rp1_ctrl.ops->is_enabled(rp1_ctrl.dev, route->local) == 0) {
		return 0;
	}

	return arm_gic_irq_is_enabled(irq);
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	const struct rp1_map_route *route;
	unsigned int channel;

	if (rp1_route_lookup(irq, &route) == RP1_ROUTE_INVALID) {
		return;
	}

	/* The GIC trigger type comes from the MIP map, not the RP1 source. */
	if (mip_channel_from_intid(irq, &channel)) {
		flags = mip_flags[channel];
	}

	arm_gic_irq_set_priority(irq, prio, flags);
}

unsigned int z_soc_irq_get_active(void)
{
	return arm_gic_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
	unsigned int intid = irq & GIC_INTID_MASK;
	const struct rp1_map_route *route;

	if (rp1_route_lookup(intid, &route) == RP1_ROUTE_OK) {
		/* Acknowledge the RP1 source before completing at the GIC. */
		rp1_ctrl.ops->eoi(rp1_ctrl.dev, route->local);
	}

	arm_gic_eoi(irq);
}
