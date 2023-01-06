/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwcxgmac_mdio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(snps_dwcxgmac_mdio, CONFIG_MDIO_LOG_LEVEL);

#define CORE_MDIO_SINGLE_COMMAND_ADDRESS_OFST  0x200
#define CORE_MDIO_SINGLE_COMMAND_ADDRESS_RA_SET(value) (((value) << 0) & 0x0000ffff)
#define CORE_MDIO_SINGLE_COMMAND_ADDRESS_PA_SET(value) (((value) << 16) & 0x001f0000)
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_OFST  0x204
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_SET_MSK    0x00400000
#define CORE_MDIO_CLAUSE_22_PORT_OFST  0x220
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SDATA_SET(value) (((value) << 0) & 0x0000ffff)
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD_SET(value) (((value) << 16) & 0x00030000)
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CR_SET(value) (((value) << 19) & 0x00380000)
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CRS_SET(value) (((value) << 31) & 0x80000000)
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_SET(value) (((value) << 22) & 0x00400000)
#define CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SDATA_GET(value) (((value)&0x0000ffff) >> 0)

struct mdio_dwcxgmac_dev_data {
	struct k_mutex mdio_transfer_lock;
};

struct mdio_dwcxgmac_dev_config {
	const struct device *xgmac_dev;
	uint32_t clk_range;
	bool clk_range_sel;
};

static int mdio_busy_wait(uint32_t reg_addr, uint32_t bit_msk, uint32_t retries)
{
	while (sys_read32(reg_addr) & bit_msk) {
		if (retries-- == 0) {
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(10)); /* 10ms polling interval */
	}

	return 0;
}

static int mdio_transfer(const struct device *dev, uint8_t prtad, uint8_t devad, uint8_t rw,
			 uint16_t data_in, uint16_t *data_out)
{
	const struct mdio_dwcxgmac_dev_config *const cfg =
		(struct mdio_dwcxgmac_dev_config *)dev->config;
	struct mdio_dwcxgmac_dev_data *const data = (struct mdio_dwcxgmac_dev_data *)dev->data;
	int retval;
	mem_addr_t ioaddr = 0;
	uint32_t reg_data, mdio_addr, mdio_data = 0;

	ioaddr = (mem_addr_t)DEVICE_MMIO_GET(cfg->xgmac_dev);

	retval = mdio_busy_wait((ioaddr + CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_OFST),
				CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_SET_MSK, 100);

	if (retval) {
		LOG_ERR("MDIO device busy wait timedout");
		return retval;
	}

	(void)k_mutex_lock(&data->mdio_transfer_lock, K_FOREVER);
	/* Set port as Clause 22 */
	reg_data = sys_read32(ioaddr + CORE_MDIO_CLAUSE_22_PORT_OFST);
	reg_data |= BIT(prtad);
	sys_write32(reg_data, ioaddr + CORE_MDIO_CLAUSE_22_PORT_OFST);

	mdio_addr = CORE_MDIO_SINGLE_COMMAND_ADDRESS_RA_SET(devad) |
		    CORE_MDIO_SINGLE_COMMAND_ADDRESS_PA_SET(prtad);
	sys_write32(mdio_addr, (ioaddr + CORE_MDIO_SINGLE_COMMAND_ADDRESS_OFST));

	mdio_data = CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SDATA_SET(data_in) |
		    CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD_SET(rw) |
		    CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CR_SET(cfg->clk_range) |
		    CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CRS_SET(cfg->clk_range_sel) |
		    CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_SET(1u);

	sys_write32(mdio_data, (ioaddr + CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_OFST));

	retval = mdio_busy_wait((ioaddr + CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_OFST),
				CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_SET_MSK, 100);

	if (retval) {
		LOG_ERR("transfer timedout");
	} else {
		if (data_out) {
			*data_out = CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SDATA_GET(
				sys_read32(ioaddr + CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_OFST));
		}
	}

	(void)k_mutex_unlock(&data->mdio_transfer_lock);

	return retval;
}

static int mdio_dwcxgmac_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			      uint16_t *data)
{
	return mdio_transfer(dev, prtad, devad, 1, 0, data);
}

static int mdio_dwcxgmac_write(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t data)
{
	return mdio_transfer(dev, prtad, devad, 0, data, NULL);
}

static void mdio_dwcxgmac_bus_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void mdio_dwcxgmac_bus_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int mdio_dwcxgmac_initialize(const struct device *dev)
{
	struct mdio_dwcxgmac_dev_data *const data = (struct mdio_dwcxgmac_dev_data *)dev->data;
	const struct mdio_dwcxgmac_dev_config *const cfg =
		(struct mdio_dwcxgmac_dev_config *)dev->config;

	DEVICE_MMIO_MAP(cfg->xgmac_dev, K_MEM_CACHE_NONE);
	k_mutex_init(&data->mdio_transfer_lock);
	return 0;
}

static const struct mdio_driver_api mdio_dwcxgmac_driver_api = {
	.read = mdio_dwcxgmac_read,
	.write = mdio_dwcxgmac_write,
	.bus_enable = mdio_dwcxgmac_bus_enable,
	.bus_disable = mdio_dwcxgmac_bus_disable,
};

#define MDIO_DWCXGMAC_CONFIG(n)                                                                    \
	static const struct mdio_dwcxgmac_dev_config mdio_dwcxgmac_dev_config_##n = {              \
		.xgmac_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                     \
		.clk_range = DT_INST_PROP(n, csr_clock_indx),                                      \
		.clk_range_sel = DT_INST_PROP(n, clock_range_sel),                                 \
	};

#define MDIO_DWCXGMAC_DEVICE(n)                                                                    \
	MDIO_DWCXGMAC_CONFIG(n);                                                                   \
	static struct mdio_dwcxgmac_dev_data mdio_dwcxgmac_dev_data##n;                            \
	DEVICE_DT_INST_DEFINE(n, &mdio_dwcxgmac_initialize, NULL, &mdio_dwcxgmac_dev_data##n,      \
			      &mdio_dwcxgmac_dev_config_##n, POST_KERNEL,                          \
			      CONFIG_MDIO_INIT_PRIORITY, &mdio_dwcxgmac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_DWCXGMAC_DEVICE)
