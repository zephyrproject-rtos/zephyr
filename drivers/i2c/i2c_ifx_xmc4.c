/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief I2C driver for Infineon XMC MCU family.
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_i2c

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_infineon_xmc4, CONFIG_I2C_LOG_LEVEL);

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>

#include "i2c-priv.h"

#include <xmc_i2c.h>
#include <xmc_usic.h>

#define USIC_IRQ_MIN  84
#define IRQS_PER_USIC 6

#define I2C_XMC_EVENTS_MASK ( \
	XMC_I2C_CH_EVENT_RECEIVE_START | \
	XMC_I2C_CH_EVENT_DATA_LOST | \
	XMC_I2C_CH_EVENT_TRANSMIT_SHIFT | \
	XMC_I2C_CH_EVENT_TRANSMIT_BUFFER | \
	XMC_I2C_CH_EVENT_STANDARD_RECEIVE | \
	XMC_I2C_CH_EVENT_ALTERNATIVE_RECEIVE | \
	XMC_I2C_CH_EVENT_BAUD_RATE_GENERATOR | \
	XMC_I2C_CH_EVENT_START_CONDITION_RECEIVED | \
	XMC_I2C_CH_EVENT_REPEATED_START_CONDITION_RECEIVED | \
	XMC_I2C_CH_EVENT_STOP_CONDITION_RECEIVED | \
	XMC_I2C_CH_EVENT_NACK | \
	XMC_I2C_CH_EVENT_ARBITRATION_LOST | \
	XMC_I2C_CH_EVENT_SLAVE_READ_REQUEST | \
	XMC_I2C_CH_EVENT_ERROR | \
	XMC_I2C_CH_EVENT_ACK)

#define I2C_XMC_STATUS_FLAG_ERROR_MASK ( \
	XMC_I2C_CH_STATUS_FLAG_WRONG_TDF_CODE_FOUND | \
	XMC_I2C_CH_STATUS_FLAG_NACK_RECEIVED | \
	XMC_I2C_CH_STATUS_FLAG_ARBITRATION_LOST | \
	XMC_I2C_CH_STATUS_FLAG_ERROR | \
	XMC_I2C_CH_STATUS_FLAG_DATA_LOST_INDICATION)

/* I2C speed */
#define XMC4_I2C_SPEED_STANDARD     (100000UL)
#define XMC4_I2C_SPEED_FAST         (400000UL)

/* Data structure */
struct ifx_xmc4_i2c_data {
	XMC_I2C_CH_CONFIG_t cfg;
	struct k_sem operation_sem;
	struct k_sem target_sem;
	struct i2c_target_config *p_target_config;
	uint32_t dev_config;
	uint8_t target_wr_byte;
	uint8_t target_wr_buffer[CONFIG_I2C_INFINEON_XMC4_TARGET_BUF];
	bool ignore_slave_select;
	bool is_configured;
};

/* Device config structure */
struct ifx_xmc4_i2c_config {
	XMC_USIC_CH_t *i2c;
	const struct pinctrl_dev_config *pcfg;
	uint8_t scl_src;
	uint8_t sda_src;
	uint32_t bitrate;
	void (*irq_config_func)(const struct device *dev);
};

static int ifx_xmc4_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	struct ifx_xmc4_i2c_data *data = dev->data;
	const struct ifx_xmc4_i2c_config *config = dev->config;

	/* This is deprecated and could be ignored in the future */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("Use I2C_MSG_ADDR_10_BITS instead of I2C_ADDR_10_BITS");
		return -EIO;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->cfg.baudrate = XMC4_I2C_SPEED_STANDARD;
		break;
	case I2C_SPEED_FAST:
		data->cfg.baudrate = XMC4_I2C_SPEED_FAST;
		break;
	default:
		LOG_ERR("Unsupported speed");
		return -ERANGE;
	}

	data->dev_config = dev_config;

	/* Acquire semaphore (block I2C operation for another thread) */
	if (k_sem_take(&data->operation_sem, K_FOREVER)) {
		return -EIO;
	}

	XMC_I2C_CH_Stop(config->i2c);

	/* Configure the I2C resource */
	data->cfg.normal_divider_mode = false;
	XMC_I2C_CH_Init(config->i2c, &data->cfg);
	XMC_I2C_CH_SetInputSource(config->i2c, XMC_I2C_CH_INPUT_SCL, config->scl_src);
	XMC_I2C_CH_SetInputSource(config->i2c, XMC_I2C_CH_INPUT_SDA, config->sda_src);
	if (data->dev_config & I2C_MODE_CONTROLLER) {
		XMC_USIC_CH_SetFractionalDivider(config->i2c,
						 XMC_USIC_CH_BRG_CLOCK_DIVIDER_MODE_FRACTIONAL,
						 1023U);
	} else {
		config->irq_config_func(dev);
	}
	XMC_I2C_CH_Start(config->i2c);
	data->is_configured = true;

	/* Release semaphore */
	k_sem_give(&data->operation_sem);

	return 0;
}

