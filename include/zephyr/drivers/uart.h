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
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Line control signals. */
enum uart_line_ctrl {
	UART_LINE_CTRL_BAUD_RATE = BIT(0), /**< Baud rate */
	UART_LINE_CTRL_RTS = BIT(1),       /**< Request To Send (RTS) */
	UART_LINE_CTRL_DTR = BIT(2),       /**< Data Terminal Ready (DTR) */
	UART_LINE_CTRL_DCD = BIT(3),       /**< Data Carrier Detect (DCD) */
	UART_LINE_CTRL_DSR = BIT(4),       /**< Data Set Ready (DSR) */
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
	/**
	 * @brief Collision error
	 *
	 * This error is raised when transmitted data does not match
	 * received data. Typically this is useful in scenarios where
	 * the TX and RX lines maybe connected together such as
	 * RS-485 half-duplex. This error is only valid on UARTs that
	 * support collision checking.
	 */
	UART_ERROR_COLLISION = (1 << 4),
	/** @brief Noise error */
	UART_ERROR_NOISE = (1 << 5),
};

/** @brief Parity modes */
enum uart_config_parity {
	UART_CFG_PARITY_NONE,   /**< No parity */
	UART_CFG_PARITY_ODD,    /**< Odd parity */
	UART_CFG_PARITY_EVEN,   /**< Even parity */
	UART_CFG_PARITY_MARK,   /**< Mark parity */
	UART_CFG_PARITY_SPACE,  /**< Space parity */
};

/** @brief Number of stop bits. */
enum uart_config_stop_bits {
	UART_CFG_STOP_BITS_0_5,  /**< 0.5 stop bit */
	UART_CFG_STOP_BITS_1,    /**< 1 stop bit */
	UART_CFG_STOP_BITS_1_5,  /**< 1.5 stop bits */
	UART_CFG_STOP_BITS_2,    /**< 2 stop bits */
};

/** @brief Number of data bits. */
enum uart_config_data_bits {
	UART_CFG_DATA_BITS_5,    /**< 5 data bits */
	UART_CFG_DATA_BITS_6,    /**< 6 data bits */
	UART_CFG_DATA_BITS_7,    /**< 7 data bits */
	UART_CFG_DATA_BITS_8,    /**< 8 data bits */
	UART_CFG_DATA_BITS_9,    /**< 9 data bits */
};

/**
 * @brief Hardware flow control options.
 *
 * With flow control set to none, any operations related to flow control
 * signals can be managed by user with uart_line_ctrl functions.
 * In other cases, flow control is managed by hardware/driver.
 */
enum uart_config_flow_control {
	UART_CFG_FLOW_CTRL_NONE,     /**< No flow control */
	UART_CFG_FLOW_CTRL_RTS_CTS,  /**< RTS/CTS flow control */
	UART_CFG_FLOW_CTRL_DTR_DSR,  /**< DTR/DSR flow control */
	UART_CFG_FLOW_CTRL_RS485,    /**< RS485 flow control */
};

/**
 * @brief UART controller configuration structure
 */
struct uart_config {
	uint32_t baudrate;  /**< Baudrate setting in bps */
	uint8_t parity;     /**< Parity bit, use @ref uart_config_parity */
	uint8_t stop_bits;  /**< Stop bits, use @ref uart_config_stop_bits */
	uint8_t data_bits;  /**< Data bits, use @ref uart_config_data_bits */
	uint8_t flow_ctrl;  /**< Flow control setting, use @ref uart_config_flow_control */
};

/**
 * @defgroup uart_interrupt Interrupt-driven UART API
 * @{
 */

/**
 * @brief Define the application callback function signature for
 * uart_irq_callback_user_data_set() function.
 *
 * @param dev UART device instance.
 * @param user_data Arbitrary user data.
 */
typedef void (*uart_irq_callback_user_data_t)(const struct device *dev,
					      void *user_data);

/**
 * @brief For configuring IRQ on each individual UART device.
 *
 * @param dev UART device instance.
 */
