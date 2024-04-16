/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qemu_ivshmem

#define LOG_LEVEL CONFIG_IVSHMEM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ivshmem);

#include <errno.h>

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/init.h>

#include <zephyr/drivers/virtualization/ivshmem.h>
#include "virt_ivshmem.h"

#ifdef CONFIG_IVSHMEM_DOORBELL

static void ivshmem_doorbell(const void *arg)
{
	const struct ivshmem_param *param = arg;

	LOG_DBG("Interrupt received on vector %u", param->vector);

	if (param->signal != NULL) {
		k_poll_signal_raise(param->signal, param->vector);
	}
}

static bool ivshmem_configure_msi_x_interrupts(const struct device *dev)
{
#if defined(CONFIG_PCIE_MSI_X) && defined(CONFIG_PCIE_MSI_MULTI_VECTOR)
	struct ivshmem *data = dev->data;
	bool ret = false;
	uint8_t n_vectors;
	uint32_t key;
	int i;

	key = irq_lock();

	n_vectors = pcie_msi_vectors_allocate(data->pcie->bdf,
					      CONFIG_IVSHMEM_INT_PRIORITY,
					      data->vectors,
					      CONFIG_IVSHMEM_MSI_X_VECTORS);
	if (n_vectors == 0) {
		LOG_ERR("Could not allocate %u MSI-X vectors",
			CONFIG_IVSHMEM_MSI_X_VECTORS);
		goto out;
	}

	LOG_DBG("Allocated %u vectors", n_vectors);

	for (i = 0; i < n_vectors; i++) {
		data->params[i].dev = dev;
		data->params[i].vector = i;

		if (!pcie_msi_vector_connect(data->pcie->bdf,
					     &data->vectors[i],
					     ivshmem_doorbell,
					     &data->params[i], 0)) {
			LOG_ERR("Failed to connect MSI-X vector %u", i);
			goto out;
		}
	}

	LOG_INF("%u MSI-X Vectors connected", n_vectors);

	if (!pcie_msi_enable(data->pcie->bdf, data->vectors, n_vectors, 0)) {
		LOG_ERR("Could not enable MSI-X");
		goto out;
	}

	data->n_vectors = n_vectors;
	ret = true;

	LOG_DBG("MSI-X configured");
out:
	irq_unlock(key);

	return ret;
#else
	return false;
#endif
}

#ifdef CONFIG_IVSHMEM_V2
static bool ivshmem_configure_int_x_interrupts(const struct device *dev)
{
	struct ivshmem *data = dev->data;
	const struct ivshmem_cfg *cfg = dev->config;
	uint32_t cfg_int = pcie_conf_read(data->pcie->bdf, PCIE_CONF_INTR);
	uint32_t cfg_intx_pin = PCIE_CONF_INTR_PIN(cfg_int);

	if (!IN_RANGE(cfg_intx_pin, PCIE_INTX_PIN_MIN, PCIE_INTX_PIN_MAX)) {
		LOG_ERR("Invalid INTx pin %u", cfg_intx_pin);
		return false;
	}

	/* Ensure INTx is enabled */
	pcie_set_cmd(data->pcie->bdf, PCIE_CONF_CMDSTAT_INTX_DISABLE, false);

	const struct intx_info *intx = &cfg->intx_info[cfg_intx_pin - 1];

	data->params[0].dev = dev;
	data->params[0].vector = 0;

	LOG_INF("Enabling INTx IRQ %u (pin %u)", intx->irq, cfg_intx_pin);
	if (intx->irq == INTX_IRQ_UNUSED ||
		!pcie_connect_dynamic_irq(
			data->pcie->bdf, intx->irq, intx->priority,
			ivshmem_doorbell, &data->params[0], intx->flags)) {
		LOG_ERR("Failed to connect INTx ISR %u", cfg_intx_pin);
		return false;
	}

	data->n_vectors = 1;

	pcie_irq_enable(data->pcie->bdf, intx->irq);

	return true;
}
#endif /* CONFIG_IVSHMEM_V2 */

