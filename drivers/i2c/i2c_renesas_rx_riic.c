/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_i2c

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <errno.h>
#include <r_riic_rx_if.h>
#include <r_riic_rx_private.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(i2c_renesas_rx, CONFIG_I2C_LOG_LEVEL);

struct i2c_rx_config {
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
	void (*riic_eei_sub)(void);
	void (*riic_txi_sub)(void);
	void (*riic_rxi_sub)(void);
	void (*riic_tei_sub)(void);
};

struct i2c_rx_data {
	riic_info_t rdp_info;
	struct k_sem bus_lock;
	struct k_sem bus_sync;
	uint32_t dev_config;
	i2c_callback_t user_callback;
	void *user_data;
	bool skip_callback;
};

static void riic_eei_isr(const struct device *dev)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	const struct i2c_rx_config *config = dev->config;

	riic_return_t rdp_ret;
	riic_info_t iic_info_m;
	riic_mcu_status_t iic_status;

	iic_info_m.ch_no = data->rdp_info.ch_no;
	rdp_ret = R_RIIC_GetStatus(&iic_info_m, &iic_status);
	if (rdp_ret == RIIC_SUCCESS) {
		if (iic_status.BIT.SP) {
			k_sem_give(&data->bus_sync);
			if ((data->user_callback != NULL) && !data->skip_callback) {
				data->user_callback(dev, 0, data->user_data);
			}
		}
		if (iic_status.BIT.TMO) {
			k_sem_give(&data->bus_sync);
			if ((data->user_callback != NULL) && !data->skip_callback) {
				data->user_callback(dev, -ETIME, data->user_data);
			}
		}
	}

	config->riic_eei_sub();
}
static void riic_rxi_isr(const struct device *dev)
{
	const struct i2c_rx_config *config = dev->config;

	config->riic_rxi_sub();
}

static void riic_txi_isr(const struct device *dev)
{
	const struct i2c_rx_config *config = dev->config;

	config->riic_txi_sub();
}

static void riic_tei_isr(const struct device *dev)
{
	const struct i2c_rx_config *config = dev->config;

	config->riic_tei_sub();
}

static void rdp_callback(void)
{
	/* Do nothing */
}

static void setup_rdp_info(struct i2c_rx_data *data, uint8_t *buf1, size_t len1, uint8_t *buf2,
			   size_t len2, uint16_t *addr)
{
	data->rdp_info.cnt1st = len1;
	data->rdp_info.cnt2nd = len2;
	data->rdp_info.p_data1st = buf1 ? buf1 : FIT_NO_PTR;
	data->rdp_info.p_data2nd = buf2 ? buf2 : FIT_NO_PTR;
	data->rdp_info.p_slv_adr = (uint8_t *)addr;
}

static int run_rx_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t addr, bool async)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	riic_return_t rdp_ret;
	int ret;

	ret = k_sem_take(&data->bus_lock, async ? K_NO_WAIT : K_FOREVER);
	if (ret != 0) {
		return -EBUSY;
	}

	k_sem_reset(&data->bus_sync);

	if (addr == 0x00) {
		/* Enter transmission pattern 4 */
		LOG_DBG("RDP RX I2C master transmit pattern 4\n");
		setup_rdp_info(data, NULL, 0, NULL, 0, NULL);
		rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
		goto transfer_blocking;
	}

	if (num_msgs == 1) {
		if (msgs[0].flags & I2C_MSG_READ) {
			/* Enter master reception pattern 1 */
			LOG_DBG("RDP RX I2C master reception pattern 1\n");
			setup_rdp_info(data, NULL, 0, msgs[0].buf, msgs[0].len, &addr);
			rdp_ret = R_RIIC_MasterReceive(&data->rdp_info);
			goto transfer_blocking;
		} else {
			/* Enter master transmission pattern 2/3 */
			LOG_DBG("RDP RX I2C master transmit pattern 2/3\n");
			setup_rdp_info(data, NULL, 0, msgs[0].len ? msgs[0].buf : NULL, msgs[0].len,
				       &addr);
			rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
			goto transfer_blocking;
		}
	} else if (num_msgs == 2) {
		if (msgs[0].flags & I2C_MSG_READ) {
			goto unsupport_pattern;
		}

		if (msgs[1].flags & I2C_MSG_READ) {
			/* Enter master reception pattern 2 */
			LOG_DBG("RDP RX I2C master reception pattern 2\n");
			setup_rdp_info(data, msgs[0].buf, msgs[0].len, msgs[1].buf, msgs[1].len,
				       &addr);
			rdp_ret = R_RIIC_MasterReceive(&data->rdp_info);
			goto transfer_blocking;
		} else {
			/* Enter master transmission pattern 1 */
			LOG_DBG("RDP RX I2C master transmit pattern 1\n");
			setup_rdp_info(data, msgs[0].buf, msgs[0].len, msgs[1].buf, msgs[1].len,
				       &addr);
			rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
			goto transfer_blocking;
		}
	}

