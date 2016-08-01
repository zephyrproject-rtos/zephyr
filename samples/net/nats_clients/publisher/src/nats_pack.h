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

#ifndef _NATS_PKT_H_
#define _NATS_PKT_H_

#include "app_buf.h"

#include <stdint.h>
#include <stddef.h>

/**
 * @brief nats_msg_type		NATS message type enum.
 *
 * @details			This sort is arbitrary and is not part of the
 *				standard.
 */
enum nats_msg_type {
	NATS_MSG_INFO = 1,
	NATS_MSG_CONNECT,
	NATS_MSG_PUB,
	NATS_MSG_SUB,
	NATS_MSG_UNSUB,
	NATS_MSG_MSG,
	NATS_MSG_PING,
	NATS_MSG_PONG,
	NATS_MSG_OK,
	NATS_MSG_ERR
};

/**
 * @brief nats_cl_ctx_t		NATS client context structure
 */
struct nats_cl_ctx_t {
	int verbose;
	int pedantic;
	int ssl_req;
	char *user;
	char *pass;
	char *name;
};

/**
 * @brief NATS_CL_INIT		NATS client context initializer
 */
#define NATS_CL_INIT {	.verbose = 1, .pedantic = 0, .ssl_req = 0,	\
			.user = NULL, .pass = NULL, .name = NULL }

/**
 * @brief nats_pack_info	Packs the NATS INFO message
 * @param [out] buf		app_buf used to store the resultant string
 * @param [in] server_id	NATS server identifier, may be NULL
 * @param [in] version		NATS server software version, may be NULL
 * @param [in] go		golang version used to build the NATS server,
 *				may be NULL
 * @param [in] host		NATS server IP address, may be NULL
 * @param [in] port		NATS server port number, may be NULL
 * @param [in] auth_req		If 0 no client authentication required
 * @param [in] ssl_req		If 0 no ssl channel required
 * @param [in] max_payload	Maximum payload size the server will accept
 *				from the client
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_info(struct app_buf_t *buf, char *server_id, char *version,
		  char *go, char *host, int port, int auth_req, int ssl_req,
		  int max_payload);


/**
 * @brief nats_pack_connect	Packs the NATS CONNECT message
 * @param [out] buf		app_buf used to store the resultant string
 * @param [in] verbose		Turns +OK protocol ACK
 * @param [in] pedantic		Turns on strict checking
 * @param [in] ssl_req		Indicates if the user requires a SSL connection
 * @param [in] auth_token	Client auth token, may be NULL
 * @param [in] user		Connection username, may be NULL
 * @param [in] pass		Connection password, may be NULL
 * @param [in] name		Optional client name, may be NULL
 * @param [in] lang		Implementation language of the client,
 *				may be NULL
 * @param [in] version		Client's version, may be NULL
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_connect(struct app_buf_t *buf, int verbose, int pedantic,
		     int ssl_req, char *auth_token, char *user, char *pass,
		     char *name, char *lang, char *version);


/**
 * @brief nats_pack_quickcon	Packs the NATS CONNECT message
 *
 * @details			This function calls nats_pack_connect with all
 *				the parameters set to 0 or NULL but name.
 *
 * @param [out] buf		app_buf used to store the resultant string
 * @param [in] name		Optional client name, may be NULL
 * @param [in] verbose		0 for quiet, anything else for verbose
 *				communication
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_quickcon(struct app_buf_t *buf, char *name, int verbose);

/**
 * @brief nats_pack_pub		Packs the NATS PUB message
 * @param [out] buf		app_buf used to store the resultant string
 * @param [in] subject		Message subject
 * @param [in] reply_to		Replay inbox subject, may be NULL
 * @param payload		C-string, may be NULL
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_pub(struct app_buf_t *buf, char *subject, char *reply_to,
		 char *payload);

/**
 * @brief nats_pack_sub		Packs the NATS SUB message
 * @param [out] buf		app_buf used to store the resultant string
 * @param [in] subject		Subject name to subscribe to
 * @param [in] queue_grp	Queue group to join to, may be NULL
 * @param [in] sid		Subscription Identifier
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_sub(struct app_buf_t *buf, char *subject, char *queue_grp,
		  char *sid);

/**
 * @brief nats_pack_unsub	Packs the NATS UNSUB message
 * @param buf			app_buf used to store the resultant string
 * @param sid			Subscription Identifier
 * @param max_msgs		Max number of messages
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_unsub(struct app_buf_t *buf, char *sid, int max_msgs);

/**
 * @brief nats_pack_msg		Packs the NATS MSG message
 * @param buf			app_buf used to store the resultant string
 * @param subject		Subject name to subscribe to
 * @param sid			Subscription Identifier
 * @param reply_to		Inbox subject for this subscription
 * @param payload		The message payload data
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_msg(struct app_buf_t *buf, char *subject, char *sid,
		 char *reply_to, char *payload);

/**
 * @brief nats_unpack_msg	Unpacks the NATS MSG message
 *
 * @details			This function unpacks a NATS MSG message.
 *				Output parameters indicate where the field
 *				was found inside the MSG and the length of
 *				the field.
 *				If the reply-to field was found, reply_start
 *				will be greater than 0, otherwise will be set
 *				to -1.
 *
 * @param [in] buf		Buffer containing a C-string with the
 *				NATS MSG message
 * @param [out] subject_start	Indicates where the subject field starts
 * @param [out] subject_len	Subject field length
 * @param [out] sid_start	Indicates where the sid field starts
 * @param [out] sid_len		sid field length
 * @param [out] reply_start	Indicates where the reply-to field starts
 * @param [out] reply_len	reply-to field length
 * @param [out] payload_start	Indicates where the payload starts
 * @param [out] payload_len	payload field length
 * @return			0 on success
 * @return			-EINVAL if a malformed MSG was received
 */
int nats_unpack_msg(struct app_buf_t *buf,
		    int *subject_start, int *subject_len,
		    int *sid_start, int *sid_len,
		    int *reply_start, int *reply_len,
		    int *payload_start, int *payload_len);

/**
 * @brief nats_unpack_info	Unpacks the NATS INFO message
 *
 * @details			This function unpacks de INFO message.
 *				INFO msg parser is WIP. So, support
 *				for this message is not yet finished.
 *				WARNING: Function signature may change.
 *
 * @param [in] buf		Buffer containing a C-string with the
 *				NATS INFO message
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_unpack_info(struct app_buf_t *buf);

/**
 * @brief nats_pack_ping	Packs the NATS PING message
 * @param [out] buf		app_buf used to store the resultant string
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_ping(struct app_buf_t *buf);

/**
 * @brief nats_unpack_ping	Unacks the NATS PING message
 * @param [in] buf		Buffer containing a C-string with the
 *				NATS PING message
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_unpack_ping(struct app_buf_t *buf);

/**
 * @brief nats_pack_ping	Packs the NATS PONG message
 * @param [out] buf		app_buf used to store the resultant string
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_pack_pong(struct app_buf_t *buf);

/**
 * @brief nats_unpack_pong	Unacks the NATS PONG message
 * @param [in] buf		Buffer containing a C-string with the
 *				NATS PONG message
 * @return			0 on success
 * @return			-EINVAL on error
 */
int nats_unpack_pong(struct app_buf_t *buf);

int nats_find_msg(struct app_buf_t *buf, char *str);

#endif
