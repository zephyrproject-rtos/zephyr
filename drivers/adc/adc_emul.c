/**
 * @file
 *
 * @brief Emulated ADC driver
 */

/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_adc_emul

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_emul, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_EMUL_MAX_RESOLUTION 16

typedef uint16_t adc_emul_res_t;

enum adc_emul_input_source {
	ADC_EMUL_CONST_VALUE,
	ADC_EMUL_CUSTOM_FUNC,
};

/**
 * @brief Channel of emulated ADC config
 *
 * This structure contains configuration of one channel of emulated ADC.
 */
struct adc_emul_chan_cfg {
	/** Pointer to function used to obtain input mV */
	adc_emul_value_func func;
	/** Pointer to data that are passed to @a func on call */
	void *func_data;
	/** Constant mV input value */
	uint32_t const_value;
	/** Gain used on output value */
	enum adc_gain gain;
	/** Reference source */
	enum adc_reference ref;
	/** Input source which is used to obtain input value */
	enum adc_emul_input_source input;
};

/**
 * @brief Emulated ADC config
 *
 * This structure contains constant data for given instance of emulated ADC.
 */
struct adc_emul_config {
	/** Number of supported channels */
	uint8_t num_channels;
};

/**
 * @brief Emulated ADC data
 *
 * This structure contains data structures used by a emulated ADC.
 */
struct adc_emul_data {
	/** Structure that handle state of ongoing read operation */
	struct adc_context ctx;
	/** Pointer to ADC emulator own device structure */
	const struct device *dev;
	/** Pointer to memory where next sample will be written */
	uint16_t *buf;
	/** Pointer to where will be data stored in case of repeated sampling */
	uint16_t *repeat_buf;
	/** Mask with channels that will be sampled */
	uint32_t channels;
	/** Mask created from requested resolution in read operation */
	uint16_t res_mask;
	/** Reference voltage for ADC_REF_VDD_1 source */
	uint16_t ref_vdd;
	/** Reference voltage for ADC_REF_EXTERNAL0 source */
	uint16_t ref_ext0;
	/** Reference voltage for ADC_REF_EXTERNAL1 source */
	uint16_t ref_ext1;
	/** Reference voltage for ADC_REF_INTERNAL source */
	uint16_t ref_int;
	/** Array of each channel configuration */
	struct adc_emul_chan_cfg *chan_cfg;
	/** Structure used for acquisition thread */
	struct k_thread thread;
	/** Semaphore used to control acquisition thread */
	struct k_sem sem;
	/** Mutex used to control access to channels config and ref voltages */
	struct k_mutex cfg_mtx;

	/** Stack for acquisition thread */
	K_KERNEL_STACK_MEMBER(stack,
			CONFIG_ADC_EMUL_ACQUISITION_THREAD_STACK_SIZE);
};

int adc_emul_const_value_set(const struct device *dev, unsigned int chan,
			     uint32_t value)
{
	const struct adc_emul_config *config = dev->config;
	struct adc_emul_data *data = dev->data;
	struct adc_emul_chan_cfg *chan_cfg;

	if (chan >= config->num_channels) {
		LOG_ERR("unsupported channel %d", chan);
		return -EINVAL;
	}

	chan_cfg = &data->chan_cfg[chan];

	k_mutex_lock(&data->cfg_mtx, K_FOREVER);

	chan_cfg->input = ADC_EMUL_CONST_VALUE;
	chan_cfg->const_value = value;

	k_mutex_unlock(&data->cfg_mtx);

	return 0;
}

int adc_emul_value_func_set(const struct device *dev, unsigned int chan,
			    adc_emul_value_func func, void *func_data)
{
	const struct adc_emul_config *config = dev->config;
	struct adc_emul_data *data = dev->data;
	struct adc_emul_chan_cfg *chan_cfg;

	if (chan >= config->num_channels) {
		LOG_ERR("unsupported channel %d", chan);
		return -EINVAL;
	}

	chan_cfg = &data->chan_cfg[chan];

	k_mutex_lock(&data->cfg_mtx, K_FOREVER);

	chan_cfg->func = func;
	chan_cfg->func_data = func_data;
	chan_cfg->input = ADC_EMUL_CUSTOM_FUNC;

	k_mutex_unlock(&data->cfg_mtx);

	return 0;
}

int adc_emul_ref_voltage_set(const struct device *dev, enum adc_reference ref,
			     uint16_t value)
{
	struct adc_emul_data *data = dev->data;
	int err = 0;

	k_mutex_lock(&data->cfg_mtx, K_FOREVER);

	switch (ref) {
	case ADC_REF_VDD_1:
		data->ref_vdd = value;
		break;
	case ADC_REF_INTERNAL:
		data->ref_int = value;
		break;
	case ADC_REF_EXTERNAL0:
		data->ref_ext0 = value;
		break;
	case ADC_REF_EXTERNAL1:
		data->ref_ext1 = value;
		break;
	default:
		err = -EINVAL;
	}

	k_mutex_unlock(&data->cfg_mtx);

	return err;
}

