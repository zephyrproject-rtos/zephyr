/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <zephyr/shell/shell.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/settings/settings.h>

#define MAX_CMD_LEN 256

int net_config_init_sshd(const struct shell *sh, struct net_if *iface,
			 const char *username, const char *password)
{
	char ip_addr_str[NET_IPV6_ADDR_LEN];
	char cmd[MAX_CMD_LEN];
	int ret;

	ret = settings_subsys_init();
	if (ret != 0) {
		LOG_ERR("Failed to init settings subsys: %d", ret);
		return ret;
	}

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
			net_addr_ntop(NET_AF_INET, src, ip_addr_str, sizeof(ip_addr_str));
		} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
			src = net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface);
			if (src == NULL) {
				LOG_ERR("No %s address configured for the interface", "IPv6");
				return -EINVAL;
			}
			net_addr_ntop(NET_AF_INET6, src, ip_addr_str, sizeof(ip_addr_str));
		} else {
			LOG_ERR("No %s address configured for the interface", "IP");
			return -EINVAL;
		}
	} else {
		ip_addr_str[0] = '\0'; /* Use wildcard address */
	}

	if (IS_ENABLED(CONFIG_NET_SAMPLE_SSH_USE_PASSWORD) &&
	    !(password != NULL && password[0] != '\0')) {
		LOG_WRN("Password authentication is enabled but no password was provided, "
			"falling back to public key authentication");
		password = NULL;
	}

	/* In this function, in order to simplify the setup, we use the shell
	 * to execute some commands to set up the SSH server, but in a real
	 * application you would likely want to do this through API calls
	 * instead.
	 *
	 * We store the our private key to slot 0 and public key to slot 1
	 */
	ret = shell_execute_cmd(sh, "net ssh_key load 0 priv id_rsa");
	if (ret != 0) {
		LOG_WRN("Could not load host key, generating new key pair...");

		ret = shell_execute_cmd(sh, "net ssh_key gen 0 rsa 2048");
		if (ret != 0) {
			LOG_ERR("Failed to generate host key");
			return ret;
		}

		(void)shell_execute_cmd(sh, "net ssh_key save 0 priv id_rsa");
	}

	/* Exporting the public key is not strictly necessary, but it allows us to
	 * easily add the key to the authorized keys in the host. In a real
	 * application you would likely want to handle this differently.
	 * You can copy the exported key from the shell output and add it to your
	 * ssh client's authorized_keys file to allow passwordless login.
	 */
	(void)shell_print(sh, "Host public key:");
	(void)shell_execute_cmd(sh, "net ssh_key pub export 0");

	/* If we have an authorized key, load it and start the server with auth method
	 * set to public key. Otherwise, start the server with password auth.
	 */
	ret = shell_execute_cmd(sh, "net ssh_key load 1 pub authorized_key_0");
	if (ret == 0 || password == NULL || password[0] == '\0') {
		if (username != NULL && username[0] != '\0') {
			LOG_INF("Starting SSH server with public key authentication as \"%s\" user",
				username);

			snprintk(cmd, sizeof(cmd), "net sshd start -i 0 -k 0 -a 1 -u %s %s",
				 username, ip_addr_str);
		} else {
			LOG_INF("Starting SSH server with public key authentication");

			snprintk(cmd, sizeof(cmd), "net sshd start -i 0 -k 0 -a 1 %s", ip_addr_str);
		}
	} else {
		if (username != NULL && username[0] != '\0') {
			LOG_INF("Starting SSH server with password authentication as \"%s\" user",
				username);

			snprintk(cmd, sizeof(cmd), "net sshd start -i 0 -k 0 -u %s -p %s %s",
				 username, password, ip_addr_str);
		} else {
			LOG_INF("Starting SSH server with password authentication");

			snprintk(cmd, sizeof(cmd), "net sshd start -i 0 -k 0 -p %s %s",
				 password, ip_addr_str);
		}
	}

	(void)shell_execute_cmd(sh, cmd);

	return 0;
}
