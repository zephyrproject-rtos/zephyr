/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <image_header.h>
#include <rom_uuid.h>
#include <version.h>

extern void z_arm_reset(void);

const T_IMG_HEADER_FORMAT img_header __attribute__((section(".image_header"))) = {
	.auth = {
			.image_mac = {[0 ... 15] = 0xFF},
		},
	.ctrl_header = {
			.ic_type = 0X10,
			.secure_version = 0,
			.ctrl_flag.flag_value.xip = 1,
			.ctrl_flag.flag_value.enc = 0,
			.ctrl_flag.flag_value.load_when_boot = 0,
			.ctrl_flag.flag_value.enc_key_select = 0,
			.ctrl_flag.flag_value.enc_load = 0,
			.ctrl_flag.flag_value.not_ready = 0,
			.ctrl_flag.flag_value.not_obsolete = 1,
			.ctrl_flag.flag_value.compressed_not_ready = 0,
			.ctrl_flag.flag_value.compressed_not_obsolete = 1,
			.ctrl_flag.flag_value.integrity_check_en_in_boot = 0,
			.image_id = AppPatch,
			.payload_len = 0x100,
		},
	.uuid = DEFINE_symboltable_uuid,
	.load_dst = 0,
	.exe_base = (unsigned int)z_arm_reset,
	.load_len = 0,
	.image_base = CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET,
	.git_ver = {
			.ver_info.sub_version._version_major = KERNEL_VERSION_MAJOR,
			.ver_info.sub_version._version_minor = KERNEL_VERSION_MINOR,
			.ver_info.sub_version._version_revision = KERNEL_PATCHLEVEL,
		},
};