typedef void (*uart_irq_config_func_t)(const struct device *dev);

/**
 * @}
 *
 * @defgroup uart_async Async UART API
 * @since 1.14
 * @version 0.8.0
 * @{
 */

/**
 * @brief Types of events passed to callback in UART_ASYNC_API
 *
 * Receiving:
 * 1. To start receiving, uart_rx_enable has to be called with first buffer
 * 2. When receiving starts to current buffer,
 *    #UART_RX_BUF_REQUEST will be generated, in response to that user can
 *    either:
 *
 *    - Provide second buffer using uart_rx_buf_rsp, when first buffer is
 *      filled, receiving will automatically start to second buffer.
 *    - Ignore the event, this way when current buffer is filled
 *      #UART_RX_RDY event will be generated and receiving will be stopped.
 *
 * 3. If some data was received and timeout occurred #UART_RX_RDY event will be
 *    generated. It can happen multiples times for the same buffer. RX timeout
 *    is counted from last byte received i.e. if no data was received, there
 *    won't be any timeout event.
 * 4. #UART_RX_BUF_RELEASED event will be generated when the current buffer is
 *    no longer used by the driver. It will immediately follow #UART_RX_RDY event.
 *    Depending on the implementation buffer may be released when it is completely
 *    or partially filled.
 * 5. If there was second buffer provided, it will become current buffer and
 *    we start again at point 2.
 *    If no second buffer was specified receiving is stopped and
 *    #UART_RX_DISABLED event is generated. After that whole process can be
 *    repeated.
 *
 * Any time during reception #UART_RX_STOPPED event can occur. if there is any
 * data received, #UART_RX_RDY event will be generated. It will be followed by
 * #UART_RX_BUF_RELEASED event for every buffer currently passed to driver and
 * finally by #UART_RX_DISABLED event.
 *
 * Receiving can be disabled using uart_rx_disable, after calling that
 * function, if there is any data received, #UART_RX_RDY event will be
 * generated. #UART_RX_BUF_RELEASED event will be generated for every buffer
 * currently passed to driver and finally #UART_RX_DISABLED event will occur.
 *
 * Transmitting:
 * 1. Transmitting starts by uart_tx function.
 * 2. If whole buffer was transmitted #UART_TX_DONE is generated. If timeout
 *    occurred #UART_TX_ABORTED will be generated.
 *
 * Transmitting can be aborted using @ref uart_tx_abort, after calling that
 * function #UART_TX_ABORTED event will be generated.
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
	 * This event is generated in the following cases:
	 * - When RX timeout occurred, and data was stored in provided buffer.
	 *   This can happen multiple times in the same buffer.
	 * - When provided buffer is full.
	 * - After uart_rx_disable().
	 * - After stopping due to external event (#UART_RX_STOPPED).
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

/** @brief UART TX event data. */
struct uart_event_tx {
	/** @brief Pointer to current buffer. */
	const uint8_t *buf;
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
	uint8_t *buf;
	/** @brief Currently received data offset in bytes. */
	size_t offset;
	/** @brief Number of new bytes received. */
	size_t len;
};

/** @brief UART RX buffer released event data. */
struct uart_event_rx_buf {
	/** @brief Pointer to buffer that is no longer in use. */
	uint8_t *buf;
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
	union uart_event_data {
		/** @brief #UART_TX_DONE and #UART_TX_ABORTED events data. */
		struct uart_event_tx tx;
		/** @brief #UART_RX_RDY event data. */
		struct uart_event_rx rx;
		/** @brief #UART_RX_BUF_RELEASED event data. */
		struct uart_event_rx_buf rx_buf;
		/** @brief #UART_RX_STOPPED event data. */
		struct uart_event_rx_stop rx_stop;
	} data;
};

/**
 * @typedef uart_callback_t
 * @brief Define the application callback function signature for
 * uart_callback_set() function.
 *
 * @param dev UART device instance.
 * @param evt Pointer to uart_event instance.
 * @param user_data Pointer to data specified by user.
 */
