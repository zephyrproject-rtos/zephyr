/* Copyright (c) 2022 Trent Piepho <trent.piepho@igorinstitute.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for Microchip MCP3564/2/1 delta-sigma ADCs.
 *
 * This attempts to map this delta-sigma ADC into the Zephyr API, which was clearly designed for
 * successive approximation ADCs, and doesn't quite fit perfectly.  The channel muxing and
 * output format is also not quite an exact fit.
 *
 * Resolutions of 32 or 24 bits are supported.  The scale is always that of a 24-bit signed
 * value, i.e. -2^23 to 2^23 - 1.  The 24 bit format is packed into 3 bytes.  The 32 bit format
 * is signed extended to the full 32 bits, but again, should be scaled as if it was 24 bits.
 * The scaled range in the 32 bit format is NOT -1*Fs to +1*Fs, as expected, but -2*Fs to +2*Fs.
 * I.e., it may return over-range values, which are not as accurate as the in-range values.
 * Keep this in mind or your calculations might overflow.
 *
 * The ADC does not have a true single-ended mode.  It always returns signed values.  The
 * "single-ended" channels, 0 to 8, only mean the negative mux is connected to analog ground
 * (mux value 0x8).
 *
 * This ADC is modeled as has having 17 channels.  The first sixteen, channels 0 - 15,
 * correspond to the fixed channels from table 5-14 in the datasheet: the external inputs in
 * single-ended mode (8), in differential pairs (4), and some internal channels (4).
 *
 * Channel 16 allows the positive and negative mux to be freely selected, while the mux values
 * for channels 0-15 can not be changed.  Think of channels 0 - 15 as the "SCAN" mode channels
 * and channel 16 as the "MUX" mode channel.
 *
 * Setting the gain for any channel sets it for all.  The channels to not have individual gains.
 * Internal channels 12, 13, and 14 have specific fixed gains, see datasheet.
 *
 * The scan mode of the ADC allows any or all of channels 0 to 15 to be selected in a sequence,
 * or channel 16 alone.  Channel 16 can not be in the same sequence as the other channels.  The
 * channels are scanned from largest ID to smallest, backward of how one might expect.
 *
 * The ΔΣ ADC oversampling rate is not the same concept as the oversampling field in the Zephyr
 * ADC API.  The driver allows the OSR value to be fixed in the build configuration and only a
 * sequence oversampling field of 0 (1 conversion per sample) will be supported.  This makes the
 * ADC look like a "normal" one.  Or the OSR can be run time configured via the sequence
 * oversampling field, but the value is based on datasheet table 5-6 and NOT the power of 2
 * conversions to average that is expected.
 *
 * acquisition_time is not used as the time the sampling capacitor is connected to the input.
 * Instead, it used as the inter-channel delay when sampling multiple channels (SCAN mode).  In
 * a ΔΣ ADC like this, acquisition_time is really more something related to oversampling ratio,
 * but there is another field we can use for that.  Only ADC_ACQ_TIME_TICKS units is supported.
 *
 * The sampling interval (adc_sequence_options.interval_us) can use the MCP356x's internal delay
 * counter.  Using the ADC's counter vs a Zephyr timer is a compile time configuration option.
 *
 * The ADC's internal MCLK may vary over a very wide range, which will effect this delay.  To
 * get an accurate sample rate an external MCLK is needed.  Using the ADC's timer means the time
 * between samples will not be affected by Zephyr interrupt latency or thread scheduling, and
 * should have much less jitter than using a Zephyr timer.
 *
 * Using the ADC timer limits the delay to 2^24 ticks of DMCLK (~13.65 sec with prescale 1 and
 * default MCLK).  interval_us will be the time between the last channel of one sample set and
 * the first channel of the next.  I.e., it does NOT include the time to sample the channels.
 * Sampling will begin immediately on an adc_read() call and not wait interval_us first.
 *
 * In ADC timer mode, channel 16 will run in contiuous mode, which allows for a higher sample
 * rate.  Channel 16 does not support non-zero interval_us values in ADC timer mode.
 *
 * Alternatively, a Zephyr timer can be used to generate the sampling rate.  This requires a SPI
 * transfer to start each conversion, and this will add jitter due to Zephyr interrupt latency
 * and thread scheduling delays.  interval_us will be the time between starts of each channel
 * set.  I.e., it does include the sampling time.  If the time to sample the channels is greater
 * than interval_us, the one-shot samples effectively run back-to-back, possibly slower than
 * interval_us.  The first sample from an adc_read() call does not start until after a
 * interval_us delay.  Channel 16 does not run in continuous mode and can not sample at as high
 * of a rate.  This is not just because of extra SPI commands and more overhead.  The manner in
 * which a ΔΣ ADC creates the samples is different between continuous conversion and a sequence
 * of one-shot conversions.
 *
 * Setting calibrate in the adc_sequence will enable the offset cancellatin algorithm, see
 * §5.1.3, which causes the ADC to sample twice, with the mux set in either direction, to
 * attempt to cancel out offset error.  This doubles the time each sample takes.
 */

