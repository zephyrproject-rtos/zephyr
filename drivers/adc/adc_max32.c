/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_adc

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_max32, CONFIG_ADC_LOG_LEVEL);

#include <wrap_max32_adc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* reference voltage for the ADC */
#define MAX32_ADC_VREF_MV DT_INST_PROP(0, vref_mv)

#define ADC_MAX32_INT_FIFO_LVL_MSK      BIT(7)
#define ADC_MAX32_SAMPLE_SIZE           2
#define ADC_MAX32_BYTE_COUNT            16

enum adc_max32_fifo_format {
	ADC_MAX32_DATA_STATUS_FIFO,
	ADC_MAX32_DATA_ONLY_FIFO,
	ADC_MAX32_RAW_DATA_ONLY_FIFO,
};

struct max32_adc_config {
	uint8_t channel_count;
	mxc_adc_regs_t *regs;
	int clock_divider;
	int track_count;
	int idle_count;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
	void (*irq_func)(void);
};

struct max32_adc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint32_t sample_channels;
	const uint8_t resolution;
#ifdef CONFIG_ADC_MAX32_STREAM
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint64_t timestamp;
	struct rtio *r_cb;
	uint32_t adc_sample;
	uint8_t data_ready_gpio;
	uint8_t no_mem;
	struct k_timer sample_timer;
	const struct adc_sequence *sequence;
	uint8_t fifo_full_irq;
#endif /* CONFIG_ADC_MAX32_STREAM */
};


#ifdef CONFIG_ADC_MAX32_STREAM
/** MAX32 qscale modes */
enum max32_qscale_modes {
	MAX32_12B_MODE = 0,
};

struct adc_max32_fifo_config {
	enum adc_max32_fifo_format fifo_format;
	uint16_t fifo_samples;
};

struct adc_max32_fifo_data {
	uint16_t is_fifo: 1;
	uint16_t max32_qscale_mode: 1;
	uint16_t diff_mode: 1;
	uint16_t res: 4;
	uint16_t fifo_byte_count: 5;
	uint16_t sample_set_size: 4;
	uint16_t vref_mv;
	uint64_t timestamp;
} __attribute__((__packed__));
#endif /* CONFIG_ADC_MAX32_STREAM */

#ifdef CONFIG_ADC_ASYNC
static void adc_complete_cb(void *req, int error)
{
	ARG_UNUSED(req);
	ARG_UNUSED(error);
}
#endif /* CONFIG_ADC_ASYNC */

#ifdef CONFIG_ADC_MAX32_STREAM
static void adc_complete_rtio_cb(const struct device *dev)
{
	struct max32_adc_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->sqe;

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}
#endif /* CONFIG_ADC_MAX32_STREAM */

