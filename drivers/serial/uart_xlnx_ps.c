/* uart_xlnx_ps.c - Xilinx Zynq family serial driver */

/*
 * Copyright (c) 2018 Xilinx, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xuartps

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
#include <drivers/uart.h>
#include <sys/sys_io.h>

/* For all register offsets and bits / bit masks:
 * Comp. Xilinx Zynq-7000 Technical Reference Manual (ug585), chap. B.33
 */

/* Register offsets within the UART device's register space */
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

/* Modem Control Register Bits Definition */
#define XUARTPS_MODEMCR_FCM_RTS_CTS	0x00000020 /**< RTS/CTS hardware flow control. */
#define XUARTPS_MODEMCR_FCM_NONE	0x00000000 /**< No hardware flow control. */
#define XUARTPS_MODEMCR_FCM_MASK	0x00000020 /**< Hardware flow control mask. */
#define XUARTPS_MODEMCR_RTS_SHIFT	1U         /**< RTS bit shift */
#define XUARTPS_MODEMCR_DTR_SHIFT	0U         /**< DTR bit shift */

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

/** Device configuration structure */
struct uart_xlnx_ps_dev_config {
	struct uart_device_config uconf;
	uint32_t baud_rate;
};

/** Device data structure */
struct uart_xlnx_ps_dev_data_t {
	uint32_t parity;
	uint32_t stopbits;
	uint32_t databits;
	uint32_t flowctrl;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
};

#define DEV_CFG(dev) \
	((const struct uart_xlnx_ps_dev_config * const) \
	 (dev)->config)
#define DEV_DATA(dev) \
	((struct uart_xlnx_ps_dev_data_t *)(dev)->data)

static const struct uart_driver_api uart_xlnx_ps_driver_api;

/**
 * @brief Disables the UART's RX and TX function.
 *
 * Writes 'Disable RX' and 'Disable TX' command bits into the respective
 * UART's Command Register, thus disabling the operation of the UART.
 *
 * While writing the disable command bits, the opposing enable command
 * bits, which are set when enabling the UART, are cleared.
 *
 * This function must be called before any configuration parameters
 * of the UART are modified at run-time.
 *
 * @param reg_base Base address of the respective UART's register space.
 */
static void xlnx_ps_disable_uart(uint32_t reg_base)
{
	uint32_t regval;

	regval = sys_read32(reg_base + XUARTPS_CR_OFFSET);
	regval &= (~XUARTPS_CR_EN_DIS_MASK);
	/* Set control register bits [5]: TX_DIS and [3]: RX_DIS */
	regval |= XUARTPS_CR_TX_DIS | XUARTPS_CR_RX_DIS;
	sys_write32(regval, reg_base + XUARTPS_CR_OFFSET);
}

/**
 * @brief Enables the UART's RX and TX function.
 *
 * Writes 'Enable RX' and 'Enable TX' command bits into the respective
 * UART's Command Register, thus enabling the operation of the UART.
 *
 * While writing the enable command bits, the opposing disable command
 * bits, which are set when disabling the UART, are cleared.
 *
 * This function must not be called while any configuration parameters
 * of the UART are being modified at run-time.
 *
 * @param reg_base Base address of the respective UART's register space.
 */
static void xlnx_ps_enable_uart(uint32_t reg_base)
{
	uint32_t regval;

	regval = sys_read32(reg_base + XUARTPS_CR_OFFSET);
	regval &= (~XUARTPS_CR_EN_DIS_MASK);
	/* Set control register bits [4]: TX_EN and [2]: RX_EN */
	regval |= XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN;
	sys_write32(regval, reg_base + XUARTPS_CR_OFFSET);
}

/**
 * @brief Calculates and sets the values of the BAUDDIV and BAUDGEN registers.
 *
 * Calculates and sets the values of the BAUDDIV and BAUDGEN registers, which
 * determine the prescaler applied to the clock driving the UART, based on
 * the target baud rate, which is provided as a decimal value.
 *
 * The calculation of the values to be written to the BAUDDIV and BAUDGEN
 * registers is described in the Zynq-7000 TRM, chapter 19.2.3 'Baud Rate
 * Generator'.
 *
 * @param dev UART device struct
 * @param baud_rate The desired baud rate as a decimal value
 */
