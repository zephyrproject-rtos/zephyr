/*
 * Product identity and an OEM Redfish resource.
 *
 * The BMC core carries no product strings: what a client sees in
 * /redfish/v1/Systems/system comes from the identity registered here, and
 * vendor specific data is added through the OEM hook and extra resources.
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/redfish.h>
#include <zephyr/version.h>

#include "bmc_git_sha.h"

LOG_MODULE_DECLARE(bmc_sample, LOG_LEVEL_INF);

static const struct bmc_redfish_identity product_identity = {
	.product_name = "Zephyr BMC Reference Board",
	.manufacturer = "Zephyr Project",
	.model = "bmc-sample",
	.serial_number = "0000000001",
	.firmware_version = BMC_GIT_SHA,
	.processor_model = "Emulated",
	.processor_count = 1,
	.memory_gib = 4,
};

static int product_identity_init(void)
{
	bmc_redfish_identity_register(&product_identity);

	return 0;
}

/* Ahead of BMC_INIT_PHASE_PLATFORM, so the core never installs its defaults. */
BMC_COMPONENT_DEFINE(sample_identity, BMC_INIT_PHASE_STORAGE, product_identity_init, false);

/*
 * Vendor extension appended to the standard Manager resource. The encoder runs
 * while the resource object is still open, so it can only add members.
 */
struct product_manager_oem {
	const char *build;
};

static const struct json_obj_descr product_manager_oem_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct product_manager_oem, "Build", build, JSON_TOK_STRING),
};

static int product_manager_oem_encode(struct bmc_redfish_ctx *ctx)
{
	const struct product_manager_oem oem = {
		.build = BMC_GIT_SHA,
	};

	return bmc_redfish_reply_add_member(ctx, "Oem", product_manager_oem_descr,
					    ARRAY_SIZE(product_manager_oem_descr), &oem);
}

BMC_REDFISH_OEM_DEFINE(product_manager_oem, BMC_REDFISH_OEM_MANAGER,
		       product_manager_oem_encode);

/*
 * A whole resource of its own, published next to the standard tree. Downstream
 * products use the same macro the BMC core uses for its own resources.
 */
struct product_info {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *name;
	const char *zephyr_version;
};

static const struct json_obj_descr product_info_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct product_info, "@odata.id", odata_id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct product_info, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct product_info, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct product_info, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct product_info, "ZephyrVersion", zephyr_version,
				  JSON_TOK_STRING),
};

static int product_info_get(struct bmc_redfish_ctx *ctx)
{
	const struct product_info info = {
		.odata_id = "/redfish/v1/Oem/Zephyr",
		.odata_type = "#ZephyrOem.v1_0_0.ZephyrOem",
		.id = "Zephyr",
		.name = "Zephyr Project Information",
		.zephyr_version = KERNEL_VERSION_STRING,
	};

	if (bmc_redfish_reply_encode(ctx, product_info_descr, ARRAY_SIZE(product_info_descr),
				     &info) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(product_info, "/redfish/v1/Oem/Zephyr", true, product_info_get, NULL,
			    NULL);
