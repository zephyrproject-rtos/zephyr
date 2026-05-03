/*
 * Copyright (c) 2026 Fiona Behrens (me@kloenk.dev)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numicro_mdio

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numicro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdio_numicro, CONFIG_MDIO_LOG_LEVEL);

#include <NuMicro.h>

#define PHY_RESPONSE_TIMEOUT_MS 20

struct numicro_mdio_config {
	EMAC_T *regs;
	const uint32_t target_freq;
	const struct numicro_scc_subsys clock_subsys;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;
};

struct numicro_mdio_data {
	struct k_sem sem;
};

static int numicro_mdio_transfer(const struct device *dev, uint8_t prtad, uint8_t regad,
				 uint16_t *buf, bool write)
{
	const struct numicro_mdio_config *config = dev->config;
	struct numicro_mdio_data *data = dev->data;
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(PHY_RESPONSE_TIMEOUT_MS));
	uint32_t tmpreg = 0;
	int err = 0;

	if (k_sem_take(&data->sem, K_MSEC(100)) != 0) {
		return -EAGAIN;
	}

	tmpreg = (prtad << EMAC_MIIMCTL_PHYADDR_Pos) | (regad << EMAC_MIIMCTL_PHYREG_Pos) |
		 EMAC_MIIMCTL_BUSY_Msk | EMAC_MIIMCTL_MDCON_Msk;
	if (write) {
		tmpreg |= EMAC_MIIMCTL_WRITE_Msk;
		config->regs->MIIMDAT = *buf;
	}

	config->regs->MIIMCTL = tmpreg;

	while (config->regs->MIIMCTL & EMAC_MIIMCTL_BUSY_Msk) {
		if (sys_timepoint_expired(timeout)) {
			LOG_ERR("Transfer: MDIO timeout");
			err = -EIO;
			goto out;
		}
	}

out:
	if (!write) {
		*buf = (uint16_t)config->regs->MIIMDAT;
	}

	k_sem_give(&data->sem);

	return err;
}

static int numicro_mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	int ret = numicro_mdio_transfer(dev, prtad, regad, data, false);

	LOG_DBG("Fetch %u: %u, %u", prtad, regad, *data);

	return ret;
}

static int numicro_mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	LOG_DBG("Write %u: %u, %u", prtad, regad, data);

	return numicro_mdio_transfer(dev, prtad, regad, &data, true);
}

static DEVICE_API(mdio, numicro_mdio_api) = {
	.read = numicro_mdio_read,
	.write = numicro_mdio_write,
};

static int numicro_mdio_init(const struct device *dev)
{
	const struct numicro_mdio_config *config = dev->config;
	struct numicro_mdio_data *data = dev->data;
	int err;

	/* clean mutable context */
	memset(data, 0x00, sizeof(*data));

	k_sem_init(&data->sem, 1, 1);

	/* Configure pinmux */
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	err = clock_control_on(config->clk_dev,
			       (const clock_control_subsys_t)&config->clock_subsys);
	if (err != 0) {
		return err;
	}

	err = clock_control_configure(config->clk_dev,
				      (const clock_control_subsys_t)&config->clock_subsys, NULL);
	if (err != 0) {
		return err;
	}

	if (config->target_freq != 0) {
		err = clock_control_set_rate(
			config->clk_dev, (const clock_control_subsys_t)&config->clock_subsys,
			(const clock_control_subsys_rate_t)&config->target_freq);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

#define NUMICRO_MDIO_DEVICE(inst)                                                                  \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct numicro_mdio_config numicro_mdio_config_##inst = {                     \
		.regs = (EMAC_T *)DT_REG_ADDR(DT_INST_PARENT(inst)),                               \
		.target_freq = DT_INST_PROP(inst, speed),                                          \
		.clock_subsys =                                                                    \
			{                                                                          \
				.subsys_id = NUMICRO_SCC_SUBSYS_ID_PCC,                            \
				.pcc =                                                             \
					{                                                          \
						.clk_mod = DT_INST_CLOCKS_CELL(                    \
							inst, clock_module_index),                 \
						.clk_src =                                         \
							DT_INST_CLOCKS_CELL(inst, clock_source),   \
						.clk_div =                                         \
							DT_INST_CLOCKS_CELL(inst, clock_divider),  \
					},                                                         \
			},                                                                         \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
	};                                                                                         \
                                                                                                   \
	static struct numicro_mdio_data numicro_mdio_data_##inst;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &numicro_mdio_init, NULL, &numicro_mdio_data_##inst,           \
			      &numicro_mdio_config_##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, \
			      &numicro_mdio_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_MDIO_DEVICE)
