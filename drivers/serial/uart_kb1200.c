/*
 * Copyright (c) 2021, Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This	is not a real serial driver. It	is used	to instantiate struct
 * devices for the "vnd,serial"	devicetree compatible used in test code.
 */

#define	DT_DRV_COMPAT ene_kb1200_uart

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#define	SER_REG_BASE	   ((SER_T *)SER1_BASE)		    /* SER Struct */
#define	GCFG_REG_BASE	   ((GCFG_T*)GCFG_BASE)

struct kb1200_uart_config {
	uintptr_t base_addr;
	uint32_t port_num;
};

struct kb1200_uart_data	{
	uart_irq_callback_user_data_t callback;
	void *callback_data;
	uint8_t	PendingFlag_data;
};

struct kb1200_uart_pins {
	uint16_t tx;
	uint16_t rx;
	uint8_t  pintype;
};

static const struct kb1200_uart_pins uart_pin_cfg[] = {
	{ SER1_TX_GPIO_Num, SER1_RX_GPIO_Num , PINMUX_FUNC_B},   /* SER1 */
	{ SER2_TX_GPIO_Num, SER2_RX_GPIO_Num , PINMUX_FUNC_C},   /* SER2 */
	{ SER3_TX_GPIO_Num, SER3_RX_GPIO_Num , PINMUX_FUNC_B},   /* SER3 */
};
/* UART	Interface (commmon API)	*/
/**********************************************************************************
 * Check whether an error was detected.
 *
 * Function: int uart_err_check	(const struct device *dev)
 *
 * Return: 0	    If no error	was detected.
 *	   err	    Error flags	as defined in uart_rx_stop_reason
 *	   -ENOSYS  If not implemented
 **********************************************************************************/
static int kb1200_uart_err_check(const struct device *dev)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	printk("kb1200_uart_err_check\n");
	int err=0;
	if(ser->SERSTS & SER_RxOverRun)
		err |= UART_ERROR_OVERRUN;
	if(ser->SERSTS & SER_ParityError)
		err |= UART_ERROR_PARITY;
	if(ser->SERSTS & SER_FramingError)
		err |= UART_ERROR_FRAMING;
	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/**********************************************************************************
 * Set UART configuration.
 *
 * Function: int uart_configure	(const struct device *dev, const struct	uart_config *cfg)
 *
 * Return: 0	    If successful.
 *	   -errno   Negative errno code	in case	of failure.
 *	   -ENOSYS  If configuration is	not supported by device	or driver does
 *		     not support setting configuration in runtime.
 *	   -ENOTSUP If API is not enabled
 **********************************************************************************/
static int kb1200_uart_configure(const struct device *dev, const struct	uart_config *cfg)
{
	uint16_t reg_baudrate=0;
	uint8_t	reg_Parity=0;
	int ret=0;
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;

	PINMUX_DEV_T ser_tx_dev = gpio_pinmux(uart_pin_cfg[config->port_num].tx);
	PINMUX_DEV_T ser_rx_dev = gpio_pinmux(uart_pin_cfg[config->port_num].rx);
	gpio_pinmux_set(ser_tx_dev.port, ser_tx_dev.pin, uart_pin_cfg[config->port_num].pintype);
	gpio_pinmux_set(ser_rx_dev.port, ser_rx_dev.pin, PINMUX_FUNC_A);

	reg_baudrate = (24000000/cfg->baudrate)-1;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		reg_Parity = 0x00;
		break;
	case UART_CFG_PARITY_ODD:
		reg_Parity = 0x01;
		break;
	case UART_CFG_PARITY_EVEN:
		reg_Parity = 0x03;
		break;
	case UART_CFG_PARITY_MARK:
	case UART_CFG_PARITY_SPACE:
	default:
		ret = -ENOSYS;
		break;
	}

	switch (cfg->stop_bits)	{
	case UART_CFG_STOP_BITS_1:
		break;
	case UART_CFG_STOP_BITS_0_5:
	case UART_CFG_STOP_BITS_1_5:
	case UART_CFG_STOP_BITS_2:
	default:
		ret = -ENOSYS;
		break;
	}

	switch (cfg->data_bits)	{
	case UART_CFG_DATA_BITS_8:
		break;
	case UART_CFG_DATA_BITS_5:
	case UART_CFG_DATA_BITS_6:
	case UART_CFG_DATA_BITS_7:
	case UART_CFG_DATA_BITS_9:
	default:
		ret = -ENOSYS;
		break;
	}

	switch (cfg->flow_ctrl)	{
	case UART_CFG_FLOW_CTRL_NONE:
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
	case UART_CFG_FLOW_CTRL_DTR_DSR:
	case UART_CFG_FLOW_CTRL_RS485:
	default:
		ret = -ENOSYS;
		break;
	}
	ser->SERCFG = 0;
	ser->SERCFG = (ser->SERCFG&0x00F3)|(reg_baudrate<<16)|(reg_Parity<<2)|0x00000003;
	ser->SERCTRL= 0x01;
	return ret;
}
/**********************************************************************************
 * Get UART configuration.
 *
 * Function: int uart_config_get (const	struct device *dev, struct uart_config *cfg)
 *
 * Return: 0	    If successful.
 *	   -errno   Negative errno code	in case	of failure.
 *	   -ENOSYS  If driver does not support getting current configuration
 *	   -ENOTSUP If API is not enabled
 **********************************************************************************/
