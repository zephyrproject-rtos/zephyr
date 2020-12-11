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

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#ifdef CONFIG_LWM2M_IPSO_ACCELEROMETER_TIMESTAMP
#define ADD_TIMESTAMPS 1
#else
#define ADD_TIMESTAMPS 0
#endif
/* Server resource IDs */
#define ACCEL_X_VALUE_ID			5702
#define ACCEL_Y_VALUE_ID			5703
#define ACCEL_Z_VALUE_ID			5704
#define ACCEL_SENSOR_UNITS_ID			5701
#define ACCEL_MIN_RANGE_VALUE_ID		5603
#define ACCEL_MAX_RANGE_VALUE_ID		5604
#if ADD_TIMESTAMPS
#define ACCEL_TIMESTAMP_ID			5518

#define ACCEL_MAX_ID		7
#else
#define ACCEL_MAX_ID		6
#endif

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_ACCELEROMETER_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with ACCEL_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (ACCEL_MAX_ID)

/* resource state */
struct ipso_accel_data {
	float32_value_t x_value;
	float32_value_t y_value;
	float32_value_t z_value;
	float32_value_t min_range;
	float32_value_t max_range;
};

static struct ipso_accel_data accel_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj accel;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(ACCEL_X_VALUE_ID, R, FLOAT32),
	OBJ_FIELD_DATA(ACCEL_Y_VALUE_ID, R_OPT, FLOAT32),
	OBJ_FIELD_DATA(ACCEL_Z_VALUE_ID, R_OPT, FLOAT32),
	OBJ_FIELD_DATA(ACCEL_SENSOR_UNITS_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(ACCEL_MIN_RANGE_VALUE_ID, R_OPT, FLOAT32),
	OBJ_FIELD_DATA(ACCEL_MAX_RANGE_VALUE_ID, R_OPT, FLOAT32),
#if ADD_TIMESTAMPS
	OBJ_FIELD_DATA(ACCEL_TIMESTAMP_ID, RW_OPT, TIME),
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
	INIT_OBJ_RES_DATA(ACCEL_X_VALUE_ID, res[avail], i, res_inst[avail], j,
			  &accel_data[avail].x_value,
			  sizeof(accel_data[avail].x_value));
	INIT_OBJ_RES_DATA(ACCEL_Y_VALUE_ID, res[avail], i, res_inst[avail], j,
			  &accel_data[avail].y_value,
			  sizeof(accel_data[avail].y_value));
	INIT_OBJ_RES_DATA(ACCEL_Z_VALUE_ID, res[avail], i, res_inst[avail], j,
			  &accel_data[avail].z_value,
			  sizeof(accel_data[avail].z_value));
	INIT_OBJ_RES_OPTDATA(ACCEL_SENSOR_UNITS_ID, res[avail], i,
			     res_inst[avail], j);
	INIT_OBJ_RES_DATA(ACCEL_MIN_RANGE_VALUE_ID, res[avail], i,
			  res_inst[avail], j,
			  &accel_data[avail].min_range,
			  sizeof(accel_data[avail].min_range));
	INIT_OBJ_RES_DATA(ACCEL_MAX_RANGE_VALUE_ID, res[avail], i,
			  res_inst[avail], j,
			  &accel_data[avail].max_range,
			  sizeof(accel_data[avail].max_range));
#if ADD_TIMESTAMPS
	INIT_OBJ_RES_OPTDATA(ACCEL_TIMESTAMP_ID, res[avail], i,
			     res_inst[avail], j);
#endif

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Accelerometer instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_accel_init(const struct device *dev)
{
	accel.obj_id = IPSO_OBJECT_ACCELEROMETER_ID;
	accel.fields = fields;
	accel.field_count = ARRAY_SIZE(fields);
	accel.max_instance_count = ARRAY_SIZE(inst);
	accel.create_cb = accel_create;
	lwm2m_register_obj(&accel);

	return 0;
}

SYS_INIT(ipso_accel_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
