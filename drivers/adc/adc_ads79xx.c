/* TI ADS79xx Series ADCs
 *
 * Copyright (c) 2026 James Walmsley <james@fullfat-fs.co.uk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <sys/errno.h>

LOG_MODULE_REGISTER(adc_ads79xx, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

enum ads79xx_cr_mode {
	ADS79xx_CR_MODE_CONTINUE = 0x0,
	ADS79xx_CR_MODE_MANUAL = 0x1,
	ADS79xx_CR_MODE_AUTO1 = 0x2,
	ADS79xx_CR_MODE_AUTO2 = 0x3,
	ADS79xx_CR_MODE_PROGRAM_GPIO = 0x4,
	ADS79xx_CR_MODE_PROGRAM_AUTO1 = 0x8,
	ADS79xx_CR_MODE_PROGRAM_ALARM1 = 0xc,
	ADS79xx_CR_MODE_PROGRAM_ALARM2 = 0xd,
	ADS79xx_CR_MODE_PROGRAM_ALARM3 = 0xe,
	ADS79xx_CR_MODE_PROGRAM_ALARM4 = 0xf,
};

#define ADS79xx_CR_MODE(mode)   ((mode & 0xf) << 12)
#define ADS79xx_CR_WRITE        BIT(11)
#define ADS79xx_CR_RESET_CHCNT  BIT(10)
#define ADS79xx_CR_CHAN(ch)     ((ch & 0xf) << 7)
#define ADS79xx_CR_RANGE_2X     BIT(6)
#define ADS79xx_CR_POWERDOWN    BIT(5)
#define ADS79xx_CR_OUTPUT_GPIO  BIT(4)
#define ADS79xx_CR_GPIO_DATA(d) ((d & 0xf) << 0)

struct ads79xx_config {
	struct spi_dt_spec spi;
	uint8_t channels;
	uint8_t resolution;
	uint8_t range;
};

struct ads79xx_data {
	struct adc_context ctx;
	const struct device *dev;

	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint16_t channels;
	uint16_t auto1_mask;

	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_ADS79XX_ACQUISITION_THREAD_STACK_SIZE);
};

static int ads79xx_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct ads79xx_config *config = dev->config;

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_VDD_1) {
		LOG_ERR("unsupported channel reference '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition time '%d'", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->channel_id >= config->channels) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("unsupported differential mode");
		return -ENOTSUP;
	}

	return 0;
}

static int ads79xx_spi_transfer(const struct device *dev, uint16_t tx_word, uint16_t *rx_word)
{
	const struct ads79xx_config *cfg = dev->config;

	uint16_t r;
	int ret;

	struct spi_buf txb = {.buf = &tx_word, .len = sizeof(tx_word)};
	struct spi_buf rxb = {.buf = &r, .len = sizeof(r)};

	struct spi_buf_set txs = {.buffers = &txb, .count = 1};
	struct spi_buf_set rxs = {.buffers = &rxb, .count = 1};

	ret = spi_transceive_dt(&cfg->spi, &txs, &rxs);
	if (ret) {
		return ret;
	}

	if (rx_word) {
		*rx_word = r;
	}

	if (IS_ENABLED(CONFIG_ADC_ADS79XX_DEBUG_SPI_TRANSFERS)) {
		uint16_t w = tx_word;

		LOG_DBG("SDI=0x%04x mode=%x prog=%u next_ch=%u range=%u pd=%u sdo_gpio=%u "
			"gpio=0x%x",
			w, (w >> 12) & 0xF, (w >> 11) & 1, (w >> 7) & 0xF, (w >> 6) & 1,
			(w >> 5) & 1, (w >> 4) & 1, w & 0xF);
		LOG_DBG("SDO=0x%04x addr=%x val=%u", r, (r >> 12), r & 0x0FFF);
	}

	return ret;
}

static inline uint16_t ads79xx_manual_command(const struct ads79xx_config *cfg, int ch)
{
	uint16_t cmd = 0;

	cmd |= ADS79xx_CR_MODE(ADS79xx_CR_MODE_MANUAL);
	cmd |= ADS79xx_CR_WRITE;
	cmd |= ADS79xx_CR_CHAN(ch);

	if (cfg->range == 2) {
		cmd |= ADS79xx_CR_RANGE_2X;
	}

	return cmd;
}

static inline uint16_t ads79xx_ctrl_continue(void)
{
	return ADS79xx_CR_MODE(ADS79xx_CR_MODE_CONTINUE);
}

static inline uint16_t ads79xx_ctrl_auto1_prog_entry(void)
{
	return ADS79xx_CR_MODE(ADS79xx_CR_MODE_PROGRAM_AUTO1);
}

static inline uint16_t ads79xx_ctrl_auto1(bool reset_counter, bool map_gpio, bool range_2x,
					  bool powerdown, uint8_t gpio_out)
{
	uint16_t w = ADS79xx_CR_MODE(ADS79xx_CR_MODE_AUTO1);

	w |= ADS79xx_CR_WRITE;

	if (reset_counter) {
		w |= ADS79xx_CR_RESET_CHCNT;
	}

	if (range_2x) {
		w |= ADS79xx_CR_RANGE_2X;
	}

	if (map_gpio) {
		w |= ADS79xx_CR_OUTPUT_GPIO;
	}

	if (powerdown) {
		w |= ADS79xx_CR_POWERDOWN;
	}

	w |= (uint16_t)(gpio_out) & 0x0fu;

	return w;
}

static inline uint16_t ads79xx_sample(const struct ads79xx_config *cfg, uint16_t raw)
{
	return (raw >> (12 - cfg->resolution)) & BIT_MASK(cfg->resolution);
}

static inline uint16_t ads79xx_rx_addr(const struct ads79xx_config *cfg, uint16_t raw)
{
	return (raw >> 12) & BIT_MASK(4);
}

/*
 * Writes the sequence channel mask into the auto1 mask register.
 */
