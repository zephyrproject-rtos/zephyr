/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * SNMP core dispatcher — routes v1/v2c and v3 messages, handles version
 * validation, community/USM security, MIB dispatch, and response encoding.
 *
 * This file also owns the instance lifecycle (snmp_agent_init(),
 * snmp_agent_deinit()) documented in include/snmp/snmp_core.h.
 */

#include <zephyr/net/snmp/snmp_core.h>
#include <zephyr/net/snmp/snmp_trap.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include "snmp_internal.h"
#include <zephyr/sys/atomic.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)

#if defined(__ZEPHYR__)
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(snmp, CONFIG_SNMP_LOG_LEVEL);
#else
#define LOG_WRN(...)
#define LOG_ERR(...)
#endif

#if defined(CONFIG_SNMP_VERSION_3)
#include <zephyr/net/snmp/snmp_usm.h>
#include <zephyr/net/snmp/snmp_vacm.h>
#endif

#ifndef CONFIG_SNMP_COMMUNITY_READ
#define CONFIG_SNMP_COMMUNITY_READ "public"
#endif

#ifndef CONFIG_SNMP_COMMUNITY_WRITE
#define CONFIG_SNMP_COMMUNITY_WRITE "private"
#endif

#ifndef CONFIG_SNMP_AGENT_PORT
#define CONFIG_SNMP_AGENT_PORT 161
#endif

#ifndef CONFIG_SNMP_V3_ENGINE_BOOTS
#define CONFIG_SNMP_V3_ENGINE_BOOTS 1
#endif

#ifdef CONFIG_SNMP_RATE_LIMIT
#ifndef CONFIG_SNMP_RATE_LIMIT_BURST
#define CONFIG_SNMP_RATE_LIMIT_BURST 5
#endif
#endif

#define INST_COUNTER_INC(inst, field) snmp_counter_inc(&(inst)->counters.field)

/* Copy a bounded string, always NUL-terminating. */
static void copy_str(char *dst, size_t dst_sz, const char *src)
{
	strncpy(dst, src, dst_sz - 1);
	dst[dst_sz - 1] = '\0';
}

/*
 * Compare a wire community against a configured one, octet for octet.
 *
 * The decoded community is a counted byte string, not a C string: it may contain
 * embedded NULs and is not guaranteed to be terminated. strcmp() therefore both
 * mis-compares valid communities and can read past the last received octet.
 */
static bool community_matches(const struct snmp_pdu *pdu, const char *configured)
{
	size_t len;

	if (!pdu || !configured) {
		return false;
	}

	len = strlen(configured);
	return pdu->community_len == len && memcmp(pdu->community, configured, len) == 0;
}

/*
 * PDU types an agent may act on. Responses, traps and reports are never requests:
 * echoing one would turn the agent into a reflector (the old dispatch fell through
 * and re-encoded whatever type it received).
 */
static bool pdu_is_request(uint8_t type)
{
	switch (type) {
	case SNMP_PDU_GET_REQ:
	case SNMP_PDU_GET_NEXT_REQ:
	case SNMP_PDU_GET_BULK_REQ:
	case SNMP_PDU_SET_REQ:
	case SNMP_PDU_INFORM_REQ:
		return true;
	default:
		return false;
	}
}

static void set_defaults(struct snmp_agent *agent)
{
	copy_str(agent->read_community, sizeof(agent->read_community), CONFIG_SNMP_COMMUNITY_READ);
	copy_str(agent->write_community, sizeof(agent->write_community),
		 CONFIG_SNMP_COMMUNITY_WRITE);

	agent->port = CONFIG_SNMP_AGENT_PORT;
	agent->sock_v4 = -1;
	agent->sock_v6 = -1;
	agent->bound_iface = NULL;
	agent->svc_slot = SNMP_SLOT_NONE;

	agent->counters.snmpEnableAuthenTraps = 1;

	copy_str(agent->sys_descr, sizeof(agent->sys_descr), "Zephyr SNMP Agent");
	copy_str(agent->sys_contact, sizeof(agent->sys_contact), "admin@example.com");
	copy_str(agent->sys_name, sizeof(agent->sys_name), "zephyr-node");
	copy_str(agent->sys_location, sizeof(agent->sys_location), "Server Room");

	agent->sys_object_id.len = 7;
	agent->sys_object_id.ids[0] = 1;
	agent->sys_object_id.ids[1] = 3;
	agent->sys_object_id.ids[2] = 6;
	agent->sys_object_id.ids[3] = 1;
	agent->sys_object_id.ids[4] = 4;
	agent->sys_object_id.ids[5] = 1;
	agent->sys_object_id.ids[6] = 0;
}

