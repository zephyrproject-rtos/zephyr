/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ssh_sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/net/net_config.h>

#if defined(CONFIG_NET_SAMPLE_SSH_USE_PASSWORD)
#define SSH_PASSWORD CONFIG_NET_SAMPLE_SSH_PASSWORD
#else
#define SSH_PASSWORD NULL
#endif

int main(void)
{
	LOG_INF("SSH sample for %s", CONFIG_BOARD_TARGET);

#if defined(CONFIG_SSH_SERVER)
	const struct shell *sh = shell_backend_uart_get_ptr();
	int ret;

	ret = net_config_init_sshd(sh, NULL,
				   CONFIG_NET_SAMPLE_SSH_USERNAME,
				   SSH_PASSWORD);
	if (ret != 0) {
		LOG_ERR("Failed to initialize SSH server");
		return 0;
	}
#endif /* CONFIG_SSH_SERVER */

	k_sleep(K_FOREVER);
}
