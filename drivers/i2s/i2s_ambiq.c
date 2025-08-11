/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/cache.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>

#include <am_mcu_apollo.h>

#define DT_DRV_COMPAT ambiq_i2s

LOG_MODULE_REGISTER(ambiq_i2s, LOG_LEVEL_ERR);

struct i2s_ambiq_data {
	void *i2s_handler;
	void *pdm_handler;
	void *mem_slab_buffer;
	struct k_mem_slab *mem_slab;
	void *i2s_dma_buf;
	struct k_sem tx_ready_sem;
	struct k_sem rx_done_sem;
	int inst_idx;
	uint32_t block_size;
	uint32_t sample_num;
	am_hal_i2s_config_t i2s_hal_cfg;
	am_hal_i2s_transfer_t i2s_transfer;
	struct i2s_config i2s_user_config;
	uint32_t *dma_tcb_tx_buf;
	uint32_t *dma_tcb_rx_buf;
	bool pm_policy_state_on;

	enum i2s_state i2s_state;
};

struct i2s_ambiq_cfg {
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
};

static void i2s_ambiq_pm_policy_state_lock_get(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct i2s_ambiq_data *data = dev->data;

		if (!data->pm_policy_state_on) {
			data->pm_policy_state_on = true;
			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
			pm_device_runtime_get(dev);
		}
	}
}

static void i2s_ambiq_pm_policy_state_lock_put(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct i2s_ambiq_data *data = dev->data;

		if (data->pm_policy_state_on) {
			data->pm_policy_state_on = false;
			pm_device_runtime_put(dev);
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		}
	}
}

static am_hal_i2s_data_format_t i2s_data_format = {
	.ePhase = AM_HAL_I2S_DATA_PHASE_SINGLE,

	.eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_16BITS,
	.eChannelLenPhase2 = AM_HAL_I2S_FRAME_WDLEN_16BITS,
	.eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_16BITS,
	.eSampleLenPhase2 = AM_HAL_I2S_SAMPLE_LENGTH_16BITS,

	.ui32ChannelNumbersPhase1 = 2,
	.ui32ChannelNumbersPhase2 = 0,
	.eDataDelay = 0x0,
	.eDataJust = AM_HAL_I2S_DATA_JUSTIFIED_LEFT,
};

static am_hal_i2s_io_signal_t i2s_io_config = {
	.sFsyncPulseCfg = {
			.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_ONE_SUBFRAME,
		},
	.eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_LOW,
	.eTxCpol = AM_HAL_I2S_IO_TX_CPOL_FALLING,
	.eRxCpol = AM_HAL_I2S_IO_RX_CPOL_RISING,
};

static void i2s_ambiq_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct i2s_ambiq_data *data = dev->data;

	am_hal_i2s_interrupt_status_get(data->i2s_handler, &ui32Status, true);
	am_hal_i2s_interrupt_clear(data->i2s_handler, ui32Status);
	am_hal_i2s_interrupt_service(data->i2s_handler, ui32Status, &(data->i2s_hal_cfg));

	if (ui32Status & AM_HAL_I2S_INT_TXDMACPL) {
		k_sem_give(&data->tx_ready_sem);
	}

	if (ui32Status & AM_HAL_I2S_INT_RXDMACPL) {
		k_sem_give(&data->rx_done_sem);
	}
}

static int i2s_ambiq_init(const struct device *dev)
{
	struct i2s_ambiq_data *data = dev->data;
	const struct i2s_ambiq_cfg *config = dev->config;

	int ret = 0;

	if (ret < 0) {
		LOG_ERR("Fail to power on I2S\n");
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config I2S pins\n");
	}

	am_hal_i2s_initialize(data->inst_idx, &data->i2s_handler);
	am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_ON, false);

	data->i2s_state = I2S_STATE_NOT_READY;

	return 0;
}

