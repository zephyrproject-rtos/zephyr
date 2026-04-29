/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2019-2026, NXP
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpi2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <fsl_lpi2c.h>
#if CONFIG_NXP_LP_FLEXCOMM
#include <zephyr/drivers/mfd/nxp_lp_flexcomm.h>
#endif

#include <zephyr/drivers/pinctrl.h>

#ifdef CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY
#include "i2c_bitbang.h"
#include <zephyr/drivers/gpio.h>
#endif /* CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcux_lpi2c);


#include "i2c-priv.h"
/* Wait for the duration of 12 bits to detect a NAK after a bus
 * address scan.  (10 appears sufficient, 20% safety factor.)
 */
#define SCAN_DELAY_US(baudrate) (12 * USEC_PER_SEC / baudrate)

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) \
	((const struct mcux_lpi2c_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_lpi2c_data *)(_dev)->data)

enum mcux_lpi2c_mode_state {
	MCUX_LPI2C_STATE_MASTER_IDLE = 0,
	MCUX_LPI2C_STATE_TARGET_READY,
	MCUX_LPI2C_STATE_MASTER_ACTIVE,
	MCUX_LPI2C_STATE_RECOVERING,
};

struct mcux_lpi2c_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
	uint32_t bus_idle_timeout_ns;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
#endif /* CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY */
};

struct mcux_lpi2c_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	lpi2c_master_handle_t handle;
	struct k_sem lock;
	struct k_sem device_sync_sem;
	status_t callback_status;

	uint32_t runtime_bitrate;
	enum mcux_lpi2c_mode_state mode_state;

	bool master_ready;
	bool master_xfer_in_progress;
#ifdef CONFIG_I2C_TARGET
	lpi2c_slave_handle_t target_handle;
	struct i2c_target_config *target_cfg;
	bool target_attached;
	bool target_enabled;
	bool first_tx;
	bool read_active;
	bool send_ack;
#endif
};

static int mcux_lpi2c_configure(const struct device *dev,
				uint32_t dev_config_raw)
{
	struct mcux_lpi2c_data *data = dev->data;
	uint32_t baudrate;
	int ret;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		baudrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		baudrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		baudrate = MHZ(1);
		break;
	default:
		return -EINVAL;
	}

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	/*
	 * Lazy programming of bitrate: store desired value here, apply it
	 * only when master hardware is actually initialized.
	 */
	data->runtime_bitrate = baudrate;

	k_sem_give(&data->lock);
	return 0;
}

static void mcux_lpi2c_master_transfer_callback(LPI2C_Type *base,
						lpi2c_master_handle_t *handle,
						status_t status, void *userData)
{
	struct mcux_lpi2c_data *data = userData;

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	data->callback_status = status;
	k_sem_give(&data->device_sync_sem);
}

static uint32_t mcux_lpi2c_convert_flags(int msg_flags)
{
	uint32_t flags = 0U;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kLPI2C_TransferNoStopFlag;
	}

	return flags;
}

