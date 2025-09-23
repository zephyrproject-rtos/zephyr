/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/audio/amic.h>
#include <zephyr/cache.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/atomic.h>
#include <am_mcu_apollo.h>

#define DT_DRV_COMPAT ambiq_audadc

#define PREAMP_FULL_GAIN 12 /* Enable op amps for full gain range */
#define CH_A0_GAIN_DB    18 /* LGA = 36 */
#define CH_A1_GAIN_DB    18

LOG_MODULE_REGISTER(ambiq_audadc, CONFIG_AUDIO_AMIC_LOG_LEVEL);

enum audadc_ambiq_pm_policy_flag {
	AUDADC_AMBIQ_PM_POLICY_STATE_FLAG,
	AUDADC_AMBIQ_PM_POLICY_DTCM_FLAG,
	AUDADC_AMBIQ_PM_POLICY_FLAG_COUNT,
};

struct amic_ambiq_audadc_data {
	void *audadc_handler;
	struct k_mem_slab *mem_slab;
	void *mem_slab_buffer;
	struct k_sem dma_done_sem;
	int inst_idx;
	uint32_t block_size;
	uint32_t sample_num;
	uint8_t channel_num;
	uint8_t sample_size_bytes;
	am_hal_audadc_sample_t *lg_sample_buf;
	am_hal_offset_cal_coeffs_array_t offset_cal_array;

	am_hal_audadc_config_t audadc_cfg;
	am_hal_audadc_irtt_config_t irtt_cfg;
	am_hal_audadc_dma_config_t dma_cfg;

	ATOMIC_DEFINE(pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_FLAG_COUNT);

	enum amic_state amic_state;
};

struct amic_ambiq_audadc_cfg {
	void (*irq_config_func)(void);
};