static int i2s_ambiq_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_config_in)
{
	struct i2s_ambiq_data *data = dev->data;
	const struct i2s_ambiq_cfg *config = dev->config;
	int i2s_clock_freq;

	if ((data->i2s_state != I2S_STATE_NOT_READY && data->i2s_state != I2S_STATE_READY) ||
	    (data->i2s_state == I2S_STATE_RUNNING)) {
		LOG_ERR("invalid state %d", data->i2s_state);
		return -EINVAL;
	}

	if (i2s_config_in->frame_clk_freq == 0U) {
		LOG_ERR("Invalid frame_clk_freq %u", i2s_config_in->frame_clk_freq);
		data->i2s_state = I2S_STATE_NOT_READY;
		return 0;
	}

	data->i2s_hal_cfg.eData = &i2s_data_format;

	if (i2s_config_in->word_size == 16) {
		data->i2s_hal_cfg.eData->eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_16BITS;
		data->i2s_hal_cfg.eData->eChannelLenPhase2 = AM_HAL_I2S_FRAME_WDLEN_16BITS;
		data->i2s_hal_cfg.eData->eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_16BITS;
		data->i2s_hal_cfg.eData->eSampleLenPhase2 = AM_HAL_I2S_SAMPLE_LENGTH_16BITS;
		data->sample_num = i2s_config_in->block_size / 2;
	} else if (i2s_config_in->word_size == 24) {
		data->i2s_hal_cfg.eData->eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_32BITS;
		data->i2s_hal_cfg.eData->eChannelLenPhase2 = AM_HAL_I2S_FRAME_WDLEN_32BITS;
		data->i2s_hal_cfg.eData->eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_24BITS;
		data->i2s_hal_cfg.eData->eSampleLenPhase2 = AM_HAL_I2S_SAMPLE_LENGTH_24BITS;
		data->sample_num = i2s_config_in->block_size / 4;
	}
	if (i2s_config_in->word_size == 32) {
		data->i2s_hal_cfg.eData->eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_32BITS;
		data->i2s_hal_cfg.eData->eChannelLenPhase2 = AM_HAL_I2S_FRAME_WDLEN_32BITS;
		data->i2s_hal_cfg.eData->eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_32BITS;
		data->i2s_hal_cfg.eData->eSampleLenPhase2 = AM_HAL_I2S_SAMPLE_LENGTH_32BITS;
		data->sample_num = i2s_config_in->block_size / 4;
	}

	data->i2s_hal_cfg.eData->ui32ChannelNumbersPhase1 = i2s_config_in->channels;

	switch (i2s_config_in->format) {
	case I2S_FMT_DATA_FORMAT_I2S:
		data->i2s_hal_cfg.eData->eDataDelay = 0x1;
		i2s_io_config.sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_CUSTOM;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		data->i2s_hal_cfg.eData->eDataDelay = 0x1;
		i2s_io_config.sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_ONE_BIT_CLOCK;
		i2s_io_config.eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		i2s_io_config.sFsyncPulseCfg.eFsyncPulseType =
			AM_HAL_I2S_FSYNC_PULSE_HALF_FRAME_PERIOD;
		i2s_io_config.eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		i2s_io_config.sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_CUSTOM;
		i2s_io_config.eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		data->i2s_hal_cfg.eData->eDataJust = AM_HAL_I2S_DATA_JUSTIFIED_RIGHT;
		i2s_io_config.sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_CUSTOM;
		i2s_io_config.eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		break;
	default:
		LOG_ERR("Unsupported data format %d", i2s_config_in->format);
		return -EINVAL;
	}

	if (i2s_io_config.sFsyncPulseCfg.eFsyncPulseType == AM_HAL_I2S_FSYNC_PULSE_CUSTOM) {
		if (data->i2s_hal_cfg.eData->eChannelLenPhase1 == AM_HAL_I2S_FRAME_WDLEN_8BITS) {
			i2s_io_config.sFsyncPulseCfg.ui32FsyncPulseWidth = 7;
		} else if (data->i2s_hal_cfg.eData->eChannelLenPhase1 ==
			   AM_HAL_I2S_FRAME_WDLEN_16BITS) {
			i2s_io_config.sFsyncPulseCfg.ui32FsyncPulseWidth = 15;
		} else if (data->i2s_hal_cfg.eData->eChannelLenPhase1 ==
			   AM_HAL_I2S_FRAME_WDLEN_32BITS) {
			i2s_io_config.sFsyncPulseCfg.ui32FsyncPulseWidth = 31;
		} else {
			LOG_ERR("Unsupported channel length %d",
				data->i2s_hal_cfg.eData->eChannelLenPhase1);
			return -EINVAL;
		}
	}

	if (dir == I2S_DIR_TX) {
		if ((i2s_config_in->format == I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED) ||
		    (i2s_config_in->format == I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED)) {
			i2s_io_config.eTxCpol = AM_HAL_I2S_IO_TX_CPOL_RISING;
		}
		data->i2s_hal_cfg.eXfer = AM_HAL_I2S_XFER_TX;
		data->i2s_hal_cfg.eMode = AM_HAL_I2S_IO_MODE_MASTER;
	} else if (dir == I2S_DIR_RX) {
		if ((i2s_config_in->format == I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED) ||
		    (i2s_config_in->format == I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED)) {
			i2s_io_config.eRxCpol = AM_HAL_I2S_IO_RX_CPOL_RISING;
		}
		data->i2s_hal_cfg.eXfer = AM_HAL_I2S_XFER_RX;
		data->i2s_hal_cfg.eMode = AM_HAL_I2S_IO_MODE_SLAVE;
	} else {
		LOG_ERR("Unsupported direction %d", dir);
		return -EINVAL;
	}

	if (i2s_config_in->options & I2S_OPT_LOOPBACK) {
		data->i2s_hal_cfg.eXfer = AM_HAL_I2S_XFER_RXTX;
		data->i2s_hal_cfg.eMode = AM_HAL_I2S_IO_MODE_MASTER;
	}

	i2s_clock_freq = i2s_config_in->frame_clk_freq * i2s_config_in->channels *
			 ((i2s_config_in->word_size == 16) ? 16 : 32);

	/*
	 * Lowest clock freq is 128 KHz (16bit / 1 channel / 8KHz sample rate)
	 * Highst clock freq is 3072 KHz (32bit / 2 channels / 48KHz sample rate)
	 */
	if (i2s_clock_freq < 128000 || i2s_clock_freq > 3072000) {
		LOG_ERR("Invalid I2S clock frequency %d", i2s_clock_freq);
		return -EINVAL;
	}

	LOG_INF("I2S clock frequency %d KHz", i2s_clock_freq / 1000);

	switch (i2s_clock_freq) {
	case 128000:
		data->i2s_hal_cfg.eClock = eAM_HAL_I2S_CLKSEL_HFRC_375kHz;
		data->i2s_hal_cfg.eDiv3 = 1;
		break;
	case 256000:
		data->i2s_hal_cfg.eClock = eAM_HAL_I2S_CLKSEL_HFRC_750kHz;
		data->i2s_hal_cfg.eDiv3 = 1;
		break;
	case 512000:
		data->i2s_hal_cfg.eClock = eAM_HAL_I2S_CLKSEL_HFRC_1_5MHz;
		data->i2s_hal_cfg.eDiv3 = 1;
		break;
	case 768000:
		data->i2s_hal_cfg.eClock = eAM_HAL_I2S_CLKSEL_HFRC_750kHz;
		data->i2s_hal_cfg.eDiv3 = 0;
		break;
	case 1024000:
		data->i2s_hal_cfg.eClock = eAM_HAL_I2S_CLKSEL_HFRC_3MHz;
		data->i2s_hal_cfg.eDiv3 = 1;
		break;
	case 1536000:
		data->i2s_hal_cfg.eClock = eAM_HAL_I2S_CLKSEL_HFRC_1_5MHz;
		data->i2s_hal_cfg.eDiv3 = 0;
		break;
	case 3072000:
		data->i2s_hal_cfg.eClock = eAM_HAL_I2S_CLKSEL_HFRC_3MHz;
		data->i2s_hal_cfg.eDiv3 = 0;
		break;
	default:
		LOG_ERR("Unsupported I2S clock frequency %d", i2s_clock_freq);
		return -EINVAL;
	}

	data->i2s_hal_cfg.eASRC = 0;
	data->i2s_hal_cfg.eIO = &i2s_io_config;

	LOG_INF("I2S eClock %d, eDiv3 %d\n", data->i2s_hal_cfg.eClock & 0xFF,
		data->i2s_hal_cfg.eDiv3);

	if (i2s_config_in->channels > 2) {
		LOG_ERR("Unsupported channel number %d", i2s_config_in->channels);
		return -EINVAL;
	}

	am_hal_i2s_configure(data->i2s_handler, &(data->i2s_hal_cfg));
	am_hal_i2s_enable(data->i2s_handler);
	config->irq_config_func();

	data->block_size = i2s_config_in->block_size;
	data->mem_slab = i2s_config_in->mem_slab;

	/*
	 * Configure DMA and target address.
	 */
	if (dir == I2S_DIR_TX) {
		uint8_t *tx_buf_8 = (uint8_t *)data->dma_tcb_tx_buf;

		data->i2s_transfer.ui32TxTotalCount =
			data->sample_num; /* I2S DMA buffer count is the number of 32-bit datawidth.
					   */
		data->i2s_transfer.ui32TxTargetAddr = (uint32_t)tx_buf_8;
		data->i2s_transfer.ui32TxTargetAddrReverse = (uint32_t)&tx_buf_8[data->block_size];
		LOG_INF("TX addr : 0x%x Cnt : %d Rev : 0x%x", data->i2s_transfer.ui32TxTargetAddr,
			data->i2s_transfer.ui32TxTotalCount,
			data->i2s_transfer.ui32TxTargetAddrReverse);
	} else {
		uint8_t *rx_buf_8 = (uint8_t *)data->dma_tcb_rx_buf;

		data->i2s_transfer.ui32RxTotalCount =
			data->sample_num; /* I2S DMA buffer count is the number of 32-bit datawidth.
					   */
		data->i2s_transfer.ui32RxTargetAddr = (uint32_t)rx_buf_8;
		data->i2s_transfer.ui32RxTargetAddrReverse = (uint32_t)&rx_buf_8[data->block_size];
		LOG_INF("RX addr : 0x%x Cnt : %d Rev : 0x%x", data->i2s_transfer.ui32RxTargetAddr,
			data->i2s_transfer.ui32RxTotalCount,
			data->i2s_transfer.ui32RxTargetAddrReverse);
	}

	memcpy(&(data->i2s_user_config), i2s_config_in, sizeof(struct i2s_config));

	data->i2s_state = I2S_STATE_READY;

	return 0;
}

