/* adc-ti-adc108s102.c - TI's ADC 108s102 driver implementation */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <misc/util.h>
#include <string.h>
#include <init.h>

#include <adc-ti-adc108s102.h>

#ifndef CONFIG_ADC_DEBUG
#define DBG(...) { ; }
#else
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_ADC_DEBUG */

static void _ti_adc108s102_sampling(int data, int unused)
{
	struct device *dev = INT_TO_POINTER(data);
	struct ti_adc108s102_data *adc = dev->driver_data;

	ARG_UNUSED(unused);

	DBG("Sampling!\n");

	/* SPI deals with uint8_t buffers so multiplying by 2 the length */
	spi_transceive(adc->spi,
			(uint8_t *) adc->cmd_buffer,
			adc->cmd_buf_len*2,
			(uint8_t *) adc->sampling_buffer,
			adc->sampling_buf_len*2);
}

static void _ti_adc108s102_completed(struct device *dev,
					enum adc_callback_type cb_type)
{
	struct ti_adc108s102_data *adc = dev->driver_data;

	adc->seq_table = NULL;

	adc->cb(dev, cb_type);
}

static inline void _ti_adc108s102_handle_result(struct device *dev)
{
	struct ti_adc108s102_data *adc = dev->driver_data;
	struct adc_seq_table *seq_table = adc->seq_table;
	struct ti_adc108s102_chan *chan;
	struct adc_seq_entry *entry;
	uint32_t s_i, i;

	DBG("_ti_adc108s102_handle_result()");

	for (i = 0, s_i = 1; i < seq_table->num_entries; i++, s_i++) {
		entry = &seq_table->entries[i];
		chan = &adc->chans[entry->channel_id];

		if (entry->buffer_length - chan->buf_idx == 0) {
			continue;
		}

		*((uint16_t *)entry->buffer+chan->buf_idx) =
				sys_be16_to_cpu(adc->sampling_buffer[s_i]);

		chan->buf_idx += 2;
	}
}

static int32_t _ti_adc108s102_prepare(struct device *dev)
{
	struct ti_adc108s102_data *adc = dev->driver_data;
	struct adc_seq_table *seq_table = adc->seq_table;
	struct ti_adc108s102_chan *chan;
	int32_t sampling_delay = 0;
	uint32_t i;

	adc->cmd_buf_len = 0;
	adc->sampling_buf_len = 1;

	for (i = 0; i < seq_table->num_entries; i++) {
		struct adc_seq_entry *entry = &seq_table->entries[i];

		/* No more space in the buffer? */
		chan = &adc->chans[entry->channel_id];
		if (entry->buffer_length - chan->buf_idx == 0) {
			continue;
		}

		adc->cmd_buffer[adc->cmd_buf_len] =
				ADC108S102_CHANNEL_CMD(entry->channel_id);

		adc->cmd_buf_len++;
		adc->sampling_buf_len++;

		sampling_delay = entry->sampling_delay;
	}

	if (adc->cmd_buf_len == 0) {
		/* We are done */
		_ti_adc108s102_completed(dev, ADC_CB_DONE);
		return 0;
	}

	/* dummy cmd byte */
	adc->cmd_buffer[adc->cmd_buf_len] = 0;
	adc->cmd_buf_len++;

	/* 64bits timestamp */
	adc->sampling_buf_len += 4;

	DBG("ADC108S102 is prepared...");

	return sampling_delay;
}

static inline void _ti_adc108s102_run_with_delay(struct device *dev,
							int32_t delay)
{
	struct ti_adc108s102_data *adc = dev->driver_data;

	fiber_delayed_start(adc->sampling_stack,
			    ADC108S102_SAMPLING_STACK_SIZE,
			    _ti_adc108s102_sampling,
			    POINTER_TO_INT(dev), 0,
			    0, 0, delay);
}

static void _ti_adc108s102_spi_cb(struct device *spi_dev,
					enum spi_cb_type cb_type,
					void *user_data)
{
	struct device *dev = user_data;
	int32_t delay;

	DBG("_ti_adc108s102_spi_cb(%d)\n", cb_type);

	switch (cb_type) {
	case SPI_CB_WRITE: /* fall through */
	case SPI_CB_READ: /* fall through */
	case SPI_CB_TRANSCEIVE:
		_ti_adc108s102_handle_result(dev);
		delay = _ti_adc108s102_prepare(dev);

		_ti_adc108s102_run_with_delay(dev, delay);
		break;
	case SPI_CB_ERROR: /* fall through */
	default:
		_ti_adc108s102_completed(dev, ADC_CB_ERROR);
		break;
	}
}

