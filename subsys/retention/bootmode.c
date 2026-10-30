/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/retention/retention.h>
#include <zephyr/retention/bootmode.h>

#define BOOT_MODE_NODE DT_CHOSEN(zephyr_boot_mode)

/* User data size of the area, without the optional prefix and checksum */
#define BOOT_MODE_USER_SIZE                                                                        \
	((int)DT_REG_SIZE(BOOT_MODE_NODE) - (int)DT_PROP_LEN_OR(BOOT_MODE_NODE, prefix, 0) -       \
	 (int)DT_PROP(BOOT_MODE_NODE, checksum))

/*
 * An area without user data holds nothing but its prefix, typically the magic
 * value a bootloader looks for: the prefix being present is the bootloader
 * boot mode and its absence the normal boot mode.
 */
#define BOOT_MODE_PREFIX_ONLY                                                                      \
	(DT_NODE_HAS_PROP(BOOT_MODE_NODE, prefix) && DT_PROP(BOOT_MODE_NODE, checksum) == 0 &&     \
	 BOOT_MODE_USER_SIZE == 0)

BUILD_ASSERT(BOOT_MODE_PREFIX_ONLY || BOOT_MODE_USER_SIZE >= 1,
	     "zephyr,boot-mode area needs a user data byte or a prefix and no user data");

static const struct device *boot_mode_dev = DEVICE_DT_GET(BOOT_MODE_NODE);

static int bootmode_prefix_only_check(uint8_t boot_mode)
{
	int rc = retention_is_valid(boot_mode_dev);

	if (rc < 0) {
		return rc;
	}

	switch (boot_mode) {
	case BOOT_MODE_TYPE_BOOTLOADER:
		return rc;
	case BOOT_MODE_TYPE_NORMAL:
		return (rc == 0) ? 1 : 0;
	default:
		return 0;
	}
}

static int bootmode_prefix_only_set(uint8_t boot_mode)
{
	switch (boot_mode) {
	case BOOT_MODE_TYPE_BOOTLOADER:
		/* There is no user data, a zero length write only stores the prefix */
		return retention_write(boot_mode_dev, 0, &boot_mode, 0);
	case BOOT_MODE_TYPE_NORMAL:
		return retention_clear(boot_mode_dev);
	default:
		return -ENOTSUP;
	}
}

int bootmode_check(uint8_t boot_mode)
{
	int rc;

	if (BOOT_MODE_PREFIX_ONLY) {
		return bootmode_prefix_only_check(boot_mode);
	}

	rc = retention_is_valid(boot_mode_dev);

	if (rc == 1 || rc == -ENOTSUP) {
		uint8_t stored_mode;

		rc = retention_read(boot_mode_dev, 0, &stored_mode, sizeof(stored_mode));

		/* Only check if modes match if there was no error, otherwise return the error */
		if (rc == 0) {
			if (stored_mode == boot_mode) {
				rc = 1;
			}
		}
	}

	return rc;
}

int bootmode_set(uint8_t boot_mode)
{
	if (BOOT_MODE_PREFIX_ONLY) {
		return bootmode_prefix_only_set(boot_mode);
	}

	return retention_write(boot_mode_dev, 0, &boot_mode, sizeof(boot_mode));
}

int bootmode_clear(void)
{
	return retention_clear(boot_mode_dev);
}
