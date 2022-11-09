/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_nxp_s32, CONFIG_NXP_S32_I3C_LOG_LEVEL);

#include <I3c_Ip.h>

struct nxp_s32_i3c_config {

	uint8_t instance;

	uint32_t functional_clk;

	I3c_Ip_MasterConfigType *i3c_master_cfg;

	const struct pinctrl_dev_config *pincfg;

	/** I3C/I2C device list struct. */
	struct i3c_dev_list device_list;

	/** Interrupt configuration function. */
	void (*irq_config_func)(const struct device *dev);
};

struct nxp_s32_i3c_data {

	/** Controller configuration parameters */
	struct i3c_config_controller ctrl_config;

	/** Address slots */
	struct i3c_addr_slots addr_slots;

	struct k_mutex lock;

	I3c_Ip_TransferConfigType i3c_transfer_cfg;

	uint32_t i3c_od_scl_hz;
};

/**
 * Find a registered I2C target device.
 *
 * Controller only API.
 *
 * This returns the I2C device descriptor of the I2C device
 * matching the device address @p addr.
 *
 * @param dev Pointer to controller device driver instance.
 * @param id I2C target device address.
 *
 * @return @see i3c_i2c_device_find.
 */
static
struct i3c_i2c_device_desc *nxp_s32_i3c_i2c_device_find(const struct device *dev, uint16_t addr)
{
	const struct nxp_s32_i3c_config *config = dev->config;

	return i3c_dev_list_i2c_addr_find(&config->device_list, addr);
}

#ifdef CONFIG_NXP_S32_I3C_INTERRUPT

#define TIMEOUT_MS 1000

static int nxp_s32_i3c_transfer_using_interrupt(uint8_t instance, struct i2c_msg *msg,
					bool read_request, struct nxp_s32_i3c_data *data)
{
	I3c_Ip_StatusType status;

	int64_t timeout, current_time;

	if (read_request) {
		status = I3c_Ip_MasterReceive(instance, msg->buf,
					msg->len, &data->i3c_transfer_cfg);
	} else {
		status = I3c_Ip_MasterSend(instance, msg->buf,
					msg->len, &data->i3c_transfer_cfg);
	}

	if (status) {
		return -EIO;
	}

	timeout = k_uptime_get() + TIMEOUT_MS;

	/*
	 * I3C LL callbacks were not intended to inform that a transfer process
	 * was actually done for transfer next messages. It is encouraged to check
	 * status of current transfer instead.
	 */
	do {
		status = I3c_Ip_MasterGetTransferStatus(instance, NULL);
		current_time = k_uptime_get();
	} while ((status != I3C_IP_STATUS_SUCCESS) &&
			(status != I3C_IP_STATUS_ERROR) && (current_time < timeout));

	if (current_time >= timeout) {
		return -ETIMEDOUT;
	}

	if (status == I3C_IP_STATUS_ERROR) {
		return -EIO;
	}

	return 0;
}
#else
static int nxp_s32_i3c_transfer_polling(uint8_t instance, struct i2c_msg *msg,
					bool read_request, struct nxp_s32_i3c_data *data)
{
	I3c_Ip_StatusType status;

	if (read_request) {
		status = I3c_Ip_MasterReceiveBlocking(instance, msg->buf,
							msg->len, &data->i3c_transfer_cfg);
	} else {
		status = I3c_Ip_MasterSendBlocking(instance, msg->buf,
							msg->len, &data->i3c_transfer_cfg);
	}

	if (status != I3C_IP_STATUS_SUCCESS) {

		if (status == I3C_IP_STATUS_TIMEOUT) {
			return -ETIMEDOUT;
		}
		return -EIO;
	}

	return 0;
}
#endif

/**
 * @brief Transfer messages in I2C mode.
 *
 * @see i3c_i2c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I2C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @return @see i3c_i2c_transfer
 */
static int nxp_s32_i3c_i2c_transfer(const struct device *dev,
					struct i3c_i2c_device_desc *i2c_dev,
					struct i2c_msg *msgs, uint8_t num_msgs)
{
	const struct nxp_s32_i3c_config *config = dev->config;
	struct nxp_s32_i3c_data *data = dev->data;
	struct i2c_msg *current_msg;

	int ret = 0;
	bool read_request;

