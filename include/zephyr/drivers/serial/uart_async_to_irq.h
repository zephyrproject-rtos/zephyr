/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_ASYNC_TO_IRQ_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_ASYNC_TO_IRQ_H_

#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/serial/uart_async_rx.h>

/**
 * @brief UART Asynchronous to Interrupt driven API adaptation layer
 * @ingroup uart_interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations. */

/** @brief Data structure used by the adaptation layer.
 *
 * Pointer to that data must be the first element of the UART device data structure.
 */
struct uart_async_to_irq_data;

/** @brief Configuration structure used by the adaptation layer.
 *
 * Pointer to this data must be the first element of the UART device configuration structure.
 */
struct uart_async_to_irq_config;

/* @brief Function that triggers trampoline to the interrupt context.
 *
 * This context is used to call user UART interrupt handler. It is to used to
 * fulfill the requirement that UART interrupt driven API shall be called from
 * the UART interrupt. Trampoline context shall have the same priority as UART.
 *
 * One option may be to use k_timer configured to expire immediately.
 */
typedef void (*uart_async_to_irq_trampoline)(const struct device *dev);

/** @brief Callback to be called from trampoline context.
 *
 * @param dev UART device.
 */
void uart_async_to_irq_trampoline_cb(const struct device *dev);

/** @brief Interrupt driven API initializer.
 *
 * It should be used in the initialization of the UART API structure in the
 * driver to provide interrupt driven API functions.
 */
#define UART_ASYNC_TO_IRQ_API_INIT()				\
	.fifo_fill	  = z_uart_async_to_irq_fifo_fill,	\
	.fifo_read	  = z_uart_async_to_irq_fifo_read,	\
	.irq_tx_enable	  = z_uart_async_to_irq_irq_tx_enable,	\
	.irq_tx_disable	  = z_uart_async_to_irq_irq_tx_disable,	\
	.irq_tx_ready	  = z_uart_async_to_irq_irq_tx_ready,	\
	.irq_rx_enable	  = z_uart_async_to_irq_irq_rx_enable,	\
	.irq_rx_disable	  = z_uart_async_to_irq_irq_rx_disable,	\
	.irq_tx_complete  = z_uart_async_to_irq_irq_tx_complete,\
	.irq_rx_ready	  = z_uart_async_to_irq_irq_rx_ready,	\
	.irq_err_enable	  = z_uart_async_to_irq_irq_err_enable,	\
	.irq_err_disable  = z_uart_async_to_irq_irq_err_disable,\
	.irq_is_pending	  = z_uart_async_to_irq_irq_is_pending,	\
	.irq_update	  = z_uart_async_to_irq_irq_update,	\
	.irq_callback_set = z_uart_async_to_irq_irq_callback_set

/** @brief Configuration structure initializer.
 *
 * @param _api Structure with UART asynchronous API.
 * @param _trampoline Function that trampolines to the interrupt context.
 * @param _baudrate UART baudrate.
 * @param _tx_buf TX buffer.
 * @param _tx_len TX buffer length.
 * @param _rx_buf RX buffer.
 * @param _rx_len RX buffer length.
 * @param _rx_cnt Number of chunks into which RX buffer is divided.
 * @param _log Logging instance, if not provided (empty) then default is used.
 */
#define UART_ASYNC_TO_IRQ_API_CONFIG_INITIALIZER(_api, _trampoline, _baudrate, _tx_buf,		\
						 _tx_len, _rx_buf, _rx_len, _rx_cnt, _log)	\
	{											\
		.tx_buf = _tx_buf,								\
		.tx_len = _tx_len,								\
		.async_rx = {									\
			.buffer = _rx_buf,							\
			.length = _rx_len,							\
			.buf_cnt = _rx_cnt							\
		},										\
		.api = _api,									\
		.trampoline = _trampoline,							\
		.baudrate = _baudrate,								\
		LOG_OBJECT_PTR_INIT(log,							\
				COND_CODE_1(IS_EMPTY(_log),					\
					    (LOG_OBJECT_PTR(UART_ASYNC_TO_IRQ_LOG_NAME)),	\
					    (_log)						\
					   )							\
				   )								\
	}

/** @brief Initialize the adaptation layer.
 *
 * @param dev UART device. Device must support asynchronous API.
 *
 * @retval 0 On successful initialization.
 */
int uart_async_to_irq_init(const struct device *dev);

/* @brief Enable RX for interrupt driven API.
 *
 * @param dev UART device. Device must support asynchronous API.
 *
 * @retval 0 on successful operation.
 * @retval -EINVAL if adaption layer has wrong configuration.
 * @retval negative value Error reported by the UART API.
 */
int uart_async_to_irq_rx_enable(const struct device *dev);

