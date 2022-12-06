/*
 * Copyright (c) 2022 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* LwM2M LPWAN Communication - Object ID 3412
 * Table view:
 * http://devtoolkit.openmobilealliance.org/OEditor/LWMOView?url=https%3A%2F%2Fraw.githubusercontent.com%2FOpenMobileAlliance%2Flwm2m-registry%2Fprod%2F3412.xml
 * XML file: https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3412.xml
 */

#define LOG_MODULE_NAME net_lwm2m_obj_lpwan
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "lwm2m_engine.h"
#include "lwm2m_obj_lpwan.h"
#include "lwm2m_object.h"

#include <stdio.h>
#include <string.h>

#include <zephyr/init.h>

#define LPWAN_VERSION_MAJOR 1
#define LPWAN_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT	      CONFIG_LWM2M_LPWAN_INSTANCE_COUNT
#define IPV4_ADDRESS_MAX	      CONFIG_LWM2M_LPWAN_IPV4_ADDRESS_MAX
#define IPV6_ADDRESS_MAX	      CONFIG_LWM2M_LPWAN_IPV6_ADDRESS_MAX
#define NETWORK_ADDRESS_MAX	      CONFIG_LWM2M_LPWAN_NETWORK_ADDRESS_MAX
#define SECONDARY_NETWORK_ADDRESS_MAX CONFIG_LWM2M_LPWAN_SECONDARY_NETWORK_ADDRESS_MAX
#define PEER_ADDRESS_MAX	      CONFIG_LWM2M_LPWAN_PEER_ADDRESS_MAX
#define MULTICAST_GRP_ADDRESS_MAX     CONFIG_LWM2M_LPWAN_MULTICAST_GRP_ADDRESS_MAX
#define MULTICAST_GRP_KEY_MAX	      CONFIG_LWM2M_LPWAN_MULTICAST_GRP_KEY_MAX

/*
 * Calculate resource instances as follows:
 * start with LPWAN_MAX_ID
 * subtract EXEC resources (0)
 * subtract MULTI resources because their counts include 0 resource (7)
 * add LPWAN_IPV4_ADDRESS_ID resource instances
 * add LPWAN_IPV6_ADDRESS_ID resource instances
 * add LPWAN_NETWORK_ADDRESS_ID resource instances
 * add LPWAN_SECONDARY_ADDRESS_ID resource instances
 * add LPWAN_PEER_ADDRESS_ID resource instances
 * add LPWAN_MULTICAST_GRP_ADDRESS_ID resource instances
 * add LPWAN_MULTICAST_GRP_KEY_ID resource instances
 */
#define NUMBER_EXEC_RESOURCES  0
#define NUMBER_MULTI_RESOURCES 7
#define RESOURCE_INSTANCE_COUNT                                                                    \
	(LPWAN_MAX_ID - NUMBER_EXEC_RESOURCES - NUMBER_MULTI_RESOURCES + IPV4_ADDRESS_MAX +        \
	 IPV6_ADDRESS_MAX + NETWORK_ADDRESS_MAX + SECONDARY_NETWORK_ADDRESS_MAX +                  \
	 PEER_ADDRESS_MAX + MULTICAST_GRP_ADDRESS_MAX + MULTICAST_GRP_KEY_MAX)

