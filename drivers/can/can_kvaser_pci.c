/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT kvaser_pcican

#include <zephyr/drivers/can/can_sja1000.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(can_kvaser_pci, CONFIG_CAN_LOG_LEVEL);

/* AMCC S5920 I/O BAR registers */
#define S5920_INTCSR_REG       0x38
#define S5920_INTCSR_ADDINT_EN BIT(13)
#define S5920_PTCR_REG	       0x60

/* Xilinx I/O BAR registers */
#define XLNX_VERINT_REG		0x07
#define XLNX_VERINT_VERSION_POS 4U

struct can_kvaser_pci_config {
	void (*irq_config_func)(const struct device *dev);
	struct pcie_dev *pcie;
};

struct can_kvaser_pci_data {
	io_port_t sja1000_base;
};

static uint8_t can_kvaser_pci_read_reg(const struct device *dev, uint8_t reg)
{
	struct can_sja1000_data *sja1000_data = dev->data;
	struct can_kvaser_pci_data *kvaser_data = sja1000_data->custom;
	io_port_t addr = kvaser_data->sja1000_base + reg;

	return sys_in8(addr);
}

static void can_kvaser_pci_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	struct can_sja1000_data *sja1000_data = dev->data;
	struct can_kvaser_pci_data *kvaser_data = sja1000_data->custom;
	io_port_t addr = kvaser_data->sja1000_base + reg;

	sys_out8(val, addr);
}

static int can_kvaser_pci_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	/* The internal clock operates at half of the oscillator frequency */
	*rate = MHZ(16) / 2;

	return 0;
}

static int can_kvaser_pci_init(const struct device *dev)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_kvaser_pci_config *kvaser_config = sja1000_config->custom;
	struct can_sja1000_data *sja1000_data = dev->data;
	struct can_kvaser_pci_data *kvaser_data = sja1000_data->custom;
	struct pcie_bar iobar;
	static io_port_t amcc_base;
	static io_port_t xlnx_base;
	uint32_t intcsr;
	int err;

	if (kvaser_config->pcie->bdf == PCIE_BDF_NONE) {
		LOG_ERR("failed to find PCIe device");
		return -ENODEV;
	}

	pcie_set_cmd(kvaser_config->pcie->bdf, PCIE_CONF_CMDSTAT_IO, true);

	/* AMCC S5920 registers */
	if (!pcie_probe_iobar(kvaser_config->pcie->bdf, 0, &iobar)) {
		LOG_ERR("failed to probe AMCC S5920 I/O BAR");
		return -ENODEV;
	}

	amcc_base = iobar.phys_addr;

	/* SJA1000 registers */
	if (!pcie_probe_iobar(kvaser_config->pcie->bdf, 1, &iobar)) {
		LOG_ERR("failed to probe SJA1000 I/O BAR");
		return -ENODEV;
	}

	kvaser_data->sja1000_base = iobar.phys_addr;

	/* Xilinx registers */
	if (!pcie_probe_iobar(kvaser_config->pcie->bdf, 2, &iobar)) {
		LOG_ERR("failed to probe Xilinx I/O BAR");
		return -ENODEV;
	}

	xlnx_base = iobar.phys_addr;
	LOG_DBG("Xilinx version: %d",
		sys_in8(xlnx_base + XLNX_VERINT_REG) >> XLNX_VERINT_VERSION_POS);

	/*
	 * Initialization sequence as per Kvaser PCIcan Hardware Reference Manual (UG 98048
	 * v3.0.0).
	 */

	/* AMCC S5920 PCI Pass-Thru Configuration Register (PTCR) */
	sys_out32(0x80808080UL, amcc_base + S5920_PTCR_REG);

	/* AMCC S5920 PCI Interrupt Control/Status Register (INTCSR) */
	intcsr = sys_in32(amcc_base + S5920_INTCSR_REG);
	intcsr |= S5920_INTCSR_ADDINT_EN;
	sys_out32(intcsr, amcc_base + S5920_INTCSR_REG);

	err = can_sja1000_init(dev);
	if (err != 0) {
		LOG_ERR("failed to initialize controller (err %d)", err);
		return err;
	}

	kvaser_config->irq_config_func(dev);

	return 0;
}

const struct can_driver_api can_kvaser_pci_driver_api = {
	.get_capabilities = can_sja1000_get_capabilities,
	.start = can_sja1000_start,
	.stop = can_sja1000_stop,
	.set_mode = can_sja1000_set_mode,
	.set_timing = can_sja1000_set_timing,
	.send = can_sja1000_send,
	.add_rx_filter = can_sja1000_add_rx_filter,
	.remove_rx_filter = can_sja1000_remove_rx_filter,
	.get_state = can_sja1000_get_state,
	.set_state_change_callback = can_sja1000_set_state_change_callback,
	.get_core_clock = can_kvaser_pci_get_core_clock,
	.get_max_filters = can_sja1000_get_max_filters,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_sja1000_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.timing_min = CAN_SJA1000_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_SJA1000_TIMING_MAX_INITIALIZER,
};

#define CAN_KVASER_PCI_OCR                                                                         \
	(CAN_SJA1000_OCR_OCMODE_NORMAL | CAN_SJA1000_OCR_OCTN0 | CAN_SJA1000_OCR_OCTP0 |           \
	 CAN_SJA1000_OCR_OCTN1 | CAN_SJA1000_OCR_OCTP1)

#define CAN_KVASER_PCI_CDR (CAN_SJA1000_CDR_CD_DIV2 | CAN_SJA1000_CDR_CLOCK_OFF)

#define CAN_KVASER_PCI_INIT(inst)                                                                  \
	static void can_kvaser_pci_config_func_##inst(const struct device *dev);                   \
	DEVICE_PCIE_INST_DECLARE(inst);                                                            \
                                                                                                   \
	static const struct can_kvaser_pci_config can_kvaser_pci_config_##inst = {                 \
		DEVICE_PCIE_INST_INIT(inst, pcie),                                                 \
		.irq_config_func = can_kvaser_pci_config_func_##inst                               \
	};                                                                                         \
                                                                                                   \
	static const struct can_sja1000_config can_sja1000_config_##inst =                         \
		CAN_SJA1000_DT_CONFIG_INST_GET(inst, &can_kvaser_pci_config_##inst,                \
					       can_kvaser_pci_read_reg, can_kvaser_pci_write_reg,  \
					       CAN_KVASER_PCI_OCR, CAN_KVASER_PCI_CDR, 0);         \
                                                                                                   \
	static struct can_kvaser_pci_data can_kvaser_pci_data_##inst;                              \
                                                                                                   \
	static struct can_sja1000_data can_sja1000_data_##inst =                                   \
		CAN_SJA1000_DATA_INITIALIZER(&can_kvaser_pci_data_##inst);                         \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_kvaser_pci_init, NULL, &can_sja1000_data_##inst,       \
				  &can_sja1000_config_##inst, POST_KERNEL,                         \
				  CONFIG_CAN_INIT_PRIORITY, &can_kvaser_pci_driver_api);           \
                                                                                                   \
	static void can_kvaser_pci_config_func_##inst(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), can_sja1000_isr,      \
			    DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, sense));                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

DT_INST_FOREACH_STATUS_OKAY(CAN_KVASER_PCI_INIT)
