/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SD Device IO Internal APIs
 *
 * This file provides internal core APIs for SD device subsystem,
 * including initialization, packet dispatch, and I/O operations.
 */

#ifndef ZEPHYR_SD_DEV_IO_H
#define ZEPHYR_SD_DEV_IO_H

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sd_dev/sd_dev_pkt.h>

/**
 * @brief SD Device IO APIs
 * @defgroup sd_dev_io SD Device IO APIs
 * @ingroup sd_dev_interface
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/* Forward declarations */
struct sd_dev_card;
struct sdio_dev_func;

/**
 * @endcond
 */

/**
 * @brief Dispatch received packet to appropriate handler
 *
 * This function routes a received packet to the appropriate function
 * handler based on the function number.
 *
 * @param dev Pointer to device instance
 * @param pkt Pointer to received packet
 */
void sd_dev_rx_dispatch(const struct device *dev, sd_dev_pkt_t *pkt);

#ifdef CONFIG_SDIO_DEV
/**
 * @brief Read data from SDIO function
 *
 * This function performs a synchronous read operation from the
 * specified SDIO function.
 *
 * @param func Pointer to SDIO function instance
 * @param data Buffer to store read data
 *
 * @retval Number of bytes read on success
 * @retval -EINVAL if func or data is NULL
 * @retval negative errno code on failure
 */
int sdio_read(struct sdio_dev_func *func, uint8_t *data);

/**
 * @brief Write data to SDIO function
 *
 * This function performs a synchronous write operation to the
 * specified SDIO function.
 *
 * @param func Pointer to SDIO function instance
 * @param data Buffer containing data to write
 * @param len Length of data in bytes
 *
 * @retval Number of bytes written on success
 * @retval -EINVAL if func or data is NULL, or len is invalid
 * @retval negative errno code on failure
 */
int sdio_write(struct sdio_dev_func *func, uint8_t *data, int len);

/**
 * @brief Read packet from SDIO function
 *
 * This function reads a complete packet from the specified SDIO function.
 * The caller is responsible for freeing the returned packet using
 * sd_dev_pkt_free().
 *
 * @param func Pointer to SDIO function instance
 *
 * @return Pointer to received packet on success, NULL on failure
 */
sd_dev_pkt_t *sdio_read_pkt(struct sdio_dev_func *func);

/**
 * @brief Write packet to SDIO function
 *
 * This function writes a complete packet to the specified SDIO function.
 *
 * @param func Pointer to SDIO function instance
 * @param pkt Pointer to packet to write
 *
 * @retval 0 on success
 * @retval -EINVAL if func or pkt is NULL
 * @retval negative errno code on failure
 */
int sdio_write_pkt(struct sdio_dev_func *func, sd_dev_pkt_t *pkt);

/**
 * @brief Poll SDIO device for events
 *
 * This function waits for specified events on the SDIO device with
 * an optional timeout.
 *
 * @param card Pointer to SDIO function instance
 * @param events Bitmask of events to wait for (SDEV_POLLIN, SDEV_POLLOUT, etc.)
 * @param revents Pointer to store returned events
 * @param timeout Timeout value (K_NO_WAIT, K_FOREVER, or specific timeout)
 *
 * @retval 0 on success
 * @retval -EINVAL if card or revents is NULL
 * @retval -ETIMEDOUT if timeout occurred
 * @retval negative errno code on failure
 */
int sdio_dev_poll(struct sdio_dev_func *card, uint32_t events, uint32_t *revents,
		  k_timeout_t timeout);

#endif /* CONFIG_SDIO_DEV */

/**
 * @}
 */

#endif /* ZEPHYR_SD_DEV_IO_H */
