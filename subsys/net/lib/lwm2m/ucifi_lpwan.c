/*
 * Copyright (c) 2022 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for uCIFI LPWAN object (3412):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3412.xml
 */

#define LOG_MODULE_NAME net_ucifi_lpwan
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>
#include <string.h>

#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "ucifi_lpwan.h"

#define LPWAN_VERSION_MAJOR 1
#define LPWAN_VERSION_MINOR 0

/* clang-format off */
#define MAX_INSTANCE_COUNT             CONFIG_LWM2M_UCIFI_LPWAN_INSTANCE_COUNT
#define IPV4_ADDRESS_MAX               CONFIG_LWM2M_UCIFI_LPWAN_IPV4_ADDRESS_MAX
#define IPV6_ADDRESS_MAX               CONFIG_LWM2M_UCIFI_LPWAN_IPV6_ADDRESS_MAX
#define NETWORK_ADDRESS_MAX            CONFIG_LWM2M_UCIFI_LPWAN_NETWORK_ADDRESS_MAX
#define SECONDARY_NETWORK_ADDRESS_MAX  CONFIG_LWM2M_UCIFI_LPWAN_SECONDARY_NETWORK_ADDRESS_MAX
#define PEER_ADDRESS_MAX               CONFIG_LWM2M_UCIFI_LPWAN_PEER_ADDRESS_MAX
#define MULTICAST_GRP_ADDRESS_MAX      CONFIG_LWM2M_UCIFI_LPWAN_MULTICAST_GRP_ADDRESS_MAX
#define MULTICAST_GRP_KEY_MAX          CONFIG_LWM2M_UCIFI_LPWAN_MULTICAST_GRP_KEY_MAX
/* clang-format on */

/*
 * Calculate resource instances as follows:
 * start with UCIFI_LPWAN_MAX_RID
 * subtract EXEC resources (0)
 * subtract MULTI resources because their counts include 0 resource (7)
 * add UCIFI_LPWAN_IPV4_ADDRESS_RID resource instances
 * add UCIFI_LPWAN_IPV6_ADDRESS_RID resource instances
 * add UCIFI_LPWAN_NETWORK_ADDRESS_RID resource instances
 * add UCIFI_LPWAN_SECONDARY_ADDRESS_RID resource instances
 * add UCIFI_LPWAN_PEER_ADDRESS_RID resource instances
 * add UCIFI_LPWAN_MULTICAST_GRP_ADDRESS_RID resource instances
 * add UCIFI_LPWAN_MULTICAST_GRP_KEY_RID resource instances
 */
#define NUMBER_EXEC_RESOURCES  0
#define NUMBER_MULTI_RESOURCES 7
#define RESOURCE_INSTANCE_COUNT                                                                    \
	(UCIFI_LPWAN_MAX_RID - NUMBER_EXEC_RESOURCES - NUMBER_MULTI_RESOURCES + IPV4_ADDRESS_MAX + \
	 IPV6_ADDRESS_MAX + NETWORK_ADDRESS_MAX + SECONDARY_NETWORK_ADDRESS_MAX +                  \
	 PEER_ADDRESS_MAX + MULTICAST_GRP_ADDRESS_MAX + MULTICAST_GRP_KEY_MAX)

