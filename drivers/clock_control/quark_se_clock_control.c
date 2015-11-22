/* quark_se_clock_control.c - Clock controller driver for Quark SE */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include <misc/__assert.h>
#include <board.h>
#include <device.h>
#include <init.h>

#include <sys_io.h>

#include <clock_control.h>
#include <clock_control/quark_se_clock_control.h>

#ifndef CONFIG_CLOCK_DEBUG
#define DBG(...) {; }
#else
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_CLOCK_DEBUG */

struct quark_se_clock_control_config {
	uint32_t base_address;
};

static inline int quark_se_clock_control_on(struct device *dev,
					    clock_control_subsys_t sub_system)
{
	struct quark_se_clock_control_config *info = dev->config->config_info;
	uint32_t subsys = POINTER_TO_INT(sub_system);

	if (sub_system == CLOCK_CONTROL_SUBSYS_ALL) {
		DBG("Enabling all clock gates on dev %p\n", dev);
		sys_write32(0xffffffff, info->base_address);

		return DEV_OK;
	}

	DBG("Enabling clock gate on dev %p subsystem %u\n", dev, subsys);

	return sys_test_and_set_bit(info->base_address, subsys);
}

static inline int quark_se_clock_control_off(struct device *dev,
					     clock_control_subsys_t sub_system)
{
	struct quark_se_clock_control_config *info = dev->config->config_info;
	uint32_t subsys = POINTER_TO_INT(sub_system);

	if (sub_system == CLOCK_CONTROL_SUBSYS_ALL) {
		DBG("Disabling all clock gates on dev %p\n", dev);
		sys_write32(0x00000000, info->base_address);

		return DEV_OK;
	}

	DBG("clock gate on dev %p subsystem %u\n", dev, subsys);

	return sys_test_and_clear_bit(info->base_address, subsys);
}

static struct clock_control_driver_api quark_se_clock_control_api = {
	.on = quark_se_clock_control_on,
	.off = quark_se_clock_control_off
};

int quark_se_clock_control_init(struct device *dev)
{
	dev->driver_api = &quark_se_clock_control_api;

	DBG("Quark Se clock controller driver initialized on device: %p\n",
									dev);
	return DEV_OK;
}

#ifdef CONFIG_CLOCK_CONTROL_QUARK_SE_PERIPHERAL

struct quark_se_clock_control_config clock_quark_se_peripheral_config = {
	.base_address = CLOCK_PERIPHERAL_BASE_ADDR
};

DECLARE_DEVICE_INIT_CONFIG(clock_quark_se_peripheral,
			   CONFIG_CLOCK_CONTROL_QUARK_SE_PERIPHERAL_DRV_NAME,
			   &quark_se_clock_control_init,
			   &clock_quark_se_peripheral_config);

pre_kernel_early_init(clock_quark_se_peripheral, NULL);

#endif /* CONFIG_CLOCK_CONTROL_QUARK_SE_PERIPHERAL */
#ifdef CONFIG_CLOCK_CONTROL_QUARK_SE_EXTERNAL

struct quark_se_clock_control_config clock_quark_se_external_config = {
	.base_address = CLOCK_EXTERNAL_BASE_ADDR
};

DECLARE_DEVICE_INIT_CONFIG(clock_quark_se_external,
			   CONFIG_CLOCK_CONTROL_QUARK_SE_EXTERNAL_DRV_NAME,
			   &quark_se_clock_control_init,
			   &clock_quark_se_external_config);

pre_kernel_early_init(clock_quark_se_external, NULL);

#endif /* CONFIG_CLOCK_CONTROL_QUARK_SE_EXTERNAL */
#ifdef CONFIG_CLOCK_CONTROL_QUARK_SE_SENSOR

struct quark_se_clock_control_config clock_quark_se_sensor_config = {
	.base_address = CLOCK_SENSOR_BASE_ADDR
};

DECLARE_DEVICE_INIT_CONFIG(clock_quark_se_sensor,
			   CONFIG_CLOCK_CONTROL_QUARK_SE_SENSOR_DRV_NAME,
			   &quark_se_clock_control_init,
			   &clock_quark_se_sensor_config);

pre_kernel_early_init(clock_quark_se_sensor, NULL);
#endif /* CONFIG_CLOCK_CONTROL_QUARK_SE_SENSOR */
