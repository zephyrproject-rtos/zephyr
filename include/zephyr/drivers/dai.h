/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the DAI (Digital Audio Interface) bus drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAI_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAI_H_

/**
 * @defgroup dai_interface DAI Interface
 * @ingroup io_interfaces
 * @brief DAI Interface
 *
 * The DAI API provides support for the standard I2S (SSP) and its common variants.
 * It supports also DMIC, HDA and SDW backends. The API has a config function
 * with bespoke data argument for device/vendor specific config. There are also
 * optional timestamping functions to get device specific audio clock time.
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Types of DAI
 *
 * The type of the DAI. This ID type is used to configure bespoke DAI HW
 * settings.
 *
 * DAIs have a lot of physical link feature variability and therefore need
 * different configuration data to cater for different use cases. We
 * usually need to pass extra bespoke configuration prior to DAI start.
 */
enum dai_type {
	DAI_LEGACY_I2S = 0,	/**< Legacy I2S compatible with i2s.h */
	DAI_INTEL_SSP,		/**< Intel SSP */
	DAI_INTEL_DMIC,		/**< Intel DMIC */
	DAI_INTEL_HDA,		/**< Intel HD/A */
	DAI_INTEL_ALH,		/**< Intel ALH */
	DAI_IMX_SAI,		/**< i.MX SAI */
	DAI_IMX_ESAI,		/**< i.MX ESAI */
	DAI_AMD_BT,		/**< Amd BT */
	DAI_AMD_SP,		/**< Amd SP */
	DAI_AMD_DMIC,		/**< Amd DMIC */
	DAI_MEDIATEK_AFE,	/**< Mtk AFE */
	DAI_INTEL_SSP_NHLT,	/**< nhlt ssp */
	DAI_INTEL_DMIC_NHLT,	/**< nhlt ssp */
	DAI_INTEL_HDA_NHLT,	/**< nhlt Intel HD/A */
	DAI_INTEL_ALH_NHLT,	/**< nhlt Intel ALH */
};

/**
 * @brief Dai Direction
 */
enum dai_dir {
	/** Receive data */
	DAI_DIR_RX = 1,
	/** Transmit data */
	DAI_DIR_TX,
	/** Both receive and transmit data */
	DAI_DIR_BOTH,
};

/** Interface state */
enum dai_state {
	/** @brief The interface is not ready.
	 *
	 * The interface was initialized but is not yet ready to receive /
	 * transmit data. Call dai_config_set() to configure interface and change
	 * its state to READY.
	 */
	DAI_STATE_NOT_READY = 0,
	/** The interface is ready to receive / transmit data. */
	DAI_STATE_READY,
	/** The interface is receiving / transmitting data. */
	DAI_STATE_RUNNING,
	/** The interface is clocking but not receiving / transmitting data. */
	DAI_STATE_PRE_RUNNING,
	/** The interface paused */
	DAI_STATE_PAUSED,
	/** The interface is draining its transmit queue. */
	DAI_STATE_STOPPING,
	/** TX buffer underrun or RX buffer overrun has occurred. */
	DAI_STATE_ERROR,
};

/** Trigger command */
enum dai_trigger_cmd {
	/** @brief Start the transmission / reception of data.
	 *
	 * If DAI_DIR_TX is set some data has to be queued for transmission by
	 * the dai_write() function. This trigger can be used in READY state
	 * only and changes the interface state to RUNNING.
	 */
	DAI_TRIGGER_START = 0,
	/** @brief Optional - Pre Start the transmission / reception of data.
	 *
	 * Allows the DAI and downstream codecs to prepare for audio Tx/Rx by
	 * starting any required clocks for downstream PLL/FLL locking.
	 */
	DAI_TRIGGER_PRE_START,
	/** @brief Stop the transmission / reception of data.
	 *
	 * Stop the transmission / reception of data at the end of the current
	 * memory block. This trigger can be used in RUNNING state only and at
	 * first changes the interface state to STOPPING. When the current TX /
	 * RX block is transmitted / received the state is changed to READY.
	 * Subsequent START trigger will resume transmission / reception where
	 * it stopped.
	 */
	DAI_TRIGGER_STOP,
	/** @brief Pause the transmission / reception of data.
	 *
	 * Pause the transmission / reception of data at the end of the current
	 * memory block. Behavior is implementation specific but usually this
	 * state doesn't completely stop the clocks or transmission. The dai could
	 * be transmitting 0's (silence), but it is not consuming data from outside.
	 */
	DAI_TRIGGER_PAUSE,
	/** @brief Optional - Post Stop the transmission / reception of data.
	 *
	 * Allows the DAI and downstream codecs to shutdown cleanly after audio
	 * Tx/Rx by stopping any required clocks for downstream audio completion.
	 */
	DAI_TRIGGER_POST_STOP,
	/** @brief Empty the transmit queue.
	 *
	 * Send all data in the transmit queue and stop the transmission.
	 * If the trigger is applied to the RX queue it has the same effect as
	 * DAI_TRIGGER_STOP. This trigger can be used in RUNNING state only and
	 * at first changes the interface state to STOPPING. When all TX blocks
	 * are transmitted the state is changed to READY.
	 */
	DAI_TRIGGER_DRAIN,
	/** @brief Discard the transmit / receive queue.
	 *
	 * Stop the transmission / reception immediately and discard the
	 * contents of the respective queue. This trigger can be used in any
	 * state other than NOT_READY and changes the interface state to READY.
	 */
	DAI_TRIGGER_DROP,
	/** @brief Prepare the queues after underrun/overrun error has occurred.
	 *
	 * This trigger can be used in ERROR state only and changes the
	 * interface state to READY.
	 */
	DAI_TRIGGER_PREPARE,
	/** @brief Reset
	 *
	 * This trigger frees resources and moves the driver back to initial
	 * state.
	 */
	DAI_TRIGGER_RESET,
	/** @brief Copy
	 *
	 * This trigger prepares for data copying.
	 */
	DAI_TRIGGER_COPY,
};

