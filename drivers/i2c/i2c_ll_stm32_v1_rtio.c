/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <stm32_ll_i2c.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ll_stm32_rtio);

#include "i2c_ll_stm32.h"
#include "i2c-priv.h"

#define DT_DRV_COMPAT st_stm32_i2c_v1

#define I2C_REQUEST_WRITE       0x00
#define I2C_REQUEST_READ        0x01
#define HEADER                  0xF0


struct i2c_stm32_data_rtio {
	struct i2c_rtio *ctx;
	uint32_t dev_config;
	uint8_t *xfer_buf;
	uint8_t xfer_len;
	uint8_t xfer_flags;
	uint8_t msg_len;
	uint8_t is_restart;
	uint16_t slave_address;
};

static bool i2c_stm32_start(const struct device *dev);

static void i2c_stm32_disable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	LL_I2C_DisableIT_EVT(i2c);
	LL_I2C_DisableIT_BUF(i2c);
}

static void i2c_stm32_enable_transfer_interrupts(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableIT_ERR(i2c);
	LL_I2C_EnableIT_EVT(i2c);
	LL_I2C_EnableIT_BUF(i2c);
}

static void i2c_stm32_generate_start_condition(I2C_TypeDef *i2c)
{
	uint16_t cr1 = LL_I2C_ReadReg(i2c, CR1);

	if (cr1 & I2C_CR1_STOP) {
		LOG_DBG("%s: START while STOP active!", __func__);
		LL_I2C_WriteReg(i2c, CR1, cr1 & ~I2C_CR1_STOP);
	}

	LL_I2C_GenerateStartCondition(i2c);
}

static void i2c_stm32_master_mode_end(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	I2C_TypeDef *i2c = cfg->i2c;

	i2c_stm32_disable_transfer_interrupts(dev);
	LL_I2C_Disable(i2c);
	if (data->xfer_len == 0 && i2c_rtio_complete(ctx, 0)) {
		i2c_stm32_start(dev);
	}

}

static inline void handle_sb(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	uint16_t saddr = data->slave_address;
	uint8_t slave;

	if (data->xfer_flags & I2C_MSG_ADDR_10_BITS) {
		slave = (((saddr & 0x0300) >> 7) & 0xFF);
		slave |= HEADER;

		if (data->is_restart == 0U) {
			data->is_restart = 1U;
		} else {
			slave |= I2C_REQUEST_READ;
			data->is_restart = 0U;
		}
		LL_I2C_TransmitData8(i2c, slave);
	} else {
		slave = (saddr << 1) & 0xFF;
		if (data->xfer_flags & I2C_MSG_READ) {
			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_READ);
			if (data->xfer_len == 2) {
				LL_I2C_EnableBitPOS(i2c);
			}
		} else {
			LL_I2C_TransmitData8(i2c, slave | I2C_REQUEST_WRITE);
		}
	}
}

static inline void handle_addr(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;


	if (data->xfer_flags & I2C_MSG_ADDR_10_BITS) {
		if ((data->xfer_flags & I2C_MSG_READ) && data->is_restart) {
			data->is_restart = 0U;
			LL_I2C_ClearFlag_ADDR(i2c);
			i2c_stm32_generate_start_condition(i2c);
			return;
		}
	}

	if (!(data->xfer_flags & I2C_MSG_READ)) {
		LL_I2C_ClearFlag_ADDR(i2c);
		return;
	}
	/* according to STM32F1 errata we need to handle these corner cases in
	 * specific way.
	 * Please ref to STM32F10xxC/D/E I2C peripheral Errata sheet 2.14.1
	 */
	if (data->xfer_len == 0U && IS_ENABLED(CONFIG_SOC_SERIES_STM32F1X)) {
		LL_I2C_GenerateStopCondition(i2c);
	} else if (data->xfer_len == 1U) {
		/* Single byte reception: enable NACK and clear POS */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
#ifdef CONFIG_SOC_SERIES_STM32F1X
		LL_I2C_ClearFlag_ADDR(i2c);
		LL_I2C_GenerateStopCondition(i2c);
#endif
	} else if (data->xfer_len == 2U) {
#ifdef CONFIG_SOC_SERIES_STM32F1X
		LL_I2C_ClearFlag_ADDR(i2c);
#endif
		/* 2-byte reception: enable NACK and set POS */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		LL_I2C_EnableBitPOS(i2c);
	}
	LL_I2C_ClearFlag_ADDR(i2c);
}

