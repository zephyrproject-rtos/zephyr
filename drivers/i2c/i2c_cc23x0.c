/*
 * Copyright (c) 2025, Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_i2c

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/irq.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_cc23x0);

#include <driverlib/clkctl.h>
#include <driverlib/i2c.h>

#include "i2c-priv.h"

#ifndef I2C_CONTROLLER_CMD_BURST_RECEIVE_START_NACK
#define I2C_CONTROLLER_CMD_BURST_RECEIVE_START_NACK I2C_CONTROLLER_CMD_BURST_SEND_START
#endif

#define I2CTARGET_INT_FLAGS (I2C_TARGET_INT_DATA | I2C_TARGET_INT_STOP)

/* I2C Target States */
#define I2CTARGET_IDLE              0      /* Nothing happening, waiting for START condition */
#define I2CTARGET_RECEIVING         BIT(0) /* Receiving data from controller */
#define I2CTARGET_TRANSMITING       BIT(1) /* Transmitting data to controller */
#define I2CTARGET_RX_NACK_NEXT_BYTE BIT(2) /* NACK next byte to be received */
#define I2CTARGET_TX_IGNORE         BIT(3) /* Ignore next byte to be transmitted */

/**
 * This structure holds the runtime state and configuration for an instance
 * of the CC23x0 I2C controller.
 */
struct i2c_cc23x0_data {
	bool is_configured;              /* Indicates if the I2C controller has been configured */
	struct k_sem sync_sem;           /* Semaphore used for blocking I2C operations */
	struct k_mutex mutex;            /* Mutex for protecting against multiple I2C operations */
	volatile int status;             /* Holds the current status of the I2C transaction */
	struct i2c_msg *msgs;            /* Pointer to chain of messages provided by user */
	uint8_t num_msgs;                /* Number of messages in the msgs chain */
	volatile uint8_t current_msg_index;       /* Index of the current message  */
	volatile bool controller_is_transmitting; /* Flag indicating controller is transmitting */
	uint32_t cfg;                             /* Cached configuration value for controller */
	uint16_t addr;                            /* I2C target address */
	bool  is_blocking;                        /* I2C synchronous mode */
#ifdef CONFIG_I2C_CALLBACK
	i2c_callback_t cb;                      /* Callback function for asynchronous operations */
	void *cb_data;                          /* User data for callback */
#endif /* CONFIG_I2C_CALLBACK */
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_cfg;   /* Pointer to target configuration */
	volatile uint8_t target_state;          /* Current state in target mode */
#endif
};

/**
 *  Static configuration of an instance of the CC23x0 I2C driver.
 */
struct i2c_cc23x0_config {
	uint32_t base;                         /* Base address of the I2C controller registers */
	const struct pinctrl_dev_config *pcfg; /* Pin control configuration */
};

/**
 * @brief Acquire a lock on the power management policy state for the I2C CC23x0 driver.
 *
 * This function is used to lock the power management policy state to prevent
 * the device from entering low-power modes while an I2C transaction is in progress.
 *
 * @param data Pointer to the I2C CC23x0 driver data structure.
 */
static inline void i2c_cc23x0_pm_policy_state_lock_get(struct i2c_cc23x0_data *data)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

/**
 * @brief Releases a previously acquired power management policy state lock for the I2C
 * peripheral.
 *
 * This function should be called when the I2C peripheral no longer needs to prevent
 * the system from entering certain low-power states. It decrements the lock count or
 * otherwise signals that the peripheral can tolerate power state changes.
 *
 * @param data Pointer to the I2C driver data structure.
 */
static inline void i2c_cc23x0_pm_policy_state_lock_put(struct i2c_cc23x0_data *data)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
#endif
}

