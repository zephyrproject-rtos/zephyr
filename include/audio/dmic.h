/*
 * Copyright (c) 2018, Intel Corporation
 *
 * Author:	Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>
 *		Sathish Kuttan <sathish.k.kuttan@intel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API header file for Digital Microphones
 *
 * This file contains the Digital Microphone APIs
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_DMIC_H_
#define ZEPHYR_INCLUDE_AUDIO_DMIC_H_


/**
 * @defgroup audio_interface Audio
 * @{
 * @}
 */

/**
 * @brief Abstraction for digital microphones
 *
 * @defgroup audio_dmic_interface Digital Microphone Interface
 * @ingroup audio_interface
 * @{
 */

#include <kernel.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DMIC driver states
 */
enum dmic_state {
	DMIC_STATE_UNINIT,	/* Uninitialized */
	DMIC_STATE_INITIALIZED,	/* Initialized */
	DMIC_STATE_CONFIGURED,	/* Configured */
	DMIC_STATE_ACTIVE,	/* Active */
	DMIC_STATE_PAUSED,	/* Paused */
};

/**
 * DMIC driver trigger commands
 */
enum dmic_trigger {
	DMIC_TRIGGER_STOP,	/* stop stream */
	DMIC_TRIGGER_START,	/* start stream */
	DMIC_TRIGGER_PAUSE,	/* pause the stream */
	DMIC_TRIGGER_RELEASE,	/* release paused stream */
	DMIC_TRIGGER_RESET,	/* reset */
};

/**
 * PDM Channels LEFT / RIGHT
 */
enum pdm_lr {
	PDM_CHAN_LEFT,
	PDM_CHAN_RIGHT,
};

/**
 * PDM Input/Output signal configuration
 */
struct pdm_io_cfg {
	/* parameters global to all PDM controllers */

	/* minimum clock frequency supported by the mic */
	uint32_t	min_pdm_clk_freq;
	/* maximum clock frequency supported by the mic */
	uint32_t	max_pdm_clk_freq;
	/* minimum duty cycle in % supported by the mic */
	uint8_t	min_pdm_clk_dc;
	/* maximum duty cycle in % supported by the mic */
	uint8_t	max_pdm_clk_dc;

	/* parameters unique to each PDM controller */

	/* Bit mask to optionally invert PDM clock */
	uint8_t	pdm_clk_pol;
	/* Bit mask to optionally invert mic data */
	uint8_t	pdm_data_pol;
	/* Collection of clock skew values for each PDM port */
	uint32_t	pdm_clk_skew;
};

/**
 * Configuration of the PCM streams to be output by the PDM hardware
 */
struct pcm_stream_cfg {
	/*
	 * if either rate or width is set to 0 for a stream,
	 * the stream would be disabled
	 */

	/* PCM sample rate of stream */
	uint32_t			pcm_rate;
	/* PCM sample width of stream */
	uint8_t			pcm_width;
	/* PCM sample block size per transfer */
	uint16_t			block_size;
	/* SLAB for DMIC driver to allocate buffers for stream */
	struct k_mem_slab	*mem_slab;
};

/**
 * Mapping/ordering of the PDM channels to logical PCM output channel
 */
struct pdm_chan_cfg {
	/*
	 * mapping of PDM controller and mic channel to logical channel
	 * since each controller can have 2 audio channels (stereo),
	 * there can be total of 8x2=16 channels.
	 * The actual number of channels shall be described in
	 * pcm_stream_cfg.num_chan.
	 * if 2 streams are enabled, the channel order will be the same for
	 * both streams
	 * Each channel is described as a 4 bit number, the least significant
	 * bit indicates LEFT/RIGHT selection of the PDM controller.
	 * The most significant 3 bits indicate the PDM controller number.
	 *	bits 0-3 are for channel 0, bit 0 indicates LEFT or RIGHT
	 *	bits 4-7 are for channel 1, bit 4 indicates LEFT or RIGHT
	 *	and so on.
	 * CONSTRAINT: The LEFT and RIGHT channels of EACH PDM controller needs
	 * to be adjacent to each other.
	 */
	/* Requested channel map */
	uint32_t	req_chan_map_lo;	/* Channels 0 to 7 */
	uint32_t	req_chan_map_hi;	/* Channels 8 to 15 */
	/* Actual channel map that the driver could configure */
	uint32_t	act_chan_map_lo;	/* Channels 0 to 7 */
	uint32_t	act_chan_map_hi;	/* Channels 8 to 15 */
	/* requested number of channels */
	uint8_t	req_num_chan;
	/* Actual number of channels that the driver could configure */
	uint8_t	act_num_chan;
	/* requested number of streams for each channel */
	uint8_t	req_num_streams;
	/* Actual number of streams that the driver could configure */
	uint8_t	act_num_streams;
};

/**
 * Input configuration structure for the DMIC configuration API
 */