static void amic_ambiq_audadc_pm_policy_state_lock_get(const struct device *dev)
{
	struct amic_ambiq_audadc_data *data = dev->data;

	if (!atomic_test_bit(data->pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_STATE_FLAG) &&
	    atomic_test_bit(data->pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_DTCM_FLAG)) {
		atomic_set_bit(data->pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_STATE_FLAG);
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

static void amic_ambiq_audadc_pm_policy_state_lock_put(const struct device *dev)
{
	struct amic_ambiq_audadc_data *data = dev->data;

	if (atomic_test_bit(data->pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_STATE_FLAG) &&
	    atomic_test_bit(data->pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_DTCM_FLAG)) {
		atomic_clear_bit(data->pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_STATE_FLAG);
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

static void amic_ambiq_audadc_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct amic_ambiq_audadc_data *data = dev->data;

	am_hal_audadc_interrupt_status(data->audadc_handler, &ui32Status, false);
	am_hal_audadc_interrupt_clear(data->audadc_handler, ui32Status);
	am_hal_audadc_interrupt_service(data->audadc_handler, ui32Status);

	if (ui32Status & AM_HAL_AUDADC_INT_DCMP) {
		k_sem_give(&data->dma_done_sem);
	}
}

static void amic_audadc_pga_init(void)
{
	/* Power up PrePGA */
	am_hal_audadc_refgen_powerup();

	am_hal_audadc_pga_powerup(0);
	am_hal_audadc_pga_powerup(1);

	am_hal_audadc_gain_set(0, 2 * PREAMP_FULL_GAIN);
	am_hal_audadc_gain_set(1, 2 * PREAMP_FULL_GAIN);

	/*  Turn on mic bias */
	am_hal_audadc_micbias_powerup(24);
	k_sleep(K_MSEC(400));
}

static void amic_audadc_slot_config(void *audadc_handle)
{
	am_hal_audadc_slot_config_t slot_config;

	/* Set up an AUDADC slot */
	slot_config.eMeasToAvg = AM_HAL_AUDADC_SLOT_AVG_1;
	slot_config.ePrecisionMode = AM_HAL_AUDADC_SLOT_12BIT;
	slot_config.ui32TrkCyc = 34;
	slot_config.eChannel = AM_HAL_AUDADC_SLOT_CHSEL_SE0;
	slot_config.bWindowCompare = false;
	slot_config.bEnabled = true;

	if (AM_HAL_STATUS_SUCCESS != am_hal_audadc_configure_slot(audadc_handle, 0, &slot_config)) {
		LOG_ERR("Error - configuring AUDADC Slot 0 failed.\n");
	}

	slot_config.eChannel = AM_HAL_AUDADC_SLOT_CHSEL_SE1;
	if (AM_HAL_STATUS_SUCCESS != am_hal_audadc_configure_slot(audadc_handle, 1, &slot_config)) {
		LOG_ERR("Error - configuring AUDADC Slot 1 failed.\n");
	}
}

static int amic_ambiq_audadc_init(const struct device *dev)
{
	struct amic_ambiq_audadc_data *data = dev->data;

	amic_audadc_pga_init();

	/* Initialize the AUDADC and get the handle. */
	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_audadc_initialize(data->inst_idx, &data->audadc_handler)) {
		LOG_ERR("Error - reservation of the AUDADC instance failed.\n");
	}

	/* Power on the AUDADC. */
	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_audadc_power_control(data->audadc_handler, AM_HAL_SYSCTRL_WAKE, false)) {
		LOG_ERR("Error - AUDADC power on failed.\n");
	}

	data->amic_state = AMIC_STATE_INITIALIZED;

	return 0;
}

static int amic_ambiq_audadc_configure(const struct device *dev, struct amic_cfg *dev_config)
{
	struct amic_ambiq_audadc_data *data = dev->data;
	const struct amic_ambiq_audadc_cfg *config = dev->config;

	struct pcm_stream_cfg *stream = &dev_config->streams[0];

	if (data->amic_state == AMIC_STATE_ACTIVE) {
		LOG_ERR("Cannot configure device while it is active");
		return -EBUSY;
	}

	if (stream->pcm_width != 16) {
		LOG_ERR("Only 16-bit samples are supported");
		return -EINVAL;
	}

	/*
	 * Set up the AUDADC configuration parameters. These settings are reasonable
	 * for accurate measurements at a low sample rate.
	 */
	data->audadc_cfg.eClock = AM_HAL_AUDADC_CLKSEL_HFRC_48MHz;
	data->audadc_cfg.ePolarity = AM_HAL_AUDADC_TRIGPOL_RISING;
	data->audadc_cfg.eTrigger = AM_HAL_AUDADC_TRIGSEL_SOFTWARE;
	data->audadc_cfg.eClockMode = AM_HAL_AUDADC_CLKMODE_LOW_POWER;
	data->audadc_cfg.ePowerMode = AM_HAL_AUDADC_LPMODE1;
	data->audadc_cfg.eSampMode = AM_HAL_AUDADC_SAMPMODE_LP;
	data->audadc_cfg.eRepeat = AM_HAL_AUDADC_REPEATING_SCAN;
	data->audadc_cfg.eRepeatTrigger = AM_HAL_AUDADC_RPTTRIGSEL_INT;

	/* Set up internal repeat trigger timer */
	data->irtt_cfg.eClkDiv = AM_HAL_AUDADC_RPTT_CLK_DIV8;

	/* sample rate = eClock/eClkDiv/(ui32IrttCountMax) */
	data->irtt_cfg.ui32IrttCountMax = 375;
	data->irtt_cfg.bIrttEnable = true;

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_audadc_configure(data->audadc_handler, &data->audadc_cfg)) {
		LOG_ERR("Error - configuring AUDADC failed.\n");
	}

	/* Set up internal repeat trigger timer */
	am_hal_audadc_configure_irtt(data->audadc_handler, &data->irtt_cfg);

	data->block_size = stream->block_size;

	data->sample_size_bytes = 2;
	data->sample_num = stream->block_size / data->sample_size_bytes;
	data->mem_slab = stream->mem_slab;
	data->channel_num = stream->channel_num;

	/* Configure DMA and target address.*/
	data->dma_cfg.ui32SampleCount = data->sample_num;
	data->dma_cfg.ui32TargetAddressReverse = data->dma_cfg.ui32TargetAddress +
						 (data->dma_cfg.ui32SampleCount * sizeof(uint32_t));

	/* One-time compute: whether DMA buffer region intersects DTCM. */
	atomic_set_bit_to(
		data->pm_policy_flag, AUDADC_AMBIQ_PM_POLICY_DTCM_FLAG,
		ambiq_buf_in_dtcm((uintptr_t)data->dma_cfg.ui32TargetAddress,
				  (size_t)data->dma_cfg.ui32SampleCount * sizeof(uint32_t)));

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_audadc_configure_dma(data->audadc_handler, &(data->dma_cfg))) {
		LOG_ERR("Error - configuring AUDADC DMA failed.\n");
	}

	am_hal_audadc_interrupt_clear(data->audadc_handler,
				      AM_HAL_AUDADC_INT_DERR | AM_HAL_AUDADC_INT_DCMP);
	am_hal_audadc_interrupt_enable(data->audadc_handler,
				       AM_HAL_AUDADC_INT_DERR | AM_HAL_AUDADC_INT_DCMP);
	config->irq_config_func();

	am_hal_audadc_gain_config_t gain_config;

	/* Gain setting */
	gain_config.ui32LGA = (uint32_t)((float)CH_A0_GAIN_DB * 2 + 12); /* LG 12dB, LGA = 36 */
	gain_config.ui32HGADELTA = (uint32_t)((float)(CH_A1_GAIN_DB * 2 + 12)) -
				   gain_config.ui32LGA; /* HGDelta = 12 */

	gain_config.eUpdateMode = AM_HAL_AUDADC_GAIN_UPDATE_IMME;
	am_hal_audadc_internal_pga_config(data->audadc_handler, &gain_config);

	amic_audadc_slot_config(data->audadc_handler);

