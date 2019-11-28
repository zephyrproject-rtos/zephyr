/* uart_xlnx_ps.c - Xilinx Zynq family serial driver */

/*
 * Copyright (c) 2018 Xilinx, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Xilnx Zynq Family Serial Driver
 *
 * This is the driver for the Xilinx Zynq family cadence serial device.
 *
 * Before individual UART port can be used, uart_xlnx_ps_init() has to be
 * called to setup the port.
 *
 * - the following macro for the number of bytes between register addresses:
 *
 *  UART_REG_ADDR_INTERVAL
 */


#include <errno.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <zephyr/types.h>
#include <soc.h>

#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <uart.h>
#include <sys_io.h>

#define XUARTPS_CR_OFFSET	0x0000U  /**< Control Register [8:0] */
#define XUARTPS_MR_OFFSET	0x0004U  /**< Mode Register [9:0] */
#define XUARTPS_IER_OFFSET	0x0008U  /**< Interrupt Enable [12:0] */
#define XUARTPS_IDR_OFFSET	0x000CU  /**< Interrupt Disable [12:0] */
#define XUARTPS_IMR_OFFSET	0x0010U  /**< Interrupt Mask [12:0] */
#define XUARTPS_ISR_OFFSET	0x0014U  /**< Interrupt Status [12:0]*/
#define XUARTPS_BAUDGEN_OFFSET	0x0018U  /**< Baud Rate Generator [15:0] */
#define XUARTPS_RXTOUT_OFFSET	0x001CU  /**< RX Timeout [7:0] */
#define XUARTPS_RXWM_OFFSET	0x0020U  /**< RX FIFO Trigger Level [5:0] */
#define XUARTPS_MODEMCR_OFFSET	0x0024U  /**< Modem Control [5:0] */
#define XUARTPS_MODEMSR_OFFSET	0x0028U  /**< Modem Status [8:0] */
#define XUARTPS_SR_OFFSET	0x002CU  /**< Channel Status [14:0] */
#define XUARTPS_FIFO_OFFSET	0x0030U  /**< FIFO [7:0] */
#define XUARTPS_BAUDDIV_OFFSET	0x0034U  /**< Baud Rate Divider [7:0] */
#define XUARTPS_FLOWDEL_OFFSET	0x0038U  /**< Flow Delay [5:0] */
#define XUARTPS_TXWM_OFFSET	0x0044U  /**< TX FIFO Trigger Level [5:0] */
#define XUARTPS_RXBS_OFFSET	0x0048U  /**< RX FIFO Byte Status [11:0] */

/* Control Register Bits Definition */
#define XUARTPS_CR_STOPBRK	0x00000100U  /**< Stop transmission of break */
#define XUARTPS_CR_STARTBRK	0x00000080U  /**< Set break */
#define XUARTPS_CR_TORST	0x00000040U  /**< RX timeout counter restart */
#define XUARTPS_CR_TX_DIS	0x00000020U  /**< TX disabled. */
#define XUARTPS_CR_TX_EN	0x00000010U  /**< TX enabled */
#define XUARTPS_CR_RX_DIS	0x00000008U  /**< RX disabled. */
#define XUARTPS_CR_RX_EN	0x00000004U  /**< RX enabled */
#define XUARTPS_CR_EN_DIS_MASK	0x0000003CU  /**< Enable/disable Mask */
#define XUARTPS_CR_TXRST	0x00000002U  /**< TX logic reset */
#define XUARTPS_CR_RXRST	0x00000001U  /**< RX logic reset */