static int kb1200_uart_config_get(const	struct device *dev, struct uart_config *cfg)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	cfg->baudrate =	24000000/((ser->SERCFG>>16)+1);
	switch ((ser->SERCFG>>3)&0x03) {
	case 0x00:
		cfg->parity = UART_CFG_PARITY_NONE;
		break;
	case 0x01:
		cfg->parity = UART_CFG_PARITY_ODD;
		break;
	case 0x03:
		cfg->parity = UART_CFG_PARITY_EVEN;
		break;
	default:
		break;
	}
	cfg->stop_bits = UART_CFG_STOP_BITS_1;
	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
    return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
/**********************************************************************************
 * Manipulate line control for UART.
 * Function: int uart_line_ctrl_set (const struct device *dev, uint32_t	ctrl, uint32_t val)
 *
 * Retrieve line control for UART.
 * Function: int uart_line_ctrl_get (const struct device *dev, uint32_t	ctrl, uint32_t val)
 *
 * Send	extra command to driver.
 * Function: int uart_drv_cmd (const struct device *dev, uint32_t cmd, uint32_t	p)
 *
 * Return: 0	    If successful.
 *	   -errno   Negative errno code	in case	of failure.
 *	   -ENOSYS  If this function is	not implemented.
 *	   -ENOTSUP If API is not enabled
 **********************************************************************************/
/*(ene kb1200 function not support)*/

/* Interrupt-driven UART API */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**********************************************************************************
 * Fill	FIFO with data.
 *
 * Function: static int	uart_fifo_fill (const struct device *dev, const	uint8_t	*tx_data, int size)
 *
 * Param:   dev	    UART device	instance.
 *	    tx_data Data to transmit.
 *	    size    Number of bytes to send.
 *
 * Return: tx_bytes Number of bytes sent.
 *	   -ENOTSUP If API is not enabled
 *	   -ENOSYS  if this function is	not supported
 **********************************************************************************/
static int kb1200_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	uint16_t tx_bytes = 0U;
	while((size-tx_bytes)>0)
	{
		/* Check Tx FIFO not Full*/
		while(ser->SERSTS &SER_TxFull);
		/* Put a character into	Tx FIFO	*/
		ser->SERTBUF = tx_data[tx_bytes];
		tx_bytes++;
	}
	return tx_bytes;
}
/**********************************************************************************
 * Read	data from FIFO.
 *
 * Function: static int	uart_fifo_read (const struct device *dev, uint8_t *rx_data, const int size)
 *
 * Param:   dev	    UART device	instance.
 *	    rx_data Data container
 *	    size    Container size.
 *
 * Return: rx_bytes Number of bytes read.
 *	   -ENOTSUP If API is not enabled
 *	   -ENOSYS  if this function is	not supported
 **********************************************************************************/
