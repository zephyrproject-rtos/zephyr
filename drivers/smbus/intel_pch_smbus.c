/*
 * Copyright (c) 2022 Intel Corporation
 *
 * Intel I/O Controller Hub (ICH) later renamed to Intel Platform Controller
 * Hub (PCH) SMbus driver.
 *
 * PCH provides SMBus 2.0 - compliant Host Controller.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/smbus.h>
#include <zephyr/drivers/pcie/pcie.h>

#define DT_DRV_COMPAT intel_pch_smbus

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intel_pch, CONFIG_SMBUS_LOG_LEVEL);

#include "intel_pch_smbus.h"

/**
 * @note Following notions are used:
 *  * periph_addr - Peripheral address (Slave address mentioned in the Specs)
 *  * command - First byte to send in the SMBus protocol operations except for
 *              Quick and Byte Read. Also known as register.
 */

/**
 * Intel PCH configuration acquired from DTS during device initialization
 */
struct pch_config {
	/* IRQ configuration function */
	void (*config_func)(const struct device *dev);
	/* PCIE BDF got from DTS */
	pcie_bdf_t pcie_bdf;
	/* PCIE ID got from DTS */
	pcie_id_t pcie_id;
};

/**
 * Intel PCH internal driver data
 */
struct pch_data {
	DEVICE_MMIO_RAM;
	io_port_t sba;
	uint32_t config;
	uint8_t status;

	struct k_mutex mutex;
	struct k_sem completion_sync;
};

/**
 * Helpers for accessing Intel PCH SMBus registers. Depending on
 * configuration option MMIO or IO method will be used.
 */
#if defined(CONFIG_SMBUS_INTEL_PCH_ACCESS_MMIO)
static uint8_t pch_reg_read(const struct device *dev, uint8_t reg)
{
	return sys_read8(DEVICE_MMIO_GET(dev) + reg);
}

static void pch_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	sys_write8(val, DEVICE_MMIO_GET(dev) + reg);
}
#elif defined(CONFIG_SMBUS_INTEL_PCH_ACCESS_IO)
static uint8_t pch_reg_read(const struct device *dev, uint8_t reg)
{
	struct pch_data *data = dev->data;

	return sys_in8(data->sba + reg);
}

static void pch_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	struct pch_data *data = dev->data;

	sys_out8(val, data->sba + reg);
}
#else
#error Wrong PCH Register Access Mode
#endif

static int pch_configure(const struct device *dev, uint32_t config)
{
	struct pch_data *data = dev->data;

	LOG_DBG("dev %p config 0x%x", dev, config);

	/* Keep config for a moment */
	data->config = config;

	return 0;
}

static int pch_get_config(const struct device *dev, uint32_t *config)
{
	struct pch_data *data = dev->data;

	*config = data->config;

	return 0;
}

/* Device initialization function */
static int pch_smbus_init(const struct device *dev)
{
	const struct pch_config * const config = dev->config;
	struct pch_data *data = dev->data;
	struct pcie_bar mbar;
	uint32_t val;

	if (!pcie_probe(config->pcie_bdf, config->pcie_id)) {
		LOG_ERR("Cannot probe PCI device");
		return -ENODEV;
	}

	val = pcie_conf_read(config->pcie_bdf, PCIE_CONF_CMDSTAT);
	if (val & PCIE_CONF_CMDSTAT_INTERRUPT) {
		LOG_WRN("Pending interrupt, continuing");
	}

	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_ACCESS_MMIO)) {
		pcie_probe_mbar(config->pcie_bdf, 0, &mbar);
		pcie_set_cmd(config->pcie_bdf, PCIE_CONF_CMDSTAT_MEM, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size,
			   K_MEM_CACHE_NONE);

		LOG_DBG("Mapped 0x%lx size 0x%lx to 0x%lx",
			mbar.phys_addr, mbar.size, DEVICE_MMIO_GET(dev));
	} else {
		pcie_set_cmd(config->pcie_bdf, PCIE_CONF_CMDSTAT_IO, true);
		val = pcie_conf_read(config->pcie_bdf, PCIE_CONF_BAR4);
		if (!PCIE_CONF_BAR_IO(val)) {
			LOG_ERR("Cannot read IO BAR");
			return -EINVAL;
		}

		data->sba = PCIE_CONF_BAR_ADDR(val);

		LOG_DBG("Using I/O address 0x%x", data->sba);
	}

	val = pcie_conf_read(config->pcie_bdf, PCH_SMBUS_HCFG);
	if ((val & PCH_SMBUS_HCFG_HST_EN) == 0) {
		LOG_ERR("SMBus Host Controller is disabled");
		return -EINVAL;
	}

	/* Initialize mutex and semaphore */
	k_mutex_init(&data->mutex);
	k_sem_init(&data->completion_sync, 0, 1);

	config->config_func(dev);

	if (pch_configure(dev, SMBUS_MODE_CONTROLLER)) {
		LOG_ERR("SMBus: Cannot set default configuration");
		return -EIO;
	}

	return 0;
}

