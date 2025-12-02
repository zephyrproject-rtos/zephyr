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
#include <zephyr/sys/atomic.h>
#include <am_mcu_apollo.h>

#define DT_DRV_COMPAT ambiq_pdm

LOG_MODULE_REGISTER(ambiq_pdm, CONFIG_AUDIO_DMIC_LOG_LEVEL);

enum pdm_ambiq_pm_policy_flag {
	PDM_AMBIQ_PM_POLICY_STATE_FLAG,
	PDM_AMBIQ_PM_POLICY_DTCM_FLAG,
	PDM_AMBIQ_PM_POLICY_FLAG_COUNT,
};

struct dmic_ambiq_pdm_data {
	int inst_idx;
	void *pdm_handler;
	uint16_t block_size;
	struct k_mem_slab *mem_slab;
	am_hal_pdm_config_t hal_cfg;
	void *rx_tip_buffer;
	bool rx_dma_stopping;
	struct k_msgq rx_dma_queue;
	ATOMIC_DEFINE(pm_policy_flag, PDM_AMBIQ_PM_POLICY_FLAG_COUNT);
	enum dmic_state dmic_state;
};

typedef struct {
	void *dma_buf;
	size_t size;
} dma_msg;

struct dmic_ambiq_pdm_cfg {
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
};

