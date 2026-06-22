/*
 * Copyright (c) 2025 Calian Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/edac.h>
#include <zephyr/drivers/edac/edac_synopsys.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(edac_synopsys, CONFIG_EDAC_LOG_LEVEL);

enum edac_synopsys_reg {
	SYNOPSYS_ECCCFG0 = 0x70,
	SYNOPSYS_ECCCFG1 = 0x74,
	SYNOPSYS_ECCCLR = 0x7C,
	SYNOPSYS_ECCERRCNT = 0x80,
	SYNOPSYS_ECCCADDR0 = 0x84,
	SYNOPSYS_ECCCADDR1 = 0x88,
	SYNOPSYS_ECCCSYN0 = 0x8C,
	SYNOPSYS_ECCCSYN1 = 0x90,
	SYNOPSYS_ECCCSYN2 = 0x94,
	SYNOPSYS_ECCCBITMASK0 = 0x98,
	SYNOPSYS_ECCCBITMASK1 = 0x9C,
	SYNOPSYS_ECCCBITMASK2 = 0xA0,
	SYNOPSYS_ECCUADDR0 = 0xA4,
	SYNOPSYS_ECCUADDR1 = 0xA8,
	SYNOPSYS_ECCUSYN0 = 0xAC,
	SYNOPSYS_ECCUSYN1 = 0xB0,
	SYNOPSYS_ECCUSYN2 = 0xB4,
	SYNOPSYS_ECCPOISONADDR0 = 0xB8,
	SYNOPSYS_ECCPOISONADDR1 = 0xBC,
	SYNOPSYS_SWCTL = 0x320,
};

#define SYNOPSYS_ECCCFG0_MODE_MASK    GENMASK(2, 0)
#define SYNOPSYS_ECCCFG0_MODE_SHIFT   0
#define SYNOPSYS_ECCCFG0_MODE_DISABLE 0
#define SYNOPSYS_ECCCFG0_MODE_ENABLE  4

#define SYNOPSYS_ECCCFG1_POISON_CORR_ERR_MASK BIT(1)
#define SYNOPSYS_ECCCFG1_POISON_ENABLE_BIT    BIT(0)

#define SYNOPSYS_ECCCLR_CLR_UNCORR_ERR_CNT_MASK BIT(3)
#define SYNOPSYS_ECCCLR_CLR_CORR_ERR_CNT_MASK   BIT(2)
#define SYNOPSYS_ECCCLR_CLR_UNCORR_ERR_MASK     BIT(1)
#define SYNOPSYS_ECCCLR_CLR_CORR_ERR_MASK       BIT(0)

#define SYNOPSYS_ECCERRCNT_UNCORR_MASK  GENMASK(31, 16)
#define SYNOPSYS_ECCERRCNT_UNCORR_SHIFT 16
#define SYNOPSYS_ECCERRCNT_CORR_MASK    GENMASK(15, 0)
#define SYNOPSYS_ECCERRCNT_CORR_SHIFT   0

#define SYNOPSYS_ECCADDR0_RANK_MASK  BIT(24)
#define SYNOPSYS_ECCADDR0_RANK_SHIFT 24
#define SYNOPSYS_ECCADDR0_ROW_MASK   GENMASK(17, 0)
#define SYNOPSYS_ECCADDR0_ROW_SHIFT  0

#define SYNOPSYS_ECCADDR1_BG_MASK    GENMASK(25, 24)
#define SYNOPSYS_ECCADDR1_BG_SHIFT   24
#define SYNOPSYS_ECCADDR1_BANK_MASK  GENMASK(18, 16)
#define SYNOPSYS_ECCADDR1_BANK_SHIFT 16
#define SYNOPSYS_ECCADDR1_COL_MASK   GENMASK(11, 0)
#define SYNOPSYS_ECCADDR1_COL_SHIFT  0

#define SYNOPSYS_SWCTL_DONE_MASK BIT(0)

#define ZYNQMP_QOS_REG_OFFSET 0x20000

enum edac_synopsys_qos_reg {
	SYNOPSYS_QOS_IRQ_STATUS = 0x200,
	SYNOPSYS_QOS_IRQ_ENABLE = 0x208,
};

#define SYNOPSYS_QOS_IRQ_ECC_UNC_MASK BIT(2)
#define SYNOPSYS_QOS_IRQ_ECC_COR_MASK BIT(1)

struct edac_synopsys_config {
	mem_addr_t reg;
	mem_addr_t qos_reg;
	void (*irq_config_func)(const struct device *dev);
};

struct edac_synopsys_data {
	edac_notify_callback_f cb;
#ifdef CONFIG_EDAC_ERROR_INJECT
	uint32_t inject_error_type;
#endif /* CONFIG_EDAC_ERROR_INJECT */
};

