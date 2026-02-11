/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_netc_emdio

#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_emdio, CONFIG_MDIO_LOG_LEVEL);

#include <Netc_EthSwt_Ip.h>

struct nxp_s32_mdio_config {
	const struct pinctrl_dev_config *pincfg;
	uint8_t instance;
};

struct nxp_s32_mdio_data {
	struct k_mutex rw_mutex;
};

static int nxp_s32_mdio_read(const struct device *dev, uint8_t prtad,
			     uint8_t regad, uint16_t *regval)
{
	const struct nxp_s32_mdio_config *cfg = dev->config;
	struct nxp_s32_mdio_data *data = dev->data;
	Std_ReturnType status;

	k_mutex_lock(&data->rw_mutex, K_FOREVER);
	status = Netc_EthSwt_Ip_ReadTrcvRegister(cfg->instance, prtad, regad, regval);
	k_mutex_unlock(&data->rw_mutex);

	return status == E_OK ? 0 : -EIO;
}

static int nxp_s32_mdio_write(const struct device *dev, uint8_t prtad,
			      uint8_t regad, uint16_t regval)
{
	const struct nxp_s32_mdio_config *cfg = dev->config;
	struct nxp_s32_mdio_data *data = dev->data;
	Std_ReturnType status;

	k_mutex_lock(&data->rw_mutex, K_FOREVER);
	status = Netc_EthSwt_Ip_WriteTrcvRegister(cfg->instance, prtad, regad, regval);
	k_mutex_unlock(&data->rw_mutex);

	return status == E_OK ? 0 : -EIO;
}

static int nxp_s32_mdio_initialize(const struct device *dev)
{
	struct nxp_s32_mdio_data *data = dev->data;
	const struct nxp_s32_mdio_config *cfg = dev->config;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	k_mutex_init(&data->rw_mutex);

	return 0;
}

static DEVICE_API(mdio, nxp_s32_mdio_api) = {
	.read = nxp_s32_mdio_read,
	.write = nxp_s32_mdio_write,
};

#define NXP_S32_MDIO_HW_INSTANCE_CHECK(i, n) \
	((DT_INST_REG_ADDR(n) == IP_NETC_EMDIO_##n##_BASE) ? i : 0)

#define NXP_S32_MDIO_HW_INSTANCE(n) \
	LISTIFY(__DEBRACKET NETC_F1_INSTANCE_COUNT, NXP_S32_MDIO_HW_INSTANCE_CHECK, (|), n)

#define NXP_S32_MDIO_INSTANCE_DEFINE(n)						\
	PINCTRL_DT_INST_DEFINE(n);						\
	static struct nxp_s32_mdio_data nxp_s32_mdio##n##_data;			\
	static const struct nxp_s32_mdio_config nxp_s32_mdio##n##_cfg = {	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.instance = NXP_S32_MDIO_HW_INSTANCE(n),			\
	};									\
	DEVICE_DT_INST_DEFINE(n,						\
			      &nxp_s32_mdio_initialize,				\
			      NULL,						\
			      &nxp_s32_mdio##n##_data,				\
			      &nxp_s32_mdio##n##_cfg,				\
			      POST_KERNEL,					\
			      CONFIG_MDIO_INIT_PRIORITY,			\
			      &nxp_s32_mdio_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_S32_MDIO_INSTANCE_DEFINE)