static int pch_prepare_transfer(const struct device *dev)
{
	uint8_t hsts;

	hsts = pch_reg_read(dev, PCH_SMBUS_HSTS);

	pch_dump_register_hsts(hsts);

	if (hsts & PCH_SMBUS_HSTS_HOST_BUSY) {
		LOG_ERR("Return BUSY status");
		return -EBUSY;
	}

	/* Check and clear HSTS status bits */
	hsts &= PCH_SMBUS_HSTS_ERROR | PCH_SMBUS_HSTS_BYTE_DONE |
		PCH_SMBUS_HSTS_INTERRUPT;
	if (hsts) {
		pch_reg_write(dev, PCH_SMBUS_HSTS, hsts);
	}

	/* TODO: Clear also CRC check bits */

	return 0;
}

static int pch_check_status(const struct device *dev)
{
	struct pch_data *data = dev->data;
	uint8_t status = data->status;

	/**
	 * Device Error means following:
	 * - unsupported Command Field Unclaimed Cycle
	 * - Host Device timeout
	 * - CRC Error
	 */
	if (status & PCH_SMBUS_HSTS_DEV_ERROR) {
		uint8_t auxs = pch_reg_read(dev, PCH_SMBUS_AUXS);

		LOG_WRN("Device Error (DERR) received");

		if (auxs & PCH_SMBUS_AUXS_CRC_ERROR) {
			LOG_DBG("AUXS register 0x%02x", auxs);
			/* Clear CRC error status bit */
			pch_reg_write(dev, PCH_SMBUS_AUXS,
				      PCH_SMBUS_AUXS_CRC_ERROR);
		}

		return -EIO;
	}

	/**
	 * Transaction collision, several masters are trying to access
	 * the bus and PCH detects arbitration lost.
	 */
	if (status & PCH_SMBUS_HSTS_BUS_ERROR) {
		LOG_WRN("Bus Error (BERR) received");

		return -EAGAIN;
	}

	/**
	 * Source of interrupt is failed bus transaction. This is set in
	 * response to KILL set to terminate the host transaction
	 */
	if (status & PCH_SMBUS_HSTS_FAILED) {
		LOG_WRN("Failed (FAIL) received");

		return -EIO;
	}

	return 0;
}

static int pch_smbus_block_start(const struct device *dev, uint16_t periph_addr,
				 uint8_t rw, uint8_t command, uint8_t count,
				 uint8_t *buf, uint8_t protocol)
{
	uint8_t reg;
	int ret;

	LOG_DBG("addr %x rw %d command %x", periph_addr, rw, command);

	/* Set TSA register */
	reg = PCH_SMBUS_TSA_ADDR_SET(periph_addr);
	reg |= rw & SMBUS_MSG_RW_MASK;
	pch_reg_write(dev, PCH_SMBUS_TSA, reg);

	/* Set HCMD register */
	pch_reg_write(dev, PCH_SMBUS_HCMD, command);

	/* Enable 32-byte buffer mode (E32b) to send block of data */
	reg = pch_reg_read(dev, PCH_SMBUS_AUXC);
	reg |= PCH_SMBUS_AUXC_EN_32BUF;
	pch_reg_write(dev, PCH_SMBUS_AUXC, reg);

	/* In E32B mode read and write to PCH_SMBUS_HBD translates to
	 * read and write to 32 byte storage array, index needs to be
	 * cleared by reading HCTL
	 */
	reg = pch_reg_read(dev, PCH_SMBUS_HCTL);
	ARG_UNUSED(reg); /* Avoid 'Dead assignment' warning */

	if (rw == SMBUS_MSG_WRITE) {
		/* Write count */
		pch_reg_write(dev, PCH_SMBUS_HD0, count);

		/* Write data to send */
		for (int i = 0; i < count; i++) {
			pch_reg_write(dev, PCH_SMBUS_HBD, buf[i]);
		}
	}

	ret = pch_prepare_transfer(dev);
	if (ret < 0) {
		return ret;
	}

	/* Set HCTL register */
	reg = PCH_SMBUS_HCTL_CMD_SET(protocol);
	reg |= PCH_SMBUS_HCTL_START;
	reg |= PCH_SMBUS_HCTL_INTR_EN;
	pch_reg_write(dev, PCH_SMBUS_HCTL, reg);

	return 0;
}

