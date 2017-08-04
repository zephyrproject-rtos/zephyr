/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family ADC (AFEC) driver.
 */

#include <errno.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <adc.h>

#define SYS_LOG_DOMAIN "dev/adc_sam_afec"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ADC_LEVEL
#include <logging/sys_log.h>

#define ADC_CHANNELS 12

struct adc_sam_dev_cfg {
	Afec *regs;
	const struct soc_gpio_pin *pin_trigger;
	void (*irq_config)(void);
	u8_t irq_id;
	u8_t periph_id;
};

struct channel_samples {
	u16_t *buffer;
	u32_t length;
};

struct adc_sam_dev_data {
	struct k_sem sem_meas;
	struct k_sem mutex_thread;
	u16_t active_channels;
	u16_t measured_channels;
	u16_t active_chan_last;
	struct channel_samples samples[ADC_CHANNELS];
};

#define CONF_ADC_PRESCALER ((SOC_ATMEL_SAM_MCK_FREQ_HZ / 15000000) - 1)

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) \
	((const struct adc_sam_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct adc_sam_dev_data *const)(dev)->driver_data)

static void adc_sam_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct adc_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct adc_sam_dev_data *const dev_data = DEV_DATA(dev);
	Afec *const afec = dev_cfg->regs;
	struct channel_samples *samples = dev_data->samples;
	u32_t isr_status;
	u16_t value;

	isr_status = afec->AFEC_ISR;
	for (int i = 0; i < ADC_CHANNELS; i++) {
		if (isr_status & BIT(i)) {
			/* Channel i has been sampled */
			afec->AFEC_CSELR = AFEC_CSELR_CSEL(i);
			value = (u16_t)(afec->AFEC_CDR);

			if (samples[i].length > 0) {
				/* Save sample */
				*samples[i].buffer = value;
				samples[i].length--;
				samples[i].buffer++;

				dev_data->measured_channels |= BIT(i);
			}
			if (samples[i].length == 0) {
				afec->AFEC_CHDR |= BIT(i);
				afec->AFEC_IDR |= BIT(i);
				dev_data->active_channels &= ~BIT(i);
			}
		}
	}

	if (dev_data->active_chan_last == dev_data->measured_channels) {
		if (!dev_data->active_channels) {
			/* No more conversions needed */
			k_sem_give(&dev_data->sem_meas);
		} else {
			/* Start another conversion with remaining channels */
			dev_data->measured_channels = 0;
			dev_data->active_chan_last = dev_data->active_channels;
			afec->AFEC_CR = AFEC_CR_START;
		}
	}
}

static int adc_sam_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	const struct adc_sam_dev_cfg *dev_cfg = DEV_CFG(dev);
	struct adc_sam_dev_data *const dev_data = DEV_DATA(dev);
	Afec *const afec = dev_cfg->regs;
	u8_t channel;
	u32_t num_samples;

	k_sem_take(&dev_data->mutex_thread, K_FOREVER);

	dev_data->active_channels = 0;

	/* Enable chosen channels */
	for (int i = 0; i < seq_tbl->num_entries; i++) {
		channel = seq_tbl->entries[i].channel_id;
		if (channel >= ADC_CHANNELS) {
			return -EINVAL;
		}

		/* Check and set number of requested samples */
		num_samples = seq_tbl->entries[i].buffer_length / sizeof(u16_t);
		if (!num_samples) {
			return -EINVAL;
		}
		dev_data->samples[channel].length = num_samples;

		/* Set start of sample buffer */
		dev_data->samples[channel].buffer =
			(u16_t *)seq_tbl->entries[i].buffer;

		/* Enable channel */
		dev_data->active_channels |= BIT(channel);
	}

	/* Enable chosen channels and their interrupts */
	afec->AFEC_CHER = dev_data->active_channels;
	afec->AFEC_IER = dev_data->active_channels;

	/* Start conversions */
	dev_data->measured_channels = 0;
	dev_data->active_chan_last = dev_data->active_channels;
	afec->AFEC_CR = AFEC_CR_START;
	k_sem_take(&dev_data->sem_meas, K_FOREVER);

	k_sem_give(&dev_data->mutex_thread);

	return 0;
}

