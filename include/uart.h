/* uart.h - public UART driver APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INCuarth
#define __INCuarth

/**
 * @brief UART Interface
 * @defgroup uart_interface UART Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <device.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif
/* options for uart init */
#define UART_OPTION_AFCE 0x01

/** Common line controls for UART */
#define LINE_CTRL_BAUD_RATE	(1 << 0)
#define LINE_CTRL_RTS		(1 << 1)
#define LINE_CTRL_DTR		(1 << 2)

/** UART device configuration */
struct uart_device_config {
	/**
	 * Base port number
	 * or memory mapped base address
	 * or register address
	 */
	union {
		uint32_t port;
		uint8_t *base;
		uint32_t regs;
	};
	uint8_t irq;		/**< interrupt request level */
	uint8_t irq_pri;	/**< interrupt priority */

	uint32_t sys_clk_freq;	/* System clock frequency in Hz */

#ifdef CONFIG_PCI
	struct pci_dev_info  pci_dev;
#endif /* CONFIG_PCI */
};

/**< Driver API struct */
struct uart_driver_api {
	/* console I/O functions */
	int (*poll_in)(struct device *dev, unsigned char *p_char);
	unsigned char (*poll_out)(struct device *dev, unsigned char out_char);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	/* interrupt driven I/O functions */
	int (*fifo_fill)(struct device *dev, const uint8_t *tx_data, int len);
	int (*fifo_read)(struct device *dev, uint8_t *rx_data, const int size);
	void (*irq_tx_enable)(struct device *dev);
	void (*irq_tx_disable)(struct device *dev);
	int (*irq_tx_ready)(struct device *dev);
	void (*irq_rx_enable)(struct device *dev);
	void (*irq_rx_disable)(struct device *dev);
	int (*irq_rx_ready)(struct device *dev);
	void (*irq_err_enable)(struct device *dev);
	void (*irq_err_disable)(struct device *dev);
	int (*irq_is_pending)(struct device *dev);
	int (*irq_update)(struct device *dev);
	unsigned int (*irq_get)(struct device *dev);
	int (*irq_input_hook)(struct device *dev, uint8_t byte);

#endif

#ifdef CONFIG_UART_LINE_CTRL
	int (*line_ctrl_set)(struct device *dev, uint32_t ctrl, uint32_t val);
#endif

#ifdef CONFIG_UART_DRV_CMD
	int (*drv_cmd)(struct device *dev, uint32_t cmd, uint32_t p);
#endif

};

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param p_char Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty,
 *         -DEV_INVALID_OP if operation not supported.
 */
static inline int uart_poll_in(struct device *dev, unsigned char *p_char)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	return api->poll_in(dev, p_char);
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * If the hardware flow control is enabled then the handshake signal CTS has to
 * be asserted in order to send a character.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param out_char Character to send
 *
 * @return Sent character
 */
static inline unsigned char uart_poll_out(struct device *dev,
					  unsigned char out_char)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	return api->poll_out(dev, out_char);
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static inline int uart_fifo_fill(struct device *dev, const uint8_t *tx_data,
				 int size)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->fifo_fill) {
		return api->fifo_fill(dev, tx_data, size);
	}

	return 0;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param rx_data Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static inline int uart_fifo_read(struct device *dev, uint8_t *rx_data,
				 const int size)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->fifo_read) {
		return api->fifo_read(dev, rx_data, size);
	}

	return 0;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void uart_irq_tx_enable(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_tx_enable) {
		api->irq_tx_enable(dev);
	}
}
/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void uart_irq_tx_disable(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_tx_disable) {
		api->irq_tx_disable(dev);
	}
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static inline int uart_irq_tx_ready(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_tx_ready) {
		return api->irq_tx_ready(dev);
	}

	return 0;
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void uart_irq_rx_enable(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_rx_enable) {
		api->irq_rx_enable(dev);
	}
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void uart_irq_rx_disable(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_tx_disable) {
		api->irq_tx_disable(dev);
	}
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static inline int uart_irq_rx_ready(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_rx_ready) {
		return api->irq_rx_ready(dev);
	}

	return 0;
}
/**
 * @brief Enable error interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void uart_irq_err_enable(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_err_enable) {
		api->irq_err_enable(dev);
	}
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static inline void uart_irq_err_disable(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_err_disable) {
		api->irq_err_disable(dev);
	}
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */

static inline int uart_irq_is_pending(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_is_pending)	{
		return api->irq_is_pending(dev);
	}

	return 0;
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return always 1
 */
static inline int uart_irq_update(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_update) {
		return api->irq_update(dev);
	}

	return 0;
}

/**
 * @brief Returns UART interrupt number
 *
 * Returns the IRQ number used by the specified UART port.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return IRQ number, or 0 if no interrupt
 */
static inline unsigned int uart_irq_get(struct device *dev)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->irq_get) {
		return api->irq_get(dev);
	}

	return 0;
}

/**
 * @brief Invoke the UART input hook routine if installed
 *
 * The input hook is a custom handler invoked by the ISR on each received
 * character.  It allows the detection of a character escape sequence that may
 * be used to override the behavior of the ISR handler.
 *
 * @param dev UART device struct (of type struct uart_device_config_t)
 * @param byte Byte to process
 *
 * @return 1 if character processing must stop, 0 or if it is to continue
 */
static inline int uart_irq_input_hook(struct device *dev, uint8_t byte)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;

	if ((api != NULL) && (api->irq_input_hook != NULL)) {
		return api->irq_input_hook(dev, byte);
	}

	return 0;
}

/**
 * @brief Set the UART input hook routine
 *
 * @param dev UART device struct (of type struct uart_device_config_t)
 * @param hook Routine to use as UART input hook
 *
 * @return N/A
 */
static inline void uart_irq_input_hook_set(struct device *dev,
		int (*hook)(struct device *, uint8_t))
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;

	if (api != NULL) {
		api->irq_input_hook = hook;
	}
}

#endif

#ifdef CONFIG_UART_LINE_CTRL

/**
 * @brief Manipualte line control for UART.
 *
 * @param dev UART device struct (of type struct uart_device_config_t)
 * @param ctrl The line control to be manipulated
 * @param val Value to set the line control
 *
 * @return DEV_OK if successful, failed otherwise
 */
static inline int uart_line_ctrl_set(struct device *dev,
				     uint32_t ctrl, uint32_t val)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;

	if (api && api->line_ctrl_set) {
		return api->line_ctrl_set(dev, ctrl, val);
	}

	return DEV_INVALID_OP;
}

#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD

/**
 * @brief Send extra command to driver
 *
 * Implementation and accepted commands are driver specific.
 * Refer to driver for more information.
 *
 * @param dev UART device struct (of type struct uart_device_config_t)
 * @param cmd Command to driver
 * @param p Parameter to the command
 *
 * @return DEV_OK if successful, failed otherwise
 */
static inline int uart_drv_cmd(struct device *dev, uint32_t cmd, uint32_t p)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;

	if (api && api->drv_cmd) {
		return api->drv_cmd(dev, cmd, p);
	}

	return DEV_INVALID_OP;
}

#endif /* CONFIG_UART_DRV_CMD */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __INCuarth */
