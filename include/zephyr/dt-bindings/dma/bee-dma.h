/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_BEE_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_BEE_DMA_H_

/**
 * @file
 * @brief Realtek Bee DMA Devicetree Bindings
 *
 * Common "config" cell field definitions shared by all Bee-series SoCs.
 * The per-SoC handshake (slot) IDs live in the matching per-IC header,
 * e.g. <dt-bindings/dma/rtl87x2g-dma.h> or <dt-bindings/dma/rtl8752h-dma.h>,
 * which each include this file.
 *
 * The "config" cell is the third cell of a "dmas" entry, a 32-bit mask.
 * Compose it from the named macros below instead of writing raw numbers:
 *
 *   #include <dt-bindings/dma/rtl87x2g-dma.h>
 *
 *   &uart2 {
 *           status = "okay";
 *           dmas = <&dma0 2 BEE_DMA_HANDSHAKE_UART2_RX
 *                   (BEE_DMA_P2M | BEE_DMA_SRC_FIXED | BEE_DMA_DST_INC |
 *                    BEE_DMA_SRC_WIDTH_8BIT | BEE_DMA_DST_WIDTH_8BIT |
 *                    BEE_DMA_SRC_MSIZE(BEE_DMA_MSIZE_1) |
 *                    BEE_DMA_DST_MSIZE(BEE_DMA_MSIZE_1) | BEE_DMA_PRIORITY(0))>,
 *                  <&dma0 3 BEE_DMA_HANDSHAKE_UART2_TX
 *                   (BEE_DMA_M2P | BEE_DMA_SRC_INC | BEE_DMA_DST_FIXED |
 *                    BEE_DMA_SRC_WIDTH_8BIT | BEE_DMA_DST_WIDTH_8BIT |
 *                    BEE_DMA_SRC_MSIZE(BEE_DMA_MSIZE_1) |
 *                    BEE_DMA_DST_MSIZE(BEE_DMA_MSIZE_1) | BEE_DMA_PRIORITY(1))>;
 *           dma-names = "rx", "tx";
 *   };
 */

/* Direction, bits [1:0] */
#define BEE_DMA_M2M 0
#define BEE_DMA_M2P 1
#define BEE_DMA_P2M 2

/* Source address adjustment, bits [3:2] */
#define BEE_DMA_SRC_INC   (0 << 2)
#define BEE_DMA_SRC_DEC   (1 << 2)
#define BEE_DMA_SRC_FIXED (2 << 2)

/* Destination address adjustment, bits [5:4] */
#define BEE_DMA_DST_INC   (0 << 4)
#define BEE_DMA_DST_DEC   (1 << 4)
#define BEE_DMA_DST_FIXED (2 << 4)

/* Source data width, bits [7:6] */
#define BEE_DMA_SRC_WIDTH_8BIT  (0 << 6)
#define BEE_DMA_SRC_WIDTH_16BIT (1 << 6)
#define BEE_DMA_SRC_WIDTH_32BIT (2 << 6)

/* Destination data width, bits [9:8] */
#define BEE_DMA_DST_WIDTH_8BIT  (0 << 8)
#define BEE_DMA_DST_WIDTH_16BIT (1 << 8)
#define BEE_DMA_DST_WIDTH_32BIT (2 << 8)

/* Source burst size (msize), bits [12:10] */
#define BEE_DMA_SRC_MSIZE(n) ((n) << 10)
/* Destination burst size (msize), bits [15:13] */
#define BEE_DMA_DST_MSIZE(n) ((n) << 13)

/* Named burst-size values for use with BEE_DMA_SRC_MSIZE / BEE_DMA_DST_MSIZE */
#define BEE_DMA_MSIZE_1   0
#define BEE_DMA_MSIZE_4   1
#define BEE_DMA_MSIZE_8   2
#define BEE_DMA_MSIZE_16  3
#define BEE_DMA_MSIZE_32  4
#define BEE_DMA_MSIZE_64  5
#define BEE_DMA_MSIZE_128 6
#define BEE_DMA_MSIZE_256 7

/* Channel priority, bits [20:16]. Bee supports 0 to 9. */
#define BEE_DMA_PRIORITY(n) ((n) << 16)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_BEE_DMA_H_ */