static int ads79xx_prog_auto1_mask(const struct device *dev, uint16_t mask)
{
	uint16_t rx;
	int ret;

	ret = ads79xx_spi_transfer(dev, ads79xx_ctrl_auto1_prog_entry(), &rx);
	if (ret) {
		return ret;
	}

	return ads79xx_spi_transfer(dev, mask, &rx);
}

static int ads79xx_set_mode_auto1(const struct device *dev, bool reset_counter)
{
	const struct ads79xx_config *cfg = dev->config;
	uint16_t w;
	uint16_t rx;

	w = ads79xx_ctrl_auto1(reset_counter, false, (cfg->range == 2), false, 0);

	return ads79xx_spi_transfer(dev, w, &rx);
}

static int ads79xx_continue(const struct device *dev, uint16_t *rx)
{
	return ads79xx_spi_transfer(dev, ads79xx_ctrl_continue(), rx);
}

static int ads79xx_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ads79xx_config *cfg = dev->config;
	uint8_t channels = POPCOUNT(sequence->channels);
	size_t needed = channels * sizeof(uint16_t);

	if (channels > cfg->channels || channels < 1) {
		return -EINVAL;
	}

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads79xx_data *data = CONTAINER_OF(ctx, struct ads79xx_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads79xx_data *data = CONTAINER_OF(ctx, struct ads79xx_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;
	k_sem_give(&data->sem);
	LOG_DBG("start_sampling");
}

static int ads79xx_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ads79xx_data *data = dev->data;
	const struct ads79xx_config *cfg = dev->config;
	int ret;

	if (sequence->resolution != cfg->resolution) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > cfg->channels) {
		LOG_ERR("unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		LOG_ERR("unsupported calibration");
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling not supported");
		return -ENOTSUP;
	}

	ret = ads79xx_validate_sequence(dev, sequence);
	if (ret) {
		LOG_ERR("invalid sequence / buffer too small");
		return ret;
	}

	data->buffer = sequence->buffer;
	data->repeat_buffer = data->buffer;
	data->channels = sequence->channels;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int ads79xx_read_async(const struct device *dev, const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct ads79xx_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = ads79xx_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int ads79xx_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return ads79xx_read_async(dev, sequence, NULL);
}

static void ads79xx_fail(struct ads79xx_data *data, int err)
{
	/* device may be in an undefined state - ensure device mode is re-programmed */
	data->auto1_mask = 0;
	if (data->ctx.sequence.options && data->ctx.sequence.options->interval_us != 0U) {
		adc_context_disable_timer(&data->ctx);
	}

	adc_context_complete(&data->ctx, err);
}

static void ads79xx_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct ads79xx_data *data = p1;
	const struct ads79xx_config *cfg = data->dev->config;
	const struct device *dev = data->dev;
	uint16_t dummy;
	int ret;

	/* Prime the ads79xx */
	ret = ads79xx_spi_transfer(dev, ads79xx_manual_command(cfg, 0), &dummy);
	if (ret) {
		LOG_ERR("SPI transfer failed (err %d)", ret);
	}

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		if (data->auto1_mask != data->channels) {
			LOG_DBG("programming auto-1 channel mask");
			ret = ads79xx_prog_auto1_mask(dev, data->channels);
			if (ret) {
				LOG_ERR("failed to configure acquisition (err %d)", ret);
				ads79xx_fail(data, ret);
				continue;
			}
			ret = ads79xx_set_mode_auto1(dev, true);
			if (ret) {
				LOG_ERR("failed to enter auto-1 (err %d)", ret);
				ads79xx_fail(data, ret);
				continue;
			}
			/* consume the first frame - its junk. */
			ret = ads79xx_continue(dev, NULL);
			if (ret) {
				LOG_ERR("SPI transfer failed (err %d)", ret);
				ads79xx_fail(data, ret);
				continue;
			}
			data->auto1_mask = data->channels;
		}

		int nsamples = POPCOUNT(data->channels);
		uint16_t rx = 0;

		for (int i = 0; i < nsamples; i++) {
			ret = ads79xx_continue(dev, &rx);
			if (ret) {
				LOG_ERR("acquisition failed (err %d)", ret);
				ads79xx_fail(data, ret);
				goto acquisition_failed;
			}
			*data->buffer++ = ads79xx_sample(cfg, rx);
			LOG_DBG("rx_addr: %d, sample: %d", ads79xx_rx_addr(cfg, rx),
				ads79xx_sample(cfg, rx));
		}
		adc_context_on_sampling_done(&data->ctx, data->dev);
acquisition_failed:;
	}
}

