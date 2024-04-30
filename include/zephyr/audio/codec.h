/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API header file for Audio Codec
 *
 * This file contains the Audio Codec APIs
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_CODEC_H_
#define ZEPHYR_INCLUDE_AUDIO_CODEC_H_

/**
 * @brief Abstraction for audio codecs
 *
 * @defgroup audio_codec_interface Audio Codec Interface
 * @since 1.13
 * @version 0.1.0
 * @ingroup audio_interface
 * @{
 */

#include <zephyr/drivers/i2s.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * PCM audio sample rates
 */
typedef enum {
	AUDIO_PCM_RATE_8K	= 8000,		/**< 8 kHz sample rate */
	AUDIO_PCM_RATE_16K	= 16000,	/**< 16 kHz sample rate */
	AUDIO_PCM_RATE_24K	= 24000,	/**< 24 kHz sample rate */
	AUDIO_PCM_RATE_32K	= 32000,	/**< 32 kHz sample rate */
	AUDIO_PCM_RATE_44P1K	= 44100,	/**< 44.1 kHz sample rate */
	AUDIO_PCM_RATE_48K	= 48000,	/**< 48 kHz sample rate */
	AUDIO_PCM_RATE_96K	= 96000,	/**< 96 kHz sample rate */
	AUDIO_PCM_RATE_192K	= 192000,	/**< 192 kHz sample rate */
} audio_pcm_rate_t;

/**
 * PCM audio sample bit widths
 */
typedef enum {
	AUDIO_PCM_WIDTH_16_BITS	= 16,	/**< 16-bit sample width */
	AUDIO_PCM_WIDTH_20_BITS	= 20,	/**< 20-bit sample width */
	AUDIO_PCM_WIDTH_24_BITS	= 24,	/**< 24-bit sample width */
	AUDIO_PCM_WIDTH_32_BITS	= 32,	/**< 32-bit sample width */
} audio_pcm_width_t;

/**
 * Digital Audio Interface (DAI) type
 */
typedef enum {
	AUDIO_DAI_TYPE_I2S,	/**< I2S Interface */
	AUDIO_DAI_TYPE_INVALID,	/**< Other interfaces can be added here */
} audio_dai_type_t;

/**
 * Codec properties that can be set by audio_codec_set_property().
 */
typedef enum {
	AUDIO_PROPERTY_OUTPUT_VOLUME,	/**< Output volume */
	AUDIO_PROPERTY_OUTPUT_MUTE,	/**< Output mute/unmute */
} audio_property_t;

/**
 * Audio channel identifiers to use in audio_codec_set_property().
 */
typedef enum {
	AUDIO_CHANNEL_FRONT_LEFT,	/**< Front left channel */
	AUDIO_CHANNEL_FRONT_RIGHT,	/**< Front right channel */
	AUDIO_CHANNEL_LFE,		/**< Low frequency effect channel */
	AUDIO_CHANNEL_FRONT_CENTER,	/**< Front center channel */
	AUDIO_CHANNEL_REAR_LEFT,	/**< Rear left channel */
	AUDIO_CHANNEL_REAR_RIGHT,	/**< Rear right channel */
	AUDIO_CHANNEL_REAR_CENTER,	/**< Rear center channel */
	AUDIO_CHANNEL_SIDE_LEFT,	/**< Side left channel */
	AUDIO_CHANNEL_SIDE_RIGHT,	/**< Side right channel */
	AUDIO_CHANNEL_ALL,		/**< All channels */
} audio_channel_t;

/**
 * @brief Digital Audio Interface Configuration.
 *
 * Configuration is dependent on DAI type
 */
typedef union {
	struct i2s_config i2s;	/**< I2S configuration */
				/* Other DAI types go here */
} audio_dai_cfg_t;

/**
 * Codec configuration parameters
 */
struct audio_codec_cfg {
	uint32_t		mclk_freq;	/**< MCLK input frequency in Hz */
	audio_dai_type_t	dai_type;	/**< Digital interface type */
	audio_dai_cfg_t		dai_cfg;	/**< DAI configuration info */
};

/**
 * Codec property values
 */
typedef union {
	int	vol;	/**< Volume level in 0.5dB resolution */
	bool	mute;	/**< Mute if @a true, unmute if @a false */
} audio_property_value_t;

/**
 * @brief Codec error type
 */
enum audio_codec_error_type {
	/** Output over-current */
	AUDIO_CODEC_ERROR_OVERCURRENT = BIT(0),

	/** Codec over-temperature */
	AUDIO_CODEC_ERROR_OVERTEMPERATURE = BIT(1),

	/** Power low voltage */
	AUDIO_CODEC_ERROR_UNDERVOLTAGE = BIT(2),

	/** Power high voltage */
	AUDIO_CODEC_ERROR_OVERVOLTAGE = BIT(3),