static inline void handle_txe(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (data->xfer_len) {
		data->xfer_len--;
		if (data->xfer_len == 0U) {
			/*
			 * This is the last byte to transmit disable Buffer
			 * interrupt and wait for a BTF interrupt
			 */
			LL_I2C_DisableIT_BUF(i2c);
		}
		LL_I2C_TransmitData8(i2c, *data->xfer_buf);
		data->xfer_buf++;
	} else {
		if (data->xfer_flags & I2C_MSG_STOP) {
			LL_I2C_GenerateStopCondition(i2c);
		}
		if (LL_I2C_IsActiveFlag_BTF(i2c)) {
			/* Read DR to clear BTF flag */
			LL_I2C_ReceiveData8(i2c);
		}
		i2c_stm32_master_mode_end(dev);
	}
}

static inline void handle_rxne(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (data->xfer_len > 0) {
		switch (data->xfer_len) {
		case 1:
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			LL_I2C_DisableBitPOS(i2c);
			/* Single byte reception */
			if (data->xfer_flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}
			LL_I2C_DisableIT_BUF(i2c);
			data->xfer_len--;
			*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
			data->xfer_buf++;
			i2c_stm32_master_mode_end(dev);
			break;
		case 2:
			/*
			 * 2-byte reception for N > 3 has already set the NACK
			 * bit, and must not set the POS bit. See pg. 854 in
			 * the F4 reference manual (RM0090).
			 */
			if (data->msg_len > 2) {
				break;
			}
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			LL_I2C_EnableBitPOS(i2c);
			__fallthrough;
		case 3:
			/*
			 * 2-byte, 3-byte reception and for N-2, N-1,
			 * N when N > 3
			 */
			LL_I2C_DisableIT_BUF(i2c);
			break;
		default:
			/* N byte reception when N > 3 */
			data->xfer_len--;
			*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
			data->xfer_buf++;
		}
	} else {
		if (data->xfer_flags & I2C_MSG_STOP) {
			LL_I2C_GenerateStopCondition(i2c);
		}
		i2c_stm32_master_mode_end(dev);
	}
}

static inline void handle_btf(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (!(data->xfer_flags & I2C_MSG_READ)) {
		handle_txe(dev);
	} else {
		uint32_t counter = 0U;

		switch (data->xfer_len) {
		case 2:
			/*
			 * Stop condition must be generated before reading the
			 * last two bytes.
			 */
			if (data->xfer_flags & I2C_MSG_STOP) {
				LL_I2C_GenerateStopCondition(i2c);
			}

			for (counter = 2U; counter > 0; counter--) {
				data->xfer_len--;
				*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
				data->xfer_buf++;
			}
			i2c_stm32_master_mode_end(dev);
			break;
		case 3:
			/* Set NACK before reading N-2 byte*/
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			data->xfer_len--;
			*data->xfer_buf = LL_I2C_ReceiveData8(i2c);
			data->xfer_buf++;
			break;
		default:
			handle_rxne(dev);
		}
	}
}

void i2c_stm32_event(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_SB(i2c)) {
		handle_sb(dev);
	} else if (LL_I2C_IsActiveFlag_ADD10(i2c)) {
		LL_I2C_TransmitData8(i2c, data->slave_address);
	} else if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		handle_addr(dev);
	} else if (LL_I2C_IsActiveFlag_BTF(i2c)) {
		handle_btf(dev);
	} else if (LL_I2C_IsActiveFlag_TXE(i2c) && (!(data->xfer_flags & I2C_MSG_READ))) {
		handle_txe(dev);
	} else if (LL_I2C_IsActiveFlag_RXNE(i2c) && (data->xfer_flags & I2C_MSG_READ)) {
		handle_rxne(dev);
	}
}

