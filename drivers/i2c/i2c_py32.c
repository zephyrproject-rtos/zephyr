/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puya_py32_i2c

#include <soc.h>
#include <py32f0xx_ll_i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/py32_clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include "i2c-priv.h"

struct i2c_py32_config {
	I2C_TypeDef *i2c;
	const struct device *clk_dev;
	struct py32_pclken pclken;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pinctrl;
	uint32_t bitrate;
	uint32_t timeout_ms;
};

struct i2c_py32_data {
	struct k_mutex lock;
	uint32_t pclk_rate;
	uint32_t bitrate;
	bool configured;
};

struct i2c_py32_rx_cursor {
	struct i2c_msg *msgs;
	uint8_t message_idx;
	uint32_t offset;
};

static int i2c_py32_wait(const struct i2c_py32_config *cfg, uint32_t mask, int64_t deadline)
{
	for (;;) {
		uint32_t status = cfg->i2c->SR1;

		if ((status & I2C_SR1_ARLO) != 0U) {
			return -EAGAIN;
		}
		if ((status & I2C_SR1_AF) != 0U) {
			return -ENXIO;
		}
		if ((status & (I2C_SR1_BERR | I2C_SR1_OVR)) != 0U) {
			return -EIO;
		}
		if ((status & mask) == mask) {
			return 0;
		}
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}
}

static int i2c_py32_wait_idle(const struct i2c_py32_config *cfg, int64_t deadline)
{
	while (((cfg->i2c->CR1 & I2C_CR1_STOP) != 0U) ||
	       (LL_I2C_IsActiveFlag_BUSY(cfg->i2c) != 0U)) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}
	return 0;
}

static int i2c_py32_apply_config(const struct device *dev)
{
	const struct i2c_py32_config *cfg = dev->config;
	struct i2c_py32_data *data = dev->data;
	int ret = reset_line_toggle_dt(&cfg->reset);

	data->configured = false;
	if (ret < 0) {
		return ret;
	}
	LL_I2C_Disable(cfg->i2c);
	LL_I2C_ConfigSpeed(cfg->i2c, data->pclk_rate, data->bitrate, LL_I2C_DUTYCYCLE_2);
	LL_I2C_SetOwnAddress1(cfg->i2c, 0U, 0U);
	LL_I2C_EnableClockStretching(cfg->i2c);
	LL_I2C_AcknowledgeNextData(cfg->i2c, LL_I2C_ACK);
	LL_I2C_Enable(cfg->i2c);
	data->configured = true;
	return 0;
}

static int i2c_py32_configure(const struct device *dev, uint32_t configuration)
{
	struct i2c_py32_data *data = dev->data;
	uint32_t bitrate;
	int unlock_ret;
	int ret;

	if (((configuration & I2C_MODE_CONTROLLER) == 0U) ||
	    ((configuration & I2C_ADDR_10_BITS) != 0U)) {
		return -ENOTSUP;
	}
	switch (I2C_SPEED_GET(configuration)) {
	case I2C_SPEED_STANDARD:
		bitrate = 100000U;
		break;
	case I2C_SPEED_FAST:
		bitrate = 400000U;
		break;
	default:
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}
	data->bitrate = bitrate;
	ret = i2c_py32_apply_config(dev);
	unlock_ret = k_mutex_unlock(&data->lock);

	return ret < 0 ? ret : unlock_ret;
}

static int i2c_py32_validate(struct i2c_msg *msgs, uint8_t count, uint16_t address)
{
	if (address > 0x7fU) {
		return -ENOTSUP;
	}
	if ((msgs == NULL) || (count == 0U)) {
		return -EINVAL;
	}
	if ((msgs[count - 1U].flags & I2C_MSG_STOP) == 0U) {
		return -ENOTSUP;
	}
	for (uint8_t i = 0U; i < count; i++) {
		if ((msgs[i].flags & ~(I2C_MSG_READ | I2C_MSG_STOP | I2C_MSG_RESTART)) != 0U) {
			return -ENOTSUP;
		}
		if (((msgs[i].len != 0U) && (msgs[i].buf == NULL)) ||
		    (((msgs[i].flags & I2C_MSG_READ) != 0U) && (msgs[i].len == 0U))) {
			return -EINVAL;
		}
		if ((i > 0U) && ((msgs[i - 1U].flags & I2C_MSG_STOP) == 0U) &&
		    ((msgs[i].flags & I2C_MSG_RESTART) == 0U) &&
		    (((msgs[i].flags ^ msgs[i - 1U].flags) & I2C_MSG_READ) != 0U)) {
			return -EINVAL;
		}
	}
	return 0;
}

