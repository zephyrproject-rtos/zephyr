/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/mdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdio_mchp_gmac_g1, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_gmac_g1_mdio

#define MDIO_MCHP_OP_TIMEOUT 25

struct mdio_dev_data {
	struct k_mutex reg_mutex;
};

struct mdio_dev_config {
	const struct pinctrl_dev_config *pinctrl_cfg;
	gmac_registers_t *const gmac_regs;
};

static inline int mdio_transfer(gmac_registers_t *regs, uint8_t prtad, uint8_t regad,
				enum mdio_opcode op, bool c45, uint16_t data_in, uint16_t *data_out)
{
	uint32_t reg_val = (c45 ? 0U : GMAC_MAN_CLTTO_Msk) | GMAC_MAN_OP(op) | GMAC_MAN_WTN(0x02) |
			   GMAC_MAN_PHYA(prtad) | GMAC_MAN_REGA(regad) | GMAC_MAN_DATA(data_in);

	regs->GMAC_MAN = reg_val;
	if (WAIT_FOR((regs->GMAC_NSR & GMAC_NSR_IDLE_Msk), MDIO_MCHP_OP_TIMEOUT, k_busy_wait(1)) ==
	    false) {
		return -ETIME;
	}

	if ((data_out) != NULL) {
		*(data_out) = regs->GMAC_MAN & GMAC_MAN_DATA_Msk;
	}

	return 0;
}

static int mdio_mchp_read(const struct device *dev, uint8_t port_addr, uint8_t reg_addr,
			  uint16_t *data)
{
	int ret;
	struct mdio_dev_data *const mdio_data = dev->data;
	const struct mdio_dev_config *const cfg = dev->config;

	k_mutex_lock(&mdio_data->reg_mutex, K_FOREVER);

	ret = mdio_transfer(cfg->gmac_regs, port_addr, reg_addr, MDIO_OP_C22_READ, false, 0, data);
	k_mutex_unlock(&mdio_data->reg_mutex);

	return ret;
}

static int mdio_mchp_write(const struct device *dev, uint8_t port_addr, uint8_t reg_addr,
			   uint16_t data)
{
	int ret;
	struct mdio_dev_data *const mdio_data = dev->data;
	const struct mdio_dev_config *const cfg = dev->config;

	k_mutex_lock(&mdio_data->reg_mutex, K_FOREVER);

	ret = mdio_transfer(cfg->gmac_regs, port_addr, reg_addr, MDIO_OP_C22_WRITE, false, data,
			    NULL);

	k_mutex_unlock(&mdio_data->reg_mutex);

	return ret;
}

static int mdio_mchp_read_c45(const struct device *dev, uint8_t port_addr, uint8_t devad,
			      uint16_t reg_addr, uint16_t *data)
{
	int err;
	struct mdio_dev_data *const mdio_data = dev->data;
	const struct mdio_dev_config *const cfg = dev->config;

	k_mutex_lock(&mdio_data->reg_mutex, K_FOREVER);

	err = mdio_transfer(cfg->gmac_regs, port_addr, devad, MDIO_OP_C45_ADDRESS, true, reg_addr,
			    NULL);
	if (err == 0) {
		err = mdio_transfer(cfg->gmac_regs, port_addr, devad, MDIO_OP_C45_READ, true, 0,
				    data);
	}

	k_mutex_unlock(&mdio_data->reg_mutex);

	return err;
}

static int mdio_mchp_write_c45(const struct device *dev, uint8_t port_addr, uint8_t devad,
			       uint16_t reg_addr, uint16_t data)
{
	int err;
	struct mdio_dev_data *const mdio_data = dev->data;
	const struct mdio_dev_config *const cfg = dev->config;

	k_mutex_lock(&mdio_data->reg_mutex, K_FOREVER);

	err = mdio_transfer(cfg->gmac_regs, port_addr, devad, MDIO_OP_C45_ADDRESS, true, reg_addr,
			    NULL);
	if (err == 0) {
		err = mdio_transfer(cfg->gmac_regs, port_addr, devad, MDIO_OP_C45_WRITE, true, data,
				    NULL);
	}

	k_mutex_unlock(&mdio_data->reg_mutex);

	return err;
}

/* Declare pin-ctrl __pinctrl_dev_config__device_dts_ord_xx before
 * PINCTRL_DT_INST_DEV_CONFIG_GET()
 */
PINCTRL_DT_INST_DEFINE(0);

static int mdio_mchp_initialize(const struct device *dev)
{
	const struct mdio_dev_config *const cfg = dev->config;
	struct mdio_dev_data *const data = dev->data;
	int retval;

	k_mutex_init(&data->reg_mutex);
	retval = pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_DEFAULT);
	if (retval != 0) {
		LOG_ERR("pinctrl_apply_state() Failed for mdio driver: %d", retval);
	}

	return retval;
}

static DEVICE_API(mdio, mdio_mchp_driver_api) = {
	.read = mdio_mchp_read,
	.write = mdio_mchp_write,
	.read_c45 = mdio_mchp_read_c45,
	.write_c45 = mdio_mchp_write_c45,
};

static const struct mdio_dev_config mdio_dev_cfg = {
	.pinctrl_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.gmac_regs = (gmac_registers_t *)DT_REG_ADDR(DT_INST_PARENT(0))};

static struct mdio_dev_data mdio_data;

DEVICE_DT_INST_DEFINE(0, &mdio_mchp_initialize, NULL, &mdio_data, &mdio_dev_cfg, POST_KERNEL,
		      CONFIG_MDIO_INIT_PRIORITY, &mdio_mchp_driver_api);
