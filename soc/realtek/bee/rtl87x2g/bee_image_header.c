/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <image_info.h>
#include <rom_uuid.h>
#include <version.h>

extern void z_arm_reset(void);

const T_IMG_HEADER_FORMAT img_header __attribute__((section(".image_header"))) = {
	.auth = {
		.image_mac = {
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		},
	},
	.ctrl_header = {
		.ic_type = 0xF,
		.secure_version = 0,
		.ctrl_flag.xip = 1,
		.ctrl_flag.enc = 0,
		.ctrl_flag.load_when_boot = 0,
		.ctrl_flag.enc_load = 0,
		.ctrl_flag.not_obsolete = 1,
		.image_id = IMG_MCUAPP,
		.payload_len = 0x100,
	},
	.git_ver = {
		.ver_info.sub_version._version_major = KERNEL_VERSION_MAJOR,
		.ver_info.sub_version._version_minor = KERNEL_VERSION_MINOR,
		.ver_info.sub_version._version_revision = KERNEL_PATCHLEVEL,
	},
	.uuid = DEFINE_symboltable_uuid,
	.exe_base = (unsigned int)&z_arm_reset,
	.image_base = CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET,
};
