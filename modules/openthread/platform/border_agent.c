/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <platform-zephyr.h>
#include <openthread/backbone_router_ftd.h>
#include <openthread/border_agent.h>
#include <openthread/border_routing.h>
#include <openthread/cli.h>
#include <openthread/dataset.h>
#include <openthread/dns.h>
#include <openthread/ip6.h>
#include <openthread/mdns.h>
#include <openthread/platform/memory.h>
#include <openthread/platform/radio.h>
#include <openthread/thread.h>
#include "common/code_utils.hpp"
#include "openthread_border_router.h"
#include <zephyr/sys/byteorder.h>

#define BORDER_AGENT_PORT 49152
#define BACKBONE_UDP_PORT 61631

#define MAX_TXT_ENTRIES_NUMBER 18

typedef enum {
	conn_not_allowed = 0,
	conn_pskc,
	conn_pskd,
	conn_vendor,
	conn_x509,
} connection_mode;

typedef enum {
	if_not_initialized = 0,
	if_initialized,
	if_active,
} thread_interface_status;

typedef enum {
	infrequent_availability = 0,
	high_availability,
} availability;

typedef enum {
	thread_role_detached = 0,
	thread_role_end_device,
	thread_role_router,
	thread_role_leader
} thread_role;

typedef struct border_agent_state_bitmap {
	uint32_t connection_mode: 3;
	uint32_t thread_if_status: 2;
	uint32_t availability: 2;
	uint32_t bbr_fcn_status_is_active: 1;
	uint32_t bbr_fcn_status_is_primary: 1;
	uint32_t thread_role: 2;
	uint32_t epskc_supported: 1;
} state_bitmap;

typedef struct meshcop_values {
	uint64_t active_timestamp;
	uint32_t partition_id;
	uint32_t bitmap_value;
	uint16_t port;
	uint8_t bbr_seq_no;
	uint8_t txt_omr_prefix[OT_IP6_PREFIX_SIZE + 1];
} meshcop_values;

static otInstance *ot_instance;
static bool border_agent_is_init;

static otMdnsService meshcop_service;
static const char meshcop_service_label[] = "_meshcop._udp";

static uint8_t add_meshcop_txt_data(otDnsTxtEntry *txt_entries, meshcop_values *meshcop_values,
				    otInstance *ot_instance);
static void handle_thread_state_changed(otChangedFlags flags, void *aContext);
static void advertise_meshcop_service(otInstance *ot_instance);
static state_bitmap get_state_bitmap(otInstance *ot_instance);
static void handle_meshcop_registration_cb(otInstance *ot_instance, otMdnsRequestId request_id,
					   otError error);
static uint32_t bitmap_to_u32(state_bitmap bitmap);
static const char *get_thread_version_as_string(void);

void border_agent_init(otInstance *instance, const char *host_name)
{
	ot_instance = instance;

	if (!border_agent_is_init) {
		meshcop_service.mHostName = host_name;
		meshcop_service.mServiceInstance =
			create_base_name(ot_instance, base_service_instance_name, true);
		meshcop_service.mServiceType = meshcop_service_label;

		advertise_meshcop_service(instance);

		otSetStateChangedCallback(instance, handle_thread_state_changed, instance);

		border_agent_is_init = true;
	}
}

void border_agent_deinit(void)
{
	(void)otMdnsUnregisterService(ot_instance, &meshcop_service);
	border_agent_is_init = false;
}

