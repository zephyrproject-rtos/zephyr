/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_npu

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/reset.h>

#include <ethosu_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ethos_u_numaker, CONFIG_ETHOS_U_LOG_LEVEL);

#include <soc.h>

struct ethos_u_numaker_config {
	void *base_addr;
	const struct device *clkctrl_dev;
	struct numaker_scc_subsys pcc;
	struct reset_dt_spec reset;
	void (*irq_config)(const struct device *dev);
	bool secure_enable;
	bool privilege_enable;
	uint8_t flush_mask;
	uint8_t invalidate_mask;
};

struct ethos_u_numaker_data {
	struct ethosu_driver drv;
};

static void ethos_u_numaker_irq_handler(const struct device *dev)
{
	struct ethos_u_numaker_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;

	ethosu_irq_handler(drv);
}

static int ethos_u_numaker_init(const struct device *dev)
{
	const struct ethos_u_numaker_config *config = dev->config;
	struct ethos_u_numaker_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;
	int rc;

	/* Invoke Clock controller to enable module clock */

	/* Equivalent to CLK_EnableModuleClock() */
	rc = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t)&config->pcc);
	if (rc < 0) {
		return rc;
	}

	/* Equivalent to CLK_SetModuleClock() */
	rc = clock_control_configure(config->clkctrl_dev, (clock_control_subsys_t)&config->pcc,
				     NULL);
	if (rc < 0) {
		return rc;
	}

	/* Invoke Reset controller to reset module to default state */
	/* Equivalent to SYS_ResetModule() */
	rc = reset_line_toggle_dt(&config->reset);
	if (rc < 0) {
		return rc;
	}

	LOG_DBG("Ethos-U DTS info: base_addr=0x%p, secure_enable=%u, privilege_enable=%u",
		config->base_addr, config->secure_enable, config->privilege_enable);

	if (ethosu_init(drv, config->base_addr, NULL, 0, config->secure_enable,
			config->privilege_enable)) {
		LOG_ERR("Failed to initialize NPU with ethosu_init().");
		return -EINVAL;
	}

	ethosu_set_basep_cache_mask(drv, config->flush_mask, config->invalidate_mask);

	config->irq_config(dev);

	return 0;
}

/* Peripheral Clock Control */
#define NUMAKER_PCC_INST_GET(inst)                                                                 \
	{                                                                                          \
		.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC,                                            \
		.pcc.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),                   \
		.pcc.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                            \
		.pcc.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),                           \
	}

#define NUMAKER_ETHOS_U_INIT(inst)                                                                 \
	static void ethos_u_numaker_irq_config_##inst(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority),                   \
			    ethos_u_numaker_irq_handler, DEVICE_DT_INST_GET(inst), 0);             \
                                                                                                   \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
	}                                                                                          \
                                                                                                   \
	static const struct ethos_u_numaker_config ethos_u_numaker_config_##inst = {               \
		.base_addr = (void *)DT_INST_REG_ADDR(inst),                                       \
		.clkctrl_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                \
		.pcc = NUMAKER_PCC_INST_GET(inst),                                                 \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.irq_config = ethos_u_numaker_irq_config_##inst,                                   \
		.secure_enable = DT_INST_PROP(inst, secure_enable),                                \
		.privilege_enable = DT_INST_PROP(inst, privilege_enable),                          \
		.flush_mask = DT_INST_PROP(inst, flush_mask),                                      \
		.invalidate_mask = DT_INST_PROP(inst, invalidate_mask),                            \
	};                                                                                         \
                                                                                                   \
	static struct ethos_u_numaker_data ethos_u_numaker_data_##inst;                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ethos_u_numaker_init, NULL, &ethos_u_numaker_data_##inst,      \
			      &ethos_u_numaker_config_##inst, POST_KERNEL,                         \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_ETHOS_U_INIT);
