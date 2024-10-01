/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief I2C driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_i2c

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <cyhal_i2c.h>
#include <cyhal_utils_impl.h>
#include <cyhal_utils_impl.h>
#include <cyhal_scb_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_infineon_cat1, CONFIG_I2C_LOG_LEVEL);

#define I2C_CAT1_EVENTS_MASK  (CYHAL_I2C_MASTER_WR_CMPLT_EVENT | CYHAL_I2C_MASTER_RD_CMPLT_EVENT | \
			       CYHAL_I2C_MASTER_ERR_EVENT)

#define I2C_CAT1_SLAVE_EVENTS_MASK                                                                 \
	(CYHAL_I2C_SLAVE_READ_EVENT | CYHAL_I2C_SLAVE_WRITE_EVENT |                                \
	 CYHAL_I2C_SLAVE_RD_BUF_EMPTY_EVENT | CYHAL_I2C_SLAVE_RD_CMPLT_EVENT |                     \
	 CYHAL_I2C_SLAVE_WR_CMPLT_EVENT | CYHAL_I2C_SLAVE_RD_BUF_EMPTY_EVENT |                     \
	 CYHAL_I2C_SLAVE_ERR_EVENT)

/* States for ASYNC operations */
#define CAT1_I2C_PENDING_NONE           (0U)
#define CAT1_I2C_PENDING_RX             (1U)
#define CAT1_I2C_PENDING_TX             (2U)
#define CAT1_I2C_PENDING_TX_RX          (3U)

/* I2C speed */
#define CAT1_I2C_SPEED_STANDARD_HZ      (100000UL)
#define CAT1_I2C_SPEED_FAST_HZ          (400000UL)
#define CAT1_I2C_SPEED_FAST_PLUS_HZ     (1000000UL)

/* Data structure */
struct ifx_cat1_i2c_data {
	cyhal_i2c_t obj;
	cyhal_i2c_cfg_t cfg;
	struct k_sem operation_sem;
	struct k_sem transfer_sem;
	uint32_t error_status;
	uint32_t async_pending;
	cyhal_resource_inst_t hw_resource;
	cyhal_clock_t clock;
	struct i2c_target_config *p_target_config;
	uint8_t i2c_target_wr_byte;
	uint8_t target_wr_buffer[CONFIG_I2C_INFINEON_CAT1_TARGET_BUF];
};

/* Device config structure */
struct ifx_cat1_i2c_config {
	uint32_t master_frequency;
	CySCB_Type *reg_addr;
	const struct pinctrl_dev_config *pcfg;
	uint8_t irq_priority;
};

/* Default SCB/I2C configuration structure */
static const cy_stc_scb_i2c_config_t _cyhal_i2c_default_config = {
	.i2cMode = CY_SCB_I2C_MASTER,
	.useRxFifo = false,
	.useTxFifo = true,
	.slaveAddress = 0U,
	.slaveAddressMask = 0U,
	.acceptAddrInFifo = false,
	.ackGeneralAddr = false,
	.enableWakeFromSleep = false,
	.enableDigitalFilter = false,
	.lowPhaseDutyCycle = 8U,
	.highPhaseDutyCycle = 8U,
};

static int32_t _get_hw_block_num(CySCB_Type *reg_addr)
{
	uint32_t i;

	for (i = 0u; i < _SCB_ARRAY_SIZE; i++) {
		if (_CYHAL_SCB_BASE_ADDRESSES[i] == reg_addr) {
			return i;
		}
	}

	return -ENOMEM;
}

