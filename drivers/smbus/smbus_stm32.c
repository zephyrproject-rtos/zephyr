/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/stm32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <soc.h>

#include "smbus_utils.h"

LOG_MODULE_REGISTER(stm32_smbus, CONFIG_SMBUS_LOG_LEVEL);

struct smbus_stm32_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *i2c_dev;
};

struct smbus_stm32_data {
	uint32_t config;
	const struct device *dev;
#ifdef CONFIG_SMBUS_STM32_SMBALERT
	sys_slist_t smbalert_callbacks;
	struct k_work smbalert_work;
#endif /* CONFIG_SMBUS_STM32_SMBALERT */
};

#ifdef CONFIG_SMBUS_STM32_SMBALERT
static void smbus_stm32_smbalert_isr(const struct device *dev)
{
	struct smbus_stm32_data *data = dev->data;

	k_work_submit(&data->smbalert_work);
}

static void smbus_stm32_smbalert_work(struct k_work *work)
{
	struct smbus_stm32_data *data = CONTAINER_OF(work, struct smbus_stm32_data, smbalert_work);
	const struct device *dev = data->dev;

	LOG_DBG("%s: got SMB alert", dev->name);

	smbus_loop_alert_devices(dev, &data->smbalert_callbacks);
}

static int smbus_stm32_smbalert_set_cb(const struct device *dev, struct smbus_callback *cb)
{
	struct smbus_stm32_data *data = dev->data;

	return smbus_callback_set(&data->smbalert_callbacks, cb);
}

static int smbus_stm32_smbalert_remove_cb(const struct device *dev, struct smbus_callback *cb)
{
	struct smbus_stm32_data *data = dev->data;

	return smbus_callback_remove(&data->smbalert_callbacks, cb);
}
#endif /* CONFIG_SMBUS_STM32_SMBALERT */

static int smbus_stm32_init(const struct device *dev)
{
	const struct smbus_stm32_config *config = dev->config;
	struct smbus_stm32_data *data = dev->data;
	int result;

	data->dev = dev;

	if (!device_is_ready(config->i2c_dev)) {
		LOG_ERR("%s: I2C device is not ready", dev->name);
		return -ENODEV;
	}

	result = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (result < 0) {
		LOG_ERR("%s: pinctrl setup failed (%d)", dev->name, result);
		return result;
	}

#ifdef CONFIG_SMBUS_STM32_SMBALERT
	k_work_init(&data->smbalert_work, smbus_stm32_smbalert_work);

	i2c_stm32_smbalert_set_callback(config->i2c_dev, smbus_stm32_smbalert_isr, dev);
#endif /* CONFIG_SMBUS_STM32_SMBALERT */

	return 0;
}

static int smbus_stm32_configure(const struct device *dev, uint32_t config_value)
{
	const struct smbus_stm32_config *config = dev->config;
	struct smbus_stm32_data *data = dev->data;

	if (config_value & SMBUS_MODE_HOST_NOTIFY) {
		LOG_ERR("%s: not available", dev->name);
		return -EINVAL;
	}

	if (config_value & SMBUS_MODE_CONTROLLER) {
		LOG_DBG("%s: configuring SMB in host mode", dev->name);
		i2c_stm32_set_smbus_mode(config->i2c_dev, I2CSTM32MODE_SMBUSHOST);
	} else {
		LOG_DBG("%s: configuring SMB in device mode", dev->name);
		i2c_stm32_set_smbus_mode(config->i2c_dev, I2CSTM32MODE_SMBUSDEVICE);
	}

	if (config_value & SMBUS_MODE_SMBALERT) {
		LOG_DBG("%s: activating SMB alert", dev->name);
		i2c_stm32_smbalert_enable(config->i2c_dev);
	} else {
		LOG_DBG("%s: deactivating SMB alert", dev->name);
		i2c_stm32_smbalert_disable(config->i2c_dev);
	}

	data->config = config_value;
	return 0;
}

static int smbus_stm32_get_config(const struct device *dev, uint32_t *config)
{
	struct smbus_stm32_data *data = dev->data;
	*config = data->config;
	return 0;
}

/* Compute PEC for the given messages, including a start byte and a repeated start byte each time
 * the direction changes.
 */
static uint8_t compute_pec(uint8_t target_address, const struct i2c_msg *msgs,
			   uint8_t num_msgs) {

	uint8_t prior_direction;

	uint8_t pec = CRC8_CCITT_INITIAL_VALUE;

	for (uint8_t i = 0; i < num_msgs; i++) {
		/* When direction changes, there is a repeated start byte. */
		uint8_t direction = msgs[i].flags & I2C_MSG_RW_MASK;
		if (i == 0 || direction != prior_direction) {
			prior_direction = direction;
			uint8_t addr_with_rw = target_address | direction;
			pec = crc8_ccitt(pec, &addr_with_rw, sizeof(addr_with_rw));
		}

		pec = crc8_ccitt(pec, msgs[i].buf, msgs[i].len);
	}

	return pec;
}