#if CONFIG_AUDADC_AMBIQ_DC_OFFSET_CALIBRATION
	/* Calculate DC offset calibration parameter. */
	int ret = am_hal_audadc_slot_dc_offset_calculate(data->audadc_handler, 2,
							 &(data->offset_cal_array));
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Error - failed to calculate offset calibration parameter. %d\n", ret);
	}
#endif

	data->amic_state = AMIC_STATE_CONFIGURED;

	return 0;
}

static void am_audadc_dma_trigger(const struct device *dev)
{
	struct amic_ambiq_audadc_data *data = dev->data;

	/* Enable internal repeat trigger timer */
	am_hal_audadc_irtt_enable(data->audadc_handler);

	/* Trigger the ADC sampling for the first time manually. */
	if (AM_HAL_STATUS_SUCCESS != am_hal_audadc_dma_transfer_start(data->audadc_handler)) {
		LOG_ERR("Error - triggering the AUDADC failed.\n");
	}
}

static int amic_ambiq_audadc_trigger(const struct device *dev, enum amic_trigger cmd)
{
	struct amic_ambiq_audadc_data *data = dev->data;

	switch (cmd) {
	case AMIC_TRIGGER_PAUSE:
	case AMIC_TRIGGER_STOP:
		if (data->amic_state == AMIC_STATE_ACTIVE) {
			/* Enable internal repeat trigger timer */
			am_hal_audadc_irtt_disable(data->audadc_handler);

			am_hal_audadc_interrupt_clear(
				data->audadc_handler,
				(AM_HAL_AUDADC_INT_DERR | AM_HAL_AUDADC_INT_DCMP));
			am_hal_audadc_interrupt_disable(
				data->audadc_handler,
				(AM_HAL_AUDADC_INT_DERR | AM_HAL_AUDADC_INT_DCMP));

			am_hal_audadc_disable(data->audadc_handler);
			data->amic_state = AMIC_STATE_PAUSED;
		}
		break;

	case AMIC_TRIGGER_RELEASE:
	case AMIC_TRIGGER_START:
		if (data->amic_state == AMIC_STATE_PAUSED ||
		    data->amic_state == AMIC_STATE_CONFIGURED) {
			am_hal_audadc_enable(data->audadc_handler);
			am_audadc_dma_trigger(dev);
			data->amic_state = AMIC_STATE_ACTIVE;
		}
		break;

	default:
		LOG_ERR("Invalid command: %d", cmd);
		return -EINVAL;
	}

	return 0;
}

