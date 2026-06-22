/*
 * Copyright 2023 Linaro.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/kernel.h>

static void ipm_receive_callback(const struct device *ipmdev, void *user_data,
			       uint32_t id, volatile void *data)
{
	ARG_UNUSED(ipmdev);
	ARG_UNUSED(user_data);

	printf("Received IPM notification over IVSHMEM\n");
}

int main(void)
{
	const struct device *ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm_ivshmem0));

	ipm_register_callback(ipm_dev, ipm_receive_callback, NULL);
	return 0;
}

static int cmd_ipm_send(const struct shell *sh,
			   size_t argc, char **argv)
{

	const struct device *ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm_ivshmem0));

	int peer_id = strtol(argv[1], NULL, 10);

	return ipm_send(ipm_dev, 0, peer_id, NULL, 0);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ivshmem_ipm,
						SHELL_CMD_ARG(ivshmem_ipm_send, NULL,
								"Send notification to other side using IPM",
								cmd_ipm_send, 2, 0),
						SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ivshmem_ipm_send,
				&sub_ivshmem_ipm,
				"Send notification to other side using IPM",
				cmd_ipm_send, 2, 0);