static void edac_synopsys_write_reg(const struct device *dev, enum edac_synopsys_reg reg,
				    uint32_t value)
{
	const struct edac_synopsys_config *config = dev->config;
	mem_addr_t reg_addr = config->reg + reg;

	sys_write32(value, reg_addr);
}

static uint32_t edac_synopsys_read_reg(const struct device *dev, enum edac_synopsys_reg reg)
{
	const struct edac_synopsys_config *config = dev->config;
	mem_addr_t reg_addr = config->reg + reg;

	return sys_read32(reg_addr);
}

static void edac_synopsys_write_qos_reg(const struct device *dev, enum edac_synopsys_qos_reg reg,
					uint32_t value)
{
	const struct edac_synopsys_config *config = dev->config;
	mem_addr_t reg_addr = config->qos_reg + reg;

	sys_write32(value, reg_addr);
}

static uint32_t edac_synopsys_qos_read_reg(const struct device *dev, enum edac_synopsys_qos_reg reg)
{
	const struct edac_synopsys_config *config = dev->config;
	mem_addr_t reg_addr = config->qos_reg + reg;

	return sys_read32(reg_addr);
}

#ifdef CONFIG_EDAC_ERROR_INJECT

static int edac_synopsys_inject_set_param1(const struct device *dev, uint64_t addr)
{
	/**
	 * Bit 24: Poison location rank
	 * Bits 0-11: Poison location column
	 */
	edac_synopsys_write_reg(dev, SYNOPSYS_ECCPOISONADDR0, (uint32_t)addr);
	return 0;
}

static int edac_synopsys_inject_get_param1(const struct device *dev, uint64_t *value)
{
	*value = edac_synopsys_read_reg(dev, SYNOPSYS_ECCPOISONADDR0);
	return 0;
}

static int edac_synopsys_inject_set_param2(const struct device *dev, uint64_t mask)
{
	/**
	 * Bits 29-28: Poison location bank group
	 * Bits 26-24: Poison location bank
	 * Bits 17-0: Poison location row
	 */
	edac_synopsys_write_reg(dev, SYNOPSYS_ECCPOISONADDR1, (uint32_t)mask);
	return 0;
}

static int edac_synopsys_inject_get_param2(const struct device *dev, uint64_t *value)
{
	*value = edac_synopsys_read_reg(dev, SYNOPSYS_ECCPOISONADDR1);
	return 0;
}

static int edac_synopsys_inject_set_error_type(const struct device *dev, uint32_t error_type)
{
	struct edac_synopsys_data *data = dev->data;

	data->inject_error_type = error_type;
	return 0;
}

static int edac_synopsys_inject_get_error_type(const struct device *dev, uint32_t *error_type)
{
	struct edac_synopsys_data *data = dev->data;

	*error_type = data->inject_error_type;
	return 0;
}