/* Missing features.
 *
 * Configure bias current (in device tree?).
 * Configure internal clock output to MCLK pin (dts).
 * Standby vs shutdown when not sampling (dts?).
 * Tweak IRQ timeout to just to expected sampling rate.
 * Support the CRC data checking mode.
 * Detect POR or unexpected config change via status bits when reading data.
 * Timestamp the DRDY interrupt, to get more accurate data timestamps.
 */

#include <drivers/adc.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <kernel.h>
#include <logging/log.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <zephyr.h>

#include "adc_mcp356x.h"

LOG_MODULE_REGISTER(adc_mcp356x, CONFIG_ADC_LOG_LEVEL);

/* Verbose debug logging.  Individual register access. IRQ timing. */
#define VERBOSE_LOG	0

#if VERBOSE_LOG
#define LOG_VDBG(...)	LOG_DBG(__VA_ARGS__)
#else
#define LOG_VDBG(...)	(void)(0)
#endif

#if !IS_ENABLED(CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER)
#define ADC_CONTEXT_USES_KERNEL_TIMER
#endif
#include "adc_context.h"

struct mcp356x_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec irq;
	unsigned int mclk_freq;	/* External or internal clock rate */
	uint8_t channels;	/* Number of channels */
	uint8_t addr;		/* Device address selection bits */
	uint8_t pre;		/* Prescaler */
	uint8_t boost;		/* Boost setting (register value) */
	bool ext_clock;		/* Using external or internal clock */
	bool push_pull;		/* Drive mode of interrupt pin */
};

struct mcp356x_data {
	struct adc_context ctx;		/* Must have this, ADC_CONTEXT_INIT macros use it */
	struct gpio_callback drdy_cb;	/* For data ready IRQ */
	struct k_thread thread;		/* Acquisition thread */
	struct k_sem acq_sem;		/* Signal acq thread for next sample */
	struct k_sem drdy_sem;		/* Signal data ready IRQ */

	uint8_t *write_ptr;		/* Current address to send data */

	uint16_t ch_mask;		/* Allow channels, based on chip type */
	uint8_t delay;			/* Inter-channel delay (register code >> 16) */
	uint8_t resolution;		/* Current resolution (in bits) */
	uint8_t osr;			/* Ovesampling (register code, not shifted) */
	uint8_t gain;			/* Gain (register code) */
	bool az_mux;			/* Auto-zero mux */

	K_KERNEL_STACK_MEMBER(stack,
			CONFIG_ADC_MCP356X_ACQUISITION_THREAD_STACK_SIZE);
};

/* There are functions for unaligned be24 and le24, but none for CPU endian! */
static inline uint32_t get_cpu24(const uint8_t src[3])
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return src[0] << 16 | src[1] << 8 || src[0];
#else
	return UNALIGNED_GET((const uint32_t *)src) & BIT_MASK(24);
#endif
}

static int reg_read_size(const struct device *dev, uint8_t reg, int len, uint8_t *status)
{
	const struct mcp356x_config *config = dev->config;
	uint8_t cmd = config->addr | (reg & 0xf) << 2 | OP_READ_INC;
	uint32_t data;

	__ASSERT(len <= 4, "ADC register length %d > 4", len);

	const struct spi_buf tx[] = {
		{ .buf = &cmd, .len = sizeof(cmd) },
	};
	const struct spi_buf rx[] = {
		{ .buf = status, .len = sizeof(*status) },
		{ .buf = &data, .len = len },
	};
	const struct spi_buf_set tx_set = { .buffers = tx, .count = ARRAY_SIZE(tx) };
	const struct spi_buf_set rx_set = { .buffers = rx, .count = ARRAY_SIZE(rx) };

	int err = spi_transceive_dt(&config->bus, &tx_set, &rx_set);

	data = sys_be32_to_cpu(data) >> (4-len) * 8;

	LOG_VDBG("%c: res %d, status 0x%02x, reg 0x%1x = 0x%0*x", 'R',
		 err, status ? *status : 0, reg, len*2, data);

	return err ? err : data;
}

