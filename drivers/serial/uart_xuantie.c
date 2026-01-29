/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for the XuanTie xiaohui&smartl FPGA
 */

#define DT_DRV_COMPAT xuantie_uart0

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <soc.h>
#include <zephyr/irq.h>

#include <drv/uart.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/porting.h>
#include <drv/tick.h>
#include "uart_xuantie_ll.h"

/* clang-format off */

#define UART_TIMEOUT 0x10000000U

/* constants for line status register */

#define LSR_RXRDY    0x01 /* receiver data available */
#define LSR_OE       0x02 /* overrun error */
#define LSR_PE       0x04 /* parity error */
#define LSR_FE       0x08 /* framing error */
#define LSR_BI       0x10 /* break interrupt */
#define LSR_EOB_MASK 0x1E /* Error or Break mask */
#define LSR_THRE     0x20 /* transmit holding register empty */
#define LSR_TEMT     0x40 /* transmitter empty */


/* equates for interrupt identification register */

#define IIR_MSTAT    0x00 /* modem status interrupt  */
#define IIR_NIP      0x01 /* no interrupt pending    */
#define IIR_THRE     0x02 /* transmit holding register empty interrupt */
#define IIR_RBRF     0x04 /* receiver buffer register full interrupt */
#define IIR_LS       0x06 /* receiver line status interrupt */
#define IIR_MASK     0x07 /* interrupt id bits mask  */
#define IIR_ID       0x06 /* interrupt ID mask without NIP */
#define IIR_FE       0xC0 /* FIFO mode enabled */
#define IIR_CH       0x0C /* Character timeout*/

/* clang-format no */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(void);
#endif

struct uart_xuantie_device_config {
	csi_uart_t uart_handle;
	uintptr_t port;
	uint32_t sys_clk_freq;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t cfg_func;
#endif
};

struct uart_xuantie_data {
	struct k_spinlock lock;
	uint8_t fifo_size;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/**< cache of IIR since it clears when read */
	uint8_t iir_cache;
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define DEV_UART_BASE(dev)						\
	((dw_uart_regs_t *)				\
	 ((const struct uart_xuantie_device_config * const)(dev)->config)->port)

#define DEV_UART_HANDLE(dev)						\
	((csi_uart_t *)                  \
	&(((const struct uart_xuantie_device_config * const) \
		(dev)->config)->uart_handle))

#define IIRC(dev) (((struct uart_xuantie_data *)(dev)->data)->iir_cache)

static csi_error_t _csi_uart_init(csi_uart_t *uart, uint32_t idx)
{
	CSI_PARAM_CHK(uart, CSI_ERROR);

	csi_error_t ret = CSI_OK;
	dw_uart_regs_t *uart_base;

	uart_base = (dw_uart_regs_t *)HANDLE_REG_BASE(uart);

	dw_uart_fifo_init(uart_base);

	uart->rx_size = 0U;
	uart->tx_size = 0U;
	uart->rx_data = NULL;
	uart->tx_data = NULL;
	uart->tx_dma  = NULL;
	uart->rx_dma  = NULL;
	dw_uart_disable_trans_irq(uart_base);
	dw_uart_disable_recv_irq(uart_base);
	dw_uart_disable_auto_flow_control(uart_base);

	return ret;
}

static ATTRIBUTE_DATA csi_error_t _csi_uart_baud(csi_uart_t *uart, uint32_t baud, uint32_t clk)
{
	CSI_PARAM_CHK(uart, CSI_ERROR);

	int32_t ret = 0;
	csi_error_t csi_ret = CSI_OK;
	dw_uart_regs_t *uart_base;

	uart_base = (dw_uart_regs_t *) HANDLE_REG_BASE(uart);

	ret = dw_uart_config_baudrate(uart_base, baud, clk);
	if (ret == 0) {
		csi_ret = CSI_OK;
	} else {
		csi_ret = CSI_ERROR;
	}

	return csi_ret;
}

static csi_error_t _csi_uart_format
					(csi_uart_t *uart,  csi_uart_data_bits_t data_bits,
					csi_uart_parity_t parity, csi_uart_stop_bits_t stop_bits)
{
	CSI_PARAM_CHK(uart, CSI_ERROR);

	int32_t ret = 0;
	csi_error_t csi_ret = CSI_OK;
	dw_uart_regs_t *uart_base;

	uart_base = (dw_uart_regs_t *)HANDLE_REG_BASE(uart);

	switch (data_bits) {
	case UART_DATA_BITS_5:
		ret = dw_uart_config_data_bits(uart_base, 5U);
		break;

	case UART_DATA_BITS_6:
		ret = dw_uart_config_data_bits(uart_base, 6U);
		break;

	case UART_DATA_BITS_7:
		ret = dw_uart_config_data_bits(uart_base, 7U);
		break;

	case UART_DATA_BITS_8:
		ret = dw_uart_config_data_bits(uart_base, 8U);
		break;

	default:
		ret = -1;
		break;
	}

	if (ret == 0) {
		switch (parity) {
		case UART_PARITY_NONE:
			ret = dw_uart_config_parity_none(uart_base);
			break;

		case UART_PARITY_ODD:
			ret = dw_uart_config_parity_odd(uart_base);
			break;

		case UART_PARITY_EVEN:
			ret = dw_uart_config_parity_even(uart_base);
			break;

		default:
			ret = -1;
			break;
		}

		if (ret == 0) {
			switch (stop_bits) {
			case UART_STOP_BITS_1:
				ret = dw_uart_config_stop_bits(uart_base, 1U);
				break;

			case UART_STOP_BITS_2:
				ret = dw_uart_config_stop_bits(uart_base, 2U);
				break;

			case UART_STOP_BITS_1_5:
				if (data_bits == UART_DATA_BITS_5) {
					ret = dw_uart_config_stop_bits(uart_base, 2U);
					break;
				}

			default:
				ret = -1;
				break;
			}

			if (ret != 0) {
				csi_ret = CSI_ERROR;
			}

		} else {
			csi_ret = CSI_ERROR;
		}

	} else {
		csi_ret = CSI_ERROR;
	}

	return csi_ret;
}

static void _csi_uart_putc(csi_uart_t *uart, uint8_t ch)
{
	CSI_PARAM_CHK_NORETVAL(uart);

	volatile int i = 10;
	uint32_t timeout = UART_TIMEOUT;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)HANDLE_REG_BASE(uart);