/* @brief Disable RX for interrupt driven API.
 *
 * @param dev UART device. Device must support asynchronous API.
 *
 * @retval 0 on successful operation.
 * @retval -EINVAL if adaption layer has wrong configuration.
 * @retval negative value Error reported by the UART API.
 */
int uart_async_to_irq_rx_disable(const struct device *dev);

/* Starting from here API is internal only. */

/** @cond INTERNAL_HIDDEN
 * @brief Structure used by the adaptation layer.
 */
struct uart_async_to_irq_config {
	/** Pointer to the TX buffer. */
	uint8_t *tx_buf;

	/** TX buffer length. */
	size_t tx_len;

	/** UART Asynchronous RX helper configuration. */
	struct uart_async_rx_config async_rx;

	/** Async API used by the a2i layer. */
	const struct uart_async_to_irq_async_api *api;

	/** Trampoline callback. */
	uart_async_to_irq_trampoline trampoline;

	/** Initial baudrate. */
	uint32_t baudrate;

	/** Instance logging handler. */
	LOG_INSTANCE_PTR_DECLARE(log);
};

/** @brief Asynchronous API used by the adaptation layer. */
struct uart_async_to_irq_async_api {
	int (*callback_set)(const struct device *dev,
			    uart_callback_t callback,
			    void *user_data);

	int (*tx)(const struct device *dev, const uint8_t *buf, size_t len,
		  int32_t timeout);
	int (*tx_abort)(const struct device *dev);

	int (*rx_enable)(const struct device *dev, uint8_t *buf, size_t len,
			 int32_t timeout);
	int (*rx_buf_rsp)(const struct device *dev, uint8_t *buf, size_t len);
	int (*rx_disable)(const struct device *dev);
};

/** @brief Structure holding receiver data. */
struct uart_async_to_irq_rx_data {
	/** Asynchronous RX helper data. */
	struct uart_async_rx async_rx;

	/** Semaphore for pending on RX disable. */
	struct k_sem sem;

	/** Number of pending buffer requests which weren't handled because lack of free buffers. */
	atomic_t pending_buf_req;
};

/** @brief Structure holding transmitter data. */
struct uart_async_to_irq_tx_data {
	/** TX buffer. */
	uint8_t *buf;

	/** Length of the buffer. */
	size_t len;
};

/** @brief Data associated with the asynchronous to the interrupt driven API adaptation layer. */
struct uart_async_to_irq_data {
	/** User callback for interrupt driven API. */
	uart_irq_callback_user_data_t callback;

	/** User data. */
	void *user_data;

	/** Interrupt request counter. */
	atomic_t irq_req;

	/** RX specific data. */
	struct uart_async_to_irq_rx_data rx;

	/** TX specific data. */
	struct uart_async_to_irq_tx_data tx;

	/** Spinlock. */
	struct k_spinlock lock;

	/** Internally used flags for holding the state of the a2i layer. */
	atomic_t flags;
};

/** Interrupt driven FIFO fill function. */
int z_uart_async_to_irq_fifo_fill(const struct device *dev,
				  const uint8_t *buf,
				  int len);

/** Interrupt driven FIFO read function. */
int z_uart_async_to_irq_fifo_read(const struct device *dev,
				  uint8_t *buf,
				  const int len);

/** Interrupt driven transfer enabling function. */
void z_uart_async_to_irq_irq_tx_enable(const struct device *dev);

/** Interrupt driven transfer disabling function */
void z_uart_async_to_irq_irq_tx_disable(const struct device *dev);

/** Interrupt driven transfer ready function */
int z_uart_async_to_irq_irq_tx_ready(const struct device *dev);

/** Interrupt driven receiver enabling function */
void z_uart_async_to_irq_irq_rx_enable(const struct device *dev);

/** Interrupt driven receiver disabling function */
void z_uart_async_to_irq_irq_rx_disable(const struct device *dev);

/** Interrupt driven transfer complete function */
int z_uart_async_to_irq_irq_tx_complete(const struct device *dev);

/** Interrupt driven receiver ready function */
int z_uart_async_to_irq_irq_rx_ready(const struct device *dev);

/** Interrupt driven error enabling function */
void z_uart_async_to_irq_irq_err_enable(const struct device *dev);

/** Interrupt driven error disabling function */
void z_uart_async_to_irq_irq_err_disable(const struct device *dev);

/** Interrupt driven pending status function */
int z_uart_async_to_irq_irq_is_pending(const struct device *dev);

/** Interrupt driven interrupt update function */
int z_uart_async_to_irq_irq_update(const struct device *dev);

/** Set the irq callback function */
void z_uart_async_to_irq_irq_callback_set(const struct device *dev,
			 uart_irq_callback_user_data_t cb,
			 void *user_data);

/** @endcond */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_ASYNC_TO_IRQ_H_ */
