/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_i2c

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <sl_i2c.h>
#include <sli_i2c.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
LOG_MODULE_REGISTER(silabs);
#include "i2c-priv.h"

/* Structure for DMA configuration */
struct i2c_silabs_dma_config {
	const struct device *dma_dev; /* Pointer to the DMA device structure */
	int dma_channel;         /* DMA channel number */
};

/* Structure for I2C device configuration */
struct i2c_silabs_dev_config {
	const struct pinctrl_dev_config *pcfg; /* Pin configuration for the I2C instance */
	sl_peripheral_t peripheral;            /* I2C peripheral structure */
	uint32_t bitrate;                      /* I2C bitrate (clock frequency) */
	void (*irq_config_func)(void);         /* IRQ configuration function */
	const struct device *clock;            /* Clock device */
	const struct silabs_clock_control_cmu_config clock_cfg; /* Clock control subsystem */
};

/* Structure for I2C device data */
struct i2c_silabs_dev_data {
	struct k_sem bus_lock;               /* Semaphore to lock the I2C bus */
	struct k_sem transfer_sem;           /* Semaphore to manage transfer */
	sl_i2c_handle_t i2c_handle;          /* I2C handle structure */
	struct i2c_silabs_dma_config dma_rx; /* DMA configuration for RX */
	struct i2c_silabs_dma_config dma_tx; /* DMA configuration for TX */
	bool asynchronous;                   /* Indicates if transfer is asynchronous */
	bool last_transfer;                  /* Transfer is the last in the sequence */
	bool is_10bit_addr;                  /* Indicates if addr is 7-bit or 10-bit */
#if defined CONFIG_I2C_CALLBACK
	i2c_callback_t callback; /* I2C callback function pointer */
	void *callback_context;  /* Context for I2C callback */
	bool callback_invoked;   /* Tracks if callback has been invoked */
#endif
	bool pm_lock_done; /* Tracks if PM lock release has occurred */
};

static bool i2c_silabs_is_dma_enabled_instance(const struct device *dev)
{
	struct i2c_silabs_dev_data *data = dev->data;

	__ASSERT_NO_MSG(!!data->dma_tx.dma_dev == !!data->dma_rx.dma_dev);

	return data->dma_rx.dma_dev != NULL;
}

static void i2c_silabs_pm_policy_state_lock_get(void)
{
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static void i2c_silabs_pm_policy_state_lock_put(void)
{
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

/* Function to configure I2C peripheral */
static int i2c_silabs_dev_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_silabs_dev_config *config = dev->config;
	struct i2c_silabs_dev_data *data = dev->data;

	/* Determine the I2C speed and corresponding baudrate */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->i2c_handle.frequency_mode = SL_I2C_FREQ_STANDARD_MODE;
		break;
	case I2C_SPEED_FAST:
		data->i2c_handle.frequency_mode = SL_I2C_FREQ_FAST_MODE;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->i2c_handle.frequency_mode = SL_I2C_FREQ_FASTPLUS_MODE;
		break;
	default:
		return -EINVAL;
	}

	/* Take the bus lock semaphore to ensure exclusive access */
	k_sem_take(&data->bus_lock, K_FOREVER);
	/* Initialize I2C parameters */
	data->i2c_handle.i2c_peripheral = config->peripheral;

	/* Set the operating mode (leader or follower) */
	if (IS_ENABLED(CONFIG_I2C_TARGET)) {
		data->i2c_handle.operating_mode = SL_I2C_FOLLOWER_MODE;
	} else {
		data->i2c_handle.operating_mode = SL_I2C_LEADER_MODE;
	}

	/* Configure the I2C instance */
	sli_i2c_init_core(&data->i2c_handle);

	/* Release the bus lock semaphore */
	k_sem_give(&data->bus_lock);

	return 0;
}

