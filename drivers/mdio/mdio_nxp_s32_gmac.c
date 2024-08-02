/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_gmac_mdio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_mdio, CONFIG_MDIO_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#include <Gmac_Ip.h>

#define GMAC_MDIO_REG_OFFSET (0x200)

#define GMAC_STATUS_TO_ERRNO(x) \
	((x) == GMAC_STATUS_SUCCESS ? 0 : ((x) == GMAC_STATUS_TIMEOUT ? -ETIMEDOUT : -EIO))

struct mdio_nxp_s32_config {
	uint8_t instance;
	bool suppress_preamble;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct mdio_nxp_s32_data {
	struct k_mutex bus_mutex;
	uint32_t clock_freq;
};

static int mdio_nxp_s32_read_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				 uint16_t regad, uint16_t *regval)
{
	const struct mdio_nxp_s32_config *const cfg = dev->config;
	struct mdio_nxp_s32_data *data = dev->data;
	Gmac_Ip_StatusType status;

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Configure MDIO controller before initiating a transmission */
	Gmac_Ip_EnableMDIO(cfg->instance, cfg->suppress_preamble, data->clock_freq);

	status = Gmac_Ip_MDIOReadMMD(cfg->instance, prtad, devad, regad, regval,
				     CONFIG_MDIO_NXP_S32_TIMEOUT);

	k_mutex_unlock(&data->bus_mutex);

	return GMAC_STATUS_TO_ERRNO(status);
}

static int mdio_nxp_s32_write_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				  uint16_t regad, uint16_t regval)
{
	const struct mdio_nxp_s32_config *const cfg = dev->config;
	struct mdio_nxp_s32_data *data = dev->data;
	Gmac_Ip_StatusType status;

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Configure MDIO controller before initiating a transmission */
	Gmac_Ip_EnableMDIO(cfg->instance, cfg->suppress_preamble, data->clock_freq);

	status = Gmac_Ip_MDIOWriteMMD(cfg->instance, prtad, devad, regad, regval,
				      CONFIG_MDIO_NXP_S32_TIMEOUT);

	k_mutex_unlock(&data->bus_mutex);

	return GMAC_STATUS_TO_ERRNO(status);
}

static int mdio_nxp_s32_read_c22(const struct device *dev, uint8_t prtad,
				 uint8_t regad, uint16_t *regval)
{
	const struct mdio_nxp_s32_config *const cfg = dev->config;
	struct mdio_nxp_s32_data *data = dev->data;
	Gmac_Ip_StatusType status;

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Configure MDIO controller before initiating a transmission */
	Gmac_Ip_EnableMDIO(cfg->instance, cfg->suppress_preamble, data->clock_freq);

	status = Gmac_Ip_MDIORead(cfg->instance, prtad, regad, regval,
				  CONFIG_MDIO_NXP_S32_TIMEOUT);

	k_mutex_unlock(&data->bus_mutex);

	return GMAC_STATUS_TO_ERRNO(status);
}

static int mdio_nxp_s32_write_c22(const struct device *dev, uint8_t prtad,
				  uint8_t regad, uint16_t regval)
{
	const struct mdio_nxp_s32_config *const cfg = dev->config;
	struct mdio_nxp_s32_data *data = dev->data;
	Gmac_Ip_StatusType status;

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Configure MDIO controller before initiating a transmission */
	Gmac_Ip_EnableMDIO(cfg->instance, cfg->suppress_preamble, data->clock_freq);

	status = Gmac_Ip_MDIOWrite(cfg->instance, prtad, regad, regval,
				   CONFIG_MDIO_NXP_S32_TIMEOUT);

	k_mutex_unlock(&data->bus_mutex);

	return GMAC_STATUS_TO_ERRNO(status);
}

static int mdio_nxp_s32_init(const struct device *dev)
{
	const struct mdio_nxp_s32_config *const cfg = dev->config;
	struct mdio_nxp_s32_data *data = dev->data;
	int err;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &data->clock_freq)) {
		LOG_ERR("Failed to get clock frequency");
		return -EIO;
	}

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	k_mutex_init(&data->bus_mutex);

	return 0;
}

static const struct mdio_driver_api mdio_nxp_s32_driver_api = {
	.read = mdio_nxp_s32_read_c22,
	.write = mdio_nxp_s32_write_c22,
	.read_c45 = mdio_nxp_s32_read_c45,
	.write_c45 = mdio_nxp_s32_write_c45,
};

#define MDIO_NXP_S32_HW_INSTANCE_CHECK(i, n)						\
	(((DT_INST_REG_ADDR(n) - GMAC_MDIO_REG_OFFSET) == IP_GMAC_##i##_BASE) ? i : 0)

#define MDIO_NXP_S32_HW_INSTANCE(n)							\
	LISTIFY(__DEBRACKET FEATURE_GMAC_NUM_INSTANCES,					\
		MDIO_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define MDIO_NXP_S32_DEVICE(n)								\
	PINCTRL_DT_INST_DEFINE(n);							\
	static struct mdio_nxp_s32_data mdio_nxp_s32_data_##n;				\
	static const struct mdio_nxp_s32_config mdio_nxp_s32_config_##n = {		\
		.instance = MDIO_NXP_S32_HW_INSTANCE(n),				\
		.suppress_preamble = (bool)DT_INST_PROP(n, suppress_preamble),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
	};										\
	DEVICE_DT_INST_DEFINE(n,							\
			&mdio_nxp_s32_init,						\
			NULL,								\
			&mdio_nxp_s32_data_##n,						\
			&mdio_nxp_s32_config_##n,					\
			POST_KERNEL,							\
			CONFIG_MDIO_INIT_PRIORITY,					\
			&mdio_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_NXP_S32_DEVICE)