int snmp_agent_init(struct snmp_agent *agent, const struct snmp_agent_cfg *cfg)
{
	int ret;

	if (agent == NULL) {
		return -EINVAL;
	}
	if (agent->state == SNMP_AGENT_RUNNING || agent->state == SNMP_AGENT_STOPPING) {
		return -EBUSY;
	}

	memset(agent, 0, sizeof(*agent));
	k_mutex_init(&agent->lock);

	set_defaults(agent);

#ifdef CONFIG_SNMP_RATE_LIMIT
	agent->rate_tokens = CONFIG_SNMP_RATE_LIMIT_BURST;
	agent->rate_last_refill = k_uptime_get();
#endif

	if (cfg != NULL) {
		if (cfg->port != 0) {
			agent->port = cfg->port;
		}
		agent->bound_iface = cfg->iface;
		if (cfg->read_community != NULL) {
			copy_str(agent->read_community, sizeof(agent->read_community),
				 cfg->read_community);
		}
		if (cfg->write_community != NULL) {
			copy_str(agent->write_community, sizeof(agent->write_community),
				 cfg->write_community);
		}
		if (cfg->sys_descr != NULL) {
			copy_str(agent->sys_descr, sizeof(agent->sys_descr), cfg->sys_descr);
		}
		if (cfg->sys_contact != NULL) {
			copy_str(agent->sys_contact, sizeof(agent->sys_contact), cfg->sys_contact);
		}
		if (cfg->sys_name != NULL) {
			copy_str(agent->sys_name, sizeof(agent->sys_name), cfg->sys_name);
		}
		if (cfg->sys_location != NULL) {
			copy_str(agent->sys_location, sizeof(agent->sys_location),
				 cfg->sys_location);
		}
		if (cfg->sys_object_id != NULL && cfg->sys_object_id->len > 0 &&
		    cfg->sys_object_id->len <= CONFIG_SNMP_MAX_OID_LEN) {
			agent->sys_object_id = *cfg->sys_object_id;
		}
		agent->user_data = (void *)cfg->user_data;
	}

	/* MIB tree: always fresh on init (defaults are instance-scoped). */
	snmp_mib_init(agent);
	ret = snmp_mib_register_defaults(agent);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_SNMP_VERSION_3)
	if (cfg != NULL) {
		if (cfg->engine_id != NULL && cfg->engine_id_len > 0 &&
		    cfg->engine_id_len <= sizeof(agent->engine_id)) {
			memcpy(agent->engine_id, cfg->engine_id, cfg->engine_id_len);
			agent->engine_id_len = cfg->engine_id_len;
		}
		if (cfg->engine_boots != 0) {
			agent->engine_boots = cfg->engine_boots;
		}
		agent->settings_prefix = cfg->settings_prefix;
	}
	ret = snmp_usm_init(agent);
	if (ret != 0) {
		return ret;
	}
	ret = snmp_vacm_init(agent);
	if (ret != 0) {
		return ret;
	}
	ret = snmp_usm_register_mibs(agent);
	if (ret != 0) {
		return ret;
	}

	/* Contract 3.7 call order: defaults -> cfg.engine_boots -> persisted override. */
	if (agent->settings_prefix != NULL) {
		int b_ret = snmp_agent_load_engine_boots(agent);

		if (b_ret < 0 && b_ret != -ENOENT) {
			LOG_WRN("Failed to load engineBoots (%d)", b_ret);
		}
	}
#endif

	agent->state = SNMP_AGENT_INITIALIZED;
	return 0;
}

