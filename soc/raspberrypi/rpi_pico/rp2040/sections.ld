/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Second Stage Bootloader is only linked to the app that
 * resides at 0x100. This can be the application, or a bootloader
 * such as mcuboot.
 */

.boot2 : {
	KEEP(*(.boot2))
} > BOOT_FLASH
