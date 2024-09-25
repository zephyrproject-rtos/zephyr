/*
 * Copyright 2021 Google LLC
 * Copyright 2022 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>

#include <hardware/adc.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(adc_rpi, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_RPI_MAX_RESOLUTION 12

/** Bits numbers of rrobin register mean an available number of channels. */
#define ADC_RPI_CHANNEL_NUM (ADC_CS_RROBIN_MSB - ADC_CS_RROBIN_LSB + 1)

/**
 * @brief RaspberryPi Pico ADC config
 *
 * This structure contains constant data for given instance of RaspberryPi Pico ADC.
 */
struct adc_rpi_config {
	/** Number of supported channels */
	uint8_t num_channels;
	/** pinctrl configs */
	const struct pinctrl_dev_config *pcfg;
	/** function pointer to irq setup */
	void (*irq_configure)(void);
	/** Pointer to clock controller device */
	const struct device *clk_dev;
	/** Clock id of ADC clock */
	clock_control_subsys_t clk_id;
	/** Reset controller config */
	const struct reset_dt_spec reset;
};

/**
 * @brief RaspberryPi Pico ADC data
 *
 * This structure contains data structures used by a RaspberryPi Pico ADC.
 */
struct adc_rpi_data {
	/** Structure that handle state of ongoing read operation */
	struct adc_context ctx;
	/** Pointer to RaspberryPi Pico ADC own device structure */
	const struct device *dev;
	/** Pointer to memory where next sample will be written */
	uint16_t *buf;
	/** Pointer to where will be data stored in case of repeated sampling */
	uint16_t *repeat_buf;
	/** Mask with channels that will be sampled */
	uint32_t channels;
};

static inline void adc_start_once(void)
{
	hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);
}

static inline uint16_t adc_get_result(void)
{
	return (uint16_t)adc_hw->result;
}

static inline bool adc_get_err(void)
{
	return (adc_hw->cs & ADC_CS_ERR_BITS) ? true : false;
}

static inline void adc_clear_errors(void)
{
	/* write 1 to clear */
	hw_set_bits(&adc_hw->fcs, ADC_FCS_OVER_BITS);
	hw_set_bits(&adc_hw->fcs, ADC_FCS_UNDER_BITS);
	hw_set_bits(&adc_hw->fcs, ADC_FCS_ERR_BITS);
	hw_set_bits(&adc_hw->cs, ADC_CS_ERR_STICKY_BITS);
}

static inline void adc_enable(void)
{
	adc_hw->cs = ADC_CS_EN_BITS;
	while (!(adc_hw->cs & ADC_CS_READY_BITS))
		;
}

static int adc_rpi_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_rpi_config *config = dev->config;

	if (channel_cfg->channel_id >= config->num_channels) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("unsupported differential mode");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not valid");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Check if buffer in @p sequence is big enough to hold all ADC samples
 *
 * @param dev RaspberryPi Pico ADC device
 * @param sequence ADC sequence description
 *
 * @return 0 on success
 * @return -ENOMEM if buffer is not big enough
 */