#ifdef CONFIG_I2C_INFINEON_CAT1_ASYNC
static void ifx_master_event_handler(void *callback_arg, cyhal_i2c_event_t event)
{
	const struct device *dev = (const struct device *) callback_arg;
	struct ifx_cat1_i2c_data *data = dev->data;

	if (((CYHAL_I2C_MASTER_ERR_EVENT | CYHAL_I2C_SLAVE_ERR_EVENT) & event) != 0U) {
		/* In case of error abort transfer */
		(void)cyhal_i2c_abort_async(&data->obj);
		data->error_status = 1;
	}

	/* Release semaphore if operation complete
	 * When we have pending TX, RX operations, the semaphore will be released
	 * after TX, RX complete.
	 */
	if (((data->async_pending == CAT1_I2C_PENDING_TX_RX) &&
	     ((CYHAL_I2C_MASTER_RD_CMPLT_EVENT & event) != 0U)) ||
	    (data->async_pending != CAT1_I2C_PENDING_TX_RX)) {

		/* Release semaphore (After I2C async transfer is complete) */
		k_sem_give(&data->transfer_sem);
	}

	if (0 != (CYHAL_I2C_SLAVE_READ_EVENT & event)) {
		if (data->p_target_config->callbacks->read_requested) {
			data->p_target_config->callbacks->read_requested(data->p_target_config,
									 &data->i2c_target_wr_byte);
			data->obj.context.slaveTxBufferIdx = 0;
			data->obj.context.slaveTxBufferCnt = 0;
			data->obj.context.slaveTxBufferSize = 1;
			data->obj.context.slaveTxBuffer = &data->i2c_target_wr_byte;
		}
	}

	if (0 != (CYHAL_I2C_SLAVE_RD_BUF_EMPTY_EVENT & event)) {
		if (data->p_target_config->callbacks->read_processed) {
			data->p_target_config->callbacks->read_processed(data->p_target_config,
									 &data->i2c_target_wr_byte);
			data->obj.context.slaveTxBufferIdx = 0;
			data->obj.context.slaveTxBufferCnt = 0;
			data->obj.context.slaveTxBufferSize = 1;
			data->obj.context.slaveTxBuffer = &data->i2c_target_wr_byte;
		}
	}

	if (0 != (CYHAL_I2C_SLAVE_WRITE_EVENT & event)) {
		cyhal_i2c_slave_config_write_buffer(&data->obj, data->target_wr_buffer,
						    CONFIG_I2C_INFINEON_CAT1_TARGET_BUF);
		if (data->p_target_config->callbacks->write_requested) {
			data->p_target_config->callbacks->write_requested(data->p_target_config);
		}
	}

	if (0 != (CYHAL_I2C_SLAVE_WR_CMPLT_EVENT & event)) {
		if (data->p_target_config->callbacks->write_received) {
			for (int i = 0; i < data->obj.context.slaveRxBufferIdx; i++) {
				data->p_target_config->callbacks->write_received(
					data->p_target_config, data->target_wr_buffer[i]);
			}
		}
		if (data->p_target_config->callbacks->stop) {
			data->p_target_config->callbacks->stop(data->p_target_config);
		}
	}

	if (0 != (CYHAL_I2C_SLAVE_RD_CMPLT_EVENT & event)) {
		if (data->p_target_config->callbacks->stop) {
			data->p_target_config->callbacks->stop(data->p_target_config);
		}
	}
}
#endif

static int ifx_cat1_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	cy_rslt_t rslt;
	int ret;

	if (dev_config != 0) {
		switch (I2C_SPEED_GET(dev_config)) {
		case I2C_SPEED_STANDARD:
			data->cfg.frequencyhal_hz = CAT1_I2C_SPEED_STANDARD_HZ;
			break;
		case I2C_SPEED_FAST:
			data->cfg.frequencyhal_hz = CAT1_I2C_SPEED_FAST_HZ;
			break;
		case I2C_SPEED_FAST_PLUS:
			data->cfg.frequencyhal_hz = CAT1_I2C_SPEED_FAST_PLUS_HZ;
			break;
		default:
			LOG_ERR("Unsupported speed");
			return -ERANGE;
		}

		/* This is deprecated and could be ignored in the future */
		if (dev_config & I2C_ADDR_10_BITS) {
			LOG_ERR("10-bit addressing mode is not supported");
			return -EIO;
		}
	}

	/* Acquire semaphore (block I2C operation for another thread) */
	ret = k_sem_take(&data->operation_sem, K_FOREVER);
	if (ret) {
		return -EIO;
	}

	/* Configure the I2C resource to be master */
	rslt = cyhal_i2c_configure(&data->obj, &data->cfg);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("cyhal_i2c_configure failed with err 0x%x", rslt);
		k_sem_give(&data->operation_sem);
		return -EIO;
	}

#ifdef CONFIG_I2C_INFINEON_CAT1_ASYNC
	/* Register an I2C event callback handler */
	cyhal_i2c_register_callback(&data->obj, ifx_master_event_handler, (void *)dev);
#endif
	/* Release semaphore */
	k_sem_give(&data->operation_sem);
	return 0;
}

static int ifx_cat1_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	uint32_t config;

	switch (data->cfg.frequencyhal_hz) {
	case CAT1_I2C_SPEED_STANDARD_HZ:
		config = I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case CAT1_I2C_SPEED_FAST_HZ:
		config = I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case CAT1_I2C_SPEED_FAST_PLUS_HZ:
		config = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	default:
		LOG_ERR("Unsupported speed");
		return -ERANGE;
	}

	/* Return current configuration */
	*dev_config = config | I2C_MODE_CONTROLLER;

	return 0;
}

