/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_eeprom

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(eeprom_xec, CONFIG_EEPROM_LOG_LEVEL);

/* EEPROM Mode Register */
#define XEC_EEPROM_MODE_ACTIVATE		BIT(0)

/* EEPROM Status Register */
#define XEC_EEPROM_STS_TRANSFER_COMPL		BIT(0)

/* EEPROM Execute Register - Transfer size bit position */
#define XEC_EEPROM_EXC_TRANSFER_SZ_BITPOS	(24)

/* EEPROM Execute Register - Commands */
#define XEC_EEPROM_EXC_CMD_READ			0x00000U
#define XEC_EEPROM_EXC_CMD_WRITE		0x10000U
#define XEC_EEPROM_EXC_CMD_READ_STS		0x20000U
#define XEC_EEPROM_EXC_CMD_WRITE_STS		0x30000U

/* EEPROM Execute Register - Address mask */
#define XEC_EEPROM_EXC_ADDR_MASK		0x7FFU

/* EEPROM Status Byte */
#define XEC_EEPROM_STS_BYTE_WIP			BIT(0)
#define XEC_EEPROM_STS_BYTE_WENB		BIT(1)

/* EEPROM Read/Write Transfer Size */
#define XEC_EEPROM_PAGE_SIZE			32U
#define XEC_EEPROM_TRANSFER_SIZE_READ		XEC_EEPROM_PAGE_SIZE
#define XEC_EEPROM_TRANSFER_SIZE_WRITE		XEC_EEPROM_PAGE_SIZE

#define XEC_EEPROM_DELAY_US			500U
#define XEC_EEPROM_DELAY_BUSY_POLL_US		50U
#define XEC_EEPROM_XFER_COMPL_RETRY_COUNT	10U

struct eeprom_xec_regs {
	uint32_t mode;
	uint32_t execute;
	uint32_t status;
	uint32_t intr_enable;
	uint32_t password;
	uint32_t unlock;
	uint32_t lock;
	uint32_t _reserved;
	uint8_t buffer[XEC_EEPROM_PAGE_SIZE];
};

struct eeprom_xec_config {
	struct eeprom_xec_regs * const regs;
	size_t size;
	const struct pinctrl_dev_config *pcfg;
};

struct eeprom_xec_data {
	struct k_mutex lock_mtx;
};

static void eeprom_xec_execute_reg_set(struct eeprom_xec_regs * const regs,
						uint32_t transfer_size, uint32_t command,
						uint16_t eeprom_addr)
{
	uint32_t temp = command + (eeprom_addr & XEC_EEPROM_EXC_ADDR_MASK);

	if (transfer_size != XEC_EEPROM_PAGE_SIZE) {
		temp += (transfer_size << XEC_EEPROM_EXC_TRANSFER_SZ_BITPOS);
	}
	regs->execute = temp;
}

static uint8_t eeprom_xec_data_buffer_read(struct eeprom_xec_regs * const regs,
					uint8_t transfer_size, uint8_t *destination_ptr)
{
	uint8_t count;

	if (transfer_size > XEC_EEPROM_PAGE_SIZE) {
		transfer_size = XEC_EEPROM_PAGE_SIZE;
	}
	for (count = 0; count < transfer_size; count++) {
		*destination_ptr = regs->buffer[count];
		destination_ptr++;
	}

	return transfer_size;
}

static uint8_t eeprom_xec_data_buffer_write(struct eeprom_xec_regs * const regs,
							uint8_t transfer_size, uint8_t *source_ptr)
{
	uint8_t count;

	if (transfer_size > XEC_EEPROM_PAGE_SIZE) {
		transfer_size = XEC_EEPROM_PAGE_SIZE;
	}
	for (count = 0; count < transfer_size; count++) {
		regs->buffer[count] = *source_ptr;
		source_ptr++;
	}

	return transfer_size;
}

