/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_FIRMWARE_H__
#define __UPDATEHUB_FIRMWARE_H__

#include <zephyr/drivers/flash.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/flash_img.h>

bool updatehub_get_firmware_version(char *version, int version_len);

#endif /* __UPDATEHUB_FIRMWARE_H__ */
