/*
 * Copyright (c) 2025 Savo Saicic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Magnetometer object (3314):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3314.xml
 */

#define LOG_MODULE_NAME net_ipso_magnetometer
#define LOG_LEVEL       CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define MAG_VERSION_MAJOR 1
#define MAG_VERSION_MINOR 0

#define MAG_MAX_ID 5

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_MAGNETOMETER_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with MAG_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT (MAG_MAX_ID)

/* resource state */
struct ipso_mag_data {
	double x_value;
};

static struct ipso_mag_data mag_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj mag;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(X_VALUE_RID, R, FLOAT),
	OBJ_FIELD_DATA(Y_VALUE_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(Z_VALUE_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(COMPASS_DIRECTION_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(SENSOR_UNITS_RID, R_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][MAG_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *mag_create(uint16_t obj_inst_id)
{
	int index, avail = -1, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < ARRAY_SIZE(inst); index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u",
				obj_inst_id);
			return NULL;
		}

		/* Save first available slot index */
		if (avail < 0 && !inst[index].obj) {
			avail = index;
		}
	}

	if (avail < 0) {
		LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	(void)memset(&mag_data[avail], 0, sizeof(mag_data[avail]));

	(void)memset(res[avail], 0, sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(X_VALUE_RID, res[avail], i, res_inst[avail], j, &mag_data[avail].x_value,
			  sizeof(mag_data[avail].x_value));
	INIT_OBJ_RES_OPTDATA(Y_VALUE_RID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(Z_VALUE_RID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(COMPASS_DIRECTION_RID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(SENSOR_UNITS_RID, res[avail], i, res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Magnetometer instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_mag_init(void)
{
	mag.obj_id = IPSO_OBJECT_MAGNETOMETER_ID;
	mag.version_major = MAG_VERSION_MAJOR;
	mag.version_minor = MAG_VERSION_MINOR;
	mag.is_core = false;
	mag.fields = fields;
	mag.field_count = ARRAY_SIZE(fields);
	mag.max_instance_count = ARRAY_SIZE(inst);
	mag.create_cb = mag_create;
	lwm2m_register_obj(&mag);

	return 0;
}

LWM2M_OBJ_INIT(ipso_mag_init);
