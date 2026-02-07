/*
 * Copyright (c) 2025 Gabriele Zampieri <opensource@arsenaling.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT maxim_max2253x

LOG_MODULE_REGISTER(adc_max2253x, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define MAX2253X_VREF_MV         1800
#define MAX2253X_CH_COUNT        4
#define MAX2253X_RESOLUTION_BITS 12

/* MAX2253x header fields */
#define MAX2253X_HDR_ADDRESS_MASK GENMASK(7, 2)
#define MAX2253X_HDR_WNR_MASK     BIT(1)
#define MAX2253X_HDR_BURST_MASK   BIT(0)

#define MAX2253X_WRITE    1
#define MAX2253X_READ     0
#define MAX2253X_BURST    1
#define MAX2253X_NO_BURST 0

#define MAX2253X_BUILD_HDR(addr, wnr, burst)                                                       \
	(FIELD_PREP(MAX2253X_HDR_ADDRESS_MASK, (addr)) |                                           \
	 FIELD_PREP(MAX2253X_HDR_WNR_MASK, (wnr)) | FIELD_PREP(MAX2253X_HDR_BURST_MASK, burst))

#define MAX2253X_HDR_WRITE(addr)       MAX2253X_BUILD_HDR(addr, MAX2253X_WRITE, MAX2253X_NO_BURST)
#define MAX2253X_HDR_READ(addr, burst) MAX2253X_BUILD_HDR(addr, MAX2253X_READ, burst)

/* MAX2253x registers*/
#define MAX2253X_ADC1             0x01
#define MAX2253X_INTERRUPT_STATUS 0x12
#define MAX2253X_INTERRUPT_ENABLE 0x13
#define MAX2253X_CONTROL          0x14

/* MAX2253X_ADCx Bit Definitions */
#define MAX2253X_ADCx_ADCS BIT(15)
#define MAX2253X_ADCx_ADC  GENMASK(11, 0)

/* MAX2253X_INTERRUPT_STATUS Bit Definitions */
#define MAX2253X_INTERRUPT_STATUS_EOC BIT(12)

/* MAX2253X_INTERRUPT_ENABLE Bit Definitions */
#define MAX2253X_INTERRUPT_ENABLE_EEOC BIT(12)

/* MAX2253X_CONTROL Bit Definitions */
#define MAX2253X_CONTROL_SRES BIT(1) /* Soft reset */

struct max2253x_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec int_gpio;
};

struct max2253x_data {
	struct adc_context ctx;
	const struct device *dev;
	struct gpio_callback int_callback;

	uint32_t channels;
	uint16_t *buffer;
	uint16_t *repeat_buffer;

	struct k_sem acq_sem;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_MAX2253X_ACQUISITION_THREAD_STACK_SIZE);

#ifdef CONFIG_ADC_MAX2253X_STREAM
	struct rtio_iodev *iodev;   /* parent SPI rtio iodev */
	struct rtio *rtio_ctx;      /* parent SPI rtio context */
	struct rtio_iodev_sqe *sqe; /* current streaming sqe */
	uint64_t timestamp;
#endif /* CONFIG_ADC_MAX2253X_STREAM */
};

#ifdef CONFIG_ADC_MAX2253X_STREAM
/** MAX2253X qscale modes */
enum max2253x_qscale_modes {
	MAX2253X_12B_MODE = 0,
};

/*
 * This struct defines the format of the data frame sent in streaming mode.
 * The driver will always read all the channels and the user can extract the ones interested in
 * through the decoder.
 */
struct adc_max2253x_frame_data {
	uint16_t magic: 10;               /* Magic number to identify MAX2253X frames */
	uint16_t max2253x_qscale_mode: 1; /* Only 12-bit mode supported */
	uint16_t diff_mode: 1;            /* Not actually supported */
	uint16_t res: 4;
	uint16_t vref_mv; /* Always set to MAX2253X_VREF_MV */
	uint64_t timestamp;
	uint8_t channel_buffer[MAX2253X_CH_COUNT * sizeof(uint16_t)];
} __attribute__((__packed__));

