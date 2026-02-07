/* Bluetooth CCP - Call Control Profile Call Control Server
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_utf8.h>

LOG_MODULE_REGISTER(bt_ccp_call_control_server, CONFIG_BT_CCP_CALL_CONTROL_SERVER_LOG_LEVEL);

/* A service instance can either be a GTBS or a TBS instance */
struct bt_ccp_call_control_server_bearer {
	char provider_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	char uci[BT_TBS_MAX_UCI_SIZE];
	uint8_t tbs_index;
	bool registered;
};

static struct bt_ccp_call_control_server_bearer
	bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT];

static struct bt_ccp_call_control_server_bearer *get_free_bearer(void)
{

	for (size_t i = 0; i < ARRAY_SIZE(bearers); i++) {
		if (!bearers[i].registered) {
			return &bearers[i];
		}
	}

	return NULL;
}

int bt_ccp_call_control_server_register_bearer(const struct bt_tbs_register_param *param,
					       struct bt_ccp_call_control_server_bearer **bearer)
{
	struct bt_ccp_call_control_server_bearer *free_bearer;
	int ret;

	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	free_bearer = get_free_bearer();
	if (free_bearer == NULL) {
		return -ENOMEM;
	}

	ret = bt_tbs_register_bearer(param);
	if (ret < 0) {
		LOG_DBG("Failed to register TBS bearer: %d", ret);

		/* Return known errors */
		if (ret == -EINVAL || ret == -EALREADY || ret == -EAGAIN || ret == -ENOMEM) {
			return ret;
		}

		return -ENOEXEC;
	}

	free_bearer->registered = true;
	free_bearer->tbs_index = (uint8_t)ret;
	(void)utf8_lcpy(free_bearer->provider_name, param->provider_name,
			sizeof(free_bearer->provider_name));
	(void)utf8_lcpy(free_bearer->uci, param->uci, sizeof(free_bearer->uci));
	*bearer = free_bearer;

	return 0;
}

int bt_ccp_call_control_server_unregister_bearer(struct bt_ccp_call_control_server_bearer *bearer)
{
	int err;

	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (!bearer->registered) {
		LOG_DBG("Bearer %p already unregistered", bearer);

		return -EALREADY;
	}

	err = bt_tbs_unregister_bearer(bearer->tbs_index);
	if (err != 0) {
		/* Return known errors */
		if (err == -EINVAL || err == -EALREADY) {
			return err;
		}

		return -ENOEXEC;
	}

	bearer->registered = false;

	return 0;
}

int bt_ccp_call_control_server_set_bearer_provider_name(
	struct bt_ccp_call_control_server_bearer *bearer, const char *name)
{
	size_t len;

	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (name == NULL) {
		LOG_DBG("name is NULL");

		return -EINVAL;
	}

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		return -EFAULT;
	}

	len = strlen(name);
	if (len > CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH || len == 0) {
		LOG_DBG("Invalid name length: %zu", len);

		return -EINVAL;
	}

	if (strcmp(bearer->provider_name, name) == 0) {
		return 0;
	}

	(void)utf8_lcpy(bearer->provider_name, name, sizeof(bearer->provider_name));

	return bt_tbs_set_bearer_provider_name(bearer->tbs_index, name);
}

int bt_ccp_call_control_server_get_bearer_provider_name(
	struct bt_ccp_call_control_server_bearer *bearer, const char **name)
{
	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (name == NULL) {
		LOG_DBG("name is NULL");

		return -EINVAL;
	}

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		return -EFAULT;
	}

	*name = bearer->provider_name;

	return 0;
}

int bt_ccp_call_control_server_get_bearer_uci(struct bt_ccp_call_control_server_bearer *bearer,
					      const char **uci)
{
	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (uci == NULL) {
		LOG_DBG("uci is NULL");

		return -EINVAL;
	}

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		return -EFAULT;
	}

	*uci = bearer->uci;

	return 0;
}
