/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_t1s_phy

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include "fsl_tenbaset_phy.h"

#define LOG_MODULE_NAME phy_nxp_t1s
#define LOG_LEVEL       CONFIG_PHY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

struct nxp_t1s_config {
	tenbaset_phy_plca_config_t plca_config;
	TENBASET_PHY_Type *base;
	void (*irq_config_func)(void);
};

struct nxp_t1s_data {
	const struct device *dev;
	tenbaset_phy_handle_t handle;
	bool is_link_up;
	phy_callback_t cb;
	void *cb_data;
};

static int phy_nxp_t1s_get_link(const struct device *dev, struct phy_link_state *state)
{
	const struct nxp_t1s_config *config = dev->config;
	TENBASET_PHY_Type *base = config->base;

	state->is_up = TENBASET_PHY_IsLinkUp(base);
	state->speed = LINK_HALF_10BASE;

	return 0;
}

static int phy_nxp_t1s_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct nxp_t1s_data *data = dev->data;
	struct phy_link_state state;

	data->cb = cb;
	data->cb_data = user_data;

	if (data->cb) {
		state.is_up = data->is_link_up;
		state.speed = LINK_HALF_10BASE;
		data->cb(dev, &state, data->cb_data);
	}

	return 0;
}

static int phy_nxp_t1s_set_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	const struct nxp_t1s_config *config = dev->config;
	TENBASET_PHY_Type *base = config->base;
	tenbaset_phy_plca_config_t plca_config = {};
	status_t status;

	plca_config.nodeId = plca_cfg->node_id;
	plca_config.nodeCount = plca_cfg->node_count;
	plca_config.toTimer = plca_cfg->to_timer;
	plca_config.burstTimer = plca_cfg->burst_timer;
	plca_config.maxBurstCount = plca_cfg->burst_count;
	plca_config.enable = plca_cfg->enable;

	status = TENBASET_PHY_SetPLCAConfig(base, &plca_config);
	if (status != kStatus_Success) {
		LOG_ERR("PHY (%p) Failed to set PLCA config", base);
		return -EIO;
	}

	return 0;
}

static int phy_nxp_t1s_get_plca_cfg(const struct device *dev, struct phy_plca_cfg *plca_cfg)
{
	const struct nxp_t1s_config *config = dev->config;
	TENBASET_PHY_Type *base = config->base;
	tenbaset_phy_plca_config_t plca_config = {};

	TENBASET_PHY_GetPLCAConfig(base, &plca_config);

	memset(plca_cfg, 0, sizeof(struct phy_plca_cfg));
	/* TODO use driver API instead of direct PLCAIDVER register access once available */
	plca_cfg->version = (base->PLCAIDVER & TENBASET_PHY_PLCAIDVER_VER_MASK) >>
						 TENBASET_PHY_PLCAIDVER_VER_SHIFT;
	plca_cfg->enable = plca_config.enable;
	plca_cfg->node_id = plca_config.nodeId;
	plca_cfg->node_count = plca_config.nodeCount;
	plca_cfg->burst_count = plca_config.maxBurstCount;
	plca_cfg->burst_timer = plca_config.burstTimer;
	plca_cfg->to_timer = plca_config.toTimer;

	return 0;
}

static int phy_nxp_t1s_get_plca_sts(const struct device *dev, bool *plca_sts)
{
	const struct nxp_t1s_config *config = dev->config;
	TENBASET_PHY_Type *base = config->base;

	*plca_sts = TENBASET_PHY_GetPLCAStatus(base);

	return 0;
}

static int phy_nxp_t1s_init(const struct device *dev)
{
	const struct nxp_t1s_config *config = dev->config;
	TENBASET_PHY_Type *base = config->base;
	struct nxp_t1s_data *data = dev->data;
	tenbaset_phy_handle_t *handle = &data->handle;
	tenbaset_phy_config_t cfg;
	status_t status;

	data->dev = dev;
	data->is_link_up = false;

	TENBASET_PHY_CreateHandle(base, handle, NULL, NULL);

	TENBASET_PHY_GetDefaultConfig(&cfg);
	cfg.plcaConfig = config->plca_config;
	cfg.interruptMask = kTENBASET_PHY_PLCAStatusFlag | kTENBASET_PHY_ModeStatusFlag;

	status = TENBASET_PHY_Init(base, &cfg, handle);
	if (status != kStatus_Success) {
		LOG_ERR("PHY (%p) Failed to initialize", base);
		return -EIO;
	}

	config->irq_config_func();

	return 0;
}

