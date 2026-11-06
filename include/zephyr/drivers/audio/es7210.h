/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ES7210 microphone ADC configuration and processing controls.
 * @ingroup es7210_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES7210_H_
#define ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES7210_H_

#include <zephyr/audio/codec.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup es7210_interface ES7210 microphone ADC
 * @ingroup audio_codec_interface_ext
 * @brief ES7210 serial interface and microphone processing controls.
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/** ADC channel indices before serial packing; pair routing may change their sources. */
enum audio_codec_es7210_channel {
	/** MIC1 / ADC1 */
	AUDIO_CODEC_ES7210_MIC1 = AUDIO_CHANNEL_PRIV_START,
	/** MIC2 / ADC2 */
	AUDIO_CODEC_ES7210_MIC2,
	/** MIC3 / ADC3 */
	AUDIO_CODEC_ES7210_MIC3,
	/** MIC4 / ADC4 */
	AUDIO_CODEC_ES7210_MIC4,
};

/** Serial output protocol. Data changes on falling BCLK edges unless inverted. */
enum audio_codec_es7210_format {
	/** One-bit delay, LRCK low for the first channel group. */
	AUDIO_CODEC_ES7210_I2S,
	/** No delay, LRCK high for the first group. */
	AUDIO_CODEC_ES7210_LEFT_JUSTIFIED,
	/** One-bit delay after the rising frame-sync edge. */
	AUDIO_CODEC_ES7210_DSP_A,
	/** No delay after the rising frame-sync edge. */
	AUDIO_CODEC_ES7210_DSP_B,
};

/** Serial configuration for a single ES7210, including four-channel TDM. */
struct audio_codec_es7210_dai_cfg {
	/** External MCLK frequency in Hz. */
	uint32_t mclk_freq;
	/** Per-channel sample rate in Hz. */
	uint32_t sample_rate;
	/** Wire protocol. */
	enum audio_codec_es7210_format format;
	/** Valid word bits: 16, 18, 20, 24 or 32. */
	uint8_t word_size;
	/** BCLKs per slot, word_size through 32; TDM requires word_size. */
	uint8_t slot_width;
	/** Two for stereo, four for TDM on SDOUT1. */
	uint8_t channels;
	/** Generate BCLK/LRCK from external MCLK. */
	bool clock_controller;
	/** Invert BCLK polarity. */
	bool invert_bclk;
	/** Invert LRCK polarity; available for I2S/left justified only. */
	bool invert_lrck;
};

/** Routing within one converter pair; applies before serial channel ordering. */
enum audio_codec_es7210_route {
	/** First/second converter to left/right. */
	AUDIO_CODEC_ES7210_ROUTE_NORMAL,
	/** First converter to both channels. */
	AUDIO_CODEC_ES7210_ROUTE_LEFT,
	/** Second converter to both channels. */
	AUDIO_CODEC_ES7210_ROUTE_RIGHT,
	/** Exchange left and right. */
	AUDIO_CODEC_ES7210_ROUTE_SWAP,
};

/** Per-input PGA gain and per-channel digital controls. Gains use 0.5 dB units. */
struct audio_codec_es7210_channel_cfg {
	/** Analog gain in 0.5 dB units: 0..66 in steps of 6, then 69, 72 or 75. */
	int16_t pga_gain;
	/** Digital gain in 0.5 dB units: -191..64. Direct gain or ALC maximum, after routing. */
	int16_t digital_gain;
	/** Invert digital channel polarity after pair routing. */
	bool invert;
};

/** Processing shared by ADC1/2 or ADC3/4. */
struct audio_codec_es7210_pair_cfg {
	/** Pair routing. */
	enum audio_codec_es7210_route route;
	/** Select digital inputs instead of analog microphones. */
	bool digital_mic;
	/** Digital microphone gain in dB, 0..42 in steps of 6. */
	uint8_t digital_mic_gain;
	/** Five-bit HPF coefficient; 6 fast, 13 slow. */
	uint8_t hpf_coefficient1;
	/** Five-bit HPF coefficient; 6 fast, 13 slow. */
	uint8_t hpf_coefficient2;
	/** Freeze HPF offset instead of automatically tracking it. */
	bool freeze_offset;
	/** Pair mask: bit 0 first, bit 1 second; zero disables ALC. */
	uint8_t alc_channels;
	/** Lower target level in 0.1 dBFS; see driver documentation. */
	int16_t alc_min_level;
	/** Upper target level in 0.1 dBFS; see driver documentation. */
	int16_t alc_max_level;
};

