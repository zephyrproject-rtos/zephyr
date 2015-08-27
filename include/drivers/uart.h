/* uart.h - public UART driver APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __INCuarth
#define __INCuarth

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif

struct uart_init_info;

/* UART device configuration */
struct uart_device_config_t {
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

#ifdef CONFIG_PCI
	struct pci_dev_info  pci_dev;
#endif /* CONFIG_PCI */

	/**
	 * Initializes the UART port.
	 * It has to configure the device to 8n1.
	 */
	void (*port_init)(struct device *dev,
			  const struct uart_init_info * const pinfo);

	/**< Configuration function */
	int (*config_func)(struct device *dev);
};

/** UART configuration structure */
struct uart_init_info {
	int baud_rate;		/* Baud rate */
	uint32_t sys_clk_freq;	/* System clock frequency in Hz */

	uint8_t irq_pri;	/* Interrupt priority level */
	uint8_t options;	/* HW Flow Control option */

	uint32_t regs;		/* Register address */
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

#endif
};

int uart_platform_init(struct device *dev);

/**
 * @brief Initialize UART
 *
 * UART driver has to configure the device to 8n1.
 *
 * @param dev UART device struct
 * @param pinfo UART configuration
 */
static inline void uart_init(struct device *dev,
			     const struct uart_init_info * const pinfo)
{
	struct uart_device_config_t *dev_cfg =
	    (struct uart_device_config_t *)dev->config->config_info;

	if (dev_cfg->port_init != 0) {
		dev_cfg->port_init(dev, pinfo);
	}
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct (of type struct uart_device_config_t)
 * @param p_char Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty,
 * 		-DEV_INVALID_OP if operation not supported.
 */
static inline int uart_poll_in(struct device *dev, unsigned char *p_char)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->poll_in) {
		return api->poll_in(dev, p_char);
	}

	return -DEV_INVALID_OP;
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
 * @param dev UART device struct (of type struct uart_device_config_t)
 * @param out_char Character to send
 *
 * @return Sent character
 */
static inline unsigned char uart_poll_out(struct device *dev,
					  unsigned char out_char)
{
	struct uart_driver_api *api;

	api = (struct uart_driver_api *)dev->driver_api;
	if (api && api->poll_out) {
		return api->poll_out(dev, out_char);
	}

	return 0;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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

#endif

#ifdef __cplusplus
}
#endif

#endif /* __INCuarth */
