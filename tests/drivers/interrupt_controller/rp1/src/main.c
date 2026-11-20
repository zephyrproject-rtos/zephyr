/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/msi_proxy.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/dt-bindings/interrupt-controller/rp1_irq.h>
#include <zephyr/ztest.h>

#if DT_NODE_EXISTS(DT_NODELABEL(test_msi))
struct fake_msi_proxy_data {
	unsigned int compose_calls;
	unsigned int enable_calls;
	unsigned int disable_calls;
};

static int fake_msi_compose(const struct device *dev, unsigned int channel,
			    struct msi_proxy_message *msg)
{
	struct fake_msi_proxy_data *data = dev->data;

	if (channel >= RP1_NUM_IRQS || msg == NULL) {
		return -EINVAL;
	}

	data->compose_calls++;
	msg->address = 0xfee00000U + (channel * 4U);
	msg->data = 0x200U + channel;

	return 0;
}

static int fake_msi_enable(const struct device *dev, unsigned int channel)
{
	struct fake_msi_proxy_data *data = dev->data;

	ARG_UNUSED(channel);
	data->enable_calls++;

	return 0;
}

static void fake_msi_disable(const struct device *dev, unsigned int channel)
{
	struct fake_msi_proxy_data *data = dev->data;

	ARG_UNUSED(channel);
	data->disable_calls++;
}

static DEVICE_API(msi_proxy, fake_msi_proxy_api) = {
	.compose = fake_msi_compose,
	.enable = fake_msi_enable,
	.disable = fake_msi_disable,
};

static struct fake_msi_proxy_data test_msi_data;

DEVICE_DT_DEFINE(DT_NODELABEL(test_msi), NULL, NULL, &test_msi_data, NULL, POST_KERNEL, 0,
		 &fake_msi_proxy_api);
#endif
DEVICE_DT_DEFINE(DT_NODELABEL(test_pcie), NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

/* Inspect the real driver's register writes without a PCIe device. */
#include "intc_rp1.c"

struct fake_pcie_data {
	uint32_t cmdstat;
	uint32_t msix_mcr;
	uint32_t table_data_at_mask;
	uint32_t table_data_at_unmask;
	uint32_t msix_tr;
	uint32_t bar0;
	bool bar0_probe;
	struct msix_vector msix_table[RP1_NUM_IRQS];
};

static uint32_t fake_pcie_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	struct fake_pcie_data *data = dev->data;

	if (bdf == PCIE_BDF(0, 0, 0)) {
		if (reg == PCIE_CONF_ID) {
			return 0x12345678U;
		}
		if (reg == PCIE_CONF_TYPE) {
			return 0U;
		}
		return 0xffffffffU;
	}

	if (bdf == PCIE_BDF(0, 1, 0)) {
		switch (reg) {
		case PCIE_CONF_ID:
			return PCIE_ID(RP1_PCIE_VENDOR, RP1_PCIE_DEVICE);
		case PCIE_CONF_CMDSTAT:
			return data->cmdstat;
		case PCIE_CONF_TYPE:
			return 0U;
		case PCIE_CONF_BAR0:
			return data->bar0_probe ? 0xfffff000U : data->bar0;
		case PCIE_CONF_CAPPTR:
			return 0x40U;
		case 0x10U:
			return PCI_CAP_ID_MSIX | data->msix_mcr;
		case 0x10U + PCIE_MSIX_TR:
			return data->msix_tr;
		default:
			return 0U;
		}
	}

	return 0xffffffffU;
}

static void fake_pcie_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
				 uint32_t val)
{
	struct fake_pcie_data *data = dev->data;

	if (bdf != PCIE_BDF(0, 1, 0)) {
		return;
	}

	switch (reg) {
	case PCIE_CONF_CMDSTAT:
		/* The capability-list bit in the status word is read-only. */
		data->cmdstat = (data->cmdstat & ~(uint32_t)UINT16_MAX) | (val & UINT16_MAX);
		break;
	case PCIE_CONF_BAR0:
		if (val == 0xffffffffU) {
			data->bar0_probe = true;
		} else {
			data->bar0_probe = false;
			data->bar0 = val;
		}
		break;
	case 0x10U + PCIE_MSIX_MCR:
		if ((val & PCIE_MSIX_MCR_FMASK) != 0U) {
			data->table_data_at_mask = data->msix_table[0].msg_data;
		} else {
			data->table_data_at_unmask = data->msix_table[RP1_NUM_IRQS - 1U].msg_data;
		}
		data->msix_mcr = val;
		break;
	default:
		break;
	}
}

