/*
 * Copyright (c) 2025 Ambiq <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_ios_i2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <soc.h>

LOG_MODULE_REGISTER(i2c_ambiq_ios, CONFIG_I2C_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_APOLLO5X)
#define IOS_ADDR_INTERVAL (IOSLAVEFD1_BASE - IOSLAVEFD0_BASE)
#else
#define IOS_ADDR_INTERVAL 1
#endif

#define AMBIQ_I2C_IOS_FIFO_BASE   0x30
#define AMBIQ_I2C_IOS_FIFO_END    0x100
#define AMBIQ_I2C_IOS_FIFO_LENGTH (AMBIQ_I2C_IOS_FIFO_END - AMBIQ_I2C_IOS_FIFO_BASE)

struct ambiq_i2c_ios_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t inst_idx;
	uint8_t fifo_thr;
	void (*irq_cfg)(void);
	void (*acc_irq_cfg)(void);
	bool has_acc_irq;
};

struct ambiq_i2c_ios_data {
	void *i2c_ios_handle;
	struct i2c_target_config *tgt;
	bool enabled;
	uint8_t sram_buf[1023];
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	bool pm_active;
#endif

	/* read buffer-mode staging */
	const uint8_t *rd_ptr;
	uint32_t rd_len;
	uint32_t rd_pos;
	/* byte-mode read state */
	bool read_active;
	uint16_t active_offset;
	/* LRAM pointer and size - depends on SOC and inst_idx */
	volatile uint8_t *lram_ptr;
	uint32_t lram_size;
};

static void i2c_ambiq_ios_reset_read_state(struct ambiq_i2c_ios_data *data)
{
	data->rd_ptr = NULL;
	data->rd_len = 0U;
	data->rd_pos = 0U;
	data->read_active = false;
}

static void i2c_ambiq_ios_feed_fifo(const struct device *dev)
{
	struct ambiq_i2c_ios_data *data = dev->data;
	uint32_t left = data->rd_len - data->rd_pos;
	uint32_t wrote = 0;

	if (!data->tgt || !data->rd_ptr) {
		return;
	}
	if (!left) {
		return;
	}
	am_hal_ios_fifo_write(data->i2c_ios_handle, (uint8_t *)&data->rd_ptr[data->rd_pos], left,
			      &wrote);
	data->rd_pos += wrote;
	am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_FIFO_UPDATE_CTR, NULL);
}

static void i2c_ambiq_ios_handle_genad(struct ambiq_i2c_ios_data *data)
{
	uint32_t gadata = 0;
	uint32_t fifo_ptr = AMBIQ_I2C_IOS_FIFO_BASE;

	if (am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_READ_GADATA, &gadata) ==
	    AM_HAL_STATUS_SUCCESS) {
		data->active_offset = (uint16_t)(gadata & 0xFFu);
		i2c_ambiq_ios_reset_read_state(data);
		fifo_ptr += gadata;
		am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_SET_FIFO_PTR, &fifo_ptr);
	}
}

static void i2c_ambiq_ios_isr(const struct device *dev)
{
	struct ambiq_i2c_ios_data *data = dev->data;
	uint32_t status = 0;

	am_hal_ios_interrupt_status_get(data->i2c_ios_handle, true, &status);

	if (status & AM_HAL_IOS_INT_GENAD) {
		i2c_ambiq_ios_handle_genad(data);
		am_hal_ios_interrupt_clear(data->i2c_ios_handle, AM_HAL_IOS_INT_GENAD);
	}

	if (status & AM_HAL_IOS_INT_FSIZE) {
		const struct i2c_target_callbacks *cb = data->tgt ? data->tgt->callbacks : NULL;
		bool byte_mode = cb && (cb->read_requested || cb->read_processed);
		bool buf_mode =
			cb && IS_ENABLED(CONFIG_I2C_TARGET_BUFFER_MODE) && cb->buf_read_requested;

		if (byte_mode) {
			/* first byte read_requested */
			if (!data->read_active && cb->read_requested) {
				uint8_t v = 0;

				if (cb->read_requested(data->tgt, &v) == 0) {
					uint32_t wrote = 0;
					(void)am_hal_ios_fifo_write(data->i2c_ios_handle, &v, 1,
								    &wrote);
					if (wrote) {
						data->read_active = true;
					}
				}
			}
			/* rest bytes read_processed */
			if (cb->read_processed) {
				for (int i = 0; i < 8; i++) {
					uint8_t v = 0;

					if (cb->read_processed(data->tgt, &v) != 0) {
						break;
					}
					uint32_t wrote = 0;
					(void)am_hal_ios_fifo_write(data->i2c_ios_handle, &v, 1,
								    &wrote);
					if (!wrote) {
						break;
					}
				}
			}
		} else if (buf_mode) {
			if (data->rd_pos == data->rd_len) {
				uint8_t *ptr = NULL;
				uint32_t len = 0;

				if (cb->buf_read_requested(data->tgt, &ptr, &len) == 0 && ptr &&
				    len) {
					data->rd_ptr = ptr;
					data->rd_len = len;
					data->rd_pos = 0;
				}
			}
			i2c_ambiq_ios_feed_fifo(dev);
		}
		am_hal_ios_interrupt_clear(data->i2c_ios_handle, AM_HAL_IOS_INT_FSIZE);
	}
	am_hal_ios_interrupt_clear(data->i2c_ios_handle,
				   status & ~(AM_HAL_IOS_INT_GENAD | AM_HAL_IOS_INT_FSIZE));
}