/* Mode Register Bits Definition */
#define XUARTPS_MR_CCLK			0x00000400U /**< Input clock select */
#define XUARTPS_MR_CHMODE_R_LOOP	0x00000300U /**< Remote loopback mode */
#define XUARTPS_MR_CHMODE_L_LOOP	0x00000200U /**< Local loopback mode */
#define XUARTPS_MR_CHMODE_ECHO		0x00000100U /**< Auto echo mode */
#define XUARTPS_MR_CHMODE_NORM		0x00000000U /**< Normal mode */
#define XUARTPS_MR_CHMODE_SHIFT		8U  /**< Mode shift */
#define XUARTPS_MR_CHMODE_MASK		0x00000300U /**< Mode mask */
#define XUARTPS_MR_STOPMODE_2_BIT	0x00000080U /**< 2 stop bits */
#define XUARTPS_MR_STOPMODE_1_5_BIT	0x00000040U /**< 1.5 stop bits */
#define XUARTPS_MR_STOPMODE_1_BIT	0x00000000U /**< 1 stop bit */
#define XUARTPS_MR_STOPMODE_SHIFT	6U  /**< Stop bits shift */
#define XUARTPS_MR_STOPMODE_MASK	0x000000A0U /**< Stop bits mask */
#define XUARTPS_MR_PARITY_NONE		0x00000020U /**< No parity mode */
#define XUARTPS_MR_PARITY_MARK		0x00000018U /**< Mark parity mode */
#define XUARTPS_MR_PARITY_SPACE		0x00000010U /**< Space parity mode */
#define XUARTPS_MR_PARITY_ODD		0x00000008U /**< Odd parity mode */
#define XUARTPS_MR_PARITY_EVEN		0x00000000U /**< Even parity mode */
#define XUARTPS_MR_PARITY_SHIFT		3U  /**< Parity setting shift */
#define XUARTPS_MR_PARITY_MASK		0x00000038U /**< Parity mask */
#define XUARTPS_MR_CHARLEN_6_BIT	0x00000006U /**< 6 bits data */
#define XUARTPS_MR_CHARLEN_7_BIT	0x00000004U /**< 7 bits data */
#define XUARTPS_MR_CHARLEN_8_BIT	0x00000000U /**< 8 bits data */
#define XUARTPS_MR_CHARLEN_SHIFT	1U  /**< Data Length shift */
#define XUARTPS_MR_CHARLEN_MASK		0x00000006U /**< Data length mask */
#define XUARTPS_MR_CLKSEL		0x00000001U /**< Input clock select */

/* Interrupt Register Bits Definition */
#define XUARTPS_IXR_RBRK	0x00002000U /**< Rx FIFO break detect interrupt */
#define XUARTPS_IXR_TOVR	0x00001000U /**< Tx FIFO Overflow interrupt */
#define XUARTPS_IXR_TNFUL	0x00000800U /**< Tx FIFO Nearly Full interrupt */
#define XUARTPS_IXR_TTRIG	0x00000400U /**< Tx Trig interrupt */
#define XUARTPS_IXR_DMS		0x00000200U /**< Modem status change interrupt */
#define XUARTPS_IXR_TOUT	0x00000100U /**< Timeout error interrupt */
#define XUARTPS_IXR_PARITY	0x00000080U /**< Parity error interrupt */
#define XUARTPS_IXR_FRAMING	0x00000040U /**< Framing error interrupt */
#define XUARTPS_IXR_RXOVR	0x00000020U /**< Overrun error interrupt */
#define XUARTPS_IXR_TXFULL	0x00000010U /**< TX FIFO full interrupt. */
#define XUARTPS_IXR_TXEMPTY	0x00000008U /**< TX FIFO empty interrupt. */
#define XUARTPS_IXR_RXFULL	0x00000004U /**< RX FIFO full interrupt. */
#define XUARTPS_IXR_RXEMPTY	0x00000002U /**< RX FIFO empty interrupt. */
#define XUARTPS_IXR_RTRIG	0x00000001U /**< RX FIFO trigger interrupt. */
#define XUARTPS_IXR_MASK	0x00003FFFU /**< Valid bit mask */

/* Channel Status Register */
#define XUARTPS_SR_TNFUL	0x00004000U /**< TX FIFO Nearly Full Status */
#define XUARTPS_SR_TTRIG	0x00002000U /**< TX FIFO Trigger Status */
#define XUARTPS_SR_FLOWDEL	0x00001000U /**< RX FIFO fill over flow delay */
#define XUARTPS_SR_TACTIVE	0x00000800U /**< TX active */
#define XUARTPS_SR_RACTIVE	0x00000400U /**< RX active */
#define XUARTPS_SR_TXFULL	0x00000010U /**< TX FIFO full */
#define XUARTPS_SR_TXEMPTY	0x00000008U /**< TX FIFO empty */
#define XUARTPS_SR_RXFULL	0x00000004U /**< RX FIFO full */
#define XUARTPS_SR_RXEMPTY	0x00000002U /**< RX FIFO empty */
#define XUARTPS_SR_RTRIG	0x00000001U /**< RX FIFO fill over trigger */

struct uart_xlnx_ps_dev_config {
	struct uart_device_config uconf;
	u32_t baud_rate;
};

