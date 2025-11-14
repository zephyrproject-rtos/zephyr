/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2021 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(can_sam, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT atmel_sam_can

enum can_memory_addr_cfg {
	/* CCFG_CANx gives the 16-bit MSB of the CAN DMA base address */
	CAN_MEM_ADDR_CFG_MSB16_HIGH,
	/* CANx accesses the lower or upper 64K SRAM controlled the bits in SFR_CAN_SRAM_SEL */
	CAN_MEM_ADDR_CFG_SRAM_SEL,
};

struct can_sam_config {
	mm_reg_t base;
	mem_addr_t mram;
	void (*config_irq)(void);
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	int divider;
	int instance;
	enum can_memory_addr_cfg mem_addr_cfg;
	mm_reg_t dma_base;
	mm_reg_t sram_sel;
};

static int can_sam_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_sam_config *sam_config = mcan_config->custom;

	return can_mcan_sys_read_reg(sam_config->base, reg, val);
}

static int can_sam_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_sam_config *sam_config = mcan_config->custom;

	return can_mcan_sys_write_reg(sam_config->base, reg, val);
}

static int can_sam_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_sam_config *sam_config = mcan_config->custom;

	return can_mcan_sys_read_mram(sam_config->mram, offset, dst, len);
}

static int can_sam_write_mram(const struct device *dev, uint16_t offset, const void *src,
				size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_sam_config *sam_config = mcan_config->custom;

	return can_mcan_sys_write_mram(sam_config->mram, offset, src, len);
}

static int can_sam_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_sam_config *sam_config = mcan_config->custom;

	return can_mcan_sys_clear_mram(sam_config->mram, offset, len);
}

static int can_sam_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_sam_config *sam_cfg = mcan_cfg->custom;

#ifdef CONFIG_SOC_SERIES_SAMX7X
	*rate = SOC_ATMEL_SAM_UPLLCK_FREQ_HZ / (sam_cfg->divider);

	return 0;
#else
	return clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				      (clock_control_subsys_t)&sam_cfg->clock_cfg,
				      rate);
#endif
}

static void can_sam_clock_enable(const struct can_sam_config *sam_cfg)
{
#ifdef CONFIG_SOC_SERIES_SAMX7X
	REG_PMC_PCK5 = PMC_PCK_CSS_UPLL_CLK | PMC_PCK_PRES(sam_cfg->divider - 1);
	PMC->PMC_SCER |= PMC_SCER_PCK5;

	/* Enable CAN clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&sam_cfg->clock_cfg);
#endif /* CONFIG_SOC_SERIES_SAMX7X */
}

static int can_sam_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_sam_config *sam_cfg = mcan_cfg->custom;
	int ret;

	can_sam_clock_enable(sam_cfg);

	ret = pinctrl_apply_state(sam_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* get actual message ram base address */
	uint32_t mrba = sam_cfg->mram & 0xFFFF0000U;

	switch (sam_cfg->mem_addr_cfg) {
	case CAN_MEM_ADDR_CFG_MSB16_HIGH:
		/* keep lower 16bit; update DMA Base Register */
		sys_write32((sys_read32(sam_cfg->dma_base) & 0x0000FFFF) | mrba,
			    sam_cfg->dma_base);
		break;
	case CAN_MEM_ADDR_CFG_SRAM_SEL:
		if (mrba < IRAM_ADDR + KB(64)) {
			sys_write32(sys_read32(sam_cfg->sram_sel) & (~BIT(sam_cfg->instance)),
				    sam_cfg->sram_sel);
		} else {
			sys_write32(sys_read32(sam_cfg->sram_sel) | BIT(sam_cfg->instance),
				    sam_cfg->sram_sel);
		}
		break;
	default:
		LOG_ERR("Configure CAN memory with an invalid method");
		return -EINVAL;
	}

	ret = can_mcan_configure_mram(dev, mrba, sam_cfg->mram);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	sam_cfg->config_irq();

	return ret;
}

static DEVICE_API(can, can_sam_driver_api) = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_core_clock = can_sam_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback =  can_mcan_set_state_change_callback,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops can_sam_ops = {
	.read_reg = can_sam_read_reg,
	.write_reg = can_sam_write_reg,
	.read_mram = can_sam_read_mram,
	.write_mram = can_sam_write_mram,
	.clear_mram = can_sam_clear_mram,
};

