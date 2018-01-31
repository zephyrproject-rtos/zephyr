/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MCUBOOT_H__
#define __MCUBOOT_H__

#include <stdbool.h>

/**
 * @brief Check if the currently running image is confirmed as OK.
 *
 * MCUboot can perform "test" upgrades. When these occur, a new
 * firmware image is installed and booted, but the old version will be
 * reverted at the next reset unless the new image explicitly marks
 * itself OK.
 *
 * This routine can be used to check if the currently running image
 * has been marked as OK.
 *
 * @return True if the image is confirmed as OK, false otherwise.
 * @see boot_write_img_confirmed()
 */
bool boot_is_img_confirmed(void);

/**
 * @brief Marks the currently running image as confirmed.
 *
 * This routine attempts to mark the currently running firmware image
 * as OK, which will install it permanently, preventing MCUboot from
 * reverting it for an older image at the next reset.
 *
 * This routine is safe to call if the current image has already been
 * confirmed. It will return a successful result in this case.
 *
 * @return 0 on success, negative errno code on fail.
 */
int boot_write_img_confirmed(void);

/**
 * @brief Marks the image in slot 1 as pending. On the next reboot, the system
 * will perform a boot of the slot 1 image.
 *
 * @param permanent Whether the image should be used permanently or
 * only tested once:
 *   0=run image once, then confirm or revert.
 *   1=run image forever.
 * @return 0 on success, negative errno code on fail.
 */
int boot_request_upgrade(int permanent);

/**
 * @brief Erase the image Bank.
 *
 * @param bank_offset address of the image bank
 * @return 0 on success, negative errno code on fail.
 */
int boot_erase_img_bank(u32_t bank_offset);

#endif	/* __MCUBOOT_H__ */
