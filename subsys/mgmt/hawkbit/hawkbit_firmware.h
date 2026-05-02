/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAWKBIT_FIRMWARE_H__
#define __HAWKBIT_FIRMWARE_H__

#include <zephyr/drivers/flash.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/flash_img.h>

bool hawkbit_get_firmware_version(char *version, int version_len);

#endif /* __HAWKBIT_FIRMWARE_H__ */
