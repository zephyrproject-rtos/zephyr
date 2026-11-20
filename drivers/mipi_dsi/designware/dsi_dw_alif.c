/*
 * Copyright (C) 2024 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/drivers/regulator.h>
#include "dsi_dw_priv.h"

#define DT_DRV_COMPAT alif_mipi_dsi

LOG_MODULE_DECLARE(dsi_dw, CONFIG_MIPI_DSI_LOG_LEVEL);

struct dsi_dw_alif_config {
	/* Must be first member- generic DSI controller configuration */
	struct dsi_dw_config dsi;
	const struct device *tx_dphy_reg;
	const struct device *pll_dphy_reg;
	const struct device *ldo_phy_reg;
};

static int dsi_dw_platform_init(const struct device *dev)
{
	const struct dsi_dw_alif_config *config = dev->config;
	int ret;

	/* Power on the PHYs */
	ret = regulator_enable(config->tx_dphy_reg);
	if (ret) {
		LOG_ERR("Failed to enable TX D-PHY regulator! ret - %d", ret);
		return ret;
	}
	ret = regulator_enable(config->pll_dphy_reg);
	if (ret) {
		LOG_ERR("Failed to enable PLL D-PHY regulator! ret - %d", ret);
		return ret;
	}
	ret = regulator_enable(config->ldo_phy_reg);
	if (ret) {
		LOG_ERR("Failed to enable LDO PHY regulator! ret - %d", ret);
		return ret;
	}

	/* Enable DSI clock. */
	ret = clock_control_on(config->dsi.clk_dev, config->dsi.pclk);
	if (ret) {
		LOG_ERR("Enable DSI clock source for APB interface failed! ret - %d", ret);
		return ret;
	}

	/* Enable D-PHY PLL reference clock. */
	ret = clock_control_on(config->dsi.clk_dev, config->dsi.dphy_clkin);
	if (ret) {
		LOG_ERR("Enable D-PHY PLL reference clock failed! ret - %d", ret);
		return ret;
	}

	/* Enable TX-DPHY clock. */
	ret = clock_control_on(config->dsi.clk_dev, config->dsi.dphy_cfgclk);
	if (ret) {
		LOG_ERR("Enable TX-DPHY clock source failed! ret - %d", ret);
		return ret;
	}

	return 0;
}

