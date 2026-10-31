/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Raspberry Pi RP1 MSI-X interrupt controller driver.
 *
 * The RP1 is the PCIe south bridge of the Raspberry Pi 5. Its 61
 * internal interrupt sources are delivered as individual MSI-X
 * vectors; the host MSI proxy turns each MSI write into a host IRQ.
 * This driver programs the RP1 MSI-X table, controls the RP1 source
 * masks and uses the host IRQ numbers provided by the platform.
 *
 * The MSI-X table lives in an RP1 BAR, so it is located through the
 * configuration space of the PCIe root complex that owns the RP1.
 * The host MSI path comes from the MSI proxy referenced through
 * "msi-parent". The RP1 drives its vectors statically, so the generic
 * PCIe MSI core, which allocates vectors through the chosen root
 * complex, is not used.
 */

#define DT_DRV_COMPAT raspberrypi_rp1_intc

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_rp1.h>
#include <zephyr/drivers/interrupt_controller/msi_proxy.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/dt-bindings/interrupt-controller/rp1-irq.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(intc_rp1, CONFIG_INTC_LOG_LEVEL);

#define RP1_NODE  DT_INST_PARENT(0)
#define PCIE_NODE DT_PARENT(RP1_NODE)

#define RP1_PCIE_VENDOR 0x1de4U
#define RP1_PCIE_DEVICE 0x0001U

/* MSI-X CFG register block that sets the selected bits. */
#define RP1_MSIX_CFG_SET 0x800U
/* MSI-X CFG register block that clears the selected bits. */
#define RP1_MSIX_CFG_CLR 0xc00U

/* MSI-X CFG register offset of vector n. */
#define RP1_MSIX_CFG(n) (0x8U + 4U * (n))

#define RP1_MSIX_CFG_ENABLE  BIT(0)
#define RP1_MSIX_CFG_IACK    BIT(2)
#define RP1_MSIX_CFG_IACK_EN BIT(3)

struct rp1_intc {
	uint32_t num_irqs;
	bool ready;
};

struct rp1_route {
	uint32_t local;
	const struct device *parent;
	uint32_t parent_channel;
};

/* clang-format off */
#define RP1_ROUTE_ENTRY(node_id, prop, i)                                                          \
	{                                                                                          \
		.local = DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0),                 \
		.parent = DEVICE_DT_GET(DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, i)),             \
		.parent_channel = DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 0),       \
	},

static const struct rp1_route rp1_routes[] = {
	DT_FOREACH_MAP_ENTRY(DT_INST(0, raspberrypi_rp1_intc), interrupt_map, RP1_ROUTE_ENTRY)
};
/* clang-format on */

BUILD_ASSERT(ARRAY_SIZE(rp1_routes) == RP1_NUM_IRQS,
	     "RP1 interrupt-map does not cover all vectors");

static const struct rp1_route *rp1_route_find(unsigned int local)
{
	for (size_t i = 0; i < ARRAY_SIZE(rp1_routes); i++) {
		if (rp1_routes[i].local == local) {
			return &rp1_routes[i];
		}
	}

	return NULL;
}

/* Configuration space word of the primary/secondary/subordinate bus numbers. */
#define PCI_CONF_BUS_NUM              6U
/* Configuration space word of the header type register. */
#define PCI_CONF_HEADER_TYPE          3U
#define PCI_HEADER_TYPE_MULTIFUNCTION BIT(7)

struct rp1_irq_config {
	DEVICE_MMIO_ROM;
	const struct device *pcie;
	uint32_t num_irqs;
};

struct rp1_irq_data {
	DEVICE_MMIO_RAM;
	struct rp1_intc intc;
};

static uint32_t rp1_intc_read(const struct device *dev, unsigned int off)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + off);
}

static void rp1_intc_write(const struct device *dev, unsigned int off, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + off);
}

static void rp1_intc_setup(const struct device *dev, uint32_t num_irqs)
{
	struct rp1_irq_data *data = dev->data;

	/*
	 * The RP1 resets with every vector disabled. Mark every vector as
	 * level-triggered so that its MSI stays asserted until the handler
	 * acknowledges it. All RP1 sources except the USB host controllers
	 * are level-triggered; the USB drivers do not exist in Zephyr yet.
	 */
	for (uint32_t n = 0; n < num_irqs; n++) {
		rp1_intc_write(dev, RP1_MSIX_CFG_CLR + RP1_MSIX_CFG(n),
			       RP1_MSIX_CFG_ENABLE | RP1_MSIX_CFG_IACK_EN);
		rp1_intc_write(dev, RP1_MSIX_CFG_SET + RP1_MSIX_CFG(n), RP1_MSIX_CFG_IACK_EN);
	}

	data->intc.num_irqs = num_irqs;
	data->intc.ready = true;
}

static bool rp1_intc_is_enabled(const struct device *dev, unsigned int hwirq)
{
	struct rp1_irq_data *data = dev->data;

	if (!data->intc.ready || hwirq >= data->intc.num_irqs) {
		return false;
	}

	return (rp1_intc_read(dev, RP1_MSIX_CFG(hwirq)) & RP1_MSIX_CFG_ENABLE) != 0U;
}