#ifdef CONFIG_I2C_TARGET
static int mcux_lpi2c_target_hw_init(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	lpi2c_slave_config_t slave_config;
	uint32_t clock_freq;

	if (!data->target_cfg) {
		return -EINVAL;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPI2C_SlaveGetDefaultConfig(&slave_config);
	slave_config.address0 = data->target_cfg->address;
	slave_config.sclStall.enableAck = true;

	LPI2C_SlaveInit(base, &slave_config, clock_freq);

	/* Apply target-mode stall/filter settings needed for this controller. */
	base->SCFGR1 |= LPI2C_SCFGR1_RXSTALL_MASK |
			LPI2C_SCFGR1_TXDSTALL_MASK;

	base->SCFGR2 =
		(base->SCFGR2 &
		 ~(LPI2C_SCFGR2_FILTSDA_MASK |
		   LPI2C_SCFGR2_FILTSCL_MASK |
		   LPI2C_SCFGR2_CLKHOLD_MASK)) |
		LPI2C_SCFGR2_FILTSDA(2) |
		LPI2C_SCFGR2_FILTSCL(2) |
		LPI2C_SCFGR2_CLKHOLD(3);

	base->SCR |= LPI2C_SCR_FILTEN_MASK | LPI2C_SCR_SEN_MASK;

	LPI2C_SlaveClearStatusFlags(base, (uint32_t)kLPI2C_SlaveClearFlags);
	LPI2C_SlaveEnableInterrupts(base,
				    kLPI2C_SlaveTxReadyFlag |
				    kLPI2C_SlaveRxReadyFlag |
				    kLPI2C_SlaveStopDetectFlag |
				    kLPI2C_SlaveAddressValidFlag |
				    kLPI2C_SlaveTransmitAckFlag);

	data->first_tx = false;
	data->read_active = false;
	data->send_ack = true;
	data->target_enabled = true;

	return 0;
}
#endif

static int mcux_lpi2c_master_hw_init(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	lpi2c_master_config_t master_config;
	uint32_t clock_freq;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPI2C_MasterGetDefaultConfig(&master_config);
	master_config.busIdleTimeout_ns = config->bus_idle_timeout_ns;
	master_config.baudRate_Hz = data->runtime_bitrate;

	LPI2C_MasterInit(base, &master_config, clock_freq);
	LPI2C_MasterTransferCreateHandle(base, &data->handle,
					 mcux_lpi2c_master_transfer_callback,
					 data);

	data->master_ready = true;
	return 0;
}

#ifdef CONFIG_I2C_TARGET
static void mcux_lpi2c_target_hw_deinit(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	if (!data->target_enabled) {
		return;
	}

	LPI2C_SlaveDisableInterrupts(base,
				     kLPI2C_SlaveTxReadyFlag |
				     kLPI2C_SlaveRxReadyFlag |
				     kLPI2C_SlaveStopDetectFlag |
				     kLPI2C_SlaveAddressValidFlag |
				     kLPI2C_SlaveTransmitAckFlag);
	LPI2C_SlaveClearStatusFlags(base, (uint32_t)kLPI2C_SlaveClearFlags);
	LPI2C_SlaveDeinit(base);

	data->target_enabled = false;
}
#endif

static void mcux_lpi2c_master_hw_deinit(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	if (!data->master_ready) {
		return;
	}

	LPI2C_MasterClearStatusFlags(base, kLPI2C_MasterClearFlags);
	LPI2C_MasterDeinit(base);
	data->master_ready = false;
}

static int mcux_lpi2c_restore_steady_state_locked(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;

#ifdef CONFIG_I2C_TARGET
	if (data->target_attached && data->target_cfg) {
		int ret = mcux_lpi2c_target_hw_init(dev);

		if (ret) {
			data->mode_state = MCUX_LPI2C_STATE_RECOVERING;
			return ret;
		}

		data->mode_state = MCUX_LPI2C_STATE_TARGET_READY;
		return 0;
	}
#endif

	data->mode_state = MCUX_LPI2C_STATE_MASTER_IDLE;
	return 0;
}

static int mcux_lpi2c_enter_master_session_locked(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	int ret;

#ifdef CONFIG_I2C_TARGET
	if (data->target_attached && data->target_enabled) {
		mcux_lpi2c_target_hw_deinit(dev);
	}
#endif

	if (!data->master_ready) {
		ret = mcux_lpi2c_master_hw_init(dev);
		if (ret) {
			data->mode_state = MCUX_LPI2C_STATE_RECOVERING;
			return ret;
		}
	}

	data->mode_state = MCUX_LPI2C_STATE_MASTER_ACTIVE;
	return 0;
}

static int mcux_lpi2c_leave_master_session_locked(const struct device *dev)
{
	mcux_lpi2c_master_hw_deinit(dev);
	return mcux_lpi2c_restore_steady_state_locked(dev);
}

static int mcux_lpi2c_recover_locked(const struct device *dev, bool restore_target)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	int ret;

	data->mode_state = MCUX_LPI2C_STATE_RECOVERING;

	if (data->master_xfer_in_progress) {
		LPI2C_MasterTransferAbort(base, &data->handle);
		data->master_xfer_in_progress = false;
	}

#ifdef CONFIG_I2C_TARGET
	if (data->target_enabled) {
		mcux_lpi2c_target_hw_deinit(dev);
	}
#endif

	if (data->master_ready) {
		mcux_lpi2c_master_hw_deinit(dev);
	}

#if CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY
	/* Optional physical bus recovery; disabled by default */
	(void)mcux_lpi2c_recover_bus_locked(dev);
#endif

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	data->callback_status = kStatus_Success;
#ifdef CONFIG_I2C_TARGET
	data->first_tx = false;
	data->read_active = false;
	data->send_ack = true;
#endif

#ifdef CONFIG_I2C_TARGET
	if (restore_target && data->target_attached && data->target_cfg) {
		ret = mcux_lpi2c_target_hw_init(dev);
		if (!ret) {
			data->mode_state = MCUX_LPI2C_STATE_TARGET_READY;
		}
		return ret;
	}
#endif

	ret = mcux_lpi2c_master_hw_init(dev);
	if (!ret) {
		data->mode_state = MCUX_LPI2C_STATE_MASTER_IDLE;
	}
	return ret;
}

