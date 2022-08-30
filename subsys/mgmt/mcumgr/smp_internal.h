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

struct smp_transport;
struct zephyr_smp_transport;
struct net_buf;

/**
 * @brief Enqueues an incoming SMP request packet for processing.
 *
 * This function always consumes the supplied net_buf.
 *
 * @param smtp                  The transport to use to send the corresponding
 *                                  response(s).
 * @param nb                    The request packet to process.
 */
void smp_rx_req(struct smp_transport *smtp, struct net_buf *nb);

__deprecated static inline
void zephyr_smp_rx_req(struct zephyr_smp_transport *smpt, struct net_buf *nb)
{
	smp_rx_req((struct smp_transport *)smpt, nb);
}

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
void *smp_alloc_rsp(const void *req, void *arg);

__deprecated static inline
void *zephyr_smp_alloc_rsp(const void *req, void *arg)
{
	return smp_alloc_rsp(req, arg);
}


/**
 * @brief Frees an allocated buffer.
 *
 * @param buf		The buffer to free.
 * @param arg		The streamer providing the callback.
 */
void smp_free_buf(void *buf, void *arg);

__deprecated static inline
void zephyr_smp_free_buf(void *buf, void *arg)
{
	smp_free_buf(buf, arg);
}

#ifdef __cplusplus
}
#endif

#endif /* MGMT_MCUMGR_SMP_INTERNAL_H_ */