unsupport_pattern:
	/* Unsupport pattern, emit each fragment as a distinct transaction  */
	LOG_DBG("%s: \"Not a generic pattern ...\" !\n", __func__);
	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			/* Enter master reception pattern 1 */
			LOG_DBG("RDP RX I2C master reception pattern 1\n");
			setup_rdp_info(data, NULL, 0, msgs[i].buf, msgs[i].len, &addr);
			rdp_ret = R_RIIC_MasterReceive(&data->rdp_info);
		} else {
			/* Enter master transmission pattern 2 */
			LOG_DBG("RDP RX I2C master transmit pattern 2/3\n");
			setup_rdp_info(data, NULL, 0, (msgs[i].len) ? msgs[i].buf : NULL,
				       msgs[i].len, &addr);
			rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
		}
		if (i == num_msgs - 1) {
			data->skip_callback = false; /* Invoke callback in the last message */
		}
		if (!rdp_ret) {
			k_sem_take(&data->bus_sync, K_FOREVER);
			k_sem_reset(&data->bus_sync);
		} else {
			break;
		}
	}
	goto transfer_exit;

transfer_blocking:
	if (!rdp_ret && !async) {
		data->skip_callback = false; /* Invoke callback */
		k_sem_take(&data->bus_sync, K_FOREVER);
	}
transfer_exit:
	k_sem_give(&data->bus_lock);

	if (!rdp_ret) {
		return 0;
	}
	return -EIO;
}

static int i2c_rx_init(const struct device *dev)
{
	const struct i2c_rx_config *config = dev->config;
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	int ret = 0;

	/* Setup pin control */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Pin control config failed.");
		return ret;
	}

	/* Init kernal object */
	k_sem_init(&data->bus_lock, 1, 1);
	k_sem_init(&data->bus_sync, 0, 1);

	/* Open Device */
	ret = R_RIIC_Open(&data->rdp_info);

	if (ret) {
		LOG_ERR("Open i2c master failed.");
		return -EIO;
	}

	/* Connect and enable Interrupts in zephyr */
	config->irq_config_func(dev);
	return 0;
}

static int i2c_rx_configure(const struct device *dev, uint32_t dev_config)
{
	/* Setup bitrate */
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	uint16_t speed; /* bitrate in kbps */
	int ret;

	/* Validate input */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Only I2C Master mode supported.");
		return -ENOTSUP;
	}
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("Only address 7 bits supported.");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed = 100;
		break;
	case I2C_SPEED_FAST:
		speed = 400;
		break;
	default:
		LOG_ERR("Unsupported speed: %d", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = riic_bps_calc(&data->rdp_info, speed);

	k_sem_give(&data->bus_lock);

	/* Validate result */
	if (ret) {
		return -EINVAL;
	}

	data->dev_config = dev_config;
	return 0;
}

static int i2c_rx_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;

	*dev_config = data->dev_config;
	return 0;
}

static int i2c_rx_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t addr)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;

	data->user_callback = NULL;
	data->user_data = NULL;
	data->skip_callback = true;

	/* Transfer */
	return run_rx_transfer(dev, msgs, num_msgs, addr, false);
}

#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK
static int i2c_rx_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr, i2c_callback_t cb, void *userdata)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;

	data->user_callback = cb;
	data->user_data = userdata;
	data->skip_callback = true;

	/* Transfer */
	return run_rx_transfer(dev, msgs, num_msgs, addr, true);
}
#endif

static int i2c_rx_recover_bus(const struct device *dev)
{
	LOG_DBG("Recover I2C Bus\n");
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	riic_return_t rdp_ret = RIIC_SUCCESS;

	k_sem_take(&data->bus_lock, K_FOREVER);
	rdp_ret |= R_RIIC_Control(&data->rdp_info, RIIC_GEN_START_CON);
	rdp_ret |= R_RIIC_Control(&data->rdp_info, RIIC_GEN_STOP_CON);
	k_sem_give(&data->bus_lock);

	if (rdp_ret != 0) {
		return -EIO;
	}
	return 0;
}

static DEVICE_API(i2c, i2c_rx_driver_api) = {
	.configure = (i2c_api_configure_t)i2c_rx_configure,
	.get_config = (i2c_api_get_config_t)i2c_rx_get_config,
	.transfer = (i2c_api_full_io_t)i2c_rx_transfer,
#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK
	.transfer_cb = (i2c_api_transfer_cb_t)i2c_rx_transfer_cb,
#endif
	.recover_bus = (i2c_api_recover_bus_t)i2c_rx_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_RX_RIIC_INIT(index)                                                                    \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static void i2c_rx_irq_config_func##index(const struct device *dev)                        \
	{                                                                                          \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, eei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, eei, priority), riic_eei_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rxi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, rxi, priority), riic_rxi_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, txi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, txi, priority), riic_txi_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, tei, priority), riic_tei_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, eei, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, rxi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, txi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, tei, irq));                                  \
	};                                                                                         \
                                                                                                   \
	static const struct i2c_rx_config i2c_rx_config_##index = {                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_config_func = i2c_rx_irq_config_func##index,                                  \
		.riic_eei_sub = riic##index##_eei_sub,                                             \
		.riic_rxi_sub = riic##index##_rxi_sub,                                             \
		.riic_txi_sub = riic##index##_txi_sub,                                             \
		.riic_tei_sub = riic##index##_tei_sub,                                             \
	};                                                                                         \
                                                                                                   \
	static struct i2c_rx_data i2c_rx_data_##index = {                                          \
		.rdp_info =                                                                        \
			{                                                                          \
				.dev_sts = RIIC_NO_INIT,                                           \
				.ch_no = DT_INST_PROP(index, channel),                             \
				.callbackfunc = rdp_callback,                                      \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_rx_init, NULL, &i2c_rx_data_##index,                  \
				  &i2c_rx_config_##index, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
				  &i2c_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RX_RIIC_INIT)
