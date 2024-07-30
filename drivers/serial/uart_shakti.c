/* For Shakti Vajra SOC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for the Shakti Vajra Processor
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>

#define DT_DRV_COMPAT shakti_uart

// #define CONFIG_UART_SHAKTI_PORT_0 1

#ifdef CONFIG_BOARD_SHAKTI_VAJRA

#define SHAKTI_NEXYS_FREQUENCY 50000000
#define SHAKTI_UART_1_CLK_FREQUENCY 50000000
#define SHAKTI_UART_BAUD 			19200
#define SHAKTI_VCU_UART_BAUD 		115200

#endif

#ifdef CONFIG_BOARD_SECURE_IOT

#define SHAKTI_NEXYS_FREQUENCY 40000000 // Change to 40000000 for nexys video board and 100 * 10^6 for vcu118 FPGA
#define SHAKTI_UART_1_CLK_FREQUENCY 40000000
#define SECIOT_NEXYS_UART_BAUD 19200
#define SECIOT_VCU118_UART_BAUD 115200

#endif

#define RXDATA_EMPTY   (1 << 31)   /* Receive FIFO Empty */
#define RXDATA_MASK    0xFF        /* Receive Data Mask */

#define TXDATA_FULL    (1 << 31)   /* Transmit FIFO Full */

#define TXCTRL_TXEN    (1 << 0)    /* Activate Tx Channel */

#define RXCTRL_RXEN    (1 << 0)    /* Activate Rx Channel */

#define IE_TXWM        (1 << 0)    /* TX Interrupt Enable/Pending */
#define IE_RXWM        (1 << 1)    /* RX Interrupt Enable/Pending */

#define UART_TX_OFFSET        0x04
#define UART_RX_OFFSET        0x08
#define UART_STATUS_OFFSET      0x0c
#define UART_EV_ENABLE_OFFSET   0x18
#define UART_BAUD_OFFSET        0x00

#define BREAK_ERROR	    1 << 7
#define FRAME_ERROR	    1 << 6
#define OVERRUN        	    1 << 5
#define PARITY_ERROR        1 << 4
#define STS_RX_FULL 	    1 << 3
#define STS_RX_NOT_EMPTY    1 << 2
#define STS_TX_FULL 	    1 << 1
#define STS_TX_EMPTY 	    1 << 0


/*
 * RX/TX Threshold count to generate TX/RX Interrupts.
 * Used by txctrl and rxctrl registers
 */
#define CTRL_CNT(x)    (((x) & 0x07) << 16)

struct uart_shakti_regs_t {
    uint16_t div;
    uint16_t reserv0;
    uint32_t tx;
    uint32_t rx;
    unsigned short  status;
    uint16_t reserv2;
    uint16_t delay;
    uint16_t reserv3;
    uint16_t control;
    uint16_t reserv4;
    uint8_t  ie; 
    uint8_t  reserv5;
    uint16_t reserv6;
    uint8_t  iqcycles;
    uint8_t  reserv7;
    uint16_t reserv8;
    uint8_t  rx_threshold;
    uint8_t  reserv9;
    uint16_t reserv10;
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(void);
#endif

struct uart_shakti_device_config {
	uint32_t       port;
	uint32_t       sys_clk_freq;
	uint32_t       baud_rate;
	uint32_t       rxcnt_irq;
	uint32_t       txcnt_irq;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t cfg_func;
#endif
};

struct uart_shakti_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define DEV_CFG(dev)						\
	((const struct uart_shakti_device_config * const)	\
	 (dev)->config)
#define DEV_UART(dev)						\
	((struct uart_shakti_regs_t *)(DEV_CFG(dev))->port)
#define DEV_DATA(dev)						\
	((struct uart_shakti_data * const)(dev)->driver_data)

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
static unsigned char uart_shakti_poll_out(struct device *dev,
					 unsigned char c)
{
  	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	// Wait while TX FIFO is full
	while (uart->status & STS_TX_FULL)
		;

	uart->tx = (int)c;

	return c; 
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_shakti_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	if (uart->status & STS_RX_NOT_EMPTY)
		return -1;

	volatile uint32_t read_val = uart->rx;
	*c = (unsigned char)(read_val & RXDATA_MASK);

	return 0;
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
static int uart_shakti_fifo_fill(struct device *dev,
				const u8_t *tx_data,
				int size)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);
	int i;

