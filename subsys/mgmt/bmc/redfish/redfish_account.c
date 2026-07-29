/*
 * Redfish AccountService and the single administrator account backed by the
 * BMC authentication configuration.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/mgmt/bmc/redfish.h>

#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/*** /redfish/v1/AccountService ***/
struct redfish_account_service {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *name;
	struct redfish_link accounts;
};

static const struct json_obj_descr account_service_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account_service, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account_service, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account_service, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account_service, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_account_service, "Accounts", accounts,
				    redfish_link_descr),
};

static int account_service_get(struct bmc_redfish_ctx *ctx)
{
	const struct redfish_account_service account_service = {
		.odata_id = REDFISH_URI_ACCOUNT_SERVICE,
		.odata_type = "#AccountService.v1_10_0.AccountService",
		.id = "AccountService",
		.name = "Account Service",
		.accounts = {.odata_id = REDFISH_URI_ACCOUNTS},
	};

	if (bmc_redfish_reply_encode(ctx, account_service_descr,
				     ARRAY_SIZE(account_service_descr), &account_service) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_account_service, REDFISH_URI_ACCOUNT_SERVICE, false,
			    account_service_get, NULL, NULL);

/*** /redfish/v1/AccountService/Accounts ***/
static int accounts_collection_get(struct bmc_redfish_ctx *ctx)
{
	int ret;

	ret = redfish_collection_open(ctx, REDFISH_URI_ACCOUNTS,
				      "#ManagerAccountCollection.ManagerAccountCollection",
				      "Accounts Collection", 1);
	if (ret == 0) {
		ret = redfish_collection_add(ctx, true, REDFISH_URI_ADMIN_ACCOUNT);
	}

	if (ret == 0) {
		ret = redfish_collection_close(ctx);
	}

	return (ret < 0) ? HTTP_500_INTERNAL_SERVER_ERROR : 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_accounts, REDFISH_URI_ACCOUNTS, true,
			    accounts_collection_get, NULL, NULL);

/*** /redfish/v1/AccountService/Accounts/1 ***/
struct redfish_account {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *role_id;
	const char *name;
	const char *user_name;
	const char *password;
};

static const struct json_obj_descr account_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account, "RoleId", role_id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account, "UserName", user_name,
				  JSON_TOK_STRING),
	/* Write-only, but the descriptor is needed to parse a PATCH. */
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_account, "Password", password, JSON_TOK_STRING),
};

static int account_patch(struct bmc_redfish_ctx *ctx)
{
	struct redfish_account payload;
	int ret;

	memset(&payload, 0, sizeof(payload));

	ret = bmc_redfish_request_parse(ctx, account_descr, ARRAY_SIZE(account_descr), &payload);
	if (ret < 0) {
		LOG_ERR("Account: malformed JSON (err=%d)", ret);
		return HTTP_400_BAD_REQUEST;
	}

	if (payload.password != NULL) {
		ret = bmc_config_admin_password_set(payload.password);
		if (ret < 0) {
			LOG_ERR("Could not update the administrator password (err=%d)", ret);
			return HTTP_400_BAD_REQUEST;
		}

		LOG_INF("Administrator password updated over Redfish");
	}

	return 0;
}

static int account_get(struct bmc_redfish_ctx *ctx)
{
	const struct redfish_account account = {
		.odata_id = REDFISH_URI_ADMIN_ACCOUNT,
		.odata_type = "#ManagerAccount.v1_9_0.ManagerAccount",
		.id = "1",
		.name = "Administrator",
		.role_id = "Administrator",
		.user_name = bmc_config_admin_user(),
		/*
		 * The JSON encoder turns a NULL string into "" rather than
		 * null, which makes strict Redfish validators complain about
		 * the write-only Password property.
		 */
		.password = NULL,
	};

	if (bmc_redfish_reply_encode(ctx, account_descr, ARRAY_SIZE(account_descr), &account) <
	    0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_account, REDFISH_URI_ADMIN_ACCOUNT, true, account_get,
			    account_patch, NULL);
