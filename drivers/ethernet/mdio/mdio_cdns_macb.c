/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MDIO controller of the Cadence MACB/GEM, a child node of the MAC. The MAC
 * driver enables the management port and sets the MDC divider.
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

static inline bool mdio_cdns_macb_is_idle(mm_reg_t base)
{
	return (sys_read32(base + MACB_NSR) & MACB_NSR_IDLE) != 0U;
}

static bool mdio_cdns_macb_wait_idle(mm_reg_t base)
{
	return WAIT_FOR(mdio_cdns_macb_is_idle(base), CONFIG_MDIO_CDNS_MACB_IDLE_TIMEOUT_US,
			k_busy_wait(1));
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
	mm_reg_t base = cdns_macb_reg_base(cfg->mac_dev);
	uint32_t man;

	if (!mdio_cdns_macb_wait_idle(base)) {
		LOG_ERR("%s: MDIO bus busy (op %u, PHY %u, reg 0x%02x)", dev->name, (uint32_t)op,
			prtad, regad);
		return -ETIMEDOUT;
	}

	man = FIELD_PREP(MACB_MAN_SOF, c45 ? MACB_MAN_C45_SOF : MACB_MAN_C22_SOF) |
	      FIELD_PREP(MACB_MAN_RW, op) | FIELD_PREP(MACB_MAN_PHYA, prtad) |
	      FIELD_PREP(MACB_MAN_REGA, regad) | FIELD_PREP(MACB_MAN_CODE, MACB_MAN_CODE_VALUE) |
	      FIELD_PREP(MACB_MAN_DATA, data_in);

	sys_write32(man, base + MACB_MAN);

	if (!mdio_cdns_macb_wait_idle(base)) {
		LOG_ERR("%s: MDIO transfer timed out (op %u, PHY %u, reg 0x%02x)", dev->name,
			(uint32_t)op, prtad, regad);
		return -ETIMEDOUT;
	}

	if (data_out != NULL) {
		*data_out = FIELD_GET(MACB_MAN_DATA, sys_read32(base + MACB_MAN));
	}

	return 0;
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
	int ret;

	ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_ADDRESS, true, regad, NULL);
	if (ret == 0) {
		ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_READ, true, 0U, data);
	}

	return ret;
}

static int mdio_cdns_macb_write_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				    uint16_t regad, uint16_t data)
{
	int ret;

	ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_ADDRESS, true, regad, NULL);
	if (ret == 0) {
		ret = mdio_cdns_macb_transfer(dev, prtad, devad, MDIO_OP_C45_WRITE, true, data,
					      NULL);
	}

	return ret;
}

static int mdio_cdns_macb_init(const struct device *dev)
{
	const struct mdio_cdns_macb_config *cfg = dev->config;

	/* The MAC maps the registers and enables the management port */
	if (!device_is_ready(cfg->mac_dev)) {
		LOG_ERR("%s: parent MAC device not ready", dev->name);
		return -ENODEV;
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
	DEVICE_DT_INST_DEFINE(n, mdio_cdns_macb_init, NULL, NULL, &mdio_cdns_macb_config_##n,      \
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, &mdio_cdns_macb_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_CDNS_MACB_DEVICE)