/**
 * @brief Convert @p ref to reference voltage value in mV
 *
 * @param data Internal data of ADC emulator
 * @param ref Select which reference source should be used
 *
 * @return Reference voltage in mV
 * @return 0 on error
 */
static uint16_t adc_emul_get_ref_voltage(struct adc_emul_data *data,
					 enum adc_reference ref)
{
	uint16_t voltage;

	k_mutex_lock(&data->cfg_mtx, K_FOREVER);

	switch (ref) {
	case ADC_REF_VDD_1:
		voltage = data->ref_vdd;
		break;
	case ADC_REF_VDD_1_2:
		voltage = data->ref_vdd / 2;
		break;
	case ADC_REF_VDD_1_3:
		voltage = data->ref_vdd / 3;
		break;
	case ADC_REF_VDD_1_4:
		voltage = data->ref_vdd / 4;
		break;
	case ADC_REF_INTERNAL:
		voltage = data->ref_int;
		break;
	case ADC_REF_EXTERNAL0:
		voltage = data->ref_ext0;
		break;
	case ADC_REF_EXTERNAL1:
		voltage = data->ref_ext1;
		break;
	default:
		voltage = 0;
	}

	k_mutex_unlock(&data->cfg_mtx);

	return voltage;
}

static int adc_emul_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_emul_config *config = dev->config;
	struct adc_emul_chan_cfg *emul_chan_cfg;
	struct adc_emul_data *data = dev->data;

	if (channel_cfg->channel_id >= config->num_channels) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (adc_emul_get_ref_voltage(data, channel_cfg->reference) == 0) {
		LOG_ERR("unsupported channel reference '%d'",
			channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("unsupported differential mode");
		return -ENOTSUP;
	}

	emul_chan_cfg = &data->chan_cfg[channel_cfg->channel_id];

	k_mutex_lock(&data->cfg_mtx, K_FOREVER);

	emul_chan_cfg->gain = channel_cfg->gain;
	emul_chan_cfg->ref = channel_cfg->reference;

	k_mutex_unlock(&data->cfg_mtx);

	return 0;
}

/**
 * @brief Check if buffer in @p sequence is big enough to hold all ADC samples
 *
 * @param dev ADC emulator device
 * @param sequence ADC sequence description
 *
 * @return 0 on success
 * @return -ENOMEM if buffer is not big enough
 */
