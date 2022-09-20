/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BUF_H_
#define ZEPHYR_INCLUDE_MGMT_BUF_H_

#include <inttypes.h>
#include <zcbor_common.h>
#include <zcbor_encode.h>
struct net_buf;

struct cbor_nb_reader {
	struct net_buf *nb;
	/* CONFIG_MGMT_MAX_DECODING_LEVELS + 2 translates to minimal
	 * zcbor backup states.
	 */
	zcbor_state_t zs[CONFIG_MGMT_MAX_DECODING_LEVELS + 2];
};

struct cbor_nb_writer {
	struct net_buf *nb;
	zcbor_state_t zs[2];
};

/**
 * @brief Allocates a net_buf for holding an mcumgr request or response.
 *
 * @return                      A newly-allocated buffer net_buf on success;
 *                              NULL on failure.
 */
struct net_buf *mcumgr_buf_alloc(void);

/**
 * @brief Frees an mcumgr net_buf
 *
 * @param nb                    The net_buf to free.
 */
void mcumgr_buf_free(struct net_buf *nb);

#endif