/* Function to handle DMA transfer */
static int i2c_silabs_transfer_dma(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr, i2c_callback_t cb, void *userdata,
				   bool asynchronous)
{
	struct i2c_silabs_dev_data *data = dev->data;
#if defined(CONFIG_I2C_CALLBACK)
	data->callback_invoked = false;
#endif
	data->pm_lock_done = false;
	uint8_t i = 0;
	int err = 0;

	if (!IS_ENABLED(CONFIG_I2C_SILABS_DMA)) {
		return -ENOTSUP;
	}

	/* Get the power management policy state lock */
	i2c_silabs_pm_policy_state_lock_get();

	while (i < num_msgs) {
		uint8_t msgs_in_transfer = 1;

		/*  Combined DMA write-read (repeated start) */
		if ((msgs[i].flags & I2C_MSG_WRITE) == 0 && (i + 1 < num_msgs) &&
		    (msgs[i + 1].flags & I2C_MSG_READ)) {
			msgs_in_transfer = 2;
		}
		data->last_transfer = (i + msgs_in_transfer) == num_msgs;

		if (msgs_in_transfer == 2) {
			if (sl_i2c_leader_transfer_non_blocking(
				    &data->i2c_handle, addr, msgs[i].buf, msgs[i].len,
				    msgs[i + 1].buf, msgs[i + 1].len, NULL) != 0) {
				k_sem_give(&data->bus_lock);
				i2c_silabs_pm_policy_state_lock_put();
				return -EIO;
			}
		} else if (msgs[i].flags & I2C_MSG_READ) {
			/* Start DMA receive */
			if (IS_ENABLED(CONFIG_I2C_TARGET)) {
				if (sl_i2c_follower_receive_non_blocking(&data->i2c_handle,
									 msgs[i].buf, msgs[i].len,
									 NULL) != 0) {
					k_sem_give(&data->bus_lock);
					i2c_silabs_pm_policy_state_lock_put();
					return -EIO;
				}
			} else {
				if (sl_i2c_leader_receive_non_blocking(&data->i2c_handle, addr,
								       msgs[i].buf, msgs[i].len,
								       NULL) != 0) {
					k_sem_give(&data->bus_lock);
					i2c_silabs_pm_policy_state_lock_put();
					return -EIO;
				}
			}
		} else {
			/* Start DMA send */
			if (IS_ENABLED(CONFIG_I2C_TARGET)) {
				if (sl_i2c_follower_send_non_blocking(&data->i2c_handle,
								      msgs[i].buf, msgs[i].len,
								      NULL) != 0) {
					k_sem_give(&data->bus_lock);
					i2c_silabs_pm_policy_state_lock_put();
					return -EIO;
				}
			} else {
				if (sl_i2c_leader_send_non_blocking(&data->i2c_handle, addr,
								    msgs[i].buf, msgs[i].len,
								    NULL) != 0) {
					k_sem_give(&data->bus_lock);
					i2c_silabs_pm_policy_state_lock_put();
					return -EIO;
				}
			}
		}
		if (!asynchronous) {
			/* Wait for DMA transfer to complete */
			if (k_sem_take(&data->transfer_sem, K_MSEC(CONFIG_I2C_SILABS_TIMEOUT))) {
				err = -ETIMEDOUT;
			}
			if (data->i2c_handle.state == SL_I2C_STATE_ERROR) {
				err = -EIO;
			}
			k_sem_reset(&data->transfer_sem);
			if (err < 0) {
				i2c_silabs_pm_policy_state_lock_put();
				break;
			}
		}
		i += msgs_in_transfer;
	}

	return err;
}

