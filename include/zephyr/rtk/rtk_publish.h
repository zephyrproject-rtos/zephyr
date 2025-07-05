/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RTK_RTK_PUBLISH_H_
#define ZEPHYR_INCLUDE_RTK_RTK_PUBLISH_H_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/rtk/rtk.h>

/* Internal function used by RTK clients to publish data-correction. */
void rtk_publish_data(const struct rtk_data *data);

#endif /* ZEPHYR_INCLUDE_RTK_RTK_H_ */
