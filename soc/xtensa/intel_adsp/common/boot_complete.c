/* Copyright(c) 2022 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/arch.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <errno.h>
#include <soc.h>

#include <mem_window.h>

int boot_complete(void)
{
	uint32_t *win;
	const struct mem_win_config *config;
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(mem_window0));

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	config = dev->config;

	win = z_soc_uncached_ptr((__sparse_force void __sparse_cache *)config->mem_base);
	/* Software protocol: "firmware entered" has the value 5 */
	win[0] = 5;

	return 0;
}

SYS_INIT(boot_complete, EARLY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