static int amic_ambiq_audadc_read(const struct device *dev, uint8_t stream, void **buffer,
				  size_t *size, int32_t timeout)
{
	struct amic_ambiq_audadc_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if (data->amic_state != AMIC_STATE_ACTIVE) {
		LOG_ERR("Device is not activated");
		return -EIO;
	}

	ret = k_sem_take(&data->dma_done_sem, SYS_TIMEOUT_MS(timeout));

	(void)pm_device_runtime_get(dev);

	amic_ambiq_audadc_pm_policy_state_lock_get(dev);

	if (ret != 0) {
		LOG_ERR("No audio data to be read %d", ret);
	} else {
		ret = k_mem_slab_alloc(data->mem_slab, &data->mem_slab_buffer, K_NO_WAIT);

		uint32_t *audadc_data_buf =
			(uint32_t *)am_hal_audadc_dma_get_buffer(data->audadc_handler);

#if CONFIG_AUDADC_AMBIQ_HANDLE_CACHE
		if (!buf_in_nocache((uintptr_t)audadc_data_buf, data->block_size)) {
			sys_cache_data_invd_range(audadc_data_buf, data->block_size);
		}
#endif
		uint32_t pcm_sample_cnt = data->sample_num;

#if CONFIG_AUDADC_AMBIQ_DC_OFFSET_CALIBRATION
		am_hal_audadc_samples_read(data->audadc_handler, audadc_data_buf, &pcm_sample_cnt,
					   true, data->lg_sample_buf, false, NULL,
					   &data->offset_cal_array);
#else
		am_hal_audadc_samples_read(data->audadc_handler, audadc_data_buf, &pcm_sample_cnt,
					   true, data->lg_sample_buf, false, NULL, NULL);
#endif
		if (pcm_sample_cnt != data->sample_num) {
			LOG_ERR("Error - AUDADC read req = %d ret = %d\n", data->sample_num,
				pcm_sample_cnt);
		}

		if (data->channel_num == 1) {
			int16_t *pi16PcmBuf = (int16_t *)data->mem_slab_buffer;

			for (int indx = 0; indx < pcm_sample_cnt; indx++) {
				pi16PcmBuf[indx] =
					data->lg_sample_buf[indx]
						.int16Sample; /* Low gain samples (MIC0) */
							      /* data to left channel. */
			}
		} else if (data->channel_num == 2) {
			uint32_t left_ch_cnt = 0;
			uint32_t right_ch_cnt = 0;
			uint32_t *pcm_buf_ptr = (uint32_t *)data->mem_slab_buffer;

			for (int indx = 0; indx < pcm_sample_cnt; indx++) {
				if (data->lg_sample_buf[indx].ui16AudChannel == 0) {
					pcm_buf_ptr[left_ch_cnt++] =
						(data->lg_sample_buf[indx].int16Sample &
						 0x0000FFFF);
				} else {
					pcm_buf_ptr[right_ch_cnt++] |=
						((data->lg_sample_buf[indx].int16Sample)
						 << 16);
				}
			}
		}

		*size = pcm_sample_cnt * data->sample_size_bytes;
		*buffer = data->mem_slab_buffer;
	}

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_audadc_interrupt_clear(data->audadc_handler, 0xFFFFFFFF)) {
		LOG_ERR("Error - clearing the AUDADC interrupts failed.\n");
	}

	amic_ambiq_audadc_pm_policy_state_lock_put(dev);

	(void)pm_device_runtime_put(dev);

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int amic_ambiq_audadc_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct amic_ambiq_audadc_data *data = dev->data;
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

	ret = am_hal_audadc_power_control(data->audadc_handler, status, true);

	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("am_hal_audadc_power_control failed: %d", ret);
		return -EPERM;
	} else {
		return 0;
	}
}
#endif

static const struct _amic_ops amic_ambiq_ops = {
	.configure = amic_ambiq_audadc_configure,
	.trigger = amic_ambiq_audadc_trigger,
	.read = amic_ambiq_audadc_read,
};

#define AMBIQ_AUDADC_DEFINE(n)                                                                     \
	static void audadc_irq_config_func_##n(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), amic_ambiq_audadc_isr,      \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static uint32_t audadc_dma_tcb_buf##n[DT_INST_PROP_OR(n, audadc_buf_size_samples, 1536)]   \
		__attribute__((section(DT_INST_PROP_OR(n, audadc_buf_location, ".data"))))         \
		__aligned(CONFIG_AUDADC_AMBIQ_BUFFER_ALIGNMENT);                                   \
	static am_hal_audadc_sample_t                                                              \
		audadc_lg_sample_buf##n[DT_INST_PROP_OR(n, audadc_buf_size_samples, 1536)];        \
	static struct amic_ambiq_audadc_data amic_ambiq_audadc_data##n = {                         \
		.dma_done_sem = Z_SEM_INITIALIZER(amic_ambiq_audadc_data##n.dma_done_sem, 0, 1),   \
		.inst_idx = n,                                                                     \
		.block_size = 0,                                                                   \
		.sample_num = 0,                                                                   \
		.amic_state = AMIC_STATE_UNINIT,                                                   \
		.dma_cfg.ui32TargetAddress = (uint32_t)audadc_dma_tcb_buf##n,                      \
		.lg_sample_buf = audadc_lg_sample_buf##n,                                          \
	};                                                                                         \
	static const struct amic_ambiq_audadc_cfg amic_ambiq_audadc_cfg##n = {                     \
		.irq_config_func = audadc_irq_config_func_##n,                                     \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, amic_ambiq_audadc_pm_action);                                  \
	DEVICE_DT_INST_DEFINE(n, amic_ambiq_audadc_init, NULL, &amic_ambiq_audadc_data##n,         \
			      &amic_ambiq_audadc_cfg##n, POST_KERNEL,                              \
			      CONFIG_AUDIO_AMIC_INIT_PRIORITY, &amic_ambiq_ops);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_AUDADC_DEFINE)