int snmp_agent_start(struct snmp_agent *agent)
{
	if (agent == NULL) {
		return -EINVAL;
	}
	if (agent->state == SNMP_AGENT_UNINIT) {
		return -EPERM;
	}
	if (agent->state == SNMP_AGENT_RUNNING || agent->state == SNMP_AGENT_STOPPING) {
		return -EPERM;
	}

#if !defined(__ZEPHYR__)
	/* The socket-service adapter is Zephyr-only; host builds keep the protocol logic
	 * testable by injecting frames through snmp_agent_process_pdu().
	 */
	return -ENOTSUP;
#else
	enum snmp_agent_state prev;
	int ret;

	/*
	 * Publish RUNNING *before* attaching: the adapter registers its descriptors last,
	 * so from the moment an fd becomes pollable the callback's post-lock state check
	 * already accepts it and no early request is dropped. The previous state is
	 * restored under the lock if attaching fails.
	 */
	prev = agent->state;

	k_mutex_lock(&agent->lock, K_FOREVER);
	agent->state = SNMP_AGENT_RUNNING;
	k_mutex_unlock(&agent->lock);

	ret = snmp_service_attach(agent);
	if (ret < 0) {
		k_mutex_lock(&agent->lock, K_FOREVER);
		agent->state = prev;
		k_mutex_unlock(&agent->lock);
	}
	return ret;
#endif
}

int snmp_agent_stop(struct snmp_agent *agent)
{
	if (agent == NULL) {
		return -EINVAL;
	}

	switch (agent->state) {
	case SNMP_AGENT_UNINIT:
		return -EPERM;
	case SNMP_AGENT_INITIALIZED:
	case SNMP_AGENT_STOPPED:
		/* Never started, or already stopped: stopping is safe and a no-op. */
		return 0;
	case SNMP_AGENT_RUNNING:
	case SNMP_AGENT_STOPPING:
	default:
		break;
	}

#if !defined(__ZEPHYR__)
	k_mutex_lock(&agent->lock, K_FOREVER);
	agent->state = SNMP_AGENT_STOPPED;
	k_mutex_unlock(&agent->lock);
	return 0;
#else
	/* Step 1 of the documented teardown ordering: publish STOPPING before the
	 * descriptor is deregistered, so a callback that already raced in is discarded by
	 * its own post-lock state check (contract 4.4).
	 */
	agent->state = SNMP_AGENT_STOPPING;
	return snmp_service_detach(agent);
#endif
}

int snmp_agent_deinit(struct snmp_agent *agent)
{
	int ret;

	if (agent == NULL) {
		return -EINVAL;
	}
	if (agent->state == SNMP_AGENT_UNINIT) {
		return -EPERM;
	}

	/* Tear the transport down first: resetting the tree while the sockets are live
	 * would let an in-flight request observe a half-cleared instance.
	 */
	if (agent->state == SNMP_AGENT_RUNNING || agent->state == SNMP_AGENT_STOPPING) {
		ret = snmp_agent_stop(agent);
		if (ret < 0) {
			return ret;
		}
	}

	k_mutex_lock(&agent->lock, K_FOREVER);

	agent->state = SNMP_AGENT_STOPPED;

	set_defaults(agent);
	memset(&agent->counters, 0, sizeof(agent->counters));
	agent->counters.snmpEnableAuthenTraps = 1;

	snmp_mib_init(agent);
	ret = snmp_mib_register_defaults(agent);
	if (ret != 0) {
		k_mutex_unlock(&agent->lock);
		return ret;
	}

#if defined(CONFIG_SNMP_VERSION_3)
	/* Drop credentials/access tables, keep this engine's identity. */
	memset(agent->users, 0, sizeof(agent->users));
	agent->user_cnt = 0;
	memset(agent->contexts, 0, sizeof(agent->contexts));
	agent->context_cnt = 0;
	memset(agent->groups, 0, sizeof(agent->groups));
	agent->group_cnt = 0;
	memset(agent->access_rules, 0, sizeof(agent->access_rules));
	agent->access_rule_cnt = 0;
	memset(agent->views, 0, sizeof(agent->views));
	agent->view_cnt = 0;
	ret = snmp_usm_init(agent);
	if (ret != 0) {
		k_mutex_unlock(&agent->lock);
		return ret;
	}
	ret = snmp_vacm_init(agent);
	if (ret != 0) {
		k_mutex_unlock(&agent->lock);
		return ret;
	}
	ret = snmp_usm_register_mibs(agent);
	if (ret != 0) {
		k_mutex_unlock(&agent->lock);
		return ret;
	}
#endif

	agent->state = SNMP_AGENT_INITIALIZED;

	k_mutex_unlock(&agent->lock);
	return 0;
}