static inline int reg_read(const struct device *dev, uint8_t reg)
{
	return reg_read_size(dev, reg, reg_lens[reg], NULL);
}

static int reg_write_size(const struct device *dev, uint8_t reg, int len, uint32_t val,
			  uint8_t *status)
{
	const struct mcp356x_config *config = dev->config;
	uint8_t cmd = config->addr | (reg & 0xf) << 2 | OP_WRITE_INC;
	uint32_t data = sys_cpu_to_be32(val << (4-len) * 8);

	__ASSERT(len <= 4, "ADC register length %d > 4", len);

	const struct spi_buf tx[] = {
		{ .buf = &cmd, .len = sizeof(cmd) },
		{ .buf = &data, .len = len },
	};
	const struct spi_buf rx[] = {
		{ .buf = status, .len = sizeof(*status) },
	};
	const struct spi_buf_set tx_set = { .buffers = tx, .count = ARRAY_SIZE(tx) };
	const struct spi_buf_set rx_set = { .buffers = rx, .count = ARRAY_SIZE(rx) };

	int err = spi_transceive_dt(&config->bus, &tx_set, &rx_set);

	LOG_VDBG("%c: res %d, status 0x%02x, reg 0x%1x = 0x%0*x", 'W',
		 err, status ? *status : 0, reg, len*2, data);

	return err;
}

/* Read ADC data register, write to buffer, convert endian */
static int adcdata_read(const struct device *dev, int bytes, void *buffer)
{
	const struct mcp356x_config *config = dev->config;
	uint8_t cmd = config->addr | OP_READ;
	uint8_t status;
	uint32_t data = 0;

	__ASSERT(bytes == 4 || bytes == 3, "Incorrect adcdata read size");

	const struct spi_buf tx[] = {
		{ .buf = &cmd, .len = sizeof(cmd) },
	};
	const struct spi_buf rx[] = {
		{ .buf = &status, .len = sizeof(status) },
		{ .buf = &data, .len = bytes },
	};
	const struct spi_buf_set tx_set = { .buffers = tx, .count = ARRAY_SIZE(tx) };
	const struct spi_buf_set rx_set = { .buffers = rx, .count = ARRAY_SIZE(rx) };

	int err = spi_transceive_dt(&config->bus, &tx_set, &rx_set);

	if (bytes == 4) {
		/* Buffer must be aligned */
		*(uint32_t *)buffer = sys_be32_to_cpu(data);
	} else {
		/* Buffer might not be aligned */
		sys_put_be24(data, buffer);
	}
	LOG_VDBG("D: res %d, status 0x%02x, %d 0x%0*x", err, status, bytes, bytes*2, data);

	return err ? err : status;
}


static inline int reg_write(const struct device *dev, uint8_t reg, uint32_t val)
{
	return reg_write_size(dev, reg, reg_lens[reg], val, NULL);
}

/* Issue a "fast command", see datasheet §6.2.5. */
static int fast_write(const struct device *dev, uint8_t fast_cmd)
{
	const struct mcp356x_config *config = dev->config;
	uint8_t cmd = config->addr | fast_cmd | OP_FAST_CMD;
	uint8_t status;
	const struct spi_buf tx[] = {
		{ .buf = &cmd, .len = sizeof(cmd) },
	};
	const struct spi_buf rx[] = {
		{ .buf = &status, .len = sizeof(status) },
	};
	const struct spi_buf_set tx_set = { .buffers = tx, .count = ARRAY_SIZE(tx) };
	const struct spi_buf_set rx_set = { .buffers = rx, .count = ARRAY_SIZE(rx) };

	int err = spi_transceive_dt(&config->bus, &tx_set, &rx_set);

	LOG_VDBG("%c: res %d, status 0x%02x, reg 0x%1x = 0x%0*x", 'C',
		 err, status, 0, 2, cmd);

	return err ? err : status;
}