typedef void (*uart_callback_t)(const struct device *dev,
				struct uart_event *evt, void *user_data);

/**
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/** @brief Driver API structure. */
__subsystem struct uart_driver_api {

#ifdef CONFIG_UART_ASYNC_API

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

#ifdef CONFIG_UART_WIDE_DATA
	int (*tx_u16)(const struct device *dev, const uint16_t *buf,
		      size_t len, int32_t timeout);
	int (*rx_enable_u16)(const struct device *dev, uint16_t *buf,
			     size_t len, int32_t timeout);
	int (*rx_buf_rsp_u16)(const struct device *dev, uint16_t *buf,
			      size_t len);
#endif

#endif

	/** Console I/O function */
	int (*poll_in)(const struct device *dev, unsigned char *p_char);
	void (*poll_out)(const struct device *dev, unsigned char out_char);

#ifdef CONFIG_UART_WIDE_DATA
	int (*poll_in_u16)(const struct device *dev, uint16_t *p_u16);
	void (*poll_out_u16)(const struct device *dev, uint16_t out_u16);
#endif

	/** Console I/O function */
	int (*err_check)(const struct device *dev);

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	/** UART configuration functions */
	int (*configure)(const struct device *dev,
			 const struct uart_config *cfg);
	int (*config_get)(const struct device *dev, struct uart_config *cfg);
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	/** Interrupt driven FIFO fill function */
	int (*fifo_fill)(const struct device *dev, const uint8_t *tx_data,
			 int len);

#ifdef CONFIG_UART_WIDE_DATA
	int (*fifo_fill_u16)(const struct device *dev, const uint16_t *tx_data,
			     int len);
#endif

	/** Interrupt driven FIFO read function */
	int (*fifo_read)(const struct device *dev, uint8_t *rx_data,
			 const int size);

#ifdef CONFIG_UART_WIDE_DATA
	int (*fifo_read_u16)(const struct device *dev, uint16_t *rx_data,
			     const int size);
#endif

	/** Interrupt driven transfer enabling function */
	void (*irq_tx_enable)(const struct device *dev);

	/** Interrupt driven transfer disabling function */
	void (*irq_tx_disable)(const struct device *dev);

	/** Interrupt driven transfer ready function */
	int (*irq_tx_ready)(const struct device *dev);

	/** Interrupt driven receiver enabling function */
	void (*irq_rx_enable)(const struct device *dev);

	/** Interrupt driven receiver disabling function */
	void (*irq_rx_disable)(const struct device *dev);

	/** Interrupt driven transfer complete function */
	int (*irq_tx_complete)(const struct device *dev);

	/** Interrupt driven receiver ready function */
	int (*irq_rx_ready)(const struct device *dev);

	/** Interrupt driven error enabling function */
	void (*irq_err_enable)(const struct device *dev);

	/** Interrupt driven error disabling function */
	void (*irq_err_disable)(const struct device *dev);

	/** Interrupt driven pending status function */
	int (*irq_is_pending)(const struct device *dev);

	/** Interrupt driven interrupt update function */
	int (*irq_update)(const struct device *dev);

	/** Set the irq callback function */
	void (*irq_callback_set)(const struct device *dev,
				 uart_irq_callback_user_data_t cb,
				 void *user_data);

#endif

#ifdef CONFIG_UART_LINE_CTRL
	int (*line_ctrl_set)(const struct device *dev, uint32_t ctrl,
			     uint32_t val);
	int (*line_ctrl_get)(const struct device *dev, uint32_t ctrl,
			     uint32_t *val);
#endif

#ifdef CONFIG_UART_DRV_CMD
	int (*drv_cmd)(const struct device *dev, uint32_t cmd, uint32_t p);
#endif

};

/** @endcond */

/**
 * @brief Check whether an error was detected.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If no error was detected.
 * @retval err Error flags as defined in @ref uart_rx_stop_reason
 * @retval -ENOSYS If not implemented.
 */
__syscall int uart_err_check(const struct device *dev);