static int mcux_lpi2c_transfer(const struct device *dev, struct i2c_msg *msgs,
			       uint8_t num_msgs, uint16_t addr)
{
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	lpi2c_master_transfer_t transfer;
	status_t status;
	int ret = 0;
	int ret2;
	bool pm_acquired = false;
	k_timeout_t xfer_timeout = K_MSEC(2000);

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	ret = mcux_lpi2c_enter_master_session_locked(dev);
	if (ret) {
		k_sem_give(&data->lock);
		return ret;
	}

	/* Pre-flight recovery if controller already looks stuck */
	if (LPI2C_MasterGetStatusFlags(base) & kLPI2C_MasterBusBusyFlag) {
		bool restore_target = false;
#ifdef CONFIG_I2C_TARGET
		restore_target = data->target_attached;
#endif
		ret = mcux_lpi2c_recover_locked(dev, restore_target);
		if (ret) {
			k_sem_give(&data->lock);
			return ret;
		}

		ret = mcux_lpi2c_enter_master_session_locked(dev);
		if (ret) {
			k_sem_give(&data->lock);
			return ret;
		}
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		(void)mcux_lpi2c_leave_master_session_locked(dev);
		k_sem_give(&data->lock);
		return ret;
	}
	pm_acquired = true;

	for (int i = 0; i < num_msgs; i++) {
		struct i2c_msg *msg = &msgs[i];
		bool retry_once = true;

		if (msg->flags & I2C_MSG_ADDR_10_BITS) {
			ret = -ENOTSUP;
			break;
		}

retry_submit:
		memset(&transfer, 0, sizeof(transfer));

		while (k_sem_take(&data->device_sync_sem, K_NO_WAIT) == 0) {
			;
		}

		data->callback_status = kStatus_Success;

		transfer.flags = mcux_lpi2c_convert_flags(msg->flags);
		if (i != 0 && !(msg->flags & I2C_MSG_RESTART)) {
			transfer.flags |= kLPI2C_TransferNoStartFlag;
		}

		transfer.slaveAddress = addr;
		transfer.direction = (msg->flags & I2C_MSG_READ) ?
			kLPI2C_Read : kLPI2C_Write;
		transfer.subaddress = 0;
		transfer.subaddressSize = 0;
		transfer.data = msg->buf;
		transfer.dataSize = msg->len;

		LPI2C_MasterClearStatusFlags(base, kLPI2C_MasterClearFlags);

		data->master_xfer_in_progress = true;
		status = LPI2C_MasterTransferNonBlocking(base, &data->handle, &transfer);
		if (status != kStatus_Success) {
			bool restore_target = false;
#ifdef CONFIG_I2C_TARGET
			restore_target = data->target_attached;
#endif
			LPI2C_MasterTransferAbort(base, &data->handle);
			data->master_xfer_in_progress = false;

			if (retry_once) {
				retry_once = false;
				ret = mcux_lpi2c_recover_locked(dev, restore_target);
				if (ret) {
					break;
				}
				ret = mcux_lpi2c_enter_master_session_locked(dev);
				if (ret) {
					break;
				}
				goto retry_submit;
			}

			ret = -EIO;
			break;
		}

		ret = k_sem_take(&data->device_sync_sem, xfer_timeout);
		data->master_xfer_in_progress = false;
		if (ret) {
			bool restore_target = false;
#ifdef CONFIG_I2C_TARGET
			restore_target = data->target_attached;
#endif
			LPI2C_MasterTransferAbort(base, &data->handle);
			ret2 = mcux_lpi2c_recover_locked(dev, restore_target);
			ret = ret2 ? ret2 : -ETIMEDOUT;
			break;
		}

		if (data->callback_status != kStatus_Success) {
			bool restore_target = false;
#ifdef CONFIG_I2C_TARGET
			restore_target = data->target_attached;
#endif
			LPI2C_MasterTransferAbort(base, &data->handle);
			ret2 = mcux_lpi2c_recover_locked(dev, restore_target);
			ret = ret2 ? ret2 : -EIO;
			break;
		}

		if (msg->len == 0U) {
			k_busy_wait(SCAN_DELAY_US(data->runtime_bitrate));
			if ((base->MSR & LPI2C_MSR_NDF_MASK) != 0U) {
				bool restore_target = false;
#ifdef CONFIG_I2C_TARGET
				restore_target = data->target_attached;
#endif
				LPI2C_MasterTransferAbort(base, &data->handle);
				ret2 = mcux_lpi2c_recover_locked(dev, restore_target);
				ret = ret2 ? ret2 : -EIO;
				break;
			}
		}
	}

	if (pm_acquired) {
		(void)pm_device_runtime_put(dev);
	}

	data->master_xfer_in_progress = false;

	ret2 = mcux_lpi2c_leave_master_session_locked(dev);
	if (ret == 0 && ret2) {
		ret = ret2;
	}

	k_sem_give(&data->lock);
	return ret;
}

