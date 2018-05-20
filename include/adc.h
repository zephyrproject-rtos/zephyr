/**
 * @file
 * @brief ADC public API header file.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE_ADC_H__
#define __INCLUDE_ADC_H__

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC driver APIs
 * @defgroup adc_interface ADC driver APIs
 * @ingroup io_interfaces
 * @{
 */


/** @brief ADC channel gain factors. */
enum adc_gain
{
	ADC_GAIN_1_6, /**< x 1/6. */
	ADC_GAIN_1_5, /**< x 1/5. */
	ADC_GAIN_1_4, /**< x 1/4. */
	ADC_GAIN_1_3, /**< x 1/3. */
	ADC_GAIN_1_2, /**< x 1/2. */
	ADC_GAIN_1,   /**< x 1. */
	ADC_GAIN_2,   /**< x 2. */
	ADC_GAIN_3,   /**< x 3. */
	ADC_GAIN_4,   /**< x 4. */
	ADC_GAIN_8,   /**< x 8. */
	ADC_GAIN_16,  /**< x 16. */
	ADC_GAIN_32,  /**< x 32. */
	ADC_GAIN_64,  /**< x 64. */
};

/** @brief ADC references. */
enum adc_reference
{
	ADC_REF_VDD_1,     /**< VDD. */
	ADC_REF_VDD_1_2,   /**< VDD/2. */
	ADC_REF_VDD_1_3,   /**< VDD/3. */
	ADC_REF_VDD_1_4,   /**< VDD/4. */
	ADC_REF_INTERNAL,  /**< Internal. */
	ADC_REF_EXTERNAL0, /**< External, input 0. */
	ADC_REF_EXTERNAL1, /**< External, input 1. */
};

#define ADC_ACQ_TIME_TICKS         (0u)
#define ADC_ACQ_TIME_MICROSECONDS  (1u)
#define ADC_ACQ_TIME_NANOSECONDS   (2u)
#define ADC_ACQ_TIME(unit, value)  (((unit) << 14) | \
				    ((value) & ((1u << 14)-1)))
#define ADC_ACQ_TIME_UNIT(time)    (((time) >> 14) & 3u)
#define ADC_ACQ_TIME_VALUE(time)   ((time) & ((1u << 14)-1)

/**
 * @brief Structure for specifying configuration of an ADC channel.
 */
struct adc_channel_cfg {
	/** Gain selection. */
	enum adc_gain gain;

	/** Reference selection. */
	enum adc_reference reference;

	/**
	 * Acquisition time.
	 * Use the ADC_ACQ_TIME macro to compose the value for this field.
	 * Particular drivers do not necessarily support all the possible units.
	 * Value range is 0-16383 for a given unit.
	 */
	u16_t acquisition_time;

	/** Channel identifier (0-31). */
	u8_t channel_id   : 5;

	/** Channel type: single-ended or differential. */
	bool differential : 1;

	/** Positive converter input. */
	u8_t input_positive;

	/** Negative converter input (used only for differential channels). */
	u8_t input_negative;
};


/**
 * @brief ADC triggers.
 */
enum adc_trigger
{
	ADC_TRIGGER_DELAY_TICKS,
	ADC_TRIGGER_DELAY_US,
	ADC_TRIGGER_DELAY_MS,

	ADC_TRIGGER_SAMPLE_RATE,

	/* more to be added */
};

/* Forward declaration of the adc_sequence structure. */
struct adc_sequence;

/**
 * @brief Actions to be performed after a sampling is done.
 */
enum adc_action
{
	ADC_ACTION_CONTINUE = 0,
	/**< The sequence should be continued normally. */

	ADC_ACTION_REPEAT,
	/**< The sampling should be repeated. New samples should be read from
	 *   the ADC and written in the same place in as the recent ones.
	 */

	ADC_ACTION_FINISH,
	/**< The sequence should be finished immediately. */
};

/**
 * @brief Type definition of the optional callback function to be called after
 *        a requested sampling is done.
 *
 * @param dev             Pointer to the device structure for the driver
 *                        instance.
 * @param sequence        Pointer to the sequence structure that triggered the
 *                        sampling.
 * @param sampling_index  Index (0-65535) of the sampling done.
 *
 * @returns Action to be performed by the driver. See @ref adc_action.
 */
typedef enum adc_action (*adc_sequence_callback)(
				struct device *dev,
				const struct adc_sequence *sequence,
				u16_t sampling_index);

/**
 * @brief Structure specifying additional options for an ADC sampling sequence.
 */
struct adc_sequence_options {
	/** Type of trigger that starts consecutive samplings. */
	enum adc_trigger trigger_type;
	/**
	 * Parameter for the selected trigger: number of ticks,
	 * microseconds, etc.
	 */
	union {
		u32_t value;
		void *pointer;
	} trigger_param;

	/**
	 * Callback to be called after each sampling is done.
	 * Optional - set to NULL if it is not needed.
	 */
	adc_sequence_callback callback;

