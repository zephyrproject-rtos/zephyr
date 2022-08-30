/*
 * Copyright (c) 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_HT16K33_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_HT16K33_H_

#include <zephyr/drivers/kscan.h>

/**
 * Register a HT16K33 keyscan device to be notified of relevant
 * keyscan events by the keyscan interrupt thread in the HT16K33
 * parent driver.
 *
 * @param parent HT16K33 parent device.
 * @param child HT16K33 child device.
 * @param callback Keyscan callback function.
 * @return 0 if successful, negative errno code on failure.
 */
int ht16k33_register_keyscan_callback(const struct device *parent,
				      const struct device *child,
				      kscan_callback_t callback);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_HT16K33_H_ */