static struct lwm2m_engine_obj lpwan_obj;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LPWAN_NETWORK_TYPE_ID, R_OPT, STRING),	    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_IPV4_ADDRESS_ID, RW_OPT, STRING),	    /* RW - Multiple - Optional  */
	OBJ_FIELD_DATA(LPWAN_IPV6_ADDRESS_ID, RW_OPT, STRING),	    /* RW - Multiple - Optional  */
	OBJ_FIELD_DATA(LPWAN_NETWORK_ADDRESS_ID, RW_OPT, STRING),   /* RW - Multiple - Optional  */
	OBJ_FIELD_DATA(LPWAN_SECONDARY_ADDRESS_ID, RW_OPT, STRING), /* RW - Multiple - Optional  */
	OBJ_FIELD_DATA(LPWAN_MAC_ADDRESS_ID, RW, STRING),	    /* RW - Single   - Mandatory */
	OBJ_FIELD_DATA(LPWAN_PEER_ADDRESS_ID, R_OPT, STRING),	    /* R  - Multiple - Optional  */
	OBJ_FIELD_DATA(LPWAN_MULTICAST_GRP_ADDRESS_ID, RW_OPT,
		       STRING),					    /* RW - Multiple - Optional  */
	OBJ_FIELD_DATA(LPWAN_MULTICAST_GRP_KEY_ID, RW_OPT, STRING), /* RW - Multiple - Optional  */
	OBJ_FIELD_DATA(LPWAN_DATA_RATE_ID, RW_OPT, INT),	    /* RW - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_TRANSMIT_POWER_ID, R_OPT, FLOAT),	    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_FREQUENCY_ID, RW_OPT, FLOAT),	    /* RW - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_SESSION_TIME_ID, RW_OPT, TIME),	    /* RW - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_SESSION_DURATION_ID, R_OPT, TIME),	    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_MESH_NODE_ID, RW_OPT, BOOL),	    /* RW - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_MAX_REPEAT_TIME_ID, RW_OPT, INT),	    /* RW - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_NUMBER_REPEATS_ID, R_OPT, INT),	    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_SIGNAL_NOISE_RATIO_ID, R_OPT, FLOAT),  /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_COMM_FAILURE_ID, R_OPT, BOOL),	    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_RSSI_ID, R_OPT, FLOAT),		    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_IMSI_ID, R_OPT, STRING),		    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_IMEI_ID, R_OPT, STRING),		    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_CURRENT_COMM_OPERATOR_ID, R_OPT,
		       STRING),					    /* R  - Single   - Optional  */
	OBJ_FIELD_DATA(LPWAN_IC_CARD_IDENTIFIER_ID, R_OPT, STRING), /* R  - Single   - Optional  */
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][LPWAN_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];
static char mac[MAX_INSTANCE_COUNT][MAC_ADDRESS_LEN];

static struct lwm2m_engine_obj_inst *lpwan_create(uint16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Cannot create instance - already existing: %u", obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (!inst[index].obj) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	(void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	(void)memset(mac[index], 0, MAC_ADDRESS_LEN);

	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPTDATA(LPWAN_NETWORK_TYPE_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_MULTI_OPTDATA(LPWAN_IPV4_ADDRESS_ID, res[index], i, res_inst[index], j,
				   IPV4_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(LPWAN_IPV6_ADDRESS_ID, res[index], i, res_inst[index], j,
				   IPV6_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(LPWAN_NETWORK_ADDRESS_ID, res[index], i, res_inst[index], j,
				   NETWORK_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(LPWAN_SECONDARY_ADDRESS_ID, res[index], i, res_inst[index], j,
				   SECONDARY_NETWORK_ADDRESS_MAX, false);
	INIT_OBJ_RES_DATA(LPWAN_MAC_ADDRESS_ID, res[index], i, res_inst[index], j, mac[index],
			  MAC_ADDRESS_LEN);
	INIT_OBJ_RES_MULTI_OPTDATA(LPWAN_PEER_ADDRESS_ID, res[index], i, res_inst[index], j,
				   PEER_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(LPWAN_MULTICAST_GRP_ADDRESS_ID, res[index], i, res_inst[index],
				   j, MULTICAST_GRP_ADDRESS_MAX, false);
	INIT_OBJ_RES_MULTI_OPTDATA(LPWAN_MULTICAST_GRP_KEY_ID, res[index], i, res_inst[index], j,
				   MULTICAST_GRP_KEY_MAX, false);
	INIT_OBJ_RES_OPTDATA(LPWAN_DATA_RATE_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_TRANSMIT_POWER_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_FREQUENCY_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_SESSION_TIME_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_SESSION_DURATION_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_MESH_NODE_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_MAX_REPEAT_TIME_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_NUMBER_REPEATS_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_SIGNAL_NOISE_RATIO_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_COMM_FAILURE_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_RSSI_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_IMSI_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_IMEI_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_CURRENT_COMM_OPERATOR_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(LPWAN_IC_CARD_IDENTIFIER_ID, res[index], i, res_inst[index], j);

	inst[index].resources = res[index];
	inst[index].resource_count = i;

	LOG_DBG("Create access control instance: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_lpwan_init(const struct device *dev)
{
	lpwan_obj.obj_id = LWM2M_OBJECT_LPWAN_ID;
	lpwan_obj.version_major = LPWAN_VERSION_MAJOR;
	lpwan_obj.version_minor = LPWAN_VERSION_MINOR;
	lpwan_obj.is_core = true;
	lpwan_obj.fields = fields;
	lpwan_obj.field_count = ARRAY_SIZE(fields);
	lpwan_obj.max_instance_count = MAX_INSTANCE_COUNT;
	lpwan_obj.create_cb = lpwan_create;
	lpwan_obj.delete_cb = NULL; /* TODO ? */
	lwm2m_register_obj(&lpwan_obj);

	return 0;
}

SYS_INIT(lwm2m_lpwan_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