	while (!dw_uart_putready(uart_base) && timeout--) {
		/* wait */
	}

	if (timeout) {
		/* fix print luanma on irq-mode sometimes. maybe hw bug */
		while (i--) {
			/* wait */
		}
		dw_uart_putchar(uart_base, ch);
	}
}

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register if transmitter is not full.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_xuantie_poll_out(const struct device *dev,
								unsigned char c)
{
	struct uart_xuantie_data *data = dev->data;
	csi_uart_t *uart_handle = DEV_UART_HANDLE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	_csi_uart_putc(uart_handle, c);
	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_xuantie_poll_in(const struct device *dev, unsigned char *c)
{
	int ret = -1;
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((uart_base->LSR & LSR_RXRDY) != 0) {
		/* got a character */
		*c = dw_uart_getchar(uart_base);
		ret = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_xuantie_fifo_fill(const struct device *dev,
								const uint8_t *tx_data,
								int size)
{
	int i;
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; (i < size) && (i < data->fifo_size); i++) {
		dw_uart_putchar(uart_base, tx_data[i]);
	}

	k_spin_unlock(&data->lock, key);

	return i;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rxData Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_xuantie_fifo_read(const struct device *dev,
								uint8_t *rx_data,
								const int size)
{
	int i;
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; (i < size) && dw_uart_getready(uart_base); i++) {
		rx_data[i] = dw_uart_getchar(uart_base);
	}

	k_spin_unlock(&data->lock, key);

	return i;
}

/**
 * @brief Enable TX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_xuantie_irq_tx_enable(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	dw_uart_enable_trans_irq(uart_base);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable TX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_xuantie_irq_tx_disable(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	dw_uart_disable_trans_irq(uart_base);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xuantie_irq_tx_ready(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((IIRC(dev) & IIR_ID) == IIR_THRE) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_xuantie_irq_tx_complete(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((uart_base->LSR & (LSR_TEMT | LSR_THRE))
			== (LSR_TEMT | LSR_THRE)) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Enable RX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_xuantie_irq_rx_enable(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	dw_uart_enable_recv_irq(uart_base);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable RX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_xuantie_irq_rx_disable(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	dw_uart_disable_recv_irq(uart_base);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xuantie_irq_rx_ready(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint8_t intr_state = (uint8_t)((IIRC(dev) & 0xfU));
	int ret = ((intr_state == DW_UART_IIR_IID_RECV_DATA_AVAIL)
			|| (intr_state == DW_UART_IIR_IID_CHARACTER_TIMEOUT)) ? 1 : 0;
#endif

	k_spin_unlock(&data->lock, key);

	return ret;
}

/* No error interrupt for this controller */
static void uart_xuantie_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_xuantie_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_xuantie_irq_is_pending(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = (!(IIRC(dev) & IIR_NIP)) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int uart_xuantie_irq_update(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	dw_uart_regs_t *uart_base = (dw_uart_regs_t *)DEV_UART_BASE(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	IIRC(dev) = (uint8_t)(uart_base->IIR);

	k_spin_unlock(&data->lock, key);

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void
uart_xuantie_irq_callback_set(const struct device *dev,
							uart_irq_callback_user_data_t cb,
							void *cb_data)
{
	struct uart_xuantie_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_xuantie_irq_handler(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_xuantie_init(const struct device *dev)
{
	struct uart_xuantie_data *data = dev->data;
	const struct uart_xuantie_device_config * const cfg = dev->config;
	csi_uart_t *uart_handle = DEV_UART_HANDLE(dev);

	/* init the console */
	HANDLE_REG_BASE(uart_handle) = (unsigned long)DEV_UART_BASE(dev);
	_csi_uart_init(uart_handle, 0);

	/* config the UART */
	_csi_uart_baud(uart_handle, cfg->baud_rate, cfg->sys_clk_freq);
	_csi_uart_format(uart_handle, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);

	/* tx/rx fifo enable as default */
	data->fifo_size = 16;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Setup IRQ handler */
	cfg->cfg_func();
#endif

	return 0;
}

static const struct uart_driver_api uart_xuantie_driver_api = {
	.poll_in          = uart_xuantie_poll_in,
	.poll_out         = uart_xuantie_poll_out,
	.err_check        = NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_xuantie_fifo_fill,
	.fifo_read        = uart_xuantie_fifo_read,
	.irq_tx_enable    = uart_xuantie_irq_tx_enable,
	.irq_tx_disable   = uart_xuantie_irq_tx_disable,
	.irq_tx_ready     = uart_xuantie_irq_tx_ready,
	.irq_tx_complete  = uart_xuantie_irq_tx_complete,
	.irq_rx_enable    = uart_xuantie_irq_rx_enable,
	.irq_rx_disable   = uart_xuantie_irq_rx_disable,
	.irq_rx_ready     = uart_xuantie_irq_rx_ready,
	.irq_err_enable   = uart_xuantie_irq_err_enable,
	.irq_err_disable  = uart_xuantie_irq_err_disable,
	.irq_is_pending   = uart_xuantie_irq_is_pending,
	.irq_update       = uart_xuantie_irq_update,
	.irq_callback_set = uart_xuantie_irq_callback_set,
#endif
};

static struct uart_xuantie_data uart_xuantie_data_0;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_xuantie_irq_cfg_func_0(void);
#endif

static const struct uart_xuantie_device_config uart_xuantie_dev_cfg_0 = {
	.port         = DT_INST_REG_ADDR(0),
	.sys_clk_freq = DT_INST_PROP(0, clock_frequency),
	.baud_rate    = DT_INST_PROP(0, current_speed),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cfg_func     = uart_xuantie_irq_cfg_func_0,
#endif
};

DEVICE_DT_INST_DEFINE(0,
					uart_xuantie_init,
					NULL,
					&uart_xuantie_data_0, &uart_xuantie_dev_cfg_0,
					PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
					(void *)&uart_xuantie_driver_api);


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_xuantie_irq_cfg_func_0(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
				uart_xuantie_irq_handler, DEVICE_DT_INST_GET(0),
				0);

	irq_enable(DT_INST_IRQN(0));
}
#endif