int snmp_agent_set_read_community(struct snmp_agent *agent, const char *community)
{
	if (agent == NULL || community == NULL ||
	    strlen(community) >= sizeof(agent->read_community)) {
		return -EINVAL;
	}
	if (agent->state == SNMP_AGENT_UNINIT) {
		return -EPERM;
	}
	k_mutex_lock(&agent->lock, K_FOREVER);
	copy_str(agent->read_community, sizeof(agent->read_community), community);
	k_mutex_unlock(&agent->lock);
	return 0;
}

int snmp_agent_set_write_community(struct snmp_agent *agent, const char *community)
{
	if (agent == NULL || community == NULL ||
	    strlen(community) >= sizeof(agent->write_community)) {
		return -EINVAL;
	}
	if (agent->state == SNMP_AGENT_UNINIT) {
		return -EPERM;
	}
	k_mutex_lock(&agent->lock, K_FOREVER);
	copy_str(agent->write_community, sizeof(agent->write_community), community);
	k_mutex_unlock(&agent->lock);
	return 0;
}

/* ---- Detect SNMP version from raw buffer ---- */

static int snmp_detect_version(const uint8_t *buf, size_t buf_len)
{
	size_t off = 0;
	uint8_t tag;
	size_t seq_len;

	int ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &seq_len);
	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {

		return -1;
	}

	int32_t version = 0;
	ret = snmp_ber_decode_int(buf, buf_len, &off, &version);

	if (ret < 0) {
		return -1;
	}

	return version;
}

/* ---- v1 / v2c processing ---- */

