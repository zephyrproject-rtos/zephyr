/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MGMT_MCUMGR_SMP_INTERNAL_H_
#define MGMT_MCUMGR_SMP_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

struct zephyr_smp_transport;
struct net_buf;

/**
 * @brief Enqueues an incoming SMP request packet for processing.
 *
 * This function always consumes the supplied net_buf.
 *
 * @param zst                   The transport to use to send the corresponding
 *                                  response(s).
 * @param nb                    The request packet to process.
 */
void zephyr_smp_rx_req(struct zephyr_smp_transport *zst, struct net_buf *nb);

/**
 * @brief Allocates a response buffer.
 *
 * If a source buf is provided, its user data is copied into the new buffer.
 *
 * @param req		An optional source buffer to copy user data from.
 * @param arg		The streamer providing the callback.
 *
 * @return	Newly-allocated buffer on success
 *		NULL on failure.
 */
void *zephyr_smp_alloc_rsp(const void *req, void *arg);

/**
 * @brief Frees an allocated buffer.
 *
 * @param buf		The buffer to free.
 * @param arg		The streamer providing the callback.
 */
void zephyr_smp_free_buf(void *buf, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* MGMT_MCUMGR_SMP_INTERNAL_H_ */
