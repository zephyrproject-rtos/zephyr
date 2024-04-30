/* NXP ENET QOS Ethernet MAC Driver
 *
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_enet_qos

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/ethernet/eth_nxp_enet_qos.h>

int nxp_enet_qos_init(const struct device *dev)
{
	const struct nxp_enet_qos_config *config = dev->config;
	int ret;

	/* TODO: once NXP reset drivers are created, use that to reset
	 * until then, make sure reset is done in platform init
	 */

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		return ret;
	}

	return pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
}

#define NXP_ENET_QOS_INIT(n)								\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static const struct nxp_enet_qos_config enet_qos_##n##_config = {		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (void *)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name),		\
		.base = (enet_qos_t *)DT_INST_REG_ADDR(n),				\
	};										\
											\
	/* Init the module before any enet device inits so priority 0 */		\
	DEVICE_DT_INST_DEFINE(n, nxp_enet_qos_init, NULL, NULL,				\
			      &enet_qos_##n##_config, POST_KERNEL, 0, NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_QOS_INIT)