static inline int z_impl_uart_err_check(const struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->err_check == NULL) {
		return -ENOSYS;
	}

	return api->err_check(dev);
}

/**
 * @defgroup uart_polling Polling UART API
 * @{
 */

/**
 * @brief Read a character from the device for input.
 *
 * This routine checks if the receiver has valid data.  When the
 * receiver has valid data, it reads a character from the device,
 * stores to the location pointed to by p_char, and returns 0 to the
 * calling thread. It returns -1, otherwise. This function is a
 * non-blocking call.
 *
 * @param dev UART device instance.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived.
 * @retval -1 If no character was available to read (i.e. the UART
 *            input buffer was empty).
 * @retval -ENOSYS If the operation is not implemented.
 * @retval -EBUSY If async reception was enabled using @ref uart_rx_enable
 */
__syscall int uart_poll_in(const struct device *dev, unsigned char *p_char);

static inline int z_impl_uart_poll_in(const struct device *dev,
				      unsigned char *p_char)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->poll_in == NULL) {
		return -ENOSYS;
	}

	return api->poll_in(dev, p_char);
}

/**
 * @brief Read a 16-bit datum from the device for input.
 *
 * This routine checks if the receiver has valid data.  When the
 * receiver has valid data, it reads a 16-bit datum from the device,
 * stores to the location pointed to by p_u16, and returns 0 to the
 * calling thread. It returns -1, otherwise. This function is a
 * non-blocking call.
 *
 * @param dev UART device instance.
 * @param p_u16 Pointer to 16-bit data.
 *
 * @retval 0  If data arrived.
 * @retval -1 If no data was available to read (i.e., the UART
 *            input buffer was empty).
 * @retval -ENOTSUP If API is not enabled.
 * @retval -ENOSYS If the function is not implemented.
 * @retval -EBUSY If async reception was enabled using @ref uart_rx_enable
 */
__syscall int uart_poll_in_u16(const struct device *dev, uint16_t *p_u16);

static inline int z_impl_uart_poll_in_u16(const struct device *dev,
					  uint16_t *p_u16)
{
#ifdef CONFIG_UART_WIDE_DATA
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->poll_in_u16 == NULL) {
		return -ENOSYS;
	}

	return api->poll_in_u16(dev, p_u16);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(p_u16);
	return -ENOTSUP;
#endif
}

/**
 * @brief Write a character to the device for output.
 *
 * This routine checks if the transmitter is full.  When the
 * transmitter is not full, it writes a character to the data
 * register. It waits and blocks the calling thread, otherwise. This
 * function is a blocking call.
 *
 * To send a character when hardware flow control is enabled, the handshake
 * signal CTS must be asserted.
 *
 * @param dev UART device instance.
 * @param out_char Character to send.
 */
__syscall void uart_poll_out(const struct device *dev,
			     unsigned char out_char);

static inline void z_impl_uart_poll_out(const struct device *dev,
					unsigned char out_char)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	api->poll_out(dev, out_char);
}

/**
 * @brief Write a 16-bit datum to the device for output.
 *
 * This routine checks if the transmitter is full. When the
 * transmitter is not full, it writes a 16-bit datum to the data
 * register. It waits and blocks the calling thread, otherwise. This
 * function is a blocking call.
 *
 * To send a datum when hardware flow control is enabled, the handshake
 * signal CTS must be asserted.
 *
 * @param dev UART device instance.
 * @param out_u16 Wide data to send.
 */
__syscall void uart_poll_out_u16(const struct device *dev, uint16_t out_u16);

static inline void z_impl_uart_poll_out_u16(const struct device *dev,
					    uint16_t out_u16)
{
#ifdef CONFIG_UART_WIDE_DATA
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	api->poll_out_u16(dev, out_u16);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(out_u16);
#endif
}

/**
 * @}
 */

