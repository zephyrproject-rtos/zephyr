/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/mgmt/updatehub.h>


static inline void z_vrfy_updatehub_autohandler(void)
{
	z_impl_updatehub_autohandler();
}
#include <zephyr/syscalls/updatehub_autohandler_mrsh.c>

static inline enum updatehub_response z_vrfy_updatehub_probe(void)
{
	return z_impl_updatehub_probe();
}
#include <zephyr/syscalls/updatehub_probe_mrsh.c>

static inline enum updatehub_response z_vrfy_updatehub_update(void)
{
	return z_impl_updatehub_update();
}
#include <zephyr/syscalls/updatehub_update_mrsh.c>

static inline int z_vrfy_updatehub_confirm(void)
{
	return z_impl_updatehub_confirm();
}
#include <zephyr/syscalls/updatehub_confirm_mrsh.c>

static inline int z_vrfy_updatehub_reboot(void)
{
	return z_impl_updatehub_reboot();
}
#include <zephyr/syscalls/updatehub_reboot_mrsh.c>
