/*
 * Redfish Sensor collection, populated at runtime from the BMC sensor
 * registry so that a product can publish any number of sensors without
 * touching the BMC core.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc/redfish.h>
#include <zephyr/mgmt/bmc/sensor.h>

#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/*** /redfish/v1/Chassis/1/Sensors ***/
static int sensor_collection_get(struct bmc_redfish_ctx *ctx)
{
	bool first = true;
	int ret;

	ret = redfish_collection_open(ctx, REDFISH_URI_SENSORS,
				      "#SensorCollection.SensorCollection",
				      "Chassis Sensor Collection", bmc_sensor_count());
	if (ret < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	BMC_SENSOR_FOREACH(sensor) {
		ret = redfish_collection_add(ctx, first, REDFISH_URI_SENSORS "/%s", sensor->id);
		if (ret < 0) {
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}

		first = false;
	}

	if (redfish_collection_close(ctx) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_sensors, REDFISH_URI_SENSORS, true, sensor_collection_get,
			    NULL, NULL);

/*** /redfish/v1/Chassis/1/Sensors/<id> ***/
struct redfish_sensor_status {
	const char *state;
};

struct redfish_sensor_resource {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *name;
	int32_t reading;
	const char *reading_type;
	const char *reading_units;
	const char *physical_context;
	struct redfish_sensor_status status;
};

static const struct json_obj_descr sensor_status_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_status, "State", state, JSON_TOK_STRING),
};

static const struct json_obj_descr sensor_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "Reading", reading,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "ReadingType", reading_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "ReadingUnits", reading_units,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_sensor_resource, "PhysicalContext",
				  physical_context, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_sensor_resource, "Status", status,
				    sensor_status_descr),
};

/*
 * The resource is registered under a wildcard URL, so the addressed sensor has
 * to be recovered from the request path.
 */
static const struct bmc_sensor *sensor_from_url(struct bmc_redfish_ctx *ctx)
{
	const char *url = bmc_redfish_request_url(ctx);
	const char *id = strrchr(url, '/');
	size_t id_len;

	if (id == NULL) {
		return NULL;
	}

	id++;
	id_len = strcspn(id, "?");

	BMC_SENSOR_FOREACH(sensor) {
		if (strlen(sensor->id) == id_len && strncmp(sensor->id, id, id_len) == 0) {
			return sensor;
		}
	}

	return NULL;
}

static int sensor_get(struct bmc_redfish_ctx *ctx)
{
	const struct bmc_sensor *sensor = sensor_from_url(ctx);
	char odata_id[sizeof(REDFISH_URI_SENSORS "/") + BMC_SENSOR_ID_MAX_LEN];
	struct redfish_sensor_resource resource;
	struct sensor_value val;
	int ret;

	if (sensor == NULL) {
		return HTTP_404_NOT_FOUND;
	}

	ret = snprintk(odata_id, sizeof(odata_id), REDFISH_URI_SENSORS "/%s", sensor->id);
	if (ret < 0 || ret >= (int)sizeof(odata_id)) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	memset(&resource, 0, sizeof(resource));
	resource.odata_id = odata_id;
	resource.odata_type = "#Sensor.v1_2_0.Sensor";
	resource.id = sensor->id;
	resource.name = sensor->name;
	resource.reading_type = sensor->reading_type;
	resource.reading_units = sensor->units;
	resource.physical_context = sensor->physical_context;

	ret = bmc_sensor_read(sensor, &val);
	if (ret < 0) {
		LOG_WRN("Could not read sensor %s (err=%d)", sensor->id, ret);
		resource.status.state = "UnavailableOffline";
	} else {
		/* Redfish Reading is a real number, but the BMC has no FPU to
		 * spare, so only the integral part is reported.
		 */
		resource.reading = val.val1;
		resource.status.state = "Enabled";
	}

	ret = bmc_redfish_reply_encode(ctx, sensor_descr, ARRAY_SIZE(sensor_descr), &resource);
	if (ret < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_sensor, REDFISH_URI_SENSORS "/*", true, sensor_get, NULL,
			    NULL);