static void register_signal(const struct device *dev,
			    struct k_poll_signal *signal,
			    uint16_t vector)
{
	struct ivshmem *data = dev->data;

	data->params[vector].signal = signal;
}

#else

#define ivshmem_configure_msi_x_interrupts(...) true
#define ivshmem_configure_int_x_interrupts(...) true
#define register_signal(...)

#endif /* CONFIG_IVSHMEM_DOORBELL */

static const struct ivshmem_reg no_reg;

__maybe_unused static uint64_t pcie_conf_read_u64(pcie_bdf_t bdf, unsigned int reg)
{
	uint64_t lo = pcie_conf_read(bdf, reg);
	uint64_t hi = pcie_conf_read(bdf, reg + 1);

	return hi << 32 | lo;
}

static bool ivshmem_configure(const struct device *dev)
{
	struct ivshmem *data = dev->data;
	struct pcie_bar mbar_regs, mbar_msi_x, mbar_shmem;

	if (!pcie_get_mbar(data->pcie->bdf, IVSHMEM_PCIE_REG_BAR_IDX, &mbar_regs)) {
		if (IS_ENABLED(CONFIG_IVSHMEM_DOORBELL)
			IF_ENABLED(CONFIG_IVSHMEM_V2, (|| data->ivshmem_v2))) {
			LOG_ERR("ivshmem regs bar not found");
			return false;
		}
		LOG_INF("ivshmem regs bar not found");
		device_map(DEVICE_MMIO_RAM_PTR(dev), (uintptr_t)&no_reg,
			   sizeof(struct ivshmem_reg), K_MEM_CACHE_NONE);
	} else {
		pcie_set_cmd(data->pcie->bdf, PCIE_CONF_CMDSTAT_MEM |
			PCIE_CONF_CMDSTAT_MASTER, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar_regs.phys_addr,
			   mbar_regs.size, K_MEM_CACHE_NONE);
	}

	bool msi_x_bar_present = pcie_get_mbar(
		data->pcie->bdf, IVSHMEM_PCIE_MSI_X_BAR_IDX, &mbar_msi_x);
	bool shmem_bar_present = pcie_get_mbar(
		data->pcie->bdf, IVSHMEM_PCIE_SHMEM_BAR_IDX, &mbar_shmem);

	LOG_INF("MSI-X bar present: %s", msi_x_bar_present ? "yes" : "no");
	LOG_INF("SHMEM bar present: %s", shmem_bar_present ? "yes" : "no");

	uintptr_t shmem_phys_addr = mbar_shmem.phys_addr;

#ifdef CONFIG_IVSHMEM_V2
	if (data->ivshmem_v2) {
		if (mbar_regs.size < sizeof(struct ivshmem_v2_reg)) {
			LOG_ERR("Invalid ivshmem regs size %zu", mbar_regs.size);
			return false;
		}

		volatile struct ivshmem_v2_reg *regs =
			(volatile struct ivshmem_v2_reg *)DEVICE_MMIO_GET(dev);

		data->max_peers = regs->max_peers;
		if (!IN_RANGE(data->max_peers, 2, 0x10000)) {
			LOG_ERR("Invalid max peers %u", data->max_peers);
			return false;
		}

		uint32_t vendor_cap = pcie_get_cap(data->pcie->bdf, PCI_CAP_ID_VNDR);
		uint32_t cap_pos;

		if (!shmem_bar_present) {
			cap_pos = vendor_cap + IVSHMEM_CFG_ADDRESS / 4;
			shmem_phys_addr = pcie_conf_read_u64(data->pcie->bdf, cap_pos);
		}

		cap_pos = vendor_cap + IVSHMEM_CFG_STATE_TAB_SZ / 4;
		size_t state_table_size = pcie_conf_read(data->pcie->bdf, cap_pos);

		LOG_INF("State table size 0x%zX", state_table_size);
		if (state_table_size < sizeof(uint32_t) * data->max_peers) {
			LOG_ERR("Invalid state table size %zu", state_table_size);
			return false;
		}

		cap_pos = vendor_cap + IVSHMEM_CFG_RW_SECTION_SZ / 4;
		data->rw_section_size = pcie_conf_read_u64(data->pcie->bdf, cap_pos);
		data->rw_section_offset = state_table_size;
		LOG_INF("RW section size 0x%zX", data->rw_section_size);

		cap_pos = vendor_cap + IVSHMEM_CFG_OUTPUT_SECTION_SZ / 4;
		data->output_section_size = pcie_conf_read_u64(data->pcie->bdf, cap_pos);
		data->output_section_offset = data->rw_section_offset + data->rw_section_size;
		LOG_INF("Output section size 0x%zX", data->output_section_size);

		data->size = data->output_section_offset +
			data->output_section_size * data->max_peers;

		/* Ensure one-shot ISR mode is disabled */
		cap_pos = vendor_cap + IVSHMEM_CFG_PRIV_CNTL / 4;
		uint32_t cfg_priv_cntl = pcie_conf_read(data->pcie->bdf, cap_pos);

		cfg_priv_cntl &= ~(IVSHMEM_PRIV_CNTL_ONESHOT_INT <<
			((IVSHMEM_CFG_PRIV_CNTL % 4) * 8));
		pcie_conf_write(data->pcie->bdf, cap_pos, cfg_priv_cntl);
	} else
#endif /* CONFIG_IVSHMEM_V2 */
	{
		if (!shmem_bar_present) {
			LOG_ERR("ivshmem mem bar not found");
			return false;
		}

		data->size = mbar_shmem.size;
	}

	z_phys_map((uint8_t **)&data->shmem,
		   shmem_phys_addr, data->size,
		   K_MEM_CACHE_WB | K_MEM_PERM_RW | K_MEM_PERM_USER);

	if (msi_x_bar_present) {
		if (!ivshmem_configure_msi_x_interrupts(dev)) {
			LOG_ERR("MSI-X init failed");
			return false;
		}
	}
#ifdef CONFIG_IVSHMEM_V2
	else if (data->ivshmem_v2) {
		if (!ivshmem_configure_int_x_interrupts(dev)) {
			LOG_ERR("INTx init failed");
			return false;
		}
	}
#endif

	LOG_INF("ivshmem configured:");
	LOG_INF("- Registers at 0x%lX (mapped to 0x%lX)",
		mbar_regs.phys_addr, DEVICE_MMIO_GET(dev));
	LOG_INF("- Shared memory of 0x%zX bytes at 0x%lX (mapped to 0x%lX)",
		data->size, shmem_phys_addr, data->shmem);

	return true;
}

