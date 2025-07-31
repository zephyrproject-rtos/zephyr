/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_BOOT_LPC54S018_H
#define SECURE_BOOT_LPC54S018_H

#include <stdint.h>
#include <stdbool.h>

/* Secure boot configuration structure */
struct secure_boot_config {
	uint32_t otp_value;         /* Raw OTP BOOTROM register value */
	bool secure_boot_enabled;   /* True if secure boot is enabled */
	uint8_t secure_boot_type;   /* 0=Disabled, 1=CMAC, 2=ECDSA, 3=User */
	uint8_t revoke_id;         /* Minimum required image version */
};

/* Initialize secure boot subsystem */
int secure_boot_init(void);

/* Read secure boot configuration from OTP */
int secure_boot_read_config(struct secure_boot_config *config);

/* Check if version meets revocation requirements */
bool secure_boot_check_version(uint8_t image_version);

/* Verify boot image */
int secure_boot_verify_image(const uint8_t *image_data, size_t image_size);

/* Verify and jump to application */
int secure_boot_verify_and_jump(uint32_t image_address);

#endif /* SECURE_BOOT_LPC54S018_H */