static void phy_nxp_t1s_isr(const struct device *dev)
{
	const struct nxp_t1s_config *config = dev->config;
	TENBASET_PHY_Type *base = config->base;
	struct nxp_t1s_data *data = dev->data;
	struct phy_link_state state;
	bool plca_status;
	uint16_t flags;

	flags = TENBASET_PHY_GetInterruptStatus(base);

	if (flags == 0U) {
		return;
	}

	TENBASET_PHY_ClearInterruptStatus(base, flags);

	if ((flags & kTENBASET_PHY_ModeStatusFlag) != 0U) {
		state.is_up = TENBASET_PHY_IsLinkUp(base);

		if (data->is_link_up != state.is_up) {
			data->is_link_up = state.is_up;
			if (data->is_link_up) {
				LOG_INF("PHY (%p) Link speed 10 Mbps, half duplex", base);
			}
			if (data->cb) {
				state.speed = LINK_HALF_10BASE;
				data->cb(dev, &state, data->cb_data);
			}
		}
	}

	if ((flags & kTENBASET_PHY_PLCAStatusFlag) != 0U) {
		plca_status = TENBASET_PHY_GetPLCAStatus(base);
		LOG_DBG("PHY (%p) PLCA status %s", base, plca_status ? "on" : "off");
	}
}

static DEVICE_API(ethphy, nxp_t1s_phy_api) = {
	.get_link = phy_nxp_t1s_get_link,
	.link_cb_set = phy_nxp_t1s_link_cb_set,
	.set_plca_cfg = phy_nxp_t1s_set_plca_cfg,
	.get_plca_cfg = phy_nxp_t1s_get_plca_cfg,
	.get_plca_sts = phy_nxp_t1s_get_plca_sts,
};

#define NXP_T1S_PHY_IRQ_CONFIG_FUNC(n)                              \
	static void phy_nxp_t1s_##n##_irq_config_func(void)             \
	{                                                               \
		IRQ_CONNECT(DT_IRQN_BY_IDX(DT_DRV_INST(n), 0),              \
					DT_IRQ_BY_IDX(DT_DRV_INST(n), 0, priority),     \
					phy_nxp_t1s_isr,                                \
					DEVICE_DT_INST_GET(n), 0);                      \
		irq_enable(DT_IRQN_BY_IDX(DT_DRV_INST(n), 0));              \
	}

#define NXP_T1S_PHY_INIT_DRIVER(n)                                  \
                                                                    \
	NXP_T1S_PHY_IRQ_CONFIG_FUNC(n)                                  \
                                                                    \
	static const struct nxp_t1s_config nxp_t1s_##n##_config = {     \
		.plca_config = {                                            \
			.nodeId = DT_INST_PROP(n, plca_node_id),                \
			.nodeCount = DT_INST_PROP(n, plca_node_count),          \
			.toTimer = DT_INST_PROP(n, plca_to_timer),              \
			.burstTimer = DT_INST_PROP(n, plca_burst_timer),        \
			.maxBurstCount = DT_INST_PROP(n, plca_burst_count),     \
			.enable = DT_INST_PROP(n, plca_enable)                  \
		},                                                          \
		.base = (TENBASET_PHY_Type *)DT_INST_REG_ADDR(n),           \
		.irq_config_func = phy_nxp_t1s_##n##_irq_config_func,       \
	};                                                              \
                                                                    \
	static struct nxp_t1s_data nxp_t1s_##n##_data;                  \
                                                                    \
	DEVICE_DT_INST_DEFINE(n, &phy_nxp_t1s_init, NULL,               \
							&nxp_t1s_##n##_data,                    \
							&nxp_t1s_##n##_config,                  \
							POST_KERNEL,                            \
							CONFIG_PHY_INIT_PRIORITY,               \
							&nxp_t1s_phy_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_T1S_PHY_INIT_DRIVER);
