/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INPUT_LINUX_EVDEV_BOTTOM_H_
#define ZEPHYR_DRIVERS_INPUT_LINUX_EVDEV_BOTTOM_H_

#include <stdint.h>

#define NATIVE_LINUX_EVDEV_NO_DATA INT32_MIN

int linux_evdev_read(int fd, uint16_t *type, uint16_t *code, int32_t *value);
int linux_evdev_open(const char *path);

#endif /* ZEPHYR_DRIVERS_INPUT_LINUX_EVDEV_BOTTOM_H_ */