static int get_bits_res(int resolution)
{
	switch (resolution) {
	case 32:
		return REG_CONFIG3_DATA_FORMAT_32_RJ;
	case 24:
		return REG_CONFIG3_DATA_FORMAT_24;
	default:
		return -EINVAL;
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct mcp356x_data *data = CONTAINER_OF(ctx, struct mcp356x_data, ctx);

	if (repeat_sampling) {
		/* I'm assuming this means start over from the beginning of the
		 * buffer rather than repeat the last sample.
		 */
		data->write_ptr = ctx->sequence.buffer;
	}
	/* else, writeptr += num_ch, which was already done in the acq thread
	 * after reading the samples, so we have nothing to do here.
	 */
}

/* This kicks mcp356x_acquisition_thread to run */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcp356x_data *data = CONTAINER_OF(ctx, struct mcp356x_data, ctx);

	k_sem_give(&data->acq_sem);
}

/* Read all configured channels after each DRDY interrupt, signalled via &data->drdy_sem */
static int sample_channels(const struct device *dev, uint32_t channels, int bps)
{
	struct mcp356x_data *data = dev->data;
	int channel;

	for (; channels; channels &= ~BIT(channel)) {
		channel = find_msb_set(channels) - 1;

		/* Wait for IRQ */
		k_sem_take(&data->drdy_sem, K_SECONDS(12));


		int ret = adcdata_read(dev, bps, data->write_ptr);

		if (ret < 0) {
			LOG_DBG("Read fail! SPI error %d", ret);
			return ret;
		} else if (ret & STATUS_nDR) {
			LOG_DBG("Read fail! Unexpected status 0x%02x", ret);
			return -ENODATA;
		}

		LOG_DBG("Channel %d data %0*x to %p", channel, bps * 2,
			bps == 3 ? get_cpu24(data->write_ptr) : *(uint32_t *)data->write_ptr,
			data->write_ptr);

		data->write_ptr += bps;
	}
	return 0;
}

#if IS_ENABLED(CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER)
/* This is run from adc_context_start_read().  Start sampling immediately, the ADC timer will
 * delay between samples, but not before the first one.  My reading of interval_us is that this
 * is the correct behavior and that delaying before the first sample is wrong.
 */
static void adc_context_enable_timer(struct adc_context *ctx)
{
	LOG_DBG("Start sampling");
	adc_context_start_sampling(ctx);
}

/* We don't use adc_context_on_sampling_done(), so there are no real call sites for
 * adc_context_disable_timer()
 */
static void adc_context_disable_timer(struct adc_context *ctx)
{
}

/* Deal with finishing one sample and moving on to the next.  Like
 * adc_context_on_sampling_done() but works with the ADC running in continuous mode and using
 * its internal timer.
 */
static bool next_samples(const struct device *dev)
{
	struct mcp356x_data *data = dev->data;
	enum adc_action action = ADC_ACTION_CONTINUE;

	if (data->ctx.sequence.options && data->ctx.options.callback) {
		action = data->ctx.options.callback(dev, &data->ctx.sequence,
						    data->ctx.sampling_index);
	}
	if (action == ADC_ACTION_CONTINUE) {
		if (!data->ctx.sequence.options ||
		    ++data->ctx.sampling_index > data->ctx.options.extra_samplings) {
			action = ADC_ACTION_FINISH;
		}
	}
	if (action == ADC_ACTION_FINISH) {
		fast_write(dev, CMD_STANDBY);
		adc_context_complete(&data->ctx, 0);
		return false;
	}
	adc_context_update_buffer_pointer(&data->ctx, action == ADC_ACTION_REPEAT);
	return true;
}

#else /* CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER */

/* In system timer mode, each sample is one-shot.  We just call adc_context_on_sampling_done(),
 * and it starts the next sample if necessary.
 */
static bool next_samples(const struct device *dev)
{
	struct mcp356x_data *data = dev->data;

	adc_context_on_sampling_done(&data->ctx, dev);
	return false; /* No more samples, it's always one at a time */
}
#endif /* CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER */

/* Thread waits for adc_context_start_sampling(), then does one conversion sequence when kicked.
 * In continuous mode, loops until the sequence finishes.  In one-shot mode, does one sample,
 * then goes back to waiting for adc_context_start_sampling().
 *
 * Sampling is done by sending the start command, waiting for the drdy irq, and then reading the
 * data out, which is in sample_channels().
 */
static void mcp356x_acquisition_thread(const struct device *dev)
{
	struct mcp356x_data *data = dev->data;

	while (true) {
		k_sem_take(&data->acq_sem, K_FOREVER);
		/* Begin a new seqeunce */

		uint32_t channels = data->ctx.sequence.channels;
		const int bps = data->ctx.sequence.resolution / 8;

		fast_write(dev, CMD_START);

		/* In continuous mode, this will loop until finished with the sequence.  In
		 * one-shot mode this will run once as next_samples() always returns false.i
		 */
		int ret = 0;

		do {
			ret = sample_channels(dev, channels, bps);
		} while (!ret && next_samples(dev));

		if (ret) {
			/* Failure, we need to stop the sequence and the ADC */
			if (IS_ENABLED(CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER))
				fast_write(dev, CMD_STANDBY);
			adc_context_complete(&data->ctx, ret);
		}
	}
}

static int mcp356x_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *cc)
{
	struct mcp356x_data *data = dev->data;
	const struct mcp356x_config *config = dev->config;

	/* Verify some things before we start programming the device registers */

	if (cc->channel_id > 16 || cc->reference != ADC_REF_EXTERNAL0) {
		return -EINVAL;
	}
	if (cc->input_positive >= 16 || cc->input_negative >= 16) {
		return -EINVAL;
	}
	if (!cc->differential) {
		LOG_WRN("The MCP356x only supports differential mode");
		return -EINVAL;
	}

	uint8_t gain;

	switch (cc->gain) {
	case ADC_GAIN_1_3:
		gain = REG_CONFIG2_GAIN_1_3;
		break;
	case ADC_GAIN_1:
		gain = REG_CONFIG2_GAIN_1;
		break;
	case ADC_GAIN_2:
		gain = REG_CONFIG2_GAIN_2;
		break;
	case ADC_GAIN_4:
		gain = REG_CONFIG2_GAIN_4;
		break;
	case ADC_GAIN_8:
		gain = REG_CONFIG2_GAIN_8;
		break;
	case ADC_GAIN_16:
		gain = REG_CONFIG2_GAIN_16;
		break;
	case ADC_GAIN_32:
		gain = REG_CONFIG2_GAIN_32;
		break;
	case ADC_GAIN_64:
		gain = REG_CONFIG2_GAIN_64;
		break;
	default:
		return -EINVAL;
	}

	/* Inter-sample delay only configurable in scan mode.  One should use ΔΣ modulator
	 * oversampling ratio to control this anyway.
	 */
	if (cc->channel_id == 16 &&
	    cc->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		return -EINVAL;
	}

	if (cc->acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		data->delay = 0;
	} else if (ADC_ACQ_TIME_UNIT(cc->acquisition_time) == ADC_ACQ_TIME_TICKS) {
		unsigned int ticks = ADC_ACQ_TIME_VALUE(cc->acquisition_time);
		/* Only powers of 2 between 8 and 512 inclusive */
		if (ticks < 8 || ticks > 512 || (ticks & (ticks - 1))) {
			return -EINVAL;
		}
		/* Only save upper 8 bits, need to shift back when used in adc read setup */
		data->delay = REG_SCAN_DLY(ticks) >> 16;
		LOG_DBG("delay of tics %u into bits %06x", ticks, REG_SCAN_DLY(ticks));
	} else {
		return -EINVAL;
	}

	/* Set Gain */
	if (gain != data->gain) {
		reg_write(dev, REG_CONFIG2, config->boost | gain |
			  (data->az_mux ? REG_CONFIG2_AZ_MUX : 0) | REG_CONFIG2_RES);
		data->gain = gain;
	}

	if (cc->channel_id == 16) {
		if ((cc->input_positive < 8 && cc->input_positive >= config->channels) ||
		    (cc->input_negative < 8 && cc->input_negative >= config->channels) ||
		    cc->input_positive == 0xa || cc->input_negative == 0xa) {
			return -EINVAL;
		}

		/* Set both muxes */
		reg_write(dev, REG_MUX, cc->input_positive << 4 | cc->input_negative);
	}
	/* else, verify input settings are required values for channels 0-15? */

	return 0;
}