static void advertise_meshcop_service(otInstance *instance)
{
	otDnsTxtEntry txt_entries[MAX_TXT_ENTRIES_NUMBER];
	meshcop_values meshcop_values = {0};
	uint8_t num_txt_entries = 0;

	num_txt_entries = add_meshcop_txt_data(txt_entries, &meshcop_values, instance);

	uint8_t txt_buffer[255] = {0};
	uint32_t txt_buffer_offset = 0;

	for (uint32_t i = 0; i < num_txt_entries; i++) {
		uint32_t key_size = strlen(txt_entries[i].mKey);
		/* add TXT entry len + 1 is for '=' */
		*(txt_buffer + txt_buffer_offset++) = key_size + txt_entries[i].mValueLength + 1;

		/* add TXT entry key */
		memcpy(txt_buffer + txt_buffer_offset, txt_entries[i].mKey, key_size);
		txt_buffer_offset += key_size;

		/* add TXT entry value if pointer is not null, if pointer is null it means
		 *we have bool value
		 */
		if (txt_entries[i].mValue) {
			*(txt_buffer + txt_buffer_offset++) = '=';
			memcpy(txt_buffer + txt_buffer_offset, txt_entries[i].mValue,
			       txt_entries[i].mValueLength);
			txt_buffer_offset += txt_entries[i].mValueLength;
		}
	}
	meshcop_service.mTxtData = txt_buffer;
	meshcop_service.mTxtDataLength = txt_buffer_offset;

	/* While Thread interface status is different from '2' (interface active),
	 *Border Agent will be stopped. In this case, publish the service with
	 *BORDER_AGENT_PORT (49152, min value of ephemeral ports range). Store the
	 *first allocated port using 'otBorderAgentGetUdpPort()', as ephemeral key
	 *service will require an ephemeral port, and in that case,
	 *'otBorderAgentGetUdpPort()' will return the port value obtained by ephemeral
	 *key service.
	 */
	if (otBorderAgentGetState(instance) != OT_BORDER_AGENT_STATE_STOPPED) {
		if (meshcop_service.mPort == BORDER_AGENT_PORT) {
			meshcop_service.mPort = otBorderAgentGetUdpPort(instance);
		}
	} else {
		meshcop_service.mPort = BORDER_AGENT_PORT;
	}

	otMdnsRegisterService(instance, &meshcop_service, 0, handle_meshcop_registration_cb);
}

static void handle_thread_state_changed(otChangedFlags flags, void *context)
{
	if (flags &
	    (OT_CHANGED_THREAD_ROLE | OT_CHANGED_THREAD_EXT_PANID | OT_CHANGED_THREAD_NETWORK_NAME |
	     OT_CHANGED_THREAD_BACKBONE_ROUTER_STATE | OT_CHANGED_THREAD_NETDATA)) {
		advertise_meshcop_service((otInstance *)context);
	}
}