static void adc_max32_start_channel(const struct device *dev)
{
	struct max32_adc_data *data = dev->data;
	int ret = 0;

#if defined(CONFIG_ADC_ASYNC)
	if (data->ctx.asynchronous) {
		ret = Wrap_MXC_ADC_StartConversionAsync(&data->sample_channels, adc_complete_cb);
		if (ret < 0) {
			LOG_ERR("Error starting conversion (%d)", ret);
		}
	} else {
#endif /* CONFIG_ADC_ASYNC */
		while (data->sample_channels) {
			ret = Wrap_MXC_ADC_StartConversion(&data->sample_channels);
			if (ret < 0) {
				LOG_ERR("Error starting conversion (%d)", ret);
				return;
			}
			Wrap_MXC_ADC_GetData(&data->buffer);
		}

		Wrap_MXC_ADC_DisableConversion();
		adc_context_on_sampling_done(&data->ctx, dev);
#if defined(CONFIG_ADC_ASYNC)
	}
#endif /* CONFIG_ADC_ASYNC */
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct max32_adc_data *data = CONTAINER_OF(ctx, struct max32_adc_data, ctx);

	data->sample_channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	adc_max32_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct max32_adc_data *data = CONTAINER_OF(ctx, struct max32_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int start_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct max32_adc_data *data = dev->data;
	uint32_t num_of_sample_channels = POPCOUNT(seq->channels);
	uint32_t num_of_sample = 1;
	int ret = 0;

	if (seq->resolution != data->resolution) {
		LOG_ERR("Unsupported resolution (%d)", seq->resolution);
		return -ENOTSUP;
	}
	if (seq->channels == 0) {
		return -EINVAL;
	}
	if ((data->channels & seq->channels) != seq->channels) {
		return -EINVAL;
	}

	ret = Wrap_MXC_ADC_AverageConfig(seq->oversampling);
	if (ret != 0) {
		return -EINVAL;
	}

	if (seq->options) {
		num_of_sample += seq->options->extra_samplings;
	}
	if (seq->buffer_size < (num_of_sample * num_of_sample_channels)) { /* Buffer size control */
		return -ENOMEM;
	}

	data->buffer = seq->buffer;
	adc_context_start_read(&data->ctx, seq);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_max32_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct max32_adc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, seq);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_MAX32_STREAM
static int start_read_stream(const struct device *dev, const struct adc_sequence *seq)
{
	struct max32_adc_data *data = dev->data;
	int ret = 0;

	if (seq->resolution != data->resolution) {
		LOG_ERR("Unsupported resolution (%d)", seq->resolution);
		return -ENOTSUP;
	}
	if (seq->channels == 0) {
		return -EINVAL;
	}
	if ((data->channels & seq->channels) != seq->channels) {
		return -EINVAL;
	}

	ret = Wrap_MXC_ADC_AverageConfig(seq->oversampling);
	if (ret != 0) {
		return -EINVAL;
	}

	data->ctx.asynchronous = 1;
	data->sample_channels = seq->channels;

	/* Here we use regular cb that does nothing, it should be
	 * adc_complete_rtio_cb but the problem is dev struct
	 * cannot be passed to HAL without some big changes
	 * in the HAL layers, for now it is set like this
	 */
	ret = Wrap_MXC_ADC_StartConversionAsyncStream(&data->sample_channels, adc_complete_cb);
	if (ret != 0) {
		return -EINVAL;
	}

	return adc_context_wait_for_completion(&data->ctx);
}

void adc_max32_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct max32_adc_data *data = (struct max32_adc_data *)dev->data;
	const struct adc_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	int rc;

	if (data->no_mem == 1) {
		data->no_mem = 0;
		return;
	}
	data->sqe = iodev_sqe;

	adc_context_lock(&data->ctx, false, NULL);
	rc = start_read_stream(dev, read_cfg->sequence);

	adc_context_release(&data->ctx, rc);

	if (rc < 0) {
		LOG_ERR("Error starting conversion (%d)", rc);
	}
}

static const uint32_t adc_max32_resolution[] = {
	[MAX32_12B_MODE] = 12,
};

static inline int adc_max32_convert_q31(q31_t *out, const uint8_t *buff,
			enum max32_qscale_modes mode, uint8_t diff_mode,
			uint16_t vref_mv, uint8_t adc_shift)
{
	int32_t data_in = 0;
	uint32_t scale = BIT(adc_max32_resolution[mode]);

	/* No Differential mode */
	if (diff_mode) {
		return -EINVAL;
	}

	uint32_t sensitivity = (vref_mv * (scale - 1)) / scale
			 * 1000 / scale; /* uV / LSB */

	if (mode == MAX32_12B_MODE) {
		data_in = (buff[1] << 8) | buff[0];
		if (diff_mode && (data_in & (BIT(adc_max32_resolution[mode] - 1)))) {
			data_in |= ~BIT_MASK(adc_max32_resolution[mode]);
		}
	} else {
		data_in = sys_get_be16(buff);
	}

	*out =  BIT(31 - adc_shift) * sensitivity / 1000000 * data_in;
	return 0;
}

static int adc_max32_decoder_get_frame_count(const uint8_t *buffer, uint32_t channel,
			   uint16_t *frame_count)
{
	const struct adc_max32_fifo_data *data = (const struct adc_max32_fifo_data *)buffer;

	*frame_count = data->fifo_byte_count/ADC_MAX32_SAMPLE_SIZE;

	return 0;
}

static int adc_max32_decoder_decode(const uint8_t *buffer, uint32_t channel, uint32_t *fit,
			  uint16_t max_count, void *data_out)
{
	const struct adc_max32_fifo_data *enc_data = (const struct adc_max32_fifo_data *)buffer;
	const uint8_t *buffer_end =
		buffer + sizeof(struct adc_max32_fifo_data) + enc_data->fifo_byte_count;
	int count = 0;
	uint8_t sample_num = 0;

	if (buffer_end <= (buffer + *fit + sizeof(struct adc_max32_fifo_data))) {
		return 0;
	}

	struct adc_data *data = (struct adc_data *)data_out;

	memset(data, 0, sizeof(struct adc_data));
	data->header.base_timestamp_ns = enc_data->timestamp;
	data->header.reading_count = 1;

	/* 32 is used because input parameter for __builtin_clz func is
	 * unsigneg int (32 bits) and func will consider any input value
	 * as 32 bit.
	 */
	data->shift = 32 - __builtin_clz(enc_data->vref_mv);

	buffer += sizeof(struct adc_max32_fifo_data);
	uint8_t sample_set_size = enc_data->sample_set_size;
	/* Calculate which sample is decoded. */
	if (*fit) {
		sample_num = *fit / sample_set_size;
	}

	while (count < max_count && buffer < buffer_end) {
		/* 125 KSPS - this can be calculated from
		 * cnt and idle dts parameters but it is hardcoded for now
		 */
		data->readings[count].timestamp_delta = sample_num * (UINT32_C(1000000000) / 62500);
		adc_max32_convert_q31(&data->readings[count].value, (buffer + *fit),
				enc_data->max32_qscale_mode, enc_data->diff_mode,
				enc_data->vref_mv, data->shift);

		sample_num++;
		*fit += sample_set_size;
		count++;
	}

	return 0;

}
#endif /* CONFIG_ADC_MAX32_STREAM */

