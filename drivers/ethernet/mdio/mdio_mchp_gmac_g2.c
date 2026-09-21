/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MDIO bus of the Microchip G2 GMAC (PIC32CZ CA "ETH"), Clause 22 only.
 *
 * The node is a child of the Ethernet controller and has no registers of its
 * own. It initialises after its parent but before the PHY on it, and the PHY
 * driver talks to the PHY during its own init, so this driver brings up
 * everything a management frame needs by itself: bus clocks, wrapper enable,
 * MDC divider and the management port enable. The MDC divider is this
 * driver's; the Ethernet driver leaves NCFGR.CLK alone.
 */

#define DT_DRV_COMPAT microchip_gmac_g2_mdio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mdio.h>

#include <soc.h>

LOG_MODULE_REGISTER(mdio_mchp_gmac_g2, CONFIG_MDIO_LOG_LEVEL);

/* IEEE 802.3 22.2.2.13: MDC may not run faster than 2.5 MHz. */
#define MDIO_G2_MDC_MAX_HZ 2500000U

/* One frame is 64 MDC cycles, 26 us at 2.5 MHz. */
#define MDIO_G2_TIMEOUT_US 1000U

/* CTRLA.SWRST and CTRLA.ENABLE both synchronise; neither should take long. */
#define MDIO_G2_SYNC_TIMEOUT_US 1000U

/* MCK divisor for each value of NCFGR.CLK. */
static const uint16_t mdio_g2_mdc_div[] = {8, 16, 32, 48, 64, 96, 128, 224};

struct mdio_g2_config {
	eth_registers_t *regs;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_ahb;
	clock_control_subsys_t mclk_apb;
	clock_control_subsys_t gclk_tsu;
};

struct mdio_g2_data {
	struct k_mutex lock;
};

static int mdio_g2_transfer(const struct device *dev, uint8_t prtad, uint8_t regad,
			    enum mdio_opcode op, uint16_t data_in, uint16_t *data_out)
{
	const struct mdio_g2_config *cfg = dev->config;
	struct mdio_g2_data *data = dev->data;
	eth_registers_t *regs = cfg->regs;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	regs->ETH_MAN = ETH_MAN_CLTTO_Msk | ETH_MAN_OP(op) | ETH_MAN_WTN(2) |
			ETH_MAN_PHYA(prtad) | ETH_MAN_REGA(regad) | ETH_MAN_DATA(data_in);

	if (!WAIT_FOR((regs->ETH_NSR & ETH_NSR_IDLE_Msk) != 0U, MDIO_G2_TIMEOUT_US,
		      k_busy_wait(1))) {
		ret = -ETIMEDOUT;
	} else if (data_out != NULL) {
		*data_out = regs->ETH_MAN & ETH_MAN_DATA_Msk;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int mdio_g2_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	return mdio_g2_transfer(dev, prtad, regad, MDIO_OP_C22_READ, 0, data);
}

static int mdio_g2_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	return mdio_g2_transfer(dev, prtad, regad, MDIO_OP_C22_WRITE, data, NULL);
}

static int mdio_g2_clock_on(const struct mdio_g2_config *cfg, clock_control_subsys_t sys)
{
	int ret = clock_control_on(cfg->clock_dev, sys);

	return (ret == -EALREADY) ? 0 : ret;
}

static int mdio_g2_init(const struct device *dev)
{
	const struct mdio_g2_config *cfg = dev->config;
	struct mdio_g2_data *data = dev->data;
	eth_registers_t *regs = cfg->regs;
	uint32_t rate;
	int clk;
	int ret;

	k_mutex_init(&data->lock);

	ret = mdio_g2_clock_on(cfg, cfg->mclk_ahb);
	if (ret == 0) {
		ret = mdio_g2_clock_on(cfg, cfg->mclk_apb);
	}
	if (ret == 0) {
		/* Without it the wrapper's SWRST never completes. */
		ret = mdio_g2_clock_on(cfg, cfg->gclk_tsu);
	}
	if (ret == 0) {
		ret = clock_control_get_rate(cfg->clock_dev, cfg->mclk_apb, &rate);
	}
	if (ret != 0) {
		LOG_ERR("Bus clock unavailable: %d", ret);
		return ret;
	}

	for (clk = 0; clk < ARRAY_SIZE(mdio_g2_mdc_div); clk++) {
		if (rate / mdio_g2_mdc_div[clk] <= MDIO_G2_MDC_MAX_HZ) {
			break;
		}
	}
	if (clk == ARRAY_SIZE(mdio_g2_mdc_div)) {
		LOG_ERR("No MDC divider brings %u Hz down to 2.5 MHz", rate);
		return -ENOTSUP;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	/* The Ethernet driver may have enabled the block already; keep it. */
	if ((regs->ETH_CTRLA & ETH_CTRLA_ENABLE_Msk) == 0U) {
		regs->ETH_CTRLA = ETH_CTRLA_SWRST_Msk;
		if (!WAIT_FOR((regs->ETH_SYNCB & ETH_SYNCB_SWRST_Msk) == 0U,
			      MDIO_G2_SYNC_TIMEOUT_US, k_busy_wait(1))) {
			return -ETIMEDOUT;
		}
		regs->ETH_CTRLA = ETH_CTRLA_ENABLE_Msk;
		if (!WAIT_FOR((regs->ETH_SYNCB & ETH_SYNCB_ENABLE_Msk) == 0U,
			      MDIO_G2_SYNC_TIMEOUT_US, k_busy_wait(1))) {
			return -ETIMEDOUT;
		}
	}

	regs->ETH_NCFGR = (regs->ETH_NCFGR & ~ETH_NCFGR_CLK_Msk) | ETH_NCFGR_CLK(clk);
	regs->ETH_NCR |= ETH_NCR_MPE_Msk;

	LOG_DBG("MDC %u Hz (MCK %u Hz / %u)", rate / mdio_g2_mdc_div[clk], rate,
		mdio_g2_mdc_div[clk]);

	return 0;
}

static DEVICE_API(mdio, mdio_g2_api) = {
	.read = mdio_g2_read,
	.write = mdio_g2_write,
};

#define MDIO_G2_CLOCK(n, name) ((void *)DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(n), name, subsystem))

#define MDIO_G2_DEFINE(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct mdio_g2_config mdio_g2_config_##n = {                                  \
		.regs = (eth_registers_t *)DT_REG_ADDR(DT_INST_PARENT(n)),                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.mclk_ahb = MDIO_G2_CLOCK(n, mclk_ahb),                                            \
		.mclk_apb = MDIO_G2_CLOCK(n, mclk_apb),                                            \
		.gclk_tsu = MDIO_G2_CLOCK(n, gclk_tsu),                                            \
	};                                                                                         \
                                                                                                   \
	static struct mdio_g2_data mdio_g2_data_##n;                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mdio_g2_init, NULL, &mdio_g2_data_##n, &mdio_g2_config_##n,       \
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, &mdio_g2_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_G2_DEFINE)