static size_t ivshmem_api_get_mem(const struct device *dev,
				  uintptr_t *memmap)
{
	struct ivshmem *data = dev->data;

	*memmap = data->shmem;

	return data->size;
}

static uint32_t ivshmem_api_get_id(const struct device *dev)
{
	uint32_t id;

#ifdef CONFIG_IVSHMEM_V2
	struct ivshmem *data = dev->data;

	if (data->ivshmem_v2) {
		volatile struct ivshmem_v2_reg *regs =
			(volatile struct ivshmem_v2_reg *) DEVICE_MMIO_GET(dev);

		id = regs->id;
	} else
#endif
	{
		volatile struct ivshmem_reg *regs =
			(volatile struct ivshmem_reg *) DEVICE_MMIO_GET(dev);

		id = regs->iv_position;
	}

	return id;
}

static uint16_t ivshmem_api_get_vectors(const struct device *dev)
{
#if CONFIG_IVSHMEM_DOORBELL
	struct ivshmem *data = dev->data;

	return data->n_vectors;
#else
	return 0;
#endif
}

static int ivshmem_api_int_peer(const struct device *dev,
				uint32_t peer_id, uint16_t vector)
{
#if CONFIG_IVSHMEM_DOORBELL
	struct ivshmem *data = dev->data;
	volatile uint32_t *doorbell_reg;
	uint32_t doorbell = IVSHMEM_GEN_DOORBELL(peer_id, vector);

	if (vector >= data->n_vectors) {
		return -EINVAL;
	}

#ifdef CONFIG_IVSHMEM_V2
	if (data->ivshmem_v2 && peer_id >= data->max_peers) {
		return -EINVAL;
	}

	if (data->ivshmem_v2) {
		volatile struct ivshmem_v2_reg *regs =
			(volatile struct ivshmem_v2_reg *) DEVICE_MMIO_GET(dev);

		doorbell_reg = &regs->doorbell;
	} else
#endif
	{
		volatile struct ivshmem_reg *regs =
			(volatile struct ivshmem_reg *) DEVICE_MMIO_GET(dev);

		doorbell_reg = &regs->doorbell;
	}
	*doorbell_reg = doorbell;

	return 0;
#else
	return -ENOSYS;
#endif
}

