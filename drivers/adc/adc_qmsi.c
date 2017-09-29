/* adc_qmsi.c - QMSI ADC driver */

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

#include "qm_isr.h"
#include "qm_adc.h"
#include "clk.h"

enum {
	ADC_STATE_IDLE,
	ADC_STATE_BUSY,
	ADC_STATE_ERROR
};

struct adc_info  {
	atomic_t  state;
	struct k_sem device_sync_sem;
	struct k_sem sem;
};

static void adc_config_irq(void);
static qm_adc_config_t cfg;

#if (CONFIG_ADC_QMSI_INTERRUPT)
static struct adc_info *adc_context;

static void complete_callback(void *data, int error, qm_adc_status_t status,
			      qm_adc_cb_source_t source)
{
	if (adc_context) {
		if (error) {
			adc_context->state = ADC_STATE_ERROR;
		}
		k_sem_give(&adc_context->device_sync_sem);
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
static void adc_qmsi_enable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	adc_lock(info);
	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_NORM_CAL);
	qm_adc_calibrate(QM_ADC_0);
	adc_unlock(info);
}

#else
static void adc_qmsi_enable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	adc_lock(info);
	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_NORM_NO_CAL);
	adc_unlock(info);
}
#endif /* CONFIG_ADC_QMSI_CALIBRATION */

static void adc_qmsi_disable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	adc_lock(info);
	/* Go to deep sleep */
	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_DEEP_PWR_DOWN);
	adc_unlock(info);
}

#if (CONFIG_ADC_QMSI_POLL)
static int adc_qmsi_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	int i, ret = 0;
	qm_adc_xfer_t xfer;
	qm_adc_status_t status;

	struct adc_info *info = dev->driver_data;


	for (i = 0; i < seq_tbl->num_entries; i++) {

		xfer.ch = (qm_adc_channel_t *)&seq_tbl->entries[i].channel_id;
		/* Just one channel at the time using the Zephyr sequence table
		 */
		xfer.ch_len = 1;
		xfer.samples = (qm_adc_sample_t *)seq_tbl->entries[i].buffer;

		/* buffer length (bytes) the number of samples, the QMSI Driver
		 * does not allow more than QM_ADC_FIFO_LEN samples at the time
		 * in polling mode, if that happens, the qm_adc_convert api will
		 * return with an error
		 */
		xfer.samples_len =
		  (seq_tbl->entries[i].buffer_length)/sizeof(qm_adc_sample_t);

		xfer.callback = NULL;
		xfer.callback_data = NULL;

		cfg.window = seq_tbl->entries[i].sampling_delay;

		adc_lock(info);

		if (qm_adc_set_config(QM_ADC_0, &cfg) != 0) {
			ret =  -EINVAL;
			adc_unlock(info);
			break;
		}

		/* Run the conversion, here the function will poll for the
		 * samples. The function will constantly read  the status
		 * register to check if the number of samples required has been
		 * captured
		 */
		if (qm_adc_convert(QM_ADC_0, &xfer, &status) != 0) {
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
static int adc_qmsi_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	int i, ret = 0;
	qm_adc_xfer_t xfer;

	struct adc_info *info = dev->driver_data;

	for (i = 0; i < seq_tbl->num_entries; i++) {

		xfer.ch = (qm_adc_channel_t *)&seq_tbl->entries[i].channel_id;
		/* Just one channel at the time using the Zephyr sequence table */
		xfer.ch_len = 1;
		xfer.samples =
			(qm_adc_sample_t *)seq_tbl->entries[i].buffer;

		xfer.samples_len =
		  (seq_tbl->entries[i].buffer_length)/sizeof(qm_adc_sample_t);

		xfer.callback = complete_callback;
		xfer.callback_data = NULL;

		cfg.window = seq_tbl->entries[i].sampling_delay;

		adc_lock(info);

		if (qm_adc_set_config(QM_ADC_0, &cfg) != 0) {
			ret =  -EINVAL;
			adc_unlock(info);
			break;
		}

		/* ADC info used by the callbacks */
		adc_context = info;

		/* This is the interrupt driven API, will generate and interrupt and
		 * call the complete_callback function once the samples have been
		 * obtained
		 */
		if (qm_adc_irq_convert(QM_ADC_0, &xfer) != 0) {
			adc_context = NULL;
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
		adc_context = NULL;

		/* Successful Analog to Digital conversion */
		adc_unlock(info);
	}

	return ret;
}
#endif /* CONFIG_ADC_QMSI_POLL */

static const struct adc_driver_api api_funcs = {
	.enable  = adc_qmsi_enable,
	.disable = adc_qmsi_disable,
	.read    = adc_qmsi_read,
};

static int adc_qmsi_init(struct device *dev)
{
	struct adc_info *info = dev->driver_data;

	/* Enable the ADC and set the clock divisor */
	clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_ADC |
				CLK_PERIPH_ADC_REGISTER);
	/* ADC clock  divider*/
	clk_adc_set_div(CONFIG_ADC_QMSI_CLOCK_RATIO);

	/* Set up config */
	/* Clock cycles between the start of each sample */
	cfg.window = CONFIG_ADC_QMSI_SERIAL_DELAY;
	cfg.resolution = CONFIG_ADC_QMSI_SAMPLE_WIDTH;

	qm_adc_set_config(QM_ADC_0, &cfg);

	k_sem_init(&info->device_sync_sem, 0, UINT_MAX);

	k_sem_init(&info->sem, 1, UINT_MAX);
	info->state = ADC_STATE_IDLE;

	adc_config_irq();

	return 0;
}

static struct adc_info adc_info_dev;

DEVICE_AND_API_INIT(adc_qmsi, CONFIG_ADC_0_NAME, &adc_qmsi_init,
		    &adc_info_dev, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    (void *)&api_funcs);

static void adc_config_irq(void)
{
	IRQ_CONNECT(QM_IRQ_ADC_0_CAL_INT, CONFIG_ADC_0_IRQ_PRI,
		qm_adc_0_cal_isr, NULL, (IOAPIC_LEVEL | IOAPIC_HIGH));

	irq_enable(QM_IRQ_ADC_0_CAL_INT);

	QM_INTERRUPT_ROUTER->adc_0_cal_int_mask &= ~BIT(0);
}
