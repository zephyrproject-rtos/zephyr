/*
 * Copyright (c) 2020 Grinn
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bootutil/bootutil_public.h"
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/init.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#include "mcuboot_priv.h"

#ifdef CONFIG_RETENTION_BOOT_MODE
#include <zephyr/retention/bootmode.h>
#ifdef CONFIG_REBOOT
#include <zephyr/sys/reboot.h>
#endif
#endif

struct area_desc {
	const char *name;
	unsigned int id;
};

static const struct area_desc areas[] = {
	{"primary", FLASH_AREA_IMAGE_PRIMARY},
#ifdef FLASH_AREA_IMAGE_SECONDARY
	{"secondary", FLASH_AREA_IMAGE_SECONDARY},
#endif
};

static const char *swap_state_magic_str(uint8_t magic)
{
	switch (magic) {
	case BOOT_MAGIC_GOOD:
		return "good";
	case BOOT_MAGIC_BAD:
		return "bad";
	case BOOT_MAGIC_UNSET:
		return "unset";
	case BOOT_MAGIC_ANY:
		return "any";
	case BOOT_MAGIC_NOTGOOD:
		return "notgood";
	}

	return "unknown";
}

static const char *swap_type_str(uint8_t type)
{
	switch (type) {
	case BOOT_SWAP_TYPE_NONE:
		return "none";
	case BOOT_SWAP_TYPE_TEST:
		return "test";
	case BOOT_SWAP_TYPE_PERM:
		return "perm";
	case BOOT_SWAP_TYPE_REVERT:
		return "revert";
	case BOOT_SWAP_TYPE_FAIL:
		return "fail";
	}

	return "unknown";
}

static const char *swap_state_flag_str(uint8_t flag)
{
	switch (flag) {
	case BOOT_FLAG_SET:
		return "set";
	case BOOT_FLAG_BAD:
		return "bad";
	case BOOT_FLAG_UNSET:
		return "unset";
	case BOOT_FLAG_ANY:
		return "any";
	}

	return "unknown";
}

static int cmd_mcuboot_erase(const struct shell *sh, size_t argc,
			     char **argv)
{
	unsigned int id;
	int err;

	id = strtoul(argv[1], NULL, 0);

	/* Check if this is the parent (MCUboot) or own slot and if so, deny the request */
#if FIXED_PARTITION_EXISTS(boot_partition)
	if (id == FIXED_PARTITION_ID(boot_partition)) {
		shell_error(sh, "Cannot erase boot partition");
		return -EACCES;
	}
#endif

#if DT_FIXED_PARTITION_EXISTS(DT_CHOSEN(zephyr_code_partition))
	if (id == DT_FIXED_PARTITION_ID(DT_CHOSEN(zephyr_code_partition))) {
		shell_error(sh, "Cannot erase active partitions");
		return -EACCES;
	}
#endif

	err = boot_erase_img_bank(id);
	if (err) {
		shell_error(sh, "failed to erase bank %u", id);
		return err;
	}

	return 0;
}

static int cmd_mcuboot_confirm(const struct shell *sh, size_t argc,
			       char **argv)
{
	int err;

	err = boot_write_img_confirmed();
	if (err) {
		shell_error(sh, "failed to confirm: %d", err);
	}

	return err;
}

static int cmd_mcuboot_request_upgrade(const struct shell *sh, size_t argc,
				       char **argv)
{
	int permanent = 0;
	int err;

	if (argc > 1) {
		if (!strcmp(argv[1], "permanent")) {
			permanent = 1;
		} else {
			shell_warn(sh, "invalid argument!");
			return -EINVAL;
		}
	}

	err = boot_request_upgrade(permanent);
	if (err) {
		shell_error(sh, "failed to request upgrade: %d", err);
	}

	return err;
}

#ifdef CONFIG_RETENTION_BOOT_MODE
static int cmd_mcuboot_serial_recovery(const struct shell *sh, size_t argc,
				       char **argv)
{
	int rc;

	rc = bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);

	if (rc) {
		shell_error(sh, "Failed to set serial recovery mode: %d", rc);

		return rc;
	}

#ifdef CONFIG_REBOOT
	sys_reboot(SYS_REBOOT_COLD);
#else
	shell_error(sh, "mcuboot serial recovery mode set, please reboot your device");
#endif

	return rc;
}
#endif

static int cmd_mcuboot_info_area(const struct shell *sh,
				 const struct area_desc *area)
{
	struct mcuboot_img_header hdr;
	struct boot_swap_state swap_state;
	int err;

	err = boot_read_bank_header(area->id, &hdr, sizeof(hdr));
	if (err) {
		shell_error(sh, "failed to read %s area (%u) %s: %d",
			    area->name, area->id, "header", err);
		return err;
	}

	shell_print(sh, "%s area (%u):", area->name, area->id);
	shell_print(sh, "  version: %u.%u.%u+%u",
		    (unsigned int) hdr.h.v1.sem_ver.major,
		    (unsigned int) hdr.h.v1.sem_ver.minor,
		    (unsigned int) hdr.h.v1.sem_ver.revision,
		    (unsigned int) hdr.h.v1.sem_ver.build_num);
	shell_print(sh, "  image size: %u",
		    (unsigned int) hdr.h.v1.image_size);

	err = boot_read_swap_state_by_id(area->id, &swap_state);
	if (err) {
		shell_error(sh, "failed to read %s area (%u) %s: %d",
			    area->name, area->id, "swap state", err);
		return err;
	}

	shell_print(sh, "  magic: %s",
		    swap_state_magic_str(swap_state.magic));

	if (IS_ENABLED(CONFIG_MCUBOOT_TRAILER_SWAP_TYPE)) {
		shell_print(sh, "  swap type: %s",
			    swap_type_str(swap_state.swap_type));
	}

	shell_print(sh, "  copy done: %s",
		    swap_state_flag_str(swap_state.copy_done));
	shell_print(sh, "  image ok: %s",
		    swap_state_flag_str(swap_state.image_ok));

	return 0;
}

static int cmd_mcuboot_info(const struct shell *sh, size_t argc,
			    char **argv)
{
	int i;

	shell_print(sh, "swap type: %s", swap_type_str(mcuboot_swap_type()));
	shell_print(sh, "confirmed: %d", boot_is_img_confirmed());

	for (i = 0; i < ARRAY_SIZE(areas); i++) {
		shell_print(sh, "");
		cmd_mcuboot_info_area(sh, &areas[i]);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mcuboot_cmds,
	SHELL_CMD_ARG(confirm, NULL, "confirm", cmd_mcuboot_confirm, 1, 0),
	SHELL_CMD_ARG(erase, NULL, "erase <area_id>", cmd_mcuboot_erase, 2, 0),
	SHELL_CMD_ARG(request_upgrade, NULL, "request_upgrade [permanent]",
		      cmd_mcuboot_request_upgrade, 1, 1),
#ifdef CONFIG_RETENTION_BOOT_MODE
	SHELL_CMD_ARG(serial_recovery, NULL, "serial_recovery", cmd_mcuboot_serial_recovery, 1, 0),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mcuboot, &mcuboot_cmds, "MCUboot commands",
		   cmd_mcuboot_info);
