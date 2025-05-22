/* Bluetooth CCP - Call Control Profile Call Control Server
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bt_ccp_call_control_server, CONFIG_BT_CCP_CALL_CONTROL_SERVER_LOG_LEVEL);

/* A service instance can either be a GTBS or a TBS instance */
struct bt_ccp_call_control_server_bearer {
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

	CHECKIF(bearer == NULL) {
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
	*bearer = free_bearer;

	return 0;
}

int bt_ccp_call_control_server_unregister_bearer(struct bt_ccp_call_control_server_bearer *bearer)
{
	int err;

	CHECKIF(bearer == NULL) {
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
