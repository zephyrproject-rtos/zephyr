/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/interconn/renesas_elc/renesas_elc.h>
#include <zephyr/syscall_handler.h>

int z_vrfy_renesas_elc_software_event_generate(const struct device *dev,
					       elc_software_event_t sw_event)
{
	K_OOPS(K_SYSCALL_DRIVER_RENESAS_ELC(dev, software_event_generate));
	return z_impl_renesas_elc_software_event_generate(dev, sw_event);
}
#include <zephyr/syscalls/renesas_elc_software_event_generate_mrsh.c>

int z_vrfy_renesas_elc_link_set(const struct device *dev, elc_peripheral_t peripheral,
				elc_event_t event)
{
	K_OOPS(K_SYSCALL_DRIVER_RENESAS_ELC(dev, link_set));
	return z_impl_renesas_elc_link_set(dev, peripheral, event);
}
#include <zephyr/syscalls/renesas_elc_link_set_mrsh.c>

int z_vrfy_renesas_elc_link_break(const struct device *dev, elc_peripheral_t peripheral)
{
	K_OOPS(K_SYSCALL_DRIVER_RENESAS_ELC(dev, link_break));
	return z_impl_renesas_elc_link_break(dev, peripheral);
}
#include <zephyr/syscalls/renesas_elc_link_break_mrsh.c>

int z_vrfy_renesas_elc_enable(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_RENESAS_ELC(dev, enable));
	return z_impl_renesas_elc_enable(dev);
}
#include <zephyr/syscalls/renesas_elc_enable_mrsh.c>

int z_vrfy_renesas_elc_disable(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_RENESAS_ELC(dev, disable));
	return z_impl_renesas_elc_disable(dev);
}
#include <zephyr/syscalls/renesas_elc_disable_mrsh.c>
