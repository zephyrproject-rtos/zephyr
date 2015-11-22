/*
 * Copyright (c) 2015 Intel Corporation.
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

#ifndef _AIO_COMPARATOR_H_
#define _AIO_COMPARATOR_H_

enum aio_cmp_ref {
	AIO_CMP_REF_A,		/**< Use reference A. */
	AIO_CMP_REF_B,		/**< Use reference B. */
};

enum aio_cmp_polarity {
	AIO_CMP_POL_RISE,	/**< Match on rising edge. */
	AIO_CMP_POL_FALL,	/**< Match on falling edge. */
};

typedef void (*aio_cmp_cb)(void *);

typedef int (*aio_cmp_api_disable)(struct device *dev, uint8_t index);

typedef int (*aio_cmp_api_configure)(struct device *dev, uint8_t index,
    enum aio_cmp_polarity polarity, enum aio_cmp_ref refsel,
    aio_cmp_cb cb, void *param);

struct aio_cmp_driver_api {
	aio_cmp_api_disable disable;
	aio_cmp_api_configure configure;
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
static inline int aio_cmp_disable(struct device *dev, uint8_t index)
{
	struct aio_cmp_driver_api *api;

	api = (struct aio_cmp_driver_api *)dev->driver_api;
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
static inline int aio_cmp_configure(struct device *dev, uint8_t index,
			     enum aio_cmp_polarity polarity,
			     enum aio_cmp_ref refsel,
			     aio_cmp_cb cb, void *param)
{
	struct aio_cmp_driver_api *api;

	api = (struct aio_cmp_driver_api *)dev->driver_api;
	return api->configure(dev, index, polarity, refsel, cb, param);
}

#endif /* _AIO_COMPARATOR_H_ */
