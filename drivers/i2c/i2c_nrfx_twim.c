/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/i2c_nrfx_twim.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_twim.h>
#include <zephyr/sys/util.h>
#include <zephyr/linker/devicetree_regions.h>
#include <dmm.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include "i2c_nrfx_twim_common.h"

LOG_MODULE_REGISTER(i2c_nrfx_twim, CONFIG_I2C_LOG_LEVEL);

#if CONFIG_I2C_NRFX_TRANSFER_TIMEOUT
#define I2C_TRANSFER_TIMEOUT_MSEC K_MSEC(CONFIG_I2C_NRFX_TRANSFER_TIMEOUT)
#else
#define I2C_TRANSFER_TIMEOUT_MSEC K_FOREVER
#endif

struct i2c_nrfx_twim_data {
	nrfx_twim_t twim;
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	volatile int res;
	uint8_t *buf_ptr;
};

int i2c_nrfx_twim_exclusive_access_acquire(const struct device *dev, k_timeout_t timeout)
{
	struct i2c_nrfx_twim_data *dev_data = dev->data;
	int ret;

	ret = k_sem_take(&dev_data->transfer_sync, timeout);

	if (ret == 0) {
		(void)pm_device_runtime_get(dev);
	}

	return ret;
}

void i2c_nrfx_twim_exclusive_access_release(const struct device *dev)
{
	struct i2c_nrfx_twim_data *dev_data = dev->data;

	(void)pm_device_runtime_put(dev);

	k_sem_give(&dev_data->transfer_sync);
}

static int i2c_nrfx_twim_transfer(const struct device *dev,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs, uint16_t addr)
{
	struct i2c_nrfx_twim_data *dev_data = dev->data;
	const struct i2c_nrfx_twim_common_config *dev_config = dev->config;
	int ret = 0;
	uint8_t *msg_buf = dev_config->msg_buf;
	uint16_t msg_buf_used = 0;
	uint16_t msg_buf_size = dev_config->msg_buf_size;
	uint8_t *buf;
	uint16_t buf_len;
	uint8_t *dma_buf;

	(void)i2c_nrfx_twim_exclusive_access_acquire(dev, K_FOREVER);

	/* Dummy take on completion_sync sem to be sure that it is empty */
	k_sem_take(&dev_data->completion_sync, K_NO_WAIT);

	for (size_t i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs[i].flags) {
			ret = -ENOTSUP;
			break;
		}

		bool dma_accessible = nrf_dma_accessible_check(&dev_data->twim, msgs[i].buf);

		/* This fragment needs to be merged with the next one if:
		 * - it is not the last fragment
		 * - it does not end a bus transaction
		 * - the next fragment does not start a bus transaction
		 * - the direction of the next fragment is the same as this one
		 */
		bool concat_next = ((i + 1) < num_msgs)
				&& !(msgs[i].flags & I2C_MSG_STOP)
				&& !(msgs[i + 1].flags & I2C_MSG_RESTART)
				&& ((msgs[i].flags & I2C_MSG_READ)
				    == (msgs[i + 1].flags & I2C_MSG_READ));

		/* If we need to concatenate the next message, or we've
		 * already committed to concatenate this message, or its buffer
		 * is not accessible by DMA, add it to the internal driver
		 * buffer after verifying there's room.
		 */
		if (concat_next || (msg_buf_used != 0) || !dma_accessible) {
			if ((msg_buf_used + msgs[i].len) > msg_buf_size) {
				LOG_ERR("Need to use the internal driver "
					"buffer but its size is insufficient "
					"(%u + %u > %u). "
					"Adjust the zephyr,concat-buf-size or "
					"zephyr,flash-buf-max-size property "
					"(the one with greater value) in the "
					"\"%s\" node.",
					msg_buf_used, msgs[i].len,
					msg_buf_size, dev->name);
				ret = -ENOSPC;
				break;
			}
			if (!(msgs[i].flags & I2C_MSG_READ)) {
				memcpy(msg_buf + msg_buf_used,
				       msgs[i].buf,
				       msgs[i].len);
			}
			msg_buf_used += msgs[i].len;
		}

		if (concat_next) {
			continue;
		}

		if (msg_buf_used == 0) {
			buf = msgs[i].buf;
			buf_len = msgs[i].len;
		} else {
			buf = msg_buf;
			buf_len = msg_buf_used;
		}

		if (msgs[i].flags & I2C_MSG_READ) {
			ret = dmm_buffer_in_prepare(dev_config->mem_reg, buf, buf_len,
							(void **)&dma_buf);
		} else {
			ret = dmm_buffer_out_prepare(dev_config->mem_reg, buf, buf_len,
							(void **)&dma_buf);
		}

		if (ret < 0) {
			LOG_ERR("Failed to prepare buffer: %d", ret);
			return ret;
		}

		dev_data->buf_ptr = buf;

		ret = i2c_nrfx_twim_msg_transfer(dev, msgs[i].flags, dma_buf, buf_len, addr);
		if (ret < 0) {
			break;
		}

		ret = k_sem_take(&dev_data->completion_sync,
				 I2C_TRANSFER_TIMEOUT_MSEC);
		if (ret != 0) {
			/* Whatever the frequency, completion_sync should have
			 * been given by the event handler.
			 *
			 * If it hasn't, it's probably due to an hardware issue
			 * on the I2C line, for example a short between SDA and
			 * GND.
			 * This is issue has also been when trying to use the
			 * I2C bus during MCU internal flash erase.
			 *
			 * In many situation, a retry is sufficient.
			 * However, some time the I2C device get stuck and need
			 * help to recover.
			 * Therefore we always call i2c_nrfx_twim_recover_bus()
			 * to make sure everything has been done to restore the
			 * bus from this error.
			 */
			(void)i2c_nrfx_twim_recover_bus(dev);
			ret = -EIO;
			break;
		}

		if (dev_data->res < 0) {
			ret = -EIO;
			break;
		}

		/* If concatenated messages were I2C_MSG_READ type, then
		 * content of concatenation buffer has to be copied back into
		 * buffers provided by user. */
		if ((msgs[i].flags & I2C_MSG_READ) && (buf == msg_buf)) {
			int j = i;

			while (msg_buf_used >= msgs[j].len) {
				msg_buf_used -= msgs[j].len;
				memcpy(msgs[j].buf,
				       msg_buf + msg_buf_used,
				       msgs[j].len);
				j--;
			}

		}

		msg_buf_used = 0;
	}

	i2c_nrfx_twim_exclusive_access_release(dev);

	return ret;
}

