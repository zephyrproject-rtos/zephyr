/*
 * Copyright (c) 2025-2026 Nordic Semiconductor ASA
 * Copyright (c) 2026, Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#define LOG_LEVEL CONFIG_MCUMGR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(transport_mgmt);

struct transport_id_lookup_t {
	struct smp_transport *transport;
	uint32_t transport_id;
	bool found;
};

static struct smp_transport_bridge bridges[CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES];
static bool bridge_active;

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_LOCKING)
static K_SEM_DEFINE(mcumgr_transport_sem, 1, 1);

void transport_mgmt_lock(void)
{
	k_sem_take(&mcumgr_transport_sem, K_FOREVER);
}

void transport_mgmt_unlock(void)
{
	k_sem_give(&mcumgr_transport_sem);
}
#endif

static bool transport_mgmt_get_id_loop(const struct smp_client_transport_entry *transport,
				       void *user_data)
{
	struct transport_id_lookup_t *transport_lookup = (struct transport_id_lookup_t *)user_data;

	if (transport->smpt == transport_lookup->transport) {
		transport_lookup->transport_id = transport->smpt_type;
		transport_lookup->found = true;
		return false;
	}

	return true;
}

bool transport_mgmt_is_bridged_internal(struct smp_transport *transport, bool direction)
{
	uint8_t i = 0;

	if (!bridge_active) {
		return false;
	}

	while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
		if (bridges[i].status == 1 && ((bridges[i].incoming_transport == transport &&
		      direction == TRANSPORT_MGMT_DIRECTION_INCOMING) ||
		     (bridges[i].outgoing_transport == transport &&
		      direction == TRANSPORT_MGMT_DIRECTION_OUTGOING))) {
			return true;
		}

		++i;
	}

	return false;
}

bool transport_mgmt_is_bridged(struct smp_transport *transport, bool direction)
{
	bool bridged = false;

	transport_mgmt_lock();

	bridged = transport_mgmt_is_bridged_internal(transport, direction);

	transport_mgmt_unlock();

	return bridged;
}

struct smp_transport *transport_mgmt_get_other_transport(struct smp_transport *transport,
							 bool direction)
{
	struct smp_transport *other_transport = NULL;

	transport_mgmt_lock();

	if (bridge_active) {
		uint8_t i = 0;

		while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
			if (bridges[i].status == 1) {
				if (bridges[i].incoming_transport == transport &&
				    direction == TRANSPORT_MGMT_DIRECTION_OUTGOING) {
					other_transport = bridges[i].outgoing_transport;
					break;
				} else if (bridges[i].outgoing_transport == transport &&
					   direction == TRANSPORT_MGMT_DIRECTION_OUTGOING) {
					other_transport = bridges[i].incoming_transport;
					break;
				}
			}

			++i;
		}
	}

	transport_mgmt_unlock();

	return other_transport;
}

const struct smp_transport_bridge *transport_mgmt_get_bridge_internal(
							struct smp_transport *transport,
							bool direction)
{
	uint8_t i = 0;

	if (!bridge_active) {
		return NULL;
	}

	while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
		if (bridges[i].status == 1 && ((direction == TRANSPORT_MGMT_DIRECTION_INCOMING &&
		      bridges[i].incoming_transport == transport) ||
		      (direction == TRANSPORT_MGMT_DIRECTION_OUTGOING &&
		       bridges[i].outgoing_transport == transport))) {
			return &bridges[i];
		}

		++i;
	}

	return NULL;
}

const struct smp_transport_bridge *transport_mgmt_get_bridge(struct smp_transport *transport,
							bool direction)
{
	const struct smp_transport_bridge *bridge = NULL;

	transport_mgmt_lock();

	bridge = transport_mgmt_get_bridge_internal(transport, direction);

	transport_mgmt_unlock();

	return bridge;
}

int transport_mgmt_disconnect_bridge(struct smp_transport_bridge *bridge)
{
	uint8_t i = 0;

	transport_mgmt_lock();

	if (!bridge_active || bridge->status == 0) {
		return -ENOTCONN;
	}

	bridge->outgoing_transport->functions.bridge_disconnect(bridge, true);
	bridge->incoming_transport->functions.bridge_disconnect(bridge, false);
	bridge->status = 0;
	bridge->incoming_transport = NULL;
	bridge->outgoing_transport = NULL;

	while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
		if (bridges[i].status == 1) {
			break;
		}

		++i;
	}

	if (i == CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
		bridge_active = false;
	}

	transport_mgmt_unlock();

	return 0;
}

int transport_mgmt_disconnect_transport(struct smp_transport *transport, bool incoming,
					bool outgoing)
{
	uint8_t i = 0;
	bool update = false;

	transport_mgmt_lock();

	if (bridge_active) {
		while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
			if (bridges[i].status == 1 && ((bridges[i].incoming_transport == transport
			    && incoming) || (bridges[i].outgoing_transport == transport
			    && outgoing))) {
				bridges[i].outgoing_transport->functions.bridge_disconnect(
									   &bridges[i], true);
				bridges[i].incoming_transport->functions.bridge_disconnect(
									   &bridges[i], false);
				bridges[i].status = 0;
				bridges[i].incoming_transport = NULL;
				bridges[i].outgoing_transport = NULL;
				update = true;
			}

			++i;
		}

		if (update) {
			i = 0;

			while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
				if (bridges[i].status == 1) {
					break;
				}

				++i;
			}

			if (i == CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
				bridge_active = false;
			}
		}
	}

	transport_mgmt_unlock();

	return 0;
}

int transport_mgmt_disconnect_all(void)
{
	transport_mgmt_lock();

	if (bridge_active) {
		uint8_t i = 0;

		while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
			if (bridges[i].status == 1) {
				bridges[i].outgoing_transport->functions.bridge_disconnect(
									&bridges[i], true);
				bridges[i].incoming_transport->functions.bridge_disconnect(
									&bridges[i], false);
				bridges[i].status = 0;
				bridges[i].incoming_transport = NULL;
				bridges[i].outgoing_transport = NULL;
			}

			++i;
		}

		bridge_active = false;
	}

	transport_mgmt_unlock();

	return 0;
}

static int transport_mgmt_connect(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok = true;
	size_t decoded = 0;
	uint32_t transport_id = 0;
	uint32_t mode = 0;
	size_t backup_element_count_reader = zsd->elem_count;
	struct smp_transport *outgoing_transport;
	uint8_t i = 0;
	uint8_t bridge_id = UCHAR_MAX;

	struct zcbor_map_decode_key_val cbor_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("transport", zcbor_uint32_decode, &transport_id),
		ZCBOR_MAP_DECODE_KEY_DECODER("mode", zcbor_uint32_decode, &mode),
	};

	if (ctxt->smpt->functions.bridge_connect == NULL ||
	    ctxt->smpt->functions.bridge_disconnect == NULL ||
	    ctxt->smpt->functions.bridge_output == NULL) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_REQUIRED_FUNCTIONS);
		goto end;
	}

	if (!zcbor_new_backup(zsd, backup_element_count_reader)) {
		LOG_ERR("Failed to create zcbor backup");
		return MGMT_ERR_ENOMEM;
	}

	ok = zcbor_map_decode_bulk(zsd, cbor_decode, ARRAY_SIZE(cbor_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	if (decoded == 0 || !zcbor_map_decode_bulk_key_found(cbor_decode,
					ARRAY_SIZE(cbor_decode), "transport")) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_TRANSPORT);
		return MGMT_ERR_EOK;
	}

	if (!zcbor_process_backup(zsd, (ZCBOR_FLAG_RESTORE | ZCBOR_FLAG_CONSUME),
				  backup_element_count_reader)) {
		LOG_ERR("Failed to restore zcbor reader backup");
		return MGMT_ERR_ENOMEM;
	}

	outgoing_transport = smp_client_transport_get(transport_id);

	if (outgoing_transport == NULL) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_TRANSPORT);
		goto end;
	}

	if (outgoing_transport->functions.bridge_connect == NULL ||
	    outgoing_transport->functions.bridge_disconnect == NULL ||
	    outgoing_transport->functions.bridge_output == NULL) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_REQUIRED_FUNCTIONS);
		goto end;
	}

	transport_mgmt_lock();

	while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
		if (bridges[i].status == 0 && bridge_id == UCHAR_MAX) {
			bridge_id = i;
		}

		if (bridges[i].status == 1) {
			if (bridges[i].incoming_transport == ctxt->smpt) {
				transport_mgmt_unlock();
				smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				  TRANSPORT_MGMT_ERR_TRANSPORT_INCOMING_TRANSPORT_ALREADY_BRIDGED);
				goto end;
			}

			if (bridges[i].outgoing_transport == outgoing_transport) {
				transport_mgmt_unlock();
				smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				  TRANSPORT_MGMT_ERR_TRANSPORT_OUTGOING_TRANSPORT_ALREADY_BRIDGED);
				goto end;
			}
		}

		++i;
	}

	if (bridge_id == UCHAR_MAX) {
		transport_mgmt_unlock();
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_ALL_CONTEXTS_USED);
		goto end;
	}

	if (!outgoing_transport->functions.bridge_connect(&bridges[bridge_id], true, mode,
			 (outgoing_transport == ctxt->smpt ? true : false), zsd, zse)) {
		transport_mgmt_unlock();
		goto end;
	}

	if (!ctxt->smpt->functions.bridge_connect(&bridges[bridge_id], false, 0,
			(outgoing_transport == ctxt->smpt ? true : false), zsd, zse)) {
		(void)outgoing_transport->functions.bridge_disconnect(&bridges[bridge_id], true);
		transport_mgmt_unlock();
		goto end;
	}

	bridges[bridge_id].status = 1;
	bridges[bridge_id].incoming_transport = ctxt->smpt;
	bridges[bridge_id].outgoing_transport = outgoing_transport;
	bridge_active = true;

	transport_mgmt_unlock();

end:
	return MGMT_RETURN_CHECK(ok);
}

static int transport_mgmt_disconnect(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok = true;
	size_t decoded = 0;
	uint32_t transport_id = 0;
	bool disconnect_all = false;

	struct zcbor_map_decode_key_val cbor_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("transport", zcbor_uint32_decode, &transport_id),
		ZCBOR_MAP_DECODE_KEY_DECODER("all", zcbor_bool_decode, &disconnect_all),
	};

	ok = zcbor_map_decode_bulk(zsd, cbor_decode, ARRAY_SIZE(cbor_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	if (zcbor_map_decode_bulk_key_found(cbor_decode, ARRAY_SIZE(cbor_decode), "transport") &&
	    disconnect_all) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_BOTH_TRANSPORT_AND_ALL_PARAMETERS);
		return MGMT_ERR_EOK;
	}

	if (!bridge_active) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT, TRANSPORT_MGMT_ERR_NOT_BRIDGED);
		return MGMT_ERR_EOK;
	}

	if (disconnect_all) {
		(void)transport_mgmt_disconnect_all();
	} else if (zcbor_map_decode_bulk_key_found(cbor_decode, ARRAY_SIZE(cbor_decode),
						   "transport")) {
		struct smp_transport *target_transport = smp_client_transport_get(transport_id);

		if (target_transport == NULL) {
			smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
					TRANSPORT_MGMT_ERR_INVALID_TRANSPORT);
			return MGMT_ERR_EOK;
		}

		(void)transport_mgmt_disconnect_transport(target_transport, true, true);
	} else {
		(void)transport_mgmt_disconnect_transport(ctxt->smpt, true, true);
	}

	return MGMT_ERR_EOK;
}

static int transport_mgmt_status(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok = true;
	uint32_t active = 0;
	bool bridged = false;
	uint8_t i = 0;
	struct transport_id_lookup_t transport_lookup = {
		.found = false,
	};

	transport_mgmt_lock();

	while (i < CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) {
		if (bridges[i].status == 1) {
			++active;

			if (bridges[i].incoming_transport == ctxt->smpt) {
				bridged = true;
				transport_lookup.transport = bridges[i].outgoing_transport;
				(void)smp_client_transport_foreach(transport_mgmt_get_id_loop,
					(void *)&transport_lookup);
			} else if (bridges[i].outgoing_transport == ctxt->smpt) {
				bridged = true;
				transport_lookup.transport = bridges[i].incoming_transport;
				(void)smp_client_transport_foreach(transport_mgmt_get_id_loop,
					(void *)&transport_lookup);
			}
		}

		++i;
	}

	transport_mgmt_unlock();

	ok = zcbor_tstr_put_lit(zse, "supported") &&
	     zcbor_uint32_put(zse, CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES) &&
	     zcbor_tstr_put_lit(zse, "active") &&
	     zcbor_uint32_put(zse, active);

	if (ok && bridged) {
		ok = zcbor_tstr_put_lit(zse, "bridged") &&
		     zcbor_bool_put(zse, true);

		if (ok && transport_lookup.found) {
			ok = zcbor_tstr_put_lit(zse, "transport") &&
			     zcbor_uint32_put(zse, transport_lookup.transport_id);
		}
	}

	return MGMT_RETURN_CHECK(ok);
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
static bool transport_mgmt_count_loop(const struct smp_client_transport_entry *transport,
				      void *user_data)
{
	uint32_t *count = (uint32_t *)user_data;

	++*count;

	return true;
}

static bool transport_mgmt_list_loop(const struct smp_client_transport_entry *transport,
				     void *user_data)
{
	zcbor_state_t *zse = (zcbor_state_t *)user_data;
	bool ok = true;

	if (transport->smpt->functions.bridge_connect != NULL &&
	    transport->smpt->functions.bridge_disconnect != NULL &&
	    transport->smpt->functions.bridge_output != NULL) {
		ok = zcbor_map_start_encode(zse, 2) &&
		     zcbor_tstr_put_lit(zse, "id") &&
		     zcbor_uint32_put(zse, transport->smpt_type);

		if (!ok) {
			goto finish;
		}

		if (transport->name != NULL) {
			ok = zcbor_tstr_put_lit(zse, "name") &&
			     zcbor_tstr_put_term(zse, transport->name, 30);

			if (!ok) {
				goto finish;
			}
		}

		ok = zcbor_map_end_encode(zse, 2);
	}

finish:
	return ok;
}

static int transport_mgmt_list(struct smp_streamer *ctxt)
{
	uint32_t transports = 0;
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok = true;

	smp_client_transport_foreach(transport_mgmt_count_loop, (void *)&transports);

	ok = zcbor_tstr_put_lit(zse, "transports") &&
	     zcbor_list_start_encode(zse, transports) &&
	     smp_client_transport_foreach(transport_mgmt_list_loop, (void *)zse) &&
	     zcbor_list_end_encode(zse, transports);

	return MGMT_RETURN_CHECK(ok);
}

static int transport_mgmt_modes(struct smp_streamer *ctxt)
{
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok = true;
	size_t decoded = 0;
	uint32_t transport_id = 0;
	struct smp_transport *transport;

	struct zcbor_map_decode_key_val cbor_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("transport", zcbor_uint32_decode, &transport_id),
	};

	ok = zcbor_map_decode_bulk(zsd, cbor_decode, ARRAY_SIZE(cbor_decode),
				   &decoded) == 0;

	if (!ok || decoded == 0 || !zcbor_map_decode_bulk_key_found(cbor_decode,
						ARRAY_SIZE(cbor_decode), "transport")) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_TRANSPORT);
		return MGMT_ERR_EOK;
	}

	transport = smp_client_transport_get(transport_id);

	if (transport == NULL) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_TRANSPORT);
		return MGMT_ERR_EOK;
	}

	if (transport->functions.bridge_modes == NULL) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_INFO_FUNCTIONS);
		return MGMT_ERR_EOK;
	}

	ok = zcbor_tstr_put_lit(zse, "modes") &&
	     zcbor_list_start_encode(zse, 30);

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	ok = transport->functions.bridge_modes(zse, &rc);

	if (!ok) {
		return rc;
	}

	ok = zcbor_list_end_encode(zse, 30);

	return MGMT_RETURN_CHECK(ok);
}

static int transport_mgmt_config_details(struct smp_streamer *ctxt)
{
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok = true;
	size_t decoded = 0;
	uint32_t transport_id = 0;
	uint32_t mode = 0;
	struct smp_transport *transport;

	struct zcbor_map_decode_key_val cbor_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("transport", zcbor_uint32_decode, &transport_id),
		ZCBOR_MAP_DECODE_KEY_DECODER("mode", zcbor_uint32_decode, &mode),
	};

	ok = zcbor_map_decode_bulk(zsd, cbor_decode, ARRAY_SIZE(cbor_decode),
				   &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	if (decoded == 0 || !zcbor_map_decode_bulk_key_found(cbor_decode,
				ARRAY_SIZE(cbor_decode), "transport")) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_TRANSPORT);
		return MGMT_ERR_EOK;
	} else if (!zcbor_map_decode_bulk_key_found(cbor_decode,
				ARRAY_SIZE(cbor_decode), "mode")) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_MODE);
		return MGMT_ERR_EOK;
	}

	transport = smp_client_transport_get(transport_id);

	if (transport == NULL) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_TRANSPORT);
		return MGMT_ERR_EOK;
	}

	if (transport->functions.bridge_config_details == NULL) {
		smp_add_cmd_err(zse, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_INFO_FUNCTIONS);
		return MGMT_ERR_EOK;
	}

	ok = zcbor_tstr_put_lit(zse, "configs") &&
	     zcbor_list_start_encode(zse, 30);

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	ok = transport->functions.bridge_config_details(mode, zse, &rc);

	if (!ok) {
		return rc;
	}

	ok = zcbor_list_end_encode(zse, 30);

	return MGMT_RETURN_CHECK(ok);
}
#endif

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
static int transport_mgmt_translate_error_code(uint16_t ret)
{
	int rc;

	switch (ret) {
	case TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_REQUIRED_FUNCTIONS:
	case TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_INFO_FUNCTIONS:
		rc = MGMT_ERR_ENOENT;
		break;

	case TRANSPORT_MGMT_ERR_INVALID_TRANSPORT:
	case TRANSPORT_MGMT_ERR_INVALID_MODE:
	case TRANSPORT_MGMT_ERR_BOTH_TRANSPORT_AND_ALL_PARAMETERS:
	case TRANSPORT_MGMT_ERR_SAME_BRIDGE_DEVICE_DISALLOWED:
		rc = MGMT_ERR_EINVAL;
		break;

	case TRANSPORT_MGMT_ERR_ALL_CONTEXTS_USED:
	case TRANSPORT_MGMT_ERR_NOT_BRIDGED:
		rc = MGMT_ERR_EBADSTATE;
		break;

	case TRANSPORT_MGMT_ERR_UNKNOWN:
	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler transport_mgmt_handlers[] = {
	[TRANSPORT_MGMT_ID_CONNECT] = {
		.mh_read = NULL,
		.mh_write = transport_mgmt_connect,
	},
	[TRANSPORT_MGMT_ID_DISCONNECT] = {
		.mh_read = NULL,
		.mh_write = transport_mgmt_disconnect,
	},
	[TRANSPORT_MGMT_ID_STATUS] = {
		.mh_read = transport_mgmt_status,
		.mh_write = NULL,
	},
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
	[TRANSPORT_MGMT_ID_LIST] = {
		.mh_read = transport_mgmt_list,
		.mh_write = NULL,
	},
	[TRANSPORT_MGMT_ID_GET_MODES] = {
		.mh_read = transport_mgmt_modes,
		.mh_write = NULL,
	},
	[TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS] = {
		.mh_read = transport_mgmt_config_details,
		.mh_write = NULL,
	},
#endif
};

static struct mgmt_group transport_mgmt_group = {
	.mg_handlers = transport_mgmt_handlers,
	.mg_handlers_count = ARRAY_SIZE(transport_mgmt_handlers),
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = transport_mgmt_translate_error_code,
#endif
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	.mg_group_name = "transport mgmt",
#endif
};

static void transport_mgmt_register_group(void)
{
	transport_mgmt_group.mg_group_id = transport_mgmt_group_id();
	mgmt_register_group(&transport_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(transport_mgmt, transport_mgmt_register_group);