#if CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY
static void mcux_lpi2c_bitbang_set_scl(void *io_context, int state)
{
	const struct mcux_lpi2c_config *config = io_context;

	gpio_pin_set_dt(&config->scl, state);
}

static void mcux_lpi2c_bitbang_set_sda(void *io_context, int state)
{
	const struct mcux_lpi2c_config *config = io_context;

	gpio_pin_set_dt(&config->sda, state);
}

static int mcux_lpi2c_bitbang_get_sda(void *io_context)
{
	const struct mcux_lpi2c_config *config = io_context;

	return gpio_pin_get_dt(&config->sda) == 0 ? 0 : 1;
}

static int mcux_lpi2c_recover_bus_locked(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct i2c_bitbang bitbang_ctx;
	struct i2c_bitbang_io bitbang_io = {
		.set_scl = mcux_lpi2c_bitbang_set_scl,
		.set_sda = mcux_lpi2c_bitbang_set_sda,
		.get_sda = mcux_lpi2c_bitbang_get_sda,
	};
	uint32_t bitrate_cfg;
	int error = 0;

	if (!gpio_is_ready_dt(&config->scl)) {
		LOG_ERR("SCL GPIO device not ready");
		return -EIO;
	}

	if (!gpio_is_ready_dt(&config->sda)) {
		LOG_ERR("SDA GPIO device not ready");
		return -EIO;
	}

	error = gpio_pin_configure_dt(&config->scl, GPIO_OUTPUT_HIGH);
	if (error != 0) {
		LOG_ERR("failed to configure SCL GPIO (err %d)", error);
		goto restore;
	}

	error = gpio_pin_configure_dt(&config->sda, GPIO_OUTPUT_HIGH);
	if (error != 0) {
		LOG_ERR("failed to configure SDA GPIO (err %d)", error);
		goto restore;
	}

	i2c_bitbang_init(&bitbang_ctx, &bitbang_io, (void *)config);

	bitrate_cfg = i2c_map_dt_bitrate(data->runtime_bitrate) | I2C_MODE_CONTROLLER;

	error = i2c_bitbang_configure(&bitbang_ctx, bitrate_cfg);
	if (error != 0) {
		LOG_ERR("failed to configure I2C bitbang (err %d)", error);
		goto restore;
	}

	error = i2c_bitbang_recover_bus(&bitbang_ctx);
	if (error != 0) {
		LOG_ERR("failed to recover bus (err %d)", error);
		goto restore;
	}

restore:
	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	return error;
}
#endif