/**
 * @brief Retrieve the current configuration of the I2C peripheral.
 *
 * This function obtains the configuration settings for the specified I2C device.
 *
 * @param dev    Pointer to the device structure for the I2C instance.
 * @param config Pointer to a variable where the configuration value will be stored.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int i2c_cc23x0_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_cc23x0_data *data = dev->data;

	if (!data->is_configured) {
		LOG_ERR("I2C controller not configured");
		return -EIO;
	}

	*config = data->cfg;

	return 0;
}

/**
 * @brief Primes an I2C transfer on the CC23x0 device.
 *
 * This function prepares the I2C hardware for a data transfer operation.
 * It configures the device with the specified message and target address,
 * setting up the necessary registers and state for either a read or write
 * transaction as described by the i2c_msg structure.
 *
 * @note This function initiates a transfer but does not wait for completion.
 *
 * @param dev   Pointer to the device structure for the I2C controller.
 * @param msg   Pointer to the I2C message structure containing transfer details
 *              such as buffer, length, and transfer direction.
 * @param addr  7-bit I2C address of the target device.
 * @param prevFlags Flags from the previous message, used to determine
 *                  if a repeated start condition is needed.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int i2c_cc23x0_prime_transfer(const struct device *dev, struct i2c_msg *msg,
									  uint16_t addr, uint8_t prevFlags)
{
	struct i2c_cc23x0_data *data = dev->data;
	const struct i2c_cc23x0_config *config = dev->config;
	bool stopRequested = (msg->flags & I2C_MSG_STOP) != 0;
	bool prevMsgWasTx = data->controller_is_transmitting;

#ifdef CONFIG_I2C_TARGET
	/* I2c module has been configured to target mode */
	if (data->target_cfg != NULL) {
		LOG_ERR("I2C module already configured to target mode");
		return -EBUSY;
	}
#endif

	if (!data->is_configured) {
		LOG_ERR("I2C module has not yet been configured");
		return -EINVAL;
	}

	if (!msg) {
		LOG_ERR("Invalid message pointer");
		return -EINVAL;
	}

	if (!msg->len || !msg->buf) {
		return -EINVAL;
	}

	/* Not supported by hardware */
	if (msg->flags & I2C_MSG_ADDR_10_BITS) {
		return -EINVAL;
	}

	data->controller_is_transmitting = (msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE;

	if (data->controller_is_transmitting) {
		/* This is a write operation */
		I2CControllerEnableInt(config->base);

		/* Specify target address and transmit mode */
		I2CControllerSetTargetAddr(config->base, addr, false);

		I2CControllerPutData(config->base, *(msg->buf++));
		I2CControllerCommand(config->base, I2C_CONTROLLER_CMD_BURST_SEND_START);
	} else {
		/* This is a read operation */
		I2CControllerEnableInt(config->base);

		/* Specify target address and receive mode */
		I2CControllerSetTargetAddr(config->base, addr, true);

		uint32_t command = I2C_CCTL_RUN_EN;

		if (prevMsgWasTx && ((prevFlags & I2C_MSG_STOP) == 0)) {
			/* The last transfer was a TX and a stop condition was not put on the bus
			 * This means the transfer is a continuation of the previous one and
			 * a repeated start condition is needed.
			 */
			if (msg->len > 1) {
				/* RUN and generate ACK to target */
				command |= I2C_CCTL_ACK_M;
			}

			/* RUN and generate a repeated START */
			command |= I2C_CCTL_START_M;
			I2CControllerCommand(config->base, command);
		} else {
			if ((msg->len == 1) && (stopRequested)) {
				/* Send START, read 1 data byte, and NACK */
				I2CControllerCommand(config->base,
							 I2C_CONTROLLER_CMD_BURST_RECEIVE_START_NACK);
			} else {
				/* Start the I2C transfer in controller receive mode */
				I2CControllerCommand(config->base,
							 I2C_CONTROLLER_CMD_BURST_RECEIVE_START);
			}
		}
	}

	return 0;
}

#ifdef CONFIG_I2C_TARGET
/**
 * @brief Registers an I2C target device configuration for the CC23x0 I2C driver.
 *
 * This function sets up the specified I2C target configuration for the given device.
 * It enables the device to act as an I2C target with the provided configuration.
 *
 * @note Calling this function will disallow the device from entering low-power
 * states until the target is unregistered.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 * @param target_cfg Pointer to the I2C target configuration structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_cc23x0_target_register(const struct device *dev,
									  struct i2c_target_config *target_cfg)
{
	struct i2c_cc23x0_data *data = dev->data;
	const struct i2c_cc23x0_config *config = dev->config;

	if (data->target_cfg != NULL) {
		LOG_ERR("Only one target can be registered at a time");
		return -EBUSY;
	}

	if (target_cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		LOG_ERR("10-bit addressing mode is not supported");
		return -ENOSYS;
	}

	/* Driver is stopped/inactive. Disable interrupts */
	I2CTargetDisableInt(config->base, I2CTARGET_INT_FLAGS);

	/* Disable, clear interrupts, then re-enable */
	I2CTargetDisable(config->base);
	I2CTargetClearInt(config->base, I2CTARGET_INT_FLAGS);
	I2CTargetEnableInt(config->base, I2CTARGET_INT_FLAGS);

	/* Disable standby and idle policy states */
	i2c_cc23x0_pm_policy_state_lock_get(data);

	/* Initialize target mode */
	I2CTargetInit(config->base, (uint8_t)target_cfg->address);

	data->target_cfg = target_cfg;
	data->target_state = I2CTARGET_IDLE;

	return 0;
}