/** @brief Properties of DAI
 *
 * This struct is used with APIs get_properties function to query DAI
 * properties like fifo address and dma handshake. These are needed
 * for example to setup dma outside the driver code.
 *
 * @param fifo_address Fifo hw address for e.g. when connecting to dma.
 * @param fifo_depth Fifo depth.
 * @param dma_hs_id Dma handshake id.
 * @param reg_init_delay Delay for initializing registers.
 * @param stream_id Stream ID.
 */
struct dai_properties {
	uint32_t fifo_address; /* fifo address */
	uint32_t fifo_depth; /* fifo depth */
	uint32_t dma_hs_id; /* dma handshake id */
	uint32_t reg_init_delay; /* delay for register init */
	int stream_id; /* stream id */
};

/** Main dai config struct
 * @brief Generic Dai interface configuration options.
 *
 * @param dai_type Type of the dai.
 * @param dai_index Index of the dai.
 * @param channels Number of audio channels, words in frame.
 * @param rate Frame clock (WS) frequency, sampling rate.
 * @param format Dai specific data stream format.
 * @param options Dai specific configuration options.
 * @param word_size Number of bits representing one data word.
 * @param block_size Size of one RX/TX memory block (buffer) in bytes.
 */
struct dai_config {
	enum dai_type type;
	uint32_t dai_index;
	uint8_t channels;
	uint32_t rate;
	uint16_t format;
	uint8_t options;
	uint8_t word_size;
	size_t block_size;
};

struct dai_ts_cfg {
	uint32_t walclk_rate; /* Rate in Hz, e.g. 19200000 */
	int type; /* SSP, DMIC, HDA, etc. */
	int direction; /* Playback, capture */
	int index; /* For SSPx to select correct timestamp register */
	int dma_id; /* DMA instance id */
	int dma_chan_index; /* Used DMA channel */
	int dma_chan_count; /* Channels in single DMA */
};

struct dai_ts_data {
	uint64_t walclk; /* Wall clock */
	uint64_t sample; /* Sample count */
	uint32_t walclk_rate; /* Rate in Hz, e.g. 19200000 */
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */
__subsystem struct dai_driver_api {
	int (*probe)(const struct device *dev);
	int (*remove)(const struct device *dev);
	int (*config_set)(const struct device *dev, const struct dai_config *cfg,
			  const void *bespoke_cfg);
	const struct dai_config *(*config_get)(const struct device *dev,
					       enum dai_dir dir);

	const struct dai_properties *(*get_properties)(const struct device *dev,
						       enum dai_dir dir,
						       int stream_id);

	int (*trigger)(const struct device *dev, enum dai_dir dir,
		       enum dai_trigger_cmd cmd);

	/* optional methods */
	int (*ts_config)(const struct device *dev, struct dai_ts_cfg *cfg);
	int (*ts_start)(const struct device *dev, struct dai_ts_cfg *cfg);
	int (*ts_stop)(const struct device *dev, struct dai_ts_cfg *cfg);
	int (*ts_get)(const struct device *dev, struct dai_ts_cfg *cfg,
		      struct dai_ts_data *tsd);
};

/**
 * @endcond
 */

/**
 * @brief Probe operation of DAI driver.
 *
 * The function will be called to power up the device and update for example
 * possible reference count of the users. It can be used also to initialize
 * internal variables and memory allocation.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
static inline int dai_probe(const struct device *dev)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	return api->probe(dev);
}

/**
 * @brief Remove operation of DAI driver.
 *
 * The function will be called to unregister/unbind the device, for example to
 * power down the device or decrease the usage reference count.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
static inline int dai_remove(const struct device *dev)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	return api->remove(dev);
}

/**
 * @brief Configure operation of a DAI driver.
 *
 * The dir parameter specifies if Transmit (TX) or Receive (RX) direction
 * will be configured by data provided via cfg parameter.
 *
 * The function can be called in NOT_READY or READY state only. If executed
 * successfully the function will change the interface state to READY.
 *
 * If the function is called with the parameter cfg->frame_clk_freq set to 0
 * the interface state will be changed to NOT_READY.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Pointer to the structure containing configuration parameters.
 * @param bespoke_cfg Pointer to the structure containing bespoke config.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS DAI_DIR_BOTH value is not supported.
 */
static inline int dai_config_set(const struct device *dev,
				 const struct dai_config *cfg,
				 const void *bespoke_cfg)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	return api->config_set(dev, cfg, bespoke_cfg);
}

