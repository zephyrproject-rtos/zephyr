/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDHOC over CoAP transport implementation (RFC 9528 Appendix A.2)
 *
 * Implements EDHOC message transfer over CoAP as specified in RFC 9528
 * Appendix A.2 ("Transferring EDHOC over CoAP").
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/coap.h>
#include <zephyr/net/coap/coap_edhoc_transport.h>
#include <zephyr/net/coap/coap_service.h>
#include "coap_edhoc.h"
#include "coap_edhoc_session.h"
#include "coap_oscore_ctx_cache.h"

#include <errno.h>
#include <string.h>

/* Content-Format values from RFC 9528 */
#define COAP_CONTENT_FORMAT_EDHOC_CBOR_SEQ       64  /* application/edhoc+cbor-seq */
#define COAP_CONTENT_FORMAT_CID_EDHOC_CBOR_SEQ   65  /* application/cid-edhoc+cbor-seq */

/* Forward declarations for weak wrappers */
extern int coap_edhoc_msg2_gen_wrapper(void *resp_ctx,
				       void *runtime_ctx,
				       const uint8_t *msg1,
				       size_t msg1_len,
				       uint8_t *msg2,
				       size_t *msg2_len,
				       uint8_t *c_r,
				       size_t *c_r_len);

extern int coap_edhoc_msg3_process_wrapper(const uint8_t *edhoc_msg3,
					   size_t edhoc_msg3_len,
					   void *resp_ctx,
					   void *runtime_ctx,
					   void *cred_i_array,
					   uint8_t *prk_out,
					   size_t *prk_out_len,
					   uint8_t *initiator_pk,
					   size_t *initiator_pk_len,
					   uint8_t *c_i,
					   size_t *c_i_len);

extern int coap_edhoc_msg4_gen_wrapper(void *resp_ctx,
				       void *runtime_ctx,
				       uint8_t *msg4,
				       size_t *msg4_len,
				       bool *msg4_required);

extern int coap_edhoc_exporter_wrapper(const uint8_t *prk_out,
					size_t prk_out_len,
					int app_hash_alg,
					uint8_t label,
					uint8_t *output,
					size_t *output_len);

extern int coap_oscore_context_init_wrapper(void *ctx,
					     const uint8_t *master_secret,
					     size_t master_secret_len,
					     const uint8_t *master_salt,
					     size_t master_salt_len,
					     const uint8_t *sender_id,
					     size_t sender_id_len,
					     const uint8_t *recipient_id,
					     size_t recipient_id_len,
					     int aead_alg,
					     int hkdf_alg);

/* Forward declaration */
#if defined(CONFIG_ZTEST)
/* Make function non-static for testing */
int coap_edhoc_transport_validate_content_format(const struct coap_packet *request);
#else
static int coap_edhoc_transport_validate_content_format(const struct coap_packet *request);
#endif

/**
 * @brief Parse CBOR connection identifier per RFC 9528 Section 3.3.2
 *
 * RFC 9528 Section 3.3.2: One-byte CBOR-encoded integers MUST be used
 * to represent byte strings that coincide with those encodings.
 *
 * @param payload Payload buffer
 * @param payload_len Length of payload
 * @param c_id Output buffer for connection identifier
 * @param c_id_len Output: length of connection identifier
 * @param consumed Output: number of bytes consumed from payload
 * @return 0 on success, negative errno on error
 */
