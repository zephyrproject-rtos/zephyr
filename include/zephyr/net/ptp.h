/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public functions for the Precision Time Protocol.
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 */

#ifndef ZEPHYR_INCLUDE_NET_PTP_H_
#define ZEPHYR_INCLUDE_NET_PTP_H_

/**
 * @brief Precision Time Protocol (PTP) support
 * @defgroup ptp PTP support
 * @ingroup networking
 * @{
 */

#include <zephyr/net/ptp_time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTP_MAJOR_VERSION 2 /**< Major PTP Version */
#define PTP_MINOR_VERSION 1 /**< Minor PTP Version */

#define PTP_VERSION (PTP_MINOR_VERSION << 4 | PTP_MAJOR_VERSION) /**< PTP version IEEE-1588:2019 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_PTP_H_ */