#if CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
static int mcux_lpi2c_recover_bus(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	bool restore_target = false;
	int ret;

#ifdef CONFIG_I2C_TARGET
	restore_target = data->target_attached;
#endif

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	ret = mcux_lpi2c_recover_locked(dev, restore_target);

	k_sem_give(&data->lock);
	return ret;
}
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */

#ifdef CONFIG_I2C_TARGET
static void mcux_lpi2c_slave_irq_handler(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct i2c_target_callbacks *target_cb = data->target_cfg->callbacks;
	int ret;
	uint32_t flags;
	uint8_t i2c_data;

	/* The HAL provides a callback-based I2C slave API, but it expects
	 * the user to provide a fixed-length transmit buffer when the first
	 * byte is received, and it does not signal user callbacks for each
	 * byte. That does not map well to the Zephyr I2C target API, which
	 * expects per-byte callbacks. For that reason, handle the LPI2C IRQ
	 * directly here.
	 */
	flags = LPI2C_SlaveGetStatusFlags(base);

	if (flags & kLPI2C_SlaveAddressValidFlag) {
		/* Read Slave address to clear flag */
		LPI2C_SlaveGetReceivedAddress(base);
		data->first_tx = true;
		/* Reset to sending ACK, in case we NAK'ed before */
		data->send_ack = true;
	}

	if (flags & kLPI2C_SlaveRxReadyFlag) {
		/* RX data is available, read it and issue callback */
		i2c_data = (uint8_t)base->SRDR;
		ret = 0;

		if (data->first_tx) {
			data->first_tx = false;
			if (target_cb->write_requested) {
				ret = target_cb->write_requested(data->target_cfg);
				if (ret < 0) {
					/* NAK further bytes */
					data->send_ack = false;
				}
			}
		}

		if ((ret == 0) && target_cb->write_received) {
			ret = target_cb->write_received(data->target_cfg, i2c_data);
			if (ret < 0) {
				/* NAK further bytes */
				data->send_ack = false;
			}
		}
	}

	if (flags & kLPI2C_SlaveTxReadyFlag) {
		/* Space is available in TX fifo, issue callback and write out */
		if (data->first_tx) {
			data->read_active = true;
			data->first_tx = false;
			if (target_cb->read_requested) {
				ret = target_cb->read_requested(data->target_cfg,
								&i2c_data);
				if (ret < 0) {
					/* Disable TX */
					data->read_active = false;
				} else {
					/* Send I2C data */
					base->STDR = i2c_data;
				}
			}
		} else if (data->read_active) {
			if (target_cb->read_processed) {
				ret = target_cb->read_processed(data->target_cfg,
								&i2c_data);
				if (ret < 0) {
					/* Disable TX */
					data->read_active = false;
				} else {
					/* Send I2C data */
					base->STDR = i2c_data;
				}
			}
		}
	}

	if (flags & kLPI2C_SlaveStopDetectFlag) {
		LPI2C_SlaveClearStatusFlags(base, kLPI2C_SlaveStopDetectFlag);

		if (target_cb->stop) {
			target_cb->stop(data->target_cfg);
		}
	}

	if (flags & kLPI2C_SlaveTransmitAckFlag) {
		LPI2C_SlaveTransmitAck(base, data->send_ack);
	}
}

static int mcux_lpi2c_target_register(const struct device *dev,
				      struct i2c_target_config *cfg)
{
	struct mcux_lpi2c_data *data = dev->data;
	int ret;