static int adc_rpi_check_buffer_size(const struct device *dev,
				     const struct adc_sequence *sequence)
{
	const struct adc_rpi_config *config = dev->config;
	uint8_t channels = 0;
	size_t needed;
	uint32_t mask;

	for (mask = BIT(config->num_channels - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	needed = channels * sizeof(uint16_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief Start processing read request
 *
 * @param dev RaspberryPi Pico ADC device
 * @param sequence ADC sequence description
 *
 * @return 0 on success
 * @return -ENOTSUP if requested resolution or channel is out side of supported
 *         range
 * @return -ENOMEM if buffer is not big enough
 *         (see @ref adc_rpi_check_buffer_size)
 * @return other error code returned by adc_context_wait_for_completion
 */
static int adc_rpi_start_read(const struct device *dev,
			      const struct adc_sequence *sequence)
{
	const struct adc_rpi_config *config = dev->config;
	struct adc_rpi_data *data = dev->data;
	int err;

	if (sequence->resolution > ADC_RPI_MAX_RESOLUTION ||
	    sequence->resolution == 0) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > config->num_channels) {
		LOG_ERR("unsupported channels in mask: 0x%08x",
			sequence->channels);
		return -ENOTSUP;
	}

	err = adc_rpi_check_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->buf = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

/**
 * Interrupt handler
 */
static void adc_rpi_isr(const struct device *dev)
{
	struct adc_rpi_data *data = dev->data;
	uint16_t result;
	uint8_t ainsel;

	/* Fetch result */
	result = adc_get_result();
	ainsel = adc_get_selected_input();

	/* Drain FIFO */
	while (!adc_fifo_is_empty()) {
		(void)adc_fifo_get();
	}

	/* Abort converting if error detected. */
	if (adc_get_err()) {
		adc_context_complete(&data->ctx, -EIO);
		return;
	}

	/* Copy to buffer and mark this channel as completed to channels bitmap. */
	*data->buf++ = result;
	data->channels &= ~(BIT(ainsel));

	/* Notify result if all data gathered. */
	if (data->channels == 0) {
		adc_context_on_sampling_done(&data->ctx, dev);
		return;
	}

	/* Kick next channel conversion */
	ainsel = (uint8_t)(find_lsb_set(data->channels) - 1);
	adc_select_input(ainsel);
	adc_start_once();
}

static int adc_rpi_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_rpi_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_rpi_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int adc_rpi_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	return adc_rpi_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_rpi_data *data = CONTAINER_OF(ctx, struct adc_rpi_data,
						 ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buf = data->buf;

	adc_clear_errors();

	/* Find next channel and start conversion */
	adc_select_input(find_lsb_set(data->channels) - 1);
	adc_start_once();
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_rpi_data *data = CONTAINER_OF(ctx, struct adc_rpi_data,
						 ctx);

	if (repeat_sampling) {
		data->buf = data->repeat_buf;
	}
}

/**
 * @brief Function called on init for each RaspberryPi Pico ADC device. It setups all
 *        channels to return constant 0 mV and create acquisition thread.
 *
 * @param dev RaspberryPi Pico ADC device
 *
 * @return 0 on success
 */
static int adc_rpi_init(const struct device *dev)
{
	const struct adc_rpi_config *config = dev->config;
	struct adc_rpi_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->clk_dev, config->clk_id);
	if (ret < 0) {
		return ret;
	}

	ret = reset_line_toggle_dt(&config->reset);
	if (ret < 0) {
		return ret;
	}

	config->irq_configure();

	/*
	 * Configure the FIFO control register.
	 * Set the threshold as 1 for getting notification immediately
	 * on converting completed.
	 */
	adc_fifo_setup(true, false, 1, true, true);

	/* Set max speed to conversion */
	adc_set_clkdiv(0.f);

	/* Enable ADC and wait becoming READY */
	adc_enable();

	/* Enable FIFO interrupt */
	adc_irq_set_enabled(true);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define IRQ_CONFIGURE_FUNC(idx)						   \
	static void adc_rpi_configure_func_##idx(void)			   \
	{								   \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), \
			    adc_rpi_isr, DEVICE_DT_INST_GET(idx), 0);	   \
		irq_enable(DT_INST_IRQN(idx));				   \
	}

#define IRQ_CONFIGURE_DEFINE(idx) .irq_configure = adc_rpi_configure_func_##idx

#define ADC_RPI_INIT(idx)                                                                          \
	IRQ_CONFIGURE_FUNC(idx)                                                                    \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static struct adc_driver_api adc_rpi_api_##idx = {                                         \
		.channel_setup = adc_rpi_channel_setup,                                            \
		.read = adc_rpi_read,                                                              \
		.ref_internal = DT_INST_PROP(idx, vref_mv),                                        \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_rpi_read_async,))                  \
	};                                                                                         \
	static const struct adc_rpi_config adc_rpi_config_##idx = {                                \
		.num_channels = ADC_RPI_CHANNEL_NUM,                                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(idx, clocks, 0, clk_id),      \
		.reset = RESET_DT_SPEC_INST_GET(idx),                                              \
		IRQ_CONFIGURE_DEFINE(idx),                                                         \
	};                                                                                         \
	static struct adc_rpi_data adc_rpi_data_##idx = {                                          \
		ADC_CONTEXT_INIT_TIMER(adc_rpi_data_##idx, ctx),                                   \
		ADC_CONTEXT_INIT_LOCK(adc_rpi_data_##idx, ctx),                                    \
		ADC_CONTEXT_INIT_SYNC(adc_rpi_data_##idx, ctx),                                    \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, adc_rpi_init, NULL,                                             \
			      &adc_rpi_data_##idx,                                                 \
			      &adc_rpi_config_##idx, POST_KERNEL,                                  \
			      CONFIG_ADC_INIT_PRIORITY,                                            \
			      &adc_rpi_api_##idx)

DT_INST_FOREACH_STATUS_OKAY(ADC_RPI_INIT);
