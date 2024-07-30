/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_nxp_s32_i3c);

#include <I3c_Ip.h>
#include <I3c_Ip_Irq.h>

#include "i2c-priv.h"

#define DT_DRV_COMPAT nxp_s32_i2c_i3c

#if CONFIG_I2C_NXP_S32_I3C_TRANSFER_TIMEOUT
#define I3C_NXP_S32_TIMEOUT_MS	K_MSEC(CONFIG_I2C_NXP_S32_I3C_TRANSFER_TIMEOUT)
#else
#define I3C_NXP_S32_TIMEOUT_MS	K_FOREVER
#endif

struct i3c_nxp_s32_config {
	uint8_t instance;
	uint8_t num_baudrate;
	uint32_t bitrate;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;

	I3c_Ip_MasterConfigType *master_cfg;

	I3c_Ip_MasterBaudRateType *baudrate_cfg;

	void (*irq_config_func)(const struct device *dev);
};

struct i3c_nxp_s32_data {
	uint32_t functional_clk;
	uint32_t curr_config;
	struct k_sem lock;
	struct k_sem transfer_done;
	I3c_Ip_TransferConfigType transfer_cfg;

#ifdef CONFIG_I2C_CALLBACK
	uint32_t num_msgs;
	uint32_t msg;
	struct i2c_msg *msgs;
	i2c_callback_t callback;
	void *userdata;
#endif
};

static int i3c_nxp_s32_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i3c_nxp_s32_config *config = dev->config;
	struct i3c_nxp_s32_data *data = dev->data;

	uint32_t idx;
	uint32_t i2c_bus_speed;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Target mode is not supported\n");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_bus_speed = I2C_BITRATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		i2c_bus_speed = I2C_BITRATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		i2c_bus_speed = I2C_BITRATE_FAST_PLUS;
		break;
	case I2C_SPEED_DT:
		i2c_bus_speed = config->bitrate;
		break;
	case I2C_SPEED_HIGH:
		__fallthrough;
	case I2C_SPEED_ULTRA:
		__fallthrough;
	default:
		return -ENOTSUP;
	}

	for (idx = 0U; idx < config->num_baudrate; idx++) {
		if (config->baudrate_cfg[idx].I2cBaudRate == i2c_bus_speed) {
			break;
		}
	}

	if (idx == config->num_baudrate) {
		LOG_ERR("Missing baudrate configuration for I2C speed %d\n", i2c_bus_speed);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (I3c_Ip_MasterSetBaudRate(config->instance, data->functional_clk,
				     &config->baudrate_cfg[idx], I3C_IP_BUS_TYPE_I2C)) {
		LOG_ERR("Cannot configure baudrate as the controller is not in idle state\n");
		k_sem_give(&data->lock);
		return -EBUSY;
	}

#if (LOG_LEVEL == LOG_LEVEL_DBG)
	I3c_Ip_MasterBaudRateType baudrate_cfg;

	I3c_Ip_MasterGetBaudRate(config->instance, data->functional_clk, &baudrate_cfg);

	LOG_DBG("Push-pull baudrate = %d, Open-drain baudrate = %d, I2C baudrate = %d\n",
		baudrate_cfg.PushPullBaudRate, baudrate_cfg.OpenDrainBaudRate,
		baudrate_cfg.I2cBaudRate);
#endif

	data->curr_config = dev_config;

	k_sem_give(&data->lock);

	return 0;
}

static int i3c_nxp_s32_configure_get(const struct device *dev, uint32_t *dev_config)
{
	struct i3c_nxp_s32_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);

	*dev_config = data->curr_config;

	k_sem_give(&data->lock);

	return 0;
}

static int i3c_nxp_s32_transfer_one_msg(const struct device *dev, struct i2c_msg *msg)
{
	const struct i3c_nxp_s32_config *config = dev->config;
	struct i3c_nxp_s32_data *data = dev->data;

	I3c_Ip_StatusType status;

	if (!!(msg->flags & I2C_MSG_ADDR_10_BITS)) {
		return -ENOTSUP;
	}

	data->transfer_cfg.SendStop = ((msg->flags & I2C_MSG_STOP) ? true : false);
	data->transfer_cfg.Direction = ((msg->flags & I2C_MSG_READ) ? I3C_IP_READ : I3C_IP_WRITE);

	if (data->transfer_cfg.Direction == I3C_IP_READ) {
		status = I3c_Ip_MasterReceive(config->instance, msg->buf,
					      msg->len, &data->transfer_cfg);
	} else {
		status = I3c_Ip_MasterSend(config->instance, msg->buf,
					   msg->len, &data->transfer_cfg);
	}

	return (status == I3C_IP_STATUS_SUCCESS) ? 0 : -EIO;
}

