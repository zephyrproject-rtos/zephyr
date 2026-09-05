/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 specific glue.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT rockchip_rk3588_gmac

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/devicetree/reset.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/util.h>

#include "eth_dwmac_priv.h"

/*
 * RK3588 is always 64-bit with an MMU. DMA descriptors must live in a
 * CPU-uncached region so the DMA engine and CPU see the same contents.
 */
BUILD_ASSERT(IS_ENABLED(CONFIG_64BIT), "RK3588 DWMAC requires 64-bit");
BUILD_ASSERT(IS_ENABLED(CONFIG_MMU), "RK3588 DWMAC requires MMU");
BUILD_ASSERT(IS_ENABLED(CONFIG_NOCACHE_MEMORY),
	     "RK3588 DWMAC requires CONFIG_NOCACHE_MEMORY for DMA descriptors");

/* The DMA bus master interface is 64-bit on this IP */
#define DATA_BUS_WIDTH 64

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

struct rk3588_gmac_clk {
	const struct device *dev;
	uint16_t id;
};

struct rk3588_gmac_config {
	struct dwmac_config dwmac;
	const struct rk3588_gmac_clk *clocks;
	uint8_t num_clocks;
	int (*platform_init)(const struct device *dev);
	const struct device *sys_grf;
	const struct device *php_grf;
	uint8_t id;
	uint8_t tx_delay;
	uint8_t rx_delay;
	struct reset_dt_spec reset;
};

/* GRF / PHP RGMII programming */

#define RK3588_PHP_CLK_CON1 0x0070U

#define GRF_BIT(nr)     (BIT(nr) | BIT((nr) + 16))
#define GRF_CLR_BIT(nr) BIT((nr) + 16)

#define RK3588_GMAC0_CLK_DIV_SHIFT  2U
#define RK3588_GMAC0_CLK_DIV_MASK   (GENMASK(1, 0) << RK3588_GMAC0_CLK_DIV_SHIFT)
#define RK3588_GMAC1_CLK_DIV_SHIFT  7U
#define RK3588_GMAC1_CLK_DIV_MASK   (GENMASK(1, 0) << RK3588_GMAC1_CLK_DIV_SHIFT)
#define RK3588_GMAC_CLK_RGMII_DIV1  0U
#define RK3588_GMAC_CLK_RGMII_DIV5  GENMASK(1, 0)
#define RK3588_GMAC_CLK_RGMII_DIV50 BIT(1)

#define RK3588_GMAC_PHY_INTF_SEL_RGMII(id)                                                         \
	(GRF_BIT(3 + (id) * 6) | GRF_CLR_BIT(4 + (id) * 6) | GRF_CLR_BIT(5 + (id) * 6))
#define RK3588_GMAC_TXCLK_DLY_ENABLE(id)  GRF_BIT(2 * (id) + 2)
#define RK3588_GMAC_RXCLK_DLY_ENABLE(id)  GRF_BIT(2 * (id) + 3)
#define RK3588_GMAC_RXCLK_DLY_DISABLE(id) GRF_CLR_BIT(2 * (id) + 3)
#define RK3588_GMAC_CLK_TX_DL_CFG(val)    (((val) & 0xffU) | (0xffU << 16))
#define RK3588_GRF_GMAC_CON7              0x031cU
#define RK3588_GRF_GMAC_CON8              0x0320U
#define RK3588_GRF_GMAC_CON9              0x0324U
#define RK3588_PHP_GMAC_CON0              0x0008U

/*
 * GRF registers carry a write-enable mask in the upper half word: only the
 * bits selected there are modified, so the value written must always pair the
 * data bits with their mask.
 */
static void rk3588s_grf_write(const struct device *grf, uint32_t offset, uint32_t val)
{
	syscon_write_reg(grf, offset, val);
}

static void rk3588s_clrsetreg(const struct device *grf, uint32_t offset, uint32_t clr,
			      uint32_t set)
{
	uint32_t reg;

	syscon_read_reg(grf, offset, &reg);
	syscon_write_reg(grf, offset, (((clr) | (set)) << 16) | ((reg & ~(clr)) | (set)));
}

static void rk3588_gmac_grf_php_clock_select(const struct device *php_grf, unsigned int id)
{
	rk3588s_clrsetreg(php_grf, RK3588_PHP_CLK_CON1, BIT(5U * id + 1U), BIT(5U * id + 4U));
}

