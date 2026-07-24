/*
 * SPDX-FileCopyrightText: Copyright 2025 L. Felten <lothar.felten@gmail.com>
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SNTP (Simple Network Time Protocol, RFC5905) Server
 */

#ifndef ZEPHYR_INCLUDE_NET_SNTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_SNTP_SERVER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple Network Time Protocol Server API
 * @defgroup sntp_server SNTP server
 * @since 4.5
 * @version 0.1.0
 * @ingroup sntp
 * @ingroup networking
 * @{
 */

/**
 * @brief Set SNTP time source id and stratum
 * @param ref_id the reference ID (RFC5905, Fig.12)
 * @param stratum indicating the stratum of the clock source
 * @param precision an exponent of two, where the resulting value is the precision
 *  of the system clock in seconds.
 *
 * @return 0 if ok, <0 if error.
 */
int sntp_server_clock_source(const uint8_t ref_id[4], uint8_t stratum, int8_t precision);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SNTP_SERVER_H_ */
