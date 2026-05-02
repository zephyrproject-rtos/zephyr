/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <openthread/instance.h>
#include <openthread/platform/misc.h>

#if defined(CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE_RETENTION)

#include <zephyr/retention/bootmode.h>

#elif defined(CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE_GPIO)

BUILD_ASSERT(DT_HAS_COMPAT_STATUS_OKAY(openthread_config),
	"`openthread,config` compatible node not found");
BUILD_ASSERT(DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(openthread_config), bootloader_gpios),
	"`bootloader-gpios` property missing from `openthread,config` compatible node");

#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec bootloader_gpio =
	GPIO_DT_SPEC_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(openthread_config),
			 bootloader_gpios);
#endif

#include "platform-zephyr.h"

void otPlatReset(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	/* This function does nothing on the Posix platform. */
	sys_reboot(SYS_REBOOT_WARM);
}

#if defined(CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE)
otError otPlatResetToBootloader(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

#if defined(CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE_RETENTION)
	if (bootmode_set(BOOT_MODE_TYPE_BOOTLOADER)) {
		return OT_ERROR_NOT_CAPABLE;
	}
	sys_reboot(SYS_REBOOT_WARM);

#elif defined(CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE_GPIO)
	/*
	 * To enable resetting to bootloader by triggering gpio pin,
	 * select `CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE_GPIO=y`,
	 * and in Devicetree create `openthread` node in `/options/` path with
	 * `compatible = "openthread,config"` property and `bootloader-gpios` property,
	 * which should represent GPIO pin's configuration,
	 * containing controller phandle, pin number and pin flags. e.g:
	 *
	 * options {
	 *	openthread {
	 *		compatible = "openthread,config";
	 *		bootloader-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
	 *	};
	 * };
	 *
	 * Note: in below implementation, chosen GPIO pin is configured as output
	 * and initialized to active state (logical value ‘1’).
	 * Configuring pin flags in `bootloader-gpios` allows to choose
	 * if pin should be active in high or in low state.
	 */

	if (!gpio_is_ready_dt(&bootloader_gpio)) {
		return OT_ERROR_NOT_CAPABLE;
	}
	gpio_pin_configure_dt(&bootloader_gpio, GPIO_OUTPUT_ACTIVE);

#endif

	/*
	 * Return OT_ERROR_NOT_CAPABLE if resetting has been unsuccessful (invalid configuration or
	 * triggering reset had no effect)
	 */
	return OT_ERROR_NOT_CAPABLE;
}
#endif /* defined(CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE) */

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return OT_PLAT_RESET_REASON_POWER_ON;
}

void otPlatWakeHost(void)
{
	/* TODO */
}

void otPlatAssertFail(const char *aFilename, int aLineNumber)
{
	/*
	 * The code below is used instead of __ASSERT(false) to print the actual assert
	 * location instead of __FILE__:__LINE__, which would point to this function.
	 */
	__ASSERT_PRINT("OpenThread ASSERT @ %s:%d\n", aFilename, aLineNumber);
	__ASSERT_POST_ACTION();
}
