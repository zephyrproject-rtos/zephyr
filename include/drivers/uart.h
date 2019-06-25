/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for UART drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_H_

/**
 * @brief UART Interface
 * @defgroup uart_interface UART Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Line control signals. */
enum uart_line_ctrl {
	UART_LINE_CTRL_RTS = BIT(1),
	UART_LINE_CTRL_DTR = BIT(2),
	UART_LINE_CTRL_DCD = BIT(3),
	UART_LINE_CTRL_DSR = BIT(4),
};

/**
 * @brief Types of events passed to callback in UART_ASYNC_API
 *
 * Receiving:
 * 1. To start receiving, uart_rx_enable has to be called with first buffer
 * 2. When receiving starts to current buffer, UART_RX_BUF_REQUEST will be
 *    generated, in response to that user can either:
 *
 *    - Provide second buffer using uart_rx_buf_rsp, when first buffer is
 *      filled, receiving will automatically start to second buffer.
 *    - Ignore the event, this way when current buffer is filled UART_RX_DONE
 *      event will be generated and receiving will be stopped.
 *
 * 3. If some data was received and timeout occurred UART_RX_RDY event will be
 *    generated. It can happen multiples times for the same buffer. RX timeout
 *    is counted from last byte received i.e. if no data was received, there
 *    won't be any timeout event.
 * 4. After buffer is filled UART_RX_RDY will be generated, immediately
 *    followed by UART_RX_BUF_RELEASED indicating that current buffer is no
 *    longer used.
 * 5. If there was second buffer provided, it will become current buffer and
 *    we start again at point 2.
 *    If no second buffer was specified receiving is stopped and
 *    UART_RX_DISABLED event is generated. After that whole process can be
 *    repeated.
 *
 * Any time during reception UART_RX_STOPPED event can occur. It will be
 * followed by UART_RX_BUF_RELEASED event for every buffer currently passed to
 * driver and finally by UART_RX_DISABLED event.
 *
 * Receiving can be disabled using uart_rx_disable, after calling that
 * function any data received will be lost, UART_RX_BUF_RELEASED event will be
 * generated for every buffer currently passed to driver and UART_RX_DISABLED
 * event will occur.
 *
 * Transmitting:
 * 1. Transmitting starts by uart_tx function.
 * 2. If whole buffer was transmitted UART_TX_DONE is generated.
 *    If timeout occurred UART_TX_ABORTED will be generated.
 *
 * Transmitting can be aborted using uart_tx_abort, after calling that
 * function UART_TX_ABORTED event will be generated.
 *
 */
enum uart_event_type {
	/** @brief Whole TX buffer was transmitted. */
	UART_TX_DONE,
	/**
	 * @brief Transmitting aborted due to timeout or uart_tx_abort call
	 *
	 * When flow control is enabled, there is a possibility that TX transfer
	 * won't finish in the allotted time. Some data may have been
	 * transferred, information about it can be found in event data.
	 */
	UART_TX_ABORTED,
	/**
	 * @brief Received data is ready for processing.
	 *
	 * This event is generated in two cases:
	 * - When RX timeout occurred, and data was stored in provided buffer.
	 *   This can happen multiple times in the same buffer.
	 * - When provided buffer is full.
	 */
	UART_RX_RDY,
	/**
	 * @brief Driver requests next buffer for continuous reception.
	 *
	 * This event is triggered when receiving has started for a new buffer,
	 * i.e. it's time to provide a next buffer for a seamless switchover to
	 * it. For continuous reliable receiving, user should provide another RX
	 * buffer in response to this event, using uart_rx_buf_rsp function
	 *
	 * If uart_rx_buf_rsp is not called before current buffer
	 * is filled up, receiving will stop.
	 */
	UART_RX_BUF_REQUEST,
	/**
	 * @brief Buffer is no longer used by UART driver.
	 */
	UART_RX_BUF_RELEASED,
	/**
	 * @brief RX has been disabled and can be reenabled.
	 *
	 * This event is generated whenever receiver has been stopped, disabled
	 * or finished its operation and can be enabled again using
	 * uart_rx_enable
	 */
	UART_RX_DISABLED,
	/**
	 * @brief RX has stopped due to external event.
	 *
	 * Reason is one of uart_rx_stop_reason.
	 */
	UART_RX_STOPPED,
};