static int ifx_cat1_i2c_msg_validate(struct i2c_msg *msg, uint8_t num_msgs)
{
	for (uint32_t i = 0u; i < num_msgs; i++) {
		if ((I2C_MSG_ADDR_10_BITS & msg[i].flags) || (msg[i].buf == NULL)) {
			return -EINVAL;
		}
	}
	return 0;
}

static int ifx_cat1_i2c_transfer(const struct device *dev, struct i2c_msg *msg, uint8_t num_msgs,
				 uint16_t addr)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret;

	if (!num_msgs) {
		return 0;
	}

	/* Acquire semaphore (block I2C transfer for another thread) */
	ret = k_sem_take(&data->operation_sem, K_FOREVER);
	if (ret) {
		return -EIO;
	}

	/* This function checks if msg.buf is not NULL and if
	 * target address is not 10 bit.
	 */
	if (ifx_cat1_i2c_msg_validate(msg, num_msgs) != 0) {
		k_sem_give(&data->operation_sem);
		return -EINVAL;
	}

#ifdef CONFIG_I2C_INFINEON_CAT1_ASYNC
	const struct ifx_cat1_i2c_config *const config = dev->config;

	struct i2c_msg *tx_msg;
	struct i2c_msg *rx_msg;

	data->error_status = 0;
	data->async_pending = CAT1_I2C_PENDING_NONE;

	/* Enable I2C Interrupt */
	cyhal_i2c_enable_event(&data->obj, (cyhal_i2c_event_t)I2C_CAT1_EVENTS_MASK,
			       config->irq_priority, true);

	for (uint32_t i = 0u; i < num_msgs; i++) {
		tx_msg = NULL;
		rx_msg = NULL;

		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			tx_msg = &msg[i];
			data->async_pending = CAT1_I2C_PENDING_TX;
		}

		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			rx_msg = &msg[i];
			data->async_pending = CAT1_I2C_PENDING_TX;
		}

		if ((tx_msg != NULL) && ((i + 1U) < num_msgs) &&
		    ((msg[i + 1U].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ)) {
			rx_msg = &msg[i + 1U];
			i++;
			data->async_pending = CAT1_I2C_PENDING_TX_RX;
		}

		/* Initiate master write and read transfer
		 * using tx_buff and rx_buff respectively
		 */
		rslt = cyhal_i2c_master_transfer_async(&data->obj, addr,
						       (tx_msg == NULL) ? NULL : tx_msg->buf,
						       (tx_msg == NULL) ? 0u : tx_msg->len,
						       (rx_msg == NULL) ? NULL : rx_msg->buf,
						       (rx_msg == NULL) ? 0u : rx_msg->len);

		if (rslt != CY_RSLT_SUCCESS) {
			k_sem_give(&data->operation_sem);
			return -EIO;
		}

		/* Acquire semaphore (block I2C async transfer for another thread) */
		ret = k_sem_take(&data->transfer_sem, K_FOREVER);
		if (ret) {
			k_sem_give(&data->operation_sem);
			return -EIO;
		}

		/* If error_status != 1 we have error during transfer async.
		 * error_status is handling in master_event_handler function.
		 */
		if (data->error_status != 0) {
			/* Release semaphore */
			k_sem_give(&data->operation_sem);
			return -EIO;
		}
	}

	/* Disable I2C Interrupt */
	cyhal_i2c_enable_event(&data->obj, (cyhal_i2c_event_t)
			       I2C_CAT1_EVENTS_MASK, config->irq_priority, false);
#else
	for (uint32_t i = 0u; i < num_msgs; i++) {
		bool stop_flag = ((msg[i].flags & I2C_MSG_STOP) != 0u) ? true : false;

		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			rslt = cyhal_i2c_master_write(&data->obj,
						      addr, msg[i].buf, msg[i].len, 0, stop_flag);
		}
		if ((msg[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			rslt = cyhal_i2c_master_read(&data->obj,
						     addr, msg[i].buf, msg[i].len, 0, stop_flag);
		}

		if (rslt != CY_RSLT_SUCCESS) {
			/* Release semaphore */
			k_sem_give(&data->operation_sem);
			return -EIO;
		}
	}
#endif
	/* Release semaphore (After I2C transfer is complete) */
	k_sem_give(&data->operation_sem);
	return 0;
}

