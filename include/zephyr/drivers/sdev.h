/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sdev.h
 * @brief SDIO device interface
 */

#ifndef ZEPHYR_DRIVERS_SDEV_H
#define ZEPHYR_DRIVERS_SDEV_H

#include <zephyr/device.h>
#include <zephyr/syscall.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd_dev/sdev_io.h>
#include <zephyr/sd_dev/sdev_pkt.h>
#include <zephyr/sd_dev/sdio_dev.h>
#include <zephyr/sd_dev/sd_dev.h>

/**
 * @brief HAL SDIO device driver API
 *
 * This structure defines the interface that must be implemented
 * by SDIO device drivers.
 */
struct sdev_driver_api {

    /**
     * @brief Read 8-bit register
     *
     * @param dev Device instance
     * @param addr Register address
     * @param val Pointer to store value
     *
     * @return 0 on success, negative error code on failure
     */
    int (*read_reg8)(const struct device *dev,
                     uint32_t addr,
                     uint8_t *val);

    /**
     * @brief Read 32-bit register
     *
     * @param dev Device instance
     * @param addr Register address
     * @param val Pointer to store value
     *
     * @return 0 on success, negative error code on failure
     */
    int (*read_reg32)(const struct device *dev,
                      uint32_t addr,
                      uint32_t *val);

    /**
     * @brief Write 8-bit register
     *
     * @param dev Device instance
     * @param addr Register address
     * @param val Value to write
     *
     * @return 0 on success, negative error code on failure
     */
    int (*write_reg8)(const struct device *dev,
                      uint32_t addr,
                      uint8_t val);

    /**
     * @brief Write 32-bit register
     *
     * @param dev Device instance
     * @param addr Register address
     * @param val Value to write
     *
     * @return 0 on success, negative error code on failure
     */
    int (*write_reg32)(const struct device *dev,
                       uint32_t addr,
                       uint32_t val);

    /**
     * @brief Configure CIS tuple
     *
     * @param dev Device instance
     *
     * @return 0 on success, negative error code on failure
     */
    int (*cis_tuple_configurate)(const struct device *dev);

    /**
     * @brief Set device ready status
     *
     * @param dev Device instance
     *
     * @return 0 on success, negative error code on failure
     */
    int (*set_dev_ready)(const struct device *dev);

    /**
     * @brief Send data through SDIO
     *
     * @param func SDIO function context
     * @param pkt Pointer to packet
     *
     * @return 0 on success, negative error code on failure
     */
    int (*send_data)(const struct sdio_dev_func *func,
                     sdev_pkt_t *pkt);
};

__syscall int sdev_read_reg8(const struct device *dev,
                uint32_t addr,
                uint8_t *val);

__syscall int sdev_read_reg32(const struct device *dev,
                 uint32_t addr,
                 uint32_t *val);

__syscall int sdev_write_reg8(const struct device *dev,
                 uint32_t addr,
                 uint8_t val);

__syscall int sdev_write_reg32(const struct device *dev,
                  uint32_t addr,
                  uint32_t val);

__syscall int sdev_cis_table_configurate(const struct device *dev);

__syscall int sdev_set_dev_ready(const struct device *dev);

__syscall int sdio_send_data(const struct sdio_dev_func *func,
                sdev_pkt_t *pkt);

/* implementations */
static inline int z_impl_sdev_read_reg8(const struct device *dev,
                    uint32_t addr,
                    uint8_t *val)
{
    const struct sdev_driver_api *api =
        (const struct sdev_driver_api *)dev->api;

    if (!api->read_reg8) {
        return -ENOTSUP;
    }

    return api->read_reg8(dev, addr, val);
}

static inline int z_impl_sdev_read_reg32(const struct device *dev,
                     uint32_t addr,
                     uint32_t *val)
{
    const struct sdev_driver_api *api =
        (const struct sdev_driver_api *)dev->api;

    if (!api->read_reg32) {
        return -ENOTSUP;
    }

    return api->read_reg32(dev, addr, val);
}

static inline int z_impl_sdev_write_reg8(const struct device *dev,
                     uint32_t addr,
                     uint8_t val)
{
    const struct sdev_driver_api *api =
        (const struct sdev_driver_api *)dev->api;

    if (!api->write_reg8) {
        return -ENOTSUP;
    }

    return api->write_reg8(dev, addr, val);
}

static inline int z_impl_sdev_write_reg32(const struct device *dev,
                      uint32_t addr,
                      uint32_t val)
{
    const struct sdev_driver_api *api =
        (const struct sdev_driver_api *)dev->api;

    if (!api->write_reg32) {
        return -ENOTSUP;
    }

    return api->write_reg32(dev, addr, val);
}

static inline int z_impl_sdev_cis_table_configurate(const struct device *dev)
{
    const struct sdev_driver_api *api =
        (const struct sdev_driver_api *)dev->api;

    if (!api->cis_tuple_configurate) {
        return -ENOTSUP;
    }

    return api->cis_tuple_configurate(dev);
}

static inline int z_impl_sdev_set_dev_ready(const struct device *dev)
{
    const struct sdev_driver_api *api =
        (const struct sdev_driver_api *)dev->api;

    if (!api->set_dev_ready) {
        return -ENOTSUP;
    }

    return api->set_dev_ready(dev);
}

static inline int z_impl_sdio_send_data(const struct sdio_dev_func *func,
                    sdev_pkt_t *pkt)
{
    const struct device *dev = func->sdio->card->dev;
    const struct sdev_driver_api *api =
        (const struct sdev_driver_api *)dev->api;

    if (!api->send_data) {
        return -ENOTSUP;
    }

    return api->send_data(func, pkt);
}

#include <zephyr/syscalls/sdev.h>

#endif /* ZEPHYR_DRIVERS_SDEV_H */