/* Fix up an array of i2c_msg that are all writes and the last message is a single
 * byte write for the PEC. The last message must be flagged I2C_MSG_WRITE | I2C_MSG_STOP.
 * Returns the actual message count.
 */
static uint8_t prepare_write_pec(const struct device *dev, uint8_t periph_addr,
				 struct i2c_msg *messages, uint8_t num_msgs) {
	const struct smbus_stm32_data *data = dev->data;

	if (data->config & SMBUS_MODE_PEC) {
		/* The last message is just the PEC byte.*/
		uint8_t pec = compute_pec(periph_addr, messages, num_msgs-1);
		messages[num_msgs-1].buf[0] = pec;

		return num_msgs;
	} else {
		/* PEC message is not sent, mark the next-to-last message as last. */
		messages[num_msgs - 2].flags |= I2C_MSG_STOP;
		return num_msgs - 1;
	}
}

/* Fix up an array of i2c_msg assuming that we will read the PEC in the last message.
 * Returns the actual message count.
 */
static uint8_t prepare_read_pec(const struct device *dev,
				struct i2c_msg *messages, uint8_t num_msgs) {
	const struct smbus_stm32_data *data = dev->data;

	if (data->config & SMBUS_MODE_PEC) {
		/* The last message is just the PEC byte.*/
		return num_msgs;
	} else {
		/* PEC message is not sent, mark the next-to-last message as last. */
		messages[num_msgs - 2].flags |= I2C_MSG_STOP;
		return num_msgs - 1;
	}
}

/* After a read transaction, verify that the received PEC is correct. */
static int check_read_pec(const struct device *dev, uint8_t periph_addr,
			  const struct i2c_msg *messages, uint8_t num_msgs) {

	const struct smbus_stm32_data *data = dev->data;
	if (!(data->config & SMBUS_MODE_PEC)) {
		return 0;
	}

	uint8_t reported_pec = messages[num_msgs-1].buf[0];
	uint8_t computed_pec = compute_pec(periph_addr, messages, num_msgs-1);

	return (reported_pec == computed_pec) ? 0 : -EIO;
}

static int smbus_stm32_quick(const struct device *dev, uint16_t periph_addr,
			     enum smbus_direction rw)
{
	const struct smbus_stm32_config *config = dev->config;

	switch (rw) {
	case SMBUS_MSG_WRITE:
		return i2c_write(config->i2c_dev, NULL, 0, periph_addr);
	case SMBUS_MSG_READ:
		return i2c_read(config->i2c_dev, NULL, 0, periph_addr);
	default:
		LOG_ERR("%s: invalid smbus direction %i", dev->name, rw);
		return -EINVAL;
	}
}

static int smbus_stm32_byte_write(const struct device *dev, uint16_t periph_addr, uint8_t command)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE, /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &pec,
			.len = sizeof(pec),
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_write_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));

	return i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);
}

static int smbus_stm32_byte_read(const struct device *dev, uint16_t periph_addr, uint8_t *byte)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t pec = 0;

	struct i2c_msg messages[] = {
		{
			.buf = byte,
			.len = sizeof(*byte),
			.flags = I2C_MSG_READ, /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &pec,
			.len = sizeof(pec),
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_read_pec(dev, messages, ARRAY_SIZE(messages));

	int ret = i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);
	if (ret != 0) return ret;

	return check_read_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));
}

static int smbus_stm32_byte_data_write(const struct device *dev, uint16_t periph_addr,
				       uint8_t command, uint8_t byte)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = &byte,
			.len = sizeof(byte),
			.flags = I2C_MSG_WRITE, /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &pec,
			.len = sizeof(pec),
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_write_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));

	return i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);
}

static int smbus_stm32_byte_data_read(const struct device *dev, uint16_t periph_addr,
				      uint8_t command, uint8_t *byte)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = byte,
			.len = sizeof(*byte),
			.flags = I2C_MSG_READ, /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &pec,
			.len = sizeof(pec),
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_read_pec(dev, messages, ARRAY_SIZE(messages));

	int ret = i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);
	if (ret != 0) return ret;

	return check_read_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));
}

static int smbus_stm32_word_data_write(const struct device *dev, uint16_t periph_addr,
				       uint8_t command, uint16_t word)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = (uint8_t*)&word,
			.len = sizeof(word),
			.flags = I2C_MSG_WRITE, /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &pec,
			.len = sizeof(pec),
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_write_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));

	return i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);
}

static int smbus_stm32_word_data_read(const struct device *dev, uint16_t periph_addr,
				      uint8_t command, uint16_t *word)
{
	uint8_t pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = (uint8_t*)word,
			.len = sizeof(*word),
			.flags = I2C_MSG_READ, /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &pec,
			.len = sizeof(pec),
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_read_pec(dev, messages, ARRAY_SIZE(messages));

	int ret = i2c_transfer(dev, messages, num_msgs, periph_addr);
	if (ret != 0) return ret;

	return check_read_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));
}

