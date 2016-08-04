/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NATS_CLIENT_H_
#define _NATS_CLIENT_H_

#include "netz.h"
#include "nats_pack.h"

/**
 * @brief nats_clapp_ctx_t	NATS Client Application Context structure
 */
struct nats_clapp_ctx_t {
	struct nats_cl_ctx_t *nats;
	struct netz_ctx_t *netz_ctx;
	struct app_buf_t *tx_buf;
	struct app_buf_t *rx_buf;
};

/**
 * @brief NATS_CLAPP_INIT	Context initializer
 */
#define NATS_CLAPP_INIT(cl, netz, tx, rx) { .nats = cl,			\
					    .netz_ctx = netz,		\
					    .tx_buf = tx,		\
					    .rx_buf = rx}

/**
 * @brief nats_connect		Connects to a NATS server
 * @param [in] ctx		NATS Client Application Context structure
 * @param [in] client_name	Client idientifier
 * @param [in] verbose		Verbose mode
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 */
int nats_connect(struct nats_clapp_ctx_t *ctx, char *client_name, int verbose);

/**
 * @brief nats_pub		Sends the NATS PUB message
 * @param [in] ctx		NATS Client Application Context structure
 * @param [in] subject		Message subject
 * @param [in] reply_to		Reply-to field
 * @param [in] payload		Message payload
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 */
int nats_pub(struct nats_clapp_ctx_t *ctx, char *subject, char *reply_to,
	     char *payload);

/**
 * @brief nats_sub		Sends the NATS SUB message
 * @param [in] ctx		NATS Client Application Context structure
 * @param [in] subject		Subscription subject
 * @param [in] queue_grp	Queue group
 * @param [in] sid		Subscription Identifier
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 */
int nats_sub(struct nats_clapp_ctx_t *ctx, char *subject, char *queue_grp,
	     char *sid);

/**
 * @brief nats_unsub		Send the NATS UNSUB message
 * @param [in] ctx		NATS Client Application Context structure
 * @param [in] sid		Subscription Identifier
 * @param [in] max_msgs		Max messages field
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 */
int nats_unsub(struct nats_clapp_ctx_t *ctx, char *sid, int max_msgs);

/**
 * @brief nats_read_ok		Reads the +OK message
 * @param [in] ctx		NATS Client Application Context structure
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 */
int nats_read_ok(struct nats_clapp_ctx_t *ctx);

/**
 * @brief nats_ping_pong	Sends a NATS PING msg and waits for
 *				a NATS PONG msg from the NATS server
 * @param [in] ctx		NATS Client Application Context structure
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was
 *				passed as argument or received from the
 *				server.
 */
int nats_ping_pong(struct nats_clapp_ctx_t *ctx);

#endif
