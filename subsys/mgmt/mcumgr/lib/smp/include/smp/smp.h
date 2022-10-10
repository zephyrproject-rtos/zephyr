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

#include "mgmt/mgmt.h"

#ifdef __cplusplus
extern "C" {
#endif

struct smp_streamer;
struct mgmt_hdr;

/**
 * @brief Decodes, encodes, and transmits SMP packets.
 */
struct smp_streamer {
	struct zephyr_smp_transport *smpt;
	struct cbor_nb_reader *reader;
	struct cbor_nb_writer *writer;
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
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int smp_process_request_packet(struct smp_streamer *streamer, void *req);

#ifdef __cplusplus
}
#endif

#endif /* H_SMP_ */