int ads79xx_init(const struct device *dev)
{
	const struct ads79xx_config *config = dev->config;
	struct ads79xx_data *data = dev->data;

	data->dev = dev;

	adc_context_init(&data->ctx);
	k_sem_init(&data->sem, 0, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
			ads79xx_acquisition_thread, data, NULL, NULL,
			CONFIG_ADC_ADS79XX_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);

	adc_context_unlock_unconditionally(&data->ctx);

	LOG_DBG("initialised (%d-bit : %d channels)", config->resolution, config->channels);

	return 0;
}

struct adc_driver_api ads79xx_api = {
	.channel_setup = ads79xx_channel_setup,
	.read = ads79xx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads79xx_read_async,
#endif
};

#define ADS79XX_SPI_OP (SPI_OP_MODE_MASTER | SPI_WORD_SET(16) | SPI_TRANSFER_MSB)

#define DT_INST_ADS79XX(inst, t) DT_INST(inst, ti_ads##t)

#define ADS79XX_INIT(t, n, chan, res)                                                              \
	static struct ads79xx_data ads##t##_data_##n;                                              \
	static const struct ads79xx_config ads##t##_cfg_##n = {                                    \
		.spi = SPI_DT_SPEC_GET(DT_INST_ADS79XX(n, t), ADS79XX_SPI_OP),                     \
		.channels = chan,                                                                  \
		.resolution = res,                                                                 \
		.range = DT_INST_PROP_OR(n, ti_range, 1),                                          \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_INST_ADS79XX(n, t), ads79xx_init, NULL, &ads##t##_data_##n,            \
			 &ads##t##_cfg_##n, POST_KERNEL, CONFIG_ADC_ADS79XX_INIT_PRIORITY,         \
			 &ads79xx_api);

/*
 * ads79xx - 12-bit ADCs
 */
#define DT_DRV_COMPAT   ti_ads7950
#define ADS7950_INIT(n) ADS79XX_INIT(7950, n, 4, 12)
DT_INST_FOREACH_STATUS_OKAY(ADS7950_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7951
#define ADS7951_INIT(n) ADS79XX_INIT(7951, n, 8, 12)
DT_INST_FOREACH_STATUS_OKAY(ADS7951_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7952
#define ADS7952_INIT(n) ADS79XX_INIT(7952, n, 12, 12)
DT_INST_FOREACH_STATUS_OKAY(ADS7952_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7953
#define ADS7953_INIT(n) ADS79XX_INIT(7953, n, 16, 12)
DT_INST_FOREACH_STATUS_OKAY(ADS7953_INIT)

/*
 * ads79xx - 10-bit ADCs
 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7954
#define ADS7954_INIT(n) ADS79XX_INIT(7954, n, 4, 10)
DT_INST_FOREACH_STATUS_OKAY(ADS7954_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7955
#define ADS7955_INIT(n) ADS79XX_INIT(7955, n, 8, 10)
DT_INST_FOREACH_STATUS_OKAY(ADS7955_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7956
#define ADS7956_INIT(n) ADS79XX_INIT(7956, n, 12, 10)
DT_INST_FOREACH_STATUS_OKAY(ADS7956_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7957
#define ADS7957_INIT(n) ADS79XX_INIT(7957, n, 16, 10)
DT_INST_FOREACH_STATUS_OKAY(ADS7957_INIT)

/*
 * ads79xx - 8-bit ADCs
 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7958
#define ADS7958_INIT(n) ADS79XX_INIT(7958, n, 4, 8)
DT_INST_FOREACH_STATUS_OKAY(ADS7958_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7959
#define ADS7959_INIT(n) ADS79XX_INIT(7959, n, 8, 8)
DT_INST_FOREACH_STATUS_OKAY(ADS7959_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7960
#define ADS7960_INIT(n) ADS79XX_INIT(7960, n, 12, 8)
DT_INST_FOREACH_STATUS_OKAY(ADS7960_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT   ti_ads7961
#define ADS7961_INIT(n) ADS79XX_INIT(7961, n, 16, 8)
DT_INST_FOREACH_STATUS_OKAY(ADS7961_INIT)