static int edac_synopsys_inject_error_trigger(const struct device *dev)
{
	struct edac_synopsys_data *data = dev->data;
	uint32_t ecccfg1;

	switch (data->inject_error_type) {
	case EDAC_ERROR_TYPE_DRAM_COR:
		ecccfg1 =
			SYNOPSYS_ECCCFG1_POISON_CORR_ERR_MASK | SYNOPSYS_ECCCFG1_POISON_ENABLE_BIT;
		break;
	case EDAC_ERROR_TYPE_DRAM_UC:
		ecccfg1 = SYNOPSYS_ECCCFG1_POISON_ENABLE_BIT;
		break;
	default:
		/* clear error injection */
		ecccfg1 = 0;
		break;
	}
	edac_synopsys_write_reg(dev, SYNOPSYS_SWCTL, 0);
	edac_synopsys_write_reg(dev, SYNOPSYS_ECCCFG1, ecccfg1);
	edac_synopsys_write_reg(dev, SYNOPSYS_SWCTL, SYNOPSYS_SWCTL_DONE_MASK);

	return 0;
}

#endif /* CONFIG_EDAC_ERROR_INJECT */

static int edac_synopsys_errors_cor_get(const struct device *dev)
{
	uint32_t eccerrcnt = edac_synopsys_read_reg(dev, SYNOPSYS_ECCERRCNT);

	return ((eccerrcnt & SYNOPSYS_ECCERRCNT_CORR_MASK) >> SYNOPSYS_ECCERRCNT_CORR_SHIFT);
}

static int edac_synopsys_errors_uc_get(const struct device *dev)
{
	uint32_t eccerrcnt = edac_synopsys_read_reg(dev, SYNOPSYS_ECCERRCNT);

	return ((eccerrcnt & SYNOPSYS_ECCERRCNT_UNCORR_MASK) >> SYNOPSYS_ECCERRCNT_UNCORR_SHIFT);
}

static int edac_synopsys_notify_callback_set(const struct device *dev, edac_notify_callback_f cb)
{
	struct edac_synopsys_data *data = dev->data;

	data->cb = cb;
	return 0;
}

