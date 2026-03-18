/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ebpf, CONFIG_EBPF_LOG_LEVEL);

int ebpf_maps_init(void);

/**
 * Initialize eBPF runtime state during Zephyr application startup.
 */
static int ebpf_init(void)
{
	LOG_INF("eBPF subsystem initializing");

	ebpf_maps_init();

	LOG_INF("eBPF subsystem ready");

	return 0;
}

SYS_INIT(ebpf_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