static int i2c_py32_address(const struct i2c_py32_config *cfg, uint16_t address, bool read,
			    bool restart_pending, int64_t deadline)
{
	int ret;

	if (!restart_pending) {
		LL_I2C_GenerateStartCondition(cfg->i2c);
	}
	ret = i2c_py32_wait(cfg, I2C_SR1_SB, deadline);
	if (ret < 0) {
		return ret;
	}
	LL_I2C_TransmitData8(cfg->i2c, (address << 1U) | (read ? 1U : 0U));
	return i2c_py32_wait(cfg, I2C_SR1_ADDR, deadline);
}

static void i2c_py32_terminate(I2C_TypeDef *i2c, bool stop)
{
	if (stop) {
		LL_I2C_GenerateStopCondition(i2c);
	} else {
		LL_I2C_GenerateStartCondition(i2c);
	}
}

static void i2c_py32_store(struct i2c_py32_rx_cursor *cursor, I2C_TypeDef *i2c)
{
	struct i2c_msg *msg = &cursor->msgs[cursor->message_idx];

	msg->buf[cursor->offset++] = LL_I2C_ReceiveData8(i2c);
	if (cursor->offset == msg->len) {
		cursor->message_idx++;
		cursor->offset = 0U;
	}
}

static int i2c_py32_receive(const struct i2c_py32_config *cfg, struct i2c_msg *msgs,
			    uint32_t remaining, bool stop, int64_t deadline)
{
	struct i2c_py32_rx_cursor cursor = {.msgs = msgs};
	unsigned int key;
	int ret;

	if (remaining == 1U) {
		LL_I2C_AcknowledgeNextData(cfg->i2c, LL_I2C_NACK);
		key = irq_lock();
		LL_I2C_ClearFlag_ADDR(cfg->i2c);
		i2c_py32_terminate(cfg->i2c, stop);
		irq_unlock(key);
		ret = i2c_py32_wait(cfg, I2C_SR1_RXNE, deadline);
		if (ret < 0) {
			return ret;
		}
		i2c_py32_store(&cursor, cfg->i2c);
	} else if (remaining == 2U) {
		LL_I2C_EnableBitPOS(cfg->i2c);
		key = irq_lock();
		LL_I2C_ClearFlag_ADDR(cfg->i2c);
		LL_I2C_AcknowledgeNextData(cfg->i2c, LL_I2C_NACK);
		irq_unlock(key);
		ret = i2c_py32_wait(cfg, I2C_SR1_BTF, deadline);
		if (ret < 0) {
			return ret;
		}
		key = irq_lock();
		i2c_py32_terminate(cfg->i2c, stop);
		i2c_py32_store(&cursor, cfg->i2c);
		i2c_py32_store(&cursor, cfg->i2c);
		irq_unlock(key);
	} else {
		LL_I2C_ClearFlag_ADDR(cfg->i2c);
		while (remaining > 3U) {
			ret = i2c_py32_wait(cfg, I2C_SR1_RXNE, deadline);
			if (ret < 0) {
				return ret;
			}
			i2c_py32_store(&cursor, cfg->i2c);
			remaining--;
		}
		ret = i2c_py32_wait(cfg, I2C_SR1_BTF, deadline);
		if (ret < 0) {
			return ret;
		}
		LL_I2C_AcknowledgeNextData(cfg->i2c, LL_I2C_NACK);
		i2c_py32_store(&cursor, cfg->i2c);
		ret = i2c_py32_wait(cfg, I2C_SR1_BTF, deadline);
		if (ret < 0) {
			return ret;
		}
		key = irq_lock();
		i2c_py32_terminate(cfg->i2c, stop);
		i2c_py32_store(&cursor, cfg->i2c);
		i2c_py32_store(&cursor, cfg->i2c);
		irq_unlock(key);
	}
	LL_I2C_DisableBitPOS(cfg->i2c);
	LL_I2C_AcknowledgeNextData(cfg->i2c, LL_I2C_ACK);
	return 0;
}

