/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ssh/keygen.h>
#include <zephyr/net/ssh/server.h>
#include <zephyr/net/net_config.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell_ssh.h>

#define MAX_CMD_LEN 256
#define MAX_SETTINGS_NAME 64

struct key_load_param {
	void *data;
	size_t len; /* Max len in, actual len out */
	bool found;
};

static int settings_load_key_cb(const char *key,
				size_t len,
				settings_read_cb read_cb,
				void *cb_arg,
				void *param)
{
	struct key_load_param *load_param = param;
	ssize_t read = read_cb(cb_arg, load_param->data, load_param->len);

	if (read < 0) {
		return (int)read;
	}

	load_param->len = read;
	load_param->found = true;

	return 0;
}

static int load_key(char setting_name[MAX_SETTINGS_NAME],
		    const struct net_config_ssh_key *config)
{
	struct key_load_param load_param = {
		.data = config->key_buf,
		.len = config->key_buf_len,
		.found = false,
	};
	int ret;

	if (snprintk(setting_name, MAX_SETTINGS_NAME, "ssh/keys/%s",
		     config->key_name) >= MAX_SETTINGS_NAME) {
		return -EINVAL;
	}

	ret = settings_load_subtree_direct(setting_name, settings_load_key_cb,
					   &load_param);
	if (ret != 0) {
		LOG_ERR("Load setting failed: %d", ret);
		return ret;
	}

	if (!load_param.found) {
		LOG_ERR("Key \"%s\" not found in settings", config->key_name);
		return -ENOENT;
	}

	if (config->key_type != SSH_HOST_KEY_TYPE_RSA) {
		LOG_ERR("Unsupported key type (%d)", config->key_type);
		return -EINVAL;
	}

	ret = ssh_keygen_import(config->key_id, config->is_private,
				SSH_HOST_KEY_FORMAT_DER,
				config->key_buf, load_param.len);
	if (ret < 0) {
		LOG_ERR("Key import failed: %d", ret);
		return ret;
	}

	return 0;
}

static int gen_key(const struct net_config_ssh_key *config)
{
	if (config->key_type != SSH_HOST_KEY_TYPE_RSA) {
		LOG_ERR("Unsupported key type (%d)", config->key_type);
		return -EINVAL;
	}

	return ssh_keygen(config->key_id, config->key_type, config->key_bits);
}

static int save_key(char setting_name[MAX_SETTINGS_NAME],
		    const struct net_config_ssh_key *config)
{
	int ret;

	if (snprintk(setting_name, MAX_SETTINGS_NAME, "ssh/keys/%s",
		     config->key_name) >= MAX_SETTINGS_NAME) {
		return -EINVAL;
	}

	ret = ssh_keygen_export(config->key_id, config->is_private,
				SSH_HOST_KEY_FORMAT_DER,
				config->key_buf, config->key_buf_len);
	if (ret < 0) {
		LOG_ERR("Key export failed: %d", ret);
		return ret;
	}

	return settings_save_one(setting_name, config->key_buf, config->key_buf_len);
}

static uint8_t *export_key(const struct net_config_ssh_key *config)
{
	int ret;

	ret = ssh_keygen_export(config->key_id, false, SSH_HOST_KEY_FORMAT_PEM,
				config->key_buf, config->key_buf_len);
	if (ret < 0) {
		LOG_ERR("Key export failed: %d", ret);
		return NULL;
	}

	return config->key_buf;
}

