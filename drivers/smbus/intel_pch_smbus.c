/*
 * Copyright (c) 2022 Intel Corporation
 *
 * Intel I/O Controller Hub (ICH) later renamed to Intel Platform Controller
 * Hub (PCH) SMBus driver.
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

#include "smbus_utils.h"
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
	struct pcie_dev *pcie;
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
	const struct device *dev;

#if defined(CONFIG_SMBUS_INTEL_PCH_SMBALERT)
	/* smbalert callback list */
	sys_slist_t smbalert_cbs;
	/* smbalert work */
	struct k_work smb_alert_work;
#endif /* CONFIG_SMBUS_INTEL_PCH_SMBALERT */

#if defined(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)
	/* Host Notify callback list */
	sys_slist_t host_notify_cbs;
	/* Host Notify work */
	struct k_work host_notify_work;
	/* Host Notify peripheral device address */
	uint8_t notify_addr;
	/* Host Notify data received */
	uint16_t notify_data;
#endif /* CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY */
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

#if defined(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)
static void host_notify_work(struct k_work *work)
{
	struct pch_data *data = CONTAINER_OF(work, struct pch_data,
					     host_notify_work);
	const struct device *dev = data->dev;
	uint8_t addr = data->notify_addr;

	smbus_fire_callbacks(&data->host_notify_cbs, dev, addr);
}

static int pch_smbus_host_notify_set_cb(const struct device *dev,
					struct smbus_callback *cb)
{
	struct pch_data *data = dev->data;

	LOG_DBG("dev %p cb %p", dev, cb);

	return smbus_callback_set(&data->host_notify_cbs, cb);
}

static int pch_smbus_host_notify_remove_cb(const struct device *dev,
					   struct smbus_callback *cb)
{
	struct pch_data *data = dev->data;

	LOG_DBG("dev %p cb %p", dev, cb);

	return smbus_callback_remove(&data->host_notify_cbs, cb);
}
#endif /* CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY */

#if defined(CONFIG_SMBUS_INTEL_PCH_SMBALERT)
static void smbalert_work(struct k_work *work)
{
	struct pch_data *data = CONTAINER_OF(work, struct pch_data,
					     smb_alert_work);
	const struct device *dev = data->dev;

	smbus_loop_alert_devices(dev, &data->smbalert_cbs);
}

static int pch_smbus_smbalert_set_sb(const struct device *dev,
				     struct smbus_callback *cb)
{
	struct pch_data *data = dev->data;

	LOG_DBG("dev %p cb %p", dev, cb);

	return smbus_callback_set(&data->smbalert_cbs, cb);
}

static int pch_smbus_smbalert_remove_sb(const struct device *dev,
					struct smbus_callback *cb)
{
	struct pch_data *data = dev->data;

	LOG_DBG("dev %p cb %p", dev, cb);

	return smbus_callback_remove(&data->smbalert_cbs, cb);
}
#endif /* CONFIG_SMBUS_INTEL_PCH_SMBALERT */