int i2c_ambiq_ios_test_read_lram(const struct device *dev, uint32_t offset, uint8_t *val)
{
	struct ambiq_i2c_ios_data *data = dev->data;

	if (!data->enabled || !val || !data->lram_ptr) {
		return -EACCES;
	}

	if (offset >= data->lram_size) {
		return -EINVAL;
	}

	*val = data->lram_ptr[offset];
	return 0;
}

static void i2c_ambiq_ios_acc_isr(const struct device *dev)
{
	struct ambiq_i2c_ios_data *data = dev->data;
	uint32_t acc_pend = 0;
	int status;

	status = am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_ACC_INTGET, &acc_pend);
	if (status != AM_HAL_STATUS_SUCCESS) {
		return;
	}

	const struct i2c_target_callbacks *cb = data->tgt ? data->tgt->callbacks : NULL;
	bool is_match = (acc_pend & AM_HAL_IOS_ACCESS_INT_00) && data->tgt && cb;

	if (!is_match || !data->lram_ptr) {
		goto clear_acc;
	}

	uint8_t len = data->lram_ptr[0];

	if (len > 0 && (len + 1) <= data->lram_size) {
		bool has_byte_cbs = (cb->write_requested || cb->write_received || cb->stop);

		if (has_byte_cbs) {
			if (cb->write_requested) {
				(void)cb->write_requested(data->tgt);
			}
			for (uint32_t i = 0; i < len; i++) {
				if (!cb->write_received) {
					continue;
				}
				uint8_t v = data->lram_ptr[1 + i];

				(void)cb->write_received(data->tgt, v);
			}
			if (cb->stop) {
				(void)cb->stop(data->tgt);
			}
		} else if (IS_ENABLED(CONFIG_I2C_TARGET_BUFFER_MODE) && cb->buf_write_received) {
			cb->buf_write_received(data->tgt, (uint8_t *)&data->lram_ptr[1], len);
		}
		data->lram_ptr[0] = 0;
	}

clear_acc:
	(void)am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_ACC_INTCLR, &acc_pend);
}