static void edac_synopsys_isr(const struct device *dev)
{
	struct edac_synopsys_data *data = dev->data;
	const uint32_t int_status = edac_synopsys_qos_read_reg(dev, SYNOPSYS_QOS_IRQ_STATUS);

	if (int_status & (SYNOPSYS_QOS_IRQ_ECC_UNC_MASK | SYNOPSYS_QOS_IRQ_ECC_COR_MASK)) {
		const uint32_t eccerrcnt = edac_synopsys_read_reg(dev, SYNOPSYS_ECCERRCNT);
		const uint32_t ecccaddr0 = edac_synopsys_read_reg(dev, SYNOPSYS_ECCCADDR0);
		const uint32_t ecccaddr1 = edac_synopsys_read_reg(dev, SYNOPSYS_ECCCADDR1);
		const uint32_t eccuaddr0 = edac_synopsys_read_reg(dev, SYNOPSYS_ECCUADDR0);
		const uint32_t eccuaddr1 = edac_synopsys_read_reg(dev, SYNOPSYS_ECCUADDR1);

		struct edac_synopsys_callback_data cb_data = {
			.corr_err_count = (eccerrcnt & SYNOPSYS_ECCERRCNT_CORR_MASK) >>
					  SYNOPSYS_ECCERRCNT_CORR_SHIFT,
			.corr_err_rank = (ecccaddr0 & SYNOPSYS_ECCADDR0_RANK_MASK) >>
					 SYNOPSYS_ECCADDR0_RANK_SHIFT,
			.corr_err_row = (ecccaddr0 & SYNOPSYS_ECCADDR0_ROW_MASK) >>
					SYNOPSYS_ECCADDR0_ROW_SHIFT,
			.corr_err_bg = (ecccaddr1 & SYNOPSYS_ECCADDR1_BG_MASK) >>
				       SYNOPSYS_ECCADDR1_BG_SHIFT,
			.corr_err_bank = (ecccaddr1 & SYNOPSYS_ECCADDR1_BANK_MASK) >>
					 SYNOPSYS_ECCADDR1_BANK_SHIFT,
			.corr_err_col = (ecccaddr1 & SYNOPSYS_ECCADDR1_COL_MASK) >>
					SYNOPSYS_ECCADDR1_COL_SHIFT,
			.corr_err_syndrome =
				((uint64_t)edac_synopsys_read_reg(dev, SYNOPSYS_ECCCSYN1) << 32) |
				edac_synopsys_read_reg(dev, SYNOPSYS_ECCCSYN0),
			.corr_err_syndrome_ecc =
				(uint8_t)edac_synopsys_read_reg(dev, SYNOPSYS_ECCCSYN2),
			.corr_err_bitmask =
				((uint64_t)edac_synopsys_read_reg(dev, SYNOPSYS_ECCCBITMASK1)
				 << 32) |
				edac_synopsys_read_reg(dev, SYNOPSYS_ECCCBITMASK0),
			.corr_err_bitmask_ecc =
				(uint8_t)edac_synopsys_read_reg(dev, SYNOPSYS_ECCCBITMASK2),
			.uncorr_err_count = (eccerrcnt & SYNOPSYS_ECCERRCNT_UNCORR_MASK) >>
					    SYNOPSYS_ECCERRCNT_UNCORR_SHIFT,
			.uncorr_err_rank = (eccuaddr0 & SYNOPSYS_ECCADDR0_RANK_MASK) >>
					   SYNOPSYS_ECCADDR0_RANK_SHIFT,
			.uncorr_err_row = (eccuaddr0 & SYNOPSYS_ECCADDR0_ROW_MASK) >>
					  SYNOPSYS_ECCADDR0_ROW_SHIFT,
			.uncorr_err_bg = (eccuaddr1 & SYNOPSYS_ECCADDR1_BG_MASK) >>
					 SYNOPSYS_ECCADDR1_BG_SHIFT,
			.uncorr_err_bank = (eccuaddr1 & SYNOPSYS_ECCADDR1_BANK_MASK) >>
					   SYNOPSYS_ECCADDR1_BANK_SHIFT,
			.uncorr_err_col = (eccuaddr1 & SYNOPSYS_ECCADDR1_COL_MASK) >>
					  SYNOPSYS_ECCADDR1_COL_SHIFT,
			.uncorr_err_syndrome =
				((uint64_t)edac_synopsys_read_reg(dev, SYNOPSYS_ECCUSYN1) << 32) |
				edac_synopsys_read_reg(dev, SYNOPSYS_ECCUSYN0),
			.uncorr_err_syndrome_ecc =
				(uint8_t)edac_synopsys_read_reg(dev, SYNOPSYS_ECCUSYN2),
		};

		if (int_status & SYNOPSYS_QOS_IRQ_ECC_UNC_MASK) {
			/* Clear the last error */
			edac_synopsys_write_reg(dev, SYNOPSYS_ECCCLR,
						SYNOPSYS_ECCCLR_CLR_UNCORR_ERR_MASK);
			LOG_ERR("Uncorrectable ECC error detected: count: %u, last: rank %u, bg "
				"%u, bank %u, row %u, col %u, syndrome 0x%016llx, syndrome_ecc "
				"0x%02x",
				cb_data.uncorr_err_count, cb_data.uncorr_err_rank,
				cb_data.uncorr_err_bg, cb_data.uncorr_err_bank,
				cb_data.uncorr_err_row, cb_data.uncorr_err_col,
				cb_data.uncorr_err_syndrome, cb_data.uncorr_err_syndrome_ecc);
		}
		if (int_status & SYNOPSYS_QOS_IRQ_ECC_COR_MASK) {
			/* Clear the last error */
			edac_synopsys_write_reg(dev, SYNOPSYS_ECCCLR,
						SYNOPSYS_ECCCLR_CLR_CORR_ERR_MASK);
			LOG_WRN("Correctable ECC error detected: count: %u, last: rank %u, bg %u, "
				"bank %u, row %u, col %u, syndrome 0x%016llx, syndrome_ecc 0x%02x, "
				"bitmask "
				"0x%016llx, bitmask_ecc 0x%02x",
				cb_data.corr_err_count, cb_data.corr_err_rank, cb_data.corr_err_bg,
				cb_data.corr_err_bank, cb_data.corr_err_row, cb_data.corr_err_col,
				cb_data.corr_err_syndrome, cb_data.corr_err_syndrome_ecc,
				cb_data.corr_err_bitmask, cb_data.corr_err_bitmask_ecc);
		}
		/* Call the callback function if set */
		if (data->cb) {
			data->cb(dev, &cb_data);
		}
		/* Clear the interrupt status */
		edac_synopsys_write_qos_reg(dev, SYNOPSYS_QOS_IRQ_STATUS,
					    int_status & (SYNOPSYS_QOS_IRQ_ECC_UNC_MASK |
							  SYNOPSYS_QOS_IRQ_ECC_COR_MASK));
	}
}