static void event_handler(nrfx_twim_event_t const *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct i2c_nrfx_twim_data *dev_data = dev->data;
	const struct i2c_nrfx_twim_common_config *config = dev->config;

	if (p_event->xfer_desc.type == NRFX_TWIM_XFER_TX) {
		dmm_buffer_out_release(config->mem_reg,
			(void **)&p_event->xfer_desc.p_primary_buf);
	} else {
		dmm_buffer_in_release(config->mem_reg, dev_data->buf_ptr,
			p_event->xfer_desc.primary_length,
			p_event->xfer_desc.p_primary_buf);
	}

	switch (p_event->type) {
	case NRFX_TWIM_EVT_DONE:
		dev_data->res = 0;
		break;
	case NRFX_TWIM_EVT_ADDRESS_NACK:
		dev_data->res = -EFAULT;
		break;
	case NRFX_TWIM_EVT_DATA_NACK:
		dev_data->res = -EAGAIN;
		break;
	default:
		dev_data->res = -EIO;
		break;
	}

	k_sem_give(&dev_data->completion_sync);
}

static int i2c_nrfx_twim_init(const struct device *dev)
{
	struct i2c_nrfx_twim_data *data = dev->data;

	k_sem_init(&data->transfer_sync, 1, 1);
	k_sem_init(&data->completion_sync, 0, 1);

	return i2c_nrfx_twim_common_init(dev);
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int i2c_nrfx_twim_deinit(const struct device *dev)
{
	return i2c_nrfx_twim_common_deinit(dev);
}
#endif

static DEVICE_API(i2c, i2c_nrfx_twim_driver_api) = {
	.configure = i2c_nrfx_twim_configure,
	.transfer = i2c_nrfx_twim_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
	.recover_bus = i2c_nrfx_twim_recover_bus,
};

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_twim)
#define DT_DRV_COMPAT nordic_nrf_twim
#endif

#define I2C_NRFX_TWIM_DEVICE(inst)							      \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(DT_DRV_INST(inst));				      \
	NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(DT_DRV_INST(inst));		      \
	BUILD_ASSERT(I2C_FREQUENCY(inst) != I2C_NRFX_TWIM_INVALID_FREQUENCY,		      \
		     "Wrong I2C " #inst " frequency setting in dts");			      \
	static struct i2c_nrfx_twim_data twim_##inst##_data;				      \
	static struct i2c_nrfx_twim_common_config twim_##inst##z_config;		      \
	NRF_DT_INST_IRQ_DIRECT_DEFINE(							      \
		inst,									      \
		nrfx_twim_irq_handler,							      \
		&CONCAT(twim_, inst, _data.twim)					      \
	)										      \
	static void pre_init##inst(void)						      \
	{										      \
		twim_##inst##z_config.twim = &twim_##inst##_data.twim;			      \
		twim_##inst##_data.twim.p_twim = (NRF_TWIM_Type *)DT_INST_REG_ADDR(inst);     \
		NRF_DT_INST_IRQ_CONNECT(						      \
			inst,								      \
			nrfx_twim_irq_handler,						      \
			&CONCAT(twim_, inst, _data.twim)				      \
		)									      \
	}										      \
	IF_ENABLED(USES_MSG_BUF(inst),							      \
		(static uint8_t twim_##inst##_msg_buf[MSG_BUF_SIZE(inst)]		      \
		 DMM_MEMORY_SECTION(DT_DRV_INST(inst));))				      \
	PINCTRL_DT_INST_DEFINE(inst);							      \
	static struct i2c_nrfx_twim_common_config twim_##inst##z_config = {		      \
		.twim_config =								      \
			{								      \
				.skip_gpio_cfg = true,					      \
				.skip_psel_cfg = true,					      \
				.frequency = I2C_FREQUENCY(inst),			      \
			},								      \
		.event_handler = event_handler,						      \
		.msg_buf_size = MSG_BUF_SIZE(inst),					      \
		.pre_init = pre_init##inst,						      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				      \
		IF_ENABLED(USES_MSG_BUF(inst),						      \
			(.msg_buf = twim_##inst##_msg_buf,))				      \
		.max_transfer_size = MAX_TRANSFER_SIZE(inst),				      \
		.mem_reg = DMM_DEV_TO_REG(DT_DRV_INST(inst)),				      \
	};										      \
	PM_DEVICE_DT_INST_DEFINE(inst, twim_nrfx_pm_action, I2C_PM_ISR_SAFE(inst));	      \
	I2C_DEVICE_DT_INST_DEINIT_DEFINE(inst, i2c_nrfx_twim_init, i2c_nrfx_twim_deinit,      \
					 PM_DEVICE_DT_INST_GET(inst), &twim_##inst##_data,    \
					 &twim_##inst##z_config, POST_KERNEL,		      \
					 CONFIG_I2C_INIT_PRIORITY, &i2c_nrfx_twim_driver_api) \

DT_INST_FOREACH_STATUS_OKAY(I2C_NRFX_TWIM_DEVICE)