static uint8_t add_meshcop_txt_data(otDnsTxtEntry *txt_entries, meshcop_values *meshcop_values,
				    otInstance *ot_instance)
{
	uint8_t i = 0;
	state_bitmap bitmap = get_state_bitmap(ot_instance);

	meshcop_values->bitmap_value = BSWAP_32(bitmap_to_u32(bitmap));

	/* RV TXT */
	txt_entries[i].mKey = "rv";
	txt_entries[i].mValue = (uint8_t *)"1";
	txt_entries[i].mValueLength = strlen("1");
	i++;

	/* Thread Specification Version TXT */
	const char *version = get_thread_version_as_string();

	txt_entries[i].mKey = "tv";
	txt_entries[i].mValue = (uint8_t *)(version);
	txt_entries[i].mValueLength = strlen(version);
	i++;

	/* State bitmap TXT (value is binary encoded) */
	txt_entries[i].mKey = "sb";
	txt_entries[i].mValue = (uint8_t *)&meshcop_values->bitmap_value;
	txt_entries[i].mValueLength = sizeof(meshcop_values->bitmap_value);
	i++;

	/* Network name TXT */
	txt_entries[i].mKey = "nn";
	txt_entries[i].mValue = (uint8_t *)otThreadGetNetworkName(ot_instance);
	txt_entries[i].mValueLength = strlen(otThreadGetNetworkName(ot_instance));
	i++;

	/* Extended PAN Id TXT (value is binary encoded) */
	txt_entries[i].mKey = "xp";
	txt_entries[i].mValue = (uint8_t *)otThreadGetExtendedPanId(ot_instance)->m8;
	txt_entries[i].mValueLength = sizeof(otThreadGetExtendedPanId(ot_instance)->m8);
	i++;

	/* Vendor name TXT */
	txt_entries[i].mKey = "vn";
	txt_entries[i].mValue = (uint8_t *)VENDOR_NAME;
	txt_entries[i].mValueLength = strlen(VENDOR_NAME);
	i++;

	/* Model name TXT */
	txt_entries[i].mKey = "mn";

	txt_entries[i].mValue = (uint8_t *)"BorderRouter";
	txt_entries[i].mValueLength = strlen("BorderRouter");
	i++;

	/* Active timestamp TXT and Partition Id TXT (values binary encoded, shall be
	 * included only if Thread Interface Status is set to '2')
	 */
	if (bitmap.thread_if_status == if_active) {
		otOperationalDataset dataset;
		uint16_t ticks_and_u_part = 0;

		otDatasetGetActive(ot_instance, &dataset);

		/* setting ticks part; clearing all but last bit and then set the most
		 * significant 15 bits.
		 */
		ticks_and_u_part = (BSWAP_16(ticks_and_u_part) & ~0xFFFE) |
				   ((dataset.mActiveTimestamp.mTicks << 1) & 0xFFFE);
		/* setting U part; */
		ticks_and_u_part = (ticks_and_u_part & 0xFFFE) |
				   (dataset.mActiveTimestamp.mAuthoritative << 0);

		meshcop_values->active_timestamp = BSWAP_64(
			(dataset.mActiveTimestamp.mSeconds << 16UL) | (uint64_t)ticks_and_u_part);

		txt_entries[i].mKey = "at";
		txt_entries[i].mValue = (uint8_t *)&meshcop_values->active_timestamp;
		txt_entries[i].mValueLength = sizeof(meshcop_values->active_timestamp);
		i++;

		meshcop_values->partition_id = BSWAP_32(otThreadGetPartitionId(ot_instance));
		txt_entries[i].mKey = "pt";
		txt_entries[i].mValue = (uint8_t *)&meshcop_values->partition_id;
		txt_entries[i].mValueLength = sizeof(meshcop_values->partition_id);
		i++;
	}

	/* Domain Name */
	txt_entries[i].mKey = "dn";
	txt_entries[i].mValue = (uint8_t *)otThreadGetDomainName(ot_instance);
	txt_entries[i].mValueLength = strlen(otThreadGetDomainName(ot_instance));
	i++;

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
	/* BBR Sequence number (binary encoded), BBR Port TXT (binary
	 * encoded)
	 */
	if (bitmap.bbr_fcn_status_is_active) {
		otBackboneRouterConfig config;

		otBackboneRouterGetConfig(ot_instance, &config);
		meshcop_values->bbr_seq_no = config.mSequenceNumber;

		txt_entries[i].mKey = "sq";
		txt_entries[i].mValue = (uint8_t *)&meshcop_values->bbr_seq_no;
		txt_entries[i].mValueLength = sizeof(meshcop_values->bbr_seq_no);
		i++;

		meshcop_values->port = BSWAP_16(BACKBONE_UDP_PORT);

		txt_entries[i].mKey = "bb";
		txt_entries[i].mValue = (uint8_t *)&meshcop_values->port;
		txt_entries[i].mValueLength = sizeof(meshcop_values->port);
		i++;
	}
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
	/* OMR TXT (binary encoded) */
	otIp6Prefix prefix;
	otRoutePreference preference;

	if (otBorderRoutingGetFavoredOmrPrefix(ot_instance, &prefix, &preference) ==
	    OT_ERROR_NONE) {
		meshcop_values->txt_omr_prefix[0] = prefix.mLength;
		memcpy(meshcop_values->txt_omr_prefix + 1, prefix.mPrefix.mFields.m8,
		       (prefix.mLength + 7) / 8);

		txt_entries[i].mKey = "omr";
		txt_entries[i].mValue = (uint8_t *)meshcop_values->txt_omr_prefix;
		txt_entries[i].mValueLength = sizeof(meshcop_values->txt_omr_prefix);
		i++;
	}

#endif

	/* Extended Address TXT (binary encoded) */
	txt_entries[i].mKey = "xa";
	txt_entries[i].mValue = (uint8_t *)otLinkGetExtendedAddress(ot_instance)->m8;
	txt_entries[i].mValueLength = sizeof(otLinkGetExtendedAddress(ot_instance)->m8);
	i++;

	/* Border Agent ID (binary encoded) */
	otBorderAgentId id;

	otBorderAgentGetId(ot_instance, &id);
	txt_entries[i].mKey = "id";
	txt_entries[i].mValue = (uint8_t *)id.mId;
	txt_entries[i].mValueLength = sizeof(otBorderAgentId);
	i++;

	return i;
}

