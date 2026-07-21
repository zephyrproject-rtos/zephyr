/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/ethernet.h>
#include "../eth_renesas_ra_rmac.h"
#include <zephyr/net/mdio.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(renesas_ra_mdio, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_mdio_rmac

#define RENESAS_RA_MDIO_TIMEOUT_US   (1000)
#define RENESAS_RA_MDIO_OPCODE_WRITE (0x1)
#define RENESAS_RA_MDIO_OPCODE_READ  (0x2)

struct mdio_renesas_ra_config {
	const struct device *eswclk_dev;
	const struct pinctrl_dev_config *pincfg;
	uint32_t mdc_clock_rate;
	struct {
		volatile uint32_t *mpsm;
		volatile uint32_t *mpic;
	} reg;
	uint8_t channel;
	uint8_t mdio_hold_time;
	uint8_t mdio_capture_time;
};

struct mdio_renesas_ra_data {
	struct k_mutex rw_lock;
};

static int mdio_renesas_ra_read(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t *regval)
{
	struct mdio_renesas_ra_data *data = dev->data;
	const struct mdio_renesas_ra_config *config = dev->config;
	uint32_t mpsm = 0;
	uint8_t phy_mode;
	int ret = 0;

	phy_mode = r_rmac_phy_get_operation_mode(config->channel);
	if (phy_mode != RENESAS_RA_ETHA_OPERATION_MODE) {
		return -EACCES;
	}

	k_mutex_lock(&data->rw_lock, K_FOREVER);

	mpsm |= R_RMAC0_MPSM_POP_Msk & (RENESAS_RA_MDIO_OPCODE_READ << R_RMAC0_MPSM_POP_Pos);
	mpsm |= R_RMAC0_MPSM_PDA_Msk & ((uint32_t)prtad << R_RMAC0_MPSM_PDA_Pos);
	mpsm |= R_RMAC0_MPSM_PRA_Msk & (regad << R_RMAC0_MPSM_PRA_Pos);

	*config->reg.mpsm = mpsm;

	/* Start read access to the phy register and wait for completion. */
	*config->reg.mpsm |= R_RMAC0_MPSM_PSME_Msk;

	if (!WAIT_FOR(((*config->reg.mpsm & R_RMAC0_MPSM_PSME_Msk) == 0),
		      RENESAS_RA_MDIO_TIMEOUT_US, NULL)) {
		LOG_ERR("MDIO read operation timed out");
		ret = -ETIMEDOUT;
	}

	*regval = (*config->reg.mpsm & R_RMAC0_MPSM_PRD_Msk) >> R_RMAC0_MPSM_PRD_Pos;

	k_mutex_unlock(&data->rw_lock);

	return ret;
}

static int mdio_renesas_ra_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				 uint16_t regval)
{
	struct mdio_renesas_ra_data *data = dev->data;
	const struct mdio_renesas_ra_config *config = dev->config;
	uint32_t mpsm = 0;
	uint8_t phy_mode;
	int ret = 0;

	phy_mode = r_rmac_phy_get_operation_mode(config->channel);
	if (phy_mode != RENESAS_RA_ETHA_OPERATION_MODE) {
		return -EACCES;
	}

	k_mutex_lock(&data->rw_lock, K_FOREVER);

	mpsm |= R_RMAC0_MPSM_POP_Msk & (RENESAS_RA_MDIO_OPCODE_WRITE << R_RMAC0_MPSM_POP_Pos);
	mpsm |= R_RMAC0_MPSM_PDA_Msk & ((uint32_t)prtad << R_RMAC0_MPSM_PDA_Pos);
	mpsm |= R_RMAC0_MPSM_PRA_Msk & (regad << R_RMAC0_MPSM_PRA_Pos);
	mpsm |= R_RMAC0_MPSM_PRD_Msk & ((uint32_t)regval << R_RMAC0_MPSM_PRD_Pos);

	/* Set configuration value. */
	*config->reg.mpsm = mpsm;

	/* Start write access to the phy register and wait for completion. */
	*config->reg.mpsm |= R_RMAC0_MPSM_PSME_Msk;

	if (!WAIT_FOR(((*config->reg.mpsm & R_RMAC0_MPSM_PSME_Msk) == 0),
		      RENESAS_RA_MDIO_TIMEOUT_US, NULL)) {
		LOG_ERR("MDIO write operation timed out");
		ret = -ETIMEDOUT;
	}

	k_mutex_unlock(&data->rw_lock);

	return ret;
}

