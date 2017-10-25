/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIO_COMPARATOR_H_
#define _AIO_COMPARATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

enum aio_cmp_ref {
	AIO_CMP_REF_A,		/**< Use reference A. */
	AIO_CMP_REF_B,		/**< Use reference B. */
};

enum aio_cmp_polarity {
	AIO_CMP_POL_RISE,	/**< Match on rising edge. */
	AIO_CMP_POL_FALL,	/**< Match on falling edge. */
};

typedef void (*aio_cmp_cb)(void *);

typedef int (*aio_cmp_api_disable)(struct device *dev, u8_t index);

typedef int (*aio_cmp_api_configure)(struct device *dev, u8_t index,
    enum aio_cmp_polarity polarity, enum aio_cmp_ref refsel,
    aio_cmp_cb cb, void *param);
typedef u32_t (*aio_cmp_api_get_pending_int)(struct device *dev);

struct aio_cmp_driver_api {
	aio_cmp_api_disable disable;
	aio_cmp_api_configure configure;
	aio_cmp_api_get_pending_int get_pending_int;
};

/**
 * @brief Disable a particular comparator.
 *
 * This disables a comparator so that it no longer triggers interrupts.
 *
 * @param dev Device struct
 * @param index The index of the comparator to disable
 *
 * @return 0 if successful, otherwise failed.
 */
__syscall int aio_cmp_disable(struct device *dev, u8_t index);

static inline int _impl_aio_cmp_disable(struct device *dev, u8_t index)
{
	const struct aio_cmp_driver_api *api = dev->driver_api;

	return api->disable(dev, index);
}

/**
 * @brief Configure and enable a particular comparator.
 *
 * This performs configuration and enable a comparator, so that it will
 * generate interrupts when conditions are met.
 *
 * @param dev Device struct
 * @param index The index of the comparator to disable
 * @param polarity Match polarity (e.g. rising or falling)
 * @param refsel Reference for trigger
 * @param cb Function callback (aio_cmp_cb)
 * @param param Parameters to be passed to callback
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int aio_cmp_configure(struct device *dev, u8_t index,
				    enum aio_cmp_polarity polarity,
				    enum aio_cmp_ref refsel,
				    aio_cmp_cb cb, void *param)
{
	const struct aio_cmp_driver_api *api = dev->driver_api;

	return api->configure(dev, index, polarity, refsel, cb, param);
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval status != 0 if at least one aio_cmp interrupt is pending.
 * @retval 0 if no aio_cmp interrupt is pending.
 */
__syscall int aio_cmp_get_pending_int(struct device *dev);

static inline int _impl_aio_cmp_get_pending_int(struct device *dev)
{
	struct aio_cmp_driver_api *api;

	api = (struct aio_cmp_driver_api *)dev->driver_api;
	return api->get_pending_int(dev);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/aio_comparator.h>

#endif /* _AIO_COMPARATOR_H_ */