static int mcp356x_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	const struct mcp356x_config *config = dev->config;
	struct mcp356x_data *data = dev->data;
	int err;

	if (sequence->channels & BIT(16)) {
		if (sequence->channels != BIT(16)) {
			LOG_WRN("Channel 16 can't be sampled with additional channels");
			return -EINVAL;
		}
		if (IS_ENABLED(CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER) &&
		    sequence->options && sequence->options->interval_us) {
			LOG_WRN("Channel 16 does not support non-zero interval in internal timer "
				"mode");
			return -EINVAL;
		}
	} else {
		if (sequence->channels & ~data->ch_mask) {
			LOG_WRN("Invalid channel mask 0x%04x, allowed bits 0x%04x\n",
				sequence->channels, data->ch_mask);
			return -EINVAL;
		}
	}
#if IS_ENABLED(CONFIG_ADC_MCP356X_OSR_FIXED)
	if (sequence->oversampling) {
#else
	if (sequence->oversampling & ~REG_CONFIG1_OSR_MASK) {
#endif
		return -EINVAL;
	}

	/* Takes ctx->lock.  Need to call adc_context_release() to give it back on error */
	adc_context_lock(&data->ctx, !!async, async);

	/* Program scan and inter-sample delay.  Note that channel 16 appears as SCAN value 0,
	 * which is correct as it doesn't use scan mode.
	 */
	reg_write(dev, REG_SCAN, (sequence->channels == BIT(16) ? 0 : data->delay << 16) |
		  (sequence->channels & 0xffff));

	/* Check for change in resolution */
	if (data->resolution != sequence->resolution) {
		LOG_DBG("resolution change %d -> %d", data->resolution, sequence->resolution);
		err = get_bits_res(sequence->resolution);
		if (err < 0) {
			goto fail;
		}
		const uint8_t res_field = err;

		data->resolution = sequence->resolution;

		reg_write(dev, REG_CONFIG3, res_field |
			  (IS_ENABLED(CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER) ?
			  REG_CONFIG3_CONV_MODE_CONT : REG_CONFIG3_CONV_MODE_OS_STBY));
	}

	/* Check buffer length */
	const unsigned int nchannels = popcount(sequence->channels);
	const unsigned int samples = 1 + sequence->options->extra_samplings;

	if (sequence->buffer_size < (data->resolution / 8) * nchannels * samples) {
		err = -ENOMEM;
		goto fail;
	}

	/* Change in calibration aka auto-zero mux mode */
	if (sequence->calibrate != data->az_mux) {
		data->az_mux = sequence->calibrate;
		reg_write(dev, REG_CONFIG2, config->boost | data->gain |
			  (data->az_mux ? REG_CONFIG2_AZ_MUX : 0) | REG_CONFIG2_RES);
	}

#if !IS_ENABLED(CONFIG_ADC_MCP356X_OSR_FIXED)
	if (sequence->oversampling != data->osr) {
		data->osr = sequence->oversampling;
		reg_write(dev, REG_CONFIG1, config->pre << REG_CONFIG1_PRE_SHIFT |
					    data->osr << REG_CONFIG1_OSR_SHIFT);
	}
#endif /* CONFIG_ADC_MCP356X_OSR_FIXED */
	/* mclk/dmclk ratio, as a power of two: mclk >> dmclk_shift = dmclk */
	const uint32_t dmclk_shift = config->pre + 2;
	uint32_t t_odr = (OSR_TO_TODR(data->osr) * 1000000ULL << dmclk_shift) / config->mclk_freq;
	uint32_t t_conv = (OSR_TO_TCONV(data->osr) * 1000000ULL << dmclk_shift) / config->mclk_freq;

	LOG_DBG("MCLK/AMCLK %u/%u Hz, OSR %dx, Todr/Tconv %u/%u µs", config->mclk_freq,
		config->mclk_freq >> config->pre, OSR_TO_TODR(data->osr), t_odr, t_conv);

	if (sequence->options && sequence->options->interval_us) {
		const uint32_t timer = ((uint64_t)sequence->options->interval_us *
				       config->mclk_freq >> dmclk_shift) / 1000000;

		if (timer > BIT_MASK(24)) {
			LOG_WRN("Interval %u µs out of range with %u Hz DMCLK",
				sequence->options->interval_us, config->mclk_freq >> dmclk_shift);
			err = -EINVAL;
			goto fail;
		}
		LOG_DBG("Interval %u µs -> %u DMCLK", sequence->options->interval_us, timer);
		reg_write(dev, REG_TIMER, timer);
	} else {
		reg_write(dev, REG_TIMER, 0);
	}

	/* Save new buffer pointer */
	data->write_ptr = sequence->buffer;

	/* Insure any extra DRDY IRQs from the end of the last operation haven't already set the
	 * semaphore before the 1st conversion is finished.
	 */
	k_sem_reset(&data->drdy_sem);

	/* This saves the sequence and options into ctx, and either calls
	 * adc_context_start_sampling() if interval_us is 0 or starts a timer that will do so
	 * after interval_us elapses.
	 */
	adc_context_start_read(&data->ctx, sequence);

	/* Wait for ctx->sync if synchronous or just return 0 if async */
	err = adc_context_wait_for_completion(&data->ctx);

fail:
	/* Release ctx->lock if sync or any error, keep it if async and no error. */
	adc_context_release(&data->ctx, err);
	return err;
}