int net_config_init_sshd(struct net_if *iface,
			 const struct net_config_ssh_key *priv_key_config,
			 const struct net_config_ssh_key *pub_key_config,
			 const struct net_config_ssh_password_auth *password_auth_config,
			 int sshd_instance)
{
	struct net_sockaddr_storage bind_addr;
	char setting_name[MAX_SETTINGS_NAME];
	char ip_addr_str[NET_IPV6_ADDR_LEN];
	const char *username, *password;
	struct ssh_server *sshd;
	int priv_key_id = -1;
	uint8_t *key;
	int ret;

	ret = settings_subsys_init();
	if (ret != 0) {
		LOG_ERR("Failed to init settings subsys: %d", ret);
		return ret;
	}

	memset(&bind_addr, 0, sizeof(bind_addr));
	net_sin(net_sad(&bind_addr))->sin_port = htons(CONFIG_SSH_PORT);

	/* If an interface was provided, use its address for the SSH server.
	 * Otherwise, bind to the wildcard address.
	 */
	if (iface != NULL) {
		void *src;

		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			src = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
			if (src == NULL) {
				LOG_ERR("No %s address configured for the interface", "IPv4");
				return -EINVAL;
			}

			memcpy(&net_sin(net_sad(&bind_addr))->sin_addr, src,
			       sizeof(struct net_in_addr));
			net_addr_ntop(NET_AF_INET, src, ip_addr_str, sizeof(ip_addr_str));
		} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
			src = net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface);
			if (src == NULL) {
				LOG_ERR("No %s address configured for the interface", "IPv6");
				return -EINVAL;
			}

			memcpy(&net_sin6(net_sad(&bind_addr))->sin6_addr, src,
			       sizeof(struct net_in6_addr));
			net_addr_ntop(NET_AF_INET6, src, ip_addr_str, sizeof(ip_addr_str));
		} else {
			LOG_ERR("No %s address configured for the interface", "IP");
			return -EINVAL;
		}
	} else {
		int family;

		if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
			family = NET_AF_INET6; /* Use IPv6 to support both IPv4 and IPv6 */
		} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
			family = NET_AF_INET; /* Default to IPv4 */
		} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
			family = NET_AF_INET6; /* Default to IPv6 */
		} else {
			return -ENOTSUP;
		}

		/* Bind to all interfaces */
		bind_addr.ss_family = family;

		strcpy(ip_addr_str, "<any>");
	}

	LOG_DBG("Binding to %s for SSH server", ip_addr_str);

	if (priv_key_config == NULL) {
		LOG_ERR("%s key config is NULL", "Private");
		return -EINVAL;
	}

	if (password_auth_config != NULL &&
	    !(password_auth_config->password != NULL &&
	      password_auth_config->password[0] != '\0')) {
		LOG_WRN("Password authentication is enabled but no password was provided, "
			"falling back to public key authentication");
	}

	if (priv_key_config->key_buf == NULL) {
		LOG_ERR("Key buffer is NULL");
		return -EINVAL;
	}

	ret = load_key(setting_name, priv_key_config);
	if (ret == 0) {
		LOG_INF("Loaded %s key: %s (#%d)", "private",
			priv_key_config->key_name, priv_key_config->key_id);

	} else if (ret == -ENOENT) {
		LOG_WRN("Failed to load private key, generating new key pair...");

		ret = gen_key(priv_key_config);
		if (ret != 0) {
			LOG_ERR("Failed to generate host key: %d", ret);
			return ret;
		}

		ret = save_key(setting_name, priv_key_config);
		if (ret != 0) {
			LOG_ERR("Failed to save private key: %d", ret);
		}

		LOG_INF("Saved %s key: %s (#%d)", "private",
			priv_key_config->key_name, priv_key_config->key_id);

	} else {
		LOG_ERR("Failed to load private key: %d", ret);
		return ret;
	}

	if (pub_key_config == NULL) {
		LOG_ERR("%s key config is NULL", "Public");
		return -EINVAL;
	}

	/* Exporting the public key is not strictly necessary, but it allows us to
	 * easily add the key to the authorized keys in the host.
	 * You can copy the exported key from the output and add it to your
	 * ssh client's authorized_keys file to allow passwordless login.
	 */
	key = export_key(priv_key_config);
	if (key == NULL) {
		LOG_ERR("Failed to export public key");
	} else {
		LOG_INF("Host public key:\n%s", (char *)key);
	}

	sshd = ssh_server_instance(sshd_instance);
	if (sshd == NULL) {
		LOG_ERR("Failed to get SSH server instance %d", sshd_instance);
		return -ENOENT;
	}

	if (password_auth_config != NULL &&
	    password_auth_config->password != NULL &&
	    password_auth_config->password[0] != '\0') {
		password = password_auth_config->password;
	} else {
		password = NULL;
	}

	if (password_auth_config != NULL &&
	    password_auth_config->username != NULL &&
	    password_auth_config->username[0] != '\0') {
		username = password_auth_config->username;
	} else {
		username = NULL;
	}

	/* If we have an authorized key, load it and start the server with auth method
	 * set to public key. Otherwise, start the server with password auth.
	 */
	ret = load_key(setting_name, pub_key_config);
	if (ret == 0 || password == NULL) {
		if (username != NULL) {
			LOG_INF("Starting SSH server with public key authentication as \"%s\" user",
				username);
		} else {
			LOG_INF("Starting SSH server with public key authentication");
		}

		priv_key_id = priv_key_config->key_id;
	} else {
		if (username != NULL) {
			LOG_INF("Starting SSH server with password authentication as \"%s\" user",
				username);
		} else {
			LOG_INF("Starting SSH server with password authentication");
		}
	}

	ret = ssh_server_start(sshd,
			       net_sad(&bind_addr),
			       priv_key_id,
			       username,
			       password,
			       &pub_key_config->key_id,
			       1,
			       shell_sshd_event_callback,
			       shell_sshd_transport_event_callback,
			       NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start SSH server: %d", ret);
		return ret;
	}

	return 0;
}