static void ti_adc108s102_enable(struct device *dev)
{
	/* There is nothing to be done. If there is no sampling going on,
	 * the chip will put itself on power-saving mode (that is because
	 * SPI will release CS) */
}

static void ti_adc108s102_disable(struct device *dev)
{
	/* Same issue as with ti_adc108s102_enable() */
}

static void ti_adc108s102_set_callback(struct device *dev, adc_callback_t cb)
{
	struct ti_adc108s102_data *adc = dev->driver_data;

	adc->cb = cb;
}

static inline int _verify_entries(struct adc_seq_table *seq_table)
{
	struct adc_seq_entry *entry;
	uint32_t chans_set = 0;
	int i;

	for (i = 0; i < seq_table->num_entries; i++) {
		entry = &seq_table->entries[i];

		if (entry->sampling_delay <= 0 ||
		    entry->channel_id > ADC108S102_CHANNELS) {
			return 0;
		}

		if (!entry->buffer_length) {
			continue;
		}

		chans_set++;
	}

	return chans_set;
}

static int ti_adc108s102_read(struct device *dev,
					struct adc_seq_table *seq_table)
{
	struct ti_adc108s102_config *config = dev->config->config_info;
	struct ti_adc108s102_data *adc = dev->driver_data;
	struct spi_config spi_conf;

	spi_conf.config = config->spi_config_flags;
	spi_conf.max_sys_freq = config->spi_freq;
	spi_conf.callback = _ti_adc108s102_spi_cb;

	if (spi_configure(adc->spi, &spi_conf, dev)) {
		return DEV_FAIL;
	}

	if (spi_slave_select(adc->spi, config->spi_slave)) {
		return DEV_FAIL;
	}

	/* Resetting all internal channel data */
	memset(adc->chans, 0, ADC108S102_CHANNELS_SIZE);

	if (_verify_entries(seq_table) == 0) {
		return DEV_INVALID_CONF;
	}

	adc->seq_table = seq_table;

	/* Requesting sampling right away */
	_ti_adc108s102_prepare(dev);
	_ti_adc108s102_sampling(POINTER_TO_INT(dev), 0);

	return DEV_OK;
}

struct adc_driver_api ti_adc108s102_api = {
	.enable = ti_adc108s102_enable,
	.disable = ti_adc108s102_disable,
	.set_callback = ti_adc108s102_set_callback,
	.read = ti_adc108s102_read,
};

int ti_adc108s102_init(struct device *dev)
{
	struct ti_adc108s102_config *config = dev->config->config_info;
	struct ti_adc108s102_data *adc = dev->driver_data;

	adc->spi = device_get_binding((char *)config->spi_port);
	if (!adc->spi) {
		return DEV_NOT_CONFIG;
	}

	DBG("ADC108s102 initialized\n");

	dev->driver_api = &ti_adc108s102_api;

	return DEV_OK;
}

#ifdef CONFIG_ADC_TI_ADC108S102_0

struct ti_adc108s102_data adc108s102_0_data;

struct ti_adc108s102_config adc108s102_0_config = {
	.spi_port = CONFIG_ADC_TI_ADC108S102_0_SPI_PORT_NAME,
	.spi_config_flags = CONFIG_ADC_TI_ADC108S102_0_SPI_CONFIGURATION,
	.spi_freq = CONFIG_ADC_TI_ADC108S102_0_SPI_MAX_FREQ,
	.spi_slave = CONFIG_ADC_TI_ADC108S102_0_SPI_SLAVE,
};

DECLARE_DEVICE_INIT_CONFIG(adc108s102_0, CONFIG_ADC_TI_ADC108S102_0_DRV_NAME,
			   ti_adc108s102_init, &adc108s102_0_config);

nano_early_init(adc108s102_0, &adc108s102_0_data);

#endif /* CONFIG_ADC_TI_ADC108S102_0 */