/**
 * @brief Set UART configuration.
 *
 * Sets UART configuration using data from *cfg.
 *
 * @param dev UART device instance.
 * @param cfg UART configuration structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 * @retval -ENOSYS If configuration is not supported by device
 *                  or driver does not support setting configuration in runtime.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_configure(const struct device *dev,
			     const struct uart_config *cfg);

static inline int z_impl_uart_configure(const struct device *dev,
					const struct uart_config *cfg)
{
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->api;

	if (api->configure == NULL) {
		return -ENOSYS;
	}
	return api->configure(dev, cfg);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
#endif
}

/**
 * @brief Get UART configuration.
 *
 * Stores current UART configuration to *cfg, can be used to retrieve initial
 * configuration after device was initialized using data from DTS.
 *
 * @param dev UART device instance.
 * @param cfg UART configuration structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 * @retval -ENOSYS If driver does not support getting current configuration.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_config_get(const struct device *dev,
			      struct uart_config *cfg);

static inline int z_impl_uart_config_get(const struct device *dev,
					 struct uart_config *cfg)
{
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->api;

	if (api->config_get == NULL) {
		return -ENOSYS;
	}

	return api->config_get(dev, cfg);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
#endif
}

/**
 * @addtogroup uart_interrupt
 * @{
 */

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
 * @param dev UART device instance.
 * @param tx_data Data to transmit.
 * @param size Number of bytes to send.
 *
 * @return Number of bytes sent.
 * @retval -ENOSYS  if this function is not supported
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_fill(const struct device *dev,
				 const uint8_t *tx_data,
				 int size)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->fifo_fill == NULL) {
		return -ENOSYS;
	}

	return api->fifo_fill(dev, tx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
}