static void rk3588_gmac_grf_prepare(const struct rk3588_gmac_config *cfg)
{
	unsigned int id = cfg->id;
	uint32_t delay_con;
	uint32_t con7;

	rk3588s_grf_write(cfg->php_grf, RK3588_PHP_GMAC_CON0,
			  RK3588_GMAC_PHY_INTF_SEL_RGMII(id));
	rk3588s_clrsetreg(cfg->php_grf, RK3588_PHP_CLK_CON1, BIT(id * 5U), 0);

	con7 = RK3588_GMAC_TXCLK_DLY_ENABLE(id);
	if (cfg->rx_delay != 0U) {
		con7 |= RK3588_GMAC_RXCLK_DLY_ENABLE(id);
	} else {
		con7 |= RK3588_GMAC_RXCLK_DLY_DISABLE(id);
	}

	rk3588s_grf_write(cfg->sys_grf, RK3588_GRF_GMAC_CON7, con7);

	delay_con = (id == 0U) ? RK3588_GRF_GMAC_CON8 : RK3588_GRF_GMAC_CON9;
	rk3588s_grf_write(cfg->sys_grf, delay_con, RK3588_GMAC_CLK_TX_DL_CFG(cfg->tx_delay));
}

static void rk3588_gmac_grf_set_rgmii_speed(const struct device *php_grf, unsigned int id,
					    enum phy_link_speed speed)
{
	uint32_t clk_div;
	uint32_t shift = (id == 0U) ? RK3588_GMAC0_CLK_DIV_SHIFT : RK3588_GMAC1_CLK_DIV_SHIFT;
	uint32_t mask = (id == 0U) ? RK3588_GMAC0_CLK_DIV_MASK : RK3588_GMAC1_CLK_DIV_MASK;

	if (PHY_LINK_IS_SPEED_1000M(speed)) {
		clk_div = RK3588_GMAC_CLK_RGMII_DIV1;
	} else if (PHY_LINK_IS_SPEED_100M(speed)) {
		clk_div = RK3588_GMAC_CLK_RGMII_DIV5;
	} else {
		clk_div = RK3588_GMAC_CLK_RGMII_DIV50;
	}

	rk3588s_clrsetreg(php_grf, RK3588_PHP_CLK_CON1, mask, clk_div << shift);
}

/* CRU clock helpers */

static int rk3588_gmac_enable_clocks(const struct device *dev)
{
	const struct rk3588_gmac_config *cfg = dev->config;

	for (uint8_t i = 0; i < cfg->num_clocks; i++) {
		const struct rk3588_gmac_clk *clk = &cfg->clocks[i];
		int ret;

		if (!device_is_ready(clk->dev)) {
			LOG_ERR("clock controller not ready");
			return -ENODEV;
		}

		ret = clock_control_on(clk->dev, (clock_control_subsys_t)(uintptr_t)clk->id);
		if (ret < 0) {
			LOG_ERR("failed to enable clock %u (%d)", clk->id, ret);
			return ret;
		}
	}

	return 0;
}

/*
 * The RGMII transmit clock is divided down from the PHP_GRF divider rather
 * than driven by the MAC, so it has to be reprogrammed on every link speed
 * change.
 */
void dwmac_platform_link_speed_changed(const struct device *dev, enum phy_link_speed speed)
{
	const struct rk3588_gmac_config *cfg = dev->config;

	rk3588_gmac_grf_set_rgmii_speed(cfg->php_grf, cfg->id, speed);
}

/* Bus init and device instances */

int dwmac_bus_init(const struct device *dev)
{
	const struct rk3588_gmac_config *cfg = dev->config;
	int ret;

	ret = rk3588_gmac_enable_clocks(dev);
	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	if (!device_is_ready(cfg->sys_grf) || !device_is_ready(cfg->php_grf)) {
		LOG_ERR("GRF syscon not ready");
		return -ENODEV;
	}

	ret = reset_line_toggle_dt(&cfg->reset);
	if (ret < 0) {
		return ret;
	}

	rk3588_gmac_grf_php_clock_select(cfg->php_grf, cfg->id);
	rk3588_gmac_grf_prepare(cfg);
	rk3588_gmac_grf_set_rgmii_speed(cfg->php_grf, cfg->id, LINK_FULL_1000BASE);
	k_msleep(1);

	LOG_INF("GMAC%u bus init done", cfg->id);

	return 0;
}

