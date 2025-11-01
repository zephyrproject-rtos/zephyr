/*
 * Xilinx AXI Ethernet Lite MDIO
 *
 * Copyright(c) 2025, CISPA Helmholtz Center for Information Security
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_axi_eth_lite, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/device.h>

#include <stdint.h>

#define AXI_ETH_LITE_MAX_PHY_DEVICES 32

#define AXI_ETH_LITE_MDIO_ADDRESS_REG_OFFSET    0x07e4
#define AXI_ETH_LITE_MDIO_WRITE_DATA_REG_OFFSET 0x07e8
#define AXI_ETH_LITE_MDIO_READ_DATA_REG_OFFSET  0x07ec
#define AXI_ETH_LITE_MDIO_CONTROL_REG_OFFSET    0x07f0

#define AXI_ETH_LITE_MDIO_CONTROL_REG_MDIO_ENABLE_MASK  BIT(3)
#define AXI_ETH_LITE_MDIO_CONTROL_REG_MDIO_BUSY_MASK    BIT(0)
#define AXI_ETH_LITE_MDIO_CONTROL_REG_MDIO_DISABLE_MASK 0

#define AXI_ETH_LITE_MDIO_ADDRESS_REG_OP_READ       BIT(10)
#define AXI_ETH_LITE_MDIO_ADDRESS_REG_OP_WRITE      0
#define AXI_ETH_LITE_MDIO_ADDRESS_REG_SHIFT_REGADDR 0
#define AXI_ETH_LITE_MDIO_ADDRESS_REG_SHIFT_PHYADDR 5

struct axi_eth_lite_data {
	struct k_mutex mutex;
};

struct axi_eth_lite_config {
	void *reg;
};

static inline uint32_t axi_eth_lite_read_reg(const struct axi_eth_lite_config *config,
					     mem_addr_t reg)
{
	return sys_read32((mem_addr_t)config->reg + reg);
}

static inline void axi_eth_lite_write_reg(const struct axi_eth_lite_config *config, mem_addr_t reg,
					  uint32_t value)
{
	sys_write32(value, (mem_addr_t)config->reg + reg);
}

static inline int axi_eth_lite_check_busy(const struct axi_eth_lite_config *config)
{
	uint32_t mdio_control_reg_val =
		axi_eth_lite_read_reg(config, AXI_ETH_LITE_MDIO_CONTROL_REG_OFFSET);

	return mdio_control_reg_val & AXI_ETH_LITE_MDIO_CONTROL_REG_MDIO_BUSY_MASK ? -EBUSY : 0;
}

static inline void axi_eth_lite_set_addr(const struct axi_eth_lite_config *config, uint8_t prtad,
					 uint8_t regad, bool is_read)
{
	uint32_t mdio_addr_val = is_read ? AXI_ETH_LITE_MDIO_ADDRESS_REG_OP_READ
					 : AXI_ETH_LITE_MDIO_ADDRESS_REG_OP_WRITE;

	/* range check done below in read/write functions */
	mdio_addr_val |= (uint32_t)regad << AXI_ETH_LITE_MDIO_ADDRESS_REG_SHIFT_REGADDR;
	mdio_addr_val |= (uint32_t)prtad << AXI_ETH_LITE_MDIO_ADDRESS_REG_SHIFT_PHYADDR;

	axi_eth_lite_write_reg(config, AXI_ETH_LITE_MDIO_ADDRESS_REG_OFFSET, mdio_addr_val);
}

static inline void axi_eth_lite_bus_enable(const struct device *dev)
{
	const struct axi_eth_lite_config *config = dev->config;

	axi_eth_lite_write_reg(config, AXI_ETH_LITE_MDIO_CONTROL_REG_OFFSET,
			       AXI_ETH_LITE_MDIO_CONTROL_REG_MDIO_ENABLE_MASK);
}

/* arbitrary but sufficient in testing */
#define MDIO_MAX_WAIT_US 1000