/**
 * @brief Reception stop reasons.
 *
 * Values that correspond to events or errors responsible for stopping
 * receiving.
 */
enum uart_rx_stop_reason {
	/** @brief Overrun error */
	UART_ERROR_OVERRUN = (1 << 0),
	/** @brief Parity error */
	UART_ERROR_PARITY  = (1 << 1),
	/** @brief Framing error */
	UART_ERROR_FRAMING = (1 << 2),
	/**
	 * @brief Break interrupt
	 *
	 * A break interrupt was received. This happens when the serial input
	 * is held at a logic '0' state for longer than the sum of
	 * start time + data bits + parity + stop bits.
	 */
	UART_BREAK = (1 << 3),
};

/** @brief Backward compatibility defines, deprecated */
#define UART_ERROR_BREAK UART_BREAK
#define LINE_CTRL_BAUD_RATE (1 << 0)
#define LINE_CTRL_RTS UART_LINE_CTRL_RTS
#define LINE_CTRL_DTR UART_LINE_CTRL_DTR
#define LINE_CTRL_DCD UART_LINE_CTRL_DCD
#define LINE_CTRL_DSR UART_LINE_CTRL_DSR


/** @brief UART TX event data. */
struct uart_event_tx {
	/** @brief Pointer to current buffer. */
	const u8_t *buf;
	/** @brief Number of bytes sent. */
	size_t len;
};

/**
 * @brief UART RX event data.
 *
 * The data represented by the event is stored in rx.buf[rx.offset] to
 * rx.buf[rx.offset+rx.len].  That is, the length is relative to the offset.
 */
struct uart_event_rx {
	/** @brief Pointer to current buffer. */
	u8_t *buf;
	/** @brief Currently received data offset in bytes. */
	size_t offset;
	/** @brief Number of new bytes received. */
	size_t len;
};

/** @brief UART RX buffer released event data. */
struct uart_event_rx_buf {
	/* @brief Pointer to buffer that is no longer in use. */
	u8_t *buf;
};

/** @brief UART RX stopped data. */
struct uart_event_rx_stop {
	/** @brief Reason why receiving stopped */
	enum uart_rx_stop_reason reason;
	/** @brief Last received data. */
	struct uart_event_rx data;
};

/** @brief Structure containing information about current event. */
struct uart_event {
	/** @brief Type of event */
	enum uart_event_type type;
	/** @brief Event data */
	union {
		/** @brief UART_TX_DONE and UART_TX_ABORTED events data. */
		struct uart_event_tx tx;
		/** @brief UART_RX_RDY event data. */
		struct uart_event_rx rx;
		/** @brief UART_RX_BUF_RELEASED event data. */
		struct uart_event_rx_buf rx_buf;
		/** @brief UART_RX_STOPPED event data. */
		struct uart_event_rx_stop rx_stop;
	} data;
};

/**
 * @typedef uart_callback_t
 * @brief Define the application callback function signature for
 * uart_callback_set() function.
 *
 * @param evt	    Pointer to uart_event structure.
 * @param user_data Pointer to data specified by user.
 */
typedef void (*uart_callback_t)(struct uart_event *evt, void *user_data);

/**
 * @brief Options for @a UART initialization.
 */
#define UART_OPTION_AFCE 0x01

/**
 * @brief UART controller configuration structure
 *
 * @param baudrate  Baudrate setting in bps
 * @param parity    Parity bit, use @ref uart_config_parity
 * @param stop_bits Stop bits, use @ref uart_config_stop_bits
 * @param data_bits Data bits, use @ref uart_config_data_bits
 * @param flow_ctrl Flow control setting, use @ref uart_config_flow_control
 */
struct uart_config {
	u32_t baudrate;
	u8_t parity;
	u8_t stop_bits;
	u8_t data_bits;
	u8_t flow_ctrl;
};

/** @brief Parity modes */
enum uart_config_parity {
	UART_CFG_PARITY_NONE,
	UART_CFG_PARITY_ODD,
	UART_CFG_PARITY_EVEN,
	UART_CFG_PARITY_MARK,
	UART_CFG_PARITY_SPACE,
};

/** @brief Number of stop bits. */
enum uart_config_stop_bits {
	UART_CFG_STOP_BITS_0_5,
	UART_CFG_STOP_BITS_1,
	UART_CFG_STOP_BITS_1_5,
	UART_CFG_STOP_BITS_2,
};

