/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lcu

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fsl_lcu.h>

#include "nxp_lcu.h"

LOG_MODULE_REGISTER(nxp_lcu, CONFIG_NXP_LCU_LOG_LEVEL);

#define CELL_MASK BIT_MASK(NXP_LCU_CELL_IO_COUNT)

struct nxp_lcu_config {
	LCU_Type *base;
	const struct device *clock;
	clock_control_subsys_t clock_id;
};

struct nxp_lcu_data {
	struct k_spinlock lock;
	const void *owners[LCU_LC_COUNT];
	uint8_t software_inputs[LCU_LC_COUNT];
	uint8_t configured;
};

static int check_cell(const struct device *dev, uint8_t cell, const void *owner, bool write)
{
	const struct nxp_lcu_config *config = dev->config;
	const struct nxp_lcu_data *data = dev->data;

	if (cell >= LCU_LC_COUNT || owner == NULL) {
		LOG_ERR("Invalid cell or owner");
		return -EINVAL;
	}
	if (data->owners[cell] != owner) {
		LOG_ERR("Cell %u is not owned by caller", cell);
		return -EPERM;
	}
	if (write && (config->base->CFG & LCU_CFG_WP_MASK) != 0U) {
		LOG_ERR("LCU configuration is write-protected");
		return -EACCES;
	}
	return 0;
}

static int lcu_request_cell(const struct device *dev, uint8_t cell, const void *owner)
{
	const struct nxp_lcu_config *config = dev->config;
	struct nxp_lcu_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	if (cell >= LCU_LC_COUNT || owner == NULL) {
		ret = -EINVAL;
	} else if ((config->base->CFG & LCU_CFG_WP_MASK) != 0U) {
		ret = -EACCES;
	} else if (data->owners[cell] != NULL) {
		ret = -EBUSY;
	} else {
		data->owners[cell] = owner;
	}
	k_spin_unlock(&data->lock, key);
	if (ret != 0) {
		LOG_ERR("Cannot reserve cell %u: %d", cell, ret);
	}
	return ret;
}