static void set_baudrate(const struct device *dev, uint32_t baud_rate)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t divisor, generator;
	uint32_t baud;
	uint32_t clk_freq;
	uint32_t reg_base;

	baud = dev_cfg->baud_rate;
	clk_freq = dev_cfg->uconf.sys_clk_freq;

	/* Calculate divisor and baud rate generator value */
	if ((baud != 0) && (clk_freq != 0)) {
		/* Covering case where input clock is so slow */
		if (clk_freq < 1000000U && baud > 4800U) {
			baud = 4800;
		}

		for (divisor = 4; divisor < 255; divisor++) {
			uint32_t tmpbaud, bauderr;

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

		/*
		 * Set baud rate divisor and generator.
		 * -> This function is always called from a context in which
		 * the receiver/transmitter is disabled, the baud rate can
		 * be changed safely at this time.
		 */

		reg_base = dev_cfg->uconf.regs;
		sys_write32(divisor, reg_base + XUARTPS_BAUDDIV_OFFSET);
		sys_write32(generator, reg_base + XUARTPS_BAUDGEN_OFFSET);
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
static int uart_xlnx_ps_init(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_val;
	uint32_t reg_base;

	reg_base = dev_cfg->uconf.regs;

	/* Disable RX/TX before changing any configuration data */
	xlnx_ps_disable_uart(reg_base);

	/* Set initial character length / start/stop bit / parity configuration */
	reg_val = sys_read32(reg_base + XUARTPS_MR_OFFSET);
	reg_val &= (~(XUARTPS_MR_CHARLEN_MASK | XUARTPS_MR_STOPMODE_MASK |
		    XUARTPS_MR_PARITY_MASK));
	reg_val |= XUARTPS_MR_CHARLEN_8_BIT | XUARTPS_MR_STOPMODE_1_BIT |
		   XUARTPS_MR_PARITY_NONE;
	sys_write32(reg_val, reg_base + XUARTPS_MR_OFFSET);

	/* Set RX FIFO trigger at 1 data bytes. */
	sys_write32(0x01U, reg_base + XUARTPS_RXWM_OFFSET);

	/* Set RX timeout to 1, which will be 4 character time */
	sys_write32(0x1U, reg_base + XUARTPS_RXTOUT_OFFSET);

	/* Disable all interrupts, polling mode is default */
	sys_write32(XUARTPS_IXR_MASK, reg_base + XUARTPS_IDR_OFFSET);

	/* Set the baud rate */
	set_baudrate(dev, dev_cfg->baud_rate);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	/* Clear any pending interrupt flags */
	sys_write32(XUARTPS_IXR_MASK, reg_base + XUARTPS_ISR_OFFSET);

	/* Attach to & unmask the corresponding interrupt vector */
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
static int uart_xlnx_ps_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_val;
	uint32_t reg_base;

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
static void uart_xlnx_ps_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_val;
	uint32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	/* wait for transmitter to ready to accept a character */
	do {
		reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	} while ((reg_val & XUARTPS_SR_TXEMPTY) == 0);

	sys_write32((uint32_t)(c & 0xFF), reg_base + XUARTPS_FIFO_OFFSET);

	do {
		reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	} while ((reg_val & XUARTPS_SR_TXEMPTY) == 0);
}

/**
 * @brief Converts a parity enum value to a Mode Register bit mask.
 *
 * Converts a value of an enumeration type provided by the driver
 * framework for the configuration of the UART's parity setting
 * into a bit mask within the Mode Register.
 *
 * It is assumed that the Mode Register contents that are being
 * modified within this function come with the bits modified by
 * this function already masked out by the caller.
 *
 * @param mode_reg Pointer to the Mode Register contents to which
 *                 the parity configuration shall be added.
 * @param parity   Enumeration value to be converted to a bit mask.
 *
 * @return Indication of success, always true for this function
 *         as all parity modes supported by the API are also supported
 *         by the hardware.
 */
static inline bool uart_xlnx_ps_cfg2ll_parity(
	uint32_t *mode_reg,
	enum uart_config_parity parity)
{
	/*
	 * Translate the new parity configuration to the mode register's
	 * bits [5..3] (PAR):
	 *  000b : even
	 *  001b : odd
	 *  010b : space
	 *  011b : mark
	 *  1xxb : none
	 */

	switch (parity) {
	default:
	case UART_CFG_PARITY_EVEN:
		*mode_reg |= XUARTPS_MR_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_ODD:
		*mode_reg |= XUARTPS_MR_PARITY_ODD;
		break;
	case UART_CFG_PARITY_SPACE:
		*mode_reg |= XUARTPS_MR_PARITY_SPACE;
		break;
	case UART_CFG_PARITY_MARK:
		*mode_reg |= XUARTPS_MR_PARITY_MARK;
		break;
	case UART_CFG_PARITY_NONE:
		*mode_reg |= XUARTPS_MR_PARITY_NONE;
		break;
	}

	return true;
}

/**
 * @brief Converts a stop bit enum value to a Mode Register bit mask.
 *
 * Converts a value of an enumeration type provided by the driver
 * framework for the configuration of the UART's stop bit setting
 * into a bit mask within the Mode Register.
 *
 * It is assumed that the Mode Register contents that are being
 * modified within this function come with the bits modified by
 * this function already masked out by the caller.
 *
 * @param mode_reg Pointer to the Mode Register contents to which
 *                 the stop bit configuration shall be added.
 * @param stopbits Enumeration value to be converted to a bit mask.
 *
 * @return Indication of success or failure in case of an unsupported
 *         stop bit configuration being provided by the caller.
 */
static inline bool uart_xlnx_ps_cfg2ll_stopbits(
	uint32_t *mode_reg,
	enum uart_config_stop_bits stopbits)
{
	/*
	 * Translate the new stop bit configuration to the mode register's
	 * bits [7..6] (NBSTOP):
	 *  00b : 1 stop bit
	 *  01b : 1.5 stop bits
	 *  10b : 2 stop bits
	 *  11b : reserved
	 */

	switch (stopbits) {
	case UART_CFG_STOP_BITS_0_5:
		/* Controller doesn't support 0.5 stop bits */
		return false;
	default:
	case UART_CFG_STOP_BITS_1:
		*mode_reg |= XUARTPS_MR_STOPMODE_1_BIT;
		break;
	case UART_CFG_STOP_BITS_1_5:
		*mode_reg |= XUARTPS_MR_STOPMODE_1_5_BIT;
		break;
	case UART_CFG_STOP_BITS_2:
		*mode_reg |= XUARTPS_MR_STOPMODE_2_BIT;
		break;
	}

	return true;
}

/**
 * @brief Converts a data bit enum value to a Mode Register bit mask.
 *
 * Converts a value of an enumeration type provided by the driver
 * framework for the configuration of the UART's data bit setting
 * into a bit mask within the Mode Register.
 *
 * It is assumed that the Mode Register contents that are being
 * modified within this function come with the bits modified by
 * this function already masked out by the caller.
 *
 * @param mode_reg Pointer to the Mode Register contents to which
 *                 the data bit configuration shall be added.
 * @param databits Enumeration value to be converted to a bit mask.
 *
 * @return Indication of success or failure in case of an unsupported
 *         data bit configuration being provided by the caller.
 */
static inline bool uart_xlnx_ps_cfg2ll_databits(
	uint32_t *mode_reg,
	enum uart_config_data_bits databits)
{
	/*
	 * Translate the new data bit configuration to the mode register's
	 * bits [2..1] (CHRL):
	 *  0xb : 8 data bits
	 *  10b : 7 data bits
	 *  11b : 6 data bits
	 */

	switch (databits) {
	case UART_CFG_DATA_BITS_5:
	case UART_CFG_DATA_BITS_9:
		/* Controller doesn't support 5 or 9 data bits */
		return false;
	default:
	case UART_CFG_DATA_BITS_8:
		*mode_reg |= XUARTPS_MR_CHARLEN_8_BIT;
		break;
	case UART_CFG_DATA_BITS_7:
		*mode_reg |= XUARTPS_MR_CHARLEN_7_BIT;
		break;
	case UART_CFG_DATA_BITS_6:
		*mode_reg |= XUARTPS_MR_CHARLEN_6_BIT;
		break;
	}

	return true;
}

/**
 * @brief Converts a flow control enum value to a Modem Control
 *        Register bit mask.
 *
 * Converts a value of an enumeration type provided by the driver
 * framework for the configuration of the UART's flow control
 * setting into a bit mask within the Modem Control Register.
 *
 * It is assumed that the Modem Control Register contents that are
 * being modified within this function come with the bits modified
 * by this function already masked out by the caller.
 *
 * @param modemcr_reg Pointer to the Modem Control Register contents
 *                    to which the flow control configuration shall
 *                    be added.
 * @param hwctrl Enumeration value to be converted to a bit mask.
 *
 * @return Indication of success or failure in case of an unsupported
 *         flow control configuration being provided by the caller.
 */
static inline bool uart_xlnx_ps_cfg2ll_hwctrl(
	uint32_t *modemcr_reg,
	enum uart_config_flow_control hwctrl)
{
	/*
	 * Translate the new flow control configuration to the modem
	 * control register's bit [5] (FCM):
	 *  0b : no flow control
	 *  1b : RTS/CTS
	 */

	if (hwctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		*modemcr_reg |= XUARTPS_MODEMCR_FCM_RTS_CTS;
	} else if (hwctrl ==  UART_CFG_FLOW_CTRL_NONE) {
		*modemcr_reg |= XUARTPS_MODEMCR_FCM_NONE;
	} else {
		/* Only no flow control or RTS/CTS is supported. */
		return false;
	}

	return true;
}

/**
 * @brief Configures the UART device at run-time.
 *
 * Configures the UART device at run-time according to the
 * configuration data provided by the caller.
 *
 * @param dev UART device struct
 * @param cfg The configuration parameters to be applied.
 *
 * @return 0 if the configuration completed successfully, ENOTSUP
 *         error if an unsupported configuration parameter is detected.
 */
static int uart_xlnx_ps_configure(const struct device *dev,
				  const struct uart_config *cfg)
{
	struct uart_xlnx_ps_dev_config *dev_cfg =
	(struct uart_xlnx_ps_dev_config *)DEV_CFG(dev);

	uint32_t reg_base    = dev_cfg->uconf.regs;
	uint32_t mode_reg    = 0;
	uint32_t modemcr_reg = 0;

	/* Read the current mode register & modem control register values */
	mode_reg    = sys_read32(reg_base + XUARTPS_MR_OFFSET);
	modemcr_reg = sys_read32(reg_base + XUARTPS_MODEMCR_OFFSET);

	/* Mask out all items that might be re-configured */
	mode_reg    &= (~XUARTPS_MR_PARITY_MASK);
	mode_reg    &= (~XUARTPS_MR_STOPMODE_MASK);
	mode_reg    &= (~XUARTPS_MR_CHARLEN_MASK);
	modemcr_reg &= (~XUARTPS_MODEMCR_FCM_MASK);

	/* Assemble the updated registers, validity checks contained within */
	if ((!uart_xlnx_ps_cfg2ll_parity(&mode_reg, cfg->parity)) ||
		(!uart_xlnx_ps_cfg2ll_stopbits(&mode_reg, cfg->stop_bits)) ||
		(!uart_xlnx_ps_cfg2ll_databits(&mode_reg, cfg->data_bits)) ||
		(!uart_xlnx_ps_cfg2ll_hwctrl(&modemcr_reg, cfg->flow_ctrl))) {
		return -ENOTSUP;
	}

	/* Disable the controller before modifying any config registers */
	xlnx_ps_disable_uart(reg_base);

	/* Set the baud rate */
	set_baudrate(dev, cfg->baudrate);
	dev_cfg->baud_rate = cfg->baudrate;

	/* Write the two control registers */
	sys_write32(mode_reg,    reg_base + XUARTPS_MR_OFFSET);
	sys_write32(modemcr_reg, reg_base + XUARTPS_MODEMCR_OFFSET);

	/* Re-enable the controller */
	xlnx_ps_enable_uart(reg_base);

	return 0;
};

/**
 * @brief Converts a Mode Register bit mask to a parity configuration
 *        enum value.
 *
 * Converts a bit mask representing the UART's parity setting within
 * the UART's Mode Register into a value of an enumeration type provided
 * by the UART driver API.
 *
 * @param mode_reg The current Mode Register contents from which the
 *                 parity setting shall be extracted.
 *
 * @return The current parity setting mapped to the UART driver API's
 *         enum type.
 */
static inline enum uart_config_parity uart_xlnx_ps_ll2cfg_parity(
	uint32_t mode_reg)
{
	/*
	 * Obtain the current parity configuration from the mode register's
	 * bits [5..3] (PAR):
	 *  000b : even -> reset value
	 *  001b : odd
	 *  010b : space
	 *  011b : mark
	 *  1xxb : none
	 */

	switch ((mode_reg & XUARTPS_MR_PARITY_MASK)) {
	case XUARTPS_MR_PARITY_EVEN:
	default:
		return UART_CFG_PARITY_EVEN;
	case XUARTPS_MR_PARITY_ODD:
		return UART_CFG_PARITY_ODD;
	case XUARTPS_MR_PARITY_SPACE:
		return UART_CFG_PARITY_SPACE;
	case XUARTPS_MR_PARITY_MARK:
		return UART_CFG_PARITY_MARK;
	case XUARTPS_MR_PARITY_NONE:
		return UART_CFG_PARITY_NONE;
	}
}

/**
 * @brief Converts a Mode Register bit mask to a stop bit configuration
 *        enum value.
 *
 * Converts a bit mask representing the UART's stop bit setting within
 * the UART's Mode Register into a value of an enumeration type provided
 * by the UART driver API.
 *
 * @param mode_reg The current Mode Register contents from which the
 *                 stop bit setting shall be extracted.
 *
 * @return The current stop bit setting mapped to the UART driver API's
 *         enum type.
 */
static inline enum uart_config_stop_bits uart_xlnx_ps_ll2cfg_stopbits(
	uint32_t mode_reg)
{
	/*
	 * Obtain the current stop bit configuration from the mode register's
	 * bits [7..6] (NBSTOP):
	 *  00b : 1 stop bit -> reset value
	 *  01b : 1.5 stop bits
	 *  10b : 2 stop bits
	 *  11b : reserved
	 */

	switch ((mode_reg & XUARTPS_MR_STOPMODE_MASK)) {
	case XUARTPS_MR_STOPMODE_1_BIT:
	default:
		return UART_CFG_STOP_BITS_1;
	case XUARTPS_MR_STOPMODE_1_5_BIT:
		return UART_CFG_STOP_BITS_1_5;
	case XUARTPS_MR_STOPMODE_2_BIT:
		return UART_CFG_STOP_BITS_2;
	}
}

/**
 * @brief Converts a Mode Register bit mask to a data bit configuration
 *        enum value.
 *
 * Converts a bit mask representing the UART's data bit setting within
 * the UART's Mode Register into a value of an enumeration type provided
 * by the UART driver API.
 *
 * @param mode_reg The current Mode Register contents from which the
 *                 data bit setting shall be extracted.
 *
 * @return The current data bit setting mapped to the UART driver API's
 *         enum type.
 */
static inline enum uart_config_data_bits uart_xlnx_ps_ll2cfg_databits(
	uint32_t mode_reg)
{
	/*
	 * Obtain the current data bit configuration from the mode register's
	 * bits [2..1] (CHRL):
	 *  0xb : 8 data bits -> reset value
	 *  10b : 7 data bits
	 *  11b : 6 data bits
	 */

	switch ((mode_reg & XUARTPS_MR_CHARLEN_MASK)) {
	case XUARTPS_MR_CHARLEN_8_BIT:
	default:
		return UART_CFG_DATA_BITS_8;
	case XUARTPS_MR_CHARLEN_7_BIT:
		return UART_CFG_DATA_BITS_7;
	case XUARTPS_MR_CHARLEN_6_BIT:
		return UART_CFG_DATA_BITS_6;
	}
}

/**
 * @brief Converts a Modem Control Register bit mask to a flow control
 *        configuration enum value.
 *
 * Converts a bit mask representing the UART's flow control setting within
 * the UART's Modem Control Register into a value of an enumeration type
 * provided by the UART driver API.
 *
 * @param modemcr_reg The current Modem Control Register contents from
 *                    which the parity setting shall be extracted.
 *
 * @return The current flow control setting mapped to the UART driver API's
 *         enum type.
 */
static inline enum uart_config_flow_control uart_xlnx_ps_ll2cfg_hwctrl(
	uint32_t modemcr_reg)
{
	/*
	 * Obtain the current flow control configuration from the modem
	 * control register's bit [5] (FCM):
	 *  0b : no flow control -> reset value
	 *  1b : RTS/CTS
	 */

	if ((modemcr_reg & XUARTPS_MODEMCR_FCM_MASK)
		== XUARTPS_MODEMCR_FCM_RTS_CTS) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

/**
 * @brief Returns the current configuration of the UART at run-time.
 *
 * Returns the current configuration of the UART at run-time by obtaining
 * the current configuration from the UART's Mode and Modem Control Registers
 * (exception: baud rate).
 *
 * @param dev UART device struct
 * @param cfg Pointer to the data structure to which the current configuration
 *            shall be written.
 *
 * @return always 0.
 */
static int uart_xlnx_ps_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);

	/*
	 * Read the Mode & Modem control registers - they contain
	 * the current data / stop bit and parity settings (Mode
	 * Register) and the current flow control setting (Modem
	 * Control register).
	 */

	uint32_t reg_base    = dev_cfg->uconf.regs;
	uint32_t mode_reg    = sys_read32(reg_base + XUARTPS_MR_OFFSET);
	uint32_t modemcr_reg = sys_read32(reg_base + XUARTPS_MODEMCR_OFFSET);

	cfg->baudrate  = dev_cfg->baud_rate;
	cfg->parity    = uart_xlnx_ps_ll2cfg_parity(mode_reg);
	cfg->stop_bits = uart_xlnx_ps_ll2cfg_stopbits(mode_reg);
	cfg->data_bits = uart_xlnx_ps_ll2cfg_databits(mode_reg);
	cfg->flow_ctrl = uart_xlnx_ps_ll2cfg_hwctrl(modemcr_reg);

	return 0;
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
static int uart_xlnx_ps_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_val;
	uint32_t reg_base;
	int onum = 0;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);
	while (onum < size && (reg_val & XUARTPS_SR_TXFULL) == 0) {
		sys_write32((uint32_t)(tx_data[onum] & 0xFF),
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
static int uart_xlnx_ps_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_val;
	uint32_t reg_base;
	int inum = 0;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_SR_OFFSET);

	while (inum < size && (reg_val & XUARTPS_SR_RXEMPTY) == 0) {
		rx_data[inum] = (uint8_t)sys_read32(reg_base
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
static void uart_xlnx_ps_irq_tx_enable(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(
		(XUARTPS_IXR_TTRIG | XUARTPS_IXR_TXEMPTY),
		reg_base + XUARTPS_IER_OFFSET);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_xlnx_ps_irq_tx_disable(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(
		(XUARTPS_IXR_TTRIG | XUARTPS_IXR_TXEMPTY),
		reg_base + XUARTPS_IDR_OFFSET);
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xlnx_ps_irq_tx_ready(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;
	uint32_t reg_val;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_ISR_OFFSET);
	if ((reg_val & (XUARTPS_IXR_TTRIG | XUARTPS_IXR_TXEMPTY)) == 0) {
		return 0;
	} else {
		sys_write32(
			(XUARTPS_IXR_TTRIG | XUARTPS_IXR_TXEMPTY),
			reg_base + XUARTPS_ISR_OFFSET);
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
static int uart_xlnx_ps_irq_tx_complete(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;
	uint32_t reg_val;

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
static void uart_xlnx_ps_irq_rx_enable(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;

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
static void uart_xlnx_ps_irq_rx_disable(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;

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
static int uart_xlnx_ps_irq_rx_ready(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;
	uint32_t reg_val;

	reg_base = dev_cfg->uconf.regs;
	reg_val = sys_read32(reg_base + XUARTPS_ISR_OFFSET);
	if ((reg_val & XUARTPS_IXR_RTRIG) == 0) {
		return 0;
	} else {
		sys_write32(XUARTPS_IXR_RTRIG, reg_base + XUARTPS_ISR_OFFSET);
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
static void uart_xlnx_ps_irq_err_enable(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(
		  XUARTPS_IXR_TOVR    /* [12] Transmitter FIFO Overflow */
		| XUARTPS_IXR_TOUT    /* [8]  Receiver Timerout */
		| XUARTPS_IXR_PARITY  /* [7]  Parity Error */
		| XUARTPS_IXR_FRAMING /* [6]  Receiver Framing Error */
		| XUARTPS_IXR_RXOVR,  /* [5]  Receiver Overflow Error */
	    reg_base + XUARTPS_IER_OFFSET);
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static void uart_xlnx_ps_irq_err_disable(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;

	reg_base = dev_cfg->uconf.regs;
	sys_write32(
		  XUARTPS_IXR_TOVR    /* [12] Transmitter FIFO Overflow */
		| XUARTPS_IXR_TOUT    /* [8]  Receiver Timerout */
		| XUARTPS_IXR_PARITY  /* [7]  Parity Error */
		| XUARTPS_IXR_FRAMING /* [6]  Receiver Framing Error */
		| XUARTPS_IXR_RXOVR,  /* [5]  Receiver Overflow Error */
	    reg_base + XUARTPS_IDR_OFFSET);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_xlnx_ps_irq_is_pending(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_config *dev_cfg = DEV_CFG(dev);
	uint32_t reg_base;
	uint32_t reg_imr;
	uint32_t reg_isr;

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
static int uart_xlnx_ps_irq_update(const struct device *dev)
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
static void uart_xlnx_ps_irq_callback_set(const struct device *dev,
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
static void uart_xlnx_ps_isr(const struct device *dev)
{
	const struct uart_xlnx_ps_dev_data_t *data = DEV_DATA(dev);

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_xlnx_ps_driver_api = {
	.poll_in = uart_xlnx_ps_poll_in,
	.poll_out = uart_xlnx_ps_poll_out,
	.configure = uart_xlnx_ps_configure,
	.config_get = uart_xlnx_ps_config_get,
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
static void uart_xlnx_ps_irq_config_##port(const struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_IRQN(port), \
	DT_INST_IRQ(port, priority), \
	uart_xlnx_ps_isr, DEVICE_GET(uart_xlnx_ps_##port), \
	0); \
	irq_enable(DT_INST_IRQN(port)); \
}

#else

#define UART_XLNX_PS_IRQ_CONF_FUNC_SET(port)
#define UART_XLNX_PS_IRQ_CONF_FUNC(port)

#endif /*CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_XLNX_PS_DEV_DATA(port) \
static struct uart_xlnx_ps_dev_data_t uart_xlnx_ps_dev_data_##port

#define UART_XLNX_PS_DEV_CFG(port) \
static struct uart_xlnx_ps_dev_config uart_xlnx_ps_dev_cfg_##port = { \
	.uconf = { \
		.regs = DT_INST_REG_ADDR(port), \
		.sys_clk_freq = DT_INST_PROP(port, clock_frequency), \
		UART_XLNX_PS_IRQ_CONF_FUNC_SET(port) \
	}, \
	.baud_rate = DT_INST_PROP(port, current_speed), \
}

#define UART_XLNX_PS_INIT(port) \
DEVICE_AND_API_INIT(uart_xlnx_ps_##port, DT_INST_LABEL(port), \
	uart_xlnx_ps_init, \
	&uart_xlnx_ps_dev_data_##port, \
	&uart_xlnx_ps_dev_cfg_##port, \
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
	&uart_xlnx_ps_driver_api)

#define UART_XLNX_INSTANTIATE(inst)		\
	UART_XLNX_PS_IRQ_CONF_FUNC(inst);	\
	UART_XLNX_PS_DEV_DATA(inst);		\
	UART_XLNX_PS_DEV_CFG(inst);		\
	UART_XLNX_PS_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(UART_XLNX_INSTANTIATE)
