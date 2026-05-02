/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_PARSE_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_PARSE_H_

#include <zephyr/types.h>

/**
 * @brief Parse decimal string to nano parts
 *
 * @example "-1231.3512" -> -1231351200000
 *
 * @param str The decimal string to be parsed
 * @param nano Destination for parsed decimal
 *
 * @retval -EINVAL if str could not be parsed
 * @retval 0 if str successfully parsed
 */
int gnss_parse_dec_to_nano(const char *str, int64_t *nano);

/**
 * @brief Parse decimal string to micro parts
 *
 * @example "-1231.3512" -> -1231351200
 *
 * @param str The decimal string to be parsed
 * @param milli Destination for parsed decimal
 *
 * @retval -EINVAL if str could not be parsed
 * @retval 0 if str successfully parsed
 */
int gnss_parse_dec_to_micro(const char *str, uint64_t *micro);

/**
 * @brief Parse decimal string to milli parts
 *
 * @example "-1231.3512" -> -1231351
 *
 * @param str The decimal string to be parsed
 * @param milli Destination for parsed decimal
 *
 * @retval -EINVAL if str could not be parsed
 * @retval 0 if str successfully parsed
 */
int gnss_parse_dec_to_milli(const char *str, int64_t *milli);

/**
 * @brief Parse integer string of configurable base to integer
 *
 * @example "-1231" -> -1231
 *
 * @param str Decimal string to be parsed
 * @param base Base of decimal string to be parsed
 * @param integer Destination for parsed integer
 *
 * @retval -EINVAL if str could not be parsed
 * @retval 0 if str successfully parsed
 */
int gnss_parse_atoi(const char *str, uint8_t base, int32_t *integer);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_PARSE_H_ */