/** Device data structure */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
struct uart_xlnx_ps_dev_data_t {
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
};
#endif

#define DEV_CFG(dev) \
	((const struct uart_xlnx_ps_dev_config * const) \
	 (dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_xlnx_ps_dev_data_t *)(dev)->driver_data)

static const struct uart_driver_api uart_xlnx_ps_driver_api;

static void xlnx_ps_disable_uart(u32_t reg_base)
{
	u32_t regval;

	regval = sys_read32(reg_base + XUARTPS_CR_OFFSET);
	regval &= (~XUARTPS_CR_EN_DIS_MASK);
	regval |= XUARTPS_CR_TX_DIS | XUARTPS_CR_RX_DIS;
	sys_write32(regval, reg_base + XUARTPS_CR_OFFSET);
}

static void xlnx_ps_enable_uart(u32_t reg_base)
{
	u32_t regval;

	regval = sys_read32(reg_base + XUARTPS_CR_OFFSET);
	regval &= (~XUARTPS_CR_EN_DIS_MASK);
	regval |= XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN;
	sys_write32(regval, reg_base + XUARTPS_CR_OFFSET);
}

static void set_baudrate(struct device *dev, u32_t baud_rate)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t divisor, generator;
	u32_t baud;
	u32_t clk_freq;
	u32_t reg_base;

	baud = dev_cfg->baud_rate;
	clk_freq = dev_cfg->uconf.sys_clk_freq;

	/* Calculate divisor and baud rate generator value */
	if ((baud != 0) && (clk_freq != 0)) {
		/* Covering case where input clock is so slow */
		if (clk_freq < 1000000U && baud > 4800U) {
			baud = 4800;
		}

		for (divisor = 4; divisor < 255; divisor++) {
			u32_t tmpbaud, bauderr;

			generator = clk_freq / (baud * (divisor + 1));
			if (generator < 2 || generator > 65535) {
				continue;
			}
			tmpbaud = clk_freq / (generator * (divisor + 1));

			if (baud > tmpbaud) {
				bauderr = baud - tmpbaud;
			} else {
				bauderr = tmpbaud - baud;
			}
			if (((bauderr * 100) / baud) < 3) {
				break;
			}
		}

		/* Disable uart before changing baud rate */
		reg_base = dev_cfg->uconf.regs;
		xlnx_ps_disable_uart(reg_base);

		/* Set baud rate divisor and generator */
		sys_write32(divisor, reg_base + XUARTPS_BAUDDIV_OFFSET);
		sys_write32(generator, reg_base + XUARTPS_BAUDGEN_OFFSET);
		xlnx_ps_enable_uart(reg_base);
	}
}

/**
 * @brief Initialize individual UART port
 *
 * This routine is called to reset the chip in a quiescent state.
 *
 * @param dev UART device struct
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_xlnx_ps_init(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_val;
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	xlnx_ps_disable_uart(reg_base);

	/* Set mode */
	reg_val = sys_read32(reg_base + XUARTPS_MR_OFFSET);
	reg_val &= (~(XUARTPS_MR_CHARLEN_MASK | XUARTPS_MR_STOPMODE_MASK |
		    XUARTPS_MR_PARITY_MASK));
	reg_val |= XUARTPS_MR_CHARLEN_8_BIT | XUARTPS_MR_STOPMODE_1_BIT |
		   XUARTPS_MR_PARITY_NONE;
	sys_write32(reg_val, reg_base + XUARTPS_MR_OFFSET);

	/* Set RX FIFO trigger at 8 data bytes. */
	sys_write32(0x08U, reg_base + XUARTPS_RXWM_OFFSET);

	/* Set RX timeout to 1, which will be 4 character time */
	sys_write32(0x1U, reg_base + XUARTPS_RXTOUT_OFFSET);

	/* Disable all interrupts, polling mode is default */
	sys_write32(XUARTPS_IXR_MASK, reg_base + XUARTPS_IDR_OFFSET);

	set_baudrate(dev, dev_cfg->baud_rate);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->uconf.irq_config_func(dev);