struct dmic_cfg {
	struct pdm_io_cfg io;
	/*
	 * Array of pcm_stream_cfg for application to provide
	 * configuration for each stream
	 */
	struct pcm_stream_cfg *streams;
	struct pdm_chan_cfg channel;
};

/**
 * Function pointers for the DMIC driver operations
 */
struct _dmic_ops {
	int (*configure)(struct device *dev, struct dmic_cfg *config);
	int (*trigger)(struct device *dev, enum dmic_trigger cmd);
	int (*read)(struct device *dev, uint8_t stream, void **buffer,
			size_t *size, int32_t timeout);
};

/**
 * Build the channel map to populate struct pdm_chan_cfg
 *
 * Returns the map of PDM controller and LEFT/RIGHT channel shifted to
 * the bit position corresponding to the input logical channel value
 *
 * @param channel The logical channel number
 * @param pdm The PDM hardware controller number
 * @param lr LEFT/RIGHT channel within the chosen PDM hardware controller
 *
 * @return Bit-map containing the PDM and L/R channel information
 */
static inline uint32_t dmic_build_channel_map(uint8_t channel, uint8_t pdm,
		enum pdm_lr lr)
{
	return ((((pdm & BIT_MASK(3)) << 1) | lr) <<
			((channel & BIT_MASK(3)) * 4U));
}

/**
 * Helper function to parse the channel map in pdm_chan_cfg
 *
 * Returns the PDM controller and LEFT/RIGHT channel corresponding to
 * the channel map and the logical channel provided as input
 *
 * @param channel_map_lo Lower order/significant bits of the channel map
 * @param channel_map_hi Higher order/significant bits of the channel map
 * @param channel The logical channel number
 * @param pdm Pointer to the PDM hardware controller number
 * @param lr Pointer to the LEFT/RIGHT channel within the PDM controller
 *
 * @return none
 */
static inline void dmic_parse_channel_map(uint32_t channel_map_lo,
		uint32_t channel_map_hi, uint8_t channel, uint8_t *pdm, enum pdm_lr *lr)
{
	uint32_t channel_map;

	channel_map = (channel < 8) ? channel_map_lo : channel_map_hi;
	channel_map >>= ((channel & BIT_MASK(3)) * 4U);

	*pdm = (channel >> 1) & BIT_MASK(3);
	*lr = channel & BIT(0);
}

/**
 * Build a bit map of clock skew values for each PDM channel
 *
 * Returns the bit-map of clock skew value shifted to the bit position
 * corresponding to the input PDM controller value
 *
 * @param pdm The PDM hardware controller number
 * @param skew The skew to apply for the clock output from the PDM controller
 *
 * @return Bit-map containing the clock skew information
 */
static inline uint32_t dmic_build_clk_skew_map(uint8_t pdm, uint8_t skew)
{
	return ((skew & BIT_MASK(4)) << ((pdm & BIT_MASK(3)) * 4U));
}

/**
 * Configure the DMIC driver and controller(s)
 *
 * Configures the DMIC driver device according to the number of channels,
 * channel mapping, PDM I/O configuration, PCM stream configuration, etc.
 *
 * @param dev Pointer to the device structure for DMIC driver instance
 * @param cfg Pointer to the structure containing the DMIC configuration
 *
 * @return 0 on success, a negative error code on failure
 */
static inline int dmic_configure(struct device *dev, struct dmic_cfg *cfg)
{
	const struct _dmic_ops *api =
		(const struct _dmic_ops *)dev->api;

	return api->configure(dev, cfg);
}

/**
 * Send a command to the DMIC driver
 *
 * Sends a command to the driver to perform a specific action
 *
 * @param dev Pointer to the device structure for DMIC driver instance
 * @param cmd The command to be sent to the driver instance
 *
 * @return 0 on success, a negative error code on failure
 */
static inline int dmic_trigger(struct device *dev, enum dmic_trigger cmd)
{
	const struct _dmic_ops *api =
		(const struct _dmic_ops *)dev->api;

	return api->trigger(dev, cmd);
}

/**
 * Read received decimated PCM data stream
 *
 * Optionally waits for audio to be received and provides the received
 * audio buffer from the requested stream
 *
 * @param dev Pointer to the device structure for DMIC driver instance
 * @param stream Stream identifier
 * @param buffer Pointer to the received buffer address
 * @param size Pointer to the received buffer size
 * @param timeout Timeout in milliseconds to wait in case audio is not yet
 * 		  received, or @ref SYS_FOREVER_MS
 *
 * @return 0 on success, a negative error code on failure
 */
static inline int dmic_read(struct device *dev, uint8_t stream, void **buffer,
		size_t *size, int32_t timeout)
{
	const struct _dmic_ops *api =
		(const struct _dmic_ops *)dev->api;

	return api->read(dev, stream, buffer, size, timeout);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_AUDIO_DMIC_H_ */