static int kb1200_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	uint16_t rx_bytes = 0U;
	/* Check Rx FIFO not Empty*/
	while((size-rx_bytes >0)&&(!(ser->SERSTS&SER_RxEmpty)))
	{
		/* Put a character into	Tx FIFO	*/
		rx_data[rx_bytes] = ser->SERRBUF;
		rx_bytes++;
	}
	return rx_bytes;
}

/**********************************************************************************
 * Fill	FIFO with wide data
 * Function: static int	uart_fifo_fill_u16 (const struct device	*dev, const uint16_t *tx_data, int size)
 *
 * Read	wide data from FIFO.
 * Function: static int	uart_fifo_read_u16 (const struct device	*dev, uint16_t *rx_data, const int size)
 *
 * Param:   dev	    UART device	instance.
 *	    tx_data/rx_data Data container
 *	    size    Container size.
 *
 * Return: bytes    Number of bytes.
 *	   -ENOTSUP If API is not enabled
 *	   -ENOSYS  if this function is	not supported
 **********************************************************************************/
/*(ene kb1200 function not support)*/

/**********************************************************************************
 * Enable TX interrupt in IER.
 *
 * Function: void uart_irq_tx_enable (const struct device *dev)
 *
 * Return: none
 **********************************************************************************/
static void kb1200_uart_irq_tx_enable(const struct device *dev)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	ser->SERPF  = 0x0002UL;
	ser->SERIE |= 0x0002UL;
}
/**********************************************************************************
 * Disable TX interrupt	in IER.
 *
 * Function: void uart_irq_tx_disable (const struct device *dev)
 *
 * Return: none
 **********************************************************************************/
static void kb1200_uart_irq_tx_disable(const struct device *dev)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	ser->SERIE &= ~0x0002UL;
	ser->SERPF  = 0x0002UL;
}
/**********************************************************************************
 * Check if UART TX buffer can accept at least one character for transmission (i.e.
 * uart_fifo_fill() will succeed and return non-zero). This function must be called
 * in a	UART interrupt handler,	or its result is undefined. Before calling this	function
 * in the interrupt handler, uart_irq_update() must be called once per the handler invocation
 *
 * Function: static int	uart_irq_tx_ready (const struct	device *dev)
 *
 * Param:   dev	    UART device	instance.
 *
 * Return:  1	    If TX interrupt is enabled and at least one	char can be written to UART.
 *	    0	    If device is not ready to write a new byte.
 *	    -ENOSYS If this function is	not implemented.
 *	    -ENOTSUP If	API is not enabled.
 **********************************************************************************/
static int kb1200_uart_irq_tx_ready(const struct device	*dev)
{
	struct kb1200_uart_data	*data;
	data = dev->data;
	return (data->PendingFlag_data & SER_Tx_Empty)?1:0;
}
/**********************************************************************************
 * Enable RX interrupt in IER.
 *
 * Function: void uart_irq_rx_enable (const struct device *dev)
 *
 * Return: none
 **********************************************************************************/
static void kb1200_uart_irq_rx_enable(const struct device *dev)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	ser->SERPF  = 0x0001UL;
	ser->SERIE |= 0x0001UL;
}
/**********************************************************************************
 * Disable RX interrupt	in IER.
 *
 * Function: void uart_irq_rx_disable (const struct device *dev)
 *
 * Return: none
 **********************************************************************************/
static void kb1200_uart_irq_rx_disable(const struct device *dev)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	ser->SERIE &= (~0x0001UL);
	ser->SERPF  = 0x0001UL;
}
/**********************************************************************************
 * Check if UART TX block finished transmission.
 * Check if any	outgoing data buffered in UART TX block	was fully transmitted and
 * TX block is idle. When this condition is true, UART device (or whole	system)	can
 * be power off. Note that this	function is not	useful to check	if UART	TX can accept
 * more	data, use uart_irq_tx_ready() for that.	This function must be called in	a UART
 * interrupt handler, or its result is undefined. Before calling this function in the
 * interrupt handler, uart_irq_update()	must be	called once per	the handler invocation.
 *
 * Function: static int	uart_irq_tx_complete (const struct device *dev)
 *
 * Param:   dev	    UART device	instance.
 *
 * Return:  1	    If nothing remains to be transmitted.
 *	    0	    If transmission is not completed.
 *	    -ENOSYS If this function is	not implemented.
 *	    -ENOTSUP If	API is not enabled.
 **********************************************************************************/

