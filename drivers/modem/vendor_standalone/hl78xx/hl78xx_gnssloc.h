/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_GNSSLOC_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_GNSSLOC_H_

#include <stdint.h>
#include <zephyr/drivers/gnss.h>

/**
 * @brief Convert a GNSSLOC DMS coordinate string to nanodegrees.
 *
 * Expected format example:
 * "52 Deg 15 Min 13.61 Sec N"
 *
 * @param str Coordinate string.
 * @param ndeg Output coordinate in nanodegrees.
 *
 * @retval 0 on success.
 * @retval -EINVAL on invalid input.
 */
int gnssloc_dms_to_ndeg(const char *str, int64_t *ndeg);

/**
 * @brief Parse a decimal value with a textual unit suffix to milli-units.
 *
 * Expected format example:
 * "123.456 m"
 *
 * @param str Value string.
 * @param milli Output value in milli-units.
 *
 * @retval 0 on success.
 * @retval -EINVAL on invalid input.
 */
int gnssloc_parse_value_with_unit(const char *str, int64_t *milli);

/**
 * @brief Parse speed string to millimeters per second.
 *
 * Expected format example:
 * "4.250 m/s"
 *
 * @param str Speed string.
 * @param mms Output speed in millimeters per second.
 *
 * @retval 0 on success.
 * @retval -EINVAL on invalid input.
 * @retval -ERANGE on negative or too large value.
 */
int gnssloc_parse_speed_to_mms(const char *str, uint32_t *mms);

/**
 * @brief Parse GNSSLOC GPS time string.
 *
 * Expected format example:
 * "2026 1 25 23:15:56"
 *
 * @param str Time string.
 * @param utc Output GNSS time.
 *
 * @retval 0 on success.
 * @retval -EINVAL on malformed input.
 * @retval -ERANGE on out-of-range date/time fields.
 */
int gnssloc_parse_gpstime(const char *str, struct gnss_time *utc);

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_GNSSLOC_H_ */