static inline int axi_eth_lite_complete_transaction(const struct axi_eth_lite_config *config)
{
	int waited_cycles = 0;

	/* start transaction - everything set up */
	axi_eth_lite_write_reg(config, AXI_ETH_LITE_MDIO_CONTROL_REG_OFFSET,
			       AXI_ETH_LITE_MDIO_CONTROL_REG_MDIO_ENABLE_MASK |
				       AXI_ETH_LITE_MDIO_CONTROL_REG_MDIO_BUSY_MASK);

	while (axi_eth_lite_check_busy(config) && waited_cycles < MDIO_MAX_WAIT_US) {
		/* no need to block the CPU */
		k_msleep(1);
		waited_cycles++;
	}

	if (waited_cycles == MDIO_MAX_WAIT_US) {
		LOG_ERR("Timed out waiting for MDIO transaction to complete!");
		return -EIO;
	}
	/* busy went low - transaction complete */
	return 0;
}

static int axi_eth_lite_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			     uint16_t *value)
{
	const struct axi_eth_lite_config *config = dev->config;
	struct axi_eth_lite_data *data = dev->data;

	if (prtad >= AXI_ETH_LITE_MAX_PHY_DEVICES) {
		LOG_ERR("Requested read port address %" PRIu8 " not supported - max %d", prtad,
			AXI_ETH_LITE_MAX_PHY_DEVICES);
		return -ENOSYS;
	}

	(void)k_mutex_lock(&data->mutex, K_FOREVER);

	if (axi_eth_lite_check_busy(config)) {
		LOG_ERR("MDIO bus busy!");
		(void)k_mutex_unlock(&data->mutex);
		return -ENOSYS;
	}

	axi_eth_lite_set_addr(config, prtad, regad, true);

	if (axi_eth_lite_complete_transaction(config)) {
		(void)k_mutex_unlock(&data->mutex);
		return -EIO;
	}

	*value = (uint16_t)axi_eth_lite_read_reg(config, AXI_ETH_LITE_MDIO_READ_DATA_REG_OFFSET);

	(void)k_mutex_unlock(&data->mutex);

	return 0;
}

static int axi_eth_lite_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t value)
{
	const struct axi_eth_lite_config *config = dev->config;
	struct axi_eth_lite_data *data = dev->data;

	if (prtad >= AXI_ETH_LITE_MAX_PHY_DEVICES) {
		LOG_ERR("Requested write port address %" PRIu8 " not supported - max %d", prtad,
			AXI_ETH_LITE_MAX_PHY_DEVICES);
		return -ENOSYS;
	}

	(void)k_mutex_lock(&data->mutex, K_FOREVER);

	if (axi_eth_lite_check_busy(config)) {
		LOG_ERR("MDIO bus busy!");
		(void)k_mutex_unlock(&data->mutex);
		return -ENOSYS;
	}
	axi_eth_lite_set_addr(config, prtad, regad, false);
	axi_eth_lite_write_reg(config, AXI_ETH_LITE_MDIO_WRITE_DATA_REG_OFFSET, value);

	if (axi_eth_lite_complete_transaction(config)) {
		(void)k_mutex_unlock(&data->mutex);
		return -EIO;
	}

	(void)k_mutex_unlock(&data->mutex);

	return 0;
}

static int axi_eth_lite_init(const struct device *dev)
{
	struct axi_eth_lite_data *data = dev->data;

	(void)k_mutex_init(&data->mutex);

	axi_eth_lite_bus_enable(dev);

	return 0;
}

static DEVICE_API(mdio, axi_eth_lite_api) = {.read = axi_eth_lite_read,
					     .write = axi_eth_lite_write};

#define XILINX_AXI_ETHERNET_LITE_MDIO_INIT(inst)                                                   \
                                                                                                   \
	static const struct axi_eth_lite_config axi_eth_lite_config##inst = {                      \
		.reg = (void *)(uintptr_t)DT_REG_ADDR(DT_INST_PARENT(inst)),                       \
	};                                                                                         \
	static struct axi_eth_lite_data axi_eth_lite_data##inst = {0};                             \
	DEVICE_DT_INST_DEFINE(inst, axi_eth_lite_init, NULL, &axi_eth_lite_data##inst,             \
			      &axi_eth_lite_config##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,  \
			      &axi_eth_lite_api);

#define DT_DRV_COMPAT xlnx_xps_ethernetlite_3_00_a_mdio
DT_INST_FOREACH_STATUS_OKAY(XILINX_AXI_ETHERNET_LITE_MDIO_INIT)