	if (!cfg) {
		return -EINVAL;
	}

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	if (data->target_attached) {
		k_sem_give(&data->lock);
		return -EBUSY;
	}

	data->target_cfg = cfg;
	data->target_attached = true;

	ret = mcux_lpi2c_target_hw_init(dev);
	if (ret) {
		data->target_cfg = NULL;
		data->target_attached = false;
		data->mode_state = MCUX_LPI2C_STATE_MASTER_IDLE;
		k_sem_give(&data->lock);
		return ret;
	}

	data->mode_state = MCUX_LPI2C_STATE_TARGET_READY;

	k_sem_give(&data->lock);
	return 0;
}

static int mcux_lpi2c_target_unregister(const struct device *dev,
					struct i2c_target_config *cfg)
{
	struct mcux_lpi2c_data *data = dev->data;
	int ret;

	ARG_UNUSED(cfg);

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	if (!data->target_attached) {
		k_sem_give(&data->lock);
		return -EINVAL;
	}

	if (data->target_enabled) {
		mcux_lpi2c_target_hw_deinit(dev);
	}

	data->target_cfg = NULL;
	data->target_attached = false;
	data->target_enabled = false;
	data->first_tx = false;
	data->read_active = false;
	data->send_ack = true;

	data->mode_state = MCUX_LPI2C_STATE_MASTER_IDLE;

	k_sem_give(&data->lock);
	return 0;
}
#endif /* CONFIG_I2C_TARGET */

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_lp_flexcomm)
#define LPI2C_IRQHANDLE_ARG LPI2C_GetInstance(base)
#else
#define LPI2C_IRQHANDLE_ARG base
#endif

static void mcux_lpi2c_isr(const struct device *dev)
{
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = (LPI2C_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

#ifdef CONFIG_I2C_TARGET
	if (data->target_attached && data->target_enabled) {
		uint32_t ssr = LPI2C_SlaveGetStatusFlags(base);
		uint32_t slave_mask = kLPI2C_SlaveTxReadyFlag |
				      kLPI2C_SlaveRxReadyFlag |
				      kLPI2C_SlaveStopDetectFlag |
				      kLPI2C_SlaveAddressValidFlag |
				      kLPI2C_SlaveTransmitAckFlag;

		if ((ssr & slave_mask) != 0U) {
			mcux_lpi2c_slave_irq_handler(dev);
			return;
		}
	}
#endif /* CONFIG_I2C_TARGET */

	if (data->master_xfer_in_progress) {
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_lp_flexcomm)
		LPI2C_MasterTransferHandleIRQ(LPI2C_GetInstance(base),
					      &data->handle);
#else
		LPI2C_MasterTransferHandleIRQ(base, &data->handle);
#endif
	}
}

static int mcux_lpi2c_suspend(const struct device *dev)
{
	int ret;
	const struct mcux_lpi2c_config *config = dev->config;

	ret = clock_control_off(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("failed clock off lpi2c");
		return ret;
	}

	return 0;
}

static int mcux_lpi2c_resume(const struct device *dev)
{
	int ret;
	const struct mcux_lpi2c_config *config = dev->config;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("failed clock on lpi2c");
		return ret;
	}

	return 0;
}

static int mcux_lpi2c_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = mcux_lpi2c_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = mcux_lpi2c_suspend(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int mcux_lpi2c_init(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->device_sync_sem, 0, 1);

	data->callback_status = kStatus_Success;
	data->runtime_bitrate = config->bitrate;
	data->mode_state = MCUX_LPI2C_STATE_MASTER_IDLE;
	data->master_ready = false;
	data->master_xfer_in_progress = false;

#ifdef CONFIG_I2C_TARGET
	data->target_cfg = NULL;
	data->target_attached = false;
	data->target_enabled = false;
	data->first_tx = false;
	data->read_active = false;
	data->send_ack = true;
#endif

	config->irq_config_func(dev);
	return pm_device_driver_init(dev, mcux_lpi2c_pm_action);
}

