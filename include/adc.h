/* adc.h - ADC public API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief ADC public API header file.
 */

#ifndef __INCLUDE_ADC_H__
#define __INCLUDE_ADC_H__

#include <stdint.h>
#include <device.h>

/**
 * Callback type.
 * ADC_CB_DONE means sampling went fine and is over
 * ADC_CB_ERROR means an error occurred
 */
enum adc_callback_type {
	ADC_CB_DONE	= 0,
	ADC_CB_ERROR	= 1
};

/** This type defines an pointer to an ADC callback routine */
typedef void (*adc_callback_t)(struct device *dev,
				enum adc_callback_type cb_type);

/**
 * @brief Sequence entry
 *
 * This structure defines a sequence entry used by the ADC driver
 * to define a sample from a specific channel.
 */
struct adc_seq_entry {
	/** Clock ticks delay before sampling the ADC */
	int32_t sampling_delay;
	/** Channel ID that should be sampled from the ADC */
	uint8_t channel_id;
	/** Buffer pointer where to write the sample */
	uint8_t *buffer;
	/** Length of the sampling buffer */
	uint32_t buffer_length;
};

/**
 * @brief Sequence table
 *
 * This structure represents a list of sequence entries that are
 * used by the ADC driver to execute a sequence of samplings.
 */
struct adc_seq_table {
	/* Pointer to a sequence entry array */
	struct adc_seq_entry *entries;
	/* Number of entries in the sequence entry array */
	uint8_t num_entries;
};

/**
 * @brief ADC driver API
 *
 * This structure holds all API function pointers.
 */
struct adc_driver_api {
	/** Pointer to the enable routine */
	void (*enable)(struct device *dev);
	/** Pointer to the disable routine */
	void (*disable)(struct device *dev);
	/** Pointer to the set_cb routine */
	void (*set_callback)(struct device *dev, adc_callback_t cb);
	/** Pointer to the read routine */
	int (*read)(struct device *dev, struct adc_seq_table *seq_table);
};

/**
 * @brief Enable ADC hardware
 *
 * This routine enables the ADC hardware block for data sampling for the
 * specified device.
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return N/A
 */
static inline void adc_enable(struct device *dev)
{
	struct adc_driver_api *api;

	api = (struct adc_driver_api *)dev->driver_api;
	api->enable(dev);
}

/**
 * @brief Disable ADC hardware
 *
 * This routine disables the ADC hardware block for data sampling for the
 * specified device.
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return N/A
 */
static inline void adc_disable(struct device *dev)
{
	struct adc_driver_api *api;

	api = (struct adc_driver_api *)dev->driver_api;
	api->disable(dev);
}

/**
 * @brief Set callback routine
 *
 * This routine sets the callback routine that will be called by the driver
 * every time that sample data is available for consumption or an error is
 * signaled.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param cb Pointer to the function that will be set as callback.
 *
 * @return N/A
 */
static inline void adc_set_callback(struct device *dev, adc_callback_t cb)
{
	struct adc_driver_api *api;

	api = (struct adc_driver_api *)dev->driver_api;
	api->set_callback(dev, cb);
}

/**
 * @brief Set a read request.
 *
 * This routine sends a read/sampling request to the ADC hardware block.
 * The read request is described by a sequence table. The read data is not
 * available for consumption until it is indicated by a callback.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param seq_tbl Pointer to the structure that represents the sequence table
 *
 * @return Returns DEV_OK on success, or else otherwise.
 */
static inline int adc_read(struct device *dev, struct adc_seq_table *seq_table)
{
	struct adc_driver_api *api;

	api = (struct adc_driver_api *)dev->driver_api;
	return api->read(dev, seq_table);
}

#endif  /* __INCLUDE_ADC_H__ */