/* Start PCH SMBus operation */
static int pch_smbus_start(const struct device *dev, uint16_t periph_addr,
			   enum smbus_direction rw, uint8_t command,
			   uint8_t *buf, uint8_t protocol)
{
	uint8_t reg;
	int ret;

	LOG_DBG("addr %x rw %d command %x", periph_addr, rw, command);

	/* Set TSA register */
	reg = PCH_SMBUS_TSA_ADDR_SET(periph_addr);
	reg |= rw & SMBUS_MSG_RW_MASK;
	pch_reg_write(dev, PCH_SMBUS_TSA, reg);

	/* Write command for every but QUICK op */
	if (protocol != SMBUS_CMD_QUICK) {
		/* Set HCMD register */
		pch_reg_write(dev, PCH_SMBUS_HCMD, command);

		/* Set Host Data 0 (HD0) register */
		if (rw == SMBUS_MSG_WRITE && protocol != SMBUS_CMD_BYTE) {
			pch_reg_write(dev, PCH_SMBUS_HD0, buf[0]);

			/* If we need to write second byte */
			if (protocol == SMBUS_CMD_WORD_DATA ||
			    protocol == SMBUS_CMD_PROC_CALL) {
				pch_reg_write(dev, PCH_SMBUS_HD1, buf[1]);
			}
		}
	}

	ret = pch_prepare_transfer(dev);
	if (ret < 0) {
		return ret;
	}

	/* Set HCTL register */
	reg = PCH_SMBUS_HCTL_CMD_SET(protocol);
	reg |= PCH_SMBUS_HCTL_START;
	reg |= PCH_SMBUS_HCTL_INTR_EN;
	pch_reg_write(dev, PCH_SMBUS_HCTL, reg);

	return 0;
}

/* Implementation of PCH SMBus API */

/* Implementation of SMBus Quick */
static int pch_smbus_quick(const struct device *dev, uint16_t periph_addr,
			   enum smbus_direction rw)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x direction %x", dev, periph_addr, rw);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, rw, 0, NULL, SMBUS_CMD_QUICK);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Quick timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Byte Write */
static int pch_smbus_byte_write(const struct device *dev, uint16_t periph_addr,
				uint8_t command)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x", dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, SMBUS_MSG_WRITE, command, NULL,
			      SMBUS_CMD_BYTE);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Byte Write timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Byte Read */
static int pch_smbus_byte_read(const struct device *dev, uint16_t periph_addr,
			       uint8_t *byte)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x", dev, periph_addr);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, SMBUS_MSG_READ, 0, NULL,
			      SMBUS_CMD_BYTE);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Byte Read timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);
	if (ret < 0) {
		goto unlock;
	}

	*byte = pch_reg_read(dev, PCH_SMBUS_HD0);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Byte Data Write */
static int pch_smbus_byte_data_write(const struct device *dev,
				     uint16_t periph_addr,
				     uint8_t command, uint8_t byte)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x", dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, SMBUS_MSG_WRITE, command,
			      &byte, SMBUS_CMD_BYTE_DATA);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Byte Data Write timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Byte Data Read */
static int pch_smbus_byte_data_read(const struct device *dev,
				    uint16_t periph_addr,
				    uint8_t command, uint8_t *byte)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x", dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, SMBUS_MSG_READ, command,
			      NULL, SMBUS_CMD_BYTE_DATA);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Byte Data Read timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);
	if (ret < 0) {
		goto unlock;
	}

	*byte = pch_reg_read(dev, PCH_SMBUS_HD0);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Word Data Write */
