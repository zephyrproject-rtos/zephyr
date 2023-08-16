/*
 * Copyright 2023 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_U8G2_U8G2_ZEPHYR_H_
#define ZEPHYR_MODULES_U8G2_U8G2_ZEPHYR_H_

#include <u8g2.h>

struct device;

/**
 * Setup u8g2_t instance.
 *
 * @param dev Zephyr's display device instance
 * @param u8g2_cb draw procedure callback
 */
u8g2_t *u8g2_SetupZephyr(const struct device *dev, const u8g2_cb_t *u8g2_cb);

/**
 * Setup u8g2_t instance.
 *
 * @param dev Zephyr's display device instance
 */
u8x8_t *u8x8_SetupZephyr(const struct device *dev);

/**
 * Teardown u8g2_t instance.
 *
 * @param u8g2 Instance that create with u8x8_SetupZephyr
 */
void u8g2_TeardownZephyr(u8g2_t *u8g2);

/**
 * Teardown u8x8_t instance.
 *
 * @param u8x8 Instance that create with u8x8_SetupZephyr
 */
void u8x8_TeardownZephyr(u8x8_t *u8x8);

#endif /* ZEPHYR_MODULES_U8G2_U8G2_ZEPHYR_H_ */
