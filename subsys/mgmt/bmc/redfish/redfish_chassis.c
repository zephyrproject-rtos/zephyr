/*
 * Redfish Chassis resources.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc/host.h>
#include <zephyr/mgmt/bmc/redfish.h>

#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/*** /redfish/v1/Chassis ***/
static int chassis_collection_get(struct bmc_redfish_ctx *ctx)
{
	int ret;

	ret = redfish_collection_open(ctx, REDFISH_URI_CHASSIS_COLL,
				      "#ChassisCollection.ChassisCollection", "Chassis Collection",
				      1);
	if (ret == 0) {
		ret = redfish_collection_add(ctx, true, REDFISH_URI_CHASSIS);
	}

	if (ret == 0) {
		ret = redfish_collection_close(ctx);
	}

	return (ret < 0) ? HTTP_500_INTERNAL_SERVER_ERROR : 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_chassis_collection, REDFISH_URI_CHASSIS_COLL, true,
			    chassis_collection_get, NULL, NULL);

/*** /redfish/v1/Chassis/1 ***/
struct redfish_chassis_links {
	size_t computer_systems_len;
	struct redfish_link computer_systems[1];
	size_t managed_by_len;
	struct redfish_link managed_by[1];
};

static const struct json_obj_descr chassis_links_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct redfish_chassis_links, "ComputerSystems",
				       computer_systems, 1, computer_systems_len,
				       redfish_link_descr, ARRAY_SIZE(redfish_link_descr)),
	JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct redfish_chassis_links, "ManagedBy", managed_by, 1,
				       managed_by_len, redfish_link_descr,
				       ARRAY_SIZE(redfish_link_descr)),
};

struct redfish_chassis {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *name;
	const char *chassis_type;
	const char *manufacturer;
	const char *model;
	const char *serial_number;
	const char *part_number;
	const char *power_state;
	struct redfish_link sensors;
	struct redfish_chassis_links links;
};

static const struct json_obj_descr chassis_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "ChassisType", chassis_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "Manufacturer", manufacturer,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "Model", model, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "SerialNumber", serial_number,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "PartNumber", part_number,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_chassis, "PowerState", power_state,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_chassis, "Sensors", sensors,
				    redfish_link_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_chassis, "Links", links, chassis_links_descr),
};

static int chassis_get(struct bmc_redfish_ctx *ctx)
{
	const struct bmc_redfish_identity *identity = bmc_redfish_identity_get();
	const struct redfish_chassis chassis = {
		.odata_id = REDFISH_URI_CHASSIS,
		.odata_type = "#Chassis.v1_22_0.Chassis",
		.id = "1",
		.name = "Chassis",
		.chassis_type = CONFIG_BMC_REDFISH_CHASSIS_TYPE,
		.manufacturer = identity->manufacturer,
		.model = identity->model,
		.serial_number = identity->serial_number,
		.part_number = identity->part_number,
		.power_state = bmc_host_power_get() ? "On" : "Off",
		.sensors = {.odata_id = REDFISH_URI_SENSORS},
		.links = {
			.computer_systems_len = 1,
			.computer_systems = {{.odata_id = REDFISH_URI_SYSTEM}},
			.managed_by_len = 1,
			.managed_by = {{.odata_id = REDFISH_URI_MANAGER}},
		},
	};

	if (redfish_encode_with_oem(ctx, chassis_descr, ARRAY_SIZE(chassis_descr), &chassis,
				    BMC_REDFISH_OEM_CHASSIS) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_chassis, REDFISH_URI_CHASSIS, true, chassis_get, NULL, NULL);
