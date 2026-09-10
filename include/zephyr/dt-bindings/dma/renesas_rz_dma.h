/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZ_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZ_DMA_H_

/* mode:                bit 0 (0: Normal, 1: Block) */
/* source data size:    bit 1, 2, 3 (0b000 -> 0b111) */
/* dest data size:      bit 4, 5, 6 (0b000 -> 0b111) */
/* source addr mode:    bit 7 (0: incremented, 1: fixed) */
/* dest addr mode:      bit 8 (0: incremented, 1: fixed) */

#define RZ_DMA_MODE_NORMAL (0U)
#define RZ_DMA_MODE_BLOCK  (1U)

/* DMA source data size config on bits 1, 2, 3 */
#define RZ_DMA_CFG_SRC_DATA_SIZE(val) ((val & 0x7) << 1)
#define RZ_DMA_SRC_1_BYTE             RZ_DMA_CFG_SRC_DATA_SIZE(0)
#define RZ_DMA_SRC_2_BYTE             RZ_DMA_CFG_SRC_DATA_SIZE(1)
#define RZ_DMA_SRC_4_BYTE             RZ_DMA_CFG_SRC_DATA_SIZE(2)
#define RZ_DMA_SRC_8_BYTE             RZ_DMA_CFG_SRC_DATA_SIZE(3)
#define RZ_DMA_SRC_16_BYTE            RZ_DMA_CFG_SRC_DATA_SIZE(4)
#define RZ_DMA_SRC_32_BYTE            RZ_DMA_CFG_SRC_DATA_SIZE(5)
#define RZ_DMA_SRC_64_BYTE            RZ_DMA_CFG_SRC_DATA_SIZE(6)
#define RZ_DMA_SRC_128_BYTE           RZ_DMA_CFG_SRC_DATA_SIZE(7)

/* DMA destination data size config on bits 4, 5, 6 */
#define RZ_DMA_CFG_DEST_DATA_SIZE(val) ((val & 0x7) << 4)
#define RZ_DMA_DEST_1_BYTE             RZ_DMA_CFG_DEST_DATA_SIZE(0)
#define RZ_DMA_DEST_2_BYTE             RZ_DMA_CFG_DEST_DATA_SIZE(1)
#define RZ_DMA_DEST_4_BYTE             RZ_DMA_CFG_DEST_DATA_SIZE(2)
#define RZ_DMA_DEST_8_BYTE             RZ_DMA_CFG_DEST_DATA_SIZE(3)
#define RZ_DMA_DEST_16_BYTE            RZ_DMA_CFG_DEST_DATA_SIZE(4)
#define RZ_DMA_DEST_32_BYTE            RZ_DMA_CFG_DEST_DATA_SIZE(5)
#define RZ_DMA_DEST_64_BYTE            RZ_DMA_CFG_DEST_DATA_SIZE(6)
#define RZ_DMA_DEST_128_BYTE           RZ_DMA_CFG_DEST_DATA_SIZE(7)

/* DMA source address mode config on bit 7 */
#define RZ_DMA_CFG_SRC_ADDR_MODE(val) ((val & 0x1) << 7)
#define RZ_DMA_SRC_INCREMENTED        RZ_DMA_CFG_SRC_ADDR_MODE(0)
#define RZ_DMA_SRC_FIXED              RZ_DMA_CFG_SRC_ADDR_MODE(1)

/* DMA source address mode config on bit 8 */
#define RZ_DMA_CFG_DEST_ADDR_MODE(val) ((val & 0x1) << 8)
#define RZ_DMA_DEST_INCREMENTED        RZ_DMA_CFG_DEST_ADDR_MODE(0)
#define RZ_DMA_DEST_FIXED              RZ_DMA_CFG_DEST_ADDR_MODE(1)

/* DMA usual combination for peripheral transfer */
#define RZ_DMA_MEM_TO_PERIPH                                                                       \
	(RZ_DMA_MODE_NORMAL | RZ_DMA_SRC_INCREMENTED | RZ_DMA_DEST_FIXED | RZ_DMA_SRC_1_BYTE |     \
	 RZ_DMA_DEST_1_BYTE)
#define RZ_DMA_PERIPH_TO_MEM                                                                       \
	(RZ_DMA_MODE_NORMAL | RZ_DMA_SRC_FIXED | RZ_DMA_DEST_INCREMENTED | RZ_DMA_SRC_1_BYTE |     \
	 RZ_DMA_DEST_1_BYTE)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZ_DMA_H_ */