static int mdio_renesas_ra_initialize(const struct device *dev)
{
	struct mdio_renesas_ra_data *data = dev->data;
	const struct mdio_renesas_ra_config *config = dev->config;
	uint32_t eswclk;
	uint32_t mpic_psmcs;
	uint8_t mdio_hold_time;
	uint8_t mdio_capture_time;

	int ret = 0;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->eswclk_dev, NULL, &eswclk);
	if (ret < 0) {
		return ret;
	}

	mdio_hold_time = DIV_ROUND_UP((config->mdio_hold_time * eswclk), 1e9);
	mdio_capture_time = DIV_ROUND_UP((config->mdio_capture_time * eswclk), 1e9);
	mpic_psmcs = ((eswclk / config->mdc_clock_rate) / 2) - 1;

	r_rmac_phy_set_operation_mode(config->channel, RENESAS_RA_ETHA_DISABLE_MODE);
	r_rmac_phy_set_operation_mode(config->channel, RENESAS_RA_ETHA_CONFIG_MODE);
	*config->reg.mpic =
		(R_RMAC0_MPIC_PSMCS_Msk & (mpic_psmcs << R_RMAC0_MPIC_PSMCS_Pos)) |
		(R_RMAC0_MPIC_PSMCT_Msk & ((uint32_t)mdio_capture_time << R_RMAC0_MPIC_PSMCT_Pos)) |
		(R_RMAC0_MPIC_PSMHT_Msk & ((uint32_t)mdio_hold_time << R_RMAC0_MPIC_PSMHT_Pos));
	r_rmac_phy_set_operation_mode(config->channel, RENESAS_RA_ETHA_DISABLE_MODE);
	r_rmac_phy_set_operation_mode(config->channel, RENESAS_RA_ETHA_OPERATION_MODE);

	k_mutex_init(&data->rw_lock);

	return 0;
}

static DEVICE_API(mdio, mdio_renesas_ra_api) = {
	.read = mdio_renesas_ra_read,
	.write = mdio_renesas_ra_write,
};

#define RENSAS_RA_MDIO_RMAC_DEFINE(n)                                                              \
	BUILD_ASSERT(DT_INST_CHILD_NUM(n) <= BSP_FEATURE_ETHER_NUM_CHANNELS);                      \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct mdio_renesas_ra_data renesas_ra_mdio##n##_data;                              \
	static const struct mdio_renesas_ra_config renesas_ra_mdio##n##_cfg = {                    \
		.eswclk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(DT_INST_PARENT(n), eswclk)),    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.mdc_clock_rate = DT_INST_PROP(n, clock_frequency),                                \
		.reg =                                                                             \
			{                                                                          \
				.mpsm = (uint32_t *)DT_INST_REG_ADDR_BY_NAME(n, mpsm),             \
				.mpic = (uint32_t *)DT_INST_REG_ADDR_BY_NAME(n, mpic),             \
			},                                                                         \
		.channel = DT_INST_PROP(n, channel),                                               \
		.mdio_hold_time = DT_INST_PROP(n, mdio_hold_time_ns),                              \
		.mdio_capture_time = DT_INST_PROP(n, mdio_capture_time_ns),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &mdio_renesas_ra_initialize, NULL, &renesas_ra_mdio##n##_data,    \
			      &renesas_ra_mdio##n##_cfg, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_renesas_ra_api);

DT_INST_FOREACH_STATUS_OKAY(RENSAS_RA_MDIO_RMAC_DEFINE)