/**
 * @brief Fill FIFO with wide data.
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
 * @param dev UART device instance.
 * @param tx_data Wide data to transmit.
 * @param size Number of datum to send.
 *
 * @return Number of datum sent.
 * @retval -ENOSYS If this function is not implemented
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_fill_u16(const struct device *dev,
				     const uint16_t *tx_data,
				     int size)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->fifo_fill_u16 == NULL) {
		return -ENOSYS;
	}

	return api->fifo_fill_u16(dev, tx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
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
 * @param dev UART device instance.
 * @param rx_data Data container.
 * @param size Container size.
 *
 * @return Number of bytes read.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_read(const struct device *dev, uint8_t *rx_data,
				 const int size)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->fifo_read == NULL) {
		return -ENOSYS;
	}

	return api->fifo_read(dev, rx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
}

/**
 * @brief Read wide data from FIFO.
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
 * @param dev UART device instance.
 * @param rx_data Wide data container.
 * @param size Container size.
 *
 * @return Number of datum read.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_read_u16(const struct device *dev,
				     uint16_t *rx_data,
				     const int size)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->fifo_read_u16 == NULL) {
		return -ENOSYS;
	}

	return api->fifo_read_u16(dev, rx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
}

/**
 * @brief Enable TX interrupt in IER.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_tx_enable(const struct device *dev);

static inline void z_impl_uart_irq_tx_enable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_tx_enable != NULL) {
		api->irq_tx_enable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

/**
 * @brief Disable TX interrupt in IER.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_tx_disable(const struct device *dev);

static inline void z_impl_uart_irq_tx_disable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_tx_disable != NULL) {
		api->irq_tx_disable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
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
 * @param dev UART device instance.
 *
 * @retval 1 If TX interrupt is enabled and at least one char can be
 *           written to UART.
 * @retval 0 If device is not ready to write a new byte.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_tx_ready(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_tx_ready == NULL) {
		return -ENOSYS;
	}

	return api->irq_tx_ready(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

/**
 * @brief Enable RX interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_rx_enable(const struct device *dev);

static inline void z_impl_uart_irq_rx_enable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_rx_enable != NULL) {
		api->irq_rx_enable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

/**
 * @brief Disable RX interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_rx_disable(const struct device *dev);

static inline void z_impl_uart_irq_rx_disable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_rx_disable != NULL) {
		api->irq_rx_disable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
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
 * @param dev UART device instance.
 *
 * @retval 1 If nothing remains to be transmitted.
 * @retval 0 If transmission is not completed.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_tx_complete(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_tx_complete == NULL) {
		return -ENOSYS;
	}
	return api->irq_tx_complete(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
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
 * @param dev UART device instance.
 *
 * @retval 1 If a received char is ready.
 * @retval 0 If a received char is not ready.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_rx_ready(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_rx_ready == NULL) {
		return -ENOSYS;
	}
	return api->irq_rx_ready(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}
/**
 * @brief Enable error interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_err_enable(const struct device *dev);

static inline void z_impl_uart_irq_err_enable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_err_enable) {
		api->irq_err_enable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

/**
 * @brief Disable error interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_err_disable(const struct device *dev);

static inline void z_impl_uart_irq_err_disable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_err_disable) {
		api->irq_err_disable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

/**
 * @brief Check if any IRQs is pending.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If an IRQ is pending.
 * @retval 0 If an IRQ is not pending.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_irq_is_pending(const struct device *dev);

static inline int z_impl_uart_irq_is_pending(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_is_pending == NULL) {
		return -ENOSYS;
	}
	return api->irq_is_pending(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
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
 * * For devices with explicit acknowledgment of interrupts, to ack
 *   any pending interrupts and likewise to cache the original value.
 * * For devices with implicit acknowledgment, this function will be
 *   empty. But the ISR must perform the actions needs to ack the
 *   interrupts (usually, call uart_fifo_read() on rx_ready, and
 *   uart_fifo_fill() on tx_ready).
 *
 * @param dev UART device instance.
 *
 * @retval 1 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_irq_update(const struct device *dev);

static inline int z_impl_uart_irq_update(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->irq_update == NULL) {
		return -ENOSYS;
	}
	return api->irq_update(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

/**
 * @brief Set the IRQ callback function pointer.
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with specified user data.
 * See description of uart_irq_update() for the requirements on ISR.
 *
 * @param dev UART device instance.
 * @param cb Pointer to the callback function.
 * @param user_data Data to pass to callback function.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_callback_user_data_set(const struct device *dev,
						  uart_irq_callback_user_data_t cb,
						  void *user_data)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if ((api != NULL) && (api->irq_callback_set != NULL)) {
		api->irq_callback_set(dev, cb, user_data);
		return 0;
	} else {
		return -ENOSYS;
	}
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
#endif
}

/**
 * @brief Set the IRQ callback function pointer (legacy).
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with the device pointer.
 *
 * @param dev UART device instance.
 * @param cb Pointer to the callback function.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t cb)
{
	return uart_irq_callback_user_data_set(dev, cb, NULL);
}

/**
 * @}
 */

/**
 * @addtogroup uart_async
 * @{
 */

/**
 * @brief Set event handler function.
 *
 * Since it is mandatory to set callback to use other asynchronous functions,
 * it can be used to detect if the device supports asynchronous API. Remaining
 * API does not have that detection.
 *
 * @param dev       UART device instance.
 * @param callback  Event handler.
 * @param user_data Data to pass to event handler function.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If not supported by the device.
 * @retval -ENOTSUP If API not enabled.
 */
static inline int uart_callback_set(const struct device *dev,
				    uart_callback_t callback,
				    void *user_data)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->api;

	if (api->callback_set == NULL) {
		return -ENOSYS;
	}

	return api->callback_set(dev, callback, user_data);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
#endif
}

/**
 * @brief Send given number of bytes from buffer through UART.
 *
 * Function returns immediately and event handler,
 * set using @ref uart_callback_set, is called after transfer is finished.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to transmit buffer.
 * @param len     Length of transmit buffer.
 * @param timeout Timeout in microseconds. Valid only if flow control is
 *		  enabled. @ref SYS_FOREVER_US disables timeout.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY If There is already an ongoing transfer.
 * @retval -errno Other negative errno value in case of failure.
 */
__syscall int uart_tx(const struct device *dev, const uint8_t *buf,
		      size_t len,
		      int32_t timeout);