static int smbus_stm32_pcall(const struct device *dev, uint16_t periph_addr, uint8_t command,
			     uint16_t send_word, uint16_t *recv_word)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t received_pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = (uint8_t*)&send_word,
			.len = sizeof(send_word),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = (uint8_t*)recv_word,
			.len = sizeof(*recv_word),
			.flags = I2C_MSG_READ /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &received_pec,
			.len = sizeof(received_pec),
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_read_pec(dev, messages, ARRAY_SIZE(messages));

	int res = i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);
	if (res != 0) return res;

	return check_read_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));
}

static int smbus_stm32_block_write(const struct device *dev, uint16_t periph_addr, uint8_t command,
				   uint8_t count, uint8_t *buf)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = &count,
			.len = sizeof(count),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = buf,
			.len = count,
			.flags = I2C_MSG_WRITE, /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &pec,
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		}
	};

	uint8_t num_msgs = prepare_write_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));

	return i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);
}

static int smbus_stm32_block_read(const struct device *dev, uint16_t periph_addr, uint8_t command,
				  uint8_t *count, uint8_t *buf)
{
	const struct smbus_stm32_config *config = dev->config;

	uint8_t received_pec;

	struct i2c_msg messages[] = {
		{
			.buf = &command,
			.len = sizeof(command),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = NULL,	/* will point to next message's len field */
			.len = 1,
			.flags = I2C_MSG_READ | I2C_MSG_RESTART,
		},
		{
			.buf = buf,
			.len = 0,	/* written by previous message! */
			.flags = I2C_MSG_READ /* | I2C_MSG_STOP if !PEC */
		},
		{
			.buf = &received_pec,
			.len = 1,
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		}
	};

	/* Count is read in msg 1 and stored in the len of msg 2.
	 * This works because the STM I2C driver processes each message serially.
	 * The addressing math assumes little-endian.
	 */
	messages[1].buf = (uint8_t*)&messages[2].len;

	uint8_t num_msgs = prepare_read_pec(dev, messages, ARRAY_SIZE(messages));

	int res = i2c_transfer(config->i2c_dev, messages, num_msgs, periph_addr);

	*count = messages[2].len;

	if (res != 0) return res;

	return check_read_pec(dev, periph_addr, messages, ARRAY_SIZE(messages));
}

static int smbus_stm32_cancel(const struct device *dev)
{
	const struct smbus_stm32_config *config = dev->config;
	return i2c_cancel(config->i2c_dev);
}

static int smbus_stm32_uncancel(const struct device *dev)
{
	const struct smbus_stm32_config *config = dev->config;
	return i2c_uncancel(config->i2c_dev);
}

static DEVICE_API(smbus, smbus_stm32_api) = {
	.configure = smbus_stm32_configure,
	.get_config = smbus_stm32_get_config,
	.smbus_quick = smbus_stm32_quick,
	.smbus_byte_write = smbus_stm32_byte_write,
	.smbus_byte_read = smbus_stm32_byte_read,
	.smbus_byte_data_write = smbus_stm32_byte_data_write,
	.smbus_byte_data_read = smbus_stm32_byte_data_read,
	.smbus_word_data_write = smbus_stm32_word_data_write,
	.smbus_word_data_read = smbus_stm32_word_data_read,
	.smbus_pcall = smbus_stm32_pcall,
	.smbus_block_write = smbus_stm32_block_write,
	.smbus_block_read = smbus_stm32_block_read,
	.smbus_cancel = smbus_stm32_cancel,
	.smbus_uncancel = smbus_stm32_uncancel,
#ifdef CONFIG_SMBUS_STM32_SMBALERT
	.smbus_smbalert_set_cb = smbus_stm32_smbalert_set_cb,
	.smbus_smbalert_remove_cb = smbus_stm32_smbalert_remove_cb,
#else
	.smbus_smbalert_set_cb = NULL,
	.smbus_smbalert_remove_cb = NULL,
#endif /* CONFIG_SMBUS_STM32_SMBALERT */
	.smbus_block_pcall = NULL,
	.smbus_host_notify_set_cb = NULL,
	.smbus_host_notify_remove_cb = NULL,
};

#define DT_DRV_COMPAT st_stm32_smbus

#define SMBUS_STM32_DEVICE_INIT(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct smbus_stm32_config smbus_stm32_config_##n = {                                \
		.i2c_dev = DEVICE_DT_GET(DT_INST_PROP(n, i2c)),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	static struct smbus_stm32_data smbus_stm32_data_##n;                                       \
                                                                                                   \
	SMBUS_DEVICE_DT_INST_DEFINE(n, smbus_stm32_init, NULL, &smbus_stm32_data_##n,              \
				    &smbus_stm32_config_##n, POST_KERNEL,                          \
				    CONFIG_SMBUS_INIT_PRIORITY, &smbus_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(SMBUS_STM32_DEVICE_INIT)