#define RK3588_GMAC_CLK_ENTRY(node_id, prop, idx)                                                  \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, idx)),                         \
		.id = DT_CLOCKS_CELL_BY_IDX(node_id, idx, clkid),                                  \
	}

#define RK3588_GMAC_DEFINE(n)                                                                      \
	static struct dwmac_dma_desc __nocache __aligned(CONFIG_DCACHE_LINE_SIZE)                  \
		dwmac_tx_descs_##n[NB_TX_DESCS];                                                   \
	static struct dwmac_dma_desc __nocache __aligned(CONFIG_DCACHE_LINE_SIZE)                  \
		dwmac_rx_descs_##n[NB_RX_DESCS];                                                   \
	static const struct rk3588_gmac_clk rk3588_gmac_clocks_##n[] = {                           \
		DT_INST_FOREACH_PROP_ELEM_SEP(n, clocks, RK3588_GMAC_CLK_ENTRY, (,))               \
	};                                                                                         \
	static int rk3588_gmac_platform_init_##n(const struct device *dev)                         \
	{                                                                                          \
		const struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n);      \
		struct dwmac_priv *p = dev->data;                                                  \
		uintptr_t tx_phys = k_mem_phys_addr(dwmac_tx_descs_##n);                           \
		uintptr_t rx_phys = k_mem_phys_addr(dwmac_rx_descs_##n);                           \
		int ret;                                                                           \
											           \
		p->tx_descs = dwmac_tx_descs_##n;                                                  \
		p->rx_descs = dwmac_rx_descs_##n;                                                  \
		p->tx_descs_phys = (struct dwmac_dma_desc *)tx_phys;                               \
		p->rx_descs_phys = (struct dwmac_dma_desc *)rx_phys;                               \
											           \
		DWMAC_REG_WRITE(MAC_CONF, MAC_CONF_DM);                                            \
		DWMAC_REG_WRITE(DMA_SYSBUS_MODE,                                                   \
				DMA_SYSBUS_MODE_AAL | DMA_SYSBUS_MODE_EAME |                       \
					DMA_SYSBUS_MODE_FB);                                       \
											           \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), dwmac_isr,                  \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
											           \
		ret = net_eth_mac_load(&mac_cfg, p->mac_addr);                                     \
		if (ret == -ENODATA) {                                                             \
			LOG_DBG("No MAC address configured");                                      \
			return 0;                                                                  \
		}                                                                                  \
		if (ret < 0) {                                                                     \
			LOG_ERR("Failed to load MAC address (%d)", ret);                           \
			return ret;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	static const struct rk3588_gmac_config rk3588_gmac_config_##n = {                          \
		.dwmac = {                                                                         \
			DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                      \
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                  \
			.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                           \
			.mac_clk = (clock_control_subsys_t)(uintptr_t)                             \
				DT_INST_CLOCKS_CELL(n, clkid),                                     \
		},                                                                                 \
		.clocks = rk3588_gmac_clocks_##n,                                                  \
		.num_clocks = ARRAY_SIZE(rk3588_gmac_clocks_##n),                                  \
		.platform_init = rk3588_gmac_platform_init_##n,                                    \
		.sys_grf = DEVICE_DT_GET(DT_INST_PHANDLE(n, rockchip_grf)),                        \
		.php_grf = DEVICE_DT_GET(DT_INST_PHANDLE(n, rockchip_php_grf)),                    \
		.id = n,                                                                           \
		.tx_delay = DT_INST_PROP(n, tx_delay),                                             \
		.rx_delay = DT_INST_PROP(n, rx_delay),                                             \
		.reset = RESET_DT_SPEC_INST_GET(n),                                                \
	};                                                                                         \
	static struct dwmac_priv dwmac_instance_##n;

DT_INST_FOREACH_STATUS_OKAY(RK3588_GMAC_DEFINE)

int dwmac_platform_init(const struct device *dev)
{
	const struct rk3588_gmac_config *cfg = dev->config;

	return cfg->platform_init(dev);
}

#define RK3588_GMAC_DEVICE_DEFINE(n)                                                               \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, dwmac_probe, NULL, &dwmac_instance_##n,                   \
				      &rk3588_gmac_config_##n, CONFIG_ETH_INIT_PRIORITY,           \
				      &dwmac_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(RK3588_GMAC_DEVICE_DEFINE)