static int pch_smbus_word_data_write(const struct device *dev,
				     uint16_t periph_addr,
				     uint8_t command, uint16_t word)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x", dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, SMBUS_MSG_WRITE, command,
			      (uint8_t *)&word, SMBUS_CMD_WORD_DATA);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Word Data Write timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Word Data Read */
static int pch_smbus_word_data_read(const struct device *dev,
				    uint16_t periph_addr,
				    uint8_t command, uint16_t *word)
{
	struct pch_data *data = dev->data;
	uint8_t *p = (uint8_t *)word;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x", dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, SMBUS_MSG_READ, command,
			      NULL, SMBUS_CMD_WORD_DATA);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Word Data Read timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);
	if (ret < 0) {
		goto unlock;
	}

	p[0] = pch_reg_read(dev, PCH_SMBUS_HD0);
	p[1] = pch_reg_read(dev, PCH_SMBUS_HD1);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Process Call */
static int pch_smbus_pcall(const struct device *dev,
			   uint16_t periph_addr, uint8_t command,
			   uint16_t send_word, uint16_t *recv_word)
{
	struct pch_data *data = dev->data;
	uint8_t *p = (uint8_t *)recv_word;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x", dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_start(dev, periph_addr, SMBUS_MSG_WRITE, command,
			      (uint8_t *)&send_word, SMBUS_CMD_PROC_CALL);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Proc Call timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);
	if (ret < 0) {
		goto unlock;
	}

	p[0] = pch_reg_read(dev, PCH_SMBUS_HD0);
	p[1] = pch_reg_read(dev, PCH_SMBUS_HD1);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Block Write */
static int pch_smbus_block_write(const struct device *dev, uint16_t periph_addr,
				 uint8_t command, uint8_t count, uint8_t *buf)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x count %u",
		dev, periph_addr, command, count);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_block_start(dev, periph_addr, SMBUS_MSG_WRITE, command,
				    count, buf, SMBUS_CMD_BLOCK);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Block Write timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Block Read */
static int pch_smbus_block_read(const struct device *dev, uint16_t periph_addr,
				uint8_t command, uint8_t *count, uint8_t *buf)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x",
		dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_block_start(dev, periph_addr, SMBUS_MSG_READ, command,
				    0, buf, SMBUS_CMD_BLOCK);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Block Read timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);
	if (ret < 0) {
		goto unlock;
	}

	*count = pch_reg_read(dev, PCH_SMBUS_HD0);
	if (*count == 0 || *count > SMBUS_BLOCK_BYTES_MAX) {
		ret = -ENODATA;
		goto unlock;
	}

	for (int i = 0; i < *count; i++) {
		buf[i] = pch_reg_read(dev, PCH_SMBUS_HBD);
	}

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