static int mcp356x_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	return mcp356x_read_async(dev, sequence, NULL);
}

static void drdy_callback(const struct device *port, struct gpio_callback *cb,
			  gpio_port_pins_t pins)
{
	struct mcp356x_data *data = CONTAINER_OF(cb, struct mcp356x_data, drdy_cb);

#if VERBOSE_LOG
	static uint32_t lastirq;
	uint32_t t = k_cycle_get_32();

	LOG_DBG("IRQ ΔT %u µs",
		(unsigned int)((t - lastirq) * 1000000ULL / sys_clock_hw_cycles_per_sec()));
	lastirq = t;
#endif

	k_sem_give(&data->drdy_sem);
}

static int mcp356x_init(const struct device *dev)
{
	const struct mcp356x_config *config = dev->config;
	struct mcp356x_data *data = dev->data;
	int err;

	LOG_INF("Initializing device %s", dev->name);

	if (!spi_is_ready(&config->bus) || !device_is_ready(config->irq.port)) {
		LOG_ERR("SPI bus or GPIO not ready");
		return -ENODEV;
	}

	k_sem_init(&data->acq_sem, 0, 1);
	k_sem_init(&data->drdy_sem, 0, 1);

	/* Allowed bits for SCAN */
	data->ch_mask = BIT_MASK(config->channels) | BIT_MASK(config->channels/2) << 8 |
			REG_SCAN_INT_CH_MASK;

	err = gpio_pin_configure_dt(&config->irq, GPIO_INPUT);
	if (err) {
		return err;
	}

	gpio_init_callback(&data->drdy_cb, drdy_callback, BIT(config->irq.pin));
	err = gpio_add_callback(config->irq.port, &data->drdy_cb);
	if (err) {
		return err;
	}

	uint8_t expstatus = (config->addr >> 5) | ((config->addr >> 6 & 1) ^ 1);
	int status = fast_write(dev, CMD_RESET);

	if (status < 0 || (status >> 3) != expstatus) {
		LOG_ERR("SPI write failed (%d) or unexpected status bits (%02x)",
			status < 0 ? status : 0, status >= 0 ? status : 0);
		return -ENODEV;
	}

	int devid = reg_read(dev, REG_DEVID);

	LOG_INF("Chip ID 0x%04x (expected 0x%04x)", devid, 0x000b + (config->channels/2));

	data->delay = 0;
	data->resolution = 32;
	data->osr = CONFIG_ADC_MCP356X_OSR;
	data->az_mux = false;
	data->gain = REG_CONFIG2_GAIN_1;

	/* Clock setting */
	uint8_t clk_sel = config->ext_clock ? REG_CONFIG0_CLK_SEL_EXT : REG_CONFIG0_CLK_SEL_INT;

	reg_write(dev, REG_LOCK, REG_LOCK_MAGIC);  /* unlock, in case it was locked */
	reg_write(dev, REG_CONFIG0, REG_CONFIG0_NO_SHUTDOWN | clk_sel | REG_CONFIG0_ADC_MODE_STBY);
	reg_write(dev, REG_CONFIG1,
		  config->pre << REG_CONFIG1_PRE_SHIFT | data->osr << REG_CONFIG1_OSR_SHIFT);
	reg_write(dev, REG_CONFIG3, REG_CONFIG3_DATA_FORMAT_32_RJ |
		  (IS_ENABLED(CONFIG_ADC_MCP356X_USE_INTERNAL_TIMER) ?
			  REG_CONFIG3_CONV_MODE_CONT : REG_CONFIG3_CONV_MODE_OS_STBY));
	/* Turn off STP interrupt, will get confused with DRDY interrupt! */
	reg_write(dev, REG_IRQ, (config->push_pull ? REG_IRQ_MODE_PP : REG_IRQ_MODE_HIGHZ) |
		  REG_IRQ_EN_FAST_CMD);

#if VERBOSE_LOG
	/* Dump registers to log in verbose debug mode */
	for (int i = 0; i <= 0xf; i++) {
		reg_read(dev, i);
	}
#endif

	k_thread_create(&data->thread, data->stack,
			CONFIG_ADC_MCP356X_ACQUISITION_THREAD_STACK_SIZE,
			(k_thread_entry_t)mcp356x_acquisition_thread,
			(void *)dev, NULL, NULL,
			CONFIG_ADC_MCP356X_ACQUISITION_THREAD_PRIO,
			0, K_NO_WAIT);
	/* Add instance number to thread name? */
	k_thread_name_set(&data->thread, "mcp356x");

	/* This will turn on the interrupt.  We must be ready to receive them. */
	err = gpio_pin_interrupt_configure_dt(&config->irq, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		return err;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api mcp356x_adc_api = {
	.channel_setup = mcp356x_channel_setup,
	.read = mcp356x_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcp356x_read_async,
#endif
};

/* Get clock frequency from node's clocks phandle, or return default value if there is no clocks
 * property.
 */
#define DT_CLOCKS_FREQ_OR(node_id, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, clocks), \
		(DT_PROP(DT_CLOCKS_CTLR(node_id), clock_frequency)), (default_value))

#ifdef ADC_CONTEXT_USES_KERNEL_TIMER
#define TIMER_INIT(inst, ctx)	ADC_CONTEXT_INIT_TIMER(inst, ctx),
#else
#define TIMER_INIT(inst, ctx)
#endif

#define MCP356X_DEVICE(inst, ch) \
	static struct mcp356x_data mcp356x_data_##inst = {				\
		ADC_CONTEXT_INIT_LOCK(mcp356x_data_##inst, ctx),			\
		ADC_CONTEXT_INIT_SYNC(mcp356x_data_##inst, ctx),			\
		TIMER_INIT(mcp356x_data_##inst, ctx)					\
	}; \
	static const struct mcp356x_config mcp356x_config_##inst = {			\
		.bus = SPI_DT_SPEC_GET(inst,						\
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),	\
		.irq = GPIO_DT_SPEC_GET(inst, irq_gpios),				\
		.channels = ch,								\
		.addr = DT_PROP_OR(inst, address, 1) << 6,				\
		.ext_clock = DT_NODE_HAS_PROP(inst, clocks),				\
		.mclk_freq = DT_CLOCKS_FREQ_OR(inst, 4915200),				\
		.boost = DT_ENUM_IDX_OR(inst, boost, 2) << REG_CONFIG2_BOOST_SHIFT,	\
		.pre = DT_ENUM_IDX_OR(inst, amclk_div, 0),				\
		.push_pull = DT_PROP(inst, drive_push_pull),				\
	}; \
	DEVICE_DT_DEFINE(inst, &mcp356x_init, NULL,					\
			 &mcp356x_data_##inst, &mcp356x_config_##inst,			\
			 POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &mcp356x_adc_api)

DT_FOREACH_STATUS_OKAY_VARGS(microchip_mcp3561, MCP356X_DEVICE, 2);
DT_FOREACH_STATUS_OKAY_VARGS(microchip_mcp3562, MCP356X_DEVICE, 4);
DT_FOREACH_STATUS_OKAY_VARGS(microchip_mcp3564, MCP356X_DEVICE, 8);