/** @brief Number of data bits. */
enum uart_config_data_bits {
	UART_CFG_DATA_BITS_5,
	UART_CFG_DATA_BITS_6,
	UART_CFG_DATA_BITS_7,
	UART_CFG_DATA_BITS_8,
	UART_CFG_DATA_BITS_9,
};

/**
 * @brief Hardware flow control options.
 *
 * With flow control set to none, any operations related to flow control
 * signals can be managed by user with uart_line_ctrl functions.
 * In other cases, flow control is managed by hardware/driver.
 */
enum uart_config_flow_control {
	UART_CFG_FLOW_CTRL_NONE,
	UART_CFG_FLOW_CTRL_RTS_CTS,
	UART_CFG_FLOW_CTRL_DTR_DSR,
};

/**
 * @typedef uart_irq_callback_user_data_t
 * @brief Define the application callback function signature for
 * uart_irq_callback_user_data_set() function.
 *
 * @param user_data Arbitrary user data.
 */
typedef void (*uart_irq_callback_user_data_t)(void *user_data);

/**
 * @typedef uart_irq_callback_t
 * @brief Define the application callback function signature for legacy
 * uart_irq_callback_set().
 *
 * @param port Device struct for the UART device.
 */
typedef void (*uart_irq_callback_t)(struct device *port);

/**
 * @typedef uart_irq_config_func_t
 * @brief For configuring IRQ on each individual UART device.
 *
 * @internal
 */
typedef void (*uart_irq_config_func_t)(struct device *port);

/**
 * @brief UART device configuration.
 *
 * @param port Base port number
 * @param base Memory mapped base address
 * @param regs Register address
 * @param sys_clk_freq System clock frequency in Hz
 */
struct uart_device_config {
	union {
		u32_t port;
		u8_t *base;
		u32_t regs;
	};

	u32_t sys_clk_freq;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t	irq_config_func;
#endif
};

/** @brief Driver API structure. */
struct uart_driver_api {

#ifdef CONFIG_UART_ASYNC_API

	int (*callback_set)(struct device *dev, uart_callback_t callback,
			    void *user_data);

	int (*tx)(struct device *dev, const u8_t *buf, size_t len,
		  u32_t timeout);
	int (*tx_abort)(struct device *dev);

	int (*rx_enable)(struct device *dev, u8_t *buf, size_t len,
			 u32_t timeout);
	int (*rx_buf_rsp)(struct device *dev, u8_t *buf, size_t len);
	int (*rx_disable)(struct device *dev);

#endif

	/** Console I/O function */
	int (*poll_in)(struct device *dev, unsigned char *p_char);
	void (*poll_out)(struct device *dev, unsigned char out_char);

	/** Console I/O function */
	int (*err_check)(struct device *dev);

	/** UART configuration functions */
	int (*configure)(struct device *dev, const struct uart_config *cfg);
	int (*config_get)(struct device *dev, struct uart_config *cfg);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	/** Interrupt driven FIFO fill function */
	int (*fifo_fill)(struct device *dev, const u8_t *tx_data, int len);

	/** Interrupt driven FIFO read function */
	int (*fifo_read)(struct device *dev, u8_t *rx_data, const int size);

	/** Interrupt driven transfer enabling function */
	void (*irq_tx_enable)(struct device *dev);

	/** Interrupt driven transfer disabling function */
	void (*irq_tx_disable)(struct device *dev);

	/** Interrupt driven transfer ready function */
	int (*irq_tx_ready)(struct device *dev);

	/** Interrupt driven receiver enabling function */
	void (*irq_rx_enable)(struct device *dev);

	/** Interrupt driven receiver disabling function */
	void (*irq_rx_disable)(struct device *dev);

	/** Interrupt driven transfer complete function */
	int (*irq_tx_complete)(struct device *dev);

	/** Interrupt driven receiver ready function */
	int (*irq_rx_ready)(struct device *dev);

	/** Interrupt driven error enabling function */
	void (*irq_err_enable)(struct device *dev);

	/** Interrupt driven error disabling function */
	void (*irq_err_disable)(struct device *dev);

	/** Interrupt driven pending status function */
	int (*irq_is_pending)(struct device *dev);