static DEVICE_API(i2c, mcux_lpi2c_driver_api) = {
	.configure = mcux_lpi2c_configure,
	.transfer = mcux_lpi2c_transfer,
#if CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
	.recover_bus = mcux_lpi2c_recover_bus,
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */
#if CONFIG_I2C_TARGET
	.target_register = mcux_lpi2c_target_register,
	.target_unregister = mcux_lpi2c_target_unregister,
#endif /* CONFIG_I2C_TARGET */
};

#if CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY
#define I2C_MCUX_LPI2C_SCL_INIT(n) .scl = GPIO_DT_SPEC_INST_GET_OR(n, scl_gpios, {0}),
#define I2C_MCUX_LPI2C_SDA_INIT(n) .sda = GPIO_DT_SPEC_INST_GET_OR(n, sda_gpios, {0}),
#else
#define I2C_MCUX_LPI2C_SCL_INIT(n)
#define I2C_MCUX_LPI2C_SDA_INIT(n)
#endif /* CONFIG_I2C_MCUX_LPI2C_GPIO_BUS_RECOVERY */

#define I2C_MCUX_LPI2C_CONFIGURE_IRQ(idx, inst)					\
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, idx), (				\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, idx, irq),			\
			DT_INST_IRQ_BY_IDX(inst, idx, priority),		\
			mcux_lpi2c_isr,						\
			DEVICE_DT_INST_GET(inst), 0);				\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, idx, irq));		\
	))

/* When using LP Flexcomm driver, register the interrupt handler
 * so we receive notification from the LP Flexcomm interrupt handler.
 */
#define I2C_MCUX_LPI2C_LPFLEXCOMM_IRQ_FUNC(n)					\
	nxp_lp_flexcomm_setirqhandler(DEVICE_DT_GET(DT_INST_PARENT(n)),		\
					DEVICE_DT_INST_GET(n),			\
					LP_FLEXCOMM_PERIPH_LPI2C,		\
					mcux_lpi2c_isr)

#define I2C_MCUX_LPI2C_IRQ_SETUP_FUNC(n)					\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_INST_PARENT(n),			\
					nxp_lp_flexcomm),			\
		    (I2C_MCUX_LPI2C_LPFLEXCOMM_IRQ_FUNC(n)),			\
		    (LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),			\
			I2C_MCUX_LPI2C_CONFIGURE_IRQ, (), n)))

#define I2C_MCUX_LPI2C_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	static void mcux_lpi2c_config_func_##n(const struct device *dev)	\
	{									\
		I2C_MCUX_LPI2C_IRQ_SETUP_FUNC(n);				\
	}									\
										\
	static const struct mcux_lpi2c_config mcux_lpi2c_config_##n = {		\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)COND_CODE_1(		\
			DT_PHA_HAS_CELL(DT_DRV_INST(n), clocks, name),		\
			(DT_INST_CLOCKS_CELL(n, name)), (0U)),			\
		.irq_config_func = mcux_lpi2c_config_func_##n,			\
		.bitrate = DT_INST_PROP(n, clock_frequency),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		I2C_MCUX_LPI2C_SCL_INIT(n)					\
		I2C_MCUX_LPI2C_SDA_INIT(n)					\
		.bus_idle_timeout_ns =						\
			UTIL_AND(DT_INST_NODE_HAS_PROP(n, bus_idle_timeout),	\
				 DT_INST_PROP(n, bus_idle_timeout)),		\
	};									\
										\
	static struct mcux_lpi2c_data mcux_lpi2c_data_##n;			\
										\
	PM_DEVICE_DT_INST_DEFINE(n, mcux_lpi2c_pm_action);			\
										\
	I2C_DEVICE_DT_INST_DEFINE(n, mcux_lpi2c_init,				\
				PM_DEVICE_DT_INST_GET(n),			\
				&mcux_lpi2c_data_##n,				\
				&mcux_lpi2c_config_##n, POST_KERNEL,		\
				CONFIG_I2C_INIT_PRIORITY,			\
				&mcux_lpi2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_MCUX_LPI2C_INIT)
