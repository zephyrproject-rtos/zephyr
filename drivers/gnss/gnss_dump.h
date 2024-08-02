/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_DUMP_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_DUMP_H_

#include <zephyr/drivers/gnss.h>

/**
 * @brief Dump struct gnss_info as string
 *
 * @param str Destination for dumped GNSS info
 * @param strsize Size of str
 * @param info GNSS info to dump
 *
 * @retval 0 if GNSS info successfully dumped
 * @retval -ENOMEM if strsize too small
 */
int gnss_dump_info(char *str, uint16_t strsize, const struct gnss_info *info);

/**
 * @brief Dump struct navigation_data as string
 *
 * @param str Destination for dumped navigation data
 * @param strsize Size of str
 * @param nav_data Navigation data to dump
 *
 * @retval 0 if navigation data successfully dumped
 * @retval -ENOMEM if strsize too small
 */
int gnss_dump_nav_data(char *str, uint16_t strsize, const struct navigation_data *nav_data);

/**
 * @brief Dump struct gnss_time as string
 *
 * @param str Destination for dumped GNSS time
 * @param strsize Size of str
 * @param utc GNSS time to dump
 *
 * @retval 0 if GNSS time successfully dumped
 * @retval -ENOMEM if strsize too small
 */
int gnss_dump_time(char *str, uint16_t strsize, const struct gnss_time *utc);

/**
 * @brief Dump struct gnss_satellite as string
 *
 * @param str Destination for dumped GNSS satellite
 * @param strsize Size of str
 * @param utc GNSS satellite to dump
 *
 * @retval 0 if GNSS satellite successfully dumped
 * @retval -ENOMEM if strsize too small
 */
int gnss_dump_satellite(char *str, uint16_t strsize, const struct gnss_satellite *satellite);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_DUMP_H_ */