static void eeprom_xec_wait_transfer_compl(struct eeprom_xec_regs * const regs)
{
	uint8_t sts = 0;
	uint8_t retry_count = 0;

	k_sleep(K_USEC(XEC_EEPROM_DELAY_US));

	do {
		if (retry_count >= XEC_EEPROM_XFER_COMPL_RETRY_COUNT) {
			LOG_ERR("XEC EEPROM retry count exceeded");
			break;
		}
		k_sleep(K_USEC(XEC_EEPROM_DELAY_BUSY_POLL_US));

		sts = XEC_EEPROM_STS_TRANSFER_COMPL & regs->status;
		retry_count++;

	} while (sts == 0);

	if (sts != 0) {
		/* Clear the appropriate status bits */
		regs->status = XEC_EEPROM_STS_TRANSFER_COMPL;
	}
}

static void eeprom_xec_wait_write_compl(struct eeprom_xec_regs * const regs)
{
	uint8_t sts = 0;
	uint8_t retry_count = 0;

	do {
		if (retry_count >= XEC_EEPROM_XFER_COMPL_RETRY_COUNT) {
			LOG_ERR("XEC EEPROM retry count exceeded");
			break;
		}

		regs->buffer[0] = 0;

		/* Issue the READ_STS command */
		regs->execute = XEC_EEPROM_EXC_CMD_READ_STS;

		eeprom_xec_wait_transfer_compl(regs);

		sts = regs->buffer[0] & (XEC_EEPROM_STS_BYTE_WIP |
							XEC_EEPROM_STS_BYTE_WENB);

		retry_count++;

	} while (sts != 0);
}

static void eeprom_xec_data_read_32_bytes(struct eeprom_xec_regs * const regs,
							uint8_t *buf, size_t len, k_off_t offset)
{
	/* Issue the READ command to transfer buffer to EEPROM memory */
	eeprom_xec_execute_reg_set(regs, len, XEC_EEPROM_EXC_CMD_READ, offset);

	/* Wait until the read operation has completed */
	eeprom_xec_wait_transfer_compl(regs);

	/* Read the data in to the software buffer */
	eeprom_xec_data_buffer_read(regs, len, buf);
}

static void eeprom_xec_data_write_32_bytes(struct eeprom_xec_regs * const regs,
							uint8_t *buf, size_t len, k_off_t offset)
{
	uint16_t sz;
	uint16_t rem_bytes;

	sz = offset % XEC_EEPROM_PAGE_SIZE;

	/* If EEPROM Addr is not on page boundary */
	if (sz != 0) {
		/* Check if we are crossing page boundary */
		if ((sz + len) > XEC_EEPROM_PAGE_SIZE) {
			rem_bytes = (XEC_EEPROM_PAGE_SIZE - sz);
			/* Update the EEPROM buffer */
			eeprom_xec_data_buffer_write(regs, rem_bytes, buf);

			/* Issue the WRITE command to transfer buffer to EEPROM memory */
			eeprom_xec_execute_reg_set(regs, rem_bytes,
						XEC_EEPROM_EXC_CMD_WRITE, offset);

			eeprom_xec_wait_transfer_compl(regs);

			eeprom_xec_wait_write_compl(regs);

			offset += rem_bytes;
			buf += rem_bytes;
			len = (len - rem_bytes);
		}
	}
	/* Update the EEPROM buffer */
	eeprom_xec_data_buffer_write(regs, len, buf);

	/* Issue the WRITE command to transfer buffer to EEPROM memory */
	eeprom_xec_execute_reg_set(regs, len, XEC_EEPROM_EXC_CMD_WRITE, offset);

	eeprom_xec_wait_transfer_compl(regs);

	eeprom_xec_wait_write_compl(regs);
}

