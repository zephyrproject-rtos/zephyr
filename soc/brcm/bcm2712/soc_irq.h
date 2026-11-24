/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_BRCM_BCM2712_SOC_IRQ_H_
#define ZEPHYR_SOC_BRCM_BCM2712_SOC_IRQ_H_

#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/devicetree/map.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

/* GICv2 interrupt ID field of GICC_IAR. */
#define BCM2712_GIC_INTID_MASK 0x3ffU

#define BCM2712_RP1_IRQ_NODE  DT_INST(0, raspberrypi_rp1_intc)
#define BCM2712_RP1_PCIE_NODE DT_PARENT(DT_PARENT(BCM2712_RP1_IRQ_NODE))

/* Operations for a peripheral's local interrupts and the device they act on. */
struct bcm_irq_ops {
	const struct device *dev;
	void (*enable)(const struct device *dev, unsigned int line);
	void (*disable)(const struct device *dev, unsigned int line);
	int (*is_enabled)(const struct device *dev, unsigned int line);
	void (*eoi)(const struct device *dev, unsigned int line);
};

struct bcm_irq_map {
	unsigned int channel;
	unsigned int source_line;
};

struct bcm2712_irq_route {
	const struct device *parent;
	const struct bcm_irq_ops *source;
	const struct bcm_irq_map *channels;
	size_t count;
};

/* clang-format off */
#define MIP_ENTRY_ASSERT(node_id, prop, i) \
	BUILD_ASSERT(DT_SAME_NODE(DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, i), \
				 DT_INST(0, arm_gic)), "MIP interrupt-map must target the GIC"); \
	BUILD_ASSERT(DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 0) == GIC_SPI, \
		     "MIP interrupt-map must target SPIs"); \
	BUILD_ASSERT(DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 1) == IRQ_TYPE_EDGE && \
		     DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 2) == IRQ_TYPE_EDGE, \
		     "MIP interrupt-map must use edge-triggered interrupts"); \
	BUILD_ASSERT(DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 1) <= \
		     GIC_SPI_MAX_INTID - GIC_SPI_INT_BASE, \
		     "MIP interrupt-map SPI exceeds GIC SPI range"); \
	BUILD_ASSERT(DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 1) + \
			     GIC_SPI_INT_BASE < CONFIG_NUM_IRQS, \
		     "MIP interrupt-map SPI exceeds CONFIG_NUM_IRQS");

#define MIP_ENTRY_ASSERTS(inst) DT_FOREACH_MAP_ENTRY(DT_DRV_INST(inst), interrupt_map, \
						    MIP_ENTRY_ASSERT)

#define MIP_CASE_INTID(node_id, prop, i) \
	case DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 1) + GIC_SPI_INT_BASE:
#define MIP_CASE_CHANNEL(node_id, prop, i) \
	case DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0):
#define MIP_INTID_CASES(inst) DT_FOREACH_MAP_ENTRY(DT_DRV_INST(inst), interrupt_map, MIP_CASE_INTID)
#define MIP_CHANNEL_CASES(inst) \
	switch (val) { \
	default: \
	DT_FOREACH_MAP_ENTRY(DT_DRV_INST(inst), interrupt_map, MIP_CASE_CHANNEL) \
		break; \
	}
#define MIP_INTID_ROUTE(node_id, prop, i, inst) \
	[DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 1) + GIC_SPI_INT_BASE] = { \
		.instance_id = (inst) + 1U, \
		.channel = DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0), \
	},
#define MIP_INTID_ROUTES(inst) DT_FOREACH_MAP_ENTRY_VARGS(DT_DRV_INST(inst), interrupt_map, \
							 MIP_INTID_ROUTE, inst)
#define MIP_INSTANCE_DEVICE(inst) [inst] = DEVICE_DT_INST_GET(inst),

#define BCM2712_IRQ_ROUTE_PARENT(node_id) DT_MAP_ENTRY_PARENT_BY_IDX(node_id, interrupt_map, 0)

#define BCM2712_IRQ_ROUTE_ASSERT_PARENT(node_id, prop, i) \
	BUILD_ASSERT(DT_SAME_NODE(DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, i), \
				  BCM2712_IRQ_ROUTE_PARENT(node_id)), \
		     "Routes from one interrupt source must use the same parent");

#define BCM2712_IRQ_ROUTE_SOURCE_CASE(node_id, prop, i) \
	case DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0):
#define BCM2712_IRQ_ROUTE_CHANNEL_CASE(node_id, prop, i) \
	case DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 0):

#define BCM2712_IRQ_ROUTE_ASSERT_UNIQUE(name, node_id) \
	static inline void __attribute__((unused)) name##_validate_unique(unsigned int val) \
	{ \
		switch (val) { \
		default: \
		DT_FOREACH_MAP_ENTRY(node_id, interrupt_map, BCM2712_IRQ_ROUTE_SOURCE_CASE) \
			break; \
		} \
		switch (val) { \
		default: \
		DT_FOREACH_MAP_ENTRY(node_id, interrupt_map, BCM2712_IRQ_ROUTE_CHANNEL_CASE) \
			break; \
		} \
	}

#define BCM2712_IRQ_ROUTE_CHANNEL(node_id, prop, i) \
	{ \
		.channel = DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 0), \
		.source_line = DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0), \
	},

/* Publish a peripheral's local interrupt sources and its devicetree routes. */
#define BCM2712_IRQ_ROUTE_DEFINE(name, source_node, pcie_node, source_ops) \
	BUILD_ASSERT(DT_MAP_LEN(source_node, interrupt_map) > 0, \
		     "Interrupt source needs a route"); \
	BUILD_ASSERT(!DT_NODE_HAS_COMPAT(pcie_node, brcm_brcmstb_pcie) || \
		     DT_SAME_NODE(DT_PHANDLE(pcie_node, msi_parent), \
				  BCM2712_IRQ_ROUTE_PARENT(source_node)), \
		     "BRCMSTB and its interrupt source must use the same MSI parent"); \
	DT_FOREACH_MAP_ENTRY(source_node, interrupt_map, BCM2712_IRQ_ROUTE_ASSERT_PARENT) \
	BCM2712_IRQ_ROUTE_ASSERT_UNIQUE(name, source_node) \
	static const struct bcm_irq_map name##_channels[] = { \
		DT_FOREACH_MAP_ENTRY(source_node, interrupt_map, BCM2712_IRQ_ROUTE_CHANNEL) \
	}; \
	static const STRUCT_SECTION_ITERABLE(bcm2712_irq_route, name) = { \
		.parent = DEVICE_DT_GET_OR_NULL(BCM2712_IRQ_ROUTE_PARENT(source_node)), \
		.source = &(source_ops), \
		.channels = name##_channels, \
		.count = ARRAY_SIZE(name##_channels), \
	}
/* clang-format on */

#endif /* ZEPHYR_SOC_BRCM_BCM2712_SOC_IRQ_H_ */