static void dmic_ambiq_pdm_pm_policy_state_lock_get(const struct device *dev)
{
	struct dmic_ambiq_pdm_data *data = dev->data;

	if (!atomic_test_bit(data->pm_policy_flag, PDM_AMBIQ_PM_POLICY_DTCM_FLAG)) {
		atomic_set_bit(data->pm_policy_flag, PDM_AMBIQ_PM_POLICY_DTCM_FLAG);
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

static void dmic_ambiq_pdm_pm_policy_state_lock_put(const struct device *dev)
{
	struct dmic_ambiq_pdm_data *data = dev->data;

	if (atomic_test_bit(data->pm_policy_flag, PDM_AMBIQ_PM_POLICY_DTCM_FLAG)) {
		atomic_clear_bit(data->pm_policy_flag, PDM_AMBIQ_PM_POLICY_DTCM_FLAG);
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

static void dmic_ambiq_dma_stop(const struct device *dev)
{
	struct dmic_ambiq_pdm_data *data = dev->data;

	dmic_ambiq_pdm_pm_policy_state_lock_put(dev);

	am_hal_pdm_interrupt_disable(data->pdm_handler, AM_HAL_PDM_INT_DCMP);
	am_hal_pdm_interrupt_clear(data->pdm_handler, AM_HAL_PDM_INT_DCMP);
	am_hal_pdm_dma_stop(data->pdm_handler);
	am_hal_pdm_disable(data->pdm_handler);
}

static void dmic_ambiq_dma_reload(const struct device *dev, dma_msg *msg)
{
	struct dmic_ambiq_pdm_data *data = dev->data;
	am_hal_pdm_transfer_t dma_transfer;

	dma_transfer.ui32TargetAddr = (uint32_t)msg->dma_buf;
	dma_transfer.ui32TotalCount = msg->size;
	dma_transfer.ui32TargetAddrReverse = 0xFFFFFFFF;
	data->rx_tip_buffer = msg->dma_buf;

	am_hal_pdm_dma_transfer_continue(data->pdm_handler, &dma_transfer);
}

static void dmic_ambiq_dma_queue_drop(const struct device *dev)
{
	dma_msg item;
	struct dmic_ambiq_pdm_data *data = dev->data;

	if (data->rx_tip_buffer != NULL) {
		k_mem_slab_free(data->mem_slab, data->rx_tip_buffer);
		data->rx_tip_buffer = NULL;
	}

	while (k_msgq_get(&data->rx_dma_queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, item.dma_buf);
	}
}

static void dmic_ambiq_rx_dmacpl_handler(const struct device *dev)
{
	int ret;
	dma_msg item;
	struct dmic_ambiq_pdm_data *data = dev->data;

	PDMn(data->inst_idx)->DMASTAT_b.DMACPL = 0;

	if (data->rx_tip_buffer == NULL) {
		goto rx_exit;
	}

	item.dma_buf = data->rx_tip_buffer;
	item.size = data->block_size;
	data->rx_tip_buffer = NULL;
	ret = k_msgq_put(&data->rx_dma_queue, &item, K_NO_WAIT);
	if (ret < 0) {
		k_mem_slab_free(data->mem_slab, item.dma_buf);
		goto rx_exit;
	}

	if (data->rx_dma_stopping == true) {
		data->rx_dma_stopping = false;
		goto rx_exit;
	}

	ret = k_mem_slab_alloc(data->mem_slab, &item.dma_buf, K_NO_WAIT);
	if (ret < 0) {
		goto rx_exit;
	}

	if (ambiq_buf_in_dtcm((uintptr_t)item.dma_buf, data->block_size)) {
		dmic_ambiq_pdm_pm_policy_state_lock_get(dev);
	} else {
		dmic_ambiq_pdm_pm_policy_state_lock_put(dev);
	}

	dmic_ambiq_dma_reload(dev, &item);
	return;

rx_exit:
	dmic_ambiq_dma_stop(dev);
	data->dmic_state = DMIC_STATE_PAUSED;
}

static void dmic_ambiq_pdm_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct dmic_ambiq_pdm_data *data = dev->data;

	am_hal_pdm_interrupt_status_get(data->pdm_handler, &ui32Status, true);
	am_hal_pdm_interrupt_clear(data->pdm_handler, ui32Status);

	if (ui32Status & AM_HAL_PDM_INT_DCMP) {
		dmic_ambiq_rx_dmacpl_handler(dev);
	}
}

static bool div_derive(uint32_t pdm_op_freq, uint32_t io_clk,
		       uint32_t *mclk_div, uint32_t *pdma_div)
{
	uint32_t total_div, div1, div2;

	if ((pdm_op_freq == 0) || (io_clk == 0)) {
		return false;
	}

	if ((pdm_op_freq % io_clk) != 0) {
		return false;
	}

	total_div = pdm_op_freq / io_clk;

	for (div1 = 2; div1 <= 4; div1++) {
		if ((total_div % div1) != 0) {
			continue;
		}
		div2 = total_div / div1;
		if (IN_RANGE(div2, 2, 16)) {
			*mclk_div = div1;
			*pdma_div = div2;
			return true;
		}
	}
	return false;
}

static int pdm_clock_settings_derive(const struct device *dev, struct dmic_cfg *dev_config)
{
	int ret;
	bool valid_settings_found = false;
	struct dmic_ambiq_pdm_data *data = dev->data;

	uint32_t osr_table[] = {64, 96, 100, 48, 50, 32, 128};
	uint32_t freq_table[] = {12288, 16384, 24000, 24576, 27648, 48000};

	uint32_t io_freq, osr, mclk_div, pdma_div, pdm_op_freq, pll_precfg_freq;
	uint32_t pcm_rate = dev_config->streams->pcm_rate;
	uint32_t max_clk_freq = dev_config->io.max_pdm_clk_freq;
	uint32_t min_clk_freq = dev_config->io.min_pdm_clk_freq;

	am_hal_clkmgr_clock_config_get(AM_HAL_CLKMGR_CLK_ID_SYSPLL, &pll_precfg_freq, 0);

	ARRAY_FOR_EACH(osr_table, idx0) {
		osr = osr_table[idx0];
		io_freq = pcm_rate * osr;
		if (IN_RANGE(io_freq, min_clk_freq, max_clk_freq) == false) {
			continue;
		}

		if (pll_precfg_freq != 0) {
			if (div_derive(pll_precfg_freq, io_freq, &mclk_div, &pdma_div)) {
				valid_settings_found = true;
				break;
			}
			continue;
		}

		ARRAY_FOR_EACH(freq_table, idx1) {
			pdm_op_freq = freq_table[idx1] * 1000;

			if (!div_derive(pdm_op_freq, io_freq, &mclk_div, &pdma_div)) {
				continue;
			}

			ret = am_hal_clkmgr_clock_config(AM_HAL_CLKMGR_CLK_ID_SYSPLL,
							 pdm_op_freq, NULL);
			if (ret != AM_HAL_STATUS_SUCCESS) {
				continue;
			}

			valid_settings_found = true;
			break;
		}

		if (valid_settings_found) {
			break;
		}
	}

	if (valid_settings_found) {
		data->hal_cfg.ePDMClkSpeed = AM_HAL_PDM_CLK_PLL;
		data->hal_cfg.eClkDivider = mclk_div - 1;
		data->hal_cfg.ePDMAClkOutDivder = pdma_div - 1;
		data->hal_cfg.ui32DecimationRate = osr / 2;
		return 0;
	} else {
		return -EINVAL;
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
	int ret;
	uint32_t channel_map0, channel_map1;
	struct dmic_ambiq_pdm_data *data = dev->data;
	const struct dmic_ambiq_pdm_cfg *config = dev->config;

	struct pdm_chan_cfg *channel = &dev_config->channel;
	struct pcm_stream_cfg *stream = &dev_config->streams[0];

	if (data->dmic_state == DMIC_STATE_ACTIVE) {
		LOG_ERR("Cannot configure device while it is active");
		return -EBUSY;
	}

	if (stream->pcm_width != 24) {

		LOG_ERR("Unsupported PCM width %d", stream->pcm_width);
		return -EINVAL;
	}

	if (channel->req_num_streams != 1) {
		LOG_ERR("Only 1 stream is supported");
		return -EINVAL;
	}

	if (channel->req_num_chan == 1) {
		channel_map0 = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
		channel_map1 = dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT);
		if (channel->req_chan_map_lo == channel_map0) {
			data->hal_cfg.ePCMChannels = AM_HAL_PDM_CHANNEL_LEFT;
			data->hal_cfg.bLRSwap = 0;
		} else if (channel->req_chan_map_lo == channel_map1) {
			data->hal_cfg.ePCMChannels = AM_HAL_PDM_CHANNEL_RIGHT;
			data->hal_cfg.bLRSwap = 1;
		} else {
			LOG_ERR("Unsupported channel map for mono");
			return -EINVAL;
		}
		channel->act_num_chan = 1;
	} else if (channel->req_num_chan == 2) {
		channel_map0 = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT) |
			       dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
		channel_map1 = dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT) |
			       dmic_build_channel_map(1, 0, PDM_CHAN_LEFT);
		if (channel->req_chan_map_lo == channel_map0) {
			data->hal_cfg.ePCMChannels = AM_HAL_PDM_CHANNEL_STEREO;
			data->hal_cfg.bLRSwap = 0;
		} else if (channel->req_chan_map_lo == channel_map1) {
			data->hal_cfg.ePCMChannels = AM_HAL_PDM_CHANNEL_STEREO;
			data->hal_cfg.bLRSwap = 1;
		} else {
			LOG_ERR("Unsupported channel map for stereo");
			return -EINVAL;
		}
		channel->act_num_chan = 2;
	} else {
		LOG_ERR("More than 2 channels are not supported");
		return -EINVAL;
	}

	channel->act_num_streams = 1;
	channel->act_chan_map_hi = 0;
	channel->act_chan_map_lo = channel->req_chan_map_lo;

	ret = pdm_clock_settings_derive(dev, dev_config);
	if (ret != 0) {
		LOG_ERR("pdm_configure: failed to set clock");
		return -EINVAL;
	}

	data->hal_cfg.bHighPassEnable = AM_HAL_PDM_HIGH_PASS_ENABLE;
	data->hal_cfg.ui32HighPassCutoff = 10;
	data->hal_cfg.eLeftGain = AM_HAL_PDM_GAIN_0DB;
	data->hal_cfg.eRightGain = AM_HAL_PDM_GAIN_0DB;
	data->hal_cfg.eStepSize = AM_HAL_PDM_GAIN_STEP_0_13DB;
	data->hal_cfg.bPDMSampleDelay = AM_HAL_PDM_CLKOUT_PHSDLY_NONE;
	data->hal_cfg.ui32GainChangeDelay = AM_HAL_PDM_CLKOUT_DELAY_NONE;
	data->hal_cfg.bSoftMute = 0;

	am_hal_pdm_configure(data->pdm_handler, &data->hal_cfg);
	config->irq_config_func();

	data->mem_slab = stream->mem_slab;
	data->block_size = stream->block_size;
	data->dmic_state = DMIC_STATE_CONFIGURED;

	return 0;
}

static int dmic_ambiq_dma_start(const struct device *dev)
{
	int ret;
	void *buf;
	struct dmic_ambiq_pdm_data *data = dev->data;
	am_hal_pdm_transfer_t dma_transfer;

	if (AM_HAL_STATUS_SUCCESS != am_hal_pdm_enable(data->pdm_handler)) {
		LOG_ERR("dmic_trigger: HAL failed to enable pdm");
		return -EIO;
	}

	ret = k_mem_slab_alloc(data->mem_slab, &buf, K_NO_WAIT);
	if (ret < 0) {
		am_hal_pdm_disable(data->pdm_handler);
		return -ENOMEM;
	}

	dma_transfer.ui32TargetAddr = (uint32_t)buf;
	dma_transfer.ui32TotalCount = data->block_size;
	dma_transfer.ui32TargetAddrReverse = 0xFFFFFFFF;
	data->rx_tip_buffer = buf;

	am_hal_pdm_interrupt_enable(data->pdm_handler, AM_HAL_PDM_INT_DCMP);

	if (ambiq_buf_in_dtcm((uintptr_t)buf, data->block_size)) {
		dmic_ambiq_pdm_pm_policy_state_lock_get(dev);
	}

	/* Start the data transfer. */
	am_hal_pdm_dma_start(data->pdm_handler, &dma_transfer);

	return 0;
}

static int dmic_ambiq_pdm_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	int ret;
	struct dmic_ambiq_pdm_data *data = dev->data;

	if ((data->dmic_state == DMIC_STATE_UNINIT) ||
	    (data->dmic_state == DMIC_STATE_INITIALIZED)) {
		LOG_ERR("Device state is not valid for trigger");
		return -EIO;
	}

	switch (cmd) {
	case DMIC_TRIGGER_PAUSE:
		if (data->dmic_state == DMIC_STATE_ACTIVE) {
			data->rx_dma_stopping = true;
		}
		break;

	case DMIC_TRIGGER_STOP:
		dmic_ambiq_dma_stop(dev);
		dmic_ambiq_dma_queue_drop(dev);
		data->dmic_state = DMIC_STATE_CONFIGURED;
		break;

	case DMIC_TRIGGER_RELEASE:
	case DMIC_TRIGGER_START:
		if (data->dmic_state == DMIC_STATE_PAUSED ||
		    data->dmic_state == DMIC_STATE_CONFIGURED) {
			ret = dmic_ambiq_dma_start(dev);
			if (ret < 0) {
				LOG_ERR("Failed to start dmic: %d", ret);
				return ret;
			}
			data->dmic_state = DMIC_STATE_ACTIVE;
		}
		data->rx_dma_stopping = false;
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
	int ret;
	dma_msg rx_dma_msg;
	struct dmic_ambiq_pdm_data *data = dev->data;

	ARG_UNUSED(stream);

	if ((data->dmic_state != DMIC_STATE_CONFIGURED) &&
	    (data->dmic_state != DMIC_STATE_ACTIVE) &&
	    (data->dmic_state != DMIC_STATE_PAUSED)) {
		LOG_ERR("Device state is not valid for read");
		return -EIO;
	}

	ret = k_msgq_get(&data->rx_dma_queue, &rx_dma_msg, SYS_TIMEOUT_MS(timeout));
	if (ret < 0) {
		return ret;
	}

#if CONFIG_PDM_AMBIQ_HANDLE_CACHE
	if (!buf_in_nocache((uintptr_t)rx_dma_msg.dma_buf, rx_dma_msg.size)) {
		sys_cache_data_invd_range(rx_dma_msg.dma_buf, rx_dma_msg.size);
	}
#endif

	*buffer = rx_dma_msg.dma_buf;
	*size = rx_dma_msg.size;

	return 0;
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
	static dma_msg rx_dma_msgs_##n[CONFIG_PDM_AMBIQ_RX_BLOCK_COUNT];                           \
	static struct dmic_ambiq_pdm_data dmic_ambiq_pdm_data##n = {                               \
		.inst_idx = n,                                                                     \
		.dmic_state = DMIC_STATE_UNINIT,                                                   \
		.rx_tip_buffer = NULL,                                                             \
		.rx_dma_stopping = false,                                                          \
		.rx_dma_queue = Z_MSGQ_INITIALIZER(dmic_ambiq_pdm_data##n.rx_dma_queue,            \
						   (char *)rx_dma_msgs_##n, sizeof(dma_msg),       \
						   CONFIG_PDM_AMBIQ_RX_BLOCK_COUNT),               \
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
