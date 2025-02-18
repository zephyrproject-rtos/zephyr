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

#include <am_mcu_apollo.h>

#define DT_DRV_COMPAT ambiq_pdm

LOG_MODULE_REGISTER(ambiq_pdm, CONFIG_AUDIO_DMIC_LOG_LEVEL);

typedef int (*ambiq_pdm_pwr_func_t)(void);
#define PWRCTRL_MAX_WAIT_US 5

struct dmic_ambiq_pdm_data {
	void *pdm_handler;
	struct k_mem_slab *mem_slab;
	void *mem_slab_buffer;
	struct k_sem dma_done_sem;
	int inst_idx;
	uint32_t block_size;
	uint32_t sample_num;
	am_hal_pdm_config_t pdm_cfg;
	am_hal_pdm_transfer_t pdm_transfer;

	enum dmic_state dmic_state;
};

struct dmic_ambiq_pdm_cfg {
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
	ambiq_pdm_pwr_func_t pwr_func;
};

static __aligned(32) struct {
	__aligned(32) uint32_t buf[CONFIG_PDM_DMA_TCB_BUFFER_SIZE]; // CONFIG_PDM_DMA_TCB_BUFFER_SIZE
								    // should be 2 x block_size
} pdm_dma_tcb_buf[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)]
	__attribute__((__section__(".dtcm_data")));

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

	ret = config->pwr_func();

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config PDM pins\n");
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

	if (stream->pcm_width != 16) {
		LOG_ERR("Only 16-bit samples are supported");
		return -EINVAL;
	}

	channel->act_num_streams = 1;
	channel->act_chan_map_hi = 0;
	channel->act_chan_map_lo = channel->req_chan_map_lo;

	data->pdm_cfg.ePDMClkSpeed = AM_HAL_PDM_CLK_HFRC_24MHZ;

	//  1.5MHz PDM CLK OUT:
	//      AM_HAL_PDM_CLK_24MHZ, AM_HAL_PDM_MCLKDIV_1, AM_HAL_PDM_PDMA_CLKO_DIV7
	//  15.625KHz 24bit Sampling:
	//      DecimationRate = 48
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
	data->sample_num = stream->block_size / sizeof(int16_t);
	data->mem_slab = stream->mem_slab;

	data->dmic_state = DMIC_STATE_CONFIGURED;

	//
	// Configure DMA and target address.
	//
	data->pdm_transfer.ui32TotalCount = data->sample_num * sizeof(uint32_t);
	data->pdm_transfer.ui32TargetAddr = (uint32_t)(pdm_dma_tcb_buf[data->inst_idx].buf);
	data->pdm_transfer.ui32TargetAddrReverse =
		data->pdm_transfer.ui32TargetAddr + data->pdm_transfer.ui32TotalCount;

	return 0;
}

static void am_pdm_dma_trigger(const struct device *dev)
{
	struct dmic_ambiq_pdm_data *data = dev->data;

	//
	// Start the data transfer.
	//
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

	if (ret != 0) {
		LOG_DBG("No audio data to be read %d", ret);
	} else {
		ret = k_mem_slab_alloc(data->mem_slab, &data->mem_slab_buffer, K_NO_WAIT);

		uint32_t *pdm_data_buf = (uint32_t *)am_hal_pdm_dma_get_buffer(data->pdm_handler);

		//
		// PDM DMA is 32-bit datawidth for each sample, so we need to invalidate 2x
		// block_size
		//
		am_hal_cachectrl_dcache_invalidate(
			&(am_hal_cachectrl_range_t){(uint32_t)pdm_data_buf, data->block_size * 2},
			false);

		//
		// Re-arrange data
		//
		uint8_t *temp1 = (uint8_t *)data->mem_slab_buffer;
		for (uint32_t i = 0; i < data->sample_num; i++) {
			temp1[2 * i] = (pdm_data_buf[i] & 0xFF00) >> 8U;
			temp1[2 * i + 1] = (pdm_data_buf[i] & 0xFF0000) >> 16U;
		}
		*buffer = temp1;

		// LOG_DBG("Released buffer %p", *buffer);

		*size = data->block_size;
	}

	return ret;
}

static const struct _dmic_ops dmic_ambiq_ops = {
	.configure = dmic_ambiq_pdm_configure,
	.trigger = dmic_ambiq_pdm_trigger,
	.read = dmic_ambiq_pdm_read,
};

#define AMBIQ_PDM_DEFINE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static int pwr_on_ambiq_pdm_##n(void)                                                      \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(PWRCTRL_MAX_WAIT_US);                                                  \
		return 0;                                                                          \
	}                                                                                          \
	static void pdm_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), dmic_ambiq_pdm_isr,         \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct dmic_ambiq_pdm_data dmic_ambiq_pdm_data##n = {                               \
		.dma_done_sem = Z_SEM_INITIALIZER(dmic_ambiq_pdm_data##n.dma_done_sem, 0, 1),      \
		.inst_idx = n,                                                                     \
		.block_size = 0,                                                                   \
		.sample_num = 0,                                                                   \
		.dmic_state = DMIC_STATE_UNINIT,                                                   \
	};                                                                                         \
	static const struct dmic_ambiq_pdm_cfg dmic_ambiq_pdm_cfg##n = {                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = pdm_irq_config_func_##n,                                        \
		.pwr_func = pwr_on_ambiq_pdm_##n,                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, dmic_ambiq_pdm_init, NULL, &dmic_ambiq_pdm_data##n,               \
			      &dmic_ambiq_pdm_cfg##n, POST_KERNEL,                                 \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_ambiq_ops);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_PDM_DEFINE)