/**
 * @brief Unregister an I2C target device from the I2C controller.
 *
 * This function removes the specified I2C target configuration from the
 * controller associated with the given device. After unregistration, the
 * target device will no longer respond to I2C transactions on the bus.
 *
 * @note Calling this API will re-enable the device to enter low-power states.
 *
 * @param dev Pointer to the I2C controller device structure.
 * @param cfg Pointer to the I2C target configuration to unregister.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int i2c_cc23x0_target_unregister(const struct device *dev,
										struct i2c_target_config *cfg)
{
	struct i2c_cc23x0_data *data = dev->data;
	const struct i2c_cc23x0_config *config = dev->config;

	if (data->target_cfg != cfg) {
		LOG_ERR("Unregistering a different target");
		return -EINVAL;
	}

	I2CTargetDisableInt(config->base, I2CTARGET_INT_FLAGS);
	I2CTargetClearInt(config->base, I2CTARGET_INT_FLAGS);
	I2CTargetDisable(config->base);

	data->target_cfg = NULL;
	data->target_state = I2CTARGET_IDLE;

	/* Re-enable standby and idle policy states */
	i2c_cc23x0_pm_policy_state_lock_put(data);

	return 0;
}
#endif /* CONFIG_I2C_TARGET */

#ifdef CONFIG_I2C_CALLBACK

/**
 * @brief Handles I2C transfer as a controller with callback support.
 *
 * This function initiates an I2C transfer on the specified device as a controller,
 * processing an array of I2C messages. It supports callback-based completion and
 * error handling mechanisms.
 *
 * @param dev   Pointer to the I2C device structure.
 * @param msgs  Pointer to an array of I2C message structures to be transferred.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_cc23x0_transfer_cb_controller(const struct device *dev, struct i2c_msg *msgs,
						 uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
						 void *userdata)
{
	struct i2c_cc23x0_data *data = dev->data;

	if (k_mutex_lock(&data->mutex, K_NO_WAIT) != 0) {
		LOG_ERR("I2C controller already busy with a transfer");
		return -EWOULDBLOCK;
	}

	data->cb = cb;
	data->cb_data = userdata;
	data->current_msg_index = 0;
	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->addr = addr;
	data->is_blocking = false;

	/* Acquire the pm policy state lock to prevent the device from entering low-power modes
	 * This constraint is released in the ISR after the final transfer has completed
	 */
	i2c_cc23x0_pm_policy_state_lock_get(data);

	/* Start the first transfer but don't wait for completion. The I2C ISR will
	 * handle submission of subsequent transfers and call the registered callback
	 * when all transfers are complete or there was an error.
	 */
	int ret = i2c_cc23x0_prime_transfer(dev, msgs, addr, 0);

	return ret;
}
#endif /* CONFIG_I2C_CALLBACK */

/**
 * @brief Perform an I2C transfer as a controller and blocks until completion of all
 * transfers.
 *
 * This function handles the transmission and/or reception of one or more I2C messages
 * on the specified I2C device. It is typically called by the Zephyr I2C subsystem
 * when an I2C transaction is requested.
 *
 * @param dev   Pointer to the device structure for the I2C controller.
 * @param msgs  Pointer to an array of I2C message structures to be transferred.
 * @param num_msgs Number of messages to transfer.
 * @param addr  7-bit I2C address of the target device.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_cc23x0_controller_transfer(const struct device *dev, struct i2c_msg *msgs,
					  uint8_t num_msgs, uint16_t addr)
{
	struct i2c_cc23x0_data *data = dev->data;

	if (k_mutex_lock(&data->mutex, K_NO_WAIT) != 0) {
		LOG_ERR("I2C controller already busy with a transfer");
		return -EWOULDBLOCK;
	}

#ifdef CONFIG_I2C_CALLBACK
	data->cb = NULL;
	data->cb_data = NULL;
#endif /* CONFIG_I2C_CALLBACK */
	data->current_msg_index = 0;
	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->addr = addr;
	data->is_blocking = true;

	__ASSERT(k_sem_count_get(&data->sync_sem) == 0,
	       "I2C semaphore already taken before transfer");

	/* Acquire the pm policy state lock to prevent the device from entering low-power modes
	 * This constraint is released in the ISR after the final transfer has completed
	 */
	i2c_cc23x0_pm_policy_state_lock_get(data);

	/* Start the first transfer, subsequent transfers will be initiated from the ISR */
	int ret = i2c_cc23x0_prime_transfer(dev, msgs, addr, 0);

	if (!ret) {
		/* Wait for all transfers to complete. This gets posted in the
		 * transfer complete function in an ISR context after the last
		 * message has been sent or an error occurred.
		 */
		k_sem_take(&data->sync_sem, K_FOREVER);

		/* The status of the transfer is stored in the data context */
		ret = data->status;
	}

	return ret;
}

