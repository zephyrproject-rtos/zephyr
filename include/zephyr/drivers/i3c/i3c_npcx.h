/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_I3C_NPCX_H_
#define ZEPHYR_DRIVERS_I3C_I3C_NPCX_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_I3C_NPCX_DMA

/**
 * @brief Configures the application's buffer for use in MDMA mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mdma_rd_buf Pointer to storage for read data
 * @param mdma_rd_buf_size Length of the buffer to storage for read data
 * @param mdma_wr_buf Pointer to the data to be written
 * @param mdma_wr_buf_size Length of the buffer to be written
 */
void npcx_i3c_target_set_mdma_buff(const struct device *dev, uint8_t *mdma_rd_buf,
				   uint16_t mdma_rd_buf_size, uint8_t *mdma_wr_buf,
				   uint16_t mdma_wr_buf_size);

/**
 * @brief Retrieves the count of data received via MDMA.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return The number of data bytes read from the bus.
 */
uint16_t npcx_i3c_target_get_mdmafb_count(const struct device *dev);

/**
 * @brief Retrieves the count of data written to the bus via MDMA.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return The number of data bytes written to the bus.
 */
uint16_t npcx_i3c_target_get_mdmatb_count(const struct device *dev);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I3C_I3C_NPCX_H_ */