static bool fake_pcie_translate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64,
				uintptr_t bar_bus_addr, uintptr_t *bar_phys_addr)
{
	struct fake_pcie_data *data = dev->data;

	ARG_UNUSED(bdf);
	ARG_UNUSED(mem);
	ARG_UNUSED(mem64);

	if (bar_bus_addr == 0x10000000U) {
		*bar_phys_addr = (uintptr_t)data->msix_table;
		return true;
	}

	return false;
}

static DEVICE_API(pcie_ctrl, fake_pcie_api) = {
	.conf_read = fake_pcie_conf_read,
	.conf_write = fake_pcie_conf_write,
	.region_translate = fake_pcie_translate,
};

ZTEST(rp1_intc, test_unready_pcie)
{
	uint32_t unused_reg;
	struct device_state pcie_state = {0};
	const struct device pcie = {
		.state = &pcie_state,
	};
	const struct rp1_irq_config config = {
		._mmio = {.phys_addr = (uintptr_t)&unused_reg, .size = sizeof(unused_reg)},
		.pcie = &pcie,
		.num_irqs = RP1_NUM_IRQS,
	};
	struct rp1_irq_data data = {0};
	const struct device dev = {
		.config = &config,
		.data = &data,
	};

	zassert_equal(rp1_irq_init(&dev), -ENODEV);
	zassert_false(data.intc.ready);
	zassert_equal(data._mmio, 0U);
}

ZTEST(rp1_intc, test_source_trigger_types)
{
	uint32_t registers[0x1000U / sizeof(uint32_t)] = {0};
	struct rp1_irq_data data = {
		._mmio = (mm_reg_t)registers,
	};
	const struct device dev = {
		.data = &data,
	};

	rp1_intc_setup(&dev, RP1_NUM_IRQS);

	for (uint32_t line = 0; line < RP1_NUM_IRQS; line++) {
		bool edge = line >= RP1_IRQ_USBHOST0 && line <= RP1_IRQ_USBHOST1_3;
		uint32_t offset = RP1_MSIX_CFG(line);
		uint32_t cfg = registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)];

		zassert_equal(registers[(RP1_MSIX_CFG_CLR + offset) / sizeof(uint32_t)],
			      RP1_MSIX_CFG_ENABLE | RP1_MSIX_CFG_IACK_EN,
			      "Vector %u was not disabled and cleared", line);
		zassert_equal(cfg, edge ? 0U : RP1_MSIX_CFG_IACK_EN,
			      "Vector %u has the wrong IACK mode", line);

		/* Model a masked source whose trigger type has already been set. */
		registers[offset / sizeof(uint32_t)] = cfg;
		registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)] = 0U;
		rp1_irq_enable(&dev, line);
		zassert_equal(registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)],
			      edge ? RP1_MSIX_CFG_ENABLE : RP1_MSIX_CFG_IACK,
			      "Vector %u has the wrong activation behavior", line);

		rp1_irq_disable(&dev, line);
		rp1_irq_enable(&dev, line);
		zassert_equal(registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)],
			      RP1_MSIX_CFG_ENABLE, "Vector %u received another IACK on re-enable",
			      line);

		registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)] = 0U;
		rp1_irq_eoi(&dev, line);
		zassert_equal(registers[(RP1_MSIX_CFG_SET + offset) / sizeof(uint32_t)],
			      edge ? 0U : RP1_MSIX_CFG_IACK, "Vector %u has the wrong EOI behavior",
			      line);
	}
}

