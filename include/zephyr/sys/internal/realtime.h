/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_INTERNAL_REALTIME_H_
#define ZEPHYR_INCLUDE_SYS_INTERNAL_REALTIME_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/realtime.h>

/** Validate timestamp */
bool sys_realtime_validate_timestamp(const int64_t *timestamp_ms);

/** Validate datetime */
bool sys_realtime_validate_datetime(const struct sys_datetime *datetime);

#endif /* ZEPHYR_INCLUDE_SYS_INTERNAL_REALTIME_H_ */