/* Function to handle synchronous transfer */
static int i2c_silabs_transfer_sync(const struct device *dev, struct i2c_msg *msgs,
				    uint8_t num_msgs, uint16_t addr)
{
	struct i2c_silabs_dev_data *data = dev->data;
	uint8_t i = 0;
	int err = 0;

	/* Get the power management policy state lock */
	i2c_silabs_pm_policy_state_lock_get();

	while (i < num_msgs) {
		uint8_t msgs_in_transfer = 1;

		if ((msgs[i].flags & I2C_MSG_WRITE) == 0 && (i + 1 < num_msgs) &&
		    (msgs[i + 1].flags & I2C_MSG_READ)) {
			msgs_in_transfer = 2;
			if (sl_i2c_leader_transfer_blocking(&data->i2c_handle, addr, msgs[i].buf,
							    msgs[i].len, msgs[i + 1].buf,
							    msgs[i + 1].len,
							    CONFIG_I2C_SILABS_TIMEOUT) != 0) {
				err = -EIO;
				goto out;
			}
			i++;
		} else if (msgs[i].flags & I2C_MSG_READ) {
			if (IS_ENABLED(CONFIG_I2C_TARGET)) {
				if (sl_i2c_follower_receive_blocking(
					    &data->i2c_handle, msgs[i].buf, msgs[i].len,
					    CONFIG_I2C_SILABS_TIMEOUT) != 0) {
					err = -ETIMEDOUT;
					goto out;
				}
			} else {
				if (sl_i2c_leader_receive_blocking(
					    &data->i2c_handle, addr, msgs[i].buf, msgs[i].len,
					    CONFIG_I2C_SILABS_TIMEOUT) != 0) {
					err = -ETIMEDOUT;
					goto out;
				}
			}
		} else {
			if (IS_ENABLED(CONFIG_I2C_TARGET)) {
				if (sl_i2c_follower_send_blocking(&data->i2c_handle, msgs[i].buf,
								  msgs[i].len,
								  CONFIG_I2C_SILABS_TIMEOUT) != 0) {
					err = -ETIMEDOUT;
					goto out;
				}
			} else {
				if (sl_i2c_leader_send_blocking(&data->i2c_handle, addr,
								msgs[i].buf, msgs[i].len,
								CONFIG_I2C_SILABS_TIMEOUT) != 0) {
					err = -ETIMEDOUT;
					goto out;
				}
			}
		}
		i += msgs_in_transfer;
	}

out:
	/* Release the bus lock semaphore */
	k_sem_give(&data->bus_lock);

	/* Release the power management policy state lock */
	i2c_silabs_pm_policy_state_lock_put();

	return err;
}

/* Function to perform I2C transfer */
static int i2c_silabs_transfer_impl(const struct device *dev, struct i2c_msg *msgs,
				    uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
				    void *userdata)
{
	struct i2c_silabs_dev_data *data = dev->data;
	int ret = -EINVAL; /* Initialize ret to a default error value */

	/* Check for invalid number of messages */
	if (!num_msgs) {
		return -EINVAL;
	}

	/* Check and set the address mode (7-bit or 10-bit) based on */
	/* the provided address */
	if (addr <= 0x7F) {
		data->is_10bit_addr = false; /* 7-bit address */
	} else if (addr <= 0x3FF) {
		data->is_10bit_addr = true; /* 10-bit address */
	} else {
		return -EINVAL;
	}

	/* Take the bus lock semaphore to ensure exclusive access */
	ret = k_sem_take(&data->bus_lock, data->asynchronous ? K_NO_WAIT : K_FOREVER);
	if (ret != 0) {
		if (data->asynchronous && ret == -EBUSY) {
			return -EWOULDBLOCK;
		}
		return ret;
	}

	if (IS_ENABLED(CONFIG_I2C_TARGET)) {
		/* Set the follower address */
		if (sl_i2c_set_follower_address(&data->i2c_handle, addr, data->is_10bit_addr) !=
						0) {
			k_sem_give(&data->bus_lock);
			return -EINVAL;
		}
	}

	if (i2c_silabs_is_dma_enabled_instance(dev)) {
		/* DMA transfer handle a/synchronous transfers */
		ret = i2c_silabs_transfer_dma(dev, msgs, num_msgs, addr, cb, userdata,
					      data->asynchronous);

	} else if (!data->asynchronous) {
		/* Polling transfer for synchronous transfers */
		ret = i2c_silabs_transfer_sync(dev, msgs, num_msgs, addr);

	} else {
		/* Asynchronous transfers without DMA is not implemented,
		 * please configure the device tree instance with the proper DMA configuration.
		 */
		ret = -ENOTSUP;
		/* Release the bus lock semaphore */
		k_sem_give(&data->bus_lock);
	}

	return ret;
}