int i2c_stm32_error(const struct device *dev)
{
	const struct i2c_stm32_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->i2c;

	if (LL_I2C_IsActiveFlag_AF(i2c)) {
		LL_I2C_ClearFlag_AF(i2c);
		LL_I2C_GenerateStopCondition(i2c);
		return -EIO;
	}
	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		return -EIO;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		return -EIO;
	}

	return 0;
}

static int i2c_stm32_msg_start(const struct device *dev, uint8_t flags,
	uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	data->xfer_buf = buf;
	data->xfer_len = buf_len;
	data->xfer_flags = flags;
	data->msg_len = buf_len;
	data->is_restart = 0;
	data->slave_address = i2c_addr;

	/* TODO deal with larger than 255 byte transfers correctly */
	if (buf_len > UINT8_MAX) {
		/* TODO LL_I2C_EnableReloadMode(i2c); */
		return -EINVAL;
	}

	LL_I2C_Enable(i2c);

	LL_I2C_DisableBitPOS(i2c);
	LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	i2c_stm32_generate_start_condition(i2c);

	i2c_stm32_enable_transfer_interrupts(dev);
	if (flags & I2C_MSG_READ) {
		LL_I2C_EnableIT_RX(i2c);
	} else {
		LL_I2C_EnableIT_TX(i2c);
	}

	return 0;
}

int i2c_stm32_configure_timing(const struct device *dev, uint32_t clock)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	I2C_TypeDef *i2c = cfg->i2c;

	switch (I2C_SPEED_GET(data->dev_config)) {
	case I2C_SPEED_STANDARD:
		LL_I2C_ConfigSpeed(i2c, clock, 100000, LL_I2C_DUTYCYCLE_2);
		break;
	case I2C_SPEED_FAST:
		LL_I2C_ConfigSpeed(i2c, clock, 400000, LL_I2C_DUTYCYCLE_2);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i2c_stm32_do_configure(const struct device *dev, uint32_t config)
{
	const struct i2c_stm32_config *cfg = dev->config;
	struct i2c_stm32_data_rtio *data = dev->data;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	I2C_TypeDef *i2c = cfg->i2c;
	uint32_t i2c_clock = 0U;
	int ret;

	if (IS_ENABLED(STM32_I2C_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		if (clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken[1],
					   &i2c_clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[1])");
			return -EIO;
		}
	} else {
		if (clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken[0],
					   &i2c_clock) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[0])");
			return -EIO;
		}
	}

	data->dev_config = config;

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("failure Enabling I2C clock");
		return ret;
	}
#endif

	LL_I2C_Disable(i2c);
	ret = i2c_stm32_configure_timing(dev, i2c_clock);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = clock_control_off(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("failure disabling I2C clock");
		return ret;
	}
#endif

	return ret;
}

static bool i2c_stm32_start(const struct device *dev)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;

	int res = 0;

	switch (sqe->op) {
	case RTIO_OP_RX:
		return i2c_stm32_msg_start(dev, I2C_MSG_READ | sqe->iodev_flags,
					   sqe->rx.buf, sqe->rx.buf_len, dt_spec->addr);
	case RTIO_OP_TINY_TX:
		return i2c_stm32_msg_start(dev,  sqe->iodev_flags,
					   sqe->tiny_tx.buf, sqe->tiny_tx.buf_len, dt_spec->addr);
	case RTIO_OP_TX:
		return i2c_stm32_msg_start(dev, sqe->iodev_flags,
					   (uint8_t *)sqe->tx.buf, sqe->tx.buf_len, dt_spec->addr);
	case RTIO_OP_I2C_CONFIGURE:
		res = i2c_stm32_do_configure(dev, sqe->i2c_config);
		return i2c_rtio_complete(data->ctx, res);
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(data->ctx, -EINVAL);
	}
}


