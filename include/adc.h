/**
 * @file
 * @brief ADC public API header file.
 */

/*
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

/**
 * @brief ADC driver Sequence entry
 *
 * This structure defines a sequence entry used
 * to define a sample from a specific channel.
 */
struct adc_seq_entry {
	/** Clock ticks delay before sampling the ADC. */
	s32_t sampling_delay;

	/** Buffer pointer where the sample is written.*/
	u8_t *buffer;

	/** Length of the sampling buffer.*/
	u32_t buffer_length;

	/** Channel ID that should be sampled from the ADC */
	u8_t channel_id;

	u8_t stride[3];
};

/**
 * @brief ADC driver Sequence table
 *
 * This structure defines a list of sequence entries
 * used to execute a sequence of samplings.
 */
struct adc_seq_table {
	/* Pointer to a sequence entry array. */
	struct adc_seq_entry *entries;

	/* Number of entries in the sequence entry array. */
	u8_t num_entries;
	u8_t stride[3];
};

/**
 * @brief ADC driver API
 *
 * This structure holds all API function pointers.
 */
struct adc_driver_api {
	/** Pointer to the enable routine. */
	void (*enable)(struct device *dev);

	/** Pointer to the disable routine. */
	void (*disable)(struct device *dev);

	/** Pointer to the read routine. */
	int (*read)(struct device *dev, struct adc_seq_table *seq_table);
};

/**
 * @brief Enable ADC hardware
 *
 * This routine enables the ADC hardware block for data sampling for the
 * specified device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return N/A
 */
__syscall void adc_enable(struct device *dev);

static inline void _impl_adc_enable(struct device *dev)
{
	const struct adc_driver_api *api = dev->driver_api;

	api->enable(dev);
}

/**
 * @brief Disable ADC hardware
 *
 * This routine disables the ADC hardware block for data sampling for the
 * specified device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return N/A
 */
__syscall void adc_disable(struct device *dev);

static inline void _impl_adc_disable(struct device *dev)
{
	const struct adc_driver_api *api = dev->driver_api;

	api->disable(dev);
}

/**
 * @brief Set a read request.
 *
 * This routine sends a read or sampling request to the ADC hardware block.
 * A sequence table describes the read request.
 * The routine returns once the ADC completes the read sequence.
 * The sample data can be retrieved from the memory buffers in
 * the sequence table structure.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param seq_table Pointer to the structure representing the sequence table.
 *
 * @retval 0 On success
 * @retval else Otherwise.
 */
__syscall int adc_read(struct device *dev, struct adc_seq_table *seq_table);

static inline int _impl_adc_read(struct device *dev,
				 struct adc_seq_table *seq_table)
{
	const struct adc_driver_api *api = dev->driver_api;

	return api->read(dev, seq_table);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/adc.h>

#endif  /* __INCLUDE_ADC_H__ */
