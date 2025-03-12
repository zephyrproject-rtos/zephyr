/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_twim

#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_twim.h>
#include <zephyr/sys/util.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include "i2c_nrfx_twim_common.h"

LOG_MODULE_REGISTER(i2c_nrfx_twim, CONFIG_I2C_LOG_LEVEL);

#if CONFIG_I2C_NRFX_TRANSFER_TIMEOUT
#define I2C_TRANSFER_TIMEOUT_MSEC K_MSEC(CONFIG_I2C_NRFX_TRANSFER_TIMEOUT)
#else
#define I2C_TRANSFER_TIMEOUT_MSEC K_FOREVER
#endif

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

	k_sem_take(&dev_data->transfer_sync, K_FOREVER);

	/* Dummy take on completion_sync sem to be sure that it is empty */
	k_sem_take(&dev_data->completion_sync, K_NO_WAIT);

	(void)pm_device_runtime_get(dev);

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
		ret = i2c_nrfx_twim_msg_transfer(dev, msgs[i].flags, buf, buf_len, addr);
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

		if (dev_data->res != NRFX_SUCCESS) {
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

	(void)pm_device_runtime_put(dev);

	k_sem_give(&dev_data->transfer_sync);

	return ret;
}

static void event_handler(nrfx_twim_evt_t const *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct i2c_nrfx_twim_data *dev_data = dev->data;

	switch (p_event->type) {
	case NRFX_TWIM_EVT_DONE:
		dev_data->res = NRFX_SUCCESS;
		break;
	case NRFX_TWIM_EVT_ADDRESS_NACK:
		dev_data->res = NRFX_ERROR_DRV_TWI_ERR_ANACK;
		break;
	case NRFX_TWIM_EVT_DATA_NACK:
		dev_data->res = NRFX_ERROR_DRV_TWI_ERR_DNACK;
		break;
	default:
		dev_data->res = NRFX_ERROR_INTERNAL;
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

static DEVICE_API(i2c, i2c_nrfx_twim_driver_api) = {
	.configure = i2c_nrfx_twim_configure,
	.transfer = i2c_nrfx_twim_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
	.recover_bus = i2c_nrfx_twim_recover_bus,
};

#define CONCAT_BUF_SIZE(inst)						       \
	COND_CODE_1(DT_INST_HAS_PROP(inst, zephyr_concat_buf_size),	       \
		    (DT_INST_PROP(inst, zephyr_concat_buf_size)), (0))
#define FLASH_BUF_MAX_SIZE(inst)					       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, zephyr_flash_buf_max_size),    \
		    (DT_INST_PROP(inst, zephyr_flash_buf_max_size)), (0))

#define USES_MSG_BUF(inst)						       \
	COND_CODE_0(CONCAT_BUF_SIZE(inst),				       \
		(COND_CODE_0(FLASH_BUF_MAX_SIZE(inst), (0), (1))),	       \
		(1))
#define MSG_BUF_SIZE(inst)  MAX(CONCAT_BUF_SIZE(inst), FLASH_BUF_MAX_SIZE(inst))

#define I2C_NRFX_TWIM_DEVICE(inst)					       \
	static void irq_connect##inst(void);				       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(DT_DRV_INST(inst));		       \
	BUILD_ASSERT(I2C_FREQUENCY(inst) !=				       \
		     I2C_NRFX_TWIM_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #inst" frequency setting in dts");	       \
	IF_ENABLED(USES_MSG_BUF(inst),					       \
		(static uint8_t twim_##inst##_msg_buf[MSG_BUF_SIZE(inst)]      \
		 I2C_MEMORY_SECTION(inst);))				       \
	static struct i2c_nrfx_twim_data twim_##inst##_data = {		       \
		.twim = NRFX_TWIM_INSTANCE(DT_INST_REG_ADDR(inst)),            \
	};								       \
	PINCTRL_DT_INST_DEFINE(inst);					       \
	static const							       \
	struct i2c_nrfx_twim_common_config twim_##inst##z_config = {	       \
		.twim_config = {					       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.frequency = I2C_FREQUENCY(inst),		       \
		},							       \
		.event_handler = event_handler,				       \
		.msg_buf_size = MSG_BUF_SIZE(inst),			       \
		.irq_connect = irq_connect##inst,			       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		       \
		IF_ENABLED(USES_MSG_BUF(inst),				       \
			(.msg_buf = twim_##inst##_msg_buf,))		       \
		.max_transfer_size = BIT_MASK(				       \
				DT_INST_PROP(inst, easydma_maxcnt_bits)),      \
	};								       \
	static void irq_connect##inst(void)				       \
	{								       \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),   \
			    nrfx_twim_irq_handler,			       \
			    &twim_##inst##_data.twim, 0);		       \
	}								       \
	PM_DEVICE_DT_INST_DEFINE(int, twim_nrfx_pm_action,		       \
				 PM_DEVICE_ISR_SAFE);			       \
	I2C_DEVICE_DT_INST_DEFINE(inst,					       \
		      i2c_nrfx_twim_init,				       \
		      PM_DEVICE_DT_INST_GET(inst),			       \
		      &twim_##inst##_data,				       \
		      &twim_##inst##z_config,				       \
		      POST_KERNEL,					       \
		      CONFIG_I2C_INIT_PRIORITY,				       \
		      &i2c_nrfx_twim_driver_api)

#define I2C_MEMORY_SECTION(inst)					       \
	COND_CODE_1(DT_INST_HAS_PROP(inst, memory_regions),		       \
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	       \
			DT_INST_PHANDLE(inst, memory_regions)))))),	       \
		())

DT_INST_FOREACH_STATUS_OKAY(I2C_NRFX_TWIM_DEVICE)
