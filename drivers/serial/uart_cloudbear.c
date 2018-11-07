/*
 * Copyright (c) 2018 CloudBEAR <www.cloudbear.ru>
 *                    Alexander Kozlov <alexander.kozlov@cloudbear.ru>
 *                    Aleksey Kovalov <alexey.kovalov@cloudbear.ru>
 *                    Vitaly Gaiduk <vitaly.gaiduk@cloudbear.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for the CloudBEAR family.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <uart.h>

/** @brief UART definitions for extra commands to driver */
#define     UART_TX_EMPTY_CMD   1
#define     UART_TX_IDLE_CMD    2

#define     UART_TX_CNT     0x08
#define     UART_RX_CNT     0x00
#define     UART_TX_EN      0x01
#define     UART_RX_EN      0x01
#define     UART_IE_TX      0x01
#define     UART_IE_RX      0x02
#define     UART_TX_FULL    0x01
#define     UART_RX_EMPTY   0x02
#define     UART_TX_EMPTY   0x04
#define     UART_TX_IDLE    0x08

#define     UART_0_IRQ        (RISCV_MAX_GENERIC_IRQ + 1)

struct uart_bear_regs_t {
	u32_t tx_data;       /* 0x000 */
	u32_t reserve_tx_data;
	u32_t rx_data;       /* 0x008 */
	u32_t reserve_rx_data;
	u32_t tx_ctrl;       /* 0x010 */
	u32_t reserve_tx_ctrl;
	u32_t rx_ctrl;       /* 0x018 */
	u32_t reserve_rx_ctrl;
	u32_t status;        /* 0x020 */
	u32_t reserve_status;
	u32_t error;         /* 0x028 */
	u32_t reserve_error;
	u32_t baud_div;      /* 0x030 */
	u32_t reserve_baud_div;
	u32_t ie;            /* 0x038 */
	u32_t reserve_ie;
	u32_t ip;            /* 0x040 */
	u32_t reserve_ip;
};

struct uart_bear_dev_data_t {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t callback;
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(void);
#endif

struct uart_bear_device_config {
	u32_t base;
	u32_t sys_clk_freq;
	u32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t cfg_func;
#endif
};

#define DEV_CFG(dev)						\
	((const struct uart_bear_device_config *const)	\
		(dev)->config->config_info)
#define DEV_DATA(dev)						\
	((struct uart_bear_dev_data_t *const)(dev)->driver_data)
#define DEV_UART(dev)						\
	((volatile struct uart_bear_regs_t *const)(DEV_CFG(dev))->base)
static struct device DEVICE_NAME_GET(uart_bear);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_bear_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct uart_bear_dev_data_t *data = DEV_DATA(dev);

	if (data->callback) {
		data->callback(dev);
	}
}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_bear_irq_cfg_func(void);
#endif
static const struct uart_bear_device_config uart_bear_dev_cfg = {
	.base = DT_BEAR_UART_0_BASE_ADDR,
	.sys_clk_freq = CONFIG_BEAR_PERIPH_CLK_FREQ,
	.baud_rate = DT_BEAR_UART_0_CURRENT_SPEED,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cfg_func     = uart_bear_irq_cfg_func
#endif
};

static struct uart_bear_dev_data_t uart_bear_dev_data = {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.callback = NULL
#endif
};

#ifdef CONFIG_UART_DRV_CMD
static int uart_dev_tx_empty(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	return !!(uart->status & UART_TX_EMPTY);
}
static int uart_dev_tx_idle(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	return !!(uart->status & UART_TX_IDLE);
}
static int uart_bear_drv_cmd(struct device *dev, u32_t cmd, u32_t p)
{
	switch (cmd) {
	case UART_TX_EMPTY_CMD:
		return uart_dev_tx_empty(dev);
	case UART_TX_IDLE_CMD:
		return uart_dev_tx_idle(dev);
	default:
		return -1;
	}
}
#endif