static DEVICE_API(edac, edac_synopsys_api) = {
#ifdef CONFIG_EDAC_ERROR_INJECT
	/* Error Injection functions */
	.inject_set_param1 = edac_synopsys_inject_set_param1,
	.inject_get_param1 = edac_synopsys_inject_get_param1,
	.inject_set_param2 = edac_synopsys_inject_set_param2,
	.inject_get_param2 = edac_synopsys_inject_get_param2,
	.inject_set_error_type = edac_synopsys_inject_set_error_type,
	.inject_get_error_type = edac_synopsys_inject_get_error_type,
	.inject_error_trigger = edac_synopsys_inject_error_trigger,
#endif /* CONFIG_EDAC_ERROR_INJECT */

	/* Get error stats */
	.errors_cor_get = edac_synopsys_errors_cor_get,
	.errors_uc_get = edac_synopsys_errors_uc_get,

	/* Notification callback set */
	.notify_cb_set = edac_synopsys_notify_callback_set,
};

static int edac_synopsys_init(const struct device *dev)
{
	const struct edac_synopsys_config *config = dev->config;
	const uint32_t ecccfg0 = edac_synopsys_read_reg(dev, SYNOPSYS_ECCCFG0);
	const uint32_t ecc_mode =
		(ecccfg0 & SYNOPSYS_ECCCFG0_MODE_MASK) >> SYNOPSYS_ECCCFG0_MODE_SHIFT;

	if (ecc_mode != SYNOPSYS_ECCCFG0_MODE_ENABLE) {
		LOG_WRN("ECC is not enabled");
	}

	/* Clear and enable interrupts on ECC errors */
	edac_synopsys_write_qos_reg(dev, SYNOPSYS_QOS_IRQ_STATUS,
				    SYNOPSYS_QOS_IRQ_ECC_UNC_MASK | SYNOPSYS_QOS_IRQ_ECC_COR_MASK);

	config->irq_config_func(dev);

	edac_synopsys_write_qos_reg(dev, SYNOPSYS_QOS_IRQ_ENABLE,
				    SYNOPSYS_QOS_IRQ_ECC_UNC_MASK | SYNOPSYS_QOS_IRQ_ECC_COR_MASK);

	return 0;
}

#define DT_DRV_COMPAT xlnx_zynqmp_ddrc_2_40a

#define XLNX_ZYNQMP_DDRC_2_40A_INIT(n)                                                             \
	static void xlnx_zynqmp_ddrc_2_40a_config_func_##n(const struct device *dev)               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), edac_synopsys_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct edac_synopsys_config xlnx_zynqmp_ddrc_2_40a_config_##n = {             \
		.reg = DT_INST_REG_ADDR(n),                                                        \
		.qos_reg = DT_INST_REG_ADDR(n) + ZYNQMP_QOS_REG_OFFSET,                            \
		.irq_config_func = xlnx_zynqmp_ddrc_2_40a_config_func_##n};                        \
                                                                                                   \
	static struct edac_synopsys_data xlnx_zynqmp_ddrc_2_40a_data_##n;                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &edac_synopsys_init, NULL, &xlnx_zynqmp_ddrc_2_40a_data_##n,      \
			      &xlnx_zynqmp_ddrc_2_40a_config_##n, POST_KERNEL,                     \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &edac_synopsys_api);

DT_INST_FOREACH_STATUS_OKAY(XLNX_ZYNQMP_DDRC_2_40A_INIT)