static int parse_connection_identifier(const uint8_t *payload,
					size_t payload_len,
					uint8_t *c_id,
					size_t *c_id_len,
					size_t *consumed)
{
	if (payload == NULL || c_id == NULL || c_id_len == NULL || consumed == NULL) {
		return -EINVAL;
	}

	if (payload_len == 0) {
		return -EINVAL;
	}

	uint8_t initial_byte = payload[0];
	uint8_t major_type = (initial_byte >> 5) & 0x07;
	uint8_t additional_info = initial_byte & 0x1f;

	/* RFC 9528 Section 3.3.2: Check for one-byte CBOR integer encoding */
	if (major_type == 0 && additional_info <= 0x17) {
		/* Major type 0 (unsigned integer), value 0-23 */
		c_id[0] = initial_byte;
		*c_id_len = 1;
		*consumed = 1;
		return 0;
	} else if (major_type == 1 && additional_info <= 0x17) {
		/* Major type 1 (negative integer), value -1 to -24 */
		c_id[0] = initial_byte;
		*c_id_len = 1;
		*consumed = 1;
		return 0;
	}

	/* Otherwise, parse as CBOR byte string (major type 2) */
	if (major_type != 2) {
		LOG_ERR("Connection identifier must be CBOR integer or bstr, got major type %d",
			major_type);
		return -EINVAL;
	}

	/* Parse byte string length */
	size_t header_len;
	size_t data_len;

	if (additional_info < 24) {
		header_len = 1;
		data_len = additional_info;
	} else if (additional_info == 24) {
		if (payload_len < 2) {
			return -EINVAL;
		}
		header_len = 2;
		data_len = payload[1];
	} else if (additional_info == 25) {
		if (payload_len < 3) {
			return -EINVAL;
		}
		header_len = 3;
		data_len = ((uint16_t)payload[1] << 8) | payload[2];
	} else if (additional_info == 26) {
		if (payload_len < 5) {
			return -EINVAL;
		}
		header_len = 5;
		data_len = ((uint32_t)payload[1] << 24) |
			   ((uint32_t)payload[2] << 16) |
			   ((uint32_t)payload[3] << 8) |
			   payload[4];
	} else {
		LOG_ERR("Invalid CBOR additional info: %d", additional_info);
		return -EINVAL;
	}

	/* Check if the entire byte string fits in the payload */
	if (header_len + data_len > payload_len) {
		LOG_ERR("Connection identifier length (%zu) exceeds payload (%zu)",
			header_len + data_len, payload_len);
		return -EINVAL;
	}

	/* Check if connection identifier is too long */
	if (data_len > 16) {
		LOG_ERR("Connection identifier too long (%zu bytes)", data_len);
		return -EINVAL;
	}

	/* Copy the byte string data (without CBOR header) */
	memcpy(c_id, payload + header_len, data_len);
	*c_id_len = data_len;
	*consumed = header_len + data_len;

	return 0;
}

/**
 * @brief Send EDHOC error response per RFC 9528 Appendix A.2.3
 *
 * RFC 9528 Appendix A.2.3: EDHOC errors over CoAP MUST be carried in payload;
 * response MUST have Content-Format application/edhoc+cbor-seq (64);
 * response codes recommended 4.00 or 5.00.
 *
 * @param service CoAP service
 * @param request Original request packet
 * @param err_code EDHOC error code
 * @param diag_msg Diagnostic message
 * @param coap_code CoAP response code (4.00 or 5.00)
 * @param client_addr Client address
 * @param client_addr_len Client address length
 * @return 0 on success, negative errno on error
 */
static int send_edhoc_error_response(const struct coap_service *service,
				      const struct coap_packet *request,
				      int err_code,
				      const char *diag_msg,
				      uint8_t coap_code,
				      const struct net_sockaddr *client_addr,
				      net_socklen_t client_addr_len)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
		       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;
	uint8_t error_payload[256];
	size_t error_len = sizeof(error_payload);
	int ret;

	/* Encode EDHOC error message as CBOR Sequence */
	ret = coap_edhoc_encode_error(err_code, diag_msg, error_payload, &error_len);
	if (ret < 0) {
		LOG_ERR("Failed to encode EDHOC error (%d)", ret);
		return ret;
	}

	/* Initialize CoAP response */
	ret = coap_packet_init(&response, buf, sizeof(buf),
			       COAP_VERSION_1, type, tkl, token, coap_code, id);
	if (ret < 0) {
		LOG_ERR("Failed to init EDHOC error response (%d)", ret);
		return ret;
	}

	/* RFC 9528 Appendix A.2.3: Content-Format MUST be 64 */
	ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_EDHOC_CBOR_SEQ);
	if (ret < 0) {
		LOG_ERR("Failed to add Content-Format to EDHOC error response (%d)", ret);
		return ret;
	}

	/* Add error payload */
	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		LOG_ERR("Failed to add payload marker to EDHOC error response (%d)", ret);
		return ret;
	}

	ret = coap_packet_append_payload(&response, error_payload, error_len);
	if (ret < 0) {
		LOG_ERR("Failed to add payload to EDHOC error response (%d)", ret);
		return ret;
	}

	return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
}