	for (i = 0; i < size && !(uart->tx & TXDATA_FULL); i++)
		uart->tx = (int)tx_data[i];

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
static int uart_shakti_fifo_read(struct device *dev,
				u8_t *rx_data,
				const int size)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);
	int i;
	uint32_t val;

	for (i = 0; i < size; i++) {
		val = uart->rx;

		if (val & RXDATA_EMPTY)
			break;

		rx_data[i] = (u8_t)(val & RXDATA_MASK);
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
static void uart_shakti_irq_tx_enable(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	uart->ie |= IE_TXWM;
}

/**
 * @brief Disable TX interrupt in ie register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_shakti_irq_tx_disable(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	uart->ie &= ~IE_TXWM;
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_shakti_irq_tx_ready(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & IE_TXWM);
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_shakti_irq_tx_complete(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	/*
	 * No TX EMTPY flag for this controller,
	 * just check if TX FIFO is not full
	 */
	return !(uart->tx & TXDATA_FULL);
}

/**
 * @brief Enable RX interrupt in ie register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_shakti_irq_rx_enable(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	uart->ie |= IE_RXWM;
}

/**
 * @brief Disable RX interrupt in ie register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_shakti_irq_rx_disable(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	uart->ie &= ~IE_RXWM;
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_shakti_irq_rx_ready(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & IE_RXWM);
}

/* No error interrupt for this controller */
static void uart_shakti_irq_err_enable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_shakti_irq_err_disable(struct device *dev)
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
static int uart_shakti_irq_is_pending(struct device *dev)
{
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & (IE_RXWM | IE_TXWM));
}