static int process_v1v2c(struct snmp_agent *agent, const uint8_t *req_buf, size_t req_len,
			 uint8_t *resp_buf, size_t max_resp_len, size_t *out_resp_len)
{
	*out_resp_len = 0;

	memset(&agent->current_pdu, 0, sizeof(agent->current_pdu));
	int ret = snmp_ber_decode_pdu(req_buf, req_len, &agent->current_pdu);
	if (ret < 0) {

		INST_COUNTER_INC(agent, snmpInASNParseErrs);
		return ret;
	}

	/* Version validation */
	if (agent->current_pdu.version == 0) {
#ifndef CONFIG_SNMP_VERSION_1
		INST_COUNTER_INC(agent, snmpInBadVersions);
		return -ENOTSUP;
#endif
		if (agent->current_pdu.type == SNMP_PDU_GET_BULK_REQ ||
		    agent->current_pdu.type == SNMP_PDU_INFORM_REQ) {
			INST_COUNTER_INC(agent, snmpInBadVersions);
			return -ENOTSUP;
		}
	} else if (agent->current_pdu.version == 1) {
#ifndef CONFIG_SNMP_VERSION_2C
		INST_COUNTER_INC(agent, snmpInBadVersions);
		return -ENOTSUP;
#endif
	} else {
		INST_COUNTER_INC(agent, snmpInBadVersions);
		return -ENOTSUP;
	}

	/* Only request PDUs are processed; everything else is dropped silently. */
	if (!pdu_is_request(agent->current_pdu.type)) {
		INST_COUNTER_INC(agent, snmpSilentDrops);
		return -EACCES;
	}

	/* Community / Security validation */
	bool is_write = (agent->current_pdu.type == SNMP_PDU_SET_REQ);

	if (is_write) {
		if (!community_matches(&agent->current_pdu, agent->write_community)) {
			if (community_matches(&agent->current_pdu, agent->read_community)) {
				INST_COUNTER_INC(agent, snmpInBadCommunityUses);
			} else {
				INST_COUNTER_INC(agent, snmpInBadCommunityNames);
			}
#if defined(CONFIG_SNMP_TRAP_ENABLED)
			snmp_trap_send_auth_failure(agent);
#endif
			return -EACCES;
		}
	} else {
		/* GET / GETNEXT / GETBULK / INFORM accepts read or write community */
		if (!community_matches(&agent->current_pdu, agent->read_community) &&
		    !community_matches(&agent->current_pdu, agent->write_community)) {
			INST_COUNTER_INC(agent, snmpInBadCommunityNames);
#if defined(CONFIG_SNMP_TRAP_ENABLED)
			snmp_trap_send_auth_failure(agent);
#endif
			return -EACCES;
		}
	}

	uint8_t req_type = agent->current_pdu.type;
	int32_t non_rep =
		agent->current_pdu.non_repeaters < 0 ? 0 : agent->current_pdu.non_repeaters;
	if (non_rep > (int32_t)agent->current_pdu.varbind_cnt) {
		non_rep = agent->current_pdu.varbind_cnt;
	}

	if (req_type == SNMP_PDU_INFORM_REQ) {
		/*
		 * An Inform is a notification this agent receives as a notification
		 * receiver: acknowledge it with a Response carrying the same varbinds
		 * (RFC 3416 §4.2.7), no MIB lookup.
		 */
		agent->current_pdu.type = SNMP_PDU_GET_RESP;
		agent->current_pdu.error_status = SNMP_ERR_NO_ERROR;
		agent->current_pdu.error_index = 0;
		INST_COUNTER_INC(agent, snmpInTraps);
	} else {
		/* Dispatch to MIB engine with instance context */
		ret = snmp_mib_dispatch(agent, &agent->current_pdu);
		if (ret < 0) {
			return ret;
		}
	}

	/* Encode response PDU */
	size_t encoded_len = 0;
	ret = snmp_ber_encode_pdu(resp_buf, max_resp_len, &encoded_len, &agent->current_pdu);

	if (ret == -ENOBUFS && req_type == SNMP_PDU_GET_BULK_REQ) {
		while (ret == -ENOBUFS && agent->current_pdu.varbind_cnt > (uint8_t)non_rep) {
			agent->current_pdu.varbind_cnt--;
			ret = snmp_ber_encode_pdu(resp_buf, max_resp_len, &encoded_len,
						  &agent->current_pdu);
		}
	}

	if (ret == 0) {
		*out_resp_len = encoded_len;
		INST_COUNTER_INC(agent, snmpOutPkts);
		return 0;
	}

	if (ret == -ENOBUFS) {
		INST_COUNTER_INC(agent, snmpOutTooBigs);
		agent->current_pdu.error_status = SNMP_ERR_TOO_BIG;
		agent->current_pdu.error_index = 0;
		agent->current_pdu.varbind_cnt = 0;
		agent->current_pdu.type = SNMP_PDU_GET_RESP;
		int enc_ret = snmp_ber_encode_pdu(resp_buf, max_resp_len, &encoded_len,
						  &agent->current_pdu);
		if (enc_ret == 0) {
			*out_resp_len = encoded_len;
			INST_COUNTER_INC(agent, snmpOutPkts);
			return 0;
		}
	}

	return ret;
}

/* ---- v3 processing ---- */

#if defined(CONFIG_SNMP_VERSION_3)

