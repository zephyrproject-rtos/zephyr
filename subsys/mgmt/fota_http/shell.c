/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/mgmt/fota_http.h>

struct shell_download {
	const struct shell *sh;
	size_t last_pct;
};

static struct shell_download download;
static char url_copy[CONFIG_FOTA_HTTP_URL_MAX_LEN];

static void progress_cb(const struct fota_http_progress *progress, void *user_data)
{
	struct shell_download *dl = user_data;
	size_t pct;

	if (progress->total == 0) {
		return;
	}

	pct = (progress->written * 100U) / progress->total;
	if (pct >= dl->last_pct + 10U) {
		dl->last_pct = pct - (pct % 10U);
		shell_print(dl->sh, "%3zu%% (%zu/%zu bytes)", pct, progress->written,
			    progress->total);
	}
}

static void done_cb(int result, void *user_data)
{
	struct shell_download *dl = user_data;

	if (result != 0) {
		shell_error(dl->sh, "Download failed (%d)", result);
		return;
	}

	shell_print(dl->sh, "Stored %zu bytes in the secondary slot", fota_http_bytes_written());
	shell_print(dl->sh, "Run 'fota apply' to test it once or 'fota apply permanent' to "
			    "keep it");
}

static int cmd_download(const struct shell *sh, size_t argc, char **argv)
{
	struct fota_http_download_params params = {
		.url = url_copy,
		.progress_cb = progress_cb,
		.user_data = &download,
	};
	int ret;

	for (size_t i = 2; i < argc; i++) {
		if (strcmp(argv[i], "resume") == 0) {
			params.resume = true;
		} else if (strncmp(argv[i], "image=", 6) == 0) {
			params.image_index = (uint8_t)strtoul(argv[i] + 6, NULL, 10);
		} else {
			shell_error(sh, "Unknown argument '%s'", argv[i]);
			return -EINVAL;
		}
	}

	if (strlen(argv[1]) >= sizeof(url_copy)) {
		shell_error(sh, "URL too long");
		return -EINVAL;
	}
	strcpy(url_copy, argv[1]);

	download.sh = sh;
	download.last_pct = 0;

	ret = fota_http_download_async(&params, done_cb);
	if (ret != 0) {
		shell_error(sh, "Cannot start download (%d)", ret);
		return ret;
	}

	shell_print(sh, "Downloading %s", url_copy);

	return 0;
}

static int cmd_cancel(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = fota_http_cancel();
	if (ret != 0) {
		shell_error(sh, "No download in progress");
		return ret;
	}

	shell_print(sh, "Cancel requested");

	return 0;
}

static int cmd_apply(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t image_index = 0;
	bool permanent = false;
	int ret;

	for (size_t i = 1; i < argc; i++) {
		if (strcmp(argv[i], "permanent") == 0) {
			permanent = true;
		} else if (strncmp(argv[i], "image=", 6) == 0) {
			image_index = (uint8_t)strtoul(argv[i] + 6, NULL, 10);
		} else {
			shell_error(sh, "Unknown argument '%s'", argv[i]);
			return -EINVAL;
		}
	}

	ret = fota_http_apply(image_index, permanent);
	if (ret != 0) {
		shell_error(sh, "Apply failed (%d)", ret);
		return ret;
	}

	if (permanent) {
		shell_print(sh, "Image marked permanent, reboot to run it");
	} else {
		shell_print(sh, "Image marked for test, reboot to run it once");
		shell_print(sh, "Confirm it with 'fota confirm' or the bootloader reverts it");
	}

	return 0;
}

static int cmd_confirm(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t image_index = 0;
	int ret;

	if (argc > 1) {
		image_index = (uint8_t)strtoul(argv[1], NULL, 10);
	}

	ret = fota_http_confirm(image_index);
	if (ret != 0) {
		shell_error(sh, "Confirm failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "Image confirmed");

	return 0;
}

static const char *swap_type_str(int type)
{
	switch (type) {
	case BOOT_SWAP_TYPE_NONE:
		return "none";
	case BOOT_SWAP_TYPE_TEST:
		return "test";
	case BOOT_SWAP_TYPE_PERM:
		return "permanent";
	case BOOT_SWAP_TYPE_REVERT:
		return "revert";
	default:
		return "unknown";
	}
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	struct mcuboot_img_header header;
	int slot;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (fota_http_confirm_pending()) {
		shell_print(sh, "Running image: not confirmed, reverted on next reboot");
	} else {
		shell_print(sh, "Running image: confirmed");
	}

	shell_print(sh, "Pending swap: %s", swap_type_str(mcuboot_swap_type()));

	slot = fota_http_upload_slot(0);
	if (slot >= 0) {
		shell_print(sh, "Upload slot: flash area %d", slot);
	}

	ret = fota_http_image_header(0, &header);
	if (ret == 0) {
		size_t written = fota_http_bytes_written();

		/* The header lands first, so a cut download still reads back as an image. */
		shell_print(sh, "Secondary slot image: version %u.%u.%u+%u, %u bytes%s",
			    header.h.v1.sem_ver.major, header.h.v1.sem_ver.minor,
			    header.h.v1.sem_ver.revision, header.h.v1.sem_ver.build_num,
			    header.h.v1.image_size,
			    (written > 0 && written < header.h.v1.image_size) ? " (incomplete)"
									      : "");
	} else {
		shell_print(sh, "Secondary slot image: none");
	}

	shell_print(sh, "Last download: %zu bytes", fota_http_bytes_written());

	return 0;
}

static int cmd_erase(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t image_index = 0;
	int slot;
	int ret;

	if (argc > 1) {
		image_index = (uint8_t)strtoul(argv[1], NULL, 10);
	}

	slot = fota_http_upload_slot(image_index);
	if (slot < 0) {
		shell_error(sh, "No slot for image %u (%d)", image_index, slot);
		return slot;
	}

	ret = boot_erase_img_bank((uint8_t)slot);
	if (ret != 0) {
		shell_error(sh, "Erase failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "Secondary slot erased");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	fota_cmds,
	SHELL_CMD_ARG(download, NULL,
		      "<url> [resume] [image=<n>] - download an image into the secondary slot",
		      cmd_download, 2, 2),
	SHELL_CMD_ARG(cancel, NULL, "Abort the download in progress", cmd_cancel, 1, 0),
	SHELL_CMD_ARG(apply, NULL,
		      "[permanent] [image=<n>] - boot the downloaded image on next reset",
		      cmd_apply, 1, 2),
	SHELL_CMD_ARG(confirm, NULL, "[image] - confirm the running image", cmd_confirm, 1, 1),
	SHELL_CMD_ARG(status, NULL, "Show images and confirmation state", cmd_status, 1, 0),
	SHELL_CMD_ARG(erase, NULL, "[image] - erase the secondary slot", cmd_erase, 1, 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(fota, &fota_cmds, "Firmware update over HTTP", NULL);