#ifdef CONFIG_ADC_ASYNC
static int adc_max32_read_async(const struct device *dev, const struct adc_sequence *seq,
				struct k_poll_signal *async)
{
	struct max32_adc_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, seq);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_max32_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	const struct max32_adc_config *conf = dev->config;
	struct max32_adc_data *data = dev->data;
	wrap_mxc_adc_scale_t wrap_mxc_scale;
	uint8_t adc_reference;
	int ret = 0;

	if (cfg->channel_id >= conf->channel_count) {
		LOG_ERR("Invalid channel (%u)", cfg->channel_id);
		return -EINVAL;
	}

	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid channel acquisition time");
		return -EINVAL;
	}

	if (cfg->differential) {
		LOG_ERR("Differential sampling not supported");
		return -ENOTSUP;
	}

	switch (cfg->reference) {
	case ADC_REF_INTERNAL:
		adc_reference = ADI_MAX32_ADC_REF_INTERNAL;
		break;
	case ADC_REF_VDD_1_2:
		adc_reference = ADI_MAX32_ADC_REF_VDD_1_2;
		break;
	case ADC_REF_EXTERNAL0:
		adc_reference = ADI_MAX32_ADC_REF_EXT0;
		break;
	default:
		return -ENOTSUP;
	}
	ret = Wrap_MXC_ADC_ReferenceSelect(adc_reference);
	if (ret != 0) {
		LOG_ERR("Reference is not supported.");
		return -ENOTSUP;
	}

	switch (cfg->gain) {
	case ADC_GAIN_1_6:
		wrap_mxc_scale = WRAP_MXC_ADC_SCALE_6;
		break;
	case ADC_GAIN_1_4:
		wrap_mxc_scale = WRAP_MXC_ADC_SCALE_4;
		break;
	case ADC_GAIN_1_3:
		wrap_mxc_scale = WRAP_MXC_ADC_SCALE_3;
		break;
	case ADC_GAIN_1_2:
		wrap_mxc_scale = WRAP_MXC_ADC_SCALE_2;
		break;
	case ADC_GAIN_1:
		wrap_mxc_scale = WRAP_MXC_ADC_SCALE_1;
		break;
	case ADC_GAIN_2:
		wrap_mxc_scale = WRAP_MXC_ADC_SCALE_2X;
		break;
	default:
		return -ENOTSUP;
	}
	ret = Wrap_MXC_ADC_SetExtScale(wrap_mxc_scale);
	if (ret != 0) {
		LOG_ERR("Gain value is not supported.");
		return -ENOTSUP;
	}

	data->channels |= BIT(cfg->channel_id);
	return 0;
}