/** Complete processing settings; obtain defaults with audio_codec_es7210_get_processing(). */
struct audio_codec_es7210_processing {
	/** Enabled physical converters, bits 0..3 for ADC1..4. */
	uint8_t mic_mask;
	/** ADC1..4 settings. */
	struct audio_codec_es7210_channel_cfg channel[4];
	/** ADC1/2 followed by ADC3/4. */
	struct audio_codec_es7210_pair_cfg pair[2];
	/** LRCK cycles per 0.25 dB step: powers of two from 2..65536. */
	uint32_t alc_ramp_cycles;
	/** LRCK cycles per 0.5 dB: zero or powers of two, 4..256. */
	uint16_t gain_ramp_cycles;
};

/** Hardware identification and state-machine status. */
struct audio_codec_es7210_status {
	/** Chip revision register. */
	uint8_t revision;
	/** State machine: 0 powered down, 1 initializing, 2 normal, 3 powering up. */
	uint8_t state;
	/** Software configuration is valid. */
	bool configured;
	/** Capture has been started. */
	bool running;
};

/**
 * @brief Configure the ES7210 serial output without configuring the host controller.
 *
 * Four-channel I2S/left-justified slot order is ADC1, ADC3, ADC2, ADC4.
 * Four-channel DSP slot order is ADC1, ADC2, ADC3, ADC4. Disabled converters retain
 * their slots. The receiver must support the requested protocol and frame geometry.
 * Successful configuration resets gain, routing, filter and mute settings.
 *
 * @param dev ES7210 device.
 * @param cfg Serial configuration.
 * @retval 0 Configuration completed.
 * @retval -EINVAL Invalid argument or device.
 * @retval -ENOTSUP Unsupported format, clocks or frame geometry.
 * @retval -EBUSY Capture is running.
 * @return Negative I2C error code on a failed transaction.
 */
int audio_codec_es7210_configure(const struct device *dev,
				 const struct audio_codec_es7210_dai_cfg *cfg);

/**
 * @brief Read the current processing settings, including generic gain changes.
 * @param dev ES7210 device.
 * @param cfg Destination for the settings.
 * @retval 0 Settings copied.
 * @retval -EINVAL Invalid argument or device.
 * @retval -EIO Configuration is invalid.
 */
int audio_codec_es7210_get_processing(const struct device *dev,
				      struct audio_codec_es7210_processing *cfg);

/**
 * @brief Apply microphone processing settings while capture is stopped.
 *
 * Settings survive stop/start. An I2C error invalidates the configuration and
 * attempts mute/shutdown. The mic_mask must select a nonempty subset of the
 * converters carried by the configured output. Unselected input and output
 * channels are disabled. Digital gain, mute and polarity apply after routing, before
 * serial packing.
 *
 * @param dev ES7210 device.
 * @param cfg Complete processing configuration.
 * @retval 0 Settings applied.
 * @retval -EINVAL Invalid argument, device or setting.
 * @retval -EBUSY Capture is running.
 * @retval -EIO Configuration is invalid.
 * @return Negative I2C error code on a failed transaction.
 */
int audio_codec_es7210_set_processing(const struct device *dev,
				      const struct audio_codec_es7210_processing *cfg);

/**
 * @brief Read chip revision and state-machine status.
 * @param dev ES7210 device.
 * @param status Destination for status.
 * @retval 0 Status read.
 * @retval -EINVAL Invalid argument or device.
 * @return Negative I2C error code on a failed transaction.
 */
int audio_codec_es7210_get_status(const struct device *dev,
				  struct audio_codec_es7210_status *status);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES7210_H_ */
