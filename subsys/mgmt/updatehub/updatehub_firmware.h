/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_FIRMWARE_H__
#define __UPDATEHUB_FIRMWARE_H__

#include <drivers/flash.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>

bool updatehub_get_firmware_version(char *version, int version_len);

#endif /* __UPDATEHUB_FIRMWARE_H__ */
