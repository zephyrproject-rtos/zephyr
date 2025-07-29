/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_inputmux

#include <zephyr/drivers/mux.h>
#include <zephyr/drivers/mux/nxp_inputmux.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_inputmux, CONFIG_MUX_CONTROL_LOG_LEVEL);

struct nxp_inputmux_config {
	INPUTMUX_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct reset_dt_spec reset;
};

static int nxp_inputmux_configure(const struct device *dev, mux_control_t mux, void *data)
{
	const struct nxp_inputmux_config *config = dev->config;
	const INPUTMUX_Type *base = config->base;
	const struct inputmux_control *control = (const struct inputmux_control *)mux;
	volatile uint32_t *reg = (uint32_t *)(config->base + control->offset);
	uint32_t value = control->value;
	uint32_t mask = control->mask;
	int err;

	ARG_UNUSED(data);

	err = reset_line_toggle(config->reset.dev, config->reset.id);
	if (err) {
		LOG_ERR("Failed to reset inputmux");
		return err;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("Failed to enable inputmux clock");
		return err;
	}

	*reg &= ~mask;
	*reg |= FIELD_PREP(mask, value);

	err = clock_control_off(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_WRN("Failed to disable inputmux clock");
		return err;
	}

	return 0;
}

static DEVICE_API(mux_control, nxp_inputmux_driver_api) = {
	.configure = nxp_inputmux_configure,
};

#define NXP_INTPUTMUX_INIT(inst)							\
	const static struct nxp_inputmux_config nxp_inputmux_cfg_##inst = {		\
		.base = (INPUTMUX_Type *)DT_INST_REG_ADDR(inst),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.reset = RESET_DT_SPEC_INST_GET(id),					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL,	NULL,					\
				nxp_inputmux_cfg_##inst, 				\
				PRE_KERNEL_1,						\
				CONFIG_MUX_CONTROL_INIT_PRIORITY,			\
				nxp_inputmux_driver_api);


DT_INST_FOREACH_STATUS_OKAY(NXP_INPUTMUX_INIT)
