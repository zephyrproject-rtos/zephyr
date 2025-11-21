/*
 * Copyright (c) 2025 Lothar Felten <lothar.felten@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SNTP (Simple Network Time Protocol, RFC5905) Server
 */

#ifndef ZEPHYR_INCLUDE_NET_SNTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_SNTP_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple Network Time Protocol Server API
 * @defgroup sntp SNTP
 * @since 4.3
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/**
 * @brief Set SNTP time source id and stratum
 * @param refid the reference ID (RFC5905, Fig.12)
 * @param stratum indicating the stratum of the clock source
 * @param precision an exponent of two, where the resulting value is the precision
 *  of the system clock in seconds.
 *
 * @return 0 if ok, <0 if error.
 */
int sntp_server_clock_source(unsigned char *refid, unsigned char stratum, char precision);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SNTP_SERVER_H_ */