static void adc_sam_configure(struct device *dev)
{
	const struct adc_sam_dev_cfg *dev_cfg = DEV_CFG(dev);
	Afec *const afec = dev_cfg->regs;

	/* Reset the AFEC */
	afec->AFEC_CR = AFEC_CR_SWRST;

	afec->AFEC_MR = AFEC_MR_TRGEN_DIS
		      | AFEC_MR_SLEEP_NORMAL
		      | AFEC_MR_FWUP_OFF
		      | AFEC_MR_FREERUN_OFF
		      | AFEC_MR_PRESCAL(CONF_ADC_PRESCALER)
		      | AFEC_MR_STARTUP_SUT96
		      | AFEC_MR_ONE
		      | AFEC_MR_USEQ_NUM_ORDER;

	/* Set all channels CM voltage to Vrefp/2 (512) */
	for (int i = 0; i < ADC_CHANNELS; i++) {
		afec->AFEC_CSELR = i;
		afec->AFEC_COCR = 512;
	}

	/* Enable PGA and Current Bias */
	afec->AFEC_ACR = AFEC_ACR_PGA0EN
		       | AFEC_ACR_PGA1EN
		       | AFEC_ACR_IBCTL(1);
}

static int adc_sam_initialize(struct device *dev)
{
	const struct adc_sam_dev_cfg *dev_cfg = DEV_CFG(dev);
	struct adc_sam_dev_data *const dev_data = DEV_DATA(dev);

	/* Initialize semaphores */
	k_sem_init(&dev_data->sem_meas, 0, 1);
	k_sem_init(&dev_data->mutex_thread, 1, 1);

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Enable module's clock */
	soc_pmc_peripheral_enable(dev_cfg->periph_id);

	/* Configure ADC */
	adc_sam_configure(dev);

	/* Enable module interrupt */
	irq_enable(dev_cfg->irq_id);

	SYS_LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct adc_driver_api adc_sam_driver_api = {
	.read    = adc_sam_read,
};

/* ADC_0 */

#ifdef CONFIG_ADC_0
static struct device DEVICE_NAME_GET(adc0_sam);

static void adc0_irq_config(void)
{
	IRQ_CONNECT(AFEC0_IRQn, CONFIG_ADC_0_IRQ_PRI, adc_sam_isr,
		    DEVICE_GET(adc0_sam), 0);
}

static const struct soc_gpio_pin pin_adc0 = PIN_AFE0_ADTRG;

static const struct adc_sam_dev_cfg adc0_sam_config = {
	.regs = AFEC0,
	.pin_trigger = &pin_adc0,
	.periph_id = ID_AFEC0,
	.irq_config = adc0_irq_config,
	.irq_id = AFEC0_IRQn,
};

static struct adc_sam_dev_data adc0_sam_data;

DEVICE_AND_API_INIT(adc0_sam, CONFIG_ADC_0_NAME, &adc_sam_initialize,
		    &adc0_sam_data, &adc0_sam_config, POST_KERNEL,
		    CONFIG_ADC_INIT_PRIORITY, &adc_sam_driver_api);
#endif

/* ADC_1 */

#ifdef CONFIG_ADC_1
static struct device DEVICE_NAME_GET(adc1_sam);

static void adc1_irq_config(void)
{
	IRQ_CONNECT(AFEC1_IRQn, CONFIG_ADC_1_IRQ_PRI, adc_sam_isr,
		    DEVICE_GET(adc1_sam), 0);
}

static const struct soc_gpio_pin pin_adc1 = PIN_AFE1_ADTRG;

static const struct adc_sam_dev_cfg adc1_sam_config = {
	.regs = AFEC1,
	.pin_trigger = &pin_adc1,
	.periph_id = ID_AFEC1,
	.irq_config = adc1_irq_config,
	.irq_id = AFEC1_IRQn,
};

static struct adc_sam_dev_data adc1_sam_data;

DEVICE_AND_API_INIT(adc1_sam, CONFIG_ADC_1_NAME, &adc_sam_initialize,
		    &adc1_sam_data, &adc1_sam_config, POST_KERNEL,
		    CONFIG_ADC_INIT_PRIORITY, &adc_sam_driver_api);
#endif
