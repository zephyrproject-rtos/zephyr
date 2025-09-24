/*
 * Copyright (c) 2024 Centro de Inovacao EDGE
 * Copyright (c) 2025 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>
#include <zephyr/dsp/print_format.h>

/* ADC node from the devicetree. */
#define SAMPLE_ADC_NODE DT_ALIAS(adc0)

#define SAMPLE_DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, SAMPLE_DT_SPEC_AND_COMMA)
};
static const int adc_channels_count = ARRAY_SIZE(adc_channels);
#endif

static void init_adc(void)
{
	int i, ret;

	ret = adc_is_ready_dt(&adc_channels[0]);

	for (i = 0; i < adc_channels_count; i++) {
		ret = adc_channel_setup_dt(&adc_channels[i]);
	}
}

/* Define triggers for ADC stream. Trigger stands for an event that
 * will cause the ADC to read data and perform an operation on it.
 *
 * Format for each trigger is:
 * {trigger, operation to be done on the data associated to the trigger}
 *
 * For more information about supported triggers and data operations,
 * refer to the enums adc_trigger_type and adc_stream_data_opt.
 */
#define SAMPLE_ADC_TRIGGERS					\
	{ADC_TRIG_FIFO_FULL, ADC_STREAM_DATA_INCLUDE},		\
	{ADC_TRIG_FIFO_WATERMARK, ADC_STREAM_DATA_INCLUDE}

ADC_DT_STREAM_IODEV(iodev, SAMPLE_ADC_NODE, adc_channels, SAMPLE_ADC_TRIGGERS);

/* Mempool is used for sharing ADC data that has been read between ADC driver and the application.
 * Data read in the ADC driver is stored in the mempool and can be processed in the application.
 * Current values are set to support range of ADC drivers and can be optimized for smaller memory
 * footprint. sizeof(void *) is used so memory blocks are properly aligned for different
 * platforms.
 */
RTIO_DEFINE_WITH_MEMPOOL(adc_ctx, 16, 16, 20, 256, sizeof(void *));

static int print_adc_stream(const struct device *adc, struct rtio_iodev *local_iodev)
{
	int rc = 0;
	const struct adc_decoder_api *decoder;
	struct rtio_cqe *cqe;
	uint8_t *buf;
	uint32_t buf_len;
	struct rtio_sqe *handles;

	/* Start the streams */
	adc_stream(local_iodev, &adc_ctx, NULL, &handles);

	while (1) {
		/* CQE is a RTIO completion event. It is created in a ADC driver that is
		 * doing the streaming when there is a batch of data to be processed
		 * or some error occurred during ADC streaming. CQE contains result of
		 * data reading and userdata (if one is passed in adc_stream).
		 * It is used to retrieve mempool buffer where data is stored.
		 */
		cqe = rtio_cqe_consume_block(&adc_ctx);

		if (cqe->result != 0) {
			printk("async read failed %d\n", cqe->result);
			return cqe->result;
		}

		rc = rtio_cqe_get_mempool_buffer(&adc_ctx, cqe, &buf, &buf_len);

		if (rc != 0) {
			printk("get mempool buffer failed %d\n", rc);
			return rc;
		}

		rtio_cqe_release(&adc_ctx, cqe);

		rc = adc_get_decoder(adc, &decoder);

		if (rc != 0) {
			printk("sensor_get_decoder failed %d\n", rc);
			return rc;
		}

		/* Frame iterator values when data comes from a FIFO */
		uint32_t adc_fit = 0;
		struct adc_data adc_data = {0};

		/* Number of accelerometer data frames */
		uint16_t frame_count;

		rc = decoder->get_frame_count(buf, 0, &frame_count);

		if (rc != 0) {
			printk("get_frame_count failed %d\n", rc);
			return rc;
		}

		/* Decode all available accelerometer sample frames */
		for (int i = 0; i < frame_count; i++) {
			decoder->decode(buf, 0, &adc_fit, 1, &adc_data);

			printk("ADC data for %s (%" PRIq(6) ") %lluns\n", adc->name,
			PRIq_arg(adc_data.readings[0].value, 6, adc_data.shift),
			(adc_data.header.base_timestamp_ns
			+ adc_data.readings[0].timestamp_delta));
		}

		rtio_release_buffer(&adc_ctx, buf, buf_len);
	}

	return rc;
}

int main(void)
{
	int ret;
	struct adc_sequence sequence;
	struct adc_read_config *read_cfg = iodev.data;

	read_cfg->sequence = &sequence;

	init_adc();
	ret = adc_sequence_init_dt(&adc_channels[0], &sequence);
	if (ret < 0) {
		printk("Failed to initialize ADC sequence: %d\n", ret);
		return 0;
	}

	print_adc_stream(adc_channels[0].dev, &iodev);

	return 0;
}