	/** Interrupt driven interrupt update function */
	int (*irq_update)(struct device *dev);

	/** Set the irq callback function */
	void (*irq_callback_set)(struct device *dev,
				 uart_irq_callback_user_data_t cb,
				 void *user_data);

#endif

#ifdef CONFIG_UART_LINE_CTRL
	int (*line_ctrl_set)(struct device *dev, u32_t ctrl, u32_t val);
	int (*line_ctrl_get)(struct device *dev, u32_t ctrl, u32_t *val);
#endif

#ifdef CONFIG_UART_DRV_CMD
	int (*drv_cmd)(struct device *dev, u32_t cmd, u32_t p);
#endif

};

#ifdef CONFIG_UART_ASYNC_API

/**
 * @brief Set event handler function.
 *
 * @param dev       UART device structure.
 * @param callback  Event handler.
 * @param user_data Data to pass to event handler function.
 *
 * @retval 0	    If successful, negative errno code otherwise.
 */
static inline int uart_callback_set(struct device *dev,
				    uart_callback_t callback,
				    void *user_data)
{
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->driver_api;

	return api->callback_set(dev, callback, user_data);
}

/**
 * @brief Send given number of bytes from buffer through UART.
 *
 * Function returns immediately and event handler,
 * set using @ref uart_callback_set, is called after transfer is finished.
 *
 * @param dev     UART device structure.
 * @param buf     Pointer to transmit buffer.
 * @param len     Length of transmit buffer.
 * @param timeout Timeout in milliseconds. Valid only if flow control is enabled
 *
 * @retval -EBUSY There is already an ongoing transfer.
 * @retval 0	  If successful, negative errno code otherwise.
 */
static inline int uart_tx(struct device *dev,
			  const u8_t *buf,
			  size_t len,
			  u32_t timeout)

{
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->driver_api;

	return api->tx(dev, buf, len, timeout);
}

/**
 * @brief Abort current TX transmission.
 *
 * UART_TX_DONE event will be generated with amount of data sent.
 *
 * @param dev UART device structure.
 *
 * @retval -EFAULT There is no active transmission.
 * @retval 0	   If successful, negative errno code otherwise.
 */
static inline int uart_tx_abort(struct device *dev)
{
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->driver_api;

	return api->tx_abort(dev);
}

/**
 * @brief Start receiving data through UART.
 *
 * Function sets given buffer as first buffer for receiving and returns
 * immediately. After that event handler, set using @ref uart_callback_set,
 * is called with UART_RX_RDY or UART_RX_BUF_REQUEST events.
 *
 * @param dev     UART device structure.
 * @param buf     Pointer to receive buffer.
 * @param len     Buffer length.
 * @param timeout Timeout in milliseconds.
 *
 * @retval -EBUSY RX already in progress.
 * @retval 0	  If successful, negative errno code otherwise.
 *
 */
static inline int uart_rx_enable(struct device *dev, u8_t *buf, size_t len,
				 u32_t timeout)
{
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->driver_api;

	return api->rx_enable(dev, buf, len, timeout);
}

/**
 * @brief Provide receive buffer in response to UART_RX_BUF_REQUEST event.
 *
 * Provide pointer to RX buffer, which will be used when current buffer is
 * filled.
 *
 * @note Providing buffer that is already in usage by driver leads to
 *       undefined behavior. Buffer can be reused when it has been released
 *       by driver.
 *
 * @param dev UART device structure.
 * @param buf Pointer to receive buffer.
 * @param len Buffer length.
 *
 * @retval -EBUSY Next buffer already set.
 * @retval 0	  If successful, negative errno code otherwise.
 *
 */
static inline int uart_rx_buf_rsp(struct device *dev, u8_t *buf, size_t len)
{
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->driver_api;

	return api->rx_buf_rsp(dev, buf, len);
}

/**
 * @brief Disable RX
 *
 * UART_RX_BUF_RELEASED event will be generated for every buffer scheduled,
 * after that UART_RX_DISABLED event will be generated.
 *
 * @param dev UART device structure.
 *
 * @retval -EFAULT There is no active reception.
 * @retval 0	   If successful, negative errno code otherwise.
 */
static inline int uart_rx_disable(struct device *dev)
{
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->driver_api;

	return api->rx_disable(dev);
}

#endif

