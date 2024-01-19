/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Accelerometer object (3313):
 * http://www.openmobilealliance.org/tech/profiles/lwm2m/3313.xml
 */

#define LOG_MODULE_NAME net_ipso_accel
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define ACCEL_VERSION_MAJOR 1

#if defined(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)
#define ACCEL_VERSION_MINOR 1
#define ACCEL_MAX_ID 11
#else
#define ACCEL_VERSION_MINOR 0
#define ACCEL_MAX_ID 6
#endif /* defined(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1) */

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_ACCELEROMETER_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with ACCEL_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (ACCEL_MAX_ID)

/* resource state */
struct ipso_accel_data {
	double x_value;
	double y_value;
	double z_value;
	double min_range;
	double max_range;
};

static struct ipso_accel_data accel_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj accel;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(X_VALUE_RID, R, FLOAT),
	OBJ_FIELD_DATA(Y_VALUE_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(Z_VALUE_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(SENSOR_UNITS_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(MIN_RANGE_VALUE_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(MAX_RANGE_VALUE_RID, R_OPT, FLOAT),
#if defined(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)
	OBJ_FIELD_DATA(APPLICATION_TYPE_RID, RW_OPT, STRING),
	OBJ_FIELD_DATA(TIMESTAMP_RID, R_OPT, TIME),
	OBJ_FIELD_DATA(FRACTIONAL_TIMESTAMP_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(MEASUREMENT_QUALITY_INDICATOR_RID, R_OPT, U8),
	OBJ_FIELD_DATA(MEASUREMENT_QUALITY_LEVEL_RID, R_OPT, U8),
#endif
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][ACCEL_MAX_ID];
static struct lwm2m_engine_res_inst
		res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *accel_create(uint16_t obj_inst_id)
{
	int index, avail = -1, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < ARRAY_SIZE(inst); index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u", obj_inst_id);
			return NULL;
		}

		/* Save first available slot index */
		if (avail < 0 && !inst[index].obj) {
			avail = index;
		}
	}

	if (avail < 0) {
		LOG_ERR("Can not create instance - no more room: %u",
			obj_inst_id);
		return NULL;
	}

	/* Set default values */
	(void)memset(&accel_data[avail], 0, sizeof(accel_data[avail]));

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(X_VALUE_RID, res[avail], i, res_inst[avail], j,
			  &accel_data[avail].x_value,
			  sizeof(accel_data[avail].x_value));
	INIT_OBJ_RES_DATA(Y_VALUE_RID, res[avail], i, res_inst[avail], j,
			  &accel_data[avail].y_value,
			  sizeof(accel_data[avail].y_value));
	INIT_OBJ_RES_DATA(Z_VALUE_RID, res[avail], i, res_inst[avail], j,
			  &accel_data[avail].z_value,
			  sizeof(accel_data[avail].z_value));
	INIT_OBJ_RES_OPTDATA(SENSOR_UNITS_RID, res[avail], i,
			     res_inst[avail], j);
	INIT_OBJ_RES_DATA(MIN_RANGE_VALUE_RID, res[avail], i, res_inst[avail],
			  j, &accel_data[avail].min_range,
			  sizeof(accel_data[avail].min_range));
	INIT_OBJ_RES_DATA(MAX_RANGE_VALUE_RID, res[avail], i, res_inst[avail],
			  j, &accel_data[avail].max_range,
			  sizeof(accel_data[avail].max_range));
#if defined(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)
	INIT_OBJ_RES_OPTDATA(APPLICATION_TYPE_RID, res[avail], i,
			     res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(TIMESTAMP_RID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(FRACTIONAL_TIMESTAMP_RID, res[avail], i,
			     res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(MEASUREMENT_QUALITY_INDICATOR_RID, res[avail],
			     i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(MEASUREMENT_QUALITY_LEVEL_RID, res[avail], i,
			     res_inst[avail], j);
#endif

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Accelerometer instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_accel_init(void)
{
	accel.obj_id = IPSO_OBJECT_ACCELEROMETER_ID;
	accel.version_major = ACCEL_VERSION_MAJOR;
	accel.version_minor = ACCEL_VERSION_MINOR;
	accel.is_core = false;
	accel.fields = fields;
	accel.field_count = ARRAY_SIZE(fields);
	accel.max_instance_count = ARRAY_SIZE(inst);
	accel.create_cb = accel_create;
	lwm2m_register_obj(&accel);

	return 0;
}

LWM2M_OBJ_INIT(ipso_accel_init);
