/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/ice_lite.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/stun.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(net_ice_lite, CONFIG_ICE_LITE_LOG_LEVEL);

/* Error codes of RFC 8489 section 18.3 and RFC 8445 section 16.2. */
#define ERR_BAD_REQUEST       400U
#define ERR_UNAUTHENTICATED   401U
#define ERR_UNKNOWN_ATTRIBUTE 420U
#define ERR_ROLE_CONFLICT     487U

#define UNKNOWN_ATTRS_MAX 8U

/* Fill `out` with `len` random ice-chars (RFC 8445 section 5.3), 6 bits each. */
static int random_ice_chars(char *out, size_t len)
{
	static const char ice_chars[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int ret;

	ret = sys_csrand_get(out, len);
	if (ret != 0) {
		return -EIO;
	}

	for (size_t i = 0; i < len; i++) {
		out[i] = ice_chars[(uint8_t)out[i] & 0x3fU];
	}
	out[len] = '\0';

	return 0;
}

int ice_lite_init(struct ice_lite_agent *agent)
{
	int ret;

	if (agent == NULL) {
		return -EINVAL;
	}

	memset(agent, 0, sizeof(*agent));

	ret = random_ice_chars(agent->ufrag, ICE_LITE_UFRAG_LEN);
	if (ret == 0) {
		ret = random_ice_chars(agent->pwd, ICE_LITE_PWD_LEN);
	}
	if ((ret == 0) && (sys_csrand_get(&agent->tiebreaker, sizeof(agent->tiebreaker)) != 0)) {
		ret = -EIO;
	}
	if (ret != 0) {
		LOG_ERR("No random credentials (%d)", ret);
	}

	return ret;
}

/* USERNAME of a check is "<ufrag of the receiver>:<ufrag of the sender>". */
static bool username_ok(const struct ice_lite_agent *agent, const struct stun_msg *msg)
{
	return (msg->username_len > ICE_LITE_UFRAG_LEN + 1U) &&
	       (memcmp(msg->username, agent->ufrag, ICE_LITE_UFRAG_LEN) == 0) &&
	       (msg->username[ICE_LITE_UFRAG_LEN] == (uint8_t)':');
}

/* Build an error response. It is signed once the request has been authenticated. */
static int error_response(const struct ice_lite_agent *agent, const struct stun_msg *req,
			  uint16_t code, const uint16_t *unknown, size_t n_unknown, uint8_t *resp,
			  size_t resp_size)
{
	struct stun_builder builder;
	int ret;

	LOG_DBG("Check refused (%u)", code);

	ret = stun_builder_init(&builder, resp, resp_size,
				stun_msg_type_compose(STUN_METHOD_BINDING, STUN_CLASS_ERROR),
				req->txid);
	if (ret != 0) {
		return ret;
	}

	ret = stun_add_error_code(&builder, code, NULL);
	if (ret != 0) {
		return ret;
	}

	if (n_unknown > 0U) {
		ret = stun_add_unknown_attributes(&builder, unknown, n_unknown);
		if (ret != 0) {
			return ret;
		}
	}

	/* The response to a role conflict carries the role of the agent (RFC 8445
	 * section 7.3.1.1).
	 */
	if (code == ERR_ROLE_CONFLICT) {
		ret = stun_add_ice_controlled(&builder, agent->tiebreaker);
		if (ret != 0) {
			return ret;
		}
	}

	if ((code == ERR_BAD_REQUEST) || (code == ERR_UNAUTHENTICATED)) {
		return stun_finish_plain(&builder);
	}

	return stun_finish(&builder, (const uint8_t *)agent->pwd, ICE_LITE_PWD_LEN);
}

int ice_lite_input(struct ice_lite_agent *agent, const uint8_t *buf, size_t len,
		   const struct net_sockaddr *src, uint8_t *resp, size_t resp_size)
{
	uint16_t unknown[UNKNOWN_ATTRS_MAX];
	struct stun_builder builder;
	struct stun_msg msg;
	int ret;

	if ((agent == NULL) || (buf == NULL) || (src == NULL) || (resp == NULL)) {
		return -EINVAL;
	}

	if (!stun_has_valid_header(buf, len)) {
		return -ENOMSG;
	}

	/* A malformed or damaged message is not STUN after all (RFC 8489
	 * section 6.3): it is left to the data protocol.
	 */
	if (stun_parse(buf, len, &msg) != 0) {
		return -ENOMSG;
	}

	ret = stun_verify_fingerprint(&msg);
	if (ret == -EBADMSG) {
		return -ENOMSG;
	}

	/* A check carries FINGERPRINT (RFC 8445 section 7.1.2). Indications are
	 * the keepalives of the peer, and the agent sends no request that a
	 * response could answer.
	 */
	if ((ret != 0) || (msg.type != STUN_BINDING_REQUEST)) {
		return 0;
	}

	/* A check carries USERNAME, MESSAGE-INTEGRITY, PRIORITY and the role of the
	 * sender (RFC 8445 section 7.3.1.1).
	 */
	if (!msg.has_integrity || (msg.username == NULL) || !msg.has_priority ||
	    (!msg.has_ice_controlling && !msg.has_ice_controlled)) {
		return error_response(agent, &msg, ERR_BAD_REQUEST, NULL, 0U, resp, resp_size);
	}

	if (!username_ok(agent, &msg)) {
		return error_response(agent, &msg, ERR_UNAUTHENTICATED, NULL, 0U, resp, resp_size);
	}

	ret = stun_verify_integrity(&msg, (const uint8_t *)agent->pwd, ICE_LITE_PWD_LEN);
	if (ret == -EBADMSG) {
		return error_response(agent, &msg, ERR_UNAUTHENTICATED, NULL, 0U, resp, resp_size);
	}
	if (ret != 0) {
		return ret;
	}

	ret = stun_unknown_attrs(&msg, NULL, 0U, unknown, ARRAY_SIZE(unknown));
	if (ret < 0) {
		return ret;
	}
	if (ret > 0) {
		return error_response(agent, &msg, ERR_UNKNOWN_ATTRIBUTE, unknown,
				      MIN((size_t)ret, ARRAY_SIZE(unknown)), resp, resp_size);
	}

	/* A lite agent cannot take the controlling role: the peer has to. */
	if (msg.has_ice_controlled) {
		return error_response(agent, &msg, ERR_ROLE_CONFLICT, NULL, 0U, resp, resp_size);
	}

	ret = stun_builder_init(&builder, resp, resp_size, STUN_BINDING_SUCCESS_RESPONSE, msg.txid);
	if (ret != 0) {
		return ret;
	}

	ret = stun_add_xor_mapped_address(&builder, src);
	if (ret != 0) {
		return ret;
	}

	ret = stun_finish(&builder, (const uint8_t *)agent->pwd, ICE_LITE_PWD_LEN);
	if ((ret > 0) && msg.has_use_candidate) {
		memset(&agent->remote, 0, sizeof(agent->remote));
		memcpy(&agent->remote, src,
		       (src->sa_family == NET_AF_INET)
			       ? sizeof(struct net_sockaddr_in)
			       : MIN(sizeof(struct net_sockaddr_in6), sizeof(agent->remote)));
		agent->nominated = true;
	}

	return ret;
}

int ice_lite_keepalive(uint8_t *buf, size_t size)
{
	uint8_t txid[STUN_TXID_SIZE];
	struct stun_builder builder;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (sys_csrand_get(txid, sizeof(txid)) != 0) {
		return -EIO;
	}

	ret = stun_builder_init(&builder, buf, size,
				stun_msg_type_compose(STUN_METHOD_BINDING, STUN_CLASS_INDICATION),
				txid);
	if (ret != 0) {
		return ret;
	}

	return stun_finish_plain(&builder);
}
