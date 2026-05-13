/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ssh_sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_config.h>

#if defined(CONFIG_NET_SAMPLE_SSH_USE_PASSWORD)
#define SSH_PASSWORD CONFIG_NET_SAMPLE_SSH_PASSWORD
#else
#define SSH_PASSWORD NULL
#endif

#define KEY_SIZE_BITS 2048

NET_CONFIG_SSH_KEY_BUF_DEFINE_STATIC(my_key_buf, KEY_SIZE_BITS);

/* We store the our private key to slot 0 and public key to slot 1 */

NET_CONFIG_SSH_PRIV_KEY_DEFINE_STATIC(my_priv_key_config,
				      my_key_buf, sizeof(my_key_buf),
				      "id_rsa", 0,
				      SSH_HOST_KEY_TYPE_RSA, KEY_SIZE_BITS);

NET_CONFIG_SSH_PUB_KEY_DEFINE_STATIC(my_pub_key_config,
				     my_key_buf, sizeof(my_key_buf),
				     "authorized_key_0", 1);

NET_CONFIG_SSH_PASSWORD_AUTH_DEFINE_STATIC(my_password_auth,
					   CONFIG_NET_SAMPLE_SSH_USERNAME,
					   SSH_PASSWORD);

int main(void)
{
	LOG_INF("SSH sample for %s", CONFIG_BOARD_TARGET);

#if defined(CONFIG_SSH_SERVER)
	int ret;

	ret = net_config_init_sshd(NULL, &my_priv_key_config,
				   &my_pub_key_config, &my_password_auth, 0);
	if (ret != 0) {
		LOG_ERR("Failed to initialize SSH server");
		return 0;
	}
#endif /* CONFIG_SSH_SERVER */

	k_sleep(K_FOREVER);
}
