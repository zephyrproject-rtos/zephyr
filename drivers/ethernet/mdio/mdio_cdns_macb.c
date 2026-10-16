/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MDIO controller of the Cadence MACB/GEM, a child node of the MAC. The MAC
 * driver enables the management port, sets the MDC divider and signals the
 * management frame done interrupt.
 */

#define DT_DRV_COMPAT cdns_macb_mdio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_mdio, CONFIG_MDIO_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mdio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "../cdns_macb/eth_cdns_macb_priv.h"

struct mdio_cdns_macb_config {
	const struct device *mac_dev;
};

struct mdio_cdns_macb_data {
	struct k_mutex lock;
};

static inline bool mdio_cdns_macb_is_idle(mm_reg_t base)
{
	return (sys_read32(base + MACB_NSR) & MACB_NSR_IDLE) != 0U;
}

static bool mdio_cdns_macb_wait_idle(mm_reg_t base)
{
	return WAIT_FOR(mdio_cdns_macb_is_idle(base), CONFIG_MDIO_CDNS_MACB_IDLE_TIMEOUT_US,
			k_busy_wait(1));
}

/* Wait for the management frame just started to be done */
static bool mdio_cdns_macb_wait_done(const struct device *dev)
{
	const struct mdio_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *mac_data = cfg->mac_dev->data;

	if (IS_ENABLED(CONFIG_MDIO_CDNS_MACB_IRQ) &&
	    (k_sem_take(&mac_data->mdio_done, K_USEC(CONFIG_MDIO_CDNS_MACB_IDLE_TIMEOUT_US)) !=
	     0)) {
		return false;
	}

	/* After the interrupt this only confirms that the bus is idle, otherwise it polls */
	return mdio_cdns_macb_wait_idle(cdns_macb_reg_base(cfg->mac_dev));
}

/*
 * One management frame. The MAN register takes the Zephyr MDIO opcodes as
 * they are: Clause 22 write 1 / read 2, Clause 45 address 0 / write 1 / read 3.
 */
static int mdio_cdns_macb_transfer(const struct device *dev, uint8_t prtad, uint8_t regad,
				   enum mdio_opcode op, bool c45, uint16_t data_in,
				   uint16_t *data_out)
{
	const struct mdio_cdns_macb_config *cfg = dev->config;
	struct mdio_cdns_macb_data *data = dev->data;
	struct cdns_macb_priv *mac_data = cfg->mac_dev->data;
	mm_reg_t base = cdns_macb_reg_base(cfg->mac_dev);
	uint32_t man;
	int ret = 0;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	if (!mdio_cdns_macb_wait_idle(base)) {
		LOG_ERR("%s: MDIO bus busy (op %u, PHY %u, reg 0x%02x)", dev->name, (uint32_t)op,
			prtad, regad);
		ret = -ETIMEDOUT;
		goto unlock;
	}

	if (IS_ENABLED(CONFIG_MDIO_CDNS_MACB_IRQ)) {
		k_sem_reset(&mac_data->mdio_done);
	}

	man = FIELD_PREP(MACB_MAN_SOF, c45 ? MACB_MAN_C45_SOF : MACB_MAN_C22_SOF) |
	      FIELD_PREP(MACB_MAN_RW, op) | FIELD_PREP(MACB_MAN_PHYA, prtad) |
	      FIELD_PREP(MACB_MAN_REGA, regad) | FIELD_PREP(MACB_MAN_CODE, MACB_MAN_CODE_VALUE) |
	      FIELD_PREP(MACB_MAN_DATA, data_in);

	sys_write32(man, base + MACB_MAN);

	if (!mdio_cdns_macb_wait_done(dev)) {
		LOG_ERR("%s: MDIO transfer timed out (op %u, PHY %u, reg 0x%02x)", dev->name,
			(uint32_t)op, prtad, regad);
		ret = -ETIMEDOUT;
		goto unlock;
	}

	if (data_out != NULL) {
		*data_out = FIELD_GET(MACB_MAN_DATA, sys_read32(base + MACB_MAN));
	}

unlock:
	(void)k_mutex_unlock(&data->lock);

	return ret;
}

static int mdio_cdns_macb_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t *data)
{
	return mdio_cdns_macb_transfer(dev, prtad, regad, MDIO_OP_C22_READ, false, 0U, data);
}

static int mdio_cdns_macb_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t data)
{
	return mdio_cdns_macb_transfer(dev, prtad, regad, MDIO_OP_C22_WRITE, false, data, NULL);
}

static int mdio_cdns_macb_read_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				   uint16_t regad, uint16_t *data)
{
	struct mdio_cdns_macb_data *drv_data = dev->data;
	int ret;

	/* Keep the address and the data frame together */
	(void)k_mutex_lock(&drv_data->lock, K_FOREVER);

	ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_ADDRESS, true, regad, NULL);
	if (ret == 0) {
		ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_READ, true, 0U, data);
	}

	(void)k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int mdio_cdns_macb_write_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				    uint16_t regad, uint16_t data)
{
	struct mdio_cdns_macb_data *drv_data = dev->data;
	int ret;

	/* Keep the address and the data frame together */
	(void)k_mutex_lock(&drv_data->lock, K_FOREVER);

	ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_ADDRESS, true, regad, NULL);
	if (ret == 0) {
		ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_WRITE, true, data,
					      NULL);
	}

	(void)k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int mdio_cdns_macb_init(const struct device *dev)
{
	const struct mdio_cdns_macb_config *cfg = dev->config;
	struct mdio_cdns_macb_data *data = dev->data;

	/* The MAC maps the registers and enables the management port */
	if (!device_is_ready(cfg->mac_dev)) {
		LOG_ERR("%s: parent MAC device not ready", dev->name);
		return -ENODEV;
	}

	(void)k_mutex_init(&data->lock);

	if (IS_ENABLED(CONFIG_MDIO_CDNS_MACB_IRQ)) {
		sys_write32(MACB_INT_MFD, cdns_macb_reg_base(cfg->mac_dev) + MACB_IER);
	}

	return 0;
}

static DEVICE_API(mdio, mdio_cdns_macb_api) = {
	.read = mdio_cdns_macb_read,
	.write = mdio_cdns_macb_write,
	.read_c45 = mdio_cdns_macb_read_c45,
	.write_c45 = mdio_cdns_macb_write_c45,
};

#define MDIO_CDNS_MACB_DEVICE(n)                                                                   \
	static const struct mdio_cdns_macb_config mdio_cdns_macb_config_##n = {                    \
		.mac_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
	};                                                                                         \
                                                                                                   \
	static struct mdio_cdns_macb_data mdio_cdns_macb_data_##n;                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mdio_cdns_macb_init, NULL, &mdio_cdns_macb_data_##n,              \
			      &mdio_cdns_macb_config_##n, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,  \
			      &mdio_cdns_macb_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_CDNS_MACB_DEVICE)
