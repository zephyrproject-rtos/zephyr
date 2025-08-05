/*
 * Copyright (c) 2025, Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API header file for Analog Microphones
 *
 * This file contains the Analog Microphone APIs
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_AMIC_H_
#define ZEPHYR_INCLUDE_AUDIO_AMIC_H_


/**
 * @defgroup audio_interface Audio
 * @{
 * @}
 */

/**
 * @brief Abstraction for Analog microphones
 *
 * @defgroup audio_amic_interface Analog Microphone Interface
 * @ingroup audio_interface
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AMIC driver states
 */
enum amic_state {
	AMIC_STATE_UNINIT,	/**< Uninitialized */
	AMIC_STATE_INITIALIZED,	/**< Initialized */
	AMIC_STATE_CONFIGURED,	/**< Configured */
	AMIC_STATE_ACTIVE,	/**< Active */
	AMIC_STATE_PAUSED,	/**< Paused */
	AMIC_STATE_ERROR,	/**< Error */
};

/**
 * AMIC driver trigger commands
 */
enum amic_trigger {
	AMIC_TRIGGER_STOP,	/**< Stop stream */
	AMIC_TRIGGER_START,	/**< Start stream */
	AMIC_TRIGGER_PAUSE,	/**< Pause stream */
	AMIC_TRIGGER_RELEASE,	/**< Release paused stream */
	AMIC_TRIGGER_RESET,	/**< Reset stream */
};

/**
 * Configuration of the PCM streams to be output by the AUDADC hardware
 */
struct pcm_stream_cfg {
	/** PCM sample rate of stream */
	uint32_t		pcm_rate;
	/** PCM sample width of stream */
	uint8_t			pcm_width;
	uint8_t			channel_num;
	/** PCM sample block size per transfer */
	uint16_t		block_size;
	/** SLAB for AMIC driver to allocate buffers for stream */
	struct k_mem_slab	*mem_slab;
};

/**
 * Input configuration structure for the AMIC configuration API
 */
struct amic_cfg {
	/**
	 * Array of pcm_stream_cfg for application to provide
	 * configuration for each stream
	 */
	struct pcm_stream_cfg *streams;
};

/**
 * Function pointers for the AMIC driver operations
 */
struct _amic_ops {
	int (*configure)(const struct device *dev, struct amic_cfg *config);
	int (*trigger)(const struct device *dev, enum amic_trigger cmd);
	int (*read)(const struct device *dev, uint8_t stream, void **buffer,
			size_t *size, int32_t timeout);
};

/**
 * Configure the AMIC driver and controller(s)
 *
 * Configures the AMIC driver device according to the number of channels,
 * channel mapping, AUDADC I/O configuration, PCM stream configuration, etc.
 *
 * @param dev Pointer to the device structure for AMIC driver instance
 * @param cfg Pointer to the structure containing the AMIC configuration
 *
 * @return 0 on success, a negative error code on failure
 */
static inline int amic_configure(const struct device *dev,
				 struct amic_cfg *cfg)
{
	const struct _amic_ops *api =
		(const struct _amic_ops *)dev->api;

	return api->configure(dev, cfg);
}

/**
 * Send a command to the AMIC driver
 *
 * Sends a command to the driver to perform a specific action
 *
 * @param dev Pointer to the device structure for AMIC driver instance
 * @param cmd The command to be sent to the driver instance
 *
 * @return 0 on success, a negative error code on failure
 */
static inline int amic_trigger(const struct device *dev,
			       enum amic_trigger cmd)
{
	const struct _amic_ops *api =
		(const struct _amic_ops *)dev->api;

	return api->trigger(dev, cmd);
}

/**
 * Read received decimated PCM data stream
 *
 * Optionally waits for audio to be received and provides the received
 * audio buffer from the requested stream
 *
 * @param dev Pointer to the device structure for AMIC driver instance
 * @param stream Stream identifier
 * @param buffer Pointer to the received buffer address
 * @param size Pointer to the received buffer size
 * @param timeout Timeout in milliseconds to wait in case audio is not yet
 *		received, or @ref SYS_FOREVER_MS
 *
 * @return 0 on success, a negative error code on failure
 */
static inline int amic_read(const struct device *dev, uint8_t stream,
			    void **buffer,
			    size_t *size, int32_t timeout)
{
	const struct _amic_ops *api =
		(const struct _amic_ops *)dev->api;

	return api->read(dev, stream, buffer, size, timeout);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_AUDIO_AMIC_H_ */