/* Blocking I2C transfer function */
static int i2c_silabs_dev_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr)
{
	struct i2c_silabs_dev_data *data = dev->data;

	data->asynchronous = false;
	return i2c_silabs_transfer_impl(dev, msgs, num_msgs, addr, NULL, NULL);
}

#if defined(CONFIG_I2C_CALLBACK)
/* Non-blocking I2C transfer function with callback */
static int i2c_silabs_dev_transfer_cb(const struct device *dev, struct i2c_msg *msgs,
				      uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
				      void *userdata)
{
	struct i2c_silabs_dev_data *data = dev->data;

	data->asynchronous = true;
	/* Store the callback and context in the data structure */
	data->callback = cb;
	data->callback_context = userdata;
	return i2c_silabs_transfer_impl(dev, msgs, num_msgs, addr, cb, userdata);
}
#endif /* CONFIG_I2C_CALLBACK */

static int i2c_silabs_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_silabs_dev_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:

		/* Enable clock */
		ret = clock_control_on(config->clock, (clock_control_subsys_t)&config->clock_cfg);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}
		/* Apply default pin configuration to resume normal operation */
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}

		break;
	case PM_DEVICE_ACTION_SUSPEND:

		/* Apply low-power pin configuration */
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}

		/* Disable clock */
		ret = clock_control_off(config->clock, (clock_control_subsys_t)&config->clock_cfg);
		if (ret < 0) {
			return ret;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* Function to initialize the I2C peripheral */
static int i2c_silabs_dev_init(const struct device *dev)
{
	__maybe_unused struct i2c_silabs_dev_data *data = dev->data;
	const struct i2c_silabs_dev_config *config = dev->config;
	uint32_t bitrate_cfg;
	int ret;

	/* Enable clock */
	ret = clock_control_on(config->clock, (clock_control_subsys_t)&config->clock_cfg);
	if (ret < 0) {
		return ret;
	}

	/* Apply default pin configuration */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Map the bitrate configuration from device tree */
	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	/* Configure the I2C device with the mapped bitrate configuration */
	if (i2c_silabs_dev_configure(dev, bitrate_cfg) != 0) {
		return -EINVAL;
	}

	if (i2c_silabs_is_dma_enabled_instance(dev)) {
		if (!device_is_ready(data->dma_rx.dma_dev) ||
		    !device_is_ready(data->dma_tx.dma_dev)) {
			return -ENODEV;
		}
		data->dma_rx.dma_channel = dma_request_channel(data->dma_rx.dma_dev, NULL);
		data->i2c_handle.dma_channel.dma_rx_channel = data->dma_rx.dma_channel;

		data->dma_tx.dma_channel = dma_request_channel(data->dma_tx.dma_dev, NULL);
		data->i2c_handle.dma_channel.dma_tx_channel = data->dma_tx.dma_channel;

		if (data->dma_rx.dma_channel < 0 || data->dma_tx.dma_channel < 0) {
			dma_release_channel(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
			dma_release_channel(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
			return -EAGAIN;
		}
	}

	/* Configure IRQ */
	config->irq_config_func();

	return pm_device_driver_init(dev, i2c_silabs_pm_action);
}

/* ISR to dispatch DMA interrupts */
void i2c_silabs_isr_handler(const struct device *dev)
{
	struct i2c_silabs_dev_data *data = dev->data;
	sl_i2c_handle_t *sl_i2c_handle = &data->i2c_handle;

	if (IS_ENABLED(CONFIG_I2C_TARGET)) {
		sli_i2c_follower_dispatch_interrupt(sl_i2c_handle);
	} else {
		sli_i2c_leader_dispatch_interrupt(sl_i2c_handle);
	}
	if (sl_i2c_handle->event != SL_I2C_EVENT_IN_PROGRESS) {
		if (!data->asynchronous) {
			k_sem_give(&data->transfer_sem);
		}
#if defined(CONFIG_I2C_CALLBACK)
		if (data->callback && !data->callback_invoked) {
			int err = 0;

			data->callback_invoked = true;
			if (sl_i2c_handle->event == SL_I2C_EVENT_ARBITRATION_LOST ||
			    sl_i2c_handle->event == SL_I2C_EVENT_BUS_ERROR ||
			    sl_i2c_handle->event == SL_I2C_EVENT_INVALID_ADDR) {
				err = -EIO;
			}
			data->callback(dev, err, data->callback_context);
		}
#endif
		if (data->last_transfer) {
			/* Release the bus lock semaphore */
			k_sem_give(&data->bus_lock);

			if (!data->pm_lock_done) {
				/* Release the power management policy state lock */
				i2c_silabs_pm_policy_state_lock_put();
				data->pm_lock_done = true;
			}
		}
	}
}

/* Store the I2C Driver APIs */
static DEVICE_API(i2c, i2c_silabs_dev_driver_api) = {
	.configure = i2c_silabs_dev_configure,
	.transfer = i2c_silabs_dev_transfer,
#if defined(CONFIG_I2C_CALLBACK)
	.transfer_cb = i2c_silabs_dev_transfer_cb,
#endif /* CONFIG_I2C_CALLBACK */
};

#define I2C_INIT(idx)                                                                              \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
                                                                                                   \
	static void i2c_silabs_irq_config_##idx(void)                                              \
	{                                                                                          \
		COND_CODE_1(CONFIG_I2C_SILABS_DMA,                                                 \
			    (IRQ_CONNECT(DT_INST_IRQ(idx, irq), DT_INST_IRQ(idx, priority),        \
					 i2c_silabs_isr_handler, DEVICE_DT_INST_GET(idx), 0);      \
			     irq_enable(DT_INST_IRQ(idx, irq));),                                  \
			    ())									   \
	}                                                                                          \
	                                                                                           \
	static const uint32_t i2c_bus_clock_##idx = DT_INST_CLOCKS_CELL(idx, enable);              \
	static const sl_peripheral_val_t i2c_peripheral_val_##idx = {                              \
		.base = DT_INST_REG_ADDR(idx),                                                     \
		.clk_branch = DT_INST_CLOCKS_CELL(idx, branch),                                    \
		.bus_clock = (DT_INST_CLOCKS_CELL(idx, enable) ? &i2c_bus_clock_##idx : NULL),     \
	};                                                                                         \
												   \
	static const struct i2c_silabs_dev_config i2c_silabs_dev_config_##idx = {                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.peripheral = &i2c_peripheral_val_##idx,                                           \
		.bitrate = DT_INST_PROP(idx, clock_frequency),                                     \
		.irq_config_func = i2c_silabs_irq_config_##idx,                                    \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                  \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
	};                                                                                         \
                                                                                                   \
	static struct i2c_silabs_dev_data i2c_silabs_dev_data_##idx = {                            \
		.bus_lock = Z_SEM_INITIALIZER(i2c_silabs_dev_data_##idx.bus_lock, 1, 1),           \
		.transfer_sem = Z_SEM_INITIALIZER(i2c_silabs_dev_data_##idx.transfer_sem, 0, 1),   \
		.dma_rx.dma_dev = COND_CODE_1(                                                     \
			CONFIG_I2C_SILABS_DMA,                                                     \
			(DEVICE_DT_GET_OR_NULL(DT_INST_DMAS_CTLR_BY_NAME(idx, rx))), (NULL)),      \
		.dma_tx.dma_dev = COND_CODE_1(                                                     \
			CONFIG_I2C_SILABS_DMA,                                                     \
			(DEVICE_DT_GET_OR_NULL(DT_INST_DMAS_CTLR_BY_NAME(idx, tx))), (NULL)),      \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(idx, i2c_silabs_pm_action);                                       \
	I2C_DEVICE_DT_INST_DEFINE(idx, i2c_silabs_dev_init, PM_DEVICE_DT_INST_GET(idx),            \
				  &i2c_silabs_dev_data_##idx, &i2c_silabs_dev_config_##idx,        \
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,                           \
				  &i2c_silabs_dev_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_INIT)