static int ivshmem_api_register_handler(const struct device *dev,
					struct k_poll_signal *signal,
					uint16_t vector)
{
#if CONFIG_IVSHMEM_DOORBELL
	struct ivshmem *data = dev->data;

	if (vector >= data->n_vectors) {
		return -EINVAL;
	}

	register_signal(dev, signal, vector);

	return 0;
#else
	return -ENOSYS;
#endif
}

#ifdef CONFIG_IVSHMEM_V2

static size_t ivshmem_api_get_rw_mem_section(const struct device *dev,
					     uintptr_t *memmap)
{
	struct ivshmem *data = dev->data;

	if (!data->ivshmem_v2) {
		memmap = NULL;
		return 0;
	}

	*memmap = data->shmem + data->rw_section_offset;

	return data->rw_section_size;
}

static size_t ivshmem_api_get_output_mem_section(const struct device *dev,
						 uint32_t peer_id,
						 uintptr_t *memmap)
{
	struct ivshmem *data = dev->data;

	if (!data->ivshmem_v2 || peer_id >= data->max_peers) {
		memmap = NULL;
		return 0;
	}

	*memmap = data->shmem + data->output_section_offset +
		data->output_section_size * peer_id;

	return data->output_section_size;
}

static uint32_t ivshmem_api_get_state(const struct device *dev,
				      uint32_t peer_id)
{
	struct ivshmem *data = dev->data;

	if (!data->ivshmem_v2 || peer_id >= data->max_peers) {
		return 0;
	}

	const volatile uint32_t *state_table =
		(const volatile uint32_t *)data->shmem;

	return state_table[peer_id];
}

static int ivshmem_api_set_state(const struct device *dev,
				 uint32_t state)
{
	struct ivshmem *data = dev->data;

	if (!data->ivshmem_v2) {
		return -ENOSYS;
	}

	volatile struct ivshmem_v2_reg *regs =
		(volatile struct ivshmem_v2_reg *) DEVICE_MMIO_GET(dev);

	regs->state = state;

	return 0;
}

static uint32_t ivshmem_api_get_max_peers(const struct device *dev)
{
	struct ivshmem *data = dev->data;

	if (!data->ivshmem_v2) {
		return 0;
	}

	return data->max_peers;
}

static uint16_t ivshmem_api_get_protocol(const struct device *dev)
{
	struct ivshmem *data = dev->data;

	if (!data->ivshmem_v2) {
		return 0;
	}

	uint16_t protocol = (data->pcie->class_rev >> 8) & 0xFFFF;

	return protocol;
}

static int ivshmem_api_enable_interrupts(const struct device *dev,
					 bool enable)
{
	struct ivshmem *data = dev->data;

	if (!data->ivshmem_v2) {
		return -ENOSYS;
	}

	volatile struct ivshmem_v2_reg *regs =
		(volatile struct ivshmem_v2_reg *) DEVICE_MMIO_GET(dev);

	regs->int_control = enable ? IVSHMEM_INT_ENABLE : 0;

	return 0;
}