static inline int z_impl_uart_tx(const struct device *dev, const uint8_t *buf,
				 size_t len, int32_t timeout)

{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->api;

	return api->tx(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

/**
 * @brief Send given number of datum from buffer through UART.
 *
 * Function returns immediately and event handler,
 * set using @ref uart_callback_set, is called after transfer is finished.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to wide data transmit buffer.
 * @param len     Length of wide data transmit buffer.
 * @param timeout Timeout in milliseconds. Valid only if flow control is
 *		  enabled. @ref SYS_FOREVER_MS disables timeout.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY If there is already an ongoing transfer.
 * @retval -errno Other negative errno value in case of failure.
 */
__syscall int uart_tx_u16(const struct device *dev, const uint16_t *buf,
			  size_t len, int32_t timeout);

static inline int z_impl_uart_tx_u16(const struct device *dev,
				     const uint16_t *buf,
				     size_t len, int32_t timeout)

{
#if defined(CONFIG_UART_ASYNC_API) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->api;

	return api->tx_u16(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

/**
 * @brief Abort current TX transmission.
 *
 * #UART_TX_DONE event will be generated with amount of data sent.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EFAULT There is no active transmission.
 * @retval -errno Other negative errno value in case of failure.
 */
__syscall int uart_tx_abort(const struct device *dev);

static inline int z_impl_uart_tx_abort(const struct device *dev)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->api;

	return api->tx_abort(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

/**
 * @brief Start receiving data through UART.
 *
 * Function sets given buffer as first buffer for receiving and returns
 * immediately. After that event handler, set using @ref uart_callback_set,
 * is called with #UART_RX_RDY or #UART_RX_BUF_REQUEST events.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to receive buffer.
 * @param len     Buffer length.
 * @param timeout Inactivity period after receiving at least a byte which
 *		  triggers  #UART_RX_RDY event. Given in microseconds.
 *		  @ref SYS_FOREVER_US disables timeout. See @ref uart_event_type
 *		  for details.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY RX already in progress.
 * @retval -errno Other negative errno value in case of failure.
 *
 */
__syscall int uart_rx_enable(const struct device *dev, uint8_t *buf,
			     size_t len,
			     int32_t timeout);

static inline int z_impl_uart_rx_enable(const struct device *dev,
					uint8_t *buf,
					size_t len, int32_t timeout)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->api;

	return api->rx_enable(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

/**
 * @brief Start receiving wide data through UART.
 *
 * Function sets given buffer as first buffer for receiving and returns
 * immediately. After that event handler, set using @ref uart_callback_set,
 * is called with #UART_RX_RDY or #UART_RX_BUF_REQUEST events.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to wide data receive buffer.
 * @param len     Buffer length.
 * @param timeout Inactivity period after receiving at least a byte which
 *		  triggers  #UART_RX_RDY event. Given in milliseconds.
 *		  @ref SYS_FOREVER_MS disables timeout. See
 *		  @ref uart_event_type for details.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY RX already in progress.
 * @retval -errno Other negative errno value in case of failure.
 *
 */
__syscall int uart_rx_enable_u16(const struct device *dev, uint16_t *buf,
				 size_t len, int32_t timeout);

static inline int z_impl_uart_rx_enable_u16(const struct device *dev,
					    uint16_t *buf, size_t len,
					    int32_t timeout)
{
#if defined(CONFIG_UART_ASYNC_API) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->api;

	return api->rx_enable_u16(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

/**
 * @brief Provide receive buffer in response to #UART_RX_BUF_REQUEST event.
 *
 * Provide pointer to RX buffer, which will be used when current buffer is
 * filled.
 *
 * @note Providing buffer that is already in usage by driver leads to
 *       undefined behavior. Buffer can be reused when it has been released
 *       by driver.
 *
 * @param dev UART device instance.
 * @param buf Pointer to receive buffer.
 * @param len Buffer length.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY Next buffer already set.
 * @retval -EACCES Receiver is already disabled (function called too late?).
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				  size_t len)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->api;

	return api->rx_buf_rsp(dev, buf, len);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -ENOTSUP;
#endif
}

/**
 * @brief Provide wide data receive buffer in response to #UART_RX_BUF_REQUEST
 * event.
 *
 * Provide pointer to RX buffer, which will be used when current buffer is
 * filled.
 *
 * @note Providing buffer that is already in usage by driver leads to
 *       undefined behavior. Buffer can be reused when it has been released
 *       by driver.
 *
 * @param dev UART device instance.
 * @param buf Pointer to wide data receive buffer.
 * @param len Buffer length.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled
 * @retval -EBUSY Next buffer already set.
 * @retval -EACCES Receiver is already disabled (function called too late?).
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_rx_buf_rsp_u16(const struct device *dev, uint16_t *buf,
				      size_t len)
{
#if defined(CONFIG_UART_ASYNC_API) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->api;

	return api->rx_buf_rsp_u16(dev, buf, len);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -ENOTSUP;
#endif
}

/**
 * @brief Disable RX
 *
 * #UART_RX_BUF_RELEASED event will be generated for every buffer scheduled,
 * after that #UART_RX_DISABLED event will be generated. Additionally, if there
 * is any pending received data, the #UART_RX_RDY event for that data will be
 * generated before the #UART_RX_BUF_RELEASED events.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EFAULT There is no active reception.
 * @retval -errno Other negative errno value in case of failure.
 */
__syscall int uart_rx_disable(const struct device *dev);

static inline int z_impl_uart_rx_disable(const struct device *dev)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->api;

	return api->rx_disable(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

/**
 * @}
 */

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev UART device instance.
 * @param ctrl The line control to manipulate (see enum uart_line_ctrl).
 * @param val Value to set to the line control.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -errno Other negative errno value in case of failure.
 */
__syscall int uart_line_ctrl_set(const struct device *dev,
				 uint32_t ctrl, uint32_t val);

static inline int z_impl_uart_line_ctrl_set(const struct device *dev,
					    uint32_t ctrl, uint32_t val)
{
#ifdef CONFIG_UART_LINE_CTRL
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->line_ctrl_set == NULL) {
		return -ENOSYS;
	}
	return api->line_ctrl_set(dev, ctrl, val);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(ctrl);
	ARG_UNUSED(val);
	return -ENOTSUP;
#endif
}

/**
 * @brief Retrieve line control for UART.
 *
 * @param dev UART device instance.
 * @param ctrl The line control to retrieve (see enum uart_line_ctrl).
 * @param val Pointer to variable where to store the line control value.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -errno Other negative errno value in case of failure.
 */
__syscall int uart_line_ctrl_get(const struct device *dev, uint32_t ctrl,
				 uint32_t *val);

static inline int z_impl_uart_line_ctrl_get(const struct device *dev,
					    uint32_t ctrl, uint32_t *val)
{
#ifdef CONFIG_UART_LINE_CTRL
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->line_ctrl_get == NULL) {
		return -ENOSYS;
	}
	return api->line_ctrl_get(dev, ctrl, val);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(ctrl);
	ARG_UNUSED(val);
	return -ENOTSUP;
#endif
}

/**
 * @brief Send extra command to driver.
 *
 * Implementation and accepted commands are driver specific.
 * Refer to the drivers for more information.
 *
 * @param dev UART device instance.
 * @param cmd Command to driver.
 * @param p Parameter to the command.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -errno Other negative errno value in case of failure.
 */
__syscall int uart_drv_cmd(const struct device *dev, uint32_t cmd, uint32_t p);

static inline int z_impl_uart_drv_cmd(const struct device *dev, uint32_t cmd,
				      uint32_t p)
{
#ifdef CONFIG_UART_DRV_CMD
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->api;

	if (api->drv_cmd == NULL) {
		return -ENOSYS;
	}
	return api->drv_cmd(dev, cmd, p);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);
	ARG_UNUSED(p);
	return -ENOTSUP;
#endif
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/uart.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_H_ */