static int uart_shakti_irq_update(struct device *dev)
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
static void uart_shakti_irq_callback_set(struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_shakti_data *data = DEV_DATA(dev);

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_shakti_irq_handler(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct uart_shakti_data *data = DEV_DATA(dev);

	if (data->callback)
		data->callback(data->cb_data);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static int uart_shakti_init(struct device *dev)
{
	const struct uart_shakti_device_config * const cfg = DEV_CFG(dev);
	volatile struct uart_shakti_regs_t *uart = DEV_UART(dev);

	//uart->base_addr = 0x11300;

	/* Enable TX and RX channels */
	//uart->txctrl = TXCTRL_TXEN | CTRL_CNT(cfg->rxcnt_irq);
	//uart->rxctrl = RXCTRL_RXEN | CTRL_CNT(cfg->txcnt_irq);

	/* Set baud rate */
	uart->div = (cfg->sys_clk_freq / cfg->baud_rate) / 16;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Ensure that uart IRQ is disabled initially */
	uart->ie = 0;

	/* Setup IRQ handler */
	cfg->cfg_func();
#endif

	return 0;
}

static const struct uart_driver_api uart_shakti_driver_api = {
	.poll_in          = uart_shakti_poll_in,
	.poll_out         = uart_shakti_poll_out,
	.err_check        = NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_shakti_fifo_fill,
	.fifo_read        = uart_shakti_fifo_read,
	.irq_tx_enable    = uart_shakti_irq_tx_enable,
	.irq_tx_disable   = uart_shakti_irq_tx_disable,
	.irq_tx_ready     = uart_shakti_irq_tx_ready,
	.irq_tx_complete  = uart_shakti_irq_tx_complete,
	.irq_rx_enable    = uart_shakti_irq_rx_enable,
	.irq_rx_disable   = uart_shakti_irq_rx_disable,
	.irq_rx_ready     = uart_shakti_irq_rx_ready,
	.irq_err_enable   = uart_shakti_irq_err_enable,
	.irq_err_disable  = uart_shakti_irq_err_disable,
	.irq_is_pending   = uart_shakti_irq_is_pending,
	.irq_update       = uart_shakti_irq_update,
	.irq_callback_set = uart_shakti_irq_callback_set,
#endif
};
// #ifdef CONFIG_UART_INTERRUPT_DRIVEN									\
// 	.cfg_func     = uart_shakti_irq_cfg_func_0,						\
// #endif

#define UART_SHAKTI_INIT(n) \
	static struct uart_shakti_device_config uart_shakti_dev_cfg_##n = { \
	.port         = DT_INST_PROP(n, base),							\
	.sys_clk_freq = SHAKTI_NEXYS_FREQUENCY,							\
	.baud_rate    = DT_INST_PROP(n, current_speed),					\
	.rxcnt_irq    = 0,												\	
	.txcnt_irq    = 0,												\	
	};																\
	DEVICE_DT_INST_DEFINE(n,										\
		    uart_shakti_init,										\
		    NULL,													\
		    NULL, &uart_shakti_dev_cfg_##n,							\
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,				\
		    (void *)&uart_shakti_driver_api);						\
// #ifdef CONFIG_UART_SHAKTI

// static struct uart_shakti_data uart_shakti_data_0;

// #ifdef CONFIG_UART_INTERRUPT_DRIVEN
// static void uart_shakti_irq_cfg_func_0(void);
// #endif

// // PINCTRL_DT_INST_DEFINE(0);

// static const struct uart_shakti_device_config uart_shakti_dev_cfg_0 = {
// 	.port         = DT_INST_PROP(0, base),
// 	.sys_clk_freq = SHAKTI_NEXYS_FREQUENCY,
// 	.baud_rate    = DT_INST_PROP(0, current_speed),
// 	.rxcnt_irq    = 0,
// 	.txcnt_irq    = 0,
// 	// .pcfg	      = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
// #ifdef CONFIG_UART_INTERRUPT_DRIVEN
// 	.cfg_func     = uart_shakti_irq_cfg_func_0,
// #endif
// };

// DEVICE_DT_INST_DEFINE(0,
// 		    uart_shakti_init,
// 		    NULL,
// 		    &uart_shakti_data_0, &uart_shakti_dev_cfg_0,
// 		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
// 		    (void *)&uart_shakti_driver_api);


// #ifdef CONFIG_UART_INTERRUPT_DRIVEN
// static void uart_shakti_irq_cfg_func_0(void)
// {
// 	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
// 		    uart_shakti_irq_handler, DEVICE_DT_INST_GET(0),
// 		    0);

// 	irq_enable(DT_INST_IRQN(0));
// }
// #endif
// #endif /* CONFIG_UART_SHAKTI_PORT_0 */

// #ifdef CONFIG_UART_SHAKTI_PORT_1

// static struct uart_shakti_data uart_shakti_data_1;

// #ifdef CONFIG_UART_INTERRUPT_DRIVEN
// static void uart_shakti_irq_cfg_func_1(void);
// #endif

// static const struct uart_shakti_device_config uart_shakti_dev_cfg_1 = {
// 	.port         = DT_SHAKTI_UART_1_BASE_ADDR,
// 	.sys_clk_freq = DT_SHAKTI_UART_1_CLK_FREQ,
// 	.baud_rate    = DT_SHAKTI_UART_1_CURRENT_SPEED,
// 	.rxcnt_irq    = CONFIG_UART_SHAKTI_PORT_1_RXCNT_IRQ,
// 	.txcnt_irq    = CONFIG_UART_SHAKTI_PORT_1_TXCNT_IRQ,
// #ifdef CONFIG_UART_INTERRUPT_DRIVEN
// 	.cfg_func     = uart_shakti_irq_cfg_func_1,
// #endif
// };

// DEVICE_AND_API_INIT(uart_shakti_1, CONFIG_SHAKTI_UART_1_LABEL,
// 		    uart_shakti_init,
// 		    &uart_shakti_data_1, &uart_shakti_dev_cfg_1,
// 		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
// 		    (void *)&uart_shakti_driver_api);

// #ifdef CONFIG_UART_INTERRUPT_DRIVEN
// static void uart_shakti_irq_cfg_func_1(void)
// {
// 	IRQ_CONNECT(DT_SHAKTI_UART_1_IRQ_0,
// 		    CONFIG_UART_SHAKTI_PORT_1_IRQ_PRIORITY,
// 		    uart_shakti_irq_handler, DEVICE_GET(uart_shakti_1),
// 		    0);

// 	irq_enable(DT_SHAKTI_UART_1_IRQ_0);
// }
// #endif

// #endif /* CONFIG_UART_SHAKTI_PORT_1 */

DT_INST_FOREACH_STATUS_OKAY(UART_SHAKTI_INIT)