static int i3c_nxp_s32_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t addr)
{
	const struct i3c_nxp_s32_config *config = dev->config;
	struct i3c_nxp_s32_data *data = dev->data;

	struct i2c_msg *current_msg;

	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	current_msg = msgs;
	data->transfer_cfg.SlaveAddress = addr;

	for (int i = 0; i < num_msgs; i++) {

		ret = i3c_nxp_s32_transfer_one_msg(dev, current_msg);
		if (ret) {
			break;
		}

		ret = k_sem_take(&data->transfer_done, I3C_NXP_S32_TIMEOUT_MS);
		if (ret) {
			break;
		}

		if (I3c_Ip_MasterGetTransferStatus(config->instance, NULL) ==
		    I3C_IP_STATUS_ERROR) {
			ret = -EIO;
			break;
		}

		current_msg++;
	}

	k_sem_give(&data->lock);

	return ret;
}

#ifdef CONFIG_I2C_CALLBACK
static void i3c_nxp_s32_transfer_async_done(const struct device *dev, int result)
{
	struct i3c_nxp_s32_data *data = dev->data;
	i2c_callback_t callback = data->callback;
	void *userdata = data->userdata;

	k_sem_give(&data->lock);

	callback(dev, result, userdata);
}

static int i3c_nxp_s32_transfer_async(const struct device *dev, struct i2c_msg *msgs,
				      uint8_t num_msgs, uint16_t addr,
				      i2c_callback_t cb, void *userdata)
{
	struct i3c_nxp_s32_data *data = dev->data;

	if (!cb) {
		LOG_ERR("Callback must be added for I2C async");
		return -EINVAL;
	}

	if (k_sem_take(&data->lock, K_NO_WAIT)) {
		return -EWOULDBLOCK;
	}

	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->msg = 0U;
	data->callback = cb;
	data->userdata = userdata;

	data->transfer_cfg.SlaveAddress = addr;

	/* Transfer the first message */
	if (i3c_nxp_s32_transfer_one_msg(dev, &data->msgs[data->msg++])) {
		i3c_nxp_s32_transfer_async_done(dev, -EIO);
		return -EIO;
	}

	return 0;
}
#endif

void i3c_nxp_s32_master_isr(const struct device *dev)
{
	const struct i3c_nxp_s32_config *config = dev->config;
	struct i3c_nxp_s32_data *data = dev->data;

	I3c_Ip_StatusType status;

	I3c_Ip_IRQHandler(config->instance);

	status = I3c_Ip_MasterGetTransferStatus(config->instance, NULL);

	if ((status == I3C_IP_STATUS_SUCCESS) || (status == I3C_IP_STATUS_ERROR)) {
#ifdef CONFIG_I2C_CALLBACK
		if (data->callback) {
			if (status == I3C_IP_STATUS_ERROR) {
				i3c_nxp_s32_transfer_async_done(dev, -EIO);
			} else if (data->msg == data->num_msgs) {
				i3c_nxp_s32_transfer_async_done(dev, 0);
			} else {
				if (i3c_nxp_s32_transfer_one_msg(dev, &data->msgs[data->msg++])) {
					i3c_nxp_s32_transfer_async_done(dev, -EIO);
				}
			}
		} else
#endif
		{
			k_sem_give(&data->transfer_done);
		}
	}
}

static int i3c_nxp_s32_init(const struct device *dev)
{
	const struct i3c_nxp_s32_config *config = dev->config;
	struct i3c_nxp_s32_data *data = dev->data;

	uint32_t bitrate_cfg;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     config->clock_subsys,
				     &data->functional_clk);
	if (ret != 0) {
		LOG_ERR("Failed to get I3C functional clock frequency");
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to configure I3C pins");
		return ret;
	}

	I3c_Ip_MasterInit(config->instance, config->master_cfg);

	k_sem_init(&data->lock, 1U, 1U);
	k_sem_init(&data->transfer_done, 0U, 1U);

	config->irq_config_func(dev);

	if (config->bitrate == I2C_BITRATE_STANDARD ||
		config->bitrate == I2C_BITRATE_FAST ||
		config->bitrate == I2C_BITRATE_FAST_PLUS) {
		bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	} else {
		bitrate_cfg = I2C_SPEED_DT << I2C_SPEED_SHIFT;
	}

	ret = i3c_nxp_s32_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2C bitrate");
		return ret;
	}

	return 0;
}

