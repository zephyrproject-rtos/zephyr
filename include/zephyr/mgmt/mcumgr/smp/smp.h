/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SMP - Simple Management Protocol.
 *
 * SMP is a basic protocol that sits on top of the mgmt layer.  SMP requests
 * and responses have the following format:
 *
 *	 [Offset 0]: Mgmt header
 *	 [Offset 8]: CBOR map of command-specific key-value pairs.
 *
 * SMP request packets may contain multiple concatenated requests.  Each
 * request must start at an offset that is a multiple of 4, so padding should
 * be inserted between requests as necessary.  Requests are processed
 * sequentially from the start of the packet to the end.  Each response is sent
 * individually in its own packet.  If a request elicits an error response,
 * processing of the packet is aborted.
 */

#ifndef H_SMP_
#define H_SMP_

#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

#include <zcbor_common.h>


#ifdef __cplusplus
extern "C" {
#endif

struct cbor_nb_reader {
	struct net_buf *nb;
	/* CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2 translates to minimal
	 * zcbor backup states.
	 */
	zcbor_state_t zs[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
};

struct cbor_nb_writer {
	struct net_buf *nb;
	zcbor_state_t zs[2];
};

/**
 * @brief Allocates a net_buf for holding an mcumgr request or response.
 *
 * @return      A newly-allocated buffer net_buf on success;
 *              NULL on failure.
 */
struct net_buf *smp_packet_alloc(void);

/**
 * @brief Frees an mcumgr net_buf
 *
 * @param nb    The net_buf to free.
 */
void smp_packet_free(struct net_buf *nb);

/**
 * @brief Decodes, encodes, and transmits SMP packets.
 */
struct smp_streamer {
	struct smp_transport *smpt;
	struct cbor_nb_reader *reader;
	struct cbor_nb_writer *writer;

#ifdef CONFIG_MCUMGR_SMP_VERBOSE_ERR_RESPONSE
	const char *rc_rsn;
#endif
};

/**
 * @brief Processes a single SMP request packet and sends all corresponding responses.
 *
 * Processes all SMP requests in an incoming packet.  Requests are processed
 * sequentially from the start of the packet to the end.  Each response is sent
 * individually in its own packet.  If a request elicits an error response,
 * processing of the packet is aborted.  This function consumes the supplied
 * request buffer regardless of the outcome.
 *
 * @param streamer	The streamer providing the required SMP callbacks.
 * @param req		The request packet to process.
 *
 * @return 0 on success, #mcumgr_err_t code on failure.
 */
int smp_process_request_packet(struct smp_streamer *streamer, void *req);

#ifdef __cplusplus
}
#endif

#endif /* H_SMP_ */
