/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_SMARTDMA_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_SMARTDMA_H_

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

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_SMARTDMA_H_ */