/**
 * @brief Fetch configuration information of a DAI driver
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dir Stream direction: RX or TX as defined by DAI_DIR_*
 * @retval Pointer to the structure containing configuration parameters,
 *         or NULL if un-configured
 */
static inline const struct dai_config *dai_config_get(const struct device *dev,
						      enum dai_dir dir)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	return api->config_get(dev, dir);
}

/**
 * @brief Fetch properties of a DAI driver
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dir Stream direction: RX or TX as defined by DAI_DIR_*
 * @param stream_id Stream id: some drivers may have stream specific
 *        properties, this id specifies the stream.
 * @retval Pointer to the structure containing properties,
 *         or NULL if error or no properties
 */
static inline const struct dai_properties *dai_get_properties(const struct device *dev,
							      enum dai_dir dir,
							      int stream_id)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	return api->get_properties(dev, dir, stream_id);
}

/**
 * @brief Send a trigger command.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dir Stream direction: RX, TX, or both, as defined by DAI_DIR_*.
 *            The DAI_DIR_BOTH value may not be supported by some drivers.
 *            For those, triggering need to be done separately for the RX
 *            and TX streams.
 * @param cmd Trigger command.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid argument.
 * @retval -EIO The trigger cannot be executed in the current state or a DMA
 *         channel cannot be allocated.
 * @retval -ENOMEM RX/TX memory block not available.
 * @retval -ENOSYS DAI_DIR_BOTH value is not supported.
 */
static inline int dai_trigger(const struct device *dev,
			      enum dai_dir dir,
			      enum dai_trigger_cmd cmd)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	return api->trigger(dev, dir, cmd);
}

/**
 * Configures timestamping in attached DAI.
 * @param dev Component device.
 * @param cfg Timestamp config.
 *
 * Optional method.
 *
 * @retval 0 If successful.
 */
static inline int dai_ts_config(const struct device *dev, struct dai_ts_cfg *cfg)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	if (!api->ts_config)
		return -EINVAL;

	return api->ts_config(dev, cfg);
}

/**
 * Starts timestamping.
 * @param dev Component device.
 * @param cfg Timestamp config.
 *
 * Optional method
 *
 * @retval 0 If successful.
 */
static inline int dai_ts_start(const struct device *dev, struct dai_ts_cfg *cfg)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	if (!api->ts_start)
		return -EINVAL;

	return api->ts_start(dev, cfg);
}

/**
 * Stops timestamping.
 * @param dev Component device.
 * @param cfg Timestamp config.
 *
 * Optional method.
 *
 * @retval 0 If successful.
 */
static inline int dai_ts_stop(const struct device *dev, struct dai_ts_cfg *cfg)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	if (!api->ts_stop)
		return -EINVAL;

	return api->ts_stop(dev, cfg);
}

/**
 * Gets timestamp.
 * @param dev Component device.
 * @param cfg Timestamp config.
 * @param tsd Receives timestamp data.
 *
 * Optional method.
 *
 * @retval 0 If successful.
 */
static inline int dai_ts_get(const struct device *dev, struct dai_ts_cfg *cfg,
			     struct dai_ts_data *tsd)
{
	const struct dai_driver_api *api = (const struct dai_driver_api *)dev->api;

	if (!api->ts_get)
		return -EINVAL;

	return api->ts_get(dev, cfg, tsd);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DAI_H_ */