/**
 * @brief Configures the I2C controller at runtime.
 *
 * This function applies the specified configuration to the I2C controller
 * associated with the given device. It is typically called to set up
 * parameters such as speed, addressing mode, and other controller-specific
 * options during runtime.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 * @param dev_config Bitmask specifying the desired I2C configuration options.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int i2c_cc23x0_runtime_controller_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_cc23x0_config *config = dev->config;
	struct i2c_cc23x0_data *data = dev->data;
	bool fast;

#ifdef CONFIG_I2C_TARGET
	/* I2c module has been configured to target mode */
	if (data->target_cfg != NULL) {
		LOG_ERR("I2C module already configured to target mode");
		return -EBUSY;
	}
#endif

	if (data->is_configured) {
		LOG_ERR("I2C controller already configured");
		return -EBUSY;
	}

	/* Support for slave mode has not been implemented */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("The I2C configure API is only for controller mode");
		return -ENOTSUP;
	}

	/* This is deprecated and could be ignored in the future */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bit addressing mode is not supported");
		return -ENOSYS;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		fast = false;
		break;
	case I2C_SPEED_FAST:
		fast = true;
		break;
	default:
		LOG_ERR("Unsupported speed");
		return -ENOTSUP;
	}

	/* Disable and clear interrupts possible from soft resets */
	I2CControllerDisableInt(config->base);
	I2CControllerClearInt(config->base);

	/* Enables and configures I2C master */
	I2CControllerInitExpClk(config->base, fast);

	I2CControllerEnable(config->base);
	I2CControllerEnableInt(config->base);

	data->cfg = dev_config;
	data->is_configured = true;

	k_mutex_init(&data->mutex);

	return 0;
}

#ifdef CONFIG_PM_DEVICE