static void rp1_intc_ack(const struct device *dev, unsigned int hwirq)
{
	struct rp1_irq_data *data = dev->data;

	if (!data->intc.ready || hwirq >= data->intc.num_irqs) {
		return;
	}

	if ((rp1_intc_read(dev, RP1_MSIX_CFG(hwirq)) & RP1_MSIX_CFG_IACK_EN) != 0U) {
		rp1_intc_write(dev, RP1_MSIX_CFG_SET + RP1_MSIX_CFG(hwirq), RP1_MSIX_CFG_IACK);
	}
}

static pcie_bdf_t rp1_find_bdf(const struct device *pcie)
{
	uint32_t buses;
	uint8_t bus;

	buses = pcie_ctrl_conf_read(pcie, PCIE_BDF(0, 0, 0), PCI_CONF_BUS_NUM);
	bus = (uint8_t)(buses >> 8);

	if (bus == 0U || bus == 0xffU) {
		LOG_ERR("PCIe root port has no secondary bus (0x%08x)", buses);
		return PCIE_BDF_NONE;
	}

	for (uint8_t dev = 0; dev < 32U; dev++) {
		for (uint8_t fn = 0; fn < 8U; fn++) {
			pcie_bdf_t bdf = PCIE_BDF(bus, dev, fn);
			uint32_t id = pcie_ctrl_conf_read(pcie, bdf, PCIE_CONF_ID);

			if (id == 0xffffffffU) {
				break;
			}

			if (id == PCIE_ID(RP1_PCIE_VENDOR, RP1_PCIE_DEVICE)) {
				return bdf;
			}

			if (fn == 0U) {
				uint32_t hdr = pcie_ctrl_conf_read(pcie, bdf, PCI_CONF_HEADER_TYPE);

				if ((hdr & (PCI_HEADER_TYPE_MULTIFUNCTION << 16)) == 0U) {
					break;
				}
			}
		}
	}

	LOG_ERR("RP1 not found on PCIe bus %u", bus);

	return PCIE_BDF_NONE;
}

static int rp1_msix_setup(const struct device *pcie, pcie_bdf_t bdf, uint32_t num_vectors)
{
	struct msi_proxy_message message;
	struct pcie_bar bar;
	const struct rp1_route *route;
	uint32_t cap, mcr, tr, table_size;
	uint8_t bir;
	uint8_t *table;

	cap = pcie_ctrl_get_cap(pcie, bdf, PCI_CAP_ID_MSIX);
	if (cap == 0U) {
		LOG_ERR("RP1 has no MSI-X capability");
		return -ENODEV;
	}

	mcr = pcie_ctrl_conf_read(pcie, bdf, cap + PCIE_MSIX_MCR);
	table_size = ((mcr & PCIE_MSIX_MCR_TSIZE) >> PCIE_MSIX_MCR_TSIZE_SHIFT) + 1U;

	if (table_size < num_vectors) {
		LOG_ERR("MSI-X table has %u vectors, expected %u", table_size, num_vectors);
		return -ENOTSUP;
	}

	tr = pcie_ctrl_conf_read(pcie, bdf, cap + PCIE_MSIX_TR);
	bir = tr & PCIE_MSIX_TR_BIR;
	if (bir > 5U) {
		return -EINVAL;
	}

	if (!pcie_ctrl_get_mbar(pcie, bdf, bir, &bar)) {
		LOG_ERR("MSI-X table BAR %u is not a usable memory BAR", bir);
		return -ENOTSUP;
	}

	k_mem_map_phys_bare(&table, bar.phys_addr + (tr & PCIE_MSIX_TR_OFFSET),
			    num_vectors * PCIE_MSIR_TABLE_ENTRY_SIZE,
			    K_MEM_CACHE_NONE | K_MEM_PERM_RW);

	for (uint32_t i = 0; i < num_vectors; i++) {
		uint8_t *entry = table + i * PCIE_MSIR_TABLE_ENTRY_SIZE;

		route = rp1_route_find(i);
		if (route == NULL) {
			LOG_ERR("No MSI route for RP1 vector %u", i);
			return -ENOENT;
		}

		if (msi_proxy_compose(route->parent, route->parent_channel, &message) != 0) {
			LOG_ERR("Failed to compose MSI message for RP1 vector %u", i);
			return -EINVAL;
		}

		sys_write32((uint32_t)message.address, (mm_reg_t)(entry + PCIE_VTBL_MA));
		sys_write32((uint32_t)(message.address >> 32), (mm_reg_t)(entry + PCIE_VTBL_MUA));
		sys_write32(message.data, (mm_reg_t)(entry + PCIE_VTBL_MD));
		sys_write32(0U, (mm_reg_t)(entry + PCIE_VTBL_VCTRL));
	}

	LOG_DBG("RP1 MSI-X: %u vectors, table BAR %u offset 0x%x", table_size, bir,
		tr & PCIE_MSIX_TR_OFFSET);

	/* The vector enables are clear at this point, so unmask the function. */
	mcr &= ~PCIE_MSIX_MCR_FMASK;
	mcr |= PCIE_MSIX_MCR_EN;
	pcie_ctrl_conf_write(pcie, bdf, cap + PCIE_MSIX_MCR, mcr);

	return 0;
}