#define ADC_MAX2253X_MAGIC 0x225
#endif /* CONFIG_ADC_MAX2253X_STREAM */

/**
 * @brief Read one or more registers, accessing them in burst mode if needed.
 *
 * @param dev Device handle.
 * @param address Register address to start reading from.
 * @param buffer Buffer to store read values.
 * @param count Number of registers to read.
 * @return 0 on success, negative error code on failure.
 */
static int max2253x_read(const struct device *dev, uint8_t address, uint16_t *buffer, size_t count)
{
	const struct max2253x_config *config = dev->config;
	int r;
	uint8_t dummy;
	uint8_t hdr = MAX2253X_HDR_READ(address, count > 1 ? MAX2253X_BURST : MAX2253X_NO_BURST);

	const struct spi_buf hdr_buf[1] = {{.buf = &hdr, .len = sizeof(hdr)}};
	const struct spi_buf_set tx = {.buffers = hdr_buf, .count = ARRAY_SIZE(hdr_buf)};
	const struct spi_buf rx_buf[2] = {
		{.buf = &dummy, .len = 1}, /* Dummy byte for header */
		{.buf = (uint8_t *)buffer, .len = sizeof(uint16_t) * count},
	};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

	r = spi_transceive_dt(&config->spi, &tx, &rx);
	if (r) {
		return r;
	}

	for (size_t i = 0; i < count; i++) {
		buffer[i] = sys_be16_to_cpu(buffer[i]);
	}
	return 0;
}

/**
 * @brief Read a single register. Wrapper around @c max2253x_read().
 *
 * @param dev Device handle.
 * @param reg Register address to read.
 * @param value Buffer to store the read value.
 * @return 0 on success, negative error code on failure.
 */
static inline int max2253x_read_reg(const struct device *dev, uint8_t reg, uint16_t *value)
{
	return max2253x_read(dev, reg, value, 1);
}

/**
 * @brief Write a single register.
 *
 * @param dev Device handle.
 * @param reg Register address to write.
 * @param value Value to write.
 * @return 0 on success, negative error code on failure.
 */
static int max2253x_write_reg(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct max2253x_config *config = dev->config;

	/* Adjust the value to match the ADC endianness */
	value = sys_cpu_to_be16(value);

	uint8_t hdr = MAX2253X_HDR_WRITE(reg);
	const struct spi_buf tx_buf[] = {
		{.buf = &hdr, .len = sizeof(hdr)},
		{.buf = &value, .len = sizeof(value)},
	};
	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

	return spi_transceive_dt(&config->spi, &tx, NULL);
}

/**
 * @brief Update specific fields of a register.
 *
 * @param dev Device handle.
 * @param reg Register address to update.
 * @param mask Mask of bits to update.
 * @param field New value for the masked bits.
 * @return 0 on success, negative error code on failure.
 */
static int max2253x_update_reg(const struct device *dev, uint8_t reg, uint16_t mask, uint16_t field)
{
	int ret;
	uint16_t reg_val = 0;

	ret = max2253x_read_reg(dev, reg, &reg_val);
	if (ret < 0) {
		return ret;
	}

	reg_val = (reg_val & ~mask) | field;
	return max2253x_write_reg(dev, reg, reg_val);
}

/**
 * @brief Read raw ADC values. Wrapper around @c max2253x_read() to extract only the ADC bits.
 *
 * @param dev Device handle.
 * @param buffer Buffer to store the read values.
 * @param count Number of values to read, starting from the first channel.
 * @return 0 on success, negative error code on failure.
 */