static int dsi_dw_alif_init(const struct device *dev)
{
	const struct dsi_dw_config *config = dev->config;
	struct dsi_dw_data *data = dev->data;
	int ret = 0;

	DEVICE_MMIO_NAMED_MAP(dev, dsi_regs, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, clkctl_regs, K_MEM_CACHE_NONE);

	ret = dsi_dw_platform_init(dev);
	if (ret) {
		LOG_ERR("Clock init failed! ret - %d", ret);
		return ret;
	}

	config->irq_config_func(dev);

	LOG_DBG("MMIO address: 0x%x", (uint32_t)DEVICE_MMIO_NAMED_GET(dev, dsi_regs));
	LOG_DBG("CLKCTL MMIO address: 0x%x", (uint32_t)DEVICE_MMIO_NAMED_GET(dev, clkctl_regs));

	LOG_DBG("irq - %d", config->irq);
	LOG_DBG("Video pattern generator: %d", config->dpi.vpg_pattern);
	LOG_DBG("Packet size - %d", data->pkt_size);
	LOG_DBG("Max Bit Clock - %d", config->max_bit_clk);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
/**
 * @brief DSI DW suspend handler.
 *
 * Stops the DSI controller, disables its clock, and marks the current
 * mode as invalid so a full reconfiguration is forced on
 * the next resume.
 *
 * @param dev DSI device struct.
 * @return 0 on success, negative errno otherwise.
 */
static int dsi_dw_suspend(const struct device *dev)
{
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	/* Stop DSI controller */
	dsi_dw_pwr_down(regs);

	/* Force a reconfiguration after resume */
	data->curr_mode = (enum dsi_dw_mode)-1;

	const struct dsi_dw_config *config = dev->config;
	int ret;

	/* Disable controller clock */
	ret = clock_control_off(config->clk_dev, config->pclk);
	if (ret) {
		LOG_ERR("Failed to disable controller clock (%d)", ret);
		return ret;
	}

	/*
	 * Turn off D-PHY clocks so clock gate registers reflect "off".
	 * After S2RAM, clock_control_on() then performs a real
	 * 0->1 transition to restart the hardware clock.
	 */
	ret = clock_control_off(config->clk_dev, config->dphy_cfgclk);
	if (ret) {
		LOG_ERR("Failed to disable TX D-PHY clock (%d)", ret);
		return ret;
	}

	ret = clock_control_off(config->clk_dev, config->dphy_clkext);
	if (ret) {
		LOG_ERR("Failed to disable D-PHY PLL bypass clock (%d)", ret);
		return ret;
	}

	ret = clock_control_off(config->clk_dev, config->dphy_clkin);
	if (ret) {
		LOG_ERR("Failed to disable D-PHY PLL reference clock (%d)", ret);
		return ret;
	}

	LOG_DBG("PM: Suspended %s", dev->name);

	return 0;
}

/**
 * @brief DSI DW resume handler.
 *
 * Re-enables the controller clock. Does not reprogram DSI mode
 * or packet-size registers directly; these are left invalidated
 * (curr_mode = -1) so the next configuration call re-applies them.
 *
 * @param dev DSI device struct.
 * @return 0 on success, negative errno otherwise.
 */
static int dsi_dw_resume(const struct device *dev)
{
	int ret = 0;

	ret = dsi_dw_clock_init(dev);
	if (ret) {
		LOG_ERR("Failed to restore clocks");
		return ret;
	}

	LOG_DBG("PM: Resumed %s", dev->name);

	return 0;
}

/**
 * @brief DSI PM device action handler
 *
 * Handles power management state transitions for the DSI DW Controller.
 *
 * @param dev DSI device struct
 * @param action PM device action
 * @return 0 if successful, negative errno otherwise
 */
static int dsi_dw_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {

	case PM_DEVICE_ACTION_SUSPEND:
		return dsi_dw_suspend(dev);

	case PM_DEVICE_ACTION_RESUME:
		return dsi_dw_resume(dev);

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		/* Power domain handling is automatic via the PM framework */
		return 0;

	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(mipi_dsi, dsi_dw_api) = {
	.attach = dsi_dw_attach,
	.transfer = dsi_dw_transfer,
};

#define MIPI_DSI_GET_CLK(i)                                                                        \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(i, clocks),                                            \
		(.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(i)),                              \
		 .pixel_clk = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(i,              \
			 pixel_clk, clkid),                                                     \
		 .pclk = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(i,              \
			 pclk, clkid),                                                    \
		 .dphy_clkin = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(i,           \
			 dphy_clkin, clkid),                                                    \
		 .dphy_clkext = ((clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(i,        \
			dphy_clkext, clkid)),                                                   \
		 .dphy_cfgclk = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(i,           \
			 dphy_cfgclk, clkid),))

#define DSI_DW_INIT(i)                                                                             \
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(dsi_regs, DT_DRV_INST(i)),                              \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(clkctl_regs, DT_DRV_INST(i)),                   \
                                                                                                   \
		MIPI_DSI_GET_CLK(i).irq = DT_INST_IRQN(i),                                         \
		.irq_config_func = dsi_dw_config_func_##i,                                         \
                                                                                                   \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(i, dpi_in), (                                  \
			.dpi = {                                                                   \
			.polarity =                                                 \
			COND_CODE_0(DT_PROP(DT_CHILD(DT_INST_PHANDLE(i, dpi_in),    \
			    display_timings), hsync_active),                        \
			    DSI_DPI_CFG_POL_HSYNC_ACTIVE_LOW,                       \
			    (0)) |                                                  \
			COND_CODE_0(DT_PROP(DT_CHILD(DT_INST_PHANDLE(i, dpi_in),    \
			    display_timings), vsync_active),                        \
			    DSI_DPI_CFG_POL_VSYNC_ACTIVE_LOW,                       \
			    (0)) |                                                  \
			COND_CODE_0(DT_PROP(DT_CHILD(DT_INST_PHANDLE(i, dpi_in),    \
			    display_timings), de_active),                           \
			    DSI_DPI_CFG_POL_DATAEN_ACTIVE_LOW,                      \
			    (0)) |                                                  \
			COND_CODE_0(DT_PROP(DT_CHILD(DT_INST_PHANDLE(i, dpi_in),    \
			    display_timings), de_active),                           \
			    DSI_DPI_CFG_POL_DATAEN_ACTIVE_LOW,                      \
			    (0)) |                                                  \
			COND_CODE_0(DT_INST_PROP(i, dpi_colorm_active),		\
			    DSI_DPI_CFG_POL_COLM_ACTIVE_LOW,		\
			    (0)) |					\
			COND_CODE_0(DT_INST_PROP(i, dpi_shutdn_active),		\
			    DSI_DPI_CFG_POL_SHUTD_ACTIVE_LOW,		\
			    (0)),					\
		.vpg_pattern = DT_INST_ENUM_IDX_OR(i, dpi_video_pattern_gen,		\
					DPI_VID_PATTERN_GEN_NONE),			\
			},)) \
		.eotp_lp_tx = DT_INST_PROP(i, eotp_lp_tx_en), \
		.eotp_rx = DT_INST_PROP(i, eotp_rx_en), \
		.ecc_recv_en = DT_INST_PROP(i, ecc_recv_en), \
		.crc_recv_en = DT_INST_PROP(i, crc_recv_en), \
		.frame_ack_en = DT_INST_PROP(i, frame_ack_en), \
		.max_bit_clk = DT_INST_PROP_OR(i, max_bit_clk, UINT32_MAX),

#define ALIF_MIPI_DSI_DEVICE(i)                                                                    \
	static void dsi_dw_config_func_##i(const struct device *dev);                              \
	static const struct dsi_dw_alif_config config_##i = {                                      \
		.dsi = {DSI_DW_INIT(i)},                                                           \
		.ldo_phy_reg = DEVICE_DT_GET(DT_INST_PHANDLE(i, ldo_phy)),                         \
		.tx_dphy_reg = DEVICE_DT_GET(DT_INST_PHANDLE(i, tx_dphy)),                         \
		.pll_dphy_reg = DEVICE_DT_GET(DT_INST_PHANDLE(i, pll_dphy)),                       \
	};                                                                                         \
	static struct dsi_dw_data data_##i = {                                                     \
		.pkt_size = DT_INST_PROP(i, vid_pkt_size),                                         \
		.num_chunks = 0,                                                                   \
		.null_size = 0,                                                                    \
	};                                                                                         \
	IF_ENABLED(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_DEFINE(i, dsi_dw_pm_action);))             \
	DEVICE_DT_INST_DEFINE(i, &dsi_dw_alif_init, PM_DEVICE_DT_INST_GET(i), &data_##i,           \
			      &config_##i, POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY,             \
			      &dsi_dw_api);                                                        \
                                                                                                   \
	static void dsi_dw_config_func_##i(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), dsi_dw_irq,                 \
			    DEVICE_DT_INST_GET(i), 0);                                             \
		irq_enable(DT_INST_IRQN(i));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(ALIF_MIPI_DSI_DEVICE)
