/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc/auth.h>
#include <zephyr/mgmt/bmc/config.h>

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

static int auth_check_builtin(const char *user, const char *password)
{
	if (strcmp(user, bmc_config_admin_user()) != 0) {
		return -EACCES;
	}

	if (strcmp(password, bmc_config_admin_password()) != 0) {
		return -EACCES;
	}

	return 0;
}

static const struct bmc_auth_ops auth_ops_builtin = {
	.check = auth_check_builtin,
};

static const struct bmc_auth_ops *auth_ops = &auth_ops_builtin;

int bmc_auth_ops_register(const struct bmc_auth_ops *ops)
{
	auth_ops = (ops != NULL) ? ops : &auth_ops_builtin;

	return 0;
}

void bmc_auth_warn_default_password(void)
{
	if (auth_ops != &auth_ops_builtin) {
		return;
	}

	if (strcmp(bmc_config_admin_password(), CONFIG_BMC_DEFAULT_ADMIN_PASSWORD) != 0) {
		return;
	}

	LOG_WRN("Administrator \"%s\" still has the password from "
		"CONFIG_BMC_DEFAULT_ADMIN_PASSWORD", bmc_config_admin_user());
	LOG_WRN("Change it with \"bmc config password <password>\" or by patching "
		"the Redfish account");
}

int bmc_auth_check(const char *user, const char *password)
{
	if (user == NULL || password == NULL) {
		return -EACCES;
	}

	if (auth_ops->check == NULL) {
		return -EACCES;
	}

	return auth_ops->check(user, password);
}

int bmc_auth_check_pair(const char *credentials, char separator)
{
	char user[BMC_CONFIG_USER_MAX_LEN + 1];
	const char *sep;
	size_t user_len;

	if (credentials == NULL) {
		return -EACCES;
	}

	sep = strchr(credentials, separator);
	if (sep == NULL) {
		return -EINVAL;
	}

	user_len = sep - credentials;
	if (user_len >= sizeof(user)) {
		return -EACCES;
	}

	memcpy(user, credentials, user_len);
	user[user_len] = '\0';

	return bmc_auth_check(user, sep + 1);
}