#ifdef CONFIG_PM_DEVICE
static int i2c_ambiq_ios_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ambiq_i2c_ios_config *cfg = dev->config;
	struct ambiq_i2c_ios_data *data = dev->data;
	int err;
	am_hal_sysctrl_power_state_e status;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Set pins to active state */
		err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}
		status = AM_HAL_SYSCTRL_WAKE;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Move pins to sleep state */
		err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
		if ((err < 0) && (err != -ENOENT)) {
			/*
			 * If returning -ENOENT, no pins where defined for sleep mode :
			 * Do not output on console (might sleep already) when going to
			 * sleep,
			 * "I2C pinctrl sleep state not available"
			 * and don't block PM suspend.
			 * Else return the error.
			 */
			return err;
		}
		status = AM_HAL_SYSCTRL_DEEPSLEEP;
		break;
	default:
		return -ENOTSUP;
	}

	err = am_hal_ios_power_ctrl(data->i2c_ios_handle, status, true);
	if (err != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("am_hal_ios_power_ctrl failed: %d", err);
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_I2C_TARGET
static int i2c_ambiq_ios_target_register(const struct device *dev, struct i2c_target_config *tcfg)
{
	const struct ambiq_i2c_ios_config *config = dev->config;
	struct ambiq_i2c_ios_data *data = dev->data;
	am_hal_ios_config_t ios = {0};
	uint32_t acc_mask = AM_HAL_IOS_ACCESS_INT_00;
	int ret;
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	bool pm_got = false;
#endif

	if (data->tgt) {
		return -EBUSY;
	}

#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}
	pm_got = true;
#endif

	/* Disable IOS instance as it cannot be configured when enabled */
	if (am_hal_ios_disable(data->i2c_ios_handle) != AM_HAL_STATUS_SUCCESS) {
		ret = -EIO;
		goto err_pm_put;
	}

	ios.ui32InterfaceSelect = AM_HAL_IOS_USE_I2C | AM_HAL_IOS_I2C_ADDRESS(tcfg->address << 1);
	ios.ui32ROBase = AMBIQ_I2C_IOS_FIFO_BASE;
	ios.ui32FIFOBase = AMBIQ_I2C_IOS_FIFO_BASE;
	ios.ui32RAMBase = AMBIQ_I2C_IOS_FIFO_END;
	/* FIFO Threshold - set to half the size */
	ios.ui32FIFOThreshold = config->fifo_thr;
	ios.pui8SRAMBuffer = data->sram_buf;
	ios.ui32SRAMBufferCap = sizeof(data->sram_buf);

	if (am_hal_ios_configure(data->i2c_ios_handle, &ios) != AM_HAL_STATUS_SUCCESS) {
		ret = -EIO;
		goto err_pm_put;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		goto err_pm_put;
	}

	if (am_hal_ios_enable(data->i2c_ios_handle) != AM_HAL_STATUS_SUCCESS) {
		ret = -EIO;
		goto err_pm_put;
	}

	am_hal_ios_interrupt_clear(data->i2c_ios_handle, AM_HAL_IOS_INT_ALL);
	am_hal_ios_interrupt_enable(data->i2c_ios_handle,
				    AM_HAL_IOS_INT_GENAD | AM_HAL_IOS_INT_FSIZE);
	am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_ACC_INTEN, &acc_mask);

	data->tgt = tcfg;
	data->enabled = true;
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	data->pm_active = pm_got;
#endif

	if (IS_ENABLED(CONFIG_I2C_TARGET_BUFFER_MODE) && data->tgt->callbacks &&
	    data->tgt->callbacks->buf_read_requested) {
		uint8_t *ptr = NULL;
		uint32_t len = 0;

		if (data->tgt->callbacks->buf_read_requested(data->tgt, &ptr, &len) == 0 && ptr &&
		    len) {
			data->rd_ptr = ptr;
			data->rd_len = len;
			data->rd_pos = 0;
			i2c_ambiq_ios_feed_fifo(dev);
		}
	}

	return 0;

err_pm_put:
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	if (pm_got) {
		(void)pm_device_runtime_put(dev);
	}
#endif
	return ret;
}

static int i2c_ambiq_ios_target_unregister(const struct device *dev, struct i2c_target_config *tcfg)
{
	const struct ambiq_i2c_ios_config *config = dev->config;
	struct ambiq_i2c_ios_data *data = dev->data;
	uint32_t acc_mask = AM_HAL_IOS_ACCESS_INT_ALL;

	if (data->tgt != tcfg) {
		return -EINVAL;
	}

	am_hal_ios_interrupt_disable(data->i2c_ios_handle, AM_HAL_IOS_INT_ALL);
	am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_ACC_INTDIS, &acc_mask);
	am_hal_ios_control(data->i2c_ios_handle, AM_HAL_IOS_REQ_FIFO_BUF_CLR, NULL);
	am_hal_ios_disable(data->i2c_ios_handle);
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	if (data->pm_active) {
		(void)pm_device_runtime_put(dev);
		data->pm_active = false;
	}
#endif
	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);

	data->tgt = NULL;
	data->enabled = false;
	data->rd_ptr = NULL;
	data->rd_len = data->rd_pos = 0;

	return 0;
}
#endif