/**
 * @brief Check whether an error was detected.
 *
 * @param dev UART device structure.
 *
 * @retval uart_rx_stop_reason If error during receiving occurred.
 * @retval 0		       Otherwise.
 */
__syscall int uart_err_check(struct device *dev);

static inline int z_impl_uart_err_check(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->err_check != NULL) {
		return api->err_check(dev);
	}
	return 0;
}


/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived.
 * @retval -1 If no character was available to read (i.e., the UART
 *            input buffer was empty).
 * @retval -ENOTSUP If the operation is not supported.
 * @retval -EBUSY If reception was enabled using uart_rx_enabled
 */
__syscall int uart_poll_in(struct device *dev, unsigned char *p_char);

static inline int z_impl_uart_poll_in(struct device *dev, unsigned char *p_char)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	return api->poll_in(dev, p_char);
}

/**
 * @brief Output a character in polled mode.
 *
 * This routine checks if the transmitter is empty.
 * When the transmitter is empty, it writes a character to the data
 * register.
 *
 * To send a character when hardware flow control is enabled, the handshake
 * signal CTS must be asserted.
 *
 * @param dev UART device structure.
 * @param out_char Character to send.
 */
__syscall void uart_poll_out(struct device *dev,
				      unsigned char out_char);

static inline void z_impl_uart_poll_out(struct device *dev,
						unsigned char out_char)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	api->poll_out(dev, out_char);
}

/**
 * @brief Set UART configuration.
 *
 * Sets UART configuration using data from *cfg.
 *
 * @param dev UART device structure.
 * @param cfg UART configuration structure.
 *
 *
 * @retval -ENOTSUP If configuration is not supported by device.
 *                  or driver does not support setting configuration in runtime.
 * @retval 0 If successful, negative errno code otherwise.
 */
__syscall int uart_configure(struct device *dev, const struct uart_config *cfg);

static inline int z_impl_uart_configure(struct device *dev,
				       const struct uart_config *cfg)
{
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->driver_api;

	if (api->configure != NULL) {
		return api->configure(dev, cfg);
	}

	return -ENOTSUP;
}

/**
 * @brief Get UART configuration.
 *
 * Stores current UART configuration to *cfg, can be used to retrieve initial
 * configuration after device was initialized using data from DTS.
 *
 * @param dev UART device structure.
 * @param cfg UART configuration structure.
 *
 * @retval -ENOTSUP If driver does not support getting current configuration.
 * @retval 0 If successful, negative errno code otherwise.
 */
__syscall int uart_config_get(struct device *dev, struct uart_config *cfg);

static inline int z_impl_uart_config_get(struct device *dev,
				     struct uart_config *cfg)
{
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->driver_api;

	if (api->config_get != NULL) {
		return api->config_get(dev, cfg);
	}

	return -ENOTSUP;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_tx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). Likewise, *not* calling this function
 * from an ISR if uart_irq_tx_ready() returns true may lead to
 * undefined behavior, e.g. infinite interrupt loops. It's
 * mandatory to test return value of this function, as different
 * hardware has different FIFO depth (oftentimes just 1).
 *
 * @param dev UART device structure.
 * @param tx_data Data to transmit.
 * @param size Number of bytes to send.
 *
 * @return Number of bytes sent.
 */
static inline int uart_fifo_fill(struct device *dev, const u8_t *tx_data,
				 int size)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->fifo_fill) {
		return api->fifo_fill(dev, tx_data, size);
	}

	return 0;
}

/**
 * @brief Read data from FIFO.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_rx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). It's unspecified whether "RX ready"
 * condition as returned by uart_irq_rx_ready() is level- or
 * edge- triggered. That means that once uart_irq_rx_ready() is
 * detected, uart_fifo_read() must be called until it reads all
 * available data in the FIFO (i.e. until it returns less data
 * than was requested).
 *
 * Note that the calling context only applies to physical UARTs and
 * no to the virtual ones found in USB CDC ACM code.
 *
 * @param dev UART device structure.
 * @param rx_data Data container.
 * @param size Container size.
 *
 * @return Number of bytes read.
 */
static inline int uart_fifo_read(struct device *dev, u8_t *rx_data,
				 const int size)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->fifo_read) {
		return api->fifo_read(dev, rx_data, size);
	}

	return 0;
}