static int adc_emul_check_buffer_size(const struct device *dev,
					 const struct adc_sequence *sequence)
{
	const struct adc_emul_config *config = dev->config;
	uint8_t channels = 0;
	size_t needed;
	uint32_t mask;

	for (mask = BIT(config->num_channels - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	needed = channels * sizeof(adc_emul_res_t);
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
 * @param dev ADC emulator device
 * @param sequence ADC sequence description
 *
 * @return 0 on success
 * @return -ENOTSUP if requested resolution or channel is out side of supported
 *         range
 * @return -ENOMEM if buffer is not big enough
 *         (see @ref adc_emul_check_buffer_size)
 * @return other error code returned by adc_context_wait_for_completion
 */
static int adc_emul_start_read(const struct device *dev,
			       const struct adc_sequence *sequence)
{
	const struct adc_emul_config *config = dev->config;
	struct adc_emul_data *data = dev->data;
	int err;

	if (sequence->resolution > ADC_EMUL_MAX_RESOLUTION ||
	    sequence->resolution == 0) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > config->num_channels) {
		LOG_ERR("unsupported channels in mask: 0x%08x",
			sequence->channels);
		return -ENOTSUP;
	}

	err = adc_emul_check_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->res_mask = BIT_MASK(sequence->resolution);
	data->buf = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_emul_read_async(const struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	struct adc_emul_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_emul_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int adc_emul_read(const struct device *dev,
			 const struct adc_sequence *sequence)
{
	return adc_emul_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_emul_data *data = CONTAINER_OF(ctx, struct adc_emul_data,
						  ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buf = data->buf;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_emul_data *data = CONTAINER_OF(ctx, struct adc_emul_data,
						  ctx);

	if (repeat_sampling) {
		data->buf = data->repeat_buf;
	}
}

/**
 * @brief Convert input voltage of ADC @p chan to raw output value
 *
 * @param data Internal data of ADC emulator
 * @param chan ADC channel to sample
 * @param result Raw output value
 *
 * @return 0 on success
 * @return -EINVAL if failed to get reference voltage or unknown input is
 *         selected
 * @return other error code returned by custom function
 */
static int adc_emul_get_chan_value(struct adc_emul_data *data,
				   unsigned int chan,
				   adc_emul_res_t *result)
{
	struct adc_emul_chan_cfg *chan_cfg = &data->chan_cfg[chan];
	uint32_t input_mV;
	uint32_t ref_v;
	uint64_t temp; /* Temporary 64 bit value prevent overflows */
	int err = 0;

	k_mutex_lock(&data->cfg_mtx, K_FOREVER);

	/* Get input voltage */
	switch (chan_cfg->input) {
	case ADC_EMUL_CONST_VALUE:
		input_mV = chan_cfg->const_value;
		break;

	case ADC_EMUL_CUSTOM_FUNC:
		err = chan_cfg->func(data->dev, chan, chan_cfg->func_data,
				     &input_mV);
		if (err) {
			LOG_ERR("failed to read channel %d (err %d)",
				chan, err);
			goto out;
		}
		break;

	default:
		LOG_ERR("unknown input source %d", chan_cfg->input);
		err = -EINVAL;
		goto out;
	}

	/* Get reference voltage and apply inverted gain */
	ref_v = adc_emul_get_ref_voltage(data, chan_cfg->ref);
	err = adc_gain_invert(chan_cfg->gain, &ref_v);
	if (ref_v == 0 || err) {
		LOG_ERR("failed to get ref voltage (channel %d)", chan);
		err = -EINVAL;
		goto out;
	}

	/* Calculate output value */
	temp = (uint64_t)input_mV * data->res_mask / ref_v;

	/* If output value is greater than resolution, it has to be trimmed */
	if (temp > data->res_mask) {
		temp = data->res_mask;
	}

	*result = temp;

out:
	k_mutex_unlock(&data->cfg_mtx);

	return err;
}

/**
 * @brief Main function of thread which is used to collect samples from
 *        emulated ADC. When adc_context_start_sampling give semaphore,
 *        for each requested channel value function is called. Returned
 *        mV value is converted to output using reference voltage, gain
 *        and requested resolution.
 *
 * @param data Internal data of ADC emulator
 *
 * @return This thread should not end
 */
static void adc_emul_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adc_emul_data *data = p1;
	int err;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		err = 0;

		while (data->channels) {
			adc_emul_res_t result = 0;
			unsigned int chan = find_lsb_set(data->channels) - 1;

			LOG_DBG("reading channel %d", chan);

			err = adc_emul_get_chan_value(data, chan, &result);
			if (err) {
				adc_context_complete(&data->ctx, err);
				break;
			}

			LOG_DBG("read channel %d, result = %d", chan, result);

			*data->buf++ = result;
			WRITE_BIT(data->channels, chan, 0);
		}

		if (!err) {
			adc_context_on_sampling_done(&data->ctx, data->dev);
		}
	}
}

/**
 * @brief Function called on init for each ADC emulator device. It setups all
 *        channels to return constant 0 mV and create acquisition thread.
 *
 * @param dev ADC emulator device
 *
 * @return 0 on success
 */
static int adc_emul_init(const struct device *dev)
{
	const struct adc_emul_config *config = dev->config;
	struct adc_emul_data *data = dev->data;
	int chan;

	data->dev = dev;

	k_sem_init(&data->sem, 0, 1);
	k_mutex_init(&data->cfg_mtx);

	for (chan = 0; chan < config->num_channels; chan++) {
		struct adc_emul_chan_cfg *chan_cfg = &data->chan_cfg[chan];

		chan_cfg->func = NULL;
		chan_cfg->func_data = NULL;
		chan_cfg->input = ADC_EMUL_CONST_VALUE;
		chan_cfg->const_value = 0;
	}

	k_thread_create(&data->thread, data->stack,
			CONFIG_ADC_EMUL_ACQUISITION_THREAD_STACK_SIZE,
			adc_emul_acquisition_thread,
			data, NULL, NULL,
			CONFIG_ADC_EMUL_ACQUISITION_THREAD_PRIO,
			0, K_NO_WAIT);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define ADC_EMUL_INIT(_num)						\
	static DEVICE_API(adc, adc_emul_api_##_num) = {			\
		.channel_setup = adc_emul_channel_setup,		\
		.read = adc_emul_read,					\
		.ref_internal = DT_INST_PROP(_num, ref_internal_mv),	\
		IF_ENABLED(CONFIG_ADC_ASYNC,				\
			(.read_async = adc_emul_read_async,))		\
	};								\
									\
	static struct adc_emul_chan_cfg					\
		adc_emul_ch_cfg_##_num[DT_INST_PROP(_num, nchannels)];	\
									\
	static const struct adc_emul_config adc_emul_config_##_num = {	\
		.num_channels = DT_INST_PROP(_num, nchannels),		\
	};								\
									\
	static struct adc_emul_data adc_emul_data_##_num = {		\
		ADC_CONTEXT_INIT_TIMER(adc_emul_data_##_num, ctx),	\
		ADC_CONTEXT_INIT_LOCK(adc_emul_data_##_num, ctx),	\
		ADC_CONTEXT_INIT_SYNC(adc_emul_data_##_num, ctx),	\
		.chan_cfg = adc_emul_ch_cfg_##_num,			\
		.ref_vdd = DT_INST_PROP(_num, ref_vdd_mv),		\
		.ref_ext0 = DT_INST_PROP(_num, ref_external0_mv),	\
		.ref_ext1 = DT_INST_PROP(_num, ref_external1_mv),	\
		.ref_int = DT_INST_PROP(_num, ref_internal_mv),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(_num, adc_emul_init, NULL,		\
			      &adc_emul_data_##_num,			\
			      &adc_emul_config_##_num, POST_KERNEL,	\
			      CONFIG_ADC_INIT_PRIORITY,			\
			      &adc_emul_api_##_num);

DT_INST_FOREACH_STATUS_OKAY(ADC_EMUL_INIT)
