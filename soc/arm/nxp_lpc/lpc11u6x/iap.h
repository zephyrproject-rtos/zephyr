/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Describe the IAP commands interface on NXP LPC11U6x MCUs.
 *
 * The IAP (In-Application Programming) commands are located in the boot ROM
 * code. Mostly they provide access to the on-chip flash and EEPROM devices.
 *
 * @note For details about IAP see the UM10732 LPC11U6x/E6x user manual,
 *       chapter 27: LPC11U6x/E6x Flash/EEPROM ISP/IAP programming.
 */

#ifndef LPC11U6X_IAP_H_
#define LPC11U6X_IAP_H_

/* Pointer to IAP function. */
#define IAP_ENTRY_ADDR			0X1FFF1FF1

/* IAP commands. */
#define IAP_CMD_FLASH_PREP_SEC		50
#define IAP_CMD_FLASH_WRITE_SEC		51
#define IAP_CMD_FLASH_ERASE_SEC		52
#define IAP_CMD_FLASH_BLANK_CHECK_SEC	53
#define IAP_CMD_READ_PART_ID		54
#define IAP_CMD_READ_BOOT_CODE_VER	55
#define IAP_CMD_MEM_COMPARE		56
#define IAP_CMD_REINVOKE_ISP		57
#define IAP_CMD_READ_UID		58
#define IAP_CMD_FLASH_ERASE_PAGE	59
#define IAP_CMD_EEPROM_WRITE		61
#define IAP_CMD_EEPROM_READ		62

/* IAP status codes. */
#define IAP_STATUS_CMD_SUCCESS		0
#define IAP_STATUS_INVALID_CMD		1
#define IAP_STATUS_SRC_ADDR_ERROR	2
#define IAP_STATUS_DST_ADDR_ERROR	3
#define IAP_STATUS_SRC_ADDR_NOT_MAPPED	4
#define IAP_STATUS_DST_ADDR_NOT_MAPPED	5
#define IAP_STATUS_COUNT_ERROR		6
#define IAP_STATUS_INVALID_SECTOR	7
#define IAP_STATUS_SECTOR_NOT_BLANK	8
#define IAP_STATUS_SECTOR_NOT_PREPARED	9
#define IAP_STATUS_COMPARE_ERROR	10
#define IAP_STATUS_BUSY			11

/**
 * @brief Entry function for IAP commands.
 */
static inline int iap_cmd(unsigned int cmd[5])
{
	int key;
	int status[5];

	/*
	 * Interrupts must be disabled when calling IAP. Indeed when executing
	 * "some" commands, the flash (where the interrupt vectors are located)
	 * is no longer accessible.
	 */
	key = irq_lock();

	/*
	 * TODO: for the flash commands, the top 32 bytes of memory must be
	 * saved before calling the IAP function, and restored after. According
	 * to the UM10732 user manual, the IAP code may use them.
	 */
	((void (*)(unsigned int[], unsigned int[]))
		IAP_ENTRY_ADDR)(cmd, status);

	irq_unlock(key);

	return status[0];
}

#endif /* LPC11U6X_IAP_H_ */