static int i2c_ambiq_ios_init(const struct device *dev)
{
	const struct ambiq_i2c_ios_config *config = dev->config;
	struct ambiq_i2c_ios_data *data = dev->data;
	uint32_t ret = am_hal_ios_initialize(config->inst_idx, &data->i2c_ios_handle);

	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize i2c target\n");
		return -EBUSY;
	}

	ret = am_hal_ios_power_ctrl(data->i2c_ios_handle, AM_HAL_SYSCTRL_WAKE, false);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to power up i2c target\n");
		am_hal_ios_uninitialize(data->i2c_ios_handle);
		return -ENXIO;
	}

	/* Set LRAM pointer and size based on SOC type and inst_idx */
	/* others use IOSLAVE */
	extern volatile uint8_t * const am_hal_ios_pui8LRAM;

	data->lram_ptr = am_hal_ios_pui8LRAM;
	data->lram_size = AM_HAL_IOS_FIFO_MAX_SIZE;

	config->irq_cfg();
	if (config->has_acc_irq && config->acc_irq_cfg) {
		config->acc_irq_cfg();
	}

	data->enabled = false;
	data->tgt = NULL;
	data->rd_ptr = NULL;
	data->rd_len = data->rd_pos = 0;
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	data->pm_active = false;
#endif
	return 0;
}

static DEVICE_API(i2c, i2c_ambiq_ios_api) = {
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_ambiq_ios_target_register,
	.target_unregister = i2c_ambiq_ios_target_unregister,
#endif
};

#define AMBIQ_I2C_IOS_IRQ_CFG(n)                                                       \
	static void i2c_ambiq_ios_irq_cfg_##n(void)                                        \
	{                                                                                  \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),   \
			i2c_ambiq_ios_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                        \
	}

#define AMBIQ_I2C_IOS_ACC_IRQ_CFG(n)                                               \
	COND_CODE_1(DT_IRQ_HAS_IDX(DT_INST_PARENT(n), 1), (                            \
		static void i2c_ambiq_ios_acc_irq_cfg_##n(void)                            \
		{                                                                          \
			IRQ_CONNECT(DT_IRQ_BY_IDX(DT_INST_PARENT(n), 1, irq),                  \
				DT_IRQ_BY_IDX(DT_INST_PARENT(n), 1, priority),                     \
				i2c_ambiq_ios_acc_isr, DEVICE_DT_INST_GET(n), 0);                  \
			irq_enable(DT_IRQ_BY_IDX(DT_INST_PARENT(n), 1, irq));                  \
		}                                                                          \
	), (/* no acc irq */))

#define AMBIQ_I2C_IOS_DEFINE(n)                                                        \
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_INST_PARENT(n)) == 1,                     \
		     "Too many children for IOS, either SPI or I2C should be enabled!");       \
	PINCTRL_DT_INST_DEFINE(n);                                                         \
	AMBIQ_I2C_IOS_IRQ_CFG(n)                                                           \
	AMBIQ_I2C_IOS_ACC_IRQ_CFG(n)                                                       \
	static const struct ambiq_i2c_ios_config cfg_##n = {                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                     \
		.inst_idx = (DT_REG_ADDR(DT_INST_PARENT(n)) - IOSLAVE_BASE) / IOS_ADDR_INTERVAL,\
		.fifo_thr = DT_INST_PROP_OR(n, fifo_threshold, 16),                            \
		.irq_cfg = i2c_ambiq_ios_irq_cfg_##n,                                          \
		.acc_irq_cfg = COND_CODE_1(DT_IRQ_HAS_IDX(DT_INST_PARENT(n), 1),               \
		     (i2c_ambiq_ios_acc_irq_cfg_##n), (NULL)),                                 \
		.has_acc_irq = DT_IRQ_HAS_IDX(DT_INST_PARENT(n), 1),                           \
	};                                                                                 \
	static struct ambiq_i2c_ios_data data_##n;                                         \
	PM_DEVICE_DT_INST_DEFINE(n, i2c_ambiq_ios_pm_action);                              \
	DEVICE_DT_INST_DEFINE(n, i2c_ambiq_ios_init, PM_DEVICE_DT_INST_GET(n), &data_##n,  \
			&cfg_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,                           \
			&i2c_ambiq_ios_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_I2C_IOS_DEFINE)