static struct lwm2m_engine_obj lpwan;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(UCIFI_LPWAN_NETWORK_TYPE_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_IPV4_ADDRESS_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_IPV6_ADDRESS_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_NETWORK_ADDRESS_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_SECONDARY_ADDRESS_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_MAC_ADDRESS_RID, RW, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_PEER_ADDRESS_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_MULTICAST_GRP_ADDRESS_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_MULTICAST_GRP_KEY_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_DATA_RATE_RID, RW_OPT, INT),
	OBJ_FIELD_DATA(UCIFI_LPWAN_TRANSMIT_POWER_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(UCIFI_LPWAN_FREQUENCY_RID, RW_OPT, FLOAT),
	OBJ_FIELD_DATA(UCIFI_LPWAN_SESSION_TIME_RID, RW_OPT, TIME),
	OBJ_FIELD_DATA(UCIFI_LPWAN_SESSION_DURATION_RID, R_OPT, TIME),
	OBJ_FIELD_DATA(UCIFI_LPWAN_MESH_NODE_RID, RW_OPT, BOOL),
	OBJ_FIELD_DATA(UCIFI_LPWAN_MAX_REPEAT_TIME_RID, RW_OPT, INT),
	OBJ_FIELD_DATA(UCIFI_LPWAN_NUMBER_REPEATS_RID, R_OPT, INT),
	OBJ_FIELD_DATA(UCIFI_LPWAN_SIGNAL_NOISE_RATIO_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(UCIFI_LPWAN_COMM_FAILURE_RID, R_OPT, BOOL),
	OBJ_FIELD_DATA(UCIFI_LPWAN_RSSI_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(UCIFI_LPWAN_IMSI_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_IMEI_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_COMM_OPERATOR_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(UCIFI_LPWAN_IC_CARD_IDENTIFIER_RID, R_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][UCIFI_LPWAN_MAX_RID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

/* resource state variables */
static char mac[MAX_INSTANCE_COUNT][MAC_ADDRESS_SIZE];

static struct lwm2m_engine_obj_inst *lpwan_create(uint16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if ((inst[index].obj != NULL) && (inst[index].obj_inst_id == obj_inst_id)) {
			LOG_ERR("Can not create instance - already existing: %u", obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj == NULL) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Reset to uninitialised values */
	(void)memset(res[index], 0, sizeof(res[index]));
	mac[index][0] = '\0';

	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_NETWORK_TYPE_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_MULTI_OPTDATA(UCIFI_LPWAN_IPV4_ADDRESS_RID, res[index], i, res_inst[index], j,
				   IPV4_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(UCIFI_LPWAN_IPV6_ADDRESS_RID, res[index], i, res_inst[index], j,
				   IPV6_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(UCIFI_LPWAN_NETWORK_ADDRESS_RID, res[index], i, res_inst[index],
				   j, NETWORK_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(UCIFI_LPWAN_SECONDARY_ADDRESS_RID, res[index], i,
				   res_inst[index], j, SECONDARY_NETWORK_ADDRESS_MAX, false);
	INIT_OBJ_RES_DATA(UCIFI_LPWAN_MAC_ADDRESS_RID, res[index], i, res_inst[index], j,
			  mac[index], MAC_ADDRESS_SIZE);
	INIT_OBJ_RES_MULTI_OPTDATA(UCIFI_LPWAN_PEER_ADDRESS_RID, res[index], i, res_inst[index], j,
				   PEER_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(UCIFI_LPWAN_MULTICAST_GRP_ADDRESS_RID, res[index], i,
				   res_inst[index], j, MULTICAST_GRP_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(UCIFI_LPWAN_MULTICAST_GRP_KEY_RID, res[index], i,
				   res_inst[index], j, MULTICAST_GRP_KEY_MAX, false);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_DATA_RATE_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_TRANSMIT_POWER_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_FREQUENCY_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_SESSION_TIME_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_SESSION_DURATION_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_MESH_NODE_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_MAX_REPEAT_TIME_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_NUMBER_REPEATS_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_SIGNAL_NOISE_RATIO_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_COMM_FAILURE_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_RSSI_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_IMSI_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_IMEI_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_COMM_OPERATOR_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(UCIFI_LPWAN_IC_CARD_IDENTIFIER_RID, res[index], i, res_inst[index], j);

	inst[index].resources = res[index];
	inst[index].resource_count = i;

	LOG_DBG("Create access control instance: %d", obj_inst_id);
	return &inst[index];
}

static int ucifi_lpwan_init(void)
{
	lpwan.obj_id = LWM2M_UCIFI_LPWAN_ID;
	lpwan.version_major = LPWAN_VERSION_MAJOR;
	lpwan.version_minor = LPWAN_VERSION_MINOR;
	lpwan.is_core = false;
	lpwan.fields = fields;
	lpwan.field_count = ARRAY_SIZE(fields);
	lpwan.max_instance_count = MAX_INSTANCE_COUNT;
	lpwan.create_cb = lpwan_create;
	lwm2m_register_obj(&lpwan);

	return 0;
}

SYS_INIT(ucifi_lpwan_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