static inline int max2253x_read_raw_adc(const struct device *dev, uint16_t *buffer, size_t count)
{
	int ret;

	if (count > MAX2253X_CH_COUNT) {
		return -EINVAL;
	}

	ret = max2253x_read(dev, MAX2253X_ADC1, buffer, count);
	if (ret) {
		return ret;
	}

	for (int i = 0; i < count; i++) {
		buffer[i] = FIELD_GET(MAX2253X_ADCx_ADC, buffer[i]);
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct max2253x_data *data = CONTAINER_OF(ctx, struct max2253x_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct max2253x_data *data = CONTAINER_OF(ctx, struct max2253x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_max2253x_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id >= MAX2253X_CH_COUNT) {
		LOG_ERR("Invalid channel %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	/* Channels can't be configured on MAX2253x. Perform a sanity check so the user won't be
	 * confused if the ADC does not behave as expected.
	 */

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("only x1 gain is supported");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("only internal reference is supported");
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("only default acquisition time is supported");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential channel not supported");
		return -EINVAL;
	}

	return 0;
}

static int adc_max2253x_validate_buffer_size(const struct device *dev,
					     const struct adc_sequence *sequence)
{
	uint8_t channels;
	size_t needed;

	channels = POPCOUNT(sequence->channels);
	needed = channels * sizeof(uint16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int adc_max2253x_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct max2253x_data *data = dev->data;
	int ret;

	if (sequence->resolution != MAX2253X_RESOLUTION_BITS) {
		LOG_ERR("invalid resolution %d", sequence->resolution);
		return -EINVAL;
	}

	ret = adc_max2253x_validate_buffer_size(dev, sequence);
	if (ret < 0) {
		LOG_ERR("insufficient buffer size");
		return ret;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_max2253x_read_async(const struct device *dev, const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct max2253x_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = adc_max2253x_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_max2253x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_max2253x_read_async(dev, sequence, NULL);
}

#ifdef CONFIG_ADC_MAX2253X_STREAM
void adc_max2253x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct max2253x_config *config = dev->config;
	struct max2253x_data *data = (struct max2253x_data *)dev->data;
	int ret;

	data->sqe = iodev_sqe;

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		rtio_iodev_sqe_err(data->sqe, ret);
	}
}

static const uint32_t adc_max2253x_resolution[] = {
	[MAX2253X_12B_MODE] = 12,
};

static inline int adc_max2253x_convert_q31(q31_t *out, const uint8_t *buff,
					   enum max2253x_qscale_modes mode, uint8_t diff_mode,
					   uint16_t vref_mv, uint8_t adc_shift)
{
	int32_t data_in = 0;
	uint32_t scale = BIT(adc_max2253x_resolution[mode]);
	/* No Differential mode */
	if (diff_mode) {
		return -EINVAL;
	}

	uint32_t sensitivity = (vref_mv * (scale - 1)) / scale * 1000 / scale; /* uV / LSB */

	if (mode == MAX2253X_12B_MODE) {
		data_in = sys_get_be16(buff);
		if (diff_mode && (data_in & (BIT(adc_max2253x_resolution[mode] - 1)))) {
			data_in |= ~BIT_MASK(adc_max2253x_resolution[mode]);
		}
	} else {
		data_in = sys_get_be16(buff);
	}

	*out = BIT(31 - adc_shift) * sensitivity / 1000000 * data_in;
	return 0;
}

static int adc_max2253x_decoder_get_frame_count(const uint8_t *buffer, uint32_t channel,
						uint16_t *frame_count)
{
	const struct adc_max2253x_frame_data *frame_data =
		(const struct adc_max2253x_frame_data *)buffer;

	if (frame_data->magic != ADC_MAX2253X_MAGIC) {
		return -EINVAL;
	}

	*frame_count = 1;

	return 0;
}

static int adc_max2253x_decoder_decode(const uint8_t *buffer, uint32_t channel, uint32_t *fit,
				       uint16_t max_count, void *data_out)
{
	const struct adc_max2253x_frame_data *enc_data =
		(const struct adc_max2253x_frame_data *)buffer;

	if (channel > MAX2253X_CH_COUNT) {
		return -ENOTSUP;
	}

	if (*fit > 0) {
		return -ENOTSUP;
	}

	if (enc_data->magic != ADC_MAX2253X_MAGIC) {
		return -EINVAL;
	}

	struct adc_data *data = (struct adc_data *)data_out;

	memset(data, 0, sizeof(struct adc_data));
	data->header.base_timestamp_ns = enc_data->timestamp;
	data->header.reading_count = 1;

	/* 32 is used because input parameter for __builtin_clz func is
	 * unsigned int (32 bits) and func will consider any input value
	 * as 32 bit.
	 */
	data->shift = 32 - __builtin_clz(enc_data->vref_mv);

	data->readings[0].timestamp_delta = 0;
	adc_max2253x_convert_q31(&data->readings[0].value,
				 &enc_data->channel_buffer[channel * sizeof(uint16_t)],
				 enc_data->max2253x_qscale_mode, enc_data->diff_mode,
				 enc_data->vref_mv, data->shift);

	*fit = 1;

	return 0;
}

static void max2253x_process_sample_cb(struct rtio *r, const struct rtio_sqe *sqe, int res,
				       void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void max2253x_stream_irq_handler(const struct device *dev)
{
	const struct max2253x_config *config = dev->config;
	struct max2253x_data *data = dev->data;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	struct rtio_sqe *sqes[3]; /* header, read, callback */
	const uint8_t hdr = MAX2253X_HDR_READ(MAX2253X_ADC1, MAX2253X_BURST);
	uint8_t *buf;
	uint32_t buf_len;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

	data->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

	if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adc_max2253x_frame_data),
			    sizeof(struct adc_max2253x_frame_data), &buf, &buf_len) != 0) {
		rtio_iodev_sqe_err(data->sqe, -ENOMEM);
		return;
	}

	struct adc_max2253x_frame_data *frame = (struct adc_max2253x_frame_data *)buf;

	frame->magic = ADC_MAX2253X_MAGIC;
	frame->timestamp = data->timestamp;
	frame->vref_mv = MAX2253X_VREF_MV;
	frame->max2253x_qscale_mode = MAX2253X_12B_MODE;
	frame->diff_mode = 0;

	if (rtio_sqe_acquire_array(data->rtio_ctx, ARRAY_SIZE(sqes), sqes) != 0) {
		rtio_iodev_sqe_err(data->sqe, -ENOMEM);
		return;
	}

	/* Header SQE */
	rtio_sqe_prep_tiny_write(sqes[0], data->iodev, RTIO_PRIO_NORM, &hdr, 1, current_sqe);
	sqes[0]->flags = RTIO_SQE_TRANSACTION | RTIO_SQE_CHAINED;
	/* Read SQE */
	rtio_sqe_prep_read(sqes[1], data->iodev, RTIO_PRIO_NORM, frame->channel_buffer,
			   sizeof(frame->channel_buffer), current_sqe);
	sqes[1]->flags = RTIO_SQE_CHAINED;
	/* Callback SQE */
	rtio_sqe_prep_callback(sqes[2], max2253x_process_sample_cb, NULL, current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

ADC_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adc_max2253x_decoder_get_frame_count,
	.decode = adc_max2253x_decoder_decode,
};

int adc_max2253x_get_decoder(const struct device *dev, const struct adc_decoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &ADC_DECODER_NAME();

	return 0;
}
#endif /* CONFIG_ADC_MAX2253X_STREAM */

static void max2253x_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct max2253x_data *data = p1;
	uint16_t adc_raw[MAX2253X_CH_COUNT];
	int ret;

	while (true) {
		k_sem_take(&data->acq_sem, K_FOREVER);

		ret = max2253x_read_raw_adc(data->dev, adc_raw, ARRAY_SIZE(adc_raw));
		if (ret) {
			LOG_ERR("Failed to read raw samples (err %d)", ret);
			adc_context_complete(&data->ctx, ret);
			continue;
		}

		for (int i = 0; i < ARRAY_SIZE(adc_raw); i++) {
			if (data->channels & BIT(i)) {
				*data->buffer++ = adc_raw[i];
			}
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

#if CONFIG_ADC_MAX2253X_STREAM
static void max2253x_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct max2253x_data *data = CONTAINER_OF(cb, struct max2253x_data, int_callback);
	const struct adc_read_config *read_config;

	ARG_UNUSED(pins);

	if (data->sqe == NULL || data->sqe->sqe.iodev == NULL ||
	    data->sqe->sqe.iodev->data == NULL) {
		LOG_WRN("MAX2253X interrupt with no active stream SQE");
		return;
	}

	read_config = data->sqe->sqe.iodev->data;

	if (read_config->is_streaming) {
		max2253x_stream_irq_handler(data->dev);
	}
}

static int max2253x_configure_irq(const struct device *dev)
{
	const struct max2253x_config *config = dev->config;
	struct max2253x_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("INT GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("failed to configure INT GPIO (err %d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_callback, max2253x_interrupt_handler,
			   BIT(config->int_gpio.pin));

	ret = gpio_add_callback_dt(&config->int_gpio, &data->int_callback);
	if (ret < 0) {
		LOG_ERR("failed to add INT GPIO callback (err %d)", ret);
		return ret;
	}

	/* Enable EOC interrupt */
	ret = max2253x_update_reg(dev, MAX2253X_INTERRUPT_ENABLE, MAX2253X_INTERRUPT_ENABLE_EEOC,
				  FIELD_PREP(MAX2253X_INTERRUPT_ENABLE_EEOC, 1));
	if (ret < 0) {
		LOG_ERR("Failed to enable EOC interrupt (err %d)", ret);
		return ret;
	}

	return ret;
}
#endif

static int max2253x_init(const struct device *dev)
{
	const struct max2253x_config *config = dev->config;
	struct max2253x_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI not ready");
		return -ENODEV;
	}

	adc_context_init(&data->ctx);

	k_sem_init(&data->acq_sem, 0, 1);

	k_tid_t tid =
		k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
				max2253x_acquisition_thread, data, NULL, NULL,
				CONFIG_ADC_MAX2253X_ACQUISITION_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, "adc_max2253x");

	/* Reset */
	ret = max2253x_update_reg(dev, MAX2253X_CONTROL, MAX2253X_CONTROL_SRES,
				  FIELD_PREP(MAX2253X_CONTROL_SRES, 1));
	if (ret < 0) {
		LOG_ERR("Failed to reset device %s", dev->name);
		return ret;
	}

#if CONFIG_ADC_MAX2253X_STREAM
	ret = max2253x_configure_irq(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure IRQ (err %d)", ret);
		return ret;
	}
#endif

	adc_context_unlock_unconditionally(&data->ctx);
	data->dev = dev;
	return ret;
}

static DEVICE_API(adc, max2253x_api) = {
	.channel_setup = adc_max2253x_channel_setup,
	.read = adc_max2253x_read,
	.ref_internal = MAX2253X_VREF_MV,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_max2253x_read_async,
#endif /* CONFIG_ADC_ASYNC */
#ifdef CONFIG_ADC_MAX2253X_STREAM
	.submit = adc_max2253x_submit_stream,
	.get_decoder = adc_max2253x_get_decoder,
#endif /* CONFIG_ADC_MAX2253X_STREAM */
};

#define MAX2253X_SPI_CFG (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

#define MAX2253X_RTIO_DEFINE(inst)                                                                 \
	SPI_DT_IODEV_DEFINE(max2253x_iodev_##inst, DT_DRV_INST(inst), MAX2253X_SPI_CFG);           \
	RTIO_DEFINE(max2253x_rtio_ctx_##inst, 16, 16);

#define MAX2253X_INIT(inst)                                                                        \
	IF_ENABLED(CONFIG_ADC_MAX2253X_STREAM, (MAX2253X_RTIO_DEFINE(inst)));                       \
	static const struct max2253x_config max2253x_config_##inst = {                             \
		.spi = SPI_DT_SPEC_INST_GET(inst, MAX2253X_SPI_CFG),                               \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
	};                                                                                         \
                                                                                                   \
	static struct max2253x_data max2253x_data_##inst = {                                       \
		IF_ENABLED(CONFIG_ADC_MAX2253X_STREAM, ( \
			.rtio_ctx = &max2253x_rtio_ctx_##inst, \
			.iodev = &max2253x_iodev_##inst \
		))};           \
	DEVICE_DT_INST_DEFINE(inst, max2253x_init, NULL, &max2253x_data_##inst,                    \
			      &max2253x_config_##inst, POST_KERNEL,                                \
			      CONFIG_ADC_MAX2253X_INIT_PRIORITY, &max2253x_api);

DT_INST_FOREACH_STATUS_OKAY(MAX2253X_INIT)
