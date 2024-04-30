/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_enet_qos_mdio

#include <zephyr/net/mdio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/ethernet/eth_nxp_enet_qos.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_nxp_enet_qos, CONFIG_MDIO_LOG_LEVEL);

struct nxp_enet_qos_mdio_config {
	const struct device *enet_dev;
};

struct nxp_enet_qos_mdio_data {
	struct k_mutex mdio_mutex;
};

struct mdio_transaction {
	enum mdio_opcode op;
	union {
		uint16_t write_data;
		uint16_t *read_data;
	};
	uint8_t portaddr;
	uint8_t regaddr;
	enet_qos_t *base;
	struct k_mutex *mdio_bus_mutex;
};

static bool check_busy(enet_qos_t *base)
{
	uint32_t val = base->MAC_MDIO_ADDRESS;

	/* Return the busy bit */
	return ENET_QOS_REG_GET(MAC_MDIO_ADDRESS, GB, val);
}

static int do_transaction(struct mdio_transaction *mdio)
{
	enet_qos_t *base = mdio->base;
	uint8_t goc_1_code;
	int ret;

	k_mutex_lock(mdio->mdio_bus_mutex, K_FOREVER);

	if (mdio->op == MDIO_OP_C22_WRITE) {
		base->MAC_MDIO_DATA =
			/* Prepare the data to be written */
			ENET_QOS_REG_PREP(MAC_MDIO_DATA, GD, mdio->write_data);
		goc_1_code = 0b0;
	} else if (mdio->op == MDIO_OP_C22_READ) {
		goc_1_code = 0b1;
	} else {
		ret = -EINVAL;
		goto done;
	}

	base->MAC_MDIO_ADDRESS =
		/* OP command */
		ENET_QOS_REG_PREP(MAC_MDIO_ADDRESS, GOC_1, goc_1_code) |
		ENET_QOS_REG_PREP(MAC_MDIO_ADDRESS, GOC_0, 0b1) |
		/* PHY address */
		ENET_QOS_REG_PREP(MAC_MDIO_ADDRESS, PA, mdio->portaddr) |
		/* Register address */
		ENET_QOS_REG_PREP(MAC_MDIO_ADDRESS, RDA, mdio->regaddr);

	base->MAC_MDIO_ADDRESS =
		/* Start the transaction */
		ENET_QOS_REG_PREP(MAC_MDIO_ADDRESS, GB, 0b1);


	ret = -ETIMEDOUT;
	for (int i = CONFIG_MDIO_NXP_ENET_QOS_RECHECK_COUNT; i > 0; i--) {
		if (!check_busy(base)) {
			ret = 0;
			break;
		}
		k_busy_wait(CONFIG_MDIO_NXP_ENET_QOS_RECHECK_TIME);
	}

	if (ret) {
		LOG_ERR("MDIO transaction timed out");
		goto done;
	}

	if (mdio->op == MDIO_OP_C22_READ) {
		uint32_t val = mdio->base->MAC_MDIO_DATA;

		*mdio->read_data =
			/* Decipher the read data */
			ENET_QOS_REG_GET(MAC_MDIO_DATA, GD, val);
	}

done:
	k_mutex_unlock(mdio->mdio_bus_mutex);

	return ret;
}

static int nxp_enet_qos_mdio_read(const struct device *dev,
				  uint8_t portaddr, uint8_t regaddr,
				  uint16_t *read_data)
{
	const struct nxp_enet_qos_mdio_config *config = dev->config;
	struct nxp_enet_qos_mdio_data *data = dev->data;
	enet_qos_t *base = ENET_QOS_MODULE_CFG(config->enet_dev)->base;
	struct mdio_transaction mdio_read = {
		.op = MDIO_OP_C22_READ,
		.read_data = read_data,
		.portaddr = portaddr,
		.regaddr = regaddr,
		.base = base,
		.mdio_bus_mutex = &data->mdio_mutex,
	};

	return do_transaction(&mdio_read);
}

static int nxp_enet_qos_mdio_write(const struct device *dev,
				   uint8_t portaddr, uint8_t regaddr,
				   uint16_t write_data)
{
	const struct nxp_enet_qos_mdio_config *config = dev->config;
	struct nxp_enet_qos_mdio_data *data = dev->data;
	enet_qos_t *base = ENET_QOS_MODULE_CFG(config->enet_dev)->base;
	struct mdio_transaction mdio_write = {
		.op = MDIO_OP_C22_WRITE,
		.write_data = write_data,
		.portaddr = portaddr,
		.regaddr = regaddr,
		.base = base,
		.mdio_bus_mutex = &data->mdio_mutex,
	};

	return do_transaction(&mdio_write);
}

static void nxp_enet_qos_mdio_bus_fn(const struct device *dev)
{
	/* Intentionally empty. IP does not support this functionality. */
	ARG_UNUSED(dev);
}

static const struct mdio_driver_api nxp_enet_qos_mdio_api = {
	.read = nxp_enet_qos_mdio_read,
	.write = nxp_enet_qos_mdio_write,
	.bus_enable = nxp_enet_qos_mdio_bus_fn,
	.bus_disable = nxp_enet_qos_mdio_bus_fn,
};

static int nxp_enet_qos_mdio_init(const struct device *dev)
{
	const struct nxp_enet_qos_mdio_config *mdio_config = dev->config;
	struct nxp_enet_qos_mdio_data *data = dev->data;
	const struct nxp_enet_qos_config *config = ENET_QOS_MODULE_CFG(mdio_config->enet_dev);
	uint32_t enet_module_clk_rate;
	int ret, divider;

	ret = k_mutex_init(&data->mdio_mutex);
	if (ret) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys,
				     &enet_module_clk_rate);
	if (ret) {
		return ret;
	}

	enet_module_clk_rate /= 1000000;
	if (enet_module_clk_rate >= 20 && enet_module_clk_rate < 35) {
		divider = 2;
	} else if (enet_module_clk_rate < 60) {
		divider = 3;
	} else if (enet_module_clk_rate < 100) {
		divider = 0;
	} else if (enet_module_clk_rate < 150) {
		divider = 1;
	} else if (enet_module_clk_rate < 250) {
		divider = 4;
	} else {
		LOG_ERR("ENET QOS clk rate does not allow MDIO");
		return -ENOTSUP;
	}

	config->base->MAC_MDIO_ADDRESS =
		/* Configure the MDIO clock range / divider */
		ENET_QOS_REG_PREP(MAC_MDIO_ADDRESS, CR, divider);

	return 0;
}

#define NXP_ENET_QOS_MDIO_INIT(inst)						\
										\
	static const struct nxp_enet_qos_mdio_config				\
	nxp_enet_qos_mdio_cfg_##inst = {					\
		.enet_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),		\
	};									\
										\
	static struct nxp_enet_qos_mdio_data nxp_enet_qos_mdio_data_##inst;	\
										\
	DEVICE_DT_INST_DEFINE(inst, &nxp_enet_qos_mdio_init, NULL,		\
			      &nxp_enet_qos_mdio_data_##inst,			\
			      &nxp_enet_qos_mdio_cfg_##inst,			\
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,		\
			      &nxp_enet_qos_mdio_api);				\

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_QOS_MDIO_INIT)
