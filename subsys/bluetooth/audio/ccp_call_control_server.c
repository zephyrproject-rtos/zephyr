/* Bluetooth CCP - Call Control Profile Call Control Server
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_utf8.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(bt_ccp_call_control_server, CONFIG_BT_CCP_CALL_CONTROL_SERVER_LOG_LEVEL);

#define MUTEX_TIMEOUT K_MSEC(1000U)

/* A service instance can either be a GTBS or a TBS instance */
struct bt_ccp_call_control_server_bearer {
	char provider_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	char uci[BT_TBS_MAX_UCI_SIZE];
	enum bt_bearer_tech bearer_tech;
	char uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 1];
	uint8_t tbs_index;
	bool registered;
	struct k_mutex mutex;
};

static struct bt_ccp_call_control_server_bearer
	bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT];

/**
 * @brief Returns a free bearer
 *
 * If the return value is not NULL, the caller is responsible for unlocking the mutex
 *
 * @return A free bearer, or NULL
 */
static struct bt_ccp_call_control_server_bearer *get_free_bearer(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(bearers); i++) {
		int err;

		err = k_mutex_lock(&bearers[i].mutex, K_NO_WAIT);
		if (err != 0) {
			continue;
		}

		if (!bearers[i].registered) {
			return &bearers[i];
		}

		err = k_mutex_unlock(&bearers[i].mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
	}

	return NULL;
}

static bool valid_uri_schemes(const char *uri_schemes, size_t *uri_schemes_len)
{
	const char *c = uri_schemes;
	size_t len = 0U;
	bool new_uri;

	if (uri_schemes == NULL) {
		LOG_DBG("uri_schemes is NULL");

		return false;
	}

	if (*uri_schemes == '\0') {
		LOG_DBG("uri_schemes cannot be empty");

		return false;
	}

	/* Below we check for valid schemes. Since the allowed schemes are defined outside of
	 * Bluetooth (controlled by Internet Assigned Numbers Authority (IANA), we cannot do a
	 * reliable check on all existing ones, as new ones may be added at any time. We can,
	 * however, check the format.
	 * Generally URI schemes are lower case, but there are a few exceptions.
	 */

	/* The scheme must start with a letter */
	if (!isalpha(*c)) {
		return false;
	}

	c++;
	len++;

	/* The rest of the scheme shall be ASCII characters, numbers or `+`, `.` or `-`
	 * Since the list contains multiple schemes, the separator (`,`) is also allowed
	 */
	new_uri = true;
	while (*c != '\0') {
		if (new_uri) {
			/* The scheme must start with a letter */
			if (!isalpha(*c)) {
				return false;
			}

			new_uri = false;
		} else {
			const bool valid_char = isalnum(*c) || *c == '+' || *c == '.' || *c == '-';

			/* If character is not valid, we check if is the separator (`,`) and if
			 * there are additional characters after that, else reject it
			 */
			if (!valid_char) {
				if (*c == ',' && *(c + 1) != '\0') {
					new_uri = true;
				} else {
					LOG_DBG("URI scheme contains invalid characters (%c)", *c);
					return false;
				}
			}
		}

		c++;
		len++;
	}

	*uri_schemes_len = len;

	return true;
}

int bt_ccp_call_control_server_register_bearer(const struct bt_tbs_register_param *param,
					       struct bt_ccp_call_control_server_bearer **bearer)
{
	struct bt_ccp_call_control_server_bearer *free_bearer;
	size_t uri_schemes_len;
	__maybe_unused int err;
	int ret;