/**
 * @brief Init UART device
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_bear_init(struct device *dev)
{
	const struct uart_bear_device_config *config = DEV_CFG(dev);
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	uart->baud_div = config->sys_clk_freq / config->baud_rate;
	uart->tx_ctrl = UART_TX_EN | (UART_TX_CNT << 16);
	uart->rx_ctrl = UART_RX_EN | (UART_RX_CNT << 16);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart->ie = 0;
	config->cfg_func();
#endif
	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_bear_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	if (!(uart->status & UART_RX_EMPTY)) {   /* rx_not_empty */
		*c = (unsigned char)uart->rx_data;   /* rx_ pop */
	} else {
		return -1;
	}
	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register if transmitter is not full.
 *
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char uart_bear_poll_out(struct device *dev, unsigned char c)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	while (uart->status & UART_TX_FULL)
		;
	uart->tx_data = (unsigned int) c;
	return c;
}

/**
 * @brief Get ERROR reg
 *
 * @param dev UART device struct
 *
 * @return error register
 */
static int uart_bear_err_check(struct device *dev)
{
	/*
	 * No conversion is needed since BEAR UART
	 * error register follow Zephyr format
	 */
	return DEV_UART(dev)->error;
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
static int uart_bear_fifo_fill(struct device *dev,
			       const u8_t *tx_data,
			       int size)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);
	int i;

	for (i = 0; i < size && !(uart->status & UART_TX_FULL); ++i) {
		uart->tx_data = tx_data[i];
	}
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
static int uart_bear_fifo_read(struct device *dev,
			       u8_t *rx_data,
			       const int size)
{
	int i;
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	for (i = 0; i < size && !(uart->status & UART_RX_EMPTY); i++) {
		*(rx_data + i) = (unsigned char)uart->rx_data;
	}
	return i;
}

/**
 * @brief Enable TX interrupt in ie register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_bear_irq_tx_enable(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	uart->ie |= UART_IE_TX;
}

/**
 * @brief Disable TX interrupt in ie register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_bear_irq_tx_disable(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	uart->ie &= ~UART_IE_TX;
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_bear_irq_tx_ready(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & UART_IE_TX);
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_bear_irq_tx_complete(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	return !!(uart->status & UART_TX_EMPTY);
}

/**
 * @brief Enable RX interrupt in ie register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_bear_irq_rx_enable(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	uart->ie |= UART_IE_RX;
}

/**
 * @brief Disable RX interrupt in ie register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_bear_irq_rx_disable(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	uart->ie &= ~UART_IE_RX;
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_bear_irq_rx_ready(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & UART_IE_RX);
}

/* No error interrupt for this controller */
static void uart_bear_irq_err_enable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_bear_irq_err_disable(struct device *dev)
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
static int uart_bear_irq_is_pending(struct device *dev)
{
	volatile struct uart_bear_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & (UART_IE_RX | UART_IE_TX));
}

static int uart_bear_irq_update(struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void uart_bear_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_bear_dev_data_t *data = DEV_DATA(dev);

	data->callback = cb;
	data->cb_data = cb_data;
}
#endif

static const struct uart_driver_api uart_bear_driver_api = {
#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd    = uart_bear_drv_cmd,
#endif
	.poll_in         = uart_bear_poll_in,
	.poll_out        = uart_bear_poll_out,
	.err_check       = uart_bear_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_bear_fifo_fill,
	.fifo_read        = uart_bear_fifo_read,
	.irq_tx_enable    = uart_bear_irq_tx_enable,
	.irq_tx_disable   = uart_bear_irq_tx_disable,
	.irq_tx_ready     = uart_bear_irq_tx_ready,
	.irq_tx_complete  = uart_bear_irq_tx_complete,
	.irq_rx_enable    = uart_bear_irq_rx_enable,
	.irq_rx_disable   = uart_bear_irq_rx_disable,
	.irq_rx_ready     = uart_bear_irq_rx_ready,
	.irq_err_enable   = uart_bear_irq_err_enable,
	.irq_err_disable  = uart_bear_irq_err_disable,
	.irq_is_pending   = uart_bear_irq_is_pending,
	.irq_update       = uart_bear_irq_update,
	.irq_callback_set = uart_bear_irq_callback_set
#endif
};

DEVICE_AND_API_INIT(uart_bear, DT_BEAR_UART_0_LABEL,
		    uart_bear_init, &uart_bear_dev_data,
		    &uart_bear_dev_cfg,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_bear_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_bear_irq_cfg_func(void)
{
	IRQ_CONNECT(UART_0_IRQ,
		    CONFIG_BEAR_UART_IRQ_PRIORITY,
		    uart_bear_isr,
		    DEVICE_GET(uart_bear),
		    0);
	irq_enable(UART_0_IRQ);
}
#endif