/**
 * @brief Enable TX interrupt in IER.
 *
 * @param dev UART device structure.
 *
 * @return N/A
 */
__syscall void uart_irq_tx_enable(struct device *dev);

static inline void z_impl_uart_irq_tx_enable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_tx_enable) {
		api->irq_tx_enable(dev);
	}
}
/**
 * @brief Disable TX interrupt in IER.
 *
 * @param dev UART device structure.
 *
 * @return N/A
 */
__syscall void uart_irq_tx_disable(struct device *dev);

static inline void z_impl_uart_irq_tx_disable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_tx_disable) {
		api->irq_tx_disable(dev);
	}
}

/**
 * @brief Check if UART TX buffer can accept a new char
 *
 * @details Check if UART TX buffer can accept at least one character
 * for transmission (i.e. uart_fifo_fill() will succeed and return
 * non-zero). This function must be called in a UART interrupt
 * handler, or its result is undefined. Before calling this function
 * in the interrupt handler, uart_irq_update() must be called once per
 * the handler invocation.
 *
 * @param dev UART device structure.
 *
 * @retval 1 If at least one char can be written to UART.
 * @retval 0 Otherwise.
 */
static inline int uart_irq_tx_ready(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_tx_ready) {
		return api->irq_tx_ready(dev);
	}

	return 0;
}

/**
 * @brief Enable RX interrupt.
 *
 * @param dev UART device structure.
 *
 * @return N/A
 */
__syscall void uart_irq_rx_enable(struct device *dev);

static inline void z_impl_uart_irq_rx_enable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_rx_enable) {
		api->irq_rx_enable(dev);
	}
}

/**
 * @brief Disable RX interrupt.
 *
 * @param dev UART device structure.
 *
 * @return N/A
 */
__syscall void uart_irq_rx_disable(struct device *dev);

static inline void z_impl_uart_irq_rx_disable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_rx_disable) {
		api->irq_rx_disable(dev);
	}
}

/**
 * @brief Check if UART TX block finished transmission
 *
 * @details Check if any outgoing data buffered in UART TX block was
 * fully transmitted and TX block is idle. When this condition is
 * true, UART device (or whole system) can be power off. Note that
 * this function is *not* useful to check if UART TX can accept more
 * data, use uart_irq_tx_ready() for that. This function must be called
 * in a UART interrupt handler, or its result is undefined. Before
 * calling this function in the interrupt handler, uart_irq_update()
 * must be called once per the handler invocation.
 *
 * @param dev UART device structure.
 *
 * @retval 1 If nothing remains to be transmitted.
 * @retval 0 Otherwise.
 * @retval -ENOTSUP if this function is not supported
 */
static inline int uart_irq_tx_complete(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_tx_complete) {
		return api->irq_tx_complete(dev);
	}

	return -ENOTSUP;
}

/**
 * @brief Check if UART RX buffer has a received char
 *
 * @details Check if UART RX buffer has at least one pending character
 * (i.e. uart_fifo_read() will succeed and return non-zero). This function
 * must be called in a UART interrupt handler, or its result is undefined.
 * Before calling this function in the interrupt handler, uart_irq_update()
 * must be called once per the handler invocation. It's unspecified whether
 * condition as returned by this function is level- or edge- triggered (i.e.
 * if this function returns true when RX FIFO is non-empty, or when a new
 * char was received since last call to it). See description of
 * uart_fifo_read() for implication of this.
 *
 * @param dev UART device structure.
 *
 * @retval 1 If a received char is ready.
 * @retval 0 Otherwise.
 * @retval -ENOTSUP if this function is not supported
 */
static inline int uart_irq_rx_ready(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_rx_ready) {
		return api->irq_rx_ready(dev);
	}

	return 0;
}
/**
 * @brief Enable error interrupt.
 *
 * @param dev UART device structure.
 *
 * @return N/A
 */
__syscall void uart_irq_err_enable(struct device *dev);

static inline void z_impl_uart_irq_err_enable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_err_enable) {
		api->irq_err_enable(dev);
	}
}

/**
 * @brief Disable error interrupt.
 *
 * @param dev UART device structure.
 *
 * @retval 1 If an IRQ is ready.
 * @retval 0 Otherwise.
 */
__syscall void uart_irq_err_disable(struct device *dev);

static inline void z_impl_uart_irq_err_disable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_err_disable) {
		api->irq_err_disable(dev);
	}
}