static int i2c_stm32_configure(const struct device *dev,
				uint32_t dev_config_raw)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	return i2c_rtio_configure(ctx, dev_config_raw);
}

static int i2c_stm32_transfer(const struct device *dev, struct i2c_msg *msgs,
				   uint8_t num_msgs, uint16_t addr)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	return i2c_rtio_transfer(ctx, msgs, num_msgs, addr);
}


int i2c_stm32_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_stm32_data_rtio *data = dev->data;

	*config = data->dev_config;

	return 0;
}

static void i2c_stm32_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct i2c_stm32_data_rtio *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_submit(ctx, iodev_sqe)) {
		i2c_stm32_start(dev);
	}
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32_configure,
	.transfer = i2c_stm32_transfer,
	.get_config = i2c_stm32_get_config,
	.iodev_submit = i2c_stm32_submit,
};


static int i2c_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct i2c_stm32_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int ret;
	struct i2c_stm32_data_rtio *data = dev->data;

	cfg->irq_config_func(dev);

	i2c_rtio_init(data->ctx, dev);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	i2c_stm32_activate(dev);

	if (IS_ENABLED(STM32_I2C_DOMAIN_CLOCK_SUPPORT) && (cfg->pclk_len > 1)) {
		/* Enable I2C clock source */
		ret = clock_control_configure(clk,
					(clock_control_subsys_t) &cfg->pclken[1],
					NULL);
		if (ret < 0) {
			return -EIO;
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32F1X)
	/*
	 * Force i2c reset for STM32F1 series.
	 * So that they can enter master mode properly.
	 * Issue described in ES096 2.14.7
	 */
	I2C_TypeDef *i2c = cfg->i2c;

	LL_I2C_EnableReset(i2c);
	LL_I2C_DisableReset(i2c);
#endif

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_do_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_enable(dev);
#endif

	return 0;
}

#define STM32_I2C_INIT(index)									\
STM32_I2C_IRQ_HANDLER_DECL(index);								\
												\
PINCTRL_DT_INST_DEFINE(index);									\
												\
static const struct stm32_pclken pclken_##index[] =						\
				 STM32_DT_INST_CLOCKS(index);					\
												\
static const struct i2c_stm32_config i2c_stm32_cfg_##index = {					\
	.i2c = (I2C_TypeDef *)DT_INST_REG_ADDR(index),						\
	.pclken = pclken_##index,								\
	.pclk_len = DT_INST_NUM_CLOCKS(index),							\
	STM32_I2C_IRQ_HANDLER_FUNCTION(index)							\
	.bitrate = DT_INST_PROP(index, clock_frequency),					\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),						\
	IF_ENABLED(CONFIG_I2C_STM32_BUS_RECOVERY,						\
		(.scl =	GPIO_DT_SPEC_INST_GET_OR(index, scl_gpios, {0}),			\
		 .sda = GPIO_DT_SPEC_INST_GET_OR(index, sda_gpios, {0}),))			\
};												\
												\
I2C_RTIO_DEFINE(CONCAT(_i2c, index, _stm32_rtio),						\
	DT_INST_PROP_OR(index, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),				\
	DT_INST_PROP_OR(index, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));				\
												\
static struct i2c_stm32_data_rtio i2c_stm32_dev_data_##index = {				\
	.ctx = &CONCAT(_i2c, index, _stm32_rtio),						\
};												\
												\
PM_DEVICE_DT_INST_DEFINE(index, i2c_stm32_pm_action);						\
												\
I2C_DEVICE_DT_INST_DEFINE(index, i2c_stm32_init,						\
			 PM_DEVICE_DT_INST_GET(index),						\
			 &i2c_stm32_dev_data_##index,						\
			 &i2c_stm32_cfg_##index,						\
			 POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,					\
			 &api_funcs);								\
												\
STM32_I2C_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(STM32_I2C_INIT)