/**********************************************************************************
 * Check if UART RX buffer has a received char.
 * Check if UART RX buffer has at least	one pending character (i.e. uart_fifo_read()
 * will	succeed	and return non-zero). This function must be called in a	UART interrupt
 * handler, or its result is undefined.	Before calling this function in	the interrupt
 * handler, uart_irq_update() must be called once per the handler invocation.
 *    It's unspecified whether condition as returned by	this function is level-	or edge-
 * triggered (i.e. if this function returns true when RX FIFO is non-empty, or when a
 * new char was	received since last call to it). See description of uart_fifo_read()
 * for implication of this.
 *
 * Function: static int	uart_irq_tx_ready (const struct	device *dev)
 *
 * Param:   dev	    UART device	instance.
 *
 * Return:  1	    If a received char is ready.
 *	    0	    If a received char is not ready.
 *	   -ENOSYS If this function is not implemented.
 *	   -ENOTSUP If API is not enabled.
 **********************************************************************************/
static int kb1200_uart_irq_rx_ready(const struct device	*dev)
{
	struct kb1200_uart_data	*data;
	data = dev->data;
	return (data->PendingFlag_data & SER_Rx_CNT_Full)?1:0;
}
/**********************************************************************************
 * Enable error	interrupt.
 *
 * Function: void uart_irq_err_enable (const struct device *dev)
 *
 * Return: none
 **********************************************************************************/
static void kb1200_uart_irq_err_enable(const struct device *dev)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	ser->SERPF  = 0x0004UL;
	ser->SERIE |= (0x0004UL);
}

/**********************************************************************************
 * Disable error interrupt.
 *
 * Function: void uart_irq_err_disable (const struct device *dev)
 *
 * Return: none
 **********************************************************************************/
static void kb1200_uart_irq_err_disable(const struct device *dev)
{
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	ser->SERIE &= (~0x0004UL);
	ser->SERPF  = 0x0004UL;
}

/**********************************************************************************
 * Check if any	IRQs is	pending.
 *
 * Function: int uart_irq_is_pending (const struct device *dev)
 *
 * Param:   dev	    UART device	instance.
 *
 * Return:  1	    If an IRQ is pending.
 *	    0	    If an IRQ is not pending.
 *	    -ENOSYS If this function is	not implemented.
 *	    -ENOTSUP If	API is not enabled.
 **********************************************************************************/
static int kb1200_uart_irq_is_pending(const struct device *dev)
{
	struct kb1200_uart_data	*data;
	data = dev->data;
	return (data->PendingFlag_data)?1:0;
}

/**********************************************************************************
 * Start processing interrupts in ISR.
 *
 * This	function should	be called the first thing in the ISR. Calling uart_irq_rx_ready(),
 * uart_irq_tx_ready(),	uart_irq_tx_complete() allowed only after this.
 *
 * The purpose of this function	is:
 *
 * For devices with auto-acknowledge of	interrupt status on register read to cache
 * the value of	this register (rx_ready, etc. then use this case).
 * For devices with explicit acknowledgment of interrupts, to ack any pending interrupts
 * and likewise	to cache the original value.
 * For devices with implicit acknowledgment, this function will	be empty. But the ISR
 * must	perform	the actions needs to ack the interrupts	(usually, call uart_fifo_read()
 * on rx_ready,	and uart_fifo_fill() on	tx_ready).
 *
 * Function: int uart_irq_update (const	struct device *dev)
 *
 * Param:   dev	    UART device	instance.
 *
 * Return:  1	    On success
 *	    -ENOSYS If this function is	not implemented.
 *	    -ENOTSUP If	API is not enabled.
 **********************************************************************************/
