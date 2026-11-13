/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLES_SUBSYS_USB_UF2_UF2_H_
#define SAMPLES_SUBSYS_USB_UF2_UF2_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

/* UF2 file format, see https://github.com/microsoft/uf2 */

#define UF2_MAGIC_START0	0x0A324655UL
#define UF2_MAGIC_START1	0x9E5D5157UL
#define UF2_MAGIC_END		0x0AB16F30UL

#define UF2_FLAG_NOFLASH	0x00000001UL
#define UF2_FLAG_FILECONTAINER	0x00001000UL
#define UF2_FLAG_FAMILY_ID	0x00002000UL

#define UF2_BLOCK_SIZE		512U
#define UF2_DATA_SIZE		476U

/* Payload size of the blocks in CURRENT.UF2 */
#define UF2_CURRENT_PAYLOAD	256U

struct uf2_block {
	uint32_t magic_start0;
	uint32_t magic_start1;
	uint32_t flags;
	uint32_t target_addr;
	uint32_t payload_size;
	uint32_t block_no;
	uint32_t num_blocks;
	uint32_t family_id;
	uint8_t data[UF2_DATA_SIZE];
	uint32_t magic_end;
} __packed;

BUILD_ASSERT(sizeof(struct uf2_block) == UF2_BLOCK_SIZE);

#endif /* SAMPLES_SUBSYS_USB_UF2_UF2_H_ */