#if DT_NODE_EXISTS(DT_NODELABEL(test_msi))
ZTEST(rp1_intc, test_static_msi_proxy_backend)
{
	const unsigned int edge_vector = RP1_IRQ_USBHOST0;
	const uint32_t initial_mcr =
		PCIE_MSIX_MCR_EN | ((RP1_NUM_IRQS - 1U) << PCIE_MSIX_MCR_TSIZE_SHIFT);
	struct fake_pcie_data pcie_data = {
		.cmdstat = PCIE_CONF_CMDSTAT_CAPS,
		.msix_mcr = initial_mcr,
		.bar0 = 0x10000000U,
		.msix_table[0].msg_data = 0x123U,
	};
	struct device_state pcie_state = {
		.initialized = true,
		.init_res = 0,
	};
	const struct device pcie = {
		.state = &pcie_state,
		.data = &pcie_data,
		.api = &fake_pcie_api,
	};
	uint32_t registers[0x1000U / sizeof(uint32_t)] = {0};
	const struct rp1_irq_config config = {
		._mmio = {.phys_addr = (uintptr_t)registers, .size = sizeof(registers)},
		.pcie = &pcie,
		.num_irqs = RP1_NUM_IRQS,
	};
	struct rp1_irq_data data = {0};
	const struct device dev = {
		.config = &config,
		.data = &data,
	};

	/* The table starts inside BAR0 but its end lies outside the BAR. */
	pcie_data.msix_tr = 0xf80U;
	zassert_equal(rp1_irq_init(&dev), -EINVAL);
	zassert_false(data.intc.ready);
	zassert_equal(data._mmio, 0U);
	zassert_equal(pcie_data.cmdstat, PCIE_CONF_CMDSTAT_CAPS);
	zassert_equal(pcie_data.msix_mcr, initial_mcr);
	zassert_equal(pcie_data.msix_table[0].msg_data, 0x123U);

	pcie_data.msix_tr = 0U;
	memset(&test_msi_data, 0, sizeof(test_msi_data));
	zassert_ok(rp1_irq_init(&dev));
	zassert_true(data.intc.ready);
	zassert_equal(pcie_data.cmdstat,
		      PCIE_CONF_CMDSTAT_CAPS | PCIE_CONF_CMDSTAT_MEM | PCIE_CONF_CMDSTAT_MASTER);
	zassert_equal(test_msi_data.compose_calls, RP1_NUM_IRQS);
	zassert_equal(pcie_data.table_data_at_mask, 0x123U);
	zassert_equal(pcie_data.table_data_at_unmask, 0x200U + RP1_NUM_IRQS - 1U);
	zassert_not_equal(pcie_data.msix_mcr & PCIE_MSIX_MCR_EN, 0U);
	zassert_equal(pcie_data.msix_mcr & PCIE_MSIX_MCR_FMASK, 0U);
	zassert_equal(pcie_data.msix_table[5].msg_addr, 0xfee00000U + (5U * 4U));
	zassert_equal(pcie_data.msix_table[5].msg_data, 0x205U);

	rp1_irq_enable(&dev, edge_vector);
	zassert_equal(registers[(RP1_MSIX_CFG_SET + RP1_MSIX_CFG(edge_vector)) / sizeof(uint32_t)],
		      RP1_MSIX_CFG_ENABLE);

	rp1_irq_disable(&dev, edge_vector);
	zassert_equal(registers[(RP1_MSIX_CFG_CLR + RP1_MSIX_CFG(edge_vector)) / sizeof(uint32_t)],
		      RP1_MSIX_CFG_ENABLE);

	/* Invalid vector numbers must be ignored. */
	rp1_irq_enable(&dev, RP1_NUM_IRQS);
	rp1_irq_disable(&dev, RP1_NUM_IRQS);
	zassert_equal(registers[(RP1_MSIX_CFG_SET + RP1_MSIX_CFG(RP1_NUM_IRQS)) / sizeof(uint32_t)],
		      0U);
	zassert_equal(registers[(RP1_MSIX_CFG_CLR + RP1_MSIX_CFG(RP1_NUM_IRQS)) / sizeof(uint32_t)],
		      0U);
	zassert_equal(test_msi_data.enable_calls, 0U);
	zassert_equal(test_msi_data.disable_calls, 0U);
}
#endif

ZTEST_SUITE(rp1_intc, NULL, NULL, NULL, NULL, NULL);