static int kb1200_uart_irq_update(const	struct device *dev)
{
	struct kb1200_uart_data	*data;
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	data = dev->data;
	data->PendingFlag_data = (ser->SERPF)&(ser->SERIE);
	/*clear	pending	flag*/
	ser->SERPF = data->PendingFlag_data;
	return 1;
}
/**********************************************************************************
 * Set the IRQ callback	function pointer.
 *
 * Function: static int	uart_irq_callback_user_data_set	(const struct device *dev, uart_irq_callback_user_data_t cb, void *user_data)
 *
 * Param:   dev	    UART device	instance.
 *	    cb	    Pointer to the callback function.
 *	    user_data Data to pass to callback function.
 *
 * Return:  0	    On success
 *	    -ENOSYS If this function is	not implemented.
 *	    -ENOTSUP If	API is not enabled.
 **********************************************************************************/
static void kb1200_uart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct kb1200_uart_data	*data;
	data = dev->data;
	data->callback = cb;
	data->callback_data = cb_data;
}
/**********************************************************************************
 * Set the IRQ callback	function pointer(legacy).
 *
 * Function: static int	uart_irq_callback_set (const struct device *dev,
 *					       uart_irq_callback_user_data_t cb)
 *
 * Param:   dev	    UART device	instance.
 *	    cb	    Pointer to the callback function.
 *
 * Return:  0	    On success
 *	    -ENOSYS If this function is	not implemented.
 *	    -ENOTSUP If	API is not enabled.
 **********************************************************************************/
 /*(ene	kb1200 function	not support)*/

/**********************************************************************************
 * Process interrupt event.
 *
 * Function: static void kb1200_uart_irq_handler(const struct device *dev)
 *
 * Return: none
 **********************************************************************************/
static void kb1200_uart_irq_handler(const struct device	*dev)
{
	struct kb1200_uart_data	*data =	dev->data;
	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
}
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN	*/


/* Polling UART	API */
/**********************************************************************************
 * Read	a character from the device for	input.
 *
 * Function: int uart_poll_in (const struct device *dev, unsigned char *c)
 *
 * Param:   dev	    UART device	instance.
 *	    c	    Pointer to character.
 *
 * Return:  0	    If a character arrived
 *	    -1	    If no character was	available to read (i.e.	the UART input buffer was empty).
 *	    -ENOSYS If this function is	not implemented.
 *	    -EBUSY  If async reception was enabled using uart_rx_enable
 **********************************************************************************/
static int kb1200_uart_poll_in(const struct device *dev, unsigned char *c)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	return kb1200_uart_fifo_read(dev, c, 1)? 0 : -1;
#else
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	/* Check Rx FIFO not Empty*/
	if(ser->SERSTS&SER_RxEmpty)
		return -1;
	/* Put a character into	Tx FIFO	*/
	*c = ser->SERRBUF;
	return 0;
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN	*/
}
/**********************************************************************************
 * Write a character to	the device for output.
 * This	routine	checks if the transmitter is full. When	the transmitter	is not full,
 * it writes a character to the	data register. It waits	and blocks the calling thread,
 * otherwise. This function is a blocking call.
 *
 * To send a character when hardware flow control is enabled, the handshake signal
 * CTS must be asserted
 *
 * Function: void uart_poll_out(const struct device *dev, unsigned char	*c)
 *
 * Param:   dev	    UART device	instance.
 *	    c	    Pointer to character.
 *
 * Return:  none
 **********************************************************************************/
