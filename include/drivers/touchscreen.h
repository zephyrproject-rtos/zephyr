/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief APIs for touchscreen drivers.
 */

#ifndef _TOUCHSCREEN_H_
#define _TOUCHSCREEN_H_

#include <device.h>
#include <zephyr/types.h>

/** A single sample of the touchscreen's state */
struct touchscreen_sample {
	/** Sampled value for touchscreen X position */
	u16_t x;
	/** Sampled value for touchscreen Y position */
	u16_t y;
	/** Sampled value for touchscreen Z (pressure) position */
	u16_t z;
	/** Flags associated with this sample */
	u16_t flags;
};

/* Following #defines are for flags in struct touchscreen_sample */

/** Touchscreen is being touched */
#define TOUCHSCREEN_TOUCHED	(1 << 0)

/* API to be implemented by touchscreen drivers */
struct touchscreen_api {
	/* Get the next sample of the touchscreen state */
	int (*get_sample)(struct device *dev,
				struct touchscreen_sample *sample);
	/* Set the callback function to be called when a sample is available */
	void (*set_callback)(struct device *dev,
				void (*callback)(struct device *));
};

/**
 * @brief Get a sample from the touchscreen.
 *
 * @param dev    Pointer to the device structure for the driver instance.
 * @param sample Pointer to the sample to update.
 *
 * @return 0 on success, -EAGAIN if no more samples available,
 *         otherwise negative error value.
 */
static inline int touchscreen_get_sample(struct device *dev,
					 struct touchscreen_sample *sample)
{
	const struct touchscreen_api *api = dev->driver_api;

	return api->get_sample(dev, sample);
}

/**
 * @brief Set the callback function.
 *
 * The callback function will be called when one or more samples becomes
 * available. It may be executed in any any context, e.g. ISR or thread context,
 * or may be called during the execution of touchscreen_get_sample(). Therefore
 * the function mustn't itself use touchscreen_get_sample() or make use of
 * kernel services that aren't safe from interrupt context.
 *
 * A typical action for the callback is to simply signal a semaphore or trigger
 * some work on a workqueue.
 *
 * The callback function won't be called again until all samples have been
 * retrieved, i.e. until the call to touchscreen_get_sample() which returned
 * -EAGAIN.
 *
 * @param dev      Pointer to the device structure for the driver instance.
 * @param callback Pointer to the callback function.
 */
static inline void touchscreen_set_callback(struct device *dev,
					   void (*callback)(struct device *))
{
	const struct touchscreen_api *api = dev->driver_api;

	api->set_callback(dev, callback);
}


#ifdef CONFIG_TOUCHSCREEN_HELPERS

/** X,Y coordinates for a point on the touchscreen or display. */
struct touchscreen_point {
	u16_t x;
	u16_t y;
};

/**
 * @brief Touchscreen coordinate translation parameters
 *
 * These are initialised by calling touchscreen_set_calibration().
 * Members should be considered private and not accessed directly.
 */
struct touchscreen_xlat {
	s64_t	A;
	s64_t	B;
	s64_t	C;
	s64_t	D;
	s64_t	E;
	s64_t	F;
};

/**
 * @brief Initialise touchscreen coordinate translation parameters.
 *
 * This requires that three different and valid display/touchscreen coordinate
 * pairs are know.
 *
 * @param xlat		Translation parameters to be initialised.
 * @param display	Three display coordinates that correspond to the given
 *			touchscreen coordinates.
 * @param touchscreen	Three touchscreen coordinates that correspond to the
 *			given display coordinates.
 */
void touchscreen_set_calibration(struct touchscreen_xlat *xlat,
				 const struct touchscreen_point display[3],
				 const struct touchscreen_point touchscreen[3]);

/**
 * @brief Translate touchscreen coordinates into screen coordinates.
 *
 * Convert a touchscreen X,Y sample into the corresponding display X,Y
 * coordinates by using the previously initialised translation parameters.
 *
 * @param xlat		Translation parameters.
 * @param dislpay_x	Display X coordinate
 * @param dislpay_x	Display Y coordinate
 * @param touch_x	Touchscreen X sample
 * @param touch_y	Touchscreen Y sample
 */
void touchscreen_translate(struct touchscreen_xlat *xlat,
			   int *display_x, int *display_y,
			   int touch_x, int touch_y);

#endif /* CONFIG_TOUCHSCREEN_HELPERS */


#endif /* _TOUCHSCREEN_H_ */
