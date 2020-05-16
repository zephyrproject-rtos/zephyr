/*
 * Copyright (c) 2020 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_SFDP_H_
#define ZEPHYR_DRIVERS_FLASH_SFDP_H_

#include <zephyr/types.h>
#include <sys/util.h>
#include <sys/byteorder.h>

union sfdp_header {
	u32_t word[2];
	u8_t  byte[8];
};

union sfdp_dword {
	u32_t word;
	u8_t  byte[4];
};

#define SFDP_HEADER_ADDRESS     0x00000000  /* Address of SFDP header */
#define SFDP_SIGNATURE          0x50444653  /* SFDP Signature */
#define SFDP_HEADER_JEDEC_ID             0  /* JEDEC header Id */

/*
 * Utility functions to deserialize Serial Flash Discovery Parameter (SFDP)
 * header data.
 */

/* Get signature from the SFDP Base Header
 *
 * @param base_header Pointer to data structure holding the base header.
 * @return SFDP Signature.
 */
static inline u32_t sfdp_get_header_signature(union sfdp_header *base_header)
{
	return sys_le32_to_cpu(base_header->word[0]);
}

/* Get Parameter Header Id number.
 *
 * @param param_header Pointer to data structure holding the parameter header.
 * @return Id number, 0x00 indicates a JEDEC specified header.
 */
static inline u8_t sfdp_get_param_header_id(union sfdp_header *param_header)
{
	return param_header->byte[0];
}

/* Get Parameter Table Length.
 *
 * @param param_header Pointer to data structure holding the parameter header.
 * @return number of dwords in parameter table.
 */
static inline u8_t sfdp_get_param_header_pt_length(union sfdp_header *param_header)
{
	return param_header->byte[3];
}

/* Get Parameter Table Pointer (PTP).
 *
 * @param param_header Pointer to data structure holding the parameter header.
 * @return 24-bit address indicating the start of the Parameter Table
 *         corresponding to this header.
 */
static inline off_t sfdp_get_param_header_pt_pointer(union sfdp_header *param_header)
{
	return sys_le32_to_cpu(param_header->word[1]) & 0x00FFFFFF;
}

/*
 * Basic Flash Parameter Table v1.0
 */

/* 2nd DWORD, bit 31
 *
 * @param dw 2nd dword in little-endian byte order.
 * @return true if flash density is greater than 2 gigabits.
 */
static inline bool sfdp_pt_1v0_dw2_is_gt_2_gigabits(union sfdp_dword dw)
{
	return (dw.byte[3] & 0x80) != 0;
}

/* 2nd DWORD, bits 30:0
 *
 * @param dw 2nd dword in little-endian byte order.
 * @return N indicating flash memory density, where:
 * - flash memory density is N+1 bits for densities 2 gigabits or less
 * - flash memory density is 2^N bits for densities greater than 2 gigabits
 */
static inline u32_t sfdp_pt_1v0_dw2_get_density_n(union sfdp_dword dw)
{
	return sys_le32_to_cpu(dw.word) & 0x7FFFFFFF;
}

/* 8th DWORD, bits 31:24
 *
 * @param dw 8th dword in little-endian byte order.
 * @return Sector Type 2 Erase Opcode
 */
static inline u8_t sfdp_pt_1v0_dw8_get_sector_type_2_erase_opcode(union sfdp_dword dw)
{
	return dw.byte[3];
}

/* 8th DWORD, bits 23:16
 *
 * @param dw 8th dword in little-endian byte order.
 * @return N indicating Sector Type 2 Size, where: sector/block size = 2^N bytes
 */
static inline u8_t sfdp_pt_1v0_dw8_get_sector_type_2_size_n(union sfdp_dword dw)
{
	return dw.byte[2];
}

/* 8th DWORD, bits 15:8
 *
 * @param dw 8th dword in little-endian byte order.
 * @return Sector Type 1 Erase Opcode
 */
static inline u8_t sfdp_pt_1v0_dw8_get_sector_type_1_erase_opcode(union sfdp_dword dw)
{
	return dw.byte[1];
}

/* 8th DWORD, bits 7:0
 *
 * @param dw 8th dword in little-endian byte order.
 * @return N indicating Sector Type 1 Size, where: sector/block size = 2^N bytes
 */
static inline u8_t sfdp_pt_1v0_dw8_get_sector_type_1_size_n(union sfdp_dword dw)
{
	return dw.byte[0];
}

/* 9th DWORD, bits 31:24
 *
 * @param dw 9th dword in little-endian byte order.
 * @return Sector Type 4 Erase Opcode
 */
static inline u8_t sfdp_pt_1v0_dw9_get_sector_type_4_erase_opcode(union sfdp_dword dw)
{
	return dw.byte[3];
}

/* 9th DWORD, bits 23:16
 *
 * @param dw 9th dword in little-endian byte order.
 * @return N indicating Sector Type 4 Size, where: sector/block size = 2^N bytes
 */
static inline u8_t sfdp_pt_1v0_dw9_get_sector_type_4_size_n(union sfdp_dword dw)
{
	return dw.byte[2];
}

/* 9th DWORD, bits 15:8
 *
 * @param dw 9th dword in little-endian byte order.
 * @return Sector Type 3 Erase Opcode
 */
static inline u8_t sfdp_pt_1v0_dw9_get_sector_type_3_erase_opcode(union sfdp_dword dw)
{
	return dw.byte[1];
}

/* 9th DWORD, bits 7:0
 *
 * @param dw 9th dword in little-endian byte order.
 * @return N indicating Sector Type 3 Size, where: sector/block size = 2^N bytes
 */
static inline u8_t sfdp_pt_1v0_dw9_get_sector_type_3_size_n(union sfdp_dword dw)
{
	return dw.byte[0];
}

#endif /* ZEPHYR_DRIVERS_FLASH_SFDP_H_ */
