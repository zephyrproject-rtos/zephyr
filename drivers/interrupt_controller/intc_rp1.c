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
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_rp1.h>
#include <zephyr/drivers/interrupt_controller/msi_proxy.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/dt-bindings/interrupt-controller/rp1_irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(intc_rp1, CONFIG_INTC_LOG_LEVEL);

#define RP1_NODE        DT_INST_PARENT(0)
#define PCIE_NODE       DT_PARENT(RP1_NODE)
#define MSI_PARENT_NODE DT_INST_PHANDLE(0, msi_parent)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only one RP1 interrupt controller instance is supported");
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(MSI_PARENT_NODE), "RP1 MSI parent must be enabled");
BUILD_ASSERT(RP1_NUM_IRQS <= 64U, "Initial IACK mask must cover all RP1 vectors");

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
	uint32_t flags;
	const struct device *parent;
	uint32_t parent_channel;
};

struct rp1_irq_config {
	DEVICE_MMIO_ROM;
	const struct device *pcie;
	uint32_t num_irqs;
};

struct rp1_irq_data {
	DEVICE_MMIO_RAM;
	struct rp1_intc intc;
	struct k_spinlock lock;
	/* Set for level sources at init and cleared after IACK on first enable. */
	uint64_t initial_iack_mask;
};

/* clang-format off */
#define RP1_ROUTE_ENTRY(node_id, prop, i)                                                          \
	{                                                                                          \
		.local = DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0),                 \
		.flags = DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 1),                 \
		.parent = DEVICE_DT_GET_OR_NULL(DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, i)),     \
		.parent_channel = DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, i, 0),       \
	},

#define RP1_ROUTE_ASSERT(node_id, prop, i)                                                         \
	BUILD_ASSERT(DT_SAME_NODE(DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, i),                    \
				  MSI_PARENT_NODE),                                                \
		     "RP1 interrupt-map parent must match msi-parent");                            \
	BUILD_ASSERT(DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 1) ==                   \
			     RP1_IRQ_TYPE_EDGE ||                                                  \
		     DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 1) ==                   \
			     RP1_IRQ_TYPE_LEVEL,                                                   \
		     "RP1 interrupt-map flags must use RP1_IRQ_TYPE_EDGE or RP1_IRQ_TYPE_LEVEL");

DT_FOREACH_MAP_ENTRY(DT_DRV_INST(0), interrupt_map, RP1_ROUTE_ASSERT)

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

	data->initial_iack_mask = 0U;
	/* Level sources need IACK to retrigger while the source remains asserted. */
	for (uint32_t n = 0; n < num_irqs; n++) {
		const struct rp1_route *route = rp1_route_find(n);

		rp1_intc_write(dev, RP1_MSIX_CFG_CLR + RP1_MSIX_CFG(n),
			       RP1_MSIX_CFG_ENABLE | RP1_MSIX_CFG_IACK_EN);
		if (route != NULL && (route->flags & RP1_IRQ_TYPE_LEVEL) != 0U) {
			rp1_intc_write(dev, RP1_MSIX_CFG_SET + RP1_MSIX_CFG(n),
				       RP1_MSIX_CFG_IACK_EN);
			data->initial_iack_mask |= BIT64(n);
		}
	}

	data->intc.num_irqs = num_irqs;
	data->intc.ready = true;
}

int rp1_irq_is_enabled(const struct device *dev, unsigned int line)
{
	struct rp1_irq_data *data = dev->data;
	k_spinlock_key_t key;
	int enabled = 0;

	key = k_spin_lock(&data->lock);
	if (data->intc.ready && line < data->intc.num_irqs) {
		enabled = (rp1_intc_read(dev, RP1_MSIX_CFG(line)) & RP1_MSIX_CFG_ENABLE) != 0U;
	}
	k_spin_unlock(&data->lock, key);

	return enabled;
}

void rp1_irq_eoi(const struct device *dev, unsigned int line)
{
	struct rp1_irq_data *data = dev->data;

	if (!data->intc.ready || line >= data->intc.num_irqs) {
		return;
	}

	if ((rp1_intc_read(dev, RP1_MSIX_CFG(line)) & RP1_MSIX_CFG_IACK_EN) != 0U) {
		rp1_intc_write(dev, RP1_MSIX_CFG_SET + RP1_MSIX_CFG(line), RP1_MSIX_CFG_IACK);
	}
}

static void rp1_msix_write_entry(struct msix_vector *entry, uint64_t address, uint32_t data)
{
	sys_write32((uint32_t)address, (mm_reg_t)&entry->msg_addr);
	sys_write32((uint32_t)(address >> 32), (mm_reg_t)&entry->msg_up_addr);
	sys_write32(data, (mm_reg_t)&entry->msg_data);
	sys_write32(0U, (mm_reg_t)&entry->vector_ctrl);
}

static bool rp1_scan_cb(const struct pcie_endpoint *ep, pcie_id_t id, void *cb_data)
{
	pcie_bdf_t *bdf = cb_data;

	if (id == PCIE_ID(RP1_PCIE_VENDOR, RP1_PCIE_DEVICE)) {
		*bdf = ep->bdf;
		return false;
	}

	return true;
}

static pcie_bdf_t rp1_find_bdf(const struct device *pcie)
{
	pcie_bdf_t bdf = PCIE_BDF_NONE;
	const struct pcie_scan_opt opt = {
		.ctrl = pcie,
		.bus = 0U,
		.cb = rp1_scan_cb,
		.cb_data = &bdf,
		.flags = PCIE_SCAN_RECURSIVE,
	};
	int ret;

	ret = pcie_scan(&opt);
	if (ret != 0) {
		LOG_ERR("PCIe scan failed: %d", ret);
		return PCIE_BDF_NONE;
	}

	if (bdf == PCIE_BDF_NONE) {
		LOG_ERR("RP1 not found on PCIe controller");
	}

	return bdf;
}

static int rp1_msix_table_addr(const struct pcie_endpoint *ep, uint32_t cap, uint32_t num_vectors,
			       uintptr_t *phys_addr_out, uint8_t *bir_out, uint32_t *offset_out)
{
	struct pcie_bar bar;
	uint32_t tr, offset;
	size_t table_bytes;
	uint8_t bir;

	tr = pcie_conf_read(ep, cap + PCIE_MSIX_TR);
	bir = tr & PCIE_MSIX_TR_BIR;
	if (bir > 5U) {
		return -EINVAL;
	}

	if (!pcie_get_mbar(ep, bir, &bar)) {
		LOG_ERR("MSI-X table BAR %u is not a usable memory BAR", bir);
		return -ENOTSUP;
	}

	offset = tr & PCIE_MSIX_TR_OFFSET;
	table_bytes = (size_t)num_vectors * PCIE_MSIR_TABLE_ENTRY_SIZE;
	if (num_vectors == 0U || (table_bytes / PCIE_MSIR_TABLE_ENTRY_SIZE) != num_vectors ||
	    offset >= bar.size || table_bytes > (bar.size - offset) ||
	    bar.phys_addr > (UINTPTR_MAX - offset)) {
		LOG_ERR("MSI-X table (offset 0x%x, size 0x%zx) exceeds BAR %u size 0x%zx", offset,
			table_bytes, bir, bar.size);
		return -EINVAL;
	}

	*phys_addr_out = bar.phys_addr + offset;
	*bir_out = bir;
	*offset_out = offset;

	return 0;
}

static int rp1_msix_setup(const struct pcie_endpoint *ep, uint32_t num_vectors)
{
	struct msi_proxy_message messages[RP1_NUM_IRQS];
	const struct rp1_route *route;
	uintptr_t table_phys;
	uint32_t cap, mcr, offset, table_size;
	uint8_t bir;
	uint8_t *table;
	int ret;

	if (num_vectors > ARRAY_SIZE(messages)) {
		return -EINVAL;
	}

	cap = pcie_get_cap(ep, PCI_CAP_ID_MSIX);
	if (cap == 0U) {
		LOG_ERR("RP1 has no MSI-X capability");
		return -ENODEV;
	}

	mcr = pcie_conf_read(ep, cap + PCIE_MSIX_MCR);
	table_size = ((mcr & PCIE_MSIX_MCR_TSIZE) >> PCIE_MSIX_MCR_TSIZE_SHIFT) + 1U;

	if (table_size < num_vectors) {
		LOG_ERR("MSI-X table has %u vectors, expected %u", table_size, num_vectors);
		return -ENOTSUP;
	}

	for (uint32_t i = 0; i < num_vectors; i++) {
		route = rp1_route_find(i);
		if (route == NULL) {
			LOG_ERR("No MSI route for RP1 vector %u", i);
			return -ENOENT;
		}

		if (!IS_ENABLED(CONFIG_MSI_PROXY) || !DEVICE_API_IS(msi_proxy, route->parent)) {
			LOG_ERR("Unsupported MSI parent for RP1 vector %u", i);
			return -ENOTSUP;
		}

		if (msi_proxy_compose(route->parent, route->parent_channel, &messages[i]) != 0) {
			LOG_ERR("Failed to compose MSI message for RP1 vector %u", i);
			return -EINVAL;
		}
	}

	ret = rp1_msix_table_addr(ep, cap, num_vectors, &table_phys, &bir, &offset);
	if (ret != 0) {
		return ret;
	}

	device_map((mm_reg_t *)&table, table_phys, (size_t)num_vectors * PCIE_MSIR_TABLE_ENTRY_SIZE,
		   K_MEM_CACHE_NONE);

	/* Firmware may have left MSI-X enabled with pending vectors. */
	pcie_conf_write(ep, cap + PCIE_MSIX_MCR, mcr | PCIE_MSIX_MCR_FMASK);

	for (uint32_t i = 0; i < num_vectors; i++) {
		struct msix_vector *entry =
			(struct msix_vector *)(table + i * PCIE_MSIR_TABLE_ENTRY_SIZE);

		rp1_msix_write_entry(entry, messages[i].address, messages[i].data);
	}

	LOG_DBG("RP1 MSI-X: %u vectors, table BAR %u offset 0x%x", table_size, bir, offset);

	/* Flush posted table writes before unmasking the function. */
	(void)sys_read32((mm_reg_t)table);
	mcr &= ~PCIE_MSIX_MCR_FMASK;
	mcr |= PCIE_MSIX_MCR_EN;
	pcie_conf_write(ep, cap + PCIE_MSIX_MCR, mcr);

	return 0;
}

static int rp1_irq_init(const struct device *dev)
{
	const struct rp1_irq_config *config = dev->config;
	struct rp1_irq_data *data = dev->data;
	struct pcie_endpoint ep;
	pcie_bdf_t bdf;
	uint32_t command;
	int ret;

	if (IS_ENABLED(CONFIG_PCIE_CONTROLLER) &&
	    ((config->pcie == NULL) || !device_is_ready(config->pcie))) {
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

	ep = pcie_endpoint_make(config->pcie, bdf);
	/* Write only the command bits, leaving write-one-to-clear status bits alone. */
	command = pcie_conf_read(&ep, PCIE_CONF_CMDSTAT) & UINT16_MAX;
	pcie_conf_write(&ep, PCIE_CONF_CMDSTAT, command | PCIE_CONF_CMDSTAT_MEM);

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Keep every vector disabled while the MSI-X table is rewritten. */
	rp1_intc_setup(dev, config->num_irqs);

	ret = rp1_msix_setup(&ep, config->num_irqs);
	if (ret != 0) {
		data->intc.ready = false;
		device_unmap(DEVICE_MMIO_GET(dev), DEVICE_MMIO_ROM_PTR(dev)->size);
		*DEVICE_MMIO_RAM_PTR(dev) = 0U;
		pcie_conf_write(&ep, PCIE_CONF_CMDSTAT, command);
		return ret;
	}

	pcie_conf_write(&ep, PCIE_CONF_CMDSTAT,
			command | PCIE_CONF_CMDSTAT_MEM | PCIE_CONF_CMDSTAT_MASTER);

	LOG_INF("RP1 IRQ controller ready: vectors 0..%u", config->num_irqs - 1U);

	return 0;
}

void rp1_irq_enable(const struct device *dev, unsigned int line)
{
	struct rp1_irq_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->intc.ready && line < data->intc.num_irqs) {
		uint64_t mask = BIT64(line);
		unsigned int offset = RP1_MSIX_CFG_SET + RP1_MSIX_CFG(line);

		rp1_intc_write(dev, offset, RP1_MSIX_CFG_ENABLE);
		/* Rearm a level source left waiting for IACK on a warm restart. */
		if ((data->initial_iack_mask & mask) != 0U) {
			rp1_intc_write(dev, offset, RP1_MSIX_CFG_IACK);
			data->initial_iack_mask &= ~mask;
		}
	}
	k_spin_unlock(&data->lock, key);
}

void rp1_irq_disable(const struct device *dev, unsigned int line)
{
	struct rp1_irq_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->intc.ready && line < data->intc.num_irqs) {
		rp1_intc_write(dev, RP1_MSIX_CFG_CLR + RP1_MSIX_CFG(line), RP1_MSIX_CFG_ENABLE);
		/* Flush the posted write before the caller masks the host path. */
		(void)rp1_intc_read(dev, RP1_MSIX_CFG(line));
	}
	k_spin_unlock(&data->lock, key);
}

/* clang-format off */
#define RP1_IRQ_INIT(n)                                                                            \
	static struct rp1_irq_data rp1_irq_data_##n;                                               \
	static const struct rp1_irq_config rp1_irq_config_##n = {                                  \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		IF_ENABLED(CONFIG_PCIE_CONTROLLER, (.pcie = DEVICE_DT_GET_OR_NULL(PCIE_NODE),))    \
		.num_irqs = RP1_NUM_IRQS,                                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, rp1_irq_init, NULL, &rp1_irq_data_##n, &rp1_irq_config_##n,       \
			      PRE_KERNEL_1, CONFIG_RP1_IRQ_INIT_PRIORITY, NULL);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(RP1_IRQ_INIT)
