
#ifndef ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_SPI_H_
#define ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_SPI_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

#include "scl3300.h"

/**
 * @brief Submit an asynchronous SPI write transaction.
 *
 * @param dev Pointer to the SCL3300 device.
 * @param command SPI command to transmit.
 *
 * @return 0 on successful submission, or a negative error code.
 */
int scl3300_spi_write(const struct device *dev, uint32_t command);

/**
 * @brief Submit an asynchronous SPI read transaction.
 *
 * @param dev Pointer to the SCL3300 device.
 * @param iodev_sqe Pointer to the RTIO I/O device SQE.
 * @param command SPI command to transmit.
 *
 * @return 0 on successful submission, or a negative error code.
 */
int scl3300_spi_read(const struct device *dev,
                           struct rtio_iodev_sqe *iodev_sqe,
                           uint32_t command);

#endif /* ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_SPI_H_ */
