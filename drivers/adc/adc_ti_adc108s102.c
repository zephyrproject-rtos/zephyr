/* adc-ti-adc108s102.c - TI's ADC 108s102 driver implementation */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <misc/util.h>
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ADC_LEVEL
#include <logging/sys_log.h>
#include <string.h>
#include <init.h>

#include "adc_ti_adc108s102.h"

static inline int _ti_adc108s102_sampling(struct device *dev)
{
	struct ti_adc108s102_data *adc = dev->driver_data;

	SYS_LOG_DBG("Sampling!\n");

	/* SPI deals with uint8_t buffers so multiplying by 2 the length */
	return spi_transceive(adc->spi, adc->cmd_buffer,
			      adc->cmd_buf_len * 2,
			      adc->sampling_buffer,
			      adc->sampling_buf_len * 2);
}

static inline void _ti_adc108s102_handle_result(struct device *dev)
{
	struct ti_adc108s102_data *adc = dev->driver_data;
	struct adc_seq_table *seq_table = adc->seq_table;
	struct ti_adc108s102_chan *chan;
	struct adc_seq_entry *entry;
	uint32_t s_i, i;

	SYS_LOG_DBG("_ti_adc108s102_handle_result()");

	for (i = 0, s_i = 1; i < seq_table->num_entries; i++, s_i++) {
		entry = &seq_table->entries[i];
		chan = &adc->chans[entry->channel_id];

		if (entry->buffer_length - chan->buf_idx == 0) {
			continue;
		}

		*((uint16_t *)(entry->buffer+chan->buf_idx)) =
				ADC108S102_RESULT(adc->sampling_buffer[s_i]);

		chan->buf_idx += 2;
	}
}

static inline int32_t _ti_adc108s102_prepare(struct device *dev)
{
	struct ti_adc108s102_data *adc = dev->driver_data;
	struct adc_seq_table *seq_table = adc->seq_table;
	struct ti_adc108s102_chan *chan;
	int32_t sampling_delay = 0;
	uint32_t i;

	adc->cmd_buf_len = 0;
	adc->sampling_buf_len = 1; /* Counting the dummy byte */

	for (i = 0; i < seq_table->num_entries; i++) {
		struct adc_seq_entry *entry = &seq_table->entries[i];

		/* No more space in the buffer? */
		chan = &adc->chans[entry->channel_id];
		if (entry->buffer_length - chan->buf_idx == 0) {
			continue;
		}

		SYS_LOG_DBG("Requesting channel %d\n", entry->channel_id);
		adc->cmd_buffer[adc->cmd_buf_len] =
				ADC108S102_CHANNEL_CMD(entry->channel_id);

		adc->cmd_buf_len++;
		adc->sampling_buf_len++;

		sampling_delay = entry->sampling_delay;
	}

	if (adc->cmd_buf_len == 0) {
		return ADC108S102_DONE;
	}

	/* dummy cmd byte */
	adc->cmd_buffer[adc->cmd_buf_len] = 0;
	adc->cmd_buf_len++;

	SYS_LOG_DBG("ADC108S102 is prepared...");

	return sampling_delay;
}

static void ti_adc108s102_enable(struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * There is nothing to be done. If there is no sampling going on,
	 * the chip will put itself on power-saving mode (that is because
	 * SPI will release CS)
	 */
}

static void ti_adc108s102_disable(struct device *dev)
{
	ARG_UNUSED(dev);

	/* Same issue as with ti_adc108s102_enable() */
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
	const struct ti_adc108s102_config *config = dev->config->config_info;
	struct ti_adc108s102_data *adc = dev->driver_data;
	struct spi_config spi_conf;
	int ret = 0;
	int32_t delay;

	spi_conf.config = config->spi_config_flags;
	spi_conf.max_sys_freq = config->spi_freq;

	if (spi_configure(adc->spi, &spi_conf)) {
		return -EIO;
	}

	if (spi_slave_select(adc->spi, config->spi_slave)) {
		return -EIO;
	}

	/* Resetting all internal channel data */
	memset(adc->chans, 0, ADC108S102_CHANNELS_SIZE);

	if (_verify_entries(seq_table) == 0) {
		return -EINVAL;
	}

	adc->seq_table = seq_table;

	/* Sampling */
	while (1) {
		delay = _ti_adc108s102_prepare(dev);
		if (delay == ADC108S102_DONE) {
			break;
		}

		/* convert to milliseconds */
		delay = (int32_t)((MSEC_PER_SEC * (uint64_t)delay) /
			sys_clock_ticks_per_sec);

		k_sleep(delay);

		ret = _ti_adc108s102_sampling(dev);
		if (ret != 0) {
			break;
		}

		_ti_adc108s102_handle_result(dev);
	}

	return ret;
}

static const struct adc_driver_api ti_adc108s102_api = {
	.enable = ti_adc108s102_enable,
	.disable = ti_adc108s102_disable,
	.read = ti_adc108s102_read,
};

static int ti_adc108s102_init(struct device *dev)
{
	const struct ti_adc108s102_config *config = dev->config->config_info;
	struct ti_adc108s102_data *adc = dev->driver_data;

	adc->spi = device_get_binding((char *)config->spi_port);
	if (!adc->spi) {
		return -EPERM;
	}

	SYS_LOG_DBG("ADC108s102 initialized\n");

	dev->driver_api = &ti_adc108s102_api;

	return 0;
}

#ifdef CONFIG_ADC_TI_ADC108S102

static struct ti_adc108s102_data adc108s102_data;

static const struct ti_adc108s102_config adc108s102_config = {
	.spi_port = CONFIG_ADC_TI_ADC108S102_SPI_PORT_NAME,
	.spi_config_flags = CONFIG_ADC_TI_ADC108S102_SPI_CONFIGURATION,
	.spi_freq = CONFIG_ADC_TI_ADC108S102_SPI_MAX_FREQ,
	.spi_slave = CONFIG_ADC_TI_ADC108S102_SPI_SLAVE,
};

DEVICE_INIT(adc108s102, CONFIG_ADC_0_NAME,
			ti_adc108s102_init,
			&adc108s102_data, &adc108s102_config,
			POST_KERNEL, CONFIG_ADC_INIT_PRIORITY);

#endif /* CONFIG_ADC_TI_ADC108S102 */