	/** Output direct-current */
	AUDIO_CODEC_ERROR_DC = BIT(4),
};

/**
 * @typedef audio_codec_error_callback_t
 * @brief Callback for error interrupt
 *
 * @param dev Pointer to the codec device
 * @param errors Device errors (bitmask of @ref audio_codec_error_type values)
 */
typedef void (*audio_codec_error_callback_t)(const struct device *dev, uint32_t errors);

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */
struct audio_codec_api {
	int (*configure)(const struct device *dev,
			 struct audio_codec_cfg *cfg);
	void (*start_output)(const struct device *dev);
	void (*stop_output)(const struct device *dev);
	int (*set_property)(const struct device *dev,
			    audio_property_t property,
			    audio_channel_t channel,
			    audio_property_value_t val);
	int (*apply_properties)(const struct device *dev);
	int (*clear_errors)(const struct device *dev);
	int (*register_error_callback)(const struct device *dev,
			 audio_codec_error_callback_t cb);
};
/**
 * @endcond
 */

/**
 * @brief Configure the audio codec
 *
 * Configure the audio codec device according to the configuration
 * parameters provided as input
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param cfg Pointer to the structure containing the codec configuration.
 *
 * @return 0 on success, negative error code on failure
 */
static inline int audio_codec_configure(const struct device *dev,
					struct audio_codec_cfg *cfg)
{
	const struct audio_codec_api *api =
		(const struct audio_codec_api *)dev->api;

	return api->configure(dev, cfg);
}

/**
 * @brief Set codec to start output audio playback
 *
 * Setup the audio codec device to start the audio playback
 *
 * @param dev Pointer to the device structure for codec driver instance.
 */
static inline void audio_codec_start_output(const struct device *dev)
{
	const struct audio_codec_api *api =
		(const struct audio_codec_api *)dev->api;

	api->start_output(dev);
}

/**
 * @brief Set codec to stop output audio playback
 *
 * Setup the audio codec device to stop the audio playback
 *
 * @param dev Pointer to the device structure for codec driver instance.
 */
static inline void audio_codec_stop_output(const struct device *dev)
{
	const struct audio_codec_api *api =
		(const struct audio_codec_api *)dev->api;

	api->stop_output(dev);
}

/**
 * @brief Set a codec property defined by audio_property_t
 *
 * Set a property such as volume level, clock configuration etc.
 *
 * @param dev Pointer to the device structure for codec driver instance.
 * @param property The codec property to set
 * @param channel The audio channel for which the property has to be set
 * @param val pointer to a property value of type audio_codec_property_value_t
 *
 * @return 0 on success, negative error code on failure
 */
static inline int audio_codec_set_property(const struct device *dev,
					   audio_property_t property,
					   audio_channel_t channel,
					   audio_property_value_t val)
{
	const struct audio_codec_api *api =
		(const struct audio_codec_api *)dev->api;

	return api->set_property(dev, property, channel, val);
}

/**
 * @brief Atomically apply any cached properties
 *
 * Following one or more invocations of audio_codec_set_property, that may have
 * been cached by the driver, audio_codec_apply_properties can be invoked to
 * apply all the properties as atomic as possible
 *
 * @param dev Pointer to the device structure for codec driver instance.
 *
 * @return 0 on success, negative error code on failure
 */
static inline int audio_codec_apply_properties(const struct device *dev)
{
	const struct audio_codec_api *api =
		(const struct audio_codec_api *)dev->api;

	return api->apply_properties(dev);
}

/**
 * @brief Clear any codec errors
 *
 * Clear all codec errors.
 * If an error interrupt exists, it will be de-asserted.
 *
 * @param dev Pointer to the device structure for codec driver instance.
 *
 * @return 0 on success, negative error code on failure
 */
static inline int audio_codec_clear_errors(const struct device *dev)
{
	const struct audio_codec_api *api =
		(const struct audio_codec_api *)dev->api;

	if (api->clear_errors == NULL) {
		return -ENOSYS;
	}

	return api->clear_errors(dev);
}

/**
 * @brief Register a callback function for codec error
 *
 * The callback will be called from a thread, so I2C or SPI operations are
 * safe.  However, the thread's stack is limited and defined by the
 * driver.  It is currently up to the caller to ensure that the callback
 * does not overflow the stack.
 *
 * @param dev Pointer to the audio codec device
 * @param cb The function that should be called when an error is detected
 * fires
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int audio_codec_register_error_callback(const struct device *dev,
				     audio_codec_error_callback_t cb)
{
	const struct audio_codec_api *api =
		(const struct audio_codec_api *)dev->api;

	if (api->register_error_callback == NULL) {
		return -ENOSYS;
	}

	return api->register_error_callback(dev, cb);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_AUDIO_CODEC_H_ */