	k_mutex_lock(&data->lock, K_FOREVER);

	current_msg = msgs;

	data->i3c_transfer_cfg.SlaveAddress = i2c_dev->addr;
	data->i3c_transfer_cfg.BusType = I3C_IP_TRANSFER_BYTES;
	data->i3c_transfer_cfg.BusType = I3C_IP_BUS_TYPE_I2C;

	for (int i = 0; i < num_msgs; i++) {

		if (!!(current_msg->flags & I2C_MSG_ADDR_10_BITS)) {
			ret = -ENOTSUP;
			break;
		}

		/*
		 * No need to check I2C_MSG_RESTART flag because I3c LL driver
		 * always requests a START to be emitted before a transfer.
		 */

		data->i3c_transfer_cfg.SendStop = !!(current_msg->flags & I2C_MSG_STOP);

		if (current_msg->flags & I2C_MSG_READ) {
			read_request = true;
			data->i3c_transfer_cfg.Direction = I3C_IP_READ;
		} else {
			read_request = false;
			data->i3c_transfer_cfg.Direction = I3C_IP_WRITE;
		}

#ifdef CONFIG_NXP_S32_I3C_INTERRUPT
		ret = nxp_s32_i3c_transfer_using_interrupt(config->instance, current_msg,
									read_request, data);
#else
		ret = nxp_s32_i3c_transfer_polling(config->instance, current_msg,
								read_request, data);
#endif

		if (ret) {
			/* A Timeout or Errors occurred */
			break;
		}

		current_msg++;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief Configure I3C hardware.
 *
 * @param dev Pointer to controller device driver instance.
 * @param type Type of configuration parameters being passed
 *             in @p config.
 * @param config Pointer to the configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid configure parameters.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int nxp_s32_i3c_configure(const struct device *dev,
				enum i3c_config_type type, void *config)
{
	const struct nxp_s32_i3c_config *dev_config = dev->config;
	struct nxp_s32_i3c_data *dev_data = dev->data;

	struct i3c_config_controller *ctrl_cfg = config;

	I3c_Ip_MasterBaudRateType i3c_baud_cfg;

	if (type != I3C_CONFIG_CONTROLLER) {
		return -EINVAL;
	}

	/*
	 * Check for valid configuration parameters.
	 *
	 * Currently, must be the primary controller.
	 */
	if ((!ctrl_cfg->is_primary) || (ctrl_cfg->scl.i2c == 0U) || (ctrl_cfg->scl.i3c == 0U)) {
		return -EINVAL;
	}

	i3c_baud_cfg.I2cBaudRate	= ctrl_cfg->scl.i2c;
	i3c_baud_cfg.OpenDrainBaudRate	= dev_data->i3c_od_scl_hz;
	i3c_baud_cfg.PushPullBaudRate	= ctrl_cfg->scl.i3c;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (I3c_Ip_MasterSetBaudRate(dev_config->instance,
					dev_config->functional_clk,
					&i3c_baud_cfg, I3C_IP_BUS_TYPE_I2C)) {

		k_mutex_unlock(&dev_data->lock);
		LOG_ERR("Cannot configure I3C host since the bus is not in idle state\n");
		return -EIO;
	}

#if CONFIG_NXP_S32_I3C_LOG_LEVEL_DBG
	I3c_Ip_MasterGetBaudRate(dev_config->instance, dev_config->functional_clk, &i3c_baud_cfg);

	LOG_DBG("Push-pull baudrate = %d,"
		" Open-drain baudrate = %d, I2C baudrate = %d\n",
		i3c_baud_cfg.PushPullBaudRate,
		i3c_baud_cfg.OpenDrainBaudRate, i3c_baud_cfg.I2cBaudRate);
#endif

	k_mutex_unlock(&dev_data->lock);

	return 0;
}

/**
 * @brief Initialize the hardware.
 *
 * @param dev Pointer to controller device driver instance.
 */
static int nxp_s32_i3c_init(const struct device *dev)
{
	const struct nxp_s32_i3c_config *config = dev->config;
	struct nxp_s32_i3c_data *data = dev->data;
	int ret = 0;

	ret = i3c_addr_slots_init(&data->addr_slots, &config->device_list);
	if (ret != 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	k_mutex_init(&data->lock);

	/* Currently can only act as primary controller. */
	data->ctrl_config.is_primary = true;

	/* HDR mode not supported at the moment. */
	data->ctrl_config.supported_hdr = 0U;

	I3c_Ip_MasterInit(config->instance, config->i3c_master_cfg);

	/* Configuring baudrate according to devicetree properties, the default
	 * value will be used in otherwise.
	 */
	if (data->ctrl_config.scl.i3c == 0U) {
		data->ctrl_config.scl.i3c = KHZ(12500);
	}

	if (data->ctrl_config.scl.i2c == 0U) {
		data->ctrl_config.scl.i2c = KHZ(400);
	}

	if (data->i3c_od_scl_hz == 0U) {
		data->i3c_od_scl_hz = KHZ(2500);
	}

	ret = nxp_s32_i3c_configure(dev, I3C_CONFIG_CONTROLLER, &data->ctrl_config);
	if (ret != 0) {
		return ret;
	}

	/* Configure interrupt */
	config->irq_config_func(dev);

	return 0;
}

static int nxp_s32_i3c_i2c_api_configure(const struct device *dev, uint32_t dev_config)
{
	/* Use i3c_configure API instead */
	return -ENOSYS;
}

static int nxp_s32_i3c_i2c_api_transfer(const struct device *dev,
					struct i2c_msg *msgs,
					uint8_t num_msgs, uint16_t addr)
{
	struct i3c_i2c_device_desc *i2c_dev = nxp_s32_i3c_i2c_device_find(dev, addr);
	int ret;

	if (i2c_dev == NULL) {
		ret = -ENODEV;
	} else {
		ret = nxp_s32_i3c_i2c_transfer(dev, i2c_dev, msgs, num_msgs);
	}

	return ret;
}

void nxp_s32_i3c_master_callback(const struct device *dev, I3c_Ip_MasterEventType event)
{
	const struct nxp_s32_i3c_config *config = dev->config;

	uint32_t merrwarn;

	if (event == I3C_IP_MASTER_EVENT_ERROR) {
		merrwarn = I3c_Ip_MasterGetError(config->instance);
		LOG_ERR("Errors occurred, MERRWARN = 0x%x\n", merrwarn);
	}
}

static const struct i3c_driver_api nxp_s32_i3c_driver_api = {
	.i2c_api.configure = nxp_s32_i3c_i2c_api_configure,
	.i2c_api.transfer = nxp_s32_i3c_i2c_api_transfer,

	.configure = nxp_s32_i3c_configure,
};

#define NXP_S32_I3C_NODE(n)		DT_NODELABEL(i3c##n)

#ifdef CONFIG_NXP_S32_I3C_INTERRUPT
#define NXP_S32_I3C_DECLARE_INTERRUPT(n)						\
	extern void I3c##n##_Isr(void);							\
	static void i3c_s32_config_func_##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_IRQN(NXP_S32_I3C_NODE(n)),				\
		DT_IRQ(NXP_S32_I3C_NODE(n), priority),					\
		I3c##n##_Isr,								\
		DEVICE_DT_GET(NXP_S32_I3C_NODE(n)),					\
		DT_IRQ_BY_IDX(NXP_S32_I3C_NODE(n), 0, flags));				\
		irq_enable(DT_IRQN(NXP_S32_I3C_NODE(n)));				\
	}

#define NXP_S32_I3C_CONFIG_INTERRUPT(n)							\
	.irq_config_func = i3c_s32_config_func_##n
#else
#define NXP_S32_I3C_DECLARE_INTERRUPT(n)
#define NXP_S32_I3C_CONFIG_INTERRUPT(n)
#endif /* CONFIG_NXP_S32_I3C_INTERRUPT */

#define NXP_S32_I3C_DEFINE_CALLBACK(n)							\
	void nxp_s32_i3c_##n##_master_callback(I3c_Ip_MasterEventType Event)		\
	{										\
		const struct device *dev = DEVICE_DT_GET(NXP_S32_I3C_NODE(n));		\
											\
		nxp_s32_i3c_master_callback(dev, Event);				\
	}

#define NXP_S32_I3C_S32_CONFIG(n)							\
	static I3c_Ip_MasterStateType nxp_s32_i3c_##n##_state =				\
	{										\
		0U,									\
		NULL_PTR,								\
		NULL_PTR,								\
		I3C_IP_STATUS_SUCCESS,							\
		{									\
			0x00,								\
			(boolean)FALSE,							\
			I3C_IP_WRITE,							\
			I3C_IP_TRANSFER_BYTES,						\
			I3C_IP_BUS_TYPE_I2C						\
		},									\
		I3C_IP_USING_INTERRUPTS,						\
		(boolean)FALSE,								\
		(I3c_Ip_MasterCallbackType)nxp_s32_i3c_##n##_master_callback,		\
	};										\
											\
	static const I3c_Ip_MasterConfigType nxp_s32_i3c_##n##_config =			\
	{										\
		I3C_IP_MASTER_ON,							\
		(boolean)TRUE,								\
		I3C_IP_MASTER_HIGH_KEEPER_NONE,						\
		0U,									\
		0U,									\
		0U,									\
		0U,									\
		(boolean)FALSE,								\
		0U,									\
		(boolean)FALSE,								\
		(boolean)FALSE,								\
		&nxp_s32_i3c_##n##_state,						\
	};

#define NXP_S32_I3C_INIT_DEVICE(n)							\
	NXP_S32_I3C_DEFINE_CALLBACK(n)							\
	NXP_S32_I3C_DECLARE_INTERRUPT(n)						\
	NXP_S32_I3C_S32_CONFIG(n)							\
	PINCTRL_DT_DEFINE(NXP_S32_I3C_NODE(n));						\
	static struct nxp_s32_i3c_data nxp_s32_i3c_data_##n;				\
	static struct i3c_device_desc s32_i3c_device_array[] =				\
		I3C_DEVICE_ARRAY_DT(NXP_S32_I3C_NODE(n));				\
	static struct i3c_i2c_device_desc s32_i3c_i2c_device_array[] =			\
		I3C_I2C_DEVICE_ARRAY_DT(NXP_S32_I3C_NODE(n));				\
	static const struct nxp_s32_i3c_config nxp_s32_i3c_config_##n = {		\
		.instance	 = n,							\
		.functional_clk  = DT_PROP(NXP_S32_I3C_NODE(n), clock_frequency),	\
		.i3c_master_cfg  = (I3c_Ip_MasterConfigType *)&nxp_s32_i3c_##n##_config,\
		.device_list.i3c = s32_i3c_device_array,				\
		.device_list.num_i3c = ARRAY_SIZE(s32_i3c_device_array),		\
		.device_list.i2c = s32_i3c_i2c_device_array,				\
		.device_list.num_i2c = ARRAY_SIZE(s32_i3c_i2c_device_array),		\
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(NXP_S32_I3C_NODE(n)),		\
		NXP_S32_I3C_CONFIG_INTERRUPT(n)						\
	};										\
	static struct nxp_s32_i3c_data nxp_s32_i3c_data_##n = {				\
		.ctrl_config = {							\
			.scl = {							\
				.i3c = DT_PROP_OR(NXP_S32_I3C_NODE(n), i3c_scl_hz, 0),	\
				.i2c = DT_PROP_OR(NXP_S32_I3C_NODE(n), i2c_scl_hz, 0),	\
			},								\
		},									\
		.i3c_od_scl_hz = DT_PROP_OR(NXP_S32_I3C_NODE(n), i3c_od_scl_hz, 0),	\
	};										\
	DEVICE_DT_DEFINE(NXP_S32_I3C_NODE(n),						\
			nxp_s32_i3c_init,						\
			NULL,								\
			&nxp_s32_i3c_data_##n,						\
			&nxp_s32_i3c_config_##n,					\
			POST_KERNEL,							\
			CONFIG_I3C_CONTROLLER_INIT_PRIORITY,				\
			&nxp_s32_i3c_driver_api);

#if DT_NODE_HAS_STATUS(NXP_S32_I3C_NODE(0), okay)
NXP_S32_I3C_INIT_DEVICE(0)
#endif

#if DT_NODE_HAS_STATUS(NXP_S32_I3C_NODE(1), okay)
NXP_S32_I3C_INIT_DEVICE(1)
#endif

#if DT_NODE_HAS_STATUS(NXP_S32_I3C_NODE(2), okay)
NXP_S32_I3C_INIT_DEVICE(2)
#endif