static int adc_max32_init(const struct device *dev)
{
	const struct max32_adc_config *config = dev->config;
	struct max32_adc_data *data = dev->data;
	uint32_t ret;
	wrap_mxc_adc_req_t req = {
		.clock = config->perclk.clk_src,
		.clkdiv = config->clock_divider,
		.cal = 1, /* Initial calibration enabled. */
		.ref = 1, /* Reference set to internal reference until user define it. */
		.trackCount = config->track_count,
		.idleCount = config->idle_count};

	/* Enable clock */
	ret = clock_control_on(config->clock, (clock_control_subsys_t)&config->perclk);
	if (ret) {
		return ret;
	}

	ret = Wrap_MXC_ADC_Init(&req);
	if (ret) {
		return -EINVAL;
	}

	ret = pinctrl_apply_state(config->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	config->irq_func();
	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_ADC_MAX32_STREAM
static void adc_max32_rtio_isr(const struct device *dev)
{
	struct max32_adc_data *const data = dev->data;
	uint32_t flags = MXC_ADC_GetFlags();
	uint32_t int_req = BIT(3);

	MXC_ADC_Handler();
	if (flags & int_req) {
		MXC_ADC_Free();
	}
	MXC_ADC_ClearFlags(flags);

	if (flags & WRAP_MXC_F_ADC_CONV_DONE_IF) {

		data->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

		const size_t min_read_size = 64;

		uint8_t *buf;
		uint32_t buf_len;

		if (rtio_sqe_rx_buf(data->sqe, min_read_size, min_read_size,
					&buf, &buf_len) != 0) {
			data->no_mem = 1;
			rtio_iodev_sqe_err(data->sqe, -ENOMEM);
			return;
		}
		struct adc_max32_fifo_data *hdr = (struct adc_max32_fifo_data *)buf;

		hdr->is_fifo = 1;
		hdr->timestamp = data->timestamp;
		hdr->vref_mv = MAX32_ADC_VREF_MV;
		hdr->max32_qscale_mode = MAX32_12B_MODE;
		hdr->fifo_byte_count = ADC_MAX32_BYTE_COUNT;
		hdr->sample_set_size = ADC_MAX32_SAMPLE_SIZE;

		uint8_t *read_buf = buf + sizeof(*hdr);

		Wrap_MXC_ADC_GetData((uint16_t **)&read_buf);

		if (data->sample_channels != 0) {
			adc_max32_start_channel(dev);
		} else {
			Wrap_MXC_ADC_DisableConversion();
			adc_context_on_sampling_done(&data->ctx, dev);
		}
	}
	if (flags & int_req) {

		adc_complete_rtio_cb(dev);
	}
}

ADC_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adc_max32_decoder_get_frame_count,
	.decode = adc_max32_decoder_decode,
};

int adc_max32_get_decoder(const struct device *dev, const struct adc_decoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &ADC_DECODER_NAME();

	return 0;
}
#else
static void adc_max32_isr(const struct device *dev)
{
	struct max32_adc_data *const data = dev->data;
	uint32_t flags = MXC_ADC_GetFlags();

	MXC_ADC_Handler();
	MXC_ADC_ClearFlags(flags);

	if (flags & WRAP_MXC_F_ADC_CONV_DONE_IF) {
		Wrap_MXC_ADC_GetData(&data->buffer);

		if (data->sample_channels != 0) {
			adc_max32_start_channel(dev);
		} else {
			Wrap_MXC_ADC_DisableConversion();
			adc_context_on_sampling_done(&data->ctx, dev);
		}
	}
}
#endif /* CONFIG_ADC_MAX32_STREAM */

static DEVICE_API(adc, adc_max32_driver_api) = {
	.channel_setup = adc_max32_channel_setup,
	.read = adc_max32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_max32_read_async,
#endif /* CONFIG_ADC_ASYNC */
	.ref_internal = MAX32_ADC_VREF_MV,
#ifdef CONFIG_ADC_MAX32_STREAM
	.submit = adc_max32_submit_stream,
	.get_decoder = adc_max32_get_decoder,
#endif /* CONFIG_ADC_MAX32_STREAM */
};

#define MAX32_ADC_INIT(_num)                                                                       \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	static void max32_adc_irq_init_##_num(void)                                                \
	{                                                                                          \
		COND_CODE_1(CONFIG_ADC_MAX32_STREAM,                                               \
		(IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority), adc_max32_rtio_isr,  \
		DEVICE_DT_INST_GET(_num), 0)),                                                     \
		(IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority), adc_max32_isr,       \
		DEVICE_DT_INST_GET(_num), 0)));                                                    \
		irq_enable(DT_INST_IRQN(_num));                                                    \
	};                                                                                         \
	static const struct max32_adc_config max32_adc_config_##_num = {                           \
		.channel_count = DT_INST_PROP(_num, channel_count),                                \
		.regs = (mxc_adc_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.clock_divider = DT_INST_PROP_OR(_num, clock_divider, 1),                          \
		.track_count = DT_INST_PROP_OR(_num, track_count, 0),                              \
		.idle_count = DT_INST_PROP_OR(_num, idle_count, 0),                                \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		.perclk.clk_src =                                                                  \
			DT_INST_PROP_OR(_num, clock_source, ADI_MAX32_PRPH_CLK_SRC_PCLK),          \
		.irq_func = max32_adc_irq_init_##_num,                                             \
	};                                                                                         \
	static struct max32_adc_data max32_adc_data_##_num = {                                     \
		ADC_CONTEXT_INIT_TIMER(max32_adc_data_##_num, ctx),                                \
		ADC_CONTEXT_INIT_LOCK(max32_adc_data_##_num, ctx),                                 \
		ADC_CONTEXT_INIT_SYNC(max32_adc_data_##_num, ctx),                                 \
		.resolution = DT_INST_PROP(_num, resolution),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_num, &adc_max32_init, NULL, &max32_adc_data_##_num,                 \
			      &max32_adc_config_##_num, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,     \
			      &adc_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX32_ADC_INIT)
