/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <lpc_boot_image.h>
#include <secure_boot_lpc54s018.h>

LOG_MODULE_REGISTER(secure_boot_demo, LOG_LEVEL_INF);

/* External PUF functions */
extern int lpc_puf_enroll(void);
extern int lpc_puf_generate_key(uint8_t key_index, uint8_t key_size);

/* Application image location in SPIFI flash */
#define APP_IMAGE_ADDRESS   0x10010000  /* 64KB offset from start */
#define BOOTLOADER_ADDRESS  0x10000000  /* Start of SPIFI */

void main(void)
{
	int ret;

	LOG_INF("=== LPC54S018 Secure Boot Demo ===");

	/* Initialize secure boot subsystem */
	ret = secure_boot_init();
	if (ret != 0) {
		LOG_ERR("Secure boot initialization failed: %d", ret);
		return;
	}

	LOG_INF("Secure boot initialized");

#ifdef CONFIG_LPC54S018_PUF_SHELL
	/* Check if PUF needs enrollment (development only) */
	LOG_INF("Checking PUF enrollment status...");
	/* PUF enrollment would be done in production provisioning */
#endif

#ifdef CONFIG_LPC54S018_DEV_KEYS
	LOG_WRN("Using development keys - NOT FOR PRODUCTION!");
#endif

	/* In a real bootloader, we would:
	 * 1. Check for firmware update in external storage
	 * 2. Verify the update signature
	 * 3. Program the update to flash if valid
	 * 4. Verify the application image
	 * 5. Jump to the application
	 */

	LOG_INF("Verifying application image at 0x%08X", APP_IMAGE_ADDRESS);

	/* Attempt to verify and execute the application */
	ret = secure_boot_verify_and_jump(APP_IMAGE_ADDRESS);
	if (ret != 0) {
		LOG_ERR("Application verification failed: %d", ret);
		LOG_ERR("System halted - no valid application found");
		
		/* In production, might try fallback image or recovery mode */
		while (1) {
			k_sleep(K_SECONDS(1));
			LOG_WRN("Waiting for valid application...");
		}
	}

	/* Should never reach here */
	LOG_ERR("Application returned unexpectedly");
}

/* Example shell commands for testing */
#ifdef CONFIG_SHELL

#include <zephyr/shell/shell.h>

static int cmd_verify_image(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t addr = APP_IMAGE_ADDRESS;
	int ret;

	if (argc > 1) {
		addr = strtoul(argv[1], NULL, 0);
	}

	shell_print(shell, "Verifying image at 0x%08X", addr);
	
	/* For LPC54S018, the boot header is embedded in the vector table */
	/* The header starts at offset 0x158 in the image */
	const uint8_t *image_data = (const uint8_t *)addr;
	const struct lpc_boot_header *header = (const struct lpc_boot_header *)(image_data + 0x158);
	
	/* Quick check for valid markers */
	if (header->header_marker != LPC_BOOT_HEADER_MARKER ||
	    header->image_marker != LPC_BOOT_IMAGE_MARKER) {
		shell_error(shell, "Invalid boot image markers");
		return -EINVAL;
	}
	
	ret = secure_boot_verify_image(image_data, 4 * 1024 * 1024);
	
	if (ret == 0) {
		shell_print(shell, "Image verification successful");
	} else {
		shell_error(shell, "Image verification failed: %d", ret);
	}

	return ret;
}

static int cmd_boot_status(const struct shell *shell, size_t argc, char **argv)
{
	struct secure_boot_config config;
	int ret;

	ret = secure_boot_read_config(&config);
	if (ret != 0) {
		shell_error(shell, "Failed to read secure boot config: %d", ret);
		return ret;
	}

	shell_print(shell, "Secure Boot Configuration:");
	shell_print(shell, "  Enabled: %s", config.secure_boot_enabled ? "YES" : "NO");
	shell_print(shell, "  Type: %s", 
		    config.secure_boot_type == 0 ? "Disabled" :
		    config.secure_boot_type == 1 ? "CMAC" :
		    config.secure_boot_type == 2 ? "ECDSA" : "User-defined");
	shell_print(shell, "  Revoke ID: %u", config.revoke_id);
	shell_print(shell, "  OTP Value: 0x%08X", config.otp_value);

	return 0;
}

static int cmd_gen_key(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t index = 0;
	uint8_t size = 16;
	int ret;

	if (argc > 1) {
		index = atoi(argv[1]);
	}
	if (argc > 2) {
		size = atoi(argv[2]);
	}

	shell_print(shell, "Generating key: index=%u, size=%u", index, size);
	
	ret = lpc_puf_generate_key(index, size);
	if (ret == 0) {
		shell_print(shell, "Key generated successfully");
	} else {
		shell_error(shell, "Key generation failed: %d", ret);
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(secure_boot_cmds,
	SHELL_CMD_ARG(verify, NULL, "Verify image at address", cmd_verify_image, 1, 1),
	SHELL_CMD(status, NULL, "Show secure boot status", cmd_boot_status),
	SHELL_CMD_ARG(genkey, NULL, "Generate PUF key <index> <size>", cmd_gen_key, 1, 2),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(secboot, &secure_boot_cmds, "Secure boot commands", NULL);

#endif /* CONFIG_SHELL */