/*
 * Copyright 2022 The ChromiumOS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_SERIAL_TEST_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_SERIAL_TEST_H_

#include <errno.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Queues data to be read by the virtual serial port.
 *
 * @warning
 * Use cases involving multiple writers virtual serial port must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes.
 *
 * @param dev Address of virtual serial port.
 * @param data Address of data.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written.
 */
int serial_vnd_queue_in_data(const struct device *dev, unsigned char *data, uint32_t size);

/**
 * @brief Returns size of unread written data.
 *
 * @param dev Address of virtual serial port.
 *
 * @return Output data size (in bytes).
 */
uint32_t serial_vnd_out_data_size_get(const struct device *dev);

/**
 * @brief Read data written to virtual serial port.
 *
 * Consumes the data, such that future calls to serial_vnd_read_out_data() will
 * not include this data. Requires CONFIG_RING_BUFFER.
 *
 * @warning
 * Use cases involving multiple reads of the data must prevent
 * concurrent read operations, either by preventing all readers from
 * being preempted or by using a mutex to govern reads.
 *
 *
 * @param dev Address of virtual serial port.
 * @param data Address of the output buffer. Can be NULL to discard data.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to the output buffer.
 */
uint32_t serial_vnd_read_out_data(const struct device *dev, unsigned char *data, uint32_t size);

/**
 * @brief Peek at data written to virtual serial port.
 *
 * Reads the data without consuming it. Future calls to serial_vnd_peek_out_data() or
 * serial_vnd_read_out_data() will return this data again.  Requires CONFIG_RING_BUFFER.
 *
 * @warning
 * Use cases involving multiple reads of the data must prevent
 * concurrent read operations, either by preventing all readers from
 * being preempted or by using a mutex to govern reads.
 *
 *
 * @param dev Address of virtual serial port.
 * @param data Address of the output buffer. Cannot be NULL.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to the output buffer.
 */
uint32_t serial_vnd_peek_out_data(const struct device *dev, unsigned char *data, uint32_t size);

/**
 * @brief Callback called after bytes written to the virtual serial port.
 *
 * @param dev Address of virtual serial port.
 * @param user_data User data.
 */
typedef void (*serial_vnd_write_cb_t)(const struct device *dev, void *user_data);

/**
 * @brief Sets the write callback on a virtual serial port.
 *
 * @param dev Address of virtual serial port.
 * @param callback Function to call after each write to the port.
 * @param user_data Opaque data to pass to the callback.
 *
 */
void serial_vnd_set_callback(const struct device *dev, serial_vnd_write_cb_t callback,
			     void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_SERIAL_TEST_H_ */
