/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_CLIENT_
#define H_SMP_CLIENT_

#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

/**
 * @brief MCUmgr SMP client API
 * @defgroup mcumgr_smp_client SMP client API
 * @ingroup mcumgr
 * @{
 */

/**
 * @brief SMP client object
 */
struct smp_client_object {
	/** Must be the first member. */
	struct k_work work;
	/** FIFO for client TX queue */
	struct k_fifo tx_fifo;
	/** SMP transport object */
	struct smp_transport *smpt;
	/** SMP SEQ */
	uint8_t smp_seq;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a SMP client object.
 *
 * @param smp_client	The Client to construct.
 * @param smp_type	SMP transport type for discovering transport object
 *
 * @return	0 if successful
 * @return	mcumgr_err_t code on failure
 */
int smp_client_object_init(struct smp_client_object *smp_client, int smp_type);

/**
 * @brief Response callback for SMP send.
 *
 * @param nb net_buf for response
 * @param user_data same user data that was provided as part of the request
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
typedef int (*smp_client_res_fn)(struct net_buf *nb, void *user_data);

/**
 * @brief SMP client response handler.
 *
 * @param nb response net_buf
 * @param res_hdr Parsed SMP header
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
int smp_client_single_response(struct net_buf *nb, const struct smp_hdr *res_hdr);

/**
 * @brief Allocate buffer and initialize with SMP header.
 *
 * @param smp_client SMP client object
 * @param group SMP group id
 * @param command_id SMP command id
 * @param op SMP operation type
 * @param version SMP MCUmgr version
 *
 * @return      A newly-allocated buffer net_buf on success
 * @return	NULL on failure.
 */
struct net_buf *smp_client_buf_allocation(struct smp_client_object *smp_client, uint16_t group,
					  uint8_t command_id, uint8_t op,
					  enum smp_mcumgr_version_t version);
/**
 * @brief Free a SMP client buffer.
 *
 * @param nb    The net_buf to free.
 */
void smp_client_buf_free(struct net_buf *nb);

/**
 * @brief SMP client data send request.
 *
 * @param smp_client SMP client object
 * @param nb net_buf packet for send
 * @param cb Callback for response handler
 * @param user_data user defined data pointer which will be returned back to response callback
 * @param timeout_in_sec	Timeout in seconds for send process. Client will retry transport
 *				based CONFIG_SMP_CMD_RETRY_TIME
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
int smp_client_send_cmd(struct smp_client_object *smp_client, struct net_buf *nb,
			smp_client_res_fn cb, void *user_data, int timeout_in_sec);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* H_SMP_CLIENT_ */