	/**
	 * Number of extra samplings to perform (the total number is equal to:
	 * 1 + extra_samplings).
	 */
	u16_t extra_samplings;
};

/**
 * @brief Structure defining an ADC sampling sequence.
 */
struct adc_sequence {
	/**
	 * Pointer to a structure defining additional options for the sequence.
	 * If NULL, the sequence consists of a single sampling.
	 */
	const struct adc_sequence_options *options;

	/**
	 * Bit-mask indicating the channels to be included in each sampling
	 * of this sequence.
	 * All selected channels must be configured with adc_channel_setup()
	 * before they are used in a sequence.
	 */
	u32_t channels;

	/**
	 * Pointer to a buffer where the samples are to be written. Samples
	 * from subsequent samplings are written sequentially in the buffer.
	 * The number of samples written for each sampling is determined by
	 * the number of channels selected in the 'channels' field.
	 * The buffer must be of an appropriate size, taking into account
	 * the number of selected channels and the ADC resolution used
	 * as well as the number of samplings contained in the sequence.
	 */
	void *buffer;

	/**
	 * Specifies the actual size of the buffer pointed by the 'buffer'
	 * field (in bytes). The driver must ensure that samples are not
	 * written beyond the limit and it must return an error if the buffer
	 * turns out to be not large enough to hold all the requested samples.
	 */
	size_t buffer_size;

	/**
	 * ADC resolution.
	 * For single-ended channels the sample values are from range:
	 *   0 .. 2^resolution - 1,
	 * for differential ones:
	 *  -2^(resolution-1) .. 2^(resolution-1) - 1.
	 */
	u8_t resolution;

	/**
	 * Oversampling setting.
	 * Each sample is averaged from 2^oversampling results.
	 */
	u8_t oversampling;
};


/**
 * @brief Type definition of ADC API function for configuring a channel.
 * See adc_channel_setup() for argument descriptions.
 */
typedef int (*adc_api_channel_setup)(struct device *dev,
				     const struct adc_channel_cfg *channel_cfg);

/**
 * @brief Type definition of ADC API function for setting a read request.
 * See adc_read() for argument descriptions.
 */
typedef int (*adc_api_read)(struct device *dev,
			    const struct adc_sequence *sequence);

#ifdef CONFIG_ADC_ASYNC
/**
 * @brief Type definition of ADC API function for setting an asynchronous
 *        read request.
 * See adc_read_async() for argument descriptions.
 */
typedef int (*adc_api_read_async)(struct device *dev,
				  const struct adc_sequence *sequence,
				  struct k_poll_signal *async);
#endif

/**
 * @brief ADC driver API
 *
 * This is the mandatory API any ADC driver needs to expose.
 */
struct adc_driver_api {
	adc_api_channel_setup channel_setup;
	adc_api_read          read;
#ifdef CONFIG_ADC_ASYNC
	adc_api_read_async    read_async;
#endif
};

/**
l * @brief Configure a converter channel.
 *
 * It is required to call this function and configure each channel before it is
 * selected for a read request.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param channel_cfg  Channel configuration.
 *
 * @returns 0 on success, negative errno code otherwise.
 */
__syscall int adc_channel_setup(struct device *dev,
				const struct adc_channel_cfg *channel_cfg);

static inline
int _impl_adc_channel_setup(struct device *dev,
			    const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_driver_api *api = dev->driver_api;

	return api->channel_setup(dev, channel_cfg);
}

/**
 * @brief Set a read request.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param sequence  Structure specifying requested sequence of samplings.
 *
 * @returns 0 on success, negative errno code otherwise.
 */
__syscall int adc_read(struct device *dev,
		       const struct adc_sequence *sequence);

static inline int _impl_adc_read(struct device *dev,
				 const struct adc_sequence *sequence)
{
	const struct adc_driver_api *api = dev->driver_api;

	return api->read(dev, sequence);
}

#ifdef CONFIG_ADC_ASYNC
/**
 * @brief Set an asynchronous read request.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param sequence  Structure specifying requested sequence of samplings.
 * @param async     Pointer to a valid and ready to be signaled struct
 *                  k_poll_signal. (Note: if NULL this function will not notify
 *                  the end of the transaction, and whether it went successfully
 *                  or not).
 *
 * @returns 0 on success, negative errno code otherwise.
 */
__syscall int adc_read_async(struct device *dev,
			     const struct adc_sequence *sequence,
			     struct k_poll_signal *signal);

static inline int _impl_adc_read_async(struct device *dev,
				       const struct adc_sequence *sequence,
				       struct k_poll_signal *async)
{
	const struct adc_driver_api *api = dev->driver_api;

	return api->read_async(dev, sequence, async);
}
#endif /* CONFIG_ADC_ASYNC */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/adc.h>

#endif  /* __INCLUDE_ADC_H__ */
