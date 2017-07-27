/* adc_qmsi.c - QMSI ADC Sensor Subsystem driver */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <init.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <board.h>
#include <adc.h>
#include <arch/cpu.h>
#include <atomic.h>

#include "qm_ss_isr.h"
#include "qm_ss_adc.h"
#include "ss_clk.h"

enum {
	ADC_STATE_IDLE,
	ADC_STATE_BUSY,
	ADC_STATE_ERROR
};

struct adc_info  {
	atomic_t  state;
	struct k_sem device_sync_sem;
	struct k_sem sem;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_ss_adc_context_t adc_ctx;
#endif
};

static void adc_config_irq(void);
static qm_ss_adc_config_t cfg;

#if (CONFIG_ADC_QMSI_INTERRUPT)

static void complete_callback(void *data, int error, qm_ss_adc_status_t status,
							  qm_ss_adc_cb_source_t source)
{
	ARG_UNUSED(status);
	ARG_UNUSED(source);

	struct device *dev = data;

	struct adc_info *info = dev->driver_data;

	if (info) {
		if (error) {
			info->state = ADC_STATE_ERROR;
		}
		k_sem_give(&info->device_sync_sem);
	}
}

#endif

static void adc_lock(struct adc_info *data)
{
	k_sem_take(&data->sem, K_FOREVER);
	data->state = ADC_STATE_BUSY;

}
static void adc_unlock(struct adc_info *data)
{
	k_sem_give(&data->sem);
	data->state = ADC_STATE_IDLE;

}
#if (CONFIG_ADC_QMSI_CALIBRATION)
static void adc_qmsi_ss_enable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	adc_lock(info);
	qm_ss_adc_set_mode(QM_SS_ADC_0, QM_SS_ADC_MODE_NORM_CAL);
	qm_ss_adc_calibrate(QM_SS_ADC_0);
	adc_unlock(info);
}

#else
static void adc_qmsi_ss_enable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	adc_lock(info);
	qm_ss_adc_set_mode(QM_SS_ADC_0, QM_SS_ADC_MODE_NORM_NO_CAL);
	adc_unlock(info);
}
#endif /* CONFIG_ADC_QMSI_CALIBRATION */

static void adc_qmsi_ss_disable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	adc_lock(info);
	/* Go to deep sleep */
	qm_ss_adc_set_mode(QM_SS_ADC_0, QM_SS_ADC_MODE_DEEP_PWR_DOWN);
	adc_unlock(info);
}

#if (CONFIG_ADC_QMSI_POLL)
static int adc_qmsi_ss_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	int i, ret = 0;
	qm_ss_adc_xfer_t xfer;
	qm_ss_adc_status_t status;

	struct adc_info *info = dev->driver_data;


	for (i = 0; i < seq_tbl->num_entries; i++) {

		xfer.ch = (qm_ss_adc_channel_t *)&seq_tbl->entries[i].channel_id;
		/* Just one channel at the time using the Zephyr sequence table
		 */
		xfer.ch_len = 1;
		xfer.samples = (qm_ss_adc_sample_t *)seq_tbl->entries[i].buffer;

		/* buffer length (bytes) the number of samples, the QMSI Driver
		 * does not allow more than QM_ADC_FIFO_LEN samples at the time
		 * in polling mode, if that happens, the qm_adc_convert api will
		 * return with an error
		 */
		xfer.samples_len =
		  (seq_tbl->entries[i].buffer_length)/sizeof(qm_ss_adc_sample_t);

		xfer.callback = NULL;
		xfer.callback_data = NULL;

		cfg.window = seq_tbl->entries[i].sampling_delay;

		adc_lock(info);

		if (qm_ss_adc_set_config(QM_SS_ADC_0, &cfg) != 0) {
			ret =  -EINVAL;
			adc_unlock(info);
			break;
		}

		/* Run the conversion, here the function will poll for the
		 * samples. The function will constantly read  the status
		 * register to check if the number of samples required has been
		 * captured
		 */
		if (qm_ss_adc_convert(QM_SS_ADC_0, &xfer, &status) != 0) {
			ret =  -EIO;
			adc_unlock(info);
			break;
		}

		/* Successful Analog to Digital conversion */
		adc_unlock(info);
	}

	return ret;
}
#else
static int adc_qmsi_ss_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	int i, ret = 0;
	qm_ss_adc_xfer_t xfer;

	struct adc_info *info = dev->driver_data;

	for (i = 0; i < seq_tbl->num_entries; i++) {

		xfer.ch = (qm_ss_adc_channel_t *)&seq_tbl->entries[i].channel_id;
		/* Just one channel at the time using the Zephyr sequence table */
		xfer.ch_len = 1;
		xfer.samples =
			(qm_ss_adc_sample_t *)seq_tbl->entries[i].buffer;

		xfer.samples_len =
		  (seq_tbl->entries[i].buffer_length)/sizeof(qm_ss_adc_sample_t);

		xfer.callback = complete_callback;
		xfer.callback_data = dev;

		cfg.window = seq_tbl->entries[i].sampling_delay;

		adc_lock(info);

		if (qm_ss_adc_set_config(QM_SS_ADC_0, &cfg) != 0) {
			ret =  -EINVAL;
			adc_unlock(info);
			break;
		}

		/* This is the interrupt driven API, will generate and interrupt and
		 * call the complete_callback function once the samples have been
		 * obtained
		 */
		if (qm_ss_adc_irq_convert(QM_SS_ADC_0, &xfer) != 0) {
			ret =  -EIO;
			adc_unlock(info);
			break;
		}

		/* Wait for the interrupt to finish */
		k_sem_take(&info->device_sync_sem, K_FOREVER);

		if (info->state == ADC_STATE_ERROR) {
			ret =  -EIO;
			adc_unlock(info);
			break;
		}
		/* Successful Analog to Digital conversion */
		adc_unlock(info);
	}

	return ret;
}
#endif /* CONFIG_ADC_QMSI_POLL */

