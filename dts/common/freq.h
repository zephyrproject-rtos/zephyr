/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DTS_COMMON_FREQ_H_
#define ZEPHYR_DTS_COMMON_FREQ_H_

/**
 * @ingroup dts_utils
 * @{
 */

/**
 * @brief Converts a frequency value in kilohertz to hertz.
 *
 * @param x Frequency value in kilohertz.
 * @return Frequency value in hertz.
 */
#define DT_FREQ_K(x) ((x) * 1000)

/**
 * @brief Converts a frequency value in megahertz to hertz.
 *
 * @param x Frequency value in megahertz.
 * @return Frequency value in hertz.
 */
#define DT_FREQ_M(x) (DT_FREQ_K(x) * 1000)

/**
 * @}
 */

#endif /* ZEPHYR_DTS_COMMON_FREQ_H_ */