static int i2c_py32_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t count,
			     uint16_t address)
{
	const struct i2c_py32_config *cfg = dev->config;
	struct i2c_py32_data *data = dev->data;
	bool held = false;
	bool restart_pending = false;
	int64_t deadline;
	int recovery_ret;
	int unlock_ret;
	int ret = i2c_py32_validate(msgs, count, address);

	if (ret < 0) {
		return ret;
	}
	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}
	deadline = k_uptime_get() + cfg->timeout_ms;
	if (!data->configured) {
		ret = -EIO;
		goto out;
	}
	for (uint8_t i = 0U; i < count; i++) {
		bool read = (msgs[i].flags & I2C_MSG_READ) != 0U;
		bool stop = (msgs[i].flags & I2C_MSG_STOP) != 0U;

		if (!held) {
			ret = i2c_py32_wait_idle(cfg, deadline);
			if (ret < 0) {
				goto recover;
			}
		}
		if (!held || ((msgs[i].flags & I2C_MSG_RESTART) != 0U)) {
			LL_I2C_DisableBitPOS(cfg->i2c);
			LL_I2C_AcknowledgeNextData(cfg->i2c, LL_I2C_ACK);
			ret = i2c_py32_address(cfg, address, read, restart_pending, deadline);
			if (ret < 0) {
				goto recover;
			}
			if (!read) {
				LL_I2C_ClearFlag_ADDR(cfg->i2c);
			}
		}
		if (read) {
			uint8_t first = i;
			uint32_t length = msgs[i].len;

			while (!stop && ((i + 1U) < count) &&
			       ((msgs[i + 1U].flags & I2C_MSG_RESTART) == 0U)) {
				i++;
				if (msgs[i].len > UINT32_MAX - length) {
					ret = -EINVAL;
					goto recover;
				}
				length += msgs[i].len;
				stop = (msgs[i].flags & I2C_MSG_STOP) != 0U;
			}
			ret = i2c_py32_receive(cfg, &msgs[first], length, stop, deadline);
			if (ret < 0) {
				goto recover;
			}
			restart_pending = !stop;
		} else {
			for (uint32_t byte = 0U; byte < msgs[i].len; byte++) {
				ret = i2c_py32_wait(cfg, I2C_SR1_TXE, deadline);
				if (ret < 0) {
					goto recover;
				}
				LL_I2C_TransmitData8(cfg->i2c, msgs[i].buf[byte]);
			}
			if (msgs[i].len != 0U) {
				ret = i2c_py32_wait(cfg, I2C_SR1_BTF, deadline);
				if (ret < 0) {
					goto recover;
				}
			}
			if (stop) {
				LL_I2C_GenerateStopCondition(cfg->i2c);
			}
			restart_pending = false;
		}
		held = !stop;
		if (stop) {
			ret = i2c_py32_wait_idle(cfg, deadline);
			if (ret < 0) {
				goto recover;
			}
		}
	}
	goto out;
recover:
	if (LL_I2C_IsActiveFlag_MSL(cfg->i2c) != 0U) {
		LL_I2C_GenerateStopCondition(cfg->i2c);
	}
	recovery_ret = i2c_py32_apply_config(dev);

	if (recovery_ret < 0) {
		ret = recovery_ret;
	}
out:
	unlock_ret = k_mutex_unlock(&data->lock);

	return ret < 0 ? ret : unlock_ret;
}

static int i2c_py32_init(const struct device *dev)
{
	const struct i2c_py32_config *cfg = dev->config;
	struct i2c_py32_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->clk_dev) || !device_is_ready(cfg->reset.dev)) {
		return -ENODEV;
	}
	ret = k_mutex_init(&data->lock);
	if (ret < 0) {
		return ret;
	}
	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		return ret;
	}
	ret = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->pclken,
				     &data->pclk_rate);
	if (ret < 0) {
		return ret;
	}
	ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	return i2c_py32_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(cfg->bitrate));
}

static DEVICE_API(i2c, i2c_py32_api) = {
	.configure = i2c_py32_configure,
	.transfer = i2c_py32_transfer,
};

#define I2C_PY32_DEFINE(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct i2c_py32_data i2c_py32_data_##inst;                                          \
	static const struct i2c_py32_config i2c_py32_config_##inst = {                             \
		.i2c = (I2C_TypeDef *)DT_INST_REG_ADDR(inst),                                      \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.pclken = {DT_INST_CLOCKS_CELL(inst, bus), DT_INST_CLOCKS_CELL(inst, bits)},       \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                                    \
		.timeout_ms = CONFIG_I2C_PY32_TIMEOUT_MS,                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, i2c_py32_init, NULL, &i2c_py32_data_##inst,                    \
			      &i2c_py32_config_##inst, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,      \
			      &i2c_py32_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_PY32_DEFINE)