/* Implementation of SMBus Block Process Call */
static int pch_smbus_block_pcall(const struct device *dev,
				 uint16_t periph_addr, uint8_t command,
				 uint8_t send_count, uint8_t *send_buf,
				 uint8_t *recv_count, uint8_t *recv_buf)
{
	struct pch_data *data = dev->data;
	int ret;

	LOG_DBG("dev %p addr 0x%02x command 0x%02x",
		dev, periph_addr, command);

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = pch_smbus_block_start(dev, periph_addr, SMBUS_MSG_WRITE, command,
				    send_count, send_buf, SMBUS_CMD_BLOCK_PROC);
	if (ret < 0) {
		goto unlock;
	}

	/* Wait for completion from ISR */
	ret = k_sem_take(&data->completion_sync, K_MSEC(30));
	if (ret != 0) {
		LOG_ERR("SMBus Block Process Call timed out");
		ret = -ETIMEDOUT;
		goto unlock;
	}

	ret = pch_check_status(dev);
	if (ret < 0) {
		goto unlock;
	}

	*recv_count = pch_reg_read(dev, PCH_SMBUS_HD0);
	if (*recv_count == 0 ||
	    *recv_count + send_count > SMBUS_BLOCK_BYTES_MAX) {
		ret = -ENODATA;
		goto unlock;
	}

	for (int i = 0; i < *recv_count; i++) {
		recv_buf[i] = pch_reg_read(dev, PCH_SMBUS_HBD);
	}

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static const struct smbus_driver_api funcs = {
	.configure = pch_configure,
	.get_config = pch_get_config,
	.smbus_quick = pch_smbus_quick,
	.smbus_byte_write = pch_smbus_byte_write,
	.smbus_byte_read = pch_smbus_byte_read,
	.smbus_byte_data_write = pch_smbus_byte_data_write,
	.smbus_byte_data_read = pch_smbus_byte_data_read,
	.smbus_word_data_write = pch_smbus_word_data_write,
	.smbus_word_data_read = pch_smbus_word_data_read,
	.smbus_pcall = pch_smbus_pcall,
	.smbus_block_write = pch_smbus_block_write,
	.smbus_block_read = pch_smbus_block_read,
	.smbus_block_pcall = pch_smbus_block_pcall,
};

static void smbus_isr(const struct device *dev)
{
	const struct pch_config * const config = dev->config;
	struct pch_data *data = dev->data;
	uint32_t sts;
	uint8_t status;

	sts = pcie_conf_read(config->pcie_bdf, PCIE_CONF_CMDSTAT);
	if (!(sts & PCIE_CONF_CMDSTAT_INTERRUPT)) {
		LOG_ERR("Not our interrupt");
		return;
	}

	status = pch_reg_read(dev, PCH_SMBUS_HSTS);

	/* HSTS dump if logging is enabled */
	pch_dump_register_hsts(status);

	if (status & PCH_SMBUS_HSTS_BYTE_DONE) {
		LOG_WRN("BYTE_DONE interrupt is not used");
	}

	/* Clear IRQ sources */
	pch_reg_write(dev, PCH_SMBUS_HSTS, status);

	data->status = status;

	k_sem_give(&data->completion_sync);
}

/* Device macro initialization  / DTS hackery */

/* PCI BDF is hacked into REG_ADDR */
#define DT_INST_PCIE_BDF(n) DT_INST_REG_ADDR(n)

/* PCI ID is hacked into REG_SIZE */
#define DT_INST_PCIE_ID(n) DT_INST_REG_SIZE(n)

#define SMBUS_PCH_IRQ_FLAGS_SENSE0(n) 0
#define SMBUS_PCH_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define SMBUS_PCH_IRQ_FLAGS(n) \
	_CONCAT(SMBUS_PCH_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

#define SMBUS_IRQ_CONFIG(n)                                                    \
	BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),                    \
		     "SMBus PCIe requires dynamic interrupts");                \
	static void pch_config_##n(const struct device *dev)                   \
	{                                                                      \
		ARG_UNUSED(dev);                                               \
		unsigned int irq;                                              \
		if (DT_INST_IRQN(n) == PCIE_IRQ_DETECT) {                      \
			irq = pcie_alloc_irq(DT_INST_PCIE_BDF(n));             \
			if (irq == PCIE_CONF_INTR_IRQ_NONE) {                  \
				return;                                        \
			}                                                      \
		} else {                                                       \
			irq = DT_INST_IRQN(n);                                 \
			pcie_conf_write(DT_INST_PCIE_BDF(n),                   \
					PCIE_CONF_INTR, irq);                  \
		}                                                              \
		pcie_connect_dynamic_irq(DT_INST_PCIE_BDF(n), irq,             \
					 DT_INST_IRQ(n, priority),             \
					 (void (*)(const void *))smbus_isr,    \
					 DEVICE_DT_INST_GET(n),                \
					 SMBUS_PCH_IRQ_FLAGS(n));              \
		pcie_irq_enable(DT_INST_PCIE_BDF(n), irq);                     \
		LOG_DBG("Configure irq %d", irq);                              \
	}

#define SMBUS_DEVICE_INIT(n)                                                   \
	static void pch_config_##n(const struct device *dev);                  \
	static const struct pch_config pch_config_data_##n = {                 \
		.config_func = pch_config_##n,                                 \
		.pcie_bdf = DT_INST_PCIE_BDF(n),                               \
		.pcie_id = DT_INST_PCIE_ID(n),                                 \
	};                                                                     \
	static struct pch_data smbus_##n##_data;                               \
	SMBUS_DEVICE_DT_INST_DEFINE(n, pch_smbus_init, NULL,                   \
				    &smbus_##n##_data, &pch_config_data_##n,   \
				    POST_KERNEL, CONFIG_SMBUS_INIT_PRIORITY,   \
				    &funcs);                                   \
	SMBUS_IRQ_CONFIG(n);

DT_INST_FOREACH_STATUS_OKAY(SMBUS_DEVICE_INIT)