#endif

	xlnx_ps_enable_uart(reg_base);

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
static int uart_xlnx_ps_poll_in(struct device *dev, unsigned char *c)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_val;
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	if ((reg_val & XUARTPS_SR_RXEMPTY) == 0) {
		*c = (unsigned char)sys_read32(reg_base +
						XUARTPS_FIFO_OFFSET);
		return 0;
	} else {
		return -1;
	}
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
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static void uart_xlnx_ps_poll_out(struct device *dev, unsigned char c)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_val;
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	/* wait for transmitter to ready to accept a character */
	do {
		reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	} while ((reg_val & XUARTPS_SR_TXEMPTY) == 0);

	sys_write32((u32_t)(c & 0xFF), reg_base + XUARTPS_FIFO_OFFSET);

	do {
		reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	} while ((reg_val & XUARTPS_SR_TXEMPTY) == 0);
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_xlnx_ps_fifo_fill(struct device *dev, const u8_t *tx_data,
				  int size)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_val;
	u32_t reg_base;
	int onum = 0;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	while (onum < size && (reg_val & XUARTPS_SR_TXFULL) == 0) {
		sys_write32((u32_t)(tx_data[onum] & 0xFF),
				reg_base + XUARTPS_FIFO_OFFSET);
		onum++;
		reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	}

	return onum;
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
static int uart_xlnx_ps_fifo_read(struct device *dev, u8_t *rx_data,
				  const int size)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_val;
	u32_t reg_base;
	int inum = 0;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);

	while (inum < size && (reg_val & XUARTPS_SR_RXEMPTY) == 0) {
		rx_data[inum] = (u8_t)sys_read32(reg_base
				+ XUARTPS_FIFO_OFFSET);
		inum++;
		reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	}

	return inum;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_xlnx_ps_irq_tx_enable(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(XUARTPS_IXR_TTRIG, reg_base + XUARTPS_IER_OFFSET);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_xlnx_ps_irq_tx_disable(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(XUARTPS_IXR_TTRIG, reg_base + XUARTPS_IDR_OFFSET);
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xlnx_ps_irq_tx_ready(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;
	u32_t reg_val;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_ISR_OFFSET);
	if ((reg_val & XUARTPS_IXR_TTRIG) == 0) {
		return 0;
	} else {
		return 1;
	}
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_xlnx_ps_irq_tx_complete(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;
	u32_t reg_val;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	if ((reg_val & XUARTPS_SR_TXEMPTY) == 0) {
		return 0;
	} else {
		return 1;
	}
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_xlnx_ps_irq_rx_enable(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(XUARTPS_IXR_RTRIG, reg_base + XUARTPS_IER_OFFSET);
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_xlnx_ps_irq_rx_disable(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(XUARTPS_IXR_RTRIG, reg_base + XUARTPS_IDR_OFFSET);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xlnx_ps_irq_rx_ready(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;
	u32_t reg_val;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_ISR_OFFSET);
	if ((reg_val & XUARTPS_IXR_RTRIG) == 0) {
		return 0;
	} else {
		return 1;
	}
}

/**
 * @brief Enable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_xlnx_ps_irq_err_enable(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(XUARTPS_IXR_PARITY | XUARTPS_IXR_FRAMING,
		    reg_base + XUARTPS_IER_OFFSET);
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static void uart_xlnx_ps_irq_err_disable(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(XUARTPS_IXR_PARITY | XUARTPS_IXR_FRAMING,
		    reg_base + XUARTPS_IDR_OFFSET);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_xlnx_ps_irq_is_pending(struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	u32_t reg_base;
	u32_t reg_imr;
	u32_t reg_isr;

	reg_base = dev_cfg->uconf.regs;
	reg_imr = sys_read32(reg_base + XUARTPS_IMR_OFFSET);
	reg_isr = sys_read32(reg_base + XUARTPS_ISR_OFFSET);

	if ((reg_imr & reg_isr) != 0) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct
 *
 * @return Always 1
 */
static int uart_xlnx_ps_irq_update(struct device *dev)
{
	(void)dev;
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
static void uart_xlnx_ps_irq_callback_set(struct device *dev,
					    uart_irq_callback_user_data_t cb,
					    void *cb_data)
{
	struct uart_xlnx_ps_dev_data_t *dev_data = DEV_DATA(dev);

	dev_data->user_cb = cb;
	dev_data->user_data = cb_data;
}

/**
 * @brief Interrupt ce routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void uart_xlnx_ps_isr(void *arg)
{
	struct device *dev = arg;
	const struct uart_xlnx_ps_dev_data_t *data = DEV_DATA(dev);

	if (data->user_cb) {
		data->user_cb(data->user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_xlnx_ps_driver_api = {
	.poll_in = uart_xlnx_ps_poll_in,
	.poll_out = uart_xlnx_ps_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = uart_xlnx_ps_fifo_fill,
	.fifo_read = uart_xlnx_ps_fifo_read,
	.irq_tx_enable = uart_xlnx_ps_irq_tx_enable,
	.irq_tx_disable = uart_xlnx_ps_irq_tx_disable,
	.irq_tx_ready = uart_xlnx_ps_irq_tx_ready,
	.irq_tx_complete = uart_xlnx_ps_irq_tx_complete,
	.irq_rx_enable = uart_xlnx_ps_irq_rx_enable,
	.irq_rx_disable = uart_xlnx_ps_irq_rx_disable,
	.irq_rx_ready = uart_xlnx_ps_irq_rx_ready,
	.irq_err_enable = uart_xlnx_ps_irq_err_enable,
	.irq_err_disable = uart_xlnx_ps_irq_err_disable,
	.irq_is_pending = uart_xlnx_ps_irq_is_pending,
	.irq_update = uart_xlnx_ps_irq_update,
	.irq_callback_set = uart_xlnx_ps_irq_callback_set,

#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define UART_XLNX_PS_IRQ_CONF_FUNC_SET(port) \
	.irq_config_func = uart_xlnx_ps_irq_config_##port,

#define UART_XLNX_PS_IRQ_CONF_FUNC(port) \
DEVICE_DECLARE(uart_xlnx_ps_##port); \
\
static void uart_xlnx_ps_irq_config_0(struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_##port##_XLNX_XUARTPS_IRQ_0, \
		    DT_INST_##port##_XLNX_XUARTPS_IRQ_0_PRIORITY, \
		    uart_xlnx_ps_isr, DEVICE_GET(uart_xlnx_ps_##port), \
		    0); \
	irq_enable(DT_INST_##port##_XLNX_XUARTPS_IRQ_0); \
}

#define UART_XLNX_PS_DEV_DATA(port) \
static struct uart_xlnx_ps_dev_data_t uart_xlnx_ps_dev_data_##port

#else

#define UART_XLNX_PS_IRQ_CONF_FUNC_SET(port)
#define UART_XLNX_PS_IRQ_CONF_FUNC(port)
#define UART_XLNX_PS_DEV_DATA(port)

#endif /*CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_XLNX_PS_DEV_CFG(port) \
static struct uart_xlnx_ps_dev_config uart_xlnx_ps_dev_cfg_##port = { \
	.uconf = { \
		.regs = DT_INST_##port##_XLNX_XUARTPS_BASE_ADDRESS, \
		.base = (u8_t *)DT_INST_##port##_XLNX_XUARTPS_BASE_ADDRESS, \
		.sys_clk_freq = DT_INST_##port##_XLNX_XUARTPS_CLOCK_FREQUENCY, \
		UART_XLNX_PS_IRQ_CONF_FUNC_SET(port) \
	}, \
	.baud_rate = DT_INST_##port##_XLNX_XUARTPS_CURRENT_SPEED, \
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_XLNX_PS_INIT(port) \
DEVICE_AND_API_INIT(uart_xlnx_ps_##port, DT_INST_##port##_XLNX_XUARTPS_LABEL, \
		    uart_xlnx_ps_init, \
		    &uart_xlnx_ps_dev_data_##port, \
		    &uart_xlnx_ps_dev_cfg_##port, \
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
		    &uart_xlnx_ps_driver_api)
#else
#define UART_XLNX_PS_INIT(port) \
DEVICE_AND_API_INIT(uart_xlnx_ps_##port, DT_INST_##port##_XLNX_XUARTPS_LABEL, \
		    uart_xlnx_ps_init, \
		    NULL, \
		    &uart_xlnx_ps_dev_cfg_##port, \
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
		    &uart_xlnx_ps_driver_api)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef DT_INST_0_XLNX_XUARTPS_BASE_ADDRESS
UART_XLNX_PS_IRQ_CONF_FUNC(0);
UART_XLNX_PS_DEV_DATA(0);
UART_XLNX_PS_DEV_CFG(0);
UART_XLNX_PS_INIT(0);
#endif

#ifdef DT_INST_1_XLNX_XUARTPS_BASE_ADDRESS
UART_XLNX_PS_IRQ_CONF_FUNC(1);
UART_XLNX_PS_DEV_DATA(1);
UART_XLNX_PS_DEV_CFG(1);
UART_XLNX_PS_INIT(1);
#endif