static state_bitmap get_state_bitmap(otInstance *ot_instance)
{
	state_bitmap bitmap = {0};

	bitmap.connection_mode = conn_pskc;
	bitmap.availability = high_availability;

	switch (otThreadGetDeviceRole(ot_instance)) {
	case OT_DEVICE_ROLE_DISABLED:
		bitmap.thread_if_status = if_not_initialized;
		bitmap.thread_role = thread_role_detached;
		break;
	case OT_DEVICE_ROLE_DETACHED:
		bitmap.thread_if_status = if_initialized;
		bitmap.thread_role = thread_role_detached;
		break;
	case OT_DEVICE_ROLE_CHILD:
		bitmap.thread_if_status = if_active;
		bitmap.thread_role = thread_role_end_device;
		break;
	case OT_DEVICE_ROLE_ROUTER:
		bitmap.thread_if_status = if_active;
		bitmap.thread_role = thread_role_router;
		break;
	case OT_DEVICE_ROLE_LEADER:
		bitmap.thread_if_status = if_active;
		bitmap.thread_role = thread_role_leader;
		break;

	default:
		bitmap.thread_if_status = if_active;
	}

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
	if (bitmap.thread_if_status == if_active) {
		bitmap.bbr_fcn_status_is_active =
			otBackboneRouterGetState(ot_instance) != OT_BACKBONE_ROUTER_STATE_DISABLED;
		bitmap.bbr_fcn_status_is_primary =
			otBackboneRouterGetState(ot_instance) == OT_BACKBONE_ROUTER_STATE_PRIMARY;
	}
#endif

	return bitmap;
}

static uint32_t bitmap_to_u32(state_bitmap bitmap)
{
	uint32_t result = 0;

	result |= bitmap.connection_mode << 0;
	result |= bitmap.thread_if_status << 3;
	result |= bitmap.availability << 5;
	result |= bitmap.bbr_fcn_status_is_active << 7;
	result |= bitmap.bbr_fcn_status_is_primary << 8;
	result |= bitmap.thread_role << 9;
	result |= bitmap.epskc_supported << 11;

	return result;
}

static void handle_meshcop_registration_cb(otInstance *ot_instance, otMdnsRequestId request_id,
					   otError error)
{
	if (error != OT_ERROR_NONE) {
		meshcop_service.mServiceInstance =
			create_alternative_base_name((char *)meshcop_service.mServiceInstance);
		advertise_meshcop_service(ot_instance);
	}
}

static const char *get_thread_version_as_string(void)
{
	const char *version = NULL;

	switch (otThreadGetVersion()) {
	case OT_THREAD_VERSION_1_1:
		version = "1.1.1";
		break;
	case OT_THREAD_VERSION_1_2:
		version = "1.2.1";
		break;
	case OT_THREAD_VERSION_1_3:
		version = "1.3.0";
		break;
	case OT_THREAD_VERSION_1_4:
		version = "1.4.0";
		break;
	default:
		break;
	}

	return version;
}