static const struct i2s_config *i2s_ambiq_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_ambiq_data *data = dev->data;

	if (data->i2s_state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	return &(data->i2s_user_config);
}

static int i2s_ambiq_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_ambiq_data *data = dev->data;
	int ret = 0;

	ARG_UNUSED(dir);

	LOG_INF("Direction: %d Command: %d", dir, cmd);
	switch (cmd) {
	case I2S_TRIGGER_STOP:
	case I2S_TRIGGER_DROP:
		if (dir == I2S_DIR_BOTH) {
			LOG_ERR("Unsupported direction %d for STOP/DRAIN/DROP", dir);
			return -EINVAL;
		}

		if (data->i2s_state == I2S_STATE_RUNNING) {
			am_hal_i2s_dma_transfer_complete(data->i2s_handler);
			am_hal_i2s_disable(data->i2s_handler);
			data->i2s_state = I2S_STATE_READY;
			k_sleep(K_MSEC(100));
		}
		break;

	case I2S_TRIGGER_DRAIN:
		if (data->i2s_state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN/STOP trigger: invalid state %d", data->i2s_state);
			ret = -EIO;
			break;
		}
		data->i2s_state = I2S_STATE_STOPPING;
		break;

	case I2S_TRIGGER_START:
		if (data->i2s_state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d", data->i2s_state);
			ret = -EIO;
			break;
		}
		am_hal_i2s_enable(data->i2s_handler);
		am_hal_i2s_dma_configure(data->i2s_handler, &(data->i2s_hal_cfg),
					 &(data->i2s_transfer));
		am_hal_i2s_dma_transfer_start(data->i2s_handler, &(data->i2s_hal_cfg));
		data->i2s_state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_PREPARE:
		if (data->i2s_state != I2S_STATE_ERROR) {
			LOG_ERR("Invalid state for PREPARE trigger: %d", data->i2s_state);
			ret = -EIO;
			break;
		}

		am_hal_i2s_disable(data->i2s_handler);
		data->i2s_state = I2S_STATE_READY;
		break;

	default:
		LOG_ERR("Invalid command: %d", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int i2s_ambiq_write(const struct device *dev, void *buffer, size_t size)
{
	struct i2s_ambiq_data *data = dev->data;
	int ret;

	if ((data->i2s_state != I2S_STATE_RUNNING) && (data->i2s_state != I2S_STATE_READY)) {
		LOG_ERR("Device is not ready or running");
		return -EIO;
	}

	if (size > data->block_size) {
		LOG_ERR("Max write size is: %u", data->block_size);
		return -EINVAL;
	}

	ret = k_sem_take(&(data->tx_ready_sem), K_MSEC(100));

	i2s_ambiq_pm_policy_state_lock_get(dev);

	uint32_t i2s_data_buf_ptr =
		am_hal_i2s_dma_get_buffer(data->i2s_handler, AM_HAL_I2S_XFER_TX);

	if (data->i2s_user_config.word_size == 16) {
		int16_t *i2s_data_buf_16bit = (int16_t *)i2s_data_buf_ptr;
		int16_t *data_buf = (int16_t *)buffer;

		for (int i = 0; i < data->sample_num; i++) {
			i2s_data_buf_16bit[i] = data_buf[i];
		}
	} else if ((data->i2s_user_config.word_size == 24) ||
		   (data->i2s_user_config.word_size == 32)) {
		int32_t *i2s_data_buf_32bit = (int32_t *)i2s_data_buf_ptr;
		int32_t *data_buf = (int32_t *)buffer;

		for (int i = 0; i < data->sample_num; i++) {
			i2s_data_buf_32bit[i] = data_buf[i];
		}
	}

#if CONFIG_I2S_AMBIQ_HANDLE_CACHE
	if (!buf_in_nocache((uintptr_t)i2s_data_buf_ptr, data->block_size)) {
		/* Clean I2S DMA buffer of block_size after filling data. */
		sys_cache_data_flush_range((uint32_t *)i2s_data_buf_ptr, data->block_size);
	}
#endif /* CONFIG_I2S_AMBIQ_HANDLE_CACHE */

	k_mem_slab_free(data->mem_slab, buffer);
	i2s_ambiq_pm_policy_state_lock_put(dev);

	return ret;
}

static int i2s_ambiq_read(const struct device *dev, void **buffer, size_t *size)
{
	struct i2s_ambiq_data *data = dev->data;
	int ret;

	if ((data->i2s_state != I2S_STATE_RUNNING) && (data->i2s_state != I2S_STATE_READY)) {
		LOG_ERR("Device is not running or ready");
		return -EIO;
	}

	ret = k_sem_take(&(data->rx_done_sem), K_MSEC(100));

	if (ret != 0) {
		LOG_DBG("No audio data to be read %d", ret);
	} else {
		i2s_ambiq_pm_policy_state_lock_get(dev);

		ret = k_mem_slab_alloc(data->mem_slab, &data->mem_slab_buffer, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("Fail to allocate memory slab");
			return -ENOMEM;
		}

		uint32_t *i2s_data_buf = (uint32_t *)am_hal_i2s_dma_get_buffer(data->i2s_handler,
									       AM_HAL_I2S_XFER_RX);

#if CONFIG_I2S_AMBIQ_HANDLE_CACHE
		if (!buf_in_nocache((uintptr_t)i2s_data_buf, data->block_size)) {
			/* I2S DMA is 32-bit datawidth for each sample, so we need to invalidate 2x
			 * block_size when we are getting 16 bits sample.
			 */
			sys_cache_data_invd_range(i2s_data_buf, data->block_size);
		}
#endif /* CONFIG_I2S_AMBIQ_HANDLE_CACHE */

		memcpy(data->mem_slab_buffer, (void *)i2s_data_buf, data->block_size);
		*size = data->block_size;
		*buffer = data->mem_slab_buffer;
	}

	i2s_ambiq_pm_policy_state_lock_put(dev);

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int i2s_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct i2s_ambiq_data *data = dev->data;
	uint32_t ret;
	am_hal_sysctrl_power_state_e status;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		status = AM_HAL_SYSCTRL_WAKE;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		status = AM_HAL_SYSCTRL_DEEPSLEEP;
		break;
	default:
		return -ENOTSUP;
	}

	ret = am_hal_i2s_power_control(data->i2s_handler, status, true);

	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("am_hal_i2s_power_control failed: %d", ret);
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(i2s, i2s_ambiq_driver_api) = {
	.configure = i2s_ambiq_configure,
	.read = i2s_ambiq_read,
	.write = i2s_ambiq_write,
	.config_get = i2s_ambiq_config_get,
	.trigger = i2s_ambiq_trigger,
};

#define AMBIQ_I2S_DEFINE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void i2s_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2s_ambiq_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static uint32_t i2s_dma_tcb_buf##n[DT_INST_PROP_OR(n, i2s_buffer_size, 1536) * 2]          \
		__attribute__((section(DT_INST_PROP_OR(n, i2s_buffer_location, ".data"))))         \
		__aligned(CONFIG_I2S_AMBIQ_BUFFER_ALIGNMENT);                                      \
	static struct i2s_ambiq_data i2s_ambiq_data##n = {                                         \
		.tx_ready_sem = Z_SEM_INITIALIZER(i2s_ambiq_data##n.tx_ready_sem, 1, 1),           \
		.rx_done_sem = Z_SEM_INITIALIZER(i2s_ambiq_data##n.rx_done_sem, 0, 1),             \
		.inst_idx = n,                                                                     \
		.block_size = 0,                                                                   \
		.sample_num = 0,                                                                   \
		.i2s_state = I2S_STATE_NOT_READY,                                                  \
		.dma_tcb_tx_buf = i2s_dma_tcb_buf##n,                                              \
		.dma_tcb_rx_buf = i2s_dma_tcb_buf##n + DT_INST_PROP_OR(n, i2s_buffer_size, 1536),  \
	};                                                                                         \
	static const struct i2s_ambiq_cfg i2s_ambiq_cfg##n = {                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = i2s_irq_config_func_##n,                                        \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, i2s_ambiq_pm_action);                                          \
	DEVICE_DT_INST_DEFINE(n, i2s_ambiq_init, NULL, &i2s_ambiq_data##n, &i2s_ambiq_cfg##n,      \
			      POST_KERNEL, CONFIG_I2S_INIT_PRIORITY, &i2s_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_I2S_DEFINE)