/**
 * @brief Check if any IRQs is pending.
 *
 * @param dev UART device structure.
 *
 * @retval 1 If an IRQ is pending.
 * @retval 0 Otherwise.
 */
__syscall int uart_irq_is_pending(struct device *dev);

static inline int z_impl_uart_irq_is_pending(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_is_pending)	{
		return api->irq_is_pending(dev);
	}

	return 0;
}

/**
 * @brief Start processing interrupts in ISR.
 *
 * This function should be called the first thing in the ISR. Calling
 * uart_irq_rx_ready(), uart_irq_tx_ready(), uart_irq_tx_complete()
 * allowed only after this.
 *
 * The purpose of this function is:
 *
 * * For devices with auto-acknowledge of interrupt status on register
 *   read to cache the value of this register (rx_ready, etc. then use
 *   this case).
 * * For devices with explicit acknowledgement of interrupts, to ack
 *   any pending interrupts and likewise to cache the original value.
 * * For devices with implicit acknowledgement, this function will be
 *   empty. But the ISR must perform the actions needs to ack the
 *   interrupts (usually, call uart_fifo_read() on rx_ready, and
 *   uart_fifo_fill() on tx_ready).
 *
 * @param dev UART device structure.
 *
 * @retval 1 Always.
 */
__syscall int uart_irq_update(struct device *dev);

static inline int z_impl_uart_irq_update(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_update) {
		return api->irq_update(dev);
	}

	return 0;
}

/**
 * @brief Set the IRQ callback function pointer.
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with specified user data.
 * See description of uart_irq_update() for the requirements on ISR.
 *
 * @param dev UART device structure.
 * @param cb Pointer to the callback function.
 * @param user_data Data to pass to callback function.
 *
 * @return N/A
 */
static inline void uart_irq_callback_user_data_set(
					struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *user_data)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if ((api != NULL) && (api->irq_callback_set != NULL)) {
		api->irq_callback_set(dev, cb, user_data);
	}
}

/**
 * @brief Set the IRQ callback function pointer (legacy).
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with the device pointer.
 *
 * @param dev UART device structure.
 * @param cb Pointer to the callback function.
 *
 * @return N/A
 */
static inline void uart_irq_callback_set(struct device *dev,
					 uart_irq_callback_t cb)
{
	uart_irq_callback_user_data_set(dev, (uart_irq_callback_user_data_t)cb,
					dev);
}

#endif

#ifdef CONFIG_UART_LINE_CTRL

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev UART device structure.
 * @param ctrl The line control to manipulate.
 * @param val Value to set to the line control.
 *
 * @retval 0 If successful.
 * @retval failed Otherwise.
 */
__syscall int uart_line_ctrl_set(struct device *dev,
				 u32_t ctrl, u32_t val);

static inline int z_impl_uart_line_ctrl_set(struct device *dev,
					   u32_t ctrl, u32_t val)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->line_ctrl_set) {
		return api->line_ctrl_set(dev, ctrl, val);
	}

	return -ENOTSUP;
}

/**
 * @brief Retrieve line control for UART.
 *
 * @param dev UART device structure.
 * @param ctrl The line control to manipulate.
 * @param val Value to get for the line control.
 *
 * @retval 0 If successful.
 * @retval failed Otherwise.
 */
__syscall int uart_line_ctrl_get(struct device *dev, u32_t ctrl, u32_t *val);

static inline int z_impl_uart_line_ctrl_get(struct device *dev,
					   u32_t ctrl, u32_t *val)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api && api->line_ctrl_get) {
		return api->line_ctrl_get(dev, ctrl, val);
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD

/**
 * @brief Send extra command to driver.
 *
 * Implementation and accepted commands are driver specific.
 * Refer to the drivers for more information.
 *
 * @param dev UART device structure.
 * @param cmd Command to driver.
 * @param p Parameter to the command.
 *
 * @retval 0 If successful.
 * @retval failed Otherwise.
 */
__syscall int uart_drv_cmd(struct device *dev, u32_t cmd, u32_t p);

static inline int z_impl_uart_drv_cmd(struct device *dev, u32_t cmd, u32_t p)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->drv_cmd) {
		return api->drv_cmd(dev, cmd, p);
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_DRV_CMD */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/uart.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_H_ */
