/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_hppass_dac_v3

#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#include <infineon_kconfig.h>
#include <cy_hppass_dac.h>

LOG_MODULE_REGISTER(dac_infineon_hppass, CONFIG_DAC_LOG_LEVEL);

#define IFX_HPPASS_DAC_RESOLUTION       12U
#define IFX_HPPASS_DAC_READY_TIMEOUT_US 1000U

/* MCPASS v3 trims: offset [9:4], compensation [11:10], slope [21:16]. */
#define IFX_HPPASS_DAC_TRIM_MASK (GENMASK(21, 16) | GENMASK(11, 4))

struct ifx_hppass_dac_config {
	const struct device *mfd;
	cy_stc_hppass_dac_buf_config_t pdl;
	uint8_t index;
};

struct ifx_hppass_dac_data {
	struct k_sem lock;
	uint16_t value;
	bool configured;
	bool suspended;
};

static int hppass_dac_configure(const struct device *dev)
{
	const struct ifx_hppass_dac_config *config = dev->config;
	const struct ifx_hppass_dac_data *data = dev->data;
	cy_en_hppass_dac_status_t status;
	uint32_t control;
	uint32_t trim;

	/* PDL enables the DAC before writing the non-retention ANA_CTRL_1. */
	status = Cy_HPPASS_DAC_Init(config->index, &config->pdl);
	if (status != CY_HPPASS_DAC_SUCCESS) {
		LOG_ERR("%s: DAC initialization failed (%d)", dev->name, status);
		return -EIO;
	}

	/* Older PDL versions overwrite trims instead of loading them from SFLASH. */
	trim = ((SFLASH_Type *)CY_SFLASH_NS_SBUS_BASE)->DAC_BUFFER_TRIM[config->index];
	control = HPPASS_DAC_ANA_CTRL_2(HPPASS_BASE, config->index);
	HPPASS_DAC_ANA_CTRL_2(HPPASS_BASE, config->index) =
		(control & ~IFX_HPPASS_DAC_TRIM_MASK) | (trim & IFX_HPPASS_DAC_TRIM_MASK);

	if (!WAIT_FOR(Cy_HPPASS_DAC_Is_Ready(config->index), IFX_HPPASS_DAC_READY_TIMEOUT_US,
		      k_busy_wait(1))) {
		Cy_HPPASS_DAC_Disable(config->index);
		LOG_ERR("%s: DAC readiness timed out", dev->name);
		return -ETIMEDOUT;
	}

	Cy_HPPASS_DAC_WriteValue(config->index, data->value);

	return 0;
}

static int ifx_hppass_dac_channel_setup(const struct device *dev,
					const struct dac_channel_cfg *channel_cfg)
{
	struct ifx_hppass_dac_data *data = dev->data;
	int ret;

	if (channel_cfg->channel_id != 0U) {
		LOG_ERR("Only channel 0 is supported");
		return -EINVAL;
	}
	if (channel_cfg->resolution != IFX_HPPASS_DAC_RESOLUTION) {
		LOG_ERR("Only 12-bit resolution is supported");
		return -ENOTSUP;
	}
	if (!channel_cfg->buffered || channel_cfg->internal) {
		LOG_ERR("Only buffered pin output is supported");
		return -ENOTSUP;
	}

	ret = k_sem_take(&data->lock, k_can_yield() ? K_FOREVER : K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("%s: Failed to lock DAC (%d)", dev->name, ret);
		return ret;
	}

	if (data->suspended) {
		LOG_ERR("%s: DAC is suspended", dev->name);
		ret = -EIO;
	} else {
		ret = hppass_dac_configure(dev);
		data->configured = (ret == 0);
	}
	k_sem_give(&data->lock);

	return ret;
}

