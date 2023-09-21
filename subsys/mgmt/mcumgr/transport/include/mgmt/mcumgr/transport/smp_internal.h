/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MGMT_MCUMGR_SMP_INTERNAL_H_
#define MGMT_MCUMGR_SMP_INTERNAL_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct smp_hdr {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t  nh_op:3;		/* MGMT_OP_[...] */
	uint8_t  nh_version:2;
	uint8_t  _res1:3;
#else
	uint8_t  _res1:3;
	uint8_t  nh_version:2;
	uint8_t  nh_op:3;		/* MGMT_OP_[...] */
#endif
	uint8_t  nh_flags;		/* Reserved for future flags */
	uint16_t nh_len;		/* Length of the payload */
	uint16_t nh_group;		/* MGMT_GROUP_ID_[...] */
	uint8_t  nh_seq;		/* Sequence number */
	uint8_t  nh_id;			/* Message ID within group */
};

struct smp_transport;
struct zephyr_smp_transport;

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

#ifdef CONFIG_SMP_CLIENT
/**
 * @brief Trig SMP client request packet for transmission.
 *
 * @param work	The transport to use to send the corresponding response(s).
 */
void smp_tx_req(struct k_work *work);
#endif

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