#endif /* CONFIG_IVSHMEM_V2 */

static const struct ivshmem_driver_api ivshmem_api = {
	.get_mem = ivshmem_api_get_mem,
	.get_id = ivshmem_api_get_id,
	.get_vectors = ivshmem_api_get_vectors,
	.int_peer = ivshmem_api_int_peer,
	.register_handler = ivshmem_api_register_handler,
#ifdef CONFIG_IVSHMEM_V2
	.get_rw_mem_section = ivshmem_api_get_rw_mem_section,
	.get_output_mem_section = ivshmem_api_get_output_mem_section,
	.get_state = ivshmem_api_get_state,
	.set_state = ivshmem_api_set_state,
	.get_max_peers = ivshmem_api_get_max_peers,
	.get_protocol = ivshmem_api_get_protocol,
	.enable_interrupts = ivshmem_api_enable_interrupts
#endif
};

static int ivshmem_init(const struct device *dev)
{
	struct ivshmem *data = dev->data;

	if (data->pcie->bdf == PCIE_BDF_NONE) {
		LOG_WRN("ivshmem device not found");
		return -ENOTSUP;
	}

	LOG_INF("PCIe: ID 0x%08X, BDF 0x%X, class-rev 0x%08X",
		data->pcie->id, data->pcie->bdf, data->pcie->class_rev);

	if (!ivshmem_configure(dev)) {
		return -EIO;
	}
	return 0;
}

#define IVSHMEM_INTX_INFO(intx_idx, drv_idx) { \
	COND_CODE_1(DT_IRQ_HAS_IDX(DT_DRV_INST(drv_idx), intx_idx), \
		( \
			.irq = DT_IRQ_BY_IDX(DT_DRV_INST(drv_idx), intx_idx, irq), \
			.priority = DT_IRQ_BY_IDX(DT_DRV_INST(drv_idx), intx_idx, priority), \
			.flags = DT_IRQ_BY_IDX(DT_DRV_INST(drv_idx), intx_idx, flags), \
		), \
		(.irq = INTX_IRQ_UNUSED)) \
	}

#define IVSHMEM_DEVICE_INIT(n) \
	BUILD_ASSERT(!IS_ENABLED(CONFIG_IVSHMEM_DOORBELL) || \
		((IS_ENABLED(CONFIG_PCIE_MSI_X) && \
			IS_ENABLED(CONFIG_PCIE_MSI_MULTI_VECTOR)) || \
		(DT_INST_PROP(n, ivshmem_v2) && \
			DT_INST_NODE_HAS_PROP(n, interrupts))), \
		"IVSHMEM_DOORBELL requires either MSI-X or INTx support"); \
	BUILD_ASSERT(IS_ENABLED(CONFIG_IVSHMEM_V2) || !DT_INST_PROP(n, ivshmem_v2), \
		"CONFIG_IVSHMEM_V2 must be enabled for ivshmem-v2"); \
	DEVICE_PCIE_INST_DECLARE(n); \
	static struct ivshmem ivshmem_data_##n = { \
		DEVICE_PCIE_INST_INIT(n, pcie), \
		IF_ENABLED(CONFIG_IVSHMEM_V2, \
			(.ivshmem_v2 = DT_INST_PROP(n, ivshmem_v2),)) \
	}; \
	IF_ENABLED(CONFIG_IVSHMEM_V2, ( \
		static struct ivshmem_cfg ivshmem_cfg_##n = { \
			.intx_info = \
			{ FOR_EACH_FIXED_ARG(IVSHMEM_INTX_INFO, (,), n, 0, 1, 2, 3) } \
		}; \
	)); \
	DEVICE_DT_INST_DEFINE(n, &ivshmem_init, NULL, \
			      &ivshmem_data_##n, \
			      COND_CODE_1(CONFIG_IVSHMEM_V2, (&ivshmem_cfg_##n), (NULL)), \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &ivshmem_api);

DT_INST_FOREACH_STATUS_OKAY(IVSHMEM_DEVICE_INIT)