static int ifx_xmc4_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct ifx_xmc4_i2c_data *data = dev->data;
	const struct ifx_xmc4_i2c_config *config = dev->config;

	if (!data->is_configured) {
		/* if not yet configured return configuration that will be used */
		/* when transfer() is called */
		uint32_t bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

		*dev_config = I2C_MODE_CONTROLLER | bitrate_cfg;
		return 0;
	}

	*dev_config = data->dev_config;

	return 0;
}

static int ifx_xmc4_i2c_msg_validate(struct i2c_msg *msg, uint8_t num_msgs)
{
	for (uint32_t i = 0u; i < num_msgs; i++) {
		if ((I2C_MSG_ADDR_10_BITS & msg[i].flags) || (msg[i].buf == NULL)) {
			return -EINVAL;
		}
	}
	return 0;
}

static int ifx_xmc4_i2c_transfer(const struct device *dev, struct i2c_msg *msg, uint8_t num_msgs,
				 uint16_t addr)
{
	struct ifx_xmc4_i2c_data *data = dev->data;
	const struct ifx_xmc4_i2c_config *config = dev->config;
	XMC_I2C_CH_CMD_t cmd_type;

	if (!num_msgs) {
		return 0;
	}

	if (!data->is_configured) {
		int ret;
		uint32_t bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

		ret = ifx_xmc4_i2c_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
		if (ret) {
			return ret;
		}
	}

	/* Acquire semaphore (block I2C transfer for another thread) */
	if (k_sem_take(&data->operation_sem, K_FOREVER)) {
		return -EIO;
	}

	/* This function checks if msg.buf is not NULL and if target address is not 10 bit. */
	if (ifx_xmc4_i2c_msg_validate(msg, num_msgs) != 0) {
		k_sem_give(&data->operation_sem);
		return -EINVAL;
	}

	for (uint32_t msg_index = 0u; msg_index < num_msgs; msg_index++) {
		XMC_I2C_CH_ClearStatusFlag(config->i2c, 0xFFFFFFFF);

		if ((msg_index == 0) || (msg[msg_index].flags & I2C_MSG_RESTART)) {
			/* Send START condition */
			cmd_type = ((msg[msg_index].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) ?
				XMC_I2C_CH_CMD_READ : XMC_I2C_CH_CMD_WRITE;

			if (msg[msg_index].flags & I2C_MSG_RESTART) {
				XMC_I2C_CH_MasterRepeatedStart(config->i2c, addr << 1, cmd_type);
			} else {
				XMC_I2C_CH_MasterStart(config->i2c, addr << 1, cmd_type);
			}

			/* Wait for acknowledge */
			while ((XMC_I2C_CH_GetStatusFlag(config->i2c) &
				XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED) == 0U) {
				/* wait for ACK from slave */
				if (XMC_I2C_CH_GetStatusFlag(config->i2c) &
				    I2C_XMC_STATUS_FLAG_ERROR_MASK) {
					k_sem_give(&data->operation_sem);
					return -EIO;
				}
			}
			XMC_I2C_CH_ClearStatusFlag(config->i2c,
						   XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED);
		}

		for (uint32_t buf_index = 0u; buf_index < msg[msg_index].len; buf_index++) {
			if (cmd_type == XMC_I2C_CH_CMD_WRITE) {
				/* Transmit next command from I2C master to I2C slave */
				XMC_I2C_CH_MasterTransmit(config->i2c,
							  msg[msg_index].buf[buf_index]);

				/* Wait for acknowledge */
				while ((XMC_I2C_CH_GetStatusFlag(config->i2c) &
					XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED) == 0U) {
					/* wait for ACK from slave */
					if (XMC_I2C_CH_GetStatusFlag(config->i2c) &
					    I2C_XMC_STATUS_FLAG_ERROR_MASK) {
						k_sem_give(&data->operation_sem);
						return -EIO;
					}
				}
				XMC_I2C_CH_ClearStatusFlag(config->i2c,
							   XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED);

				/* Wait until TX FIFO is empty */
				while (!XMC_USIC_CH_TXFIFO_IsEmpty(config->i2c)) {
					/* wait until all data is sent by HW */
					if (XMC_I2C_CH_GetStatusFlag(config->i2c) &
					    I2C_XMC_STATUS_FLAG_ERROR_MASK) {
						k_sem_give(&data->operation_sem);
						return -EIO;
					}
				}
			} else {
				if (buf_index == (msg[msg_index].len - 1)) {
					XMC_I2C_CH_MasterReceiveNack(config->i2c);
				} else {
					XMC_I2C_CH_MasterReceiveAck(config->i2c);
				}

				while ((XMC_I2C_CH_GetStatusFlag(config->i2c) &
				       (XMC_I2C_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION |
					XMC_I2C_CH_STATUS_FLAG_RECEIVE_INDICATION)) == 0U) {
					/* wait for data byte from slave */
					if (XMC_I2C_CH_GetStatusFlag(config->i2c) &
					    I2C_XMC_STATUS_FLAG_ERROR_MASK) {
						k_sem_give(&data->operation_sem);
						return -EIO;
					}
				}
				XMC_I2C_CH_ClearStatusFlag(config->i2c,
					XMC_I2C_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION |
					XMC_I2C_CH_STATUS_FLAG_RECEIVE_INDICATION);

				msg[msg_index].buf[buf_index] =
					XMC_I2C_CH_GetReceivedData(config->i2c);
			}
		}

		/* Send STOP condition */
		if (msg[msg_index].flags & I2C_MSG_STOP) {
			XMC_I2C_CH_MasterStop(config->i2c);
		}
	}

	/* Release semaphore (After I2C transfer is complete) */
	k_sem_give(&data->operation_sem);
	return 0;
}

static int ifx_xmc4_i2c_init(const struct device *dev)
{
	struct ifx_xmc4_i2c_data *data = dev->data;
	const struct ifx_xmc4_i2c_config *config = dev->config;
	int ret;

	/* Configure semaphores */
	ret = k_sem_init(&data->operation_sem, 1, 1);
	if (ret) {
		return ret;
	}

	ret = k_sem_init(&data->target_sem, 1, 1);
	if (ret) {
		return ret;
	}

	/* Configure dt provided device signals when available */
	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

static int ifx_xmc4_i2c_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_xmc4_i2c_data *data = dev->data;
	const struct ifx_xmc4_i2c_config *config = dev->config;
	uint32_t bitrate_cfg;

	if (!cfg ||
	    !cfg->callbacks->read_requested ||
	    !cfg->callbacks->read_processed ||
	    !cfg->callbacks->write_requested ||
	    !cfg->callbacks->write_received ||
	    !cfg->callbacks->stop) {
		return -EINVAL;
	}

	if (cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	/* Acquire semaphore (block I2C operation for another thread) */
	if (k_sem_take(&data->target_sem, K_FOREVER)) {
		return -EIO;
	}

	data->p_target_config = cfg;
	data->cfg.address = cfg->address << 1;

	if (data->is_configured) {
		uint32_t bitrate;

		bitrate = I2C_SPEED_GET(data->dev_config);
		bitrate_cfg = i2c_map_dt_bitrate(bitrate);
	} else {
		bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	}

	if (ifx_xmc4_i2c_configure(dev, bitrate_cfg) != 0) {
		/* Release semaphore */
		k_sem_give(&data->target_sem);
		return -EIO;
	}

	k_sem_give(&data->target_sem);
	return 0;
}

static int ifx_xmc4_i2c_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_xmc4_i2c_data *data = dev->data;
	const struct ifx_xmc4_i2c_config *config = dev->config;

	/* Acquire semaphore (block I2C operation for another thread) */
	if (k_sem_take(&data->operation_sem, K_FOREVER)) {
		return -EIO;
	}

	data->p_target_config = NULL;
	XMC_I2C_CH_DisableEvent(config->i2c, I2C_XMC_EVENTS_MASK);

	/* Release semaphore */
	k_sem_give(&data->operation_sem);
	return 0;
}


static void i2c_xmc4_isr(const struct device *dev)
{
	struct ifx_xmc4_i2c_data *data = dev->data;
	const struct ifx_xmc4_i2c_config *config = dev->config;
	const struct i2c_target_callbacks *callbacks = data->p_target_config->callbacks;
	uint32_t status = XMC_I2C_CH_GetStatusFlag(config->i2c);

	while (status) {
		XMC_I2C_CH_ClearStatusFlag(config->i2c, status);

		if (status & XMC_I2C_CH_STATUS_FLAG_STOP_CONDITION_RECEIVED) {
			/* Flush the TX buffer */
			XMC_USIC_CH_SetTransmitBufferStatus(config->i2c,
							    XMC_USIC_CH_TBUF_STATUS_SET_IDLE);

			callbacks->stop(data->p_target_config);
			break;
		}

		if (!data->ignore_slave_select && (status & XMC_I2C_CH_STATUS_FLAG_SLAVE_SELECT)) {
			data->ignore_slave_select = true;

			/* Start a slave read */
			if (status & XMC_I2C_CH_STATUS_FLAG_SLAVE_READ_REQUESTED) {
				callbacks->read_requested(data->p_target_config,
							  &data->target_wr_byte);
				XMC_I2C_CH_SlaveTransmit(config->i2c, data->target_wr_byte);
			} else {
				callbacks->write_requested(data->p_target_config);
			}
		}

		/* Continue a slave read */
		if (status & XMC_I2C_CH_STATUS_FLAG_TRANSMIT_SHIFT_INDICATION) {
			callbacks->read_processed(data->p_target_config, &data->target_wr_byte);
			XMC_I2C_CH_SlaveTransmit(config->i2c, data->target_wr_byte);
		}

		/* Start/Continue a slave write */
		if (status & (XMC_I2C_CH_STATUS_FLAG_RECEIVE_INDICATION |
		    XMC_I2C_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION)) {
			callbacks->write_received(data->p_target_config,
						  XMC_I2C_CH_GetReceivedData(config->i2c));
		}

		if ((status & XMC_I2C_CH_STATUS_FLAG_START_CONDITION_RECEIVED) ||
		    (status & XMC_I2C_CH_STATUS_FLAG_REPEATED_START_CONDITION_RECEIVED)) {
			data->ignore_slave_select = false;
		}

		status = XMC_I2C_CH_GetStatusFlag(config->i2c);
	}
}


/* I2C API structure */
static const struct i2c_driver_api i2c_xmc4_driver_api = {
	.configure = ifx_xmc4_i2c_configure,
	.transfer = ifx_xmc4_i2c_transfer,
	.get_config = ifx_xmc4_i2c_get_config,
	.target_register = ifx_xmc4_i2c_target_register,
	.target_unregister = ifx_xmc4_i2c_target_unregister,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* Macros for I2C instance declaration */
#define XMC4_IRQ_HANDLER_INIT(index)                                                               \
	static void i2c_xmc4_irq_setup_##index(const struct device *dev)                           \
	{                                                                                          \
		const struct ifx_xmc4_i2c_config *config = dev->config;                            \
		uint8_t irq_num = DT_INST_IRQN(index);                                             \
		uint8_t service_request = (irq_num - USIC_IRQ_MIN) % IRQS_PER_USIC;                \
												   \
		XMC_I2C_CH_SelectInterruptNodePointer(config->i2c,                                 \
			XMC_I2C_CH_INTERRUPT_NODE_POINTER_RECEIVE, service_request);               \
		XMC_I2C_CH_SelectInterruptNodePointer(config->i2c,                                 \
			XMC_I2C_CH_INTERRUPT_NODE_POINTER_ALTERNATE_RECEIVE, service_request);     \
												   \
		XMC_I2C_CH_EnableEvent(config->i2c, I2C_XMC_EVENTS_MASK);                          \
												   \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2c_xmc4_isr,       \
			    DEVICE_DT_INST_GET(index), 0);                                         \
												   \
		irq_enable(irq_num);                                                               \
	}

#define XMC4_IRQ_HANDLER_STRUCT_INIT(index) .irq_config_func = i2c_xmc4_irq_setup_##index

#define INFINEON_XMC4_I2C_INIT(n)                                                                  \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	XMC4_IRQ_HANDLER_INIT(n)                                                                   \
                                                                                                   \
	static struct ifx_xmc4_i2c_data ifx_xmc4_i2c_data##n;                                      \
                                                                                                   \
	static const struct ifx_xmc4_i2c_config i2c_xmc4_cfg_##n = {                               \
		.i2c = (XMC_USIC_CH_t *)DT_INST_REG_ADDR(n),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.scl_src = DT_INST_ENUM_IDX(n, scl_src),                                           \
		.sda_src = DT_INST_ENUM_IDX(n, sda_src),                                           \
		.bitrate = DT_INST_PROP_OR(n, clock_frequency, I2C_SPEED_STANDARD),                \
		XMC4_IRQ_HANDLER_STRUCT_INIT(n)                                                    \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, ifx_xmc4_i2c_init, NULL, &ifx_xmc4_i2c_data##n,               \
				  &i2c_xmc4_cfg_##n, POST_KERNEL,                                  \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_xmc4_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_XMC4_I2C_INIT)