/**
 * @brief Handles power management actions for the CC23x0 I2C device.
 *
 * This function is called to perform specific power management actions
 * (such as suspend, resume, or turn off) on the given I2C device.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 * @param action The power management action to be performed.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_cc23x0_config *config = dev->config;
	struct i2c_cc23x0_data *data = dev->data;

	/* Only perform actions if the I2C controller has been configured */
	if (data->is_configured) {
		switch (action) {
		case PM_DEVICE_ACTION_SUSPEND:
			/* Disable the controller */
			I2CControllerDisable(config->base);
			I2CControllerDisableInt(config->base);
			I2CControllerClearInt(config->base);
			break;
		case PM_DEVICE_ACTION_RESUME:
			/* Re-enable the controller */
			I2CControllerClearInt(config->base);
			I2CControllerEnable(config->base);
			I2CControllerEnableInt(config->base);
			break;
		default:
			return -ENOTSUP;
		}
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_I2C_TARGET

/**
 * @brief Interrupt Service Routine (ISR) for I2C target mode.
 *
 * Handles I2C events when the device is operating in target mode.
 * This function is triggered by hardware interrupts and is responsible for
 * processing incoming I2C transactions, such as read and write requests from
 * the I2C controller.
 *
 * @param dev Pointer to the device structure for the I2C driver instance.
 */
static void i2c_cc23x0_isr_target(const struct device *dev)
{
	const struct i2c_cc23x0_config *config = dev->config;
	struct i2c_cc23x0_data *data = dev->data;
	int cb_status;

	/* Get interrupt status and clear */
	uint32_t intFlags = I2CTargetIntStatus(config->base, true);

	I2CTargetClearInt(config->base, I2CTARGET_INT_FLAGS);

	/* Check if controller has written or requested data */
	uint32_t status = I2CTargetStatus(config->base);

	/* Controller will write to target (target-receiver) */
	if (status & I2C_TARGET_ACT_RREQ) {
		data->target_state |= I2CTARGET_RECEIVING;
		if (data->target_cfg->callbacks->write_requested != NULL) {
			/* Call the write requested callback function */
			cb_status = data->target_cfg->callbacks->write_requested(data->target_cfg);

			/* Set the NACK bit (has no effect but might be worth tracking for the future) */
			if (cb_status != 0) {
				data->target_state |= I2CTARGET_RX_NACK_NEXT_BYTE;
			} else {
				data->target_state &= ~I2CTARGET_RX_NACK_NEXT_BYTE;
			}
		}
	}

	/* Controller has written first byte to target (target-receiver) */
	if (status & I2C_TARGET_ACT_RREQ_FBR) {
		data->target_state |= I2CTARGET_RECEIVING;

		/* Read data byte from I2C peripheral */
		uint8_t dataByte = I2CTargetGetData(config->base);

		if (data->target_cfg->callbacks->write_received != NULL) {
			cb_status = data->target_cfg->callbacks->write_received(data->target_cfg,
										dataByte);

			/* Set the NACK bit (has no effect but might be worth tracking for the future) */
			if (cb_status != 0) {
				data->target_state |= I2CTARGET_RX_NACK_NEXT_BYTE;
			} else {
				data->target_state &= ~I2CTARGET_RX_NACK_NEXT_BYTE;
			}
		}
	}

	/* Controller wants to read from target (target-transmitter) */
	if (status & I2C_TARGET_ACT_TREQ) {
		uint8_t dataByte;

		/* Use internal state machine to differentiate between
		 * - the first byte (read requested) - I2CTARGET_IDLE, and
		 * - subsequent bytes (read processed) - I2CTARGET_TRANSMITING.
		 */
		if (!(data->target_state & I2CTARGET_TRANSMITING)) {
			data->target_state = I2CTARGET_TRANSMITING;
			if (data->target_cfg->callbacks->read_requested != NULL) {
				/* Call the read requested callback function (first byte requested) */
				cb_status = data->target_cfg->callbacks->read_requested(
					data->target_cfg, &dataByte);
				if (cb_status != 0) {
					data->target_state |= I2CTARGET_TX_IGNORE;
				} else {
					/* Write byte to I2C peripheral */
					I2CTargetPutData(config->base, dataByte);
				}
			}
		} else if ((data->target_state & I2CTARGET_TRANSMITING) &&
			   !(data->target_state & I2CTARGET_TX_IGNORE)) {
			if (data->target_cfg->callbacks->read_processed != NULL) {
				/* Call the read processed callback function (subsequent bytes requested) */
				cb_status = data->target_cfg->callbacks->read_processed(
					data->target_cfg, &dataByte);
				if (cb_status != 0) {
					data->target_state |= I2CTARGET_TX_IGNORE;
				} else {
					/* Write byte to I2C peripheral */
					I2CTargetPutData(config->base, dataByte);
				}
			}
		} else {
			LOG_DBG("Ignoring read request. Current target state: 0x%x",
				data->target_state);
		}
	}

	if (intFlags & I2C_TARGET_INT_STOP) {
		data->target_state = I2CTARGET_IDLE;
		if (data->target_cfg->callbacks->stop != NULL) {
			/* Call the stop received callback function */
			(void)data->target_cfg->callbacks->stop(data->target_cfg);
		}
	}
}
#endif /* CONFIG_I2C_TARGET */

/**
 * @brief Handles the completion of an I2C controller transfer.
 *
 * This function is called when an I2C transfer initiated by the controller
 * has finished from an ISR context. It is responsible for performing any
 * necessary cleanup, notifying any pending contexts internal to the driver
 * and continuing processing if there are more messages chained for transmission.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 */
static void i2c_cc23x0_controller_transfer_complete(const struct device *dev)
{
	const struct i2c_cc23x0_config *config = dev->config;
	bool completed = false;
	struct i2c_cc23x0_data *data = dev->data;

	/* Handle chained transactions in callback mode. */
	data->current_msg_index++;
	if ((data->current_msg_index < data->num_msgs) && !(data->status)) {
		/* Send the next message */
		(void)i2c_cc23x0_prime_transfer(dev, &data->msgs[data->current_msg_index],
							 data->addr, data->msgs[data->current_msg_index-1].flags);
	} else {
		/* All messages have been sent, or there was an error */
		completed = true;
	}

	if (completed) {
		/* Post the semaphore to indicate completion (sync mode) */
		if (data->is_blocking) {
			k_sem_give(&(data->sync_sem));
		}

		/* Disable and clear any interrupts */
		I2CControllerDisableInt(config->base);
		I2CControllerClearInt(config->base);

		/* Release the power dependency */
		i2c_cc23x0_pm_policy_state_lock_put(data);

		/* Release the mutex to allow other transfers */
		k_mutex_unlock(&data->mutex);

#ifdef CONFIG_I2C_CALLBACK
		/* Call the user callback function */
		if (data->cb != NULL) {
			data->cb(dev, data->status, data->cb_data);
			data->cb = NULL;
			data->cb_data = NULL;
			data->current_msg_index = 0;
		}
#endif /* CONFIG_I2C_CALLBACK */
	}
}

/**
 * @brief Interrupt Service Routine (ISR) for the I2C controller on CC23x0 devices.
 *
 * This function handles I2C controller interrupts for the specified device.
 * It processes interrupt events such as data transmission, reception, and error
 * conditions, ensuring correct I2C bus operation.
 *
 * @param dev Pointer to the device structure for the I2C controller instance.
 */
static void i2c_cc23x0_controller_isr(const struct device *dev)
{
	const struct i2c_cc23x0_config *config = dev->config;
	struct i2c_cc23x0_data *data = dev->data;
	struct i2c_msg *msg = &data->msgs[data->current_msg_index];

	/* Clear the interrupt */
	I2CControllerClearInt(config->base);

	uint32_t status = HWREG(I2C0_BASE + I2C_O_CSTA);

	/* Handle errors. ERR bit is not set if arbitration lost.
	 * The I2C peripheral has an issue where the DATACK_N bit
	 * is not updated if the previous command sets the ACK bit
	 * (Controller automatically ACK's received data). This condition
	 * can be detected by the state of writeCount, readCount, and
	 * the status register. If the condition is true, don't enter
	 * the error-handling block, but carry on reading instead.
	 */
	if ((status & (I2C_CSTA_ERR_M | I2C_CSTA_ARBLST_M)) &&
		!(msg->len == 0 && data->controller_is_transmitting &&
		  ((status & 0x1F) == (I2C_CSTA_ERR_M | I2C_CSTA_DATACKN_M)))) {
		/* Decode interrupt status */
		if (status & I2C_CSTA_ARBLST_M) {
			/* Arbitration lost */
			data->status = -EIO;
		}
		/*
		 * The I2C peripheral has an issue where the first data byte
		 * is always transmitted, regardless of the ADDR NACK. Therefore,
		 * we should check this error condition first.
		 */
		else if (status & I2C_CSTA_ADRACKN_M) {
			/* I2C target address not acknowledged */
			data->status = -EIO;
		} else {
			/* Last possible bit is the I2C_MSTAT_DATACK_N */
			data->status = -EIO;
		}

		/*
		 * The Low Power F3 I2C peripheral does not have an explicit STOP
		 * interrupt bit. Therefore, if an error occurred, we send the STOP
		 * bit and complete the transfer immediately.
		 */
		I2CControllerCommand(config->base, I2C_CCTL_STOP_EN);
		i2c_cc23x0_controller_transfer_complete(dev);
	} else if ((msg->len != 0) && (data->controller_is_transmitting)) {
		/* Just sent a byte */
		msg->len--;
		/* Is there more to transmit? */
		if (msg->len) {
			I2CControllerPutData(config->base, *(msg->buf++));
			I2CControllerCommand(config->base, I2C_CCTL_RUN_EN);
		} else if (msg->flags & I2C_MSG_STOP) {
			/* If the last byte is sent, send STOP */
			I2CControllerCommand(config->base, I2C_CCTL_STOP_EN);
		} else {
			data->status = 0;
			i2c_cc23x0_controller_transfer_complete(dev);
		}
	} else if ((msg->len != 0) && !(data->controller_is_transmitting)) {
		/* Just received a byte */
		msg->len--;
		/* Read data */
		*(msg->buf++) = I2CControllerGetData(config->base);

		uint32_t command = I2C_CCTL_RUN_DIS;

		if (msg->len > 1) {
			/* Send ACK and RUN */
			command = I2C_CCTL_RUN_EN | I2C_CCTL_ACK_EN;
		} else if ((msg->len < 1) && (msg->flags & I2C_MSG_STOP)) {
			/* Send STOP */
			command = I2C_CCTL_STOP_EN;
		} else if (msg->len < 1) {
			/* This is the last byte but a stop wasn't requested so this cb will
			 * not get triggered again and we have to clean things up here
			 */
			data->status = 0;
			i2c_cc23x0_controller_transfer_complete(dev);
		} else {
			/* Send RUN */
			command = I2C_CCTL_RUN_EN;
		}

		if (command != I2C_CCTL_RUN_DIS) {
			I2CControllerCommand(config->base, command);
		}
	} else {
		/* Getting here signals a stop condition was sent and the transaction is complete */
		data->status = 0;
		i2c_cc23x0_controller_transfer_complete(dev);
	}
}

/**
 * @brief Interrupt Service Routine (ISR) for the CC23x0 I2C driver.
 *
 * This function handles I2C-related interrupts for the specified device.
 * It is responsible for processing I2C events such as data transmission,
 * reception, errors, and other interrupt-driven operations.
 *
 * @param dev Pointer to the device structure for the I2C controller instance.
 */
static void i2c_cc23x0_isr(const struct device *dev)
{
#ifdef CONFIG_I2C_TARGET
	struct i2c_cc23x0_data *data = dev->data;

	if (data->target_cfg != NULL) {
		/* Target mode */
		i2c_cc23x0_isr_target(dev);
	} else
#endif /* CONFIG_I2C_TARGET */
	{
		/* Controller mode */
		i2c_cc23x0_controller_isr(dev);
	}
}

/**
 * @brief I2C driver API structure for CC23x0 devices.
 *
 * This structure defines the set of function pointers implementing the
 * I2C driver operations for the CC23x0 series. It is used by the Zephyr
 * I2C subsystem to interface with the hardware-specific driver functions.
 */
static const struct i2c_driver_api i2c_cc23x0_driver_api = {
	.configure = i2c_cc23x0_runtime_controller_configure,
	.transfer = i2c_cc23x0_controller_transfer,
	.get_config = i2c_cc23x0_get_config,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i2c_cc23x0_transfer_cb_controller,
#endif
	.recover_bus = NULL, /* Not supported */
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_cc23x0_target_register,
	.target_unregister = i2c_cc23x0_target_unregister,
#endif
};

#define I2C_CC23X0_INIT_FUNC(id)                                                            \
	static int i2c_cc23x0_init##id(const struct device *dev)                                \
	{                                                                                       \
		const struct i2c_cc23x0_config *config = dev->config;                               \
																							\
		int err;                                                                            \
																							\
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), i2c_cc23x0_isr,            \
				DEVICE_DT_INST_GET(id), 0);                                                 \
																							\
		irq_enable(DT_INST_IRQN(id));                                                       \
																							\
		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);                     \
		if (err < 0) {                                                                      \
			LOG_ERR("Failed to configure pinctrl state\n");                                 \
			return err;                                                                     \
		}                                                                                   \
																							\
		/* Turn the peripheral on */                                                        \
		CLKCTLEnable(CLKCTL_BASE, CLKCTL_I2C0);                                             \
																							\
		return 0;                                                                           \
	}

#define CC23X0_I2C(id)                                                                      \
	I2C_CC23X0_INIT_FUNC(id);                                                               \
	PM_DEVICE_DT_INST_DEFINE(id, i2c_cc23x0_pm_action);                                     \
	PINCTRL_DT_INST_DEFINE(id);                                                             \
																							\
	static const struct i2c_cc23x0_config i2c_cc23x0_##id##_config = {                      \
		.base = DT_INST_REG_ADDR(id),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                         \
	};                                                                                      \
																							\
	static struct i2c_cc23x0_data i2c_cc23x0_##id##_data = {                                \
		.sync_sem = Z_SEM_INITIALIZER(i2c_cc23x0_##id##_data.sync_sem, 0, 1),               \
		.status = 0,                                                                        \
		.cfg = 0,                                                                           \
	};                                                                                      \
																							\
	I2C_DEVICE_DT_INST_DEFINE(id, i2c_cc23x0_init##id, PM_DEVICE_DT_INST_GET(id),           \
				  &i2c_cc23x0_##id##_data, &i2c_cc23x0_##id##_config, POST_KERNEL,          \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_cc23x0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CC23X0_I2C);