static int lcu_release(const struct device *dev, uint8_t cell, const void *owner)
{
	const struct nxp_lcu_config *config = dev->config;
	struct nxp_lcu_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = check_cell(dev, cell, owner, true);

	if (ret == 0) {
		LCU_EnableOutput(config->base, CELL_MASK << (cell * 4U), false);
		LCU_EnableSoftwareOverride(config->base, CELL_MASK << (cell * 4U), false);
		data->configured &= ~BIT(cell);
		data->software_inputs[cell] = 0U;
		data->owners[cell] = NULL;
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int lcu_configure(const struct device *dev, uint8_t cell, const void *owner,
		      const struct nxp_lcu_cell_config *cell_config)
{
	const struct nxp_lcu_config *config = dev->config;
	struct nxp_lcu_data *data = dev->data;
	lcu_output_config_t output;
	int ret;

	if (cell_config == NULL || (cell_config->software_inputs & ~CELL_MASK) != 0U) {
		LOG_ERR("Invalid cell configuration");
		return -EINVAL;
	}
	for (uint8_t i = 0U; i < NXP_LCU_CELL_IO_COUNT; i++) {
		if (cell_config->sources[i] > NXP_LCU_SOURCE_OUTPUT(11U)) {
			LOG_ERR("Invalid source for input %u", i);
			return -EINVAL;
		}
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = check_cell(dev, cell, owner, true);
	if (ret != 0) {
		goto out;
	}
	if ((config->base->OUTEN & (CELL_MASK << (cell * 4U))) != 0U) {
		LOG_ERR("Disable cell %u before reconfiguration", cell);
		ret = -EBUSY;
		goto out;
	}

	LCU_GetOutputDefaultConfig(&output);
	for (uint8_t i = 0U; i < NXP_LCU_CELL_IO_COUNT; i++) {
		uint8_t io = cell * 4U + i;

		output.lutValue = cell_config->outputs[i].lut;
		output.riseFilter = cell_config->outputs[i].rise_filter;
		output.fallFilter = cell_config->outputs[i].fall_filter;
		output.outputPolarity = cell_config->outputs[i].inverted ?
					kLCU_OutputPolarityInverted :
					kLCU_OutputPolarityNotInverted;
		LCU_OutputInit(config->base, (lcu_outputs_t)io, &output);
		LCU_MuxSelect(config->base, (lcu_inputs_t)io,
			      (lcu_muxcel_source_t)cell_config->sources[i]);
	}
	LCU_EnableDebugMode(config->base, CELL_MASK << (cell * 4U), false);
	LCU_EnableSoftwareOverride(config->base, cell_config->software_inputs << (cell * 4U),
				   true);
	data->software_inputs[cell] = cell_config->software_inputs;
	data->configured |= BIT(cell);
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int lcu_set_inputs(const struct device *dev, uint8_t cell, const void *owner, uint8_t values)
{
	const struct nxp_lcu_config *config = dev->config;
	struct nxp_lcu_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = check_cell(dev, cell, owner, false);

	if (ret == 0) {
		if ((values & ~CELL_MASK) != 0U || (data->configured & BIT(cell)) == 0U) {
			LOG_ERR("Invalid input values or unconfigured cell");
			ret = -EINVAL;
		} else {
			uint32_t mask = data->software_inputs[cell] << (cell * 4U);
			uint32_t high = values << (cell * 4U);

			LCU_SetSoftwareOverrideValue(config->base, mask & ~high,
						    kLCU_SoftwareOverride0);
			LCU_SetSoftwareOverrideValue(config->base, mask & high,
						    kLCU_SoftwareOverride1);
		}
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int lcu_set_outputs(const struct device *dev, uint8_t cell, const void *owner,
			   uint8_t enabled)
{
	const struct nxp_lcu_config *config = dev->config;
	struct nxp_lcu_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = check_cell(dev, cell, owner, true);

	if (ret == 0) {
		if ((enabled & ~CELL_MASK) != 0U || (data->configured & BIT(cell)) == 0U) {
			LOG_ERR("Invalid output mask or unconfigured cell");
			ret = -EINVAL;
		} else {
			config->base->OUTEN = (config->base->OUTEN & ~(CELL_MASK << (cell * 4U))) |
					     ((uint32_t)enabled << (cell * 4U));
		}
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int lcu_get_state(const struct device *dev, uint8_t cell, const void *owner,
		      struct nxp_lcu_state *state)
{
	const struct nxp_lcu_config *config = dev->config;
	struct nxp_lcu_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = check_cell(dev, cell, owner, false);

	if (ret == 0) {
		if (state == NULL) {
			LOG_ERR("Missing state destination");
			ret = -EINVAL;
		} else {
			uint8_t shift = cell * 4U;

			state->inputs = (LCU_GetInputs(config->base) >> shift) & CELL_MASK;
			state->overridden =
				(LCU_GetSoftwareOverrideInputs(config->base) >> shift) & CELL_MASK;
			state->outputs = (LCU_GetOutputs(config->base) >> shift) & CELL_MASK;
			state->enabled = (config->base->OUTEN >> shift) & CELL_MASK;
		}
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int lcu_get_rate(const struct device *dev, uint32_t *rate)
{
	const struct nxp_lcu_config *config = dev->config;

	if (rate == NULL) {
		LOG_ERR("Missing clock rate destination");
		return -EINVAL;
	}
	return clock_control_get_rate(config->clock, config->clock_id, rate);
}

static int nxp_lcu_init(const struct device *dev)
{
	const struct nxp_lcu_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->clock)) {
		LOG_ERR_DEVICE_NOT_READY(config->clock);
		return -ENODEV;
	}
	ret = clock_control_on(config->clock, config->clock_id);
	if (ret != 0) {
		LOG_ERR("Cannot enable LCU clock: %d", ret);
		return ret;
	}
	if ((config->base->CFG & LCU_CFG_WP_MASK) != 0U) {
		LOG_ERR("LCU was write-protected before initialization");
		return -EACCES;
	}
	LCU_EnableOutput(config->base, LCU_OUTEN_OUTEN_MASK, false);
	LCU_EnableDebugMode(config->base, LCU_DBGEN_DBGEN_MASK, false);
	LCU_SetLutDma(config->base, LCU_OUTEN_OUTEN_MASK, false);
	LCU_SetForceDma(config->base, LCU_OUTEN_OUTEN_MASK, false);
	LCU_SetLutInterrupt(config->base, LCU_OUTEN_OUTEN_MASK, false);
	LCU_SetForceInterrupt(config->base, LCU_OUTEN_OUTEN_MASK, false);
	return 0;
}

static const struct nxp_lcu_driver_api nxp_lcu_api = {
	.request_cell = lcu_request_cell,
	.release = lcu_release,
	.configure = lcu_configure,
	.set_inputs = lcu_set_inputs,
	.set_outputs = lcu_set_outputs,
	.get_state = lcu_get_state,
	.get_rate = lcu_get_rate,
};

#define NXP_LCU_DEFINE(n) \
	static struct nxp_lcu_data nxp_lcu_data_##n; \
	static const struct nxp_lcu_config nxp_lcu_config_##n = { \
		.base = (LCU_Type *)DT_INST_REG_ADDR(n), \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.clock_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name), \
	}; \
	DEVICE_DT_INST_DEFINE(n, nxp_lcu_init, NULL, &nxp_lcu_data_##n, &nxp_lcu_config_##n, \
			      POST_KERNEL, CONFIG_NXP_LCU_INIT_PRIORITY, &nxp_lcu_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_LCU_DEFINE)