static int ifx_cat1_i2c_init(const struct device *dev)
{
	struct ifx_cat1_i2c_data *data = dev->data;
	const struct ifx_cat1_i2c_config *config = dev->config;
	cy_rslt_t result;
	int ret;

	/* Configuration structure to initialisation I2C */
	cyhal_i2c_configurator_t i2c_init_cfg = {
		.resource = &data->hw_resource,
		.config = &_cyhal_i2c_default_config,
		.clock = &data->clock,
	};

	/* Dedicate SCB HW resource */
	data->hw_resource.type = CYHAL_RSC_SCB;
	data->hw_resource.block_num = _get_hw_block_num(config->reg_addr);


	/* Configure semaphores */
	ret = k_sem_init(&data->transfer_sem, 0, 1);
	if (ret) {
		return ret;
	}

	ret = k_sem_init(&data->operation_sem, 1, 1);
	if (ret) {
		return ret;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Allocating clock for I2C driver */
	result = _cyhal_utils_allocate_clock(&data->clock, &data->hw_resource,
					     CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT, true);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Assigns a programmable divider to a selected IP block */
	en_clk_dst_t clk_idx = _cyhal_scb_get_clock_index(i2c_init_cfg.resource->block_num);

	result = _cyhal_utils_peri_pclk_assign_divider(clk_idx, i2c_init_cfg.clock);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Initialize the I2C peripheral */
	result = cyhal_i2c_init_cfg(&data->obj, &i2c_init_cfg);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}
	data->obj.is_clock_owned = true;

	/* Store Master initial configuration */
	data->cfg.is_slave = false;
	data->cfg.address = 0;
	data->cfg.frequencyhal_hz = config->master_frequency;

	if (ifx_cat1_i2c_configure(dev, 0) != 0) {
		/* Free I2C resource */
		cyhal_i2c_free(&data->obj);
	}
	return 0;
}

static int ifx_cat1_i2c_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_cat1_i2c_data *data = (struct ifx_cat1_i2c_data *)dev->data;
	const struct ifx_cat1_i2c_config *config = dev->config;

	if (!cfg) {
		return -EINVAL;
	}

	if (cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	data->p_target_config = cfg;
	data->cfg.is_slave = true;
	data->cfg.address = data->p_target_config->address;
	data->cfg.frequencyhal_hz = 100000;

	if (ifx_cat1_i2c_configure(dev, I2C_SPEED_SET(I2C_SPEED_FAST)) != 0) {
		/* Free I2C resource */
		cyhal_i2c_free(&data->obj);
		/* Release semaphore */
		k_sem_give(&data->operation_sem);
		return -EIO;
	}

	cyhal_i2c_enable_event(&data->obj, (cyhal_i2c_event_t)I2C_CAT1_SLAVE_EVENTS_MASK,
			       config->irq_priority, true);
	return 0;
}

static int ifx_cat1_i2c_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_cat1_i2c_data *data = (struct ifx_cat1_i2c_data *)dev->data;
	const struct ifx_cat1_i2c_config *config = dev->config;

	/* Acquire semaphore (block I2C operation for another thread) */
	k_sem_take(&data->operation_sem, K_FOREVER);

	cyhal_i2c_free(&data->obj);
	data->p_target_config = NULL;
	cyhal_i2c_enable_event(&data->obj, (cyhal_i2c_event_t)I2C_CAT1_SLAVE_EVENTS_MASK,
			       config->irq_priority, false);

	/* Release semaphore */
	k_sem_give(&data->operation_sem);
	return 0;
}

/* I2C API structure */
static const struct i2c_driver_api i2c_cat1_driver_api = {
	.configure = ifx_cat1_i2c_configure,
	.transfer = ifx_cat1_i2c_transfer,
	.get_config = ifx_cat1_i2c_get_config,
	.target_register = ifx_cat1_i2c_target_register,
	.target_unregister = ifx_cat1_i2c_target_unregister,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* Macros for I2C instance declaration */
#define INFINEON_CAT1_I2C_INIT(n)                                                                  \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct ifx_cat1_i2c_data ifx_cat1_i2c_data##n;                                      \
                                                                                                   \
	static const struct ifx_cat1_i2c_config i2c_cat1_cfg_##n = {                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.master_frequency = DT_INST_PROP_OR(n, clock_frequency, 100000),                   \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),                                     \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, ifx_cat1_i2c_init, NULL, &ifx_cat1_i2c_data##n,               \
				  &i2c_cat1_cfg_##n, POST_KERNEL,                                  \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_cat1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_I2C_INIT)