static int pch_configure(const struct device *dev, uint32_t config)
{
	struct pch_data *data = dev->data;

	LOG_DBG("dev %p config 0x%x", dev, config);

	if (config & SMBUS_MODE_HOST_NOTIFY) {
		uint8_t status;

		if (!IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)) {
			LOG_ERR("Error configuring Host Notify");
			return -EINVAL;
		}

		/* Enable Host Notify interrupts */
		status = pch_reg_read(dev, PCH_SMBUS_SCMD);
		status |= PCH_SMBUS_SCMD_HNI_EN;
		pch_reg_write(dev, PCH_SMBUS_SCMD, status);
	}

	if (config & SMBUS_MODE_SMBALERT) {
		uint8_t status;

		if (!IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_SMBALERT)) {
			LOG_ERR("Error configuring SMBALERT");
			return -EINVAL;
		}

		/* Disable SMBALERT_DIS */
		status = pch_reg_read(dev, PCH_SMBUS_SCMD);
		status &= ~PCH_SMBUS_SCMD_SMBALERT_DIS;
		pch_reg_write(dev, PCH_SMBUS_SCMD, status);
	}

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

	if (config->pcie->bdf == PCIE_BDF_NONE) {
		LOG_ERR("Cannot probe PCI device");
		return -ENODEV;
	}

	val = pcie_conf_read(config->pcie->bdf, PCIE_CONF_CMDSTAT);
	if (val & PCIE_CONF_CMDSTAT_INTERRUPT) {
		LOG_WRN("Pending interrupt, continuing");
	}

	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_ACCESS_MMIO)) {
		pcie_probe_mbar(config->pcie->bdf, 0, &mbar);
		pcie_set_cmd(config->pcie->bdf, PCIE_CONF_CMDSTAT_MEM, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size,
			   K_MEM_CACHE_NONE);

		LOG_DBG("Mapped 0x%lx size 0x%lx to 0x%lx",
			mbar.phys_addr, mbar.size, DEVICE_MMIO_GET(dev));
	} else {
		pcie_set_cmd(config->pcie->bdf, PCIE_CONF_CMDSTAT_IO, true);
		val = pcie_conf_read(config->pcie->bdf, PCIE_CONF_BAR4);
		if (!PCIE_CONF_BAR_IO(val)) {
			LOG_ERR("Cannot read IO BAR");
			return -EINVAL;
		}

		data->sba = PCIE_CONF_BAR_ADDR(val);

		LOG_DBG("Using I/O address 0x%x", data->sba);
	}

	val = pcie_conf_read(config->pcie->bdf, PCH_SMBUS_HCFG);
	if ((val & PCH_SMBUS_HCFG_HST_EN) == 0) {
		LOG_ERR("SMBus Host Controller is disabled");
		return -EINVAL;
	}

	/* Initialize mutex and semaphore */
	k_mutex_init(&data->mutex);
	k_sem_init(&data->completion_sync, 0, 1);

	data->dev = dev;

	/* Initialize work structures */

#if defined(CONFIG_SMBUS_INTEL_PCH_SMBALERT)
	k_work_init(&data->smb_alert_work, smbalert_work);
#endif /* CONFIG_SMBUS_INTEL_PCH_SMBALERT */

#if defined(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)
	k_work_init(&data->host_notify_work, host_notify_work);
#endif /* CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY */

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

	LOG_DBG("addr 0x%02x rw %d command %x", periph_addr, rw, command);

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
#if defined(CONFIG_SMBUS_INTEL_PCH_SMBALERT)
	.smbus_smbalert_set_cb = pch_smbus_smbalert_set_sb,
	.smbus_smbalert_remove_cb = pch_smbus_smbalert_remove_sb,
#endif /* CONFIG_SMBUS_INTEL_PCH_SMBALERT */
#if defined(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)
	.smbus_host_notify_set_cb = pch_smbus_host_notify_set_cb,
	.smbus_host_notify_remove_cb = pch_smbus_host_notify_remove_cb,
#endif /* CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY */
};