static void kb1200_uart_poll_out(const struct device *dev, unsigned char c)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	kb1200_uart_fifo_fill(dev, &c, 1);
#else
	const struct kb1200_uart_config *config = dev->config;
	SER_T* ser = (SER_T *)config->base_addr;
	/* Wait	Tx FIFO	not Full*/
	while(ser->SERSTS &SER_TxFull);
	/* Put a character into	Tx FIFO	*/
	ser->SERTBUF = c;
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN	*/
}
/**********************************************************************************
 * Read	a 16-bit dat from the device for input.
 * Function: int uart_poll_in_u16 (const struct	device *dev, uint16_t *p_u16)
 *
 * Write a 16-bit datum	to the device for output.
 * Function: void uart_poll_out_u16 (const struct device *dev, uint16_t	out_u16)
 **********************************************************************************/
/*(ene kb1200 function not support)*/



static const struct uart_driver_api kb1200_uart_api = {
	.poll_in	    = kb1200_uart_poll_in,
	.poll_out	    = kb1200_uart_poll_out,
	.err_check	    = kb1200_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure	    = kb1200_uart_configure,
	.config_get	    = kb1200_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill	    = kb1200_uart_fifo_fill,
	.fifo_read	    = kb1200_uart_fifo_read,
	.irq_tx_enable	    = kb1200_uart_irq_tx_enable,
	.irq_tx_disable	    = kb1200_uart_irq_tx_disable,
	.irq_tx_ready	    = kb1200_uart_irq_tx_ready,
	.irq_rx_enable	    = kb1200_uart_irq_rx_enable,
	.irq_rx_disable	    = kb1200_uart_irq_rx_disable,
	.irq_rx_ready	    = kb1200_uart_irq_rx_ready,
	.irq_err_enable	    = kb1200_uart_irq_err_enable,
	.irq_err_disable    = kb1200_uart_irq_err_disable,
	.irq_is_pending	    = kb1200_uart_irq_is_pending,
	.irq_update	    = kb1200_uart_irq_update,
	.irq_callback_set   = kb1200_uart_irq_callback_set
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define MAX_SERIAL_PORT	3
static struct device* ser_device[MAX_SERIAL_PORT] = { NULL };
static int ser_device_count = 0;
static void kb1200_uart_isr_wrap(const struct device *dev)
{
	for (int i = 0; i < ser_device_count; i++)
	{
		struct device* dev = ser_device[i];
		const struct kb1200_uart_config *config = dev->config;
		SER_T* ser = (SER_T *)config->base_addr;
		if (ser->SERIE & ser->SERPF)
			kb1200_uart_irq_handler(dev);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN*/

static int kb1200_uart_init(const struct device	*dev)
{
	struct uart_config kb1200_uart_cfg = {
		.baudrate = 115200,
		.parity	= UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};
	/*SER1*/
	kb1200_uart_configure(dev, &kb1200_uart_cfg);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	ser_device[ser_device_count] = (struct device *)dev;
	ser_device_count++;
	static bool irq_connected = false;
	if (!irq_connected)
	{
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
			    kb1200_uart_isr_wrap, DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQN(0));
		irq_connected = true;
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN*/

	return 0;
}

#define	KB1200_UART_DATA_BUFFER(n)								   \
	RING_BUF_DECLARE(written_data_##n, DT_INST_PROP(n, buffer_size));			   \
	RING_BUF_DECLARE(read_queue_##n, DT_INST_PROP(n, buffer_size));				   \
	static struct kb1200_uart_data kb1200_uart_data_##n = {					   \
		.written = &written_data_##n,							   \
		.read_queue = &read_queue_##n,							   \
	};
#define	KB1200_UART_DATA(n) static struct kb1200_uart_data kb1200_uart_data_##n	= {};
#define	KB1200_UART_INIT(n)									    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, buffer_size), (KB1200_UART_DATA_BUFFER(n)),	    \
		   (KB1200_UART_DATA(n)))							    \
	static const struct kb1200_uart_config kb1200_uart_config_##n = {	\
		.base_addr = (uintptr_t)DT_INST_REG_ADDR(n),				\
		.port_num = (uint32_t)DT_INST_PROP(n, port_num),		\
	};								\
	DEVICE_DT_INST_DEFINE(n, &kb1200_uart_init, NULL, \
			&kb1200_uart_data_##n, &kb1200_uart_config_##n, \
			PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &kb1200_uart_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_UART_INIT)