#define CAN_SAM_IRQ_CFG_FUNCTION(inst)                                                         \
static void config_can_##inst##_irq(void)                                                      \
{                                                                                              \
	LOG_DBG("Enable CAN##inst## IRQ");                                                     \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int0, irq),                                      \
		    DT_INST_IRQ_BY_NAME(inst, int0, priority), can_mcan_line_0_isr,            \
					DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, int0, irq));                                      \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int1, irq),                                      \
		    DT_INST_IRQ_BY_NAME(inst, int1, priority), can_mcan_line_1_isr,            \
					DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, int1, irq));                                      \
}

#define CAN_SAM_MRAM_DEFINE(inst)								\
		IF_ENABLED(DT_INST_REG_HAS_NAME(inst, dma_base),				\
			   (CAN_MCAN_DT_INST_MRAM_DEFINE(inst, can_sam_mram_##inst);))

#define CAN_SAM_MEMORY_DEFINE(inst)								\
		IF_ENABLED(DT_INST_REG_HAS_NAME(inst, dma_base),				\
			   (.mram = (mem_addr_t)POINTER_TO_UINT(&can_sam_mram_##inst),))	\
		IF_ENABLED(DT_INST_REG_HAS_NAME(inst, message_ram),				\
			   (.mram = (mm_reg_t)DT_INST_REG_ADDR_BY_NAME(inst, message_ram),))	\
												\
		IF_ENABLED(DT_INST_REG_HAS_NAME(inst, dma_base),				\
			   (.mem_addr_cfg = CAN_MEM_ADDR_CFG_MSB16_HIGH,))			\
		IF_ENABLED(DT_INST_REG_HAS_NAME(inst, dma_base),				\
			   (.dma_base = (mm_reg_t)DT_INST_REG_ADDR_BY_NAME(inst, dma_base),))	\
												\
		IF_ENABLED(DT_INST_REG_HAS_NAME(inst, sram_sel),				\
			   (.mem_addr_cfg = CAN_MEM_ADDR_CFG_SRAM_SEL,))			\
		IF_ENABLED(DT_INST_REG_HAS_NAME(inst, sram_sel),				\
			   (.sram_sel = (mm_reg_t)DT_INST_REG_ADDR_BY_NAME(inst, sram_sel),))

#define CAN_SAM_CLOCK_DIVIDER_DEFINE(inst)							\
		IF_ENABLED(CONFIG_SOC_SERIES_SAMX7X, (.divider = DT_INST_PROP(inst, divider),))

#define CAN_SAM_CFG_INST(inst)						\
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(inst, can_sam_cbs_##inst);	\
	CAN_SAM_MRAM_DEFINE(inst)					\
									\
	static const struct can_sam_config can_sam_cfg_##inst = {	\
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(inst),		\
		.instance = inst,					\
		CAN_SAM_MEMORY_DEFINE(inst)				\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),		\
		CAN_SAM_CLOCK_DIVIDER_DEFINE(inst)			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.config_irq = config_can_##inst##_irq,			\
	};								\
									\
	static const struct can_mcan_config can_mcan_cfg_##inst =	\
		CAN_MCAN_DT_CONFIG_INST_GET(inst, &can_sam_cfg_##inst,  \
					    &can_sam_ops,		\
					    &can_sam_cbs_##inst);

#define CAN_SAM_DATA_INST(inst)						\
	static struct can_mcan_data can_mcan_data_##inst =		\
		CAN_MCAN_DATA_INITIALIZER(NULL);

#define CAN_SAM_DEVICE_INST(inst)						\
	CAN_DEVICE_DT_INST_DEFINE(inst, can_sam_init, NULL,			\
				  &can_mcan_data_##inst,			\
				  &can_mcan_cfg_##inst,				\
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
				  &can_sam_driver_api);

#define CAN_SAM_INST(inst)                                                     \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(inst);                          \
	PINCTRL_DT_INST_DEFINE(inst);                                          \
	CAN_SAM_IRQ_CFG_FUNCTION(inst)                                         \
	CAN_SAM_CFG_INST(inst)                                                 \
	CAN_SAM_DATA_INST(inst)                                                \
	CAN_SAM_DEVICE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_SAM_INST)
