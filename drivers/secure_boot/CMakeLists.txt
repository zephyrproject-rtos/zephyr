# Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
# Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LPC54S018_SECURE_BOOT
	secure_boot_lpc54s018.c
	lpc_boot_image.c
	secure_boot_verify.c
	ecdsa_p256.c
	secure_boot_decrypt.c
)