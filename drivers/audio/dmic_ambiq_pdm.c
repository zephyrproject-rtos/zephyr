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
#include <zephyr/audio/dmic.h>
#include <zephyr/cache.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>
#include <am_mcu_apollo.h>

#define DT_DRV_COMPAT ambiq_pdm

LOG_MODULE_REGISTER(ambiq_pdm, CONFIG_AUDIO_DMIC_LOG_LEVEL);

struct dmic_ambiq_pdm_data {
	void *pdm_handler;
	struct k_mem_slab *mem_slab;
	void *mem_slab_buffer;
	struct k_sem dma_done_sem;
	int inst_idx;
	uint32_t block_size;
	uint32_t sample_num;
	uint8_t frame_size_bytes;
	am_hal_pdm_config_t pdm_cfg;
	am_hal_pdm_transfer_t pdm_transfer;
	bool pm_policy_state_on;

	enum dmic_state dmic_state;
};

struct dmic_ambiq_pdm_cfg {
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
};

static void dmic_ambiq_pdm_pm_policy_state_lock_get(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct dmic_ambiq_pdm_data *data = dev->data;

		if (!data->pm_policy_state_on) {
			data->pm_policy_state_on = true;
			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
			pm_device_runtime_get(dev);
		}
	}
}

static void dmic_ambiq_pdm_pm_policy_state_lock_put(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct dmic_ambiq_pdm_data *data = dev->data;

		if (data->pm_policy_state_on) {
			data->pm_policy_state_on = false;
			pm_device_runtime_put(dev);
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		}
	}
}

static void dmic_ambiq_pdm_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct dmic_ambiq_pdm_data *data = dev->data;

	am_hal_pdm_interrupt_status_get(data->pdm_handler, &ui32Status, true);
	am_hal_pdm_interrupt_clear(data->pdm_handler, ui32Status);
	am_hal_pdm_interrupt_service(data->pdm_handler, ui32Status, &(data->pdm_transfer));

	if (ui32Status & AM_HAL_PDM_INT_DCMP) {
		k_sem_give(&data->dma_done_sem);
	}
}

static int dmic_ambiq_pdm_init(const struct device *dev)
{
	struct dmic_ambiq_pdm_data *data = dev->data;
	const struct dmic_ambiq_pdm_cfg *config = dev->config;

	int ret = 0;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config PDM pins\n");
		return ret;
	}

	am_hal_pdm_initialize(data->inst_idx, &data->pdm_handler);
	am_hal_pdm_power_control(data->pdm_handler, AM_HAL_PDM_POWER_ON, false);

	data->dmic_state = DMIC_STATE_INITIALIZED;

	return 0;
}

static int dmic_ambiq_pdm_configure(const struct device *dev, struct dmic_cfg *dev_config)
{
	struct dmic_ambiq_pdm_data *data = dev->data;
	const struct dmic_ambiq_pdm_cfg *config = dev->config;

	struct pdm_chan_cfg *channel = &dev_config->channel;
	struct pcm_stream_cfg *stream = &dev_config->streams[0];

	if (data->dmic_state == DMIC_STATE_ACTIVE) {
		LOG_ERR("Cannot configure device while it is active");
		return -EBUSY;
	}

	if (channel->req_num_streams != 1 || channel->req_num_chan > 2 ||
	    channel->req_num_chan < 1 || channel->req_chan_map_hi != channel->act_chan_map_hi) {
		LOG_ERR("Requested configuration is not supported");
		return -EINVAL;
	}

	if ((stream->pcm_width != 16) && (stream->pcm_width != 24)) {
		LOG_ERR("Only 16-bit or 24-bit samples are supported");
		return -EINVAL;
	}

	channel->act_num_streams = 1;
	channel->act_chan_map_hi = 0;
	channel->act_chan_map_lo = channel->req_chan_map_lo;

	data->pdm_cfg.ePDMClkSpeed = AM_HAL_PDM_CLK_HFRC_24MHZ;

	/*  1.5MHz PDM CLK OUT:
	 *      AM_HAL_PDM_CLK_24MHZ, AM_HAL_PDM_MCLKDIV_1, AM_HAL_PDM_PDMA_CLKO_DIV7
	 *  15.625KHz 24bit Sampling:
	 *      DecimationRate = 48
	 */
	data->pdm_cfg.eClkDivider = AM_HAL_PDM_MCLKDIV_1;
	data->pdm_cfg.ePDMAClkOutDivder = AM_HAL_PDM_PDMA_CLKO_DIV7;
	data->pdm_cfg.ui32DecimationRate = 48;

	data->pdm_cfg.eStepSize = AM_HAL_PDM_GAIN_STEP_0_13DB;

	data->pdm_cfg.bHighPassEnable = AM_HAL_PDM_HIGH_PASS_ENABLE;
	data->pdm_cfg.ui32HighPassCutoff = 0x3;

	data->pdm_cfg.eLeftGain = AM_HAL_PDM_GAIN_0DB;
	data->pdm_cfg.eRightGain = AM_HAL_PDM_GAIN_0DB;

	data->pdm_cfg.bDataPacking = 1;

	if (channel->req_num_chan == 1) {
		data->pdm_cfg.ePCMChannels = AM_HAL_PDM_CHANNEL_LEFT;
	} else {
		data->pdm_cfg.ePCMChannels = AM_HAL_PDM_CHANNEL_STEREO;
	}

	data->pdm_cfg.bPDMSampleDelay = AM_HAL_PDM_CLKOUT_PHSDLY_NONE;
	data->pdm_cfg.ui32GainChangeDelay = AM_HAL_PDM_CLKOUT_DELAY_NONE;

	data->pdm_cfg.bSoftMute = 0;
	data->pdm_cfg.bLRSwap = 1;

	am_hal_pdm_configure(data->pdm_handler, &data->pdm_cfg);

	am_hal_pdm_interrupt_clear(data->pdm_handler, (AM_HAL_PDM_INT_DERR | AM_HAL_PDM_INT_DCMP |
						       AM_HAL_PDM_INT_UNDFL | AM_HAL_PDM_INT_OVF));
	am_hal_pdm_interrupt_enable(data->pdm_handler, (AM_HAL_PDM_INT_DERR | AM_HAL_PDM_INT_DCMP |
							AM_HAL_PDM_INT_UNDFL | AM_HAL_PDM_INT_OVF));
	config->irq_config_func();

	data->block_size = stream->block_size;

	if (stream->pcm_width == 16) {
		data->frame_size_bytes = 2;
	} else if (stream->pcm_width == 24) {
		data->frame_size_bytes = 4;
	}

	data->sample_num = stream->block_size / data->frame_size_bytes;
	data->mem_slab = stream->mem_slab;

	data->dmic_state = DMIC_STATE_CONFIGURED;

	/* Configure DMA and target address.*/
	data->pdm_transfer.ui32TotalCount = data->sample_num * sizeof(uint32_t);
	data->pdm_transfer.ui32TargetAddrReverse =
		data->pdm_transfer.ui32TargetAddr + data->pdm_transfer.ui32TotalCount;

	return 0;
}

static void am_pdm_dma_trigger(const struct device *dev)
{
	struct dmic_ambiq_pdm_data *data = dev->data;

	/* Start the data transfer. */
	am_hal_pdm_dma_start(data->pdm_handler, &(data->pdm_transfer));
}

static int dmic_ambiq_pdm_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct dmic_ambiq_pdm_data *data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_PAUSE:
	case DMIC_TRIGGER_STOP:
		if (data->dmic_state == DMIC_STATE_ACTIVE) {
			am_hal_pdm_disable(data->pdm_handler);
			data->dmic_state = DMIC_STATE_PAUSED;
		}
		break;

	case DMIC_TRIGGER_RELEASE:
	case DMIC_TRIGGER_START:
		if (data->dmic_state == DMIC_STATE_PAUSED ||
		    data->dmic_state == DMIC_STATE_CONFIGURED) {
			am_hal_pdm_enable(data->pdm_handler);
			am_pdm_dma_trigger(dev);
			data->dmic_state = DMIC_STATE_ACTIVE;
		}
		break;

	default:
		LOG_ERR("Invalid command: %d", cmd);
		return -EINVAL;
	}

	return 0;
}

static int dmic_ambiq_pdm_read(const struct device *dev, uint8_t stream, void **buffer,
			       size_t *size, int32_t timeout)
{
	struct dmic_ambiq_pdm_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if (data->dmic_state != DMIC_STATE_ACTIVE) {
		LOG_ERR("Device is not activated");
		return -EIO;
	}

	ret = k_sem_take(&data->dma_done_sem, SYS_TIMEOUT_MS(timeout));
	dmic_ambiq_pdm_pm_policy_state_lock_get(dev);

	if (ret != 0) {
		LOG_DBG("No audio data to be read %d", ret);
	} else {
		ret = k_mem_slab_alloc(data->mem_slab, &data->mem_slab_buffer, K_NO_WAIT);

		uint32_t *pdm_data_buf = (uint32_t *)am_hal_pdm_dma_get_buffer(data->pdm_handler);

		if (data->frame_size_bytes == 2) {
			/*
			 * PDM DMA is 32-bit datawidth for each sample, so we need to invalidate 2x
			 * block_size on 16 bit PCM data.
			 */
#if CONFIG_PDM_AMBIQ_HANDLE_CACHE
			if (!buf_in_nocache((uintptr_t)pdm_data_buf, data->block_size * 2)) {
				sys_cache_data_invd_range(pdm_data_buf, data->block_size * 2);
			}
#endif /* PDM_AMBIQ_HANDLE_CACHE */
			uint8_t *temp1 = (uint8_t *)data->mem_slab_buffer;
			/* Re-arrange data */
			for (uint32_t i = 0; i < data->sample_num; i++) {
				temp1[2 * i] = (pdm_data_buf[i] & 0xFF00) >> 8U;
				temp1[2 * i + 1] = (pdm_data_buf[i] & 0xFF0000) >> 16U;
			}
		} else if (data->frame_size_bytes == 4) {
#if CONFIG_PDM_AMBIQ_HANDLE_CACHE
			if (!buf_in_nocache((uintptr_t)pdm_data_buf, data->block_size)) {
				sys_cache_data_invd_range(pdm_data_buf, data->block_size);
			}
#endif /* PDM_AMBIQ_HANDLE_CACHE */
			memcpy((void *)data->mem_slab_buffer, (void *)pdm_data_buf,
			       data->block_size);
		}

		*size = data->block_size;
		*buffer = data->mem_slab_buffer;
	}

	dmic_ambiq_pdm_pm_policy_state_lock_put(dev);
	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int dmic_ambiq_pdm_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct dmic_ambiq_pdm_data *data = dev->data;
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

	ret = am_hal_pdm_power_control(data->pdm_handler, status, true);

	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("am_hal_pdm_power_control failed: %d", ret);
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /* CONFIG_PM_DEVICE */

static const struct _dmic_ops dmic_ambiq_ops = {
	.configure = dmic_ambiq_pdm_configure,
	.trigger = dmic_ambiq_pdm_trigger,
	.read = dmic_ambiq_pdm_read,
};

#define AMBIQ_PDM_DEFINE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void pdm_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), dmic_ambiq_pdm_isr,         \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static uint32_t pdm_dma_tcb_buf##n[DT_INST_PROP_OR(n, pdm_buffer_size, 1536)]              \
		__attribute__((section(DT_INST_PROP_OR(n, pdm_buffer_location, ".data"))))         \
		__aligned(CONFIG_PDM_AMBIQ_BUFFER_ALIGNMENT);                                      \
	static struct dmic_ambiq_pdm_data dmic_ambiq_pdm_data##n = {                               \
		.dma_done_sem = Z_SEM_INITIALIZER(dmic_ambiq_pdm_data##n.dma_done_sem, 0, 1),      \
		.inst_idx = n,                                                                     \
		.block_size = 0,                                                                   \
		.sample_num = 0,                                                                   \
		.dmic_state = DMIC_STATE_UNINIT,                                                   \
		.pdm_transfer.ui32TargetAddr = (uint32_t)pdm_dma_tcb_buf##n,                       \
	};                                                                                         \
	static const struct dmic_ambiq_pdm_cfg dmic_ambiq_pdm_cfg##n = {                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = pdm_irq_config_func_##n,                                        \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, dmic_ambiq_pdm_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(n, dmic_ambiq_pdm_init, NULL, &dmic_ambiq_pdm_data##n,               \
			      &dmic_ambiq_pdm_cfg##n, POST_KERNEL,                                 \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_ambiq_ops);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_PDM_DEFINE)