static const struct i2c_driver_api i3c_nxp_s32_driver_api = {
	.configure = i3c_nxp_s32_configure,
	.transfer = i3c_nxp_s32_transfer,
	.get_config = i3c_nxp_s32_configure_get,

#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i3c_nxp_s32_transfer_async
#endif
};

#define I3C_NXP_S32_DECLARE_INTERRUPT(n)						\
	static void i2c_s32_config_func_##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
		DT_INST_IRQ(n, priority),						\
		i3c_nxp_s32_master_isr,							\
		DEVICE_DT_INST_GET(n),							\
		DT_INST_IRQ(n, flags));							\
		irq_enable(DT_INST_IRQN(n));						\
	}

#define I3C_NXP_S32_CONFIG_INTERRUPT(n)							\
	.irq_config_func = i2c_s32_config_func_##n

#define I3C_NXP_S32_INSTANCE_CHECK(indx, n)						\
	((DT_INST_REG_ADDR(n) == IP_I3C_##indx##_BASE) ? indx : 0)

#define I3C_NXP_S32_GET_INSTANCE(n)							\
	LISTIFY(__DEBRACKET I3C_INSTANCE_COUNT, I3C_NXP_S32_INSTANCE_CHECK, (|), n)

#define I3C_NXP_S32_CONFIG(n)								\
	static I3c_Ip_MasterStateType i3c_nxp_s32_state_##n =				\
	{										\
		.BufferSize = 0U,							\
		.TxDataBuffer = NULL_PTR,						\
		.RxDataBuffer = NULL_PTR,						\
		.Status = I3C_IP_STATUS_SUCCESS,					\
		.TransferOption =							\
			{								\
				.SlaveAddress = 0U,					\
				.SendStop = FALSE,					\
				.Direction = I3C_IP_WRITE,				\
				.TransferSize = I3C_IP_TRANSFER_BYTES,			\
				.BusType = I3C_IP_BUS_TYPE_I2C				\
			},								\
		.TransferType = I3C_IP_USING_INTERRUPTS,				\
		.OpenDrainStop = FALSE,							\
		.MasterCallback = NULL_PTR,						\
		.MasterCallbackParam = 0,						\
	};										\
											\
	static const I3c_Ip_MasterConfigType i3c_nxp_s32_master_config##n =		\
	{										\
		.MasterEnable = I3C_IP_MASTER_ON,					\
		.DisableTimeout = DT_INST_PROP(n, disable_timeout),			\
		.I2cBaud = 11U,								\
		.OpenDrainBaud = 8U,							\
		.PushPullBaud = 10U,							\
		.PushPullLow = 0U,							\
		.OpenDrainHighPP = FALSE,						\
		.Skew = 0U,								\
		.MasterState = &i3c_nxp_s32_state_##n,					\
	};

#define I3C_NXP_S32_BAUDRATE(n)								\
	static uint32_t i3c_nxp_s32_baud_cfg_##n[] = DT_INST_PROP(n, baudrate_cfg);

#define I3C_NXP_S32_INIT_DEVICE(n)							\
	I3C_NXP_S32_DECLARE_INTERRUPT(n)						\
	I3C_NXP_S32_CONFIG(n)								\
	I3C_NXP_S32_BAUDRATE(n)								\
	PINCTRL_DT_INST_DEFINE(n);							\
	static struct i3c_nxp_s32_data i3c_nxp_s32_data##n = {				\
		.transfer_cfg = {							\
			.TransferSize = I3C_IP_TRANSFER_BYTES,				\
			.BusType = I3C_IP_BUS_TYPE_I2C,					\
		},									\
	};										\
	static const struct i3c_nxp_s32_config i3c_nxp_s32_config##n = {		\
		.instance = I3C_NXP_S32_GET_INSTANCE(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.master_cfg  = (I3c_Ip_MasterConfigType *)&i3c_nxp_s32_master_config##n,\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.baudrate_cfg = (I3c_Ip_MasterBaudRateType *)i3c_nxp_s32_baud_cfg_##n,	\
		.num_baudrate = ARRAY_SIZE(i3c_nxp_s32_baud_cfg_##n) / 3U,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),				\
		I3C_NXP_S32_CONFIG_INTERRUPT(n)						\
	};										\
	DEVICE_DT_INST_DEFINE(n,							\
			i3c_nxp_s32_init,						\
			NULL,								\
			&i3c_nxp_s32_data##n,						\
			&i3c_nxp_s32_config##n,						\
			POST_KERNEL,							\
			CONFIG_I2C_INIT_PRIORITY,					\
			&i3c_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I3C_NXP_S32_INIT_DEVICE)