static int rp1_irq_init(const struct device *dev)
{
	const struct rp1_irq_config *config = dev->config;
	struct rp1_irq_data *data = dev->data;
	pcie_bdf_t bdf;
	uint32_t cmdstat;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (!device_is_ready(config->pcie)) {
		LOG_ERR("PCIe root complex is not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(rp1_routes); i++) {
		for (size_t j = i + 1; j < ARRAY_SIZE(rp1_routes); j++) {
			if (rp1_routes[i].parent == rp1_routes[j].parent &&
			    rp1_routes[i].parent_channel == rp1_routes[j].parent_channel) {
				LOG_ERR("RP1 vectors %u and %u share MSI channel %u",
					rp1_routes[i].local, rp1_routes[j].local,
					rp1_routes[i].parent_channel);
				return -ENOTSUP;
			}
		}

		if (!device_is_ready(rp1_routes[i].parent)) {
			LOG_ERR("MSI proxy for RP1 vector %u is not ready", rp1_routes[i].local);
			return -ENODEV;
		}
	}

	bdf = rp1_find_bdf(config->pcie);
	if (bdf == PCIE_BDF_NONE) {
		return -ENODEV;
	}

	cmdstat = pcie_ctrl_conf_read(config->pcie, bdf, PCIE_CONF_CMDSTAT);
	pcie_ctrl_conf_write(config->pcie, bdf, PCIE_CONF_CMDSTAT,
			     cmdstat | PCIE_CONF_CMDSTAT_MEM | PCIE_CONF_CMDSTAT_MASTER);

	/* Keep every vector disabled while the MSI-X table is rewritten. */
	rp1_intc_setup(dev, config->num_irqs);

	ret = rp1_msix_setup(config->pcie, bdf, config->num_irqs);
	if (ret != 0) {
		data->intc.ready = false;
		return ret;
	}

	LOG_INF("RP1 IRQ controller ready: vectors 0..%u", config->num_irqs - 1U);

	return 0;
}

static bool rp1_route_is_ready(const struct device *dev)
{
	const struct rp1_irq_data *data = dev->data;

	return data->intc.ready;
}

static void rp1_route_enable(const struct device *dev, unsigned int line)
{
	struct rp1_irq_data *data = dev->data;
	const struct rp1_route *route = rp1_route_find(line);

	if (route == NULL || !data->intc.ready || line >= data->intc.num_irqs) {
		return;
	}

	/* Enable the parent path before unmasking the RP1 source. */
	if (msi_proxy_enable(route->parent, route->parent_channel) != 0) {
		LOG_ERR("Failed to enable MSI proxy channel %u", route->parent_channel);
		return;
	}

	rp1_intc_write(dev, RP1_MSIX_CFG_SET + RP1_MSIX_CFG(line), RP1_MSIX_CFG_ENABLE);
}

static void rp1_route_disable(const struct device *dev, unsigned int line)
{
	struct rp1_irq_data *data = dev->data;
	const struct rp1_route *route = rp1_route_find(line);

	if (route == NULL || !data->intc.ready || line >= data->intc.num_irqs) {
		return;
	}

	/* Mask the RP1 source before masking the parent path. */
	rp1_intc_write(dev, RP1_MSIX_CFG_CLR + RP1_MSIX_CFG(line), RP1_MSIX_CFG_ENABLE);
	msi_proxy_disable(route->parent, route->parent_channel);
}

static int rp1_route_is_enabled(const struct device *dev, unsigned int line)
{
	return rp1_intc_is_enabled(dev, line) ? 1 : 0;
}

static void rp1_route_eoi(const struct device *dev, unsigned int line)
{
	rp1_intc_ack(dev, line);
}

const struct irq_route_ops rp1_intc_route_ops = {
	.is_ready = rp1_route_is_ready,
	.enable = rp1_route_enable,
	.disable = rp1_route_disable,
	.is_enabled = rp1_route_is_enabled,
	.eoi = rp1_route_eoi,
};

#define RP1_IRQ_INIT(n)                                                                            \
	static struct rp1_irq_data rp1_irq_data_##n;                                               \
	static const struct rp1_irq_config rp1_irq_config_##n = {                                  \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.pcie = DEVICE_DT_GET(PCIE_NODE),                                                  \
		.num_irqs = RP1_NUM_IRQS,                                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, rp1_irq_init, NULL, &rp1_irq_data_##n, &rp1_irq_config_##n,       \
			      PRE_KERNEL_1, CONFIG_RP1_IRQ_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(RP1_IRQ_INIT)
