/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I3C_COMMON_H_
#define ZEPHYR_MCTP_I3C_COMMON_H_

/** @cond INTERNAL_HIDDEN */

/* Max packet size */
#define MCTP_I3C_MAX_PKT_SIZE 255

/* DMTF mandatory byte with IBI signaling a pending Read */
#define MCTP_I3C_MDB_PENDING_READ 0xAE

/** @endcond INTERNAL_HIDDEN */

#endif /* ZEPHYR_MCTP_I3C_COMMON_H_ */