static int eeprom_xec_read(const struct device *dev, k_off_t offset,
				void *buf,
				size_t len)
{
	const struct eeprom_xec_config *config = dev->config;
	struct eeprom_xec_data * const data = dev->data;
	struct eeprom_xec_regs * const regs = config->regs;
	uint8_t *data_buf = (uint8_t *)buf;
	uint32_t chunk_idx = 0;
	uint32_t chunk_size = XEC_EEPROM_TRANSFER_SIZE_READ;

	if (len == 0) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock_mtx, K_FOREVER);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* EEPROM HW READ */
	for (chunk_idx = 0; chunk_idx < len; chunk_idx += XEC_EEPROM_TRANSFER_SIZE_READ) {
		if ((len-chunk_idx) < XEC_EEPROM_TRANSFER_SIZE_READ) {
			chunk_size = (len-chunk_idx);
		}
		eeprom_xec_data_read_32_bytes(regs, &data_buf[chunk_idx],
						chunk_size, (offset+chunk_idx));
	}
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_unlock(&data->lock_mtx);

	return 0;
}

static int eeprom_xec_write(const struct device *dev, k_off_t offset,
				const void *buf, size_t len)
{
	const struct eeprom_xec_config *config = dev->config;
	struct eeprom_xec_data * const data = dev->data;
	struct eeprom_xec_regs * const regs = config->regs;
	uint8_t *data_buf = (uint8_t *)buf;
	uint32_t chunk_idx = 0;
	uint32_t chunk_size = XEC_EEPROM_TRANSFER_SIZE_WRITE;

	if (len == 0) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock_mtx, K_FOREVER);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* EEPROM HW WRITE */
	for (chunk_idx = 0; chunk_idx < len; chunk_idx += XEC_EEPROM_TRANSFER_SIZE_WRITE) {
		if ((len-chunk_idx) < XEC_EEPROM_TRANSFER_SIZE_WRITE) {
			chunk_size = (len-chunk_idx);
		}
		eeprom_xec_data_write_32_bytes(regs, &data_buf[chunk_idx],
							chunk_size, (offset+chunk_idx));
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_unlock(&data->lock_mtx);

	return 0;
}

static size_t eeprom_xec_size(const struct device *dev)
{
	const struct eeprom_xec_config *config = dev->config;

	return config->size;
}

#ifdef CONFIG_PM_DEVICE
static int eeprom_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct eeprom_xec_config *const devcfg = dev->config;
	struct eeprom_xec_regs * const regs = devcfg->regs;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			LOG_ERR("XEC EEPROM pinctrl setup failed (%d)", ret);
			return ret;
		}
		regs->mode |= XEC_EEPROM_MODE_ACTIVATE;
	break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Disable EEPROM Controller */
		regs->mode &= (~XEC_EEPROM_MODE_ACTIVATE);
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_SLEEP);
		/* pinctrl-1 does not exist. */
		if (ret == -ENOENT) {
			ret = 0;
		}
	break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int eeprom_xec_init(const struct device *dev)
{
	const struct eeprom_xec_config *config = dev->config;
	struct eeprom_xec_data * const data = dev->data;
	struct eeprom_xec_regs * const regs = config->regs;

	k_mutex_init(&data->lock_mtx);

	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("XEC EEPROM pinctrl init failed (%d)", ret);
		return ret;
	}

	regs->mode |= XEC_EEPROM_MODE_ACTIVATE;

	return 0;
}

static const struct eeprom_driver_api eeprom_xec_api = {
	.read = eeprom_xec_read,
	.write = eeprom_xec_write,
	.size = eeprom_xec_size,
};

PINCTRL_DT_INST_DEFINE(0);

static const struct eeprom_xec_config eeprom_config = {
	.regs = (struct eeprom_xec_regs * const)DT_INST_REG_ADDR(0),
	.size = DT_INST_PROP(0, size),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct eeprom_xec_data eeprom_data;

PM_DEVICE_DT_INST_DEFINE(0, eeprom_xec_pm_action);

DEVICE_DT_INST_DEFINE(0, &eeprom_xec_init, PM_DEVICE_DT_INST_GET(0), &eeprom_data,
		    &eeprom_config, POST_KERNEL,
		    CONFIG_EEPROM_INIT_PRIORITY, &eeprom_xec_api);
