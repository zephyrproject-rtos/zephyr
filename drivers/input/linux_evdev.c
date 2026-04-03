/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_linux_evdev

#include <cmdline.h>
#include <nsi_host_trampolines.h>
#include <posix_native_task.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "linux_evdev_bottom.h"

LOG_MODULE_REGISTER(linux_evdev, CONFIG_INPUT_LOG_LEVEL);

static int linux_evdev_fd = -1;
static const char *linux_evdev_path;
static struct k_thread linux_evdev_thread;
static K_KERNEL_STACK_DEFINE(linux_evdev_thread_stack,
			     CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);

static void linux_evdev_options(void)
{
	static struct args_struct_t linux_evdev_options[] = {
		{
			.is_mandatory = true,
			.option = "evdev",
			.name = "path",
			.type = 's',
			.dest = (void *)&linux_evdev_path,
			.descript = "Path of the evdev device to use",
		},
		ARG_TABLE_ENDMARKER,
	};

	native_add_command_line_opts(linux_evdev_options);
}

static void linux_evdev_check_arg(void)
{
	if (linux_evdev_path == NULL) {
		posix_print_error_and_exit(
				"Error: evdev device missing.\n"
				"Please specify an evdev device with the --evdev "
				"argument when using CONFIG_NATIVE_LINUX_EVDEV=y\n");
	}
}

static void linux_evdev_cleanup(void)
{
	if (linux_evdev_fd >= 0) {
		nsi_host_close(linux_evdev_fd);
	}
}

NATIVE_TASK(linux_evdev_options, PRE_BOOT_1, 10);
NATIVE_TASK(linux_evdev_check_arg, PRE_BOOT_2, 10);
NATIVE_TASK(linux_evdev_cleanup, ON_EXIT, 10);

static void linux_evdev_thread_fn(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	uint16_t type;
	uint16_t code;
	int32_t value;
	int ret;

	while (true) {
		ret = linux_evdev_read(linux_evdev_fd, &type, &code, &value);
		if (ret == NATIVE_LINUX_EVDEV_NO_DATA) {
			/* Let other threads run. */
			k_sleep(K_MSEC(CONFIG_NATIVE_LINUX_THREAD_SLEEP_MS));
			continue;
		} else if (ret < 0) {
			return;
		}

		LOG_DBG("evdev event: type=%d code=%d val=%d", type, code, value);

		if (type == 0) { /* EV_SYN */
			input_report(dev, 0, 0, 0, true, K_FOREVER);
		} else if (type == INPUT_EV_KEY && value == 2) {
			/* nothing, ignore key repeats */
		} else {
			input_report(dev, type, code, value, false, K_FOREVER);
		}
	}
}

static int linux_evdev_init(const struct device *dev)
{
	linux_evdev_fd = linux_evdev_open(linux_evdev_path);

	k_thread_create(&linux_evdev_thread, linux_evdev_thread_stack,
			K_KERNEL_STACK_SIZEOF(linux_evdev_thread_stack),
			linux_evdev_thread_fn, (void *)dev, NULL, NULL,
			CONFIG_NATIVE_LINUX_EVDEV_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(&linux_evdev_thread, dev->name);

	return 0;
}

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one zephyr,native-linux-evdev compatible node is supported");

DEVICE_DT_INST_DEFINE(0, linux_evdev_init, NULL,
		      NULL, NULL,
		      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);
