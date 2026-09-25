/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cadence GEM glue for the Microchip PIC32CZ CA "ETH" peripheral: the GEM
 * behind a Microchip wrapper that resets and enables it and selects GMII.
 */

#define DT_DRV_COMPAT microchip_pic32cz_ca_gem

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_plat, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/irq.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

#include <soc.h>

#include "eth_cdns_macb_priv.h"

/* The descriptor rings are not cache maintained, they need an uncached region */
BUILD_ASSERT(!IS_ENABLED(CONFIG_DCACHE) || IS_ENABLED(CONFIG_NOCACHE_MEMORY),
	     "CONFIG_NOCACHE_MEMORY is required with the data cache enabled");

/* The DMA bus master interface of the GEM is 64 bits wide */
CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(64);

/* CTRLA.SWRST and CTRLA.ENABLE are synchronized, neither takes long */
#define MCHP_GEM_SYNC_TIMEOUT_US 1000U

/* Required by the named DEVICE_MMIO macros */
#define DEV_CFG(dev)  ((const struct eth_mchp_cdns_macb_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_mchp_cdns_macb_data *)(dev)->data)

struct eth_mchp_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	DEVICE_MMIO_NAMED_ROM(wrapper);
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_ahb;
	clock_control_subsys_t mclk_apb;
	clock_control_subsys_t gclk_tx;
	clock_control_subsys_t gclk_tsu;
	int (*platform_init)(const struct device *dev);
};

struct eth_mchp_cdns_macb_data {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_priv macb;
	DEVICE_MMIO_NAMED_RAM(wrapper);
};

static int mchp_gem_clock_on(const struct eth_mchp_cdns_macb_config *cfg,
			     clock_control_subsys_t sys)
{
	int ret = clock_control_on(cfg->clock_dev, sys);

	/* -EALREADY: the board devicetree turned it on at boot */
	return (ret == -EALREADY) ? 0 : ret;
}

/* Reset and enable the wrapper, then select the PHY interface */
static int mchp_gem_wrapper_init(const struct device *dev)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, wrapper);

	sys_write32(ETH_CTRLA_SWRST_Msk, base + ETH_CTRLA_REG_OFST);
	if (!WAIT_FOR((sys_read32(base + ETH_SYNCB_REG_OFST) & ETH_SYNCB_SWRST_Msk) == 0U,
		      MCHP_GEM_SYNC_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	/* CTRLB is enable-protected, so it is written before CTRLA.ENABLE */
	sys_write32(sys_read32(base + ETH_CTRLB_REG_OFST) | ETH_CTRLB_GMIIEN_Msk |
			    ETH_CTRLB_GBITCLKREQ_Msk,
		    base + ETH_CTRLB_REG_OFST);

	sys_write32(ETH_CTRLA_ENABLE_Msk, base + ETH_CTRLA_REG_OFST);
	if (!WAIT_FOR((sys_read32(base + ETH_SYNCB_REG_OFST) & ETH_SYNCB_ENABLE_Msk) == 0U,
		      MCHP_GEM_SYNC_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Everything the GEM needs before the core reads its first register: clocks,
 * pins and the wrapper. The PHY node descends from this one, so this runs
 * before the PHY driver releases the PHY from reset, and the pulls that set
 * its strap-in values are already applied by then.
 */
static int eth_mchp_cdns_macb_init(const struct device *dev)
{
	const struct eth_mchp_cdns_macb_config *cfg = dev->config;
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, wrapper, K_MEM_CACHE_NONE);

	ret = mchp_gem_clock_on(cfg, cfg->mclk_ahb);
	if (ret == 0) {
		ret = mchp_gem_clock_on(cfg, cfg->mclk_apb);
	}
	if (ret == 0) {
		ret = mchp_gem_clock_on(cfg, cfg->gclk_tx);
	}
	if (ret == 0) {
		/* Without it the wrapper's software reset never completes */
		ret = mchp_gem_clock_on(cfg, cfg->gclk_tsu);
	}
	if (ret != 0) {
		LOG_ERR("failed to enable the clocks: %d", ret);
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = mchp_gem_wrapper_init(dev);
	if (ret != 0) {
		LOG_ERR("wrapper did not come out of reset");
		return ret;
	}

	return cdns_macb_probe(dev);
}

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_mchp_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	/* No MMU, the rings sit at their physical address */
	p->rings_phys = POINTER_TO_UINT(cfg->macb.rings);

	return cfg->platform_init(dev);
}

/*
 * USRIO bit 0 set selects MII, which GMII falls back to at 10 and 100 Mbit/s.
 * The TX clock is the PHY's TXCK at those speeds and gclk-tx at 1000 Mbit/s,
 * neither divided by the glue, so it has nothing to retune on a speed change.
 */
#define ETH_MCHP_GEM_CAPS (CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_USRIO_HAS_MII)

#define ETH_MCHP_GEM_CLOCK(n, name)                                                                \
	((clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, name, subsystem))

#define ETH_MCHP_GEM_DEVICE(n)                                                                     \
	BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii),                         \
		     "phy-connection-type must be gmii");                                          \
                                                                                                   \
	static struct cdns_macb_rings eth_mchp_gem##n##_rings __nocache_noinit                     \
		__aligned(CDNS_MACB_RINGS_ALIGN);                                                  \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static int eth_mchp_gem##n##_platform_init(const struct device *dev)                       \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		/* Queue 0 only, the tied off queues keep their lines unconnected */               \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    cdns_macb_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct eth_mchp_cdns_macb_config eth_mchp_gem##n##_config = {                 \
		.macb = {                                                                          \
			DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                      \
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                  \
			.rings = &eth_mchp_gem##n##_rings,                                         \
			.caps = ETH_MCHP_GEM_CAPS,                                                 \
			.dma_burst_length = 4,                                                     \
			.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                               \
			.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                             \
		},                                                                                 \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(wrapper, DT_DRV_INST(n)),                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.mclk_ahb = ETH_MCHP_GEM_CLOCK(n, mclk_ahb),                                       \
		.mclk_apb = ETH_MCHP_GEM_CLOCK(n, mclk_apb),                                       \
		.gclk_tx = ETH_MCHP_GEM_CLOCK(n, gclk_tx),                                         \
		.gclk_tsu = ETH_MCHP_GEM_CLOCK(n, gclk_tsu),                                       \
		.platform_init = eth_mchp_gem##n##_platform_init,                                  \
	};                                                                                         \
                                                                                                   \
	static struct eth_mchp_cdns_macb_data eth_mchp_gem##n##_data;                              \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_mchp_cdns_macb_init, NULL, &eth_mchp_gem##n##_data,   \
				      &eth_mchp_gem##n##_config, CONFIG_ETH_INIT_PRIORITY,         \
				      &cdns_macb_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_MCHP_GEM_DEVICE)