static void smbus_isr(const struct device *dev)
{
	const struct pch_config * const config = dev->config;
	struct pch_data *data = dev->data;
	uint32_t sts;
	uint8_t status;

	sts = pcie_conf_read(config->pcie->bdf, PCIE_CONF_CMDSTAT);
	if (!(sts & PCIE_CONF_CMDSTAT_INTERRUPT)) {
		LOG_ERR("Not our interrupt");
		return;
	}

	/**
	 * Handle first Host Notify since for that we need to read SSTS
	 * register and for all other sources HSTS.
	 *
	 * Intel PCH implements Host Notify protocol in hardware.
	 */
#if defined(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)
	if (data->config & SMBUS_MODE_HOST_NOTIFY) {
		status = pch_reg_read(dev, PCH_SMBUS_SSTS);
		if (status & PCH_SMBUS_SSTS_HNS) {
			/* Notify address */
			data->notify_addr =
				pch_reg_read(dev, PCH_SMBUS_NDA) >> 1;

			/* Notify data */
			data->notify_data = pch_reg_read(dev, PCH_SMBUS_NDLB);
			data->notify_data |=
				pch_reg_read(dev, PCH_SMBUS_NDHB) << 8;

			k_work_submit(&data->host_notify_work);

			/* Clear Host Notify */
			pch_reg_write(dev, PCH_SMBUS_SSTS, PCH_SMBUS_SSTS_HNS);

			return;
		}
	}
#endif /* CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY */

	status = pch_reg_read(dev, PCH_SMBUS_HSTS);

	/* HSTS dump if logging is enabled */
	pch_dump_register_hsts(status);

	if (status & PCH_SMBUS_HSTS_BYTE_DONE) {
		LOG_WRN("BYTE_DONE interrupt is not used");
	}

	/* Handle SMBALERT# signal */
#if defined(CONFIG_SMBUS_INTEL_PCH_SMBALERT)
	if (data->config & SMBUS_MODE_SMBALERT &&
	    status & PCH_SMBUS_HSTS_SMB_ALERT) {
		k_work_submit(&data->smb_alert_work);
	}
#endif /* CONFIG_SMBUS_INTEL_PCH_SMBALERT */

	/* Clear IRQ sources */
	pch_reg_write(dev, PCH_SMBUS_HSTS, status);

	data->status = status;

	k_sem_give(&data->completion_sync);
}

/* Device macro initialization  / DTS hackery */

#define SMBUS_PCH_IRQ_FLAGS(n)                                                 \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, sense),                            \
		    (DT_INST_IRQ(n, sense)),                                   \
		    (0))

#define SMBUS_IRQ_CONFIG(n)                                                    \
	BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),                    \
		     "SMBus PCIe requires dynamic interrupts");                \
	static void pch_config_##n(const struct device *dev)                   \
	{                                                                      \
		const struct pch_config * const config = dev->config;          \
		unsigned int irq;                                              \
		if (DT_INST_IRQN(n) == PCIE_IRQ_DETECT) {                      \
			irq = pcie_alloc_irq(config->pcie->bdf);               \
			if (irq == PCIE_CONF_INTR_IRQ_NONE) {                  \
				return;                                        \
			}                                                      \
		} else {                                                       \
			irq = DT_INST_IRQN(n);                                 \
			pcie_conf_write(config->pcie->bdf,                     \
					PCIE_CONF_INTR, irq);                  \
		}                                                              \
		pcie_connect_dynamic_irq(config->pcie->bdf, irq,               \
					 DT_INST_IRQ(n, priority),             \
					 (void (*)(const void *))smbus_isr,    \
					 DEVICE_DT_INST_GET(n),                \
					 SMBUS_PCH_IRQ_FLAGS(n));              \
		pcie_irq_enable(config->pcie->bdf, irq);                       \
		LOG_DBG("Configure irq %d", irq);                              \
	}

#define SMBUS_DEVICE_INIT(n)                                                   \
	DEVICE_PCIE_INST_DECLARE(n);                                           \
	static void pch_config_##n(const struct device *dev);                  \
	static const struct pch_config pch_config_data_##n = {                 \
		DEVICE_PCIE_INST_INIT(n, pcie),                                \
		.config_func = pch_config_##n,                                 \
	};                                                                     \
	static struct pch_data smbus_##n##_data;                               \
	SMBUS_DEVICE_DT_INST_DEFINE(n, pch_smbus_init, NULL,                   \
				    &smbus_##n##_data, &pch_config_data_##n,   \
				    POST_KERNEL, CONFIG_SMBUS_INIT_PRIORITY,   \
				    &funcs);                                   \
	SMBUS_IRQ_CONFIG(n);

DT_INST_FOREACH_STATUS_OKAY(SMBUS_DEVICE_INIT)