	CHECKIF(bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (param == NULL) {
		LOG_DBG("param is NULL");

		return -EINVAL;
	}

	if (!valid_uri_schemes(param->uri_schemes_supported, &uri_schemes_len)) {
		return -EINVAL;
	}

	if (uri_schemes_len > CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH) {
		LOG_DBG("uri_schemes buffer not large enough (is %zu > %d)", uri_schemes_len,
			CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH);

		return -ENOMEM;
	}

	free_bearer = get_free_bearer();
	if (free_bearer == NULL) {
		return -ENOMEM;
	}

	ret = bt_tbs_register_bearer(param);
	if (ret < 0) {
		LOG_DBG("Failed to register TBS bearer: %d", ret);

		/* Return known errors */
		if (!(ret == -EINVAL || ret == -EALREADY || ret == -EAGAIN || ret == -ENOMEM)) {
			ret = -ENOEXEC;
		}
	} else {
		free_bearer->registered = true;
		free_bearer->tbs_index = (uint8_t)ret;
		(void)utf8_lcpy(free_bearer->provider_name, param->provider_name,
				sizeof(free_bearer->provider_name));
		(void)utf8_lcpy(free_bearer->uci, param->uci, sizeof(free_bearer->uci));
		free_bearer->bearer_tech = param->technology;

		(void)memcpy(free_bearer->uri_schemes, param->uri_schemes_supported,
			     uri_schemes_len);
		free_bearer->uri_schemes[uri_schemes_len] = '\0';

		*bearer = free_bearer;

		ret = 0;
	}

	err = k_mutex_unlock(&free_bearer->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

int bt_ccp_call_control_server_unregister_bearer(struct bt_ccp_call_control_server_bearer *bearer)
{
	__maybe_unused int err;
	int ret;

	CHECKIF(bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	err = k_mutex_lock(&bearer->mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	if (!bearer->registered) {
		LOG_DBG("Bearer %p already unregistered", bearer);

		ret = -EALREADY;
	} else {
		ret = bt_tbs_unregister_bearer(bearer->tbs_index);
		if (ret == 0) {
			bearer->registered = false;
		} else {
			/* Return known errors */
			if (!(ret == -EINVAL || ret == -EALREADY)) {
				ret = -ENOEXEC;
			}
		}
	}

	err = k_mutex_unlock(&bearer->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

int bt_ccp_call_control_server_set_bearer_provider_name(
	struct bt_ccp_call_control_server_bearer *bearer, const char *name)
{
	__maybe_unused int err;
	size_t len;
	int ret;

	CHECKIF(bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	CHECKIF(name == NULL) {
		LOG_DBG("name is NULL");

		return -EINVAL;
	}

	len = strlen(name);
	if (len > CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH || len == 0) {
		LOG_DBG("Invalid name length: %zu", len);

		return -EINVAL;
	}

	err = k_mutex_lock(&bearer->mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		ret = -EFAULT;
	} else {
		if (strcmp(bearer->provider_name, name) != 0) {
			ret = bt_tbs_set_bearer_provider_name(bearer->tbs_index, name);
			if (ret == 0) {
				(void)utf8_lcpy(bearer->provider_name, name,
						sizeof(bearer->provider_name));
			} else {
				/* Return known errors */
				if (!(ret == -EINVAL || ret == -EBUSY)) {
					ret = -ENOEXEC;
				}
			}
		} else {
			ret = 0;
		}
	}

	err = k_mutex_unlock(&bearer->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

int bt_ccp_call_control_server_get_bearer_provider_name(
	struct bt_ccp_call_control_server_bearer *bearer, const char **name)
{
	__maybe_unused int err;
	int ret;

	CHECKIF(bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	CHECKIF(name == NULL) {
		LOG_DBG("name is NULL");

		return -EINVAL;
	}

	err = k_mutex_lock(&bearer->mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		ret = -EFAULT;
	} else {
		*name = bearer->provider_name;
		ret = 0;
	}

	err = k_mutex_unlock(&bearer->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

int bt_ccp_call_control_server_get_bearer_uci(struct bt_ccp_call_control_server_bearer *bearer,
					      const char **uci)
{
	__maybe_unused int err;
	int ret;

	CHECKIF(bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	CHECKIF(uci == NULL) {
		LOG_DBG("uci is NULL");

		return -EINVAL;
	}

	err = k_mutex_lock(&bearer->mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		ret = -EFAULT;
	} else {
		*uci = bearer->uci;
		ret = 0;
	}

	err = k_mutex_unlock(&bearer->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

int bt_ccp_call_control_server_set_bearer_tech(struct bt_ccp_call_control_server_bearer *bearer,
					       enum bt_bearer_tech tech)
{
	int err;

	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		return -EFAULT;
	}

	if (!IN_RANGE(tech, BT_BEARER_TECH_3G, BT_BEARER_TECH_WCDMA)) {
		LOG_DBG("Invalid tech param: %d", tech);

		return -EINVAL;
	}

	if (bearer->bearer_tech == tech) {
		return 0;
	}

	err = bt_tbs_set_bearer_technology(bearer->tbs_index, tech);
	if (err == 0) {
		bearer->bearer_tech = tech;
	}

	return err;
}

int bt_ccp_call_control_server_get_bearer_tech(
	const struct bt_ccp_call_control_server_bearer *bearer, enum bt_bearer_tech *tech)
{
	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (tech == NULL) {
		LOG_DBG("tech is NULL");

		return -EINVAL;
	}

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		return -EFAULT;
	}

	*tech = bearer->bearer_tech;

	return 0;
}

int bt_ccp_call_control_server_set_bearer_uri_schemes(
	struct bt_ccp_call_control_server_bearer *bearer, const char *uri_schemes)
{
	size_t uri_schemes_len;
	__maybe_unused int err;
	int ret;

	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (!valid_uri_schemes(uri_schemes, &uri_schemes_len)) {
		return -EINVAL;
	}

	if (uri_schemes_len > CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH) {
		LOG_DBG("uri_schemes buffer not large enough (is %zu > %d)", uri_schemes_len,
			CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH);

		return -ENOMEM;
	}

	err = k_mutex_lock(&bearer->mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		ret = -EFAULT;
	} else if (strcmp(bearer->uri_schemes, uri_schemes) == 0) {
		LOG_DBG("Value unchanged");
		ret = 0;
	} else {
		ret = bt_tbs_set_uri_scheme_list(bearer->tbs_index, uri_schemes);
		if (ret == 0) {
			(void)memcpy(bearer->uri_schemes, uri_schemes, uri_schemes_len);
			bearer->uri_schemes[uri_schemes_len] = '\0';
		}
	}

	err = k_mutex_unlock(&bearer->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

int bt_ccp_call_control_server_get_bearer_uri_schemes(
	struct bt_ccp_call_control_server_bearer *bearer, char *uri_schemes,
	size_t uri_schemes_size)
{
	size_t uri_schemes_len;
	__maybe_unused int err;
	int ret;

	if (bearer == NULL) {
		LOG_DBG("bearer is NULL");

		return -EINVAL;
	}

	if (uri_schemes == NULL) {
		LOG_DBG("uri_schemes is NULL");

		return -EINVAL;
	}

	err = k_mutex_lock(&bearer->mutex, MUTEX_TIMEOUT);
	__ASSERT(err == 0, "Failed to lock mutex: %d", err);

	if (!bearer->registered) {
		LOG_DBG("Bearer %p not registered", bearer);

		ret = -EFAULT;
	} else {
		uri_schemes_len = strlen(bearer->uri_schemes);
		if (uri_schemes_size <= uri_schemes_len) {
			LOG_DBG("name buffer not large enough (is <= %zu)", uri_schemes_len);

			ret = -ENOMEM;
		} else {
			(void)memcpy(uri_schemes, bearer->uri_schemes, uri_schemes_len);
			uri_schemes[uri_schemes_len] = '\0';
			ret = 0;
		}
	}

	err = k_mutex_unlock(&bearer->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

static int ccp_server_init(void)
{
	ARRAY_FOR_EACH_PTR(bearers, bearer) {
		__maybe_unused int err;

		err = k_mutex_init(&bearer->mutex);
		__ASSERT(err == 0, "Failed to init mutex: %d", err);
	}

	return 0;
}

SYS_INIT(ccp_server_init, APPLICATION, 0);