static void adc_qmsi_ss_rx_isr(void *arg)
{
	ARG_UNUSED(arg);
	qm_ss_adc_0_isr(NULL);
}

static void adc_qmsi_ss_err_isr(void *arg)
{
	ARG_UNUSED(arg);
	qm_ss_adc_0_error_isr(NULL);
}

static const struct adc_driver_api api_funcs = {
	.enable  = adc_qmsi_ss_enable,
	.disable = adc_qmsi_ss_disable,
	.read    = adc_qmsi_ss_read,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void adc_qmsi_ss_set_power_state(struct device *dev,
					u32_t power_state)
{
	struct adc_info *context = dev->driver_data;

	context->device_power_state = power_state;
}

static u32_t adc_qmsi_ss_get_power_state(struct device *dev)
{
	struct adc_info *context = dev->driver_data;

	return context->device_power_state;
}

static int adc_qmsi_ss_suspend_device(struct device *dev)
{
	struct adc_info *context = dev->driver_data;

	qm_ss_adc_save_context(QM_SS_ADC_0, &context->adc_ctx);

	adc_qmsi_ss_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int adc_qmsi_ss_resume_device_from_suspend(struct device *dev)
{
	struct adc_info *context = dev->driver_data;

	qm_ss_adc_restore_context(QM_SS_ADC_0, &context->adc_ctx);

	adc_qmsi_ss_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

static int adc_qmsi_ss_device_ctrl(struct device *dev, u32_t ctrl_command,
				   void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return adc_qmsi_ss_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return adc_qmsi_ss_resume_device_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = adc_qmsi_ss_get_power_state(dev);
	}

	return 0;
}
#else
#define adc_qmsi_ss_set_power_state(...)
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */
static int adc_qmsi_ss_init(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	/* Set up config */
	/* Clock cycles between the start of each sample */
	cfg.window = CONFIG_ADC_QMSI_SERIAL_DELAY;
	cfg.resolution = CONFIG_ADC_QMSI_SAMPLE_WIDTH;

	qm_ss_adc_set_config(QM_SS_ADC_0, &cfg);

	ss_clk_adc_enable();
	ss_clk_adc_set_div(CONFIG_ADC_QMSI_CLOCK_RATIO);
	k_sem_init(&info->device_sync_sem, 0, UINT_MAX);

	k_sem_init(&info->sem, 1, UINT_MAX);
	info->state = ADC_STATE_IDLE;

	adc_config_irq();

	adc_qmsi_ss_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

static struct adc_info adc_info_dev;

DEVICE_DEFINE(adc_qmsi_ss, CONFIG_ADC_0_NAME, &adc_qmsi_ss_init,
	      adc_qmsi_ss_device_ctrl, &adc_info_dev, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api_funcs);

static void adc_config_irq(void)
{
	u32_t *scss_intmask;

	IRQ_CONNECT(IRQ_ADC_IRQ, CONFIG_ADC_0_IRQ_PRI,
				adc_qmsi_ss_rx_isr, DEVICE_GET(adc_qmsi_ss), 0);
	irq_enable(IRQ_ADC_IRQ);

	IRQ_CONNECT(IRQ_ADC_ERR, CONFIG_ADC_0_IRQ_PRI,
		    adc_qmsi_ss_err_isr, DEVICE_GET(adc_qmsi_ss), 0);
	irq_enable(IRQ_ADC_ERR);

	scss_intmask =
		(u32_t *)&QM_INTERRUPT_ROUTER->ss_adc_0_error_int_mask;
	*scss_intmask &= ~BIT(8);

	scss_intmask = (u32_t *)&QM_INTERRUPT_ROUTER->ss_adc_0_int_mask;
	*scss_intmask &= ~BIT(8);
}