static int process_v3(struct snmp_agent *agent, const uint8_t *req_buf, size_t req_len,
		      uint8_t *resp_buf, size_t max_resp_len, size_t *out_resp_len)
{
	*out_resp_len = 0;

	struct snmp_v3_msg msg;
	int ret = snmp_ber_decode_v3_msg(req_buf, req_len, &msg);
	if (ret < 0) {

		INST_COUNTER_INC(agent, snmpInASNParseErrs);
		if (ret == -ENOTSUP) {
			agent->mpd_stats.unknown_security_models++;
		} else {
			agent->mpd_stats.invalid_msgs++;
		}
		return ret;
	}

	/* USM verify */
	ret = snmp_usm_verify_incoming(agent, &msg, req_buf, req_len);
	if (ret < 0) {
		if (ret == -EACCES) {
			INST_COUNTER_INC(agent, snmpSilentDrops);
		}
		return ret;
	}

	/* Engine discovery / USM reports: USM already filled a complete Report PDU. */
	if (msg.scoped.pdu.type == SNMP_PDU_REPORT) {
		/* Echo incoming msg_id as the Report-PDU request_id */
		msg.scoped.pdu.request_id = msg.global.msg_id;

		msg.global.msg_max_size = CONFIG_SNMP_MAX_PDU_SIZE;
		msg.global.msg_security_model = SNMP_SEC_MODEL_USM;

		ret = snmp_usm_build_outgoing(agent, &msg);
		if (ret < 0) {
			return ret;
		}

		size_t encoded_len = 0;
		ret = snmp_ber_encode_v3_msg(resp_buf, max_resp_len, &encoded_len, &msg);

		if (ret < 0) {
			return ret;
		}

		*out_resp_len = encoded_len;
		INST_COUNTER_INC(agent, snmpOutPkts);
		return 0;
	}

	/* Only request PDUs are processed; a Response/Trap/Report is dropped. */
	if (!pdu_is_request(msg.scoped.pdu.type)) {
		INST_COUNTER_INC(agent, snmpSilentDrops);
		agent->mpd_stats.unknown_pdu_handlers++;
		return -EACCES;
	}

	/* VACM access check per RFC 3415, using the *verified* level. */
	int sec_level = msg.sec_level;
	int access_type = (msg.scoped.pdu.type == SNMP_PDU_SET_REQ) ? SNMP_VACM_ACCESS_WRITE
								    : SNMP_VACM_ACCESS_READ;

	char user_name[sizeof(msg.usm.user_name) + 1];
	size_t name_len = msg.usm.user_name_len < sizeof(msg.usm.user_name)
				  ? msg.usm.user_name_len
				  : sizeof(msg.usm.user_name);
	memcpy(user_name, msg.usm.user_name, name_len);
	user_name[name_len] = '\0';

	bool authorized = true;

	if (msg.scoped.pdu.varbind_cnt == 0) {
		authorized = snmp_vacm_check_access_ex(agent, (const char *)msg.scoped.context_name,
						       msg.global.msg_security_model, user_name,
						       sec_level, access_type, NULL) == 0;
	} else {
		for (size_t i = 0; i < msg.scoped.pdu.varbind_cnt; i++) {
			if (snmp_vacm_check_access_ex(agent, (const char *)msg.scoped.context_name,
						      msg.global.msg_security_model, user_name,
						      sec_level, access_type,
						      &msg.scoped.pdu.varbinds[i].oid) < 0) {
				authorized = false;
				break;
			}
		}
	}

	uint8_t req_type = msg.scoped.pdu.type;
	int32_t non_rep = msg.scoped.pdu.non_repeaters < 0 ? 0 : msg.scoped.pdu.non_repeaters;
	if (non_rep > (int32_t)msg.scoped.pdu.varbind_cnt) {

		non_rep = msg.scoped.pdu.varbind_cnt;
	}

	if (!authorized) {
		/*
		 * RFC 3415 §5.5: a read/write class denial is answered with a Response
		 * carrying authorizationError and errorIndex 0. Dropping the message
		 * instead (the previous behaviour) makes a misconfigured view
		 * indistinguishable from packet loss on the manager side.
		 */
		msg.scoped.pdu.type = SNMP_PDU_GET_RESP;
		msg.scoped.pdu.error_status = SNMP_ERR_AUTHORIZATION_ERR;
		msg.scoped.pdu.error_index = 0;
	} else if (req_type == SNMP_PDU_INFORM_REQ) {
		/* Acknowledge the notification; no MIB lookup (RFC 3416 §4.2.7). */
		msg.scoped.pdu.type = SNMP_PDU_GET_RESP;
		msg.scoped.pdu.error_status = SNMP_ERR_NO_ERROR;
		msg.scoped.pdu.error_index = 0;
		INST_COUNTER_INC(agent, snmpInTraps);
	} else {
		ret = snmp_mib_dispatch(agent, &msg.scoped.pdu);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set response metadata (RFC 3412 §7.1: echo request's contextEngineID and contextName) */
	msg.global.msg_max_size = CONFIG_SNMP_MAX_PDU_SIZE;
	msg.global.msg_security_model = SNMP_SEC_MODEL_USM;
	/* Response must use the same verified security level as the request. */
	msg.sec_level = (uint8_t)sec_level;

	/* Build outgoing USM */
	ret = snmp_usm_build_outgoing(agent, &msg);
	if (ret < 0) {
		return ret;
	}

	/* Encode */
	size_t encoded_len = 0;
	ret = snmp_ber_encode_v3_msg(resp_buf, max_resp_len, &encoded_len, &msg);

	if (ret == -ENOBUFS && req_type == SNMP_PDU_GET_BULK_REQ) {
		while (ret == -ENOBUFS && msg.scoped.pdu.varbind_cnt > (uint8_t)non_rep) {
			msg.scoped.pdu.varbind_cnt--;
			ret = snmp_usm_build_outgoing(agent, &msg);
			if (ret < 0) {
				break;
			}
			ret = snmp_ber_encode_v3_msg(resp_buf, max_resp_len, &encoded_len, &msg);
		}
	}

	if (ret == 0) {
		*out_resp_len = encoded_len;
		INST_COUNTER_INC(agent, snmpOutPkts);
		return 0;
	}

	if (ret == -ENOBUFS) {
		INST_COUNTER_INC(agent, snmpOutTooBigs);
		msg.scoped.pdu.error_status = SNMP_ERR_TOO_BIG;
		msg.scoped.pdu.error_index = 0;
		msg.scoped.pdu.varbind_cnt = 0;
		msg.scoped.pdu.type = SNMP_PDU_GET_RESP;
		if (snmp_usm_build_outgoing(agent, &msg) == 0) {
			int enc_ret =
				snmp_ber_encode_v3_msg(resp_buf, max_resp_len, &encoded_len, &msg);
			if (enc_ret == 0) {
				*out_resp_len = encoded_len;
				INST_COUNTER_INC(agent, snmpOutPkts);
				return 0;
			}
		}
	}

	return ret;
}

#endif /* CONFIG_SNMP_VERSION_3 */

/* ---- Main entry points ---- */

int snmp_agent_process_pdu(struct snmp_agent *agent, const uint8_t *req_buf, size_t req_len,
			   uint8_t *resp_buf, size_t max_resp_len, size_t *out_resp_len)
{
	int ret;

	if (agent == NULL || req_buf == NULL || resp_buf == NULL || out_resp_len == NULL) {
		return -EINVAL;
	}

	/*
	 * Serialise the whole request against the agent's own scratch state: rx/tx
	 * buffers, current_pdu and the MIB tree. The socket-service callback already
	 * holds this lock (Zephyr k_mutex is recursive for its owner, so re-locking
	 * here is safe) but direct/test callers do not, and process_v3() previously
	 * ran completely unlocked.
	 */
	k_mutex_lock(&agent->lock, K_FOREVER);

	if (agent->state == SNMP_AGENT_UNINIT) {
		k_mutex_unlock(&agent->lock);
		return -EPERM;
	}

	*out_resp_len = 0;
	INST_COUNTER_INC(agent, snmpInPkts);

	int version = snmp_detect_version(req_buf, req_len);

	if (version < 0) {
		/* Not an SNMP message at all: this is a BER failure, not a bad version. */
		INST_COUNTER_INC(agent, snmpInASNParseErrs);
		k_mutex_unlock(&agent->lock);
		return -EBADMSG;
	}

	switch (version) {
	case 0:
	case 1:
		ret = process_v1v2c(agent, req_buf, req_len, resp_buf, max_resp_len, out_resp_len);
		break;
	case 3:
#if defined(CONFIG_SNMP_VERSION_3)
		ret = process_v3(agent, req_buf, req_len, resp_buf, max_resp_len, out_resp_len);
#else
		INST_COUNTER_INC(agent, snmpInBadVersions);
		ret = -ENOTSUP;
#endif
		break;
	default:
		INST_COUNTER_INC(agent, snmpInBadVersions);
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&agent->lock);
	return ret;
}

#endif /* CONFIG_SNMP_AGENT */
