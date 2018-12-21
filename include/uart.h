/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for UART drivers
 */

#ifndef ZEPHYR_INCLUDE_UART_H_
#define ZEPHYR_INCLUDE_UART_H_

/**
 * @brief UART Interface
 * @defgroup uart_interface UART Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stddef.h>

#include <device.h>

#ifdef CONFIG_PCI
#include <drivers/pci/pci.h>
#include <drivers/pci/pci_mgr.h>
#endif
/**
 * @brief Options for @a UART initialization.
 */
#define UART_OPTION_AFCE 0x01

/** Common line controls for UART.*/
#define LINE_CTRL_BAUD_RATE	(1 << 0)
#define LINE_CTRL_RTS		(1 << 1)
#define LINE_CTRL_DTR		(1 << 2)
#define LINE_CTRL_DCD		(1 << 3)
#define LINE_CTRL_DSR		(1 << 4)

/* Common communication errors for UART.*/

/** @brief Overrun error */
#define UART_ERROR_OVERRUN  (1 << 0)

/** @brief Parity error */
#define UART_ERROR_PARITY   (1 << 1)

/** @brief Framing error */
#define UART_ERROR_FRAMING  (1 << 2)

/**
 * @brief Break interrupt error:
 *
 * A break interrupt was received. This happens when the serial input is
 * held at a logic '0' state for longer than the sum of start time + data bits
 * + parity + stop bits.
 */
#define UART_ERROR_BREAK    (1 << 3)

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

#ifdef CONFIG_PCI
	struct pci_dev_info  pci_dev;
#endif /* CONFIG_PCI */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t	irq_config_func;
#endif
};

/** @brief Driver API structure. */
struct uart_driver_api {
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

/**
 * @brief Check whether an error was detected.
 *
 * @param dev UART device structure.
 *
 * @retval UART_ERROR_OVERRUN if an overrun error was detected.
 * @retval UART_ERROR_PARITY if a parity error was detected.
 * @retval UART_ERROR_FRAMING if a framing error was detected.
 * @retval UART_ERROR_BREAK if a break error was detected.
 * @retval 0 Otherwise.
 */
__syscall int uart_err_check(struct device *dev);

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
 */
__syscall int uart_poll_in(struct device *dev, unsigned char *p_char);

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

/**
 * @brief Disable TX interrupt in IER.
 *
 * @param dev UART device structure.
 *
 * @return N/A
 */
__syscall void uart_irq_tx_disable(struct device *dev);

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

/**
 * @brief Disable RX interrupt.
 *
 * @param dev UART device structure.
 *
 * @return N/A
 */
__syscall void uart_irq_rx_disable(struct device *dev);

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

/**
 * @brief Disable error interrupt.
 *
 * @param dev UART device structure.
 *
 * @retval 1 If an IRQ is ready.
 * @retval 0 Otherwise.
 */
__syscall void uart_irq_err_disable(struct device *dev);

/**
 * @brief Check if any IRQs is pending.
 *
 * @param dev UART device structure.
 *
 * @retval 1 If an IRQ is pending.
 * @retval 0 Otherwise.
 */
__syscall int uart_irq_is_pending(struct device *dev);

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

#endif /* CONFIG_UART_DRV_CMD */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/uart.h>

#endif /* ZEPHYR_INCLUDE_UART_H_ */
