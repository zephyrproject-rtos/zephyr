/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_SMARTDMA_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_SMARTDMA_H_

/* Write RGB565 data to MIPI DSI via DMA. */
#define DMA_SMARTDMA_MIPI_RGB565_DMA 0
/* Write RGB888 data to MIPI DSI via DMA */
#define DMA_SMARTDMA_MIPI_RGB888_DMA 1
/* Write RGB565 data to MIPI DSI via DMA. Rotate output data by 180 degrees */
#define DMA_SMARTDMA_MIPI_RGB565_180 2
/* Write RGB888 data to MIPI DSI via DMA. Rotate output data by 180 degrees */
#define DMA_SMARTDMA_MIPI_RGB888_180 3

/* Write RGB565 data to MIPI DSI via DMA. Swap data endianness, so that
 * little endian RGB565 data will be written big endian style.
 */
#define DMA_SMARTDMA_MIPI_RGB565_DMA_SWAP 4
/* Write RGB888 data to MIPI DSI via DMA. Swap data endianness, so that
 * little endian RGB888 data will be written big endian style.
 */
#define DMA_SMARTDMA_MIPI_RGB888_DMA_SWAP 5
/* Write RGB565 data to MIPI DSI via DMA. Rotate output data by 180 degrees,
 * and swap data endianness
 */
#define DMA_SMARTDMA_MIPI_RGB565_180_SWAP 6
/* Write RGB888 data to MIPI DSI via DMA. Rotate output data by 180 degrees,
 * and swap data endianness
 */
#define DMA_SMARTDMA_MIPI_RGB888_180_SWAP 7



/**
 * @brief install SMARTDMA firmware
 *
 * Install a custom firmware for the smartDMA. This function allows the user
 * to install a custom firmware into the smartDMA, which implements
 * different API functions than the standard MCUX SDK firmware.
 * @param dev: smartDMA device
 * @param firmware: address of buffer containing smartDMA firmware
 * @param len: length of firmware buffer
 */
void dma_smartdma_install_fw(const struct device *dev, uint8_t *firmware,
			     uint32_t len);

#define GD32_DMA_FEATURES_FIFO_THRESHOLD(threshold) (threshold & 0x3)

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_SMARTDMA_H_ */