static int ifx_hppass_dac_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct ifx_hppass_dac_config *config = dev->config;
	struct ifx_hppass_dac_data *data = dev->data;
	int ret;

	if (channel != 0U || value >= BIT(IFX_HPPASS_DAC_RESOLUTION)) {
		LOG_ERR("Invalid DAC channel or value");
		return -EINVAL;
	}

	ret = k_sem_take(&data->lock, k_can_yield() ? K_FOREVER : K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("%s: Failed to lock DAC (%d)", dev->name, ret);
		return ret;
	}

	if (!data->configured || data->suspended) {
		LOG_ERR("%s: DAC channel is not active", dev->name);
		ret = -EACCES;
	} else {
		Cy_HPPASS_DAC_WriteValue(config->index, value);
		data->value = value;
	}
	k_sem_give(&data->lock);

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int ifx_hppass_dac_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ifx_hppass_dac_config *config = dev->config;
	struct ifx_hppass_dac_data *data = dev->data;
	int ret;

	ret = k_sem_take(&data->lock, k_can_yield() ? K_FOREVER : K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("%s: Failed to lock DAC (%d)", dev->name, ret);
		return ret;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (data->configured) {
			Cy_HPPASS_DAC_Disable(config->index);
		}
		data->suspended = true;
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (data->configured) {
			ret = hppass_dac_configure(dev);
		}
		data->suspended = (ret != 0);
		break;
#if defined(CONFIG_PM_S2RAM) || defined(CONFIG_PM_DEVICE_POWER_DOMAIN)
	case PM_DEVICE_ACTION_TURN_ON:
		/* Power restoration leaves the DAC suspended until RESUME. */
		Cy_HPPASS_DAC_Disable(config->index);
		data->suspended = true;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		/* SUSPEND already disabled the DAC before power is removed. */
		data->suspended = true;
		break;
#endif
	default:
		ret = -ENOTSUP;
		break;
	}

	k_sem_give(&data->lock);

	return ret;
}
#endif

static int ifx_hppass_dac_init(const struct device *dev)
{
	const struct ifx_hppass_dac_config *config = dev->config;
	struct ifx_hppass_dac_data *data = dev->data;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR("%s: HPPASS parent is not ready", dev->name);
		return -ENODEV;
	}

	return k_sem_init(&data->lock, 1, 1);
}

static DEVICE_API(dac, ifx_hppass_dac_api) = {
	.channel_setup = ifx_hppass_dac_channel_setup,
	.write_value = ifx_hppass_dac_write_value,
};

#define IFX_HPPASS_DAC_OFFSET(n)                                                                   \
	(DT_INST_REG_ADDR(n) - DT_REG_ADDR(DT_INST_PARENT(n)) - offsetof(HPPASS_Type, DAC_BUFFER))
#define IFX_HPPASS_DAC_INDEX(n)                                                                    \
	(IFX_HPPASS_DAC_OFFSET(n) / sizeof(HPPASS_DAC_BUFFER_DAC_BUFFER_Type))

BUILD_ASSERT(CONFIG_DAC_INIT_PRIORITY > CONFIG_MFD_INFINEON_HPPASS_INIT_PRIORITY,
	     "HPPASS must initialize before its DAC children");

#define IFX_HPPASS_DAC_INIT(n)                                                                     \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_INST_PARENT(n), infineon_hppass_analog_v3),             \
		     "Standalone DAC requires an MCPASS v3 parent");                               \
	BUILD_ASSERT(DT_REG_ADDR(DT_INST_PARENT(n)) == HPPASS_BASE,                                \
		     "DAC parent must match the PDL HPPASS base");                                 \
	BUILD_ASSERT(IFX_HPPASS_DAC_OFFSET(n) % sizeof(HPPASS_DAC_BUFFER_DAC_BUFFER_Type) == 0 &&  \
			     IFX_HPPASS_DAC_INDEX(n) < CY_HPPASS_DAC_NUM_INSTANCE,                 \
		     "Invalid standalone DAC register address");                                   \
	static const struct ifx_hppass_dac_config ifx_hppass_dac_config_##n = {                    \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.index = IFX_HPPASS_DAC_INDEX(n),                                                  \
		.pdl =                                                                             \
			{                                                                          \
				.powerEnable = true,                                               \
				.powerMode = DT_INST_PROP(n, power_mode),                          \
			},                                                                         \
	};                                                                                         \
	static struct ifx_hppass_dac_data ifx_hppass_dac_data_##n;                                 \
	PM_DEVICE_DT_INST_DEFINE(n, ifx_hppass_dac_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(n, ifx_hppass_dac_init, PM_DEVICE_DT_INST_GET(n),                    \
			      &ifx_hppass_dac_data_##n, &ifx_hppass_dac_config_##n, POST_KERNEL,   \
			      CONFIG_DAC_INIT_PRIORITY, &ifx_hppass_dac_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_HPPASS_DAC_INIT)