/**
 * @brief Process EDHOC message_1 and generate message_2
 *
 * Per RFC 9528 Appendix A.2.1, message_1 is prepended with CBOR true (0xF5).
 *
 * @param service CoAP service
 * @param request CoAP request packet
 * @param payload Request payload
 * @param payload_len Request payload length
 * @param client_addr Client address
 * @param client_addr_len Client address length
 * @return 0 on success, negative errno on error
 */
static int process_edhoc_message_1(const struct coap_service *service,
				    const struct coap_packet *request,
				    const uint8_t *payload,
				    size_t payload_len,
				    const struct net_sockaddr *client_addr,
				    net_socklen_t client_addr_len)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
		       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;
	uint8_t msg2_buf[256];
	size_t msg2_len = sizeof(msg2_buf);
	uint8_t c_r[16];
	size_t c_r_len = sizeof(c_r);
	int ret;

	/* RFC 9528 Appendix A.2.1: Verify payload starts with CBOR true (0xF5) */
	if (payload_len < 1 || payload[0] != 0xF5) {
		LOG_ERR("EDHOC message_1 must be prepended with CBOR true (0xF5)");
		return send_edhoc_error_response(service, request, 1,
						 "Invalid message_1 prefix",
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* Extract message_1 (skip the 0xF5 prefix) */
	const uint8_t *msg1 = payload + 1;
	size_t msg1_len = payload_len - 1;

	if (msg1_len == 0) {
		LOG_ERR("EDHOC message_1 is empty");
		return send_edhoc_error_response(service, request, 1,
						 "Empty message_1",
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* Call wrapper to generate message_2 */
	ret = coap_edhoc_msg2_gen_wrapper(NULL, NULL, msg1, msg1_len,
					  msg2_buf, &msg2_len, c_r, &c_r_len);
	if (ret < 0) {
		LOG_ERR("Failed to generate EDHOC message_2 (%d)", ret);
		return send_edhoc_error_response(service, request, 1,
						 "Failed to process message_1",
						 COAP_RESPONSE_CODE_INTERNAL_ERROR,
						 client_addr, client_addr_len);
	}

	/* Store EDHOC session keyed by C_R */
	struct coap_edhoc_session *session;
	session = coap_edhoc_session_insert(service->data->edhoc_session_cache,
					    CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
					    c_r, c_r_len);
	if (session == NULL) {
		LOG_ERR("Failed to insert EDHOC session");
		return send_edhoc_error_response(service, request, 1,
						 "Session cache full",
						 COAP_RESPONSE_CODE_INTERNAL_ERROR,
						 client_addr, client_addr_len);
	}

	/* Initialize CoAP response: 2.04 Changed */
	ret = coap_packet_init(&response, buf, sizeof(buf),
			       COAP_VERSION_1, type, tkl, token,
			       COAP_RESPONSE_CODE_CHANGED, id);
	if (ret < 0) {
		LOG_ERR("Failed to init EDHOC message_2 response (%d)", ret);
		return ret;
	}

	/* RFC 9528 Appendix A.2: Content-Format 64 (application/edhoc+cbor-seq) */
	ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_EDHOC_CBOR_SEQ);
	if (ret < 0) {
		LOG_ERR("Failed to add Content-Format to message_2 response (%d)", ret);
		return ret;
	}

	/* Add message_2 payload */
	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		LOG_ERR("Failed to add payload marker to message_2 response (%d)", ret);
		return ret;
	}

	ret = coap_packet_append_payload(&response, msg2_buf, msg2_len);
	if (ret < 0) {
		LOG_ERR("Failed to add payload to message_2 response (%d)", ret);
		return ret;
	}

	LOG_DBG("Sending EDHOC message_2 (%zu bytes)", msg2_len);
	return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
}

/**
 * @brief Process EDHOC message_3 and optionally generate message_4
 *
 * Per RFC 9528 Appendix A.2.1, message_3 is prepended with C_R.
 * After successful processing, derives OSCORE context per RFC 9528 Appendix A.1.
 *
 * @param service CoAP service
 * @param request CoAP request packet
 * @param payload Request payload
 * @param payload_len Request payload length
 * @param client_addr Client address
 * @param client_addr_len Client address length
 * @return 0 on success, negative errno on error
 */
static int process_edhoc_message_3(const struct coap_service *service,
				    const struct coap_packet *request,
				    const uint8_t *payload,
				    size_t payload_len,
				    const struct net_sockaddr *client_addr,
				    net_socklen_t client_addr_len)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint16_t id = coap_header_get_id(request);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
		       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;
	uint8_t c_r[16];
	size_t c_r_len;
	size_t consumed;
	uint8_t c_i[16];
	size_t c_i_len = sizeof(c_i);
	uint8_t prk_out[64];
	size_t prk_out_len = sizeof(prk_out);
	uint8_t initiator_pk[64];
	size_t initiator_pk_len = sizeof(initiator_pk);
	uint8_t msg4_buf[256];
	size_t msg4_len = sizeof(msg4_buf);
	bool msg4_required = false;
	int ret;

	/* Parse C_R from payload */
	ret = parse_connection_identifier(payload, payload_len, c_r, &c_r_len, &consumed);
	if (ret < 0) {
		LOG_ERR("Failed to parse C_R from message_3 (%d)", ret);
		return send_edhoc_error_response(service, request, 1,
						 "Invalid C_R",
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* Extract message_3 (after C_R) */
	const uint8_t *msg3 = payload + consumed;
	size_t msg3_len = payload_len - consumed;

	if (msg3_len == 0) {
		LOG_ERR("EDHOC message_3 is empty");
		return send_edhoc_error_response(service, request, 1,
						 "Empty message_3",
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* Look up EDHOC session by C_R */
	struct coap_edhoc_session *session;
	session = coap_edhoc_session_find(service->data->edhoc_session_cache,
					  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
					  c_r, c_r_len);
	if (session == NULL) {
		LOG_ERR("EDHOC session not found for C_R");
		return send_edhoc_error_response(service, request, 1,
						 "Session not found",
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* Process message_3 and derive PRK_out */
	ret = coap_edhoc_msg3_process_wrapper(msg3, msg3_len,
					      session->resp_ctx,
					      session->runtime_ctx,
					      NULL, /* cred_i_array */
					      prk_out, &prk_out_len,
					      initiator_pk, &initiator_pk_len,
					      c_i, &c_i_len);
	if (ret < 0) {
		LOG_ERR("Failed to process EDHOC message_3 (%d)", ret);
		return send_edhoc_error_response(service, request, 1,
						 "Failed to process message_3",
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* Store C_I in session for OSCORE ID mapping */
	ret = coap_edhoc_session_set_ci(session, c_i, c_i_len);
	if (ret < 0) {
		LOG_ERR("Failed to set C_I in session (%d)", ret);
	}

	/* Generate message_4 if required */
	ret = coap_edhoc_msg4_gen_wrapper(session->resp_ctx,
					  session->runtime_ctx,
					  msg4_buf, &msg4_len,
					  &msg4_required);
	if (ret < 0) {
		LOG_ERR("Failed to generate EDHOC message_4 (%d)", ret);
		return send_edhoc_error_response(service, request, 1,
						 "Failed to generate message_4",
						 COAP_RESPONSE_CODE_INTERNAL_ERROR,
						 client_addr, client_addr_len);
	}

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
	/* RFC 9528 Appendix A.1: Derive OSCORE master secret and salt */
	uint8_t master_secret[32];
	size_t master_secret_len = sizeof(master_secret);
	uint8_t master_salt[16];
	size_t master_salt_len = sizeof(master_salt);

	/* Derive master secret (label 0) */
	ret = coap_edhoc_exporter_wrapper(prk_out, prk_out_len,
					  0, /* app_hash_alg - use default */
					  0, /* label 0 for master secret */
					  master_secret, &master_secret_len);
	if (ret < 0) {
		LOG_ERR("Failed to derive OSCORE master secret (%d)", ret);
		goto send_response;
	}

	/* Derive master salt (label 1) */
	ret = coap_edhoc_exporter_wrapper(prk_out, prk_out_len,
					  0, /* app_hash_alg - use default */
					  1, /* label 1 for master salt */
					  master_salt, &master_salt_len);
	if (ret < 0) {
		LOG_ERR("Failed to derive OSCORE master salt (%d)", ret);
		goto send_response;
	}

	/* RFC 9528 Table 14: Responder OSCORE Sender ID = C_I, Recipient ID = C_R */
	struct coap_oscore_ctx_cache_entry *ctx_entry;
	ctx_entry = coap_oscore_ctx_cache_insert(service->data->oscore_ctx_cache,
						 CONFIG_COAP_OSCORE_CTX_CACHE_SIZE,
						 c_r, c_r_len);
	if (ctx_entry == NULL) {
		LOG_ERR("Failed to insert OSCORE context into cache");
		goto send_response;
	}

	/* Initialize OSCORE context */
	ret = coap_oscore_context_init_wrapper(ctx_entry->oscore_ctx,
					       master_secret, master_secret_len,
					       master_salt, master_salt_len,
					       c_i, c_i_len,  /* sender_id */
					       c_r, c_r_len,  /* recipient_id */
					       0, /* aead_alg - use default */
					       0  /* hkdf_alg - use default */);
	if (ret < 0) {
		LOG_ERR("Failed to initialize OSCORE context (%d)", ret);
		/* Remove the cache entry on failure */
		coap_oscore_ctx_cache_remove(service->data->oscore_ctx_cache,
					     CONFIG_COAP_OSCORE_CTX_CACHE_SIZE,
					     c_r, c_r_len);
	} else {
		LOG_DBG("Derived OSCORE context from EDHOC (kid_len=%zu)", c_r_len);
	}

	/* Zeroize sensitive material */
	memset(prk_out, 0, sizeof(prk_out));
	memset(master_secret, 0, sizeof(master_secret));
	memset(master_salt, 0, sizeof(master_salt));

send_response:
#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST */

	/* Initialize CoAP response: 2.04 Changed */
	ret = coap_packet_init(&response, buf, sizeof(buf),
			       COAP_VERSION_1, type, tkl, token,
			       COAP_RESPONSE_CODE_CHANGED, id);
	if (ret < 0) {
		LOG_ERR("Failed to init EDHOC message_4 response (%d)", ret);
		return ret;
	}

	/* RFC 9528 Appendix A.2: Content-Format 64 (application/edhoc+cbor-seq) */
	ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_EDHOC_CBOR_SEQ);
	if (ret < 0) {
		LOG_ERR("Failed to add Content-Format to message_4 response (%d)", ret);
		return ret;
	}

	/* Add message_4 payload if required */
	if (msg4_required && msg4_len > 0) {
		ret = coap_packet_append_payload_marker(&response);
		if (ret < 0) {
			LOG_ERR("Failed to add payload marker to message_4 response (%d)", ret);
			return ret;
		}

		ret = coap_packet_append_payload(&response, msg4_buf, msg4_len);
		if (ret < 0) {
			LOG_ERR("Failed to add payload to message_4 response (%d)", ret);
			return ret;
		}
		LOG_DBG("Sending EDHOC message_4 (%zu bytes)", msg4_len);
	} else {
		LOG_DBG("Sending EDHOC response without message_4");
	}

	/* Remove EDHOC session after successful completion */
	coap_edhoc_session_remove(service->data->edhoc_session_cache,
				  CONFIG_COAP_EDHOC_SESSION_CACHE_SIZE,
				  c_r, c_r_len);

	return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
}

/**
 * @brief Handle EDHOC-over-CoAP request to /.well-known/edhoc
 *
 * Per RFC 9528 Appendix A.2, EDHOC messages are transferred via POST
 * requests to /.well-known/edhoc.
 *
 * @param service CoAP service
 * @param request CoAP request packet
 * @param client_addr Client address
 * @param client_addr_len Client address length
 * @return 0 on success, negative errno on error
 */
int coap_edhoc_transport_handle_request(const struct coap_service *service,
					 const struct coap_packet *request,
					 const struct net_sockaddr *client_addr,
					 net_socklen_t client_addr_len)
{
	uint8_t code = coap_header_get_code(request);
	const uint8_t *payload;
	uint16_t payload_len;
	int ret;

	/* RFC 9528 Appendix A.2: Only POST method is allowed */
	if (code != COAP_METHOD_POST) {
		LOG_WRN("EDHOC endpoint only accepts POST, got method %d", code);
		/* Send 4.05 Method Not Allowed */
		uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		struct coap_packet response;
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t tkl = coap_header_get_token(request, token);
		uint16_t id = coap_header_get_id(request);
		uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON)
			       ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;

		ret = coap_packet_init(&response, buf, sizeof(buf),
				       COAP_VERSION_1, type, tkl, token,
				       COAP_RESPONSE_CODE_NOT_ALLOWED, id);
		if (ret < 0) {
			return ret;
		}

		return coap_service_send(service, &response, client_addr, client_addr_len, NULL);
	}

	/* Get payload */
	payload = coap_packet_get_payload(request, &payload_len);
	if (payload == NULL || payload_len == 0) {
		LOG_ERR("EDHOC request has no payload");
		return send_edhoc_error_response(service, request, 1,
						 "Empty payload",
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* RFC 9528 Appendix A.2: Validate Content-Format option */
	ret = coap_edhoc_transport_validate_content_format(request);
	if (ret < 0) {
		const char *error_msg;
		
		switch (ret) {
		case -ENOENT:
			error_msg = "Missing Content-Format";
			LOG_ERR("EDHOC request missing required Content-Format option");
			break;
		case -EMSGSIZE:
			error_msg = "Duplicate Content-Format";
			LOG_ERR("EDHOC request has duplicate Content-Format options");
			break;
		case -EBADMSG:
			error_msg = "Invalid Content-Format";
			LOG_ERR("Invalid Content-Format for EDHOC request (expected %d)",
				COAP_CONTENT_FORMAT_CID_EDHOC_CBOR_SEQ);
			break;
		default:
			error_msg = "Malformed Content-Format";
			LOG_ERR("Failed to parse Content-Format option (%d)", ret);
			break;
		}
		
		return send_edhoc_error_response(service, request, 1, error_msg,
						 COAP_RESPONSE_CODE_BAD_REQUEST,
						 client_addr, client_addr_len);
	}

	/* Determine message type by inspecting payload prefix */
	if (payload[0] == 0xF5) {
		/* CBOR true (0xF5) indicates message_1 */
		return process_edhoc_message_1(service, request, payload, payload_len,
					       client_addr, client_addr_len);
	} else {
		/* Otherwise, assume message_3 (prepended with C_R) */
		return process_edhoc_message_3(service, request, payload, payload_len,
					       client_addr, client_addr_len);
	}
}

/**
 * @brief Validate EDHOC request Content-Format per RFC 9528 Appendix A.2
 *
 * RFC 9528 Appendix A.2: Client requests MUST use Content-Format 65
 * (application/cid-edhoc+cbor-seq) for messages with prepended indicators.
 *
 * @param request CoAP request packet
 * @return 0 on success, -ENOENT if missing, -EMSGSIZE if duplicate,
 *         -EBADMSG if wrong value, negative errno on other errors
 */
#if defined(CONFIG_ZTEST)
int coap_edhoc_transport_validate_content_format(const struct coap_packet *request)
#else
static int coap_edhoc_transport_validate_content_format(const struct coap_packet *request)
#endif
{
	struct coap_option opt[2];
	int opt_count;
	int ret;
	uint16_t content_format;

	if (request == NULL) {
		return -EINVAL;
	}

	/* Check for Content-Format option */
	opt_count = coap_find_options(request, COAP_OPTION_CONTENT_FORMAT, opt, 2);
	if (opt_count < 0) {
		return -EINVAL;
	}

	/* Content-Format MUST be present */
	if (opt_count == 0) {
		return -ENOENT;
	}

	/* Reject duplicate Content-Format options */
	if (opt_count > 1) {
		return -EMSGSIZE;
	}

	/* Extract and validate Content-Format value */
	ret = coap_get_option_int(request, COAP_OPTION_CONTENT_FORMAT);
	if (ret < 0) {
		return ret;
	}
	content_format = (uint16_t)ret;

	/* Client requests MUST use Content-Format 65 */
	if (content_format != COAP_CONTENT_FORMAT_CID_EDHOC_CBOR_SEQ) {
		return -EBADMSG;
	}

	return 0;
}
