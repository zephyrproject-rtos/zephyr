/*
 * Copyright (c) 2018 Xilinx, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_uart

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_cdns, CONFIG_UART_LOG_LEVEL);

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

/* For all register offsets and bits / bit masks:
 * Comp. Xilinx Zynq-7000 Technical Reference Manual (ug585), chap. B.33
 */

/* Register offsets within the UART device's register space */
#define CDNS_CR_OFFSET      0x0000U /**< Control Register [8:0] */
#define CDNS_MR_OFFSET      0x0004U /**< Mode Register [9:0] */
#define CDNS_IER_OFFSET     0x0008U /**< Interrupt Enable [12:0] */
#define CDNS_IDR_OFFSET     0x000CU /**< Interrupt Disable [12:0] */
#define CDNS_IMR_OFFSET     0x0010U /**< Interrupt Mask [12:0] */
#define CDNS_ISR_OFFSET     0x0014U /**< Interrupt Status [12:0]*/
#define CDNS_BAUDGEN_OFFSET 0x0018U /**< Baud Rate Generator [15:0] */
#define CDNS_RXTOUT_OFFSET  0x001CU /**< RX Timeout [7:0] */
#define CDNS_RXWM_OFFSET    0x0020U /**< RX FIFO Trigger Level [5:0] */
#define CDNS_MODEMCR_OFFSET 0x0024U /**< Modem Control [5:0] */
#define CDNS_MODEMSR_OFFSET 0x0028U /**< Modem Status [8:0] */
#define CDNS_SR_OFFSET      0x002CU /**< Channel Status [14:0] */
#define CDNS_FIFO_OFFSET    0x0030U /**< FIFO [7:0] */
#define CDNS_BAUDDIV_OFFSET 0x0034U /**< Baud Rate Divider [7:0] */
#define CDNS_FLOWDEL_OFFSET 0x0038U /**< Flow Delay [5:0] */
#define CDNS_TXWM_OFFSET    0x0044U /**< TX FIFO Trigger Level [5:0] */
#define CDNS_RXBS_OFFSET    0x0048U /**< RX FIFO Byte Status [11:0] */

/* Control Register Bits Definition */
#define CDNS_CR_STOPBRK     0x00000100U /**< Stop transmission of break */
#define CDNS_CR_STARTBRK    0x00000080U /**< Set break */
#define CDNS_CR_TORST       0x00000040U /**< RX timeout counter restart */
#define CDNS_CR_TX_DIS      0x00000020U /**< TX disabled. */
#define CDNS_CR_TX_EN       0x00000010U /**< TX enabled */
#define CDNS_CR_RX_DIS      0x00000008U /**< RX disabled. */
#define CDNS_CR_RX_EN       0x00000004U /**< RX enabled */
#define CDNS_CR_EN_DIS_MASK 0x0000003CU /**< Enable/disable Mask */
#define CDNS_CR_TXRST       0x00000002U /**< TX logic reset */
#define CDNS_CR_RXRST       0x00000001U /**< RX logic reset */

/* Mode Register Bits Definition */
#define CDNS_MR_CCLK             0x00000400U /**< Input clock select */
#define CDNS_MR_CHMODE_R_LOOP    0x00000300U /**< Remote loopback mode */
#define CDNS_MR_CHMODE_L_LOOP    0x00000200U /**< Local loopback mode */
#define CDNS_MR_CHMODE_ECHO      0x00000100U /**< Auto echo mode */
#define CDNS_MR_CHMODE_NORM      0x00000000U /**< Normal mode */
#define CDNS_MR_CHMODE_SHIFT     8U          /**< Mode shift */
#define CDNS_MR_CHMODE_MASK      0x00000300U /**< Mode mask */
#define CDNS_MR_STOPMODE_2_BIT   0x00000080U /**< 2 stop bits */
#define CDNS_MR_STOPMODE_1_5_BIT 0x00000040U /**< 1.5 stop bits */
#define CDNS_MR_STOPMODE_1_BIT   0x00000000U /**< 1 stop bit */
#define CDNS_MR_STOPMODE_SHIFT   6U          /**< Stop bits shift */
#define CDNS_MR_STOPMODE_MASK    0x000000A0U /**< Stop bits mask */
#define CDNS_MR_PARITY_NONE      0x00000020U /**< No parity mode */
#define CDNS_MR_PARITY_MARK      0x00000018U /**< Mark parity mode */
#define CDNS_MR_PARITY_SPACE     0x00000010U /**< Space parity mode */
#define CDNS_MR_PARITY_ODD       0x00000008U /**< Odd parity mode */
#define CDNS_MR_PARITY_EVEN      0x00000000U /**< Even parity mode */
#define CDNS_MR_PARITY_SHIFT     3U          /**< Parity setting shift */
#define CDNS_MR_PARITY_MASK      0x00000038U /**< Parity mask */
#define CDNS_MR_CHARLEN_6_BIT    0x00000006U /**< 6 bits data */
#define CDNS_MR_CHARLEN_7_BIT    0x00000004U /**< 7 bits data */
#define CDNS_MR_CHARLEN_8_BIT    0x00000000U /**< 8 bits data */
#define CDNS_MR_CHARLEN_SHIFT    1U          /**< Data Length shift */
#define CDNS_MR_CHARLEN_MASK     0x00000006U /**< Data length mask */
#define CDNS_MR_CLKSEL           0x00000001U /**< Input clock select */

/* Interrupt Register Bits Definition */
#define CDNS_IXR_RBRK    0x00002000U /**< Rx FIFO break detect interrupt */
#define CDNS_IXR_TOVR    0x00001000U /**< Tx FIFO Overflow interrupt */
#define CDNS_IXR_TNFUL   0x00000800U /**< Tx FIFO Nearly Full interrupt */
#define CDNS_IXR_TTRIG   0x00000400U /**< Tx Trig interrupt */
#define CDNS_IXR_DMS     0x00000200U /**< Modem status change interrupt */
#define CDNS_IXR_TOUT    0x00000100U /**< Timeout error interrupt */
#define CDNS_IXR_PARITY  0x00000080U /**< Parity error interrupt */
#define CDNS_IXR_FRAMING 0x00000040U /**< Framing error interrupt */
#define CDNS_IXR_RXOVR   0x00000020U /**< Overrun error interrupt */
#define CDNS_IXR_TXFULL  0x00000010U /**< TX FIFO full interrupt. */
#define CDNS_IXR_TXEMPTY 0x00000008U /**< TX FIFO empty interrupt. */
#define CDNS_IXR_RXFULL  0x00000004U /**< RX FIFO full interrupt. */
#define CDNS_IXR_RXEMPTY 0x00000002U /**< RX FIFO empty interrupt. */
#define CDNS_IXR_RTRIG   0x00000001U /**< RX FIFO trigger interrupt. */
#define CDNS_IXR_MASK    0x00003FFFU /**< Valid bit mask */

/* Modem Control Register Bits Definition */
#define CDNS_MODEMCR_FCM_RTS_CTS 0x00000020 /**< RTS/CTS hardware flow control. */
#define CDNS_MODEMCR_FCM_NONE    0x00000000 /**< No hardware flow control. */
#define CDNS_MODEMCR_FCM_MASK    0x00000020 /**< Hardware flow control mask. */
#define CDNS_MODEMCR_RTS_SHIFT   1U         /**< RTS bit shift */
#define CDNS_MODEMCR_DTR_SHIFT   0U         /**< DTR bit shift */

/* Channel Status Register */
#define CDNS_SR_TNFUL   0x00004000U /**< TX FIFO Nearly Full Status */
#define CDNS_SR_TTRIG   0x00002000U /**< TX FIFO Trigger Status */
#define CDNS_SR_FLOWDEL 0x00001000U /**< RX FIFO fill over flow delay */
#define CDNS_SR_TACTIVE 0x00000800U /**< TX active */
#define CDNS_SR_RACTIVE 0x00000400U /**< RX active */
#define CDNS_SR_TXFULL  0x00000010U /**< TX FIFO full */
#define CDNS_SR_TXEMPTY 0x00000008U /**< TX FIFO empty */
#define CDNS_SR_RXFULL  0x00000004U /**< RX FIFO full */
#define CDNS_SR_RXEMPTY 0x00000002U /**< RX FIFO empty */
#define CDNS_SR_RTRIG   0x00000001U /**< RX FIFO fill over trigger */

/* Cadence UART baud generator (CDNSUART user guide, sec. 4.4):
 *   baud = sel_clk / (CD * (BDIV + 1))
 *   CD   : 1..65535   baud_sample = sel_clk / CD
 *   BDIV : 0..255     (BDIV + 1) = bit oversampling factor
 * Set CDNS_CD_MIN to 2 if a given integration dislikes the divide-by-1
 * (bypass) case.
 */
#define CDNS_CD_MIN   1U
#define CDNS_CD_MAX   65535U
#define CDNS_BDIV_MIN 4U
#define CDNS_BDIV_MAX 255U

/** Device configuration structure */
struct uart_cdns_dev_config {
	DEVICE_MMIO_ROM;
	uint32_t sys_clk_freq;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

/** Device data structure */
struct uart_cdns_dev_data_t {
	DEVICE_MMIO_RAM;
	uint32_t parity;
	uint32_t stopbits;
	uint32_t databits;
	uint32_t flowctrl;
	uint32_t baud_rate;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct device *dev;
	struct k_timer timer;
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
};

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
static void cdns_disable_uart(uintptr_t reg_base)
{
	uint32_t reg_val = sys_read32(reg_base + CDNS_CR_OFFSET);

	reg_val &= (~CDNS_CR_EN_DIS_MASK);
	/* Set control register bits [5]: TX_DIS and [3]: RX_DIS */
	reg_val |= CDNS_CR_TX_DIS | CDNS_CR_RX_DIS;
	sys_write32(reg_val, reg_base + CDNS_CR_OFFSET);
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
static void cdns_enable_uart(uintptr_t reg_base)
{
	uint32_t reg_val = sys_read32(reg_base + CDNS_CR_OFFSET);

	reg_val &= (~CDNS_CR_EN_DIS_MASK);
	/* Set control register bits [4]: TX_EN and [2]: RX_EN */
	reg_val |= CDNS_CR_TX_EN | CDNS_CR_RX_EN;
	sys_write32(reg_val, reg_base + CDNS_CR_OFFSET);
}

/**
 * @brief Calculates and sets the values of the BAUDDIV and BAUDGEN registers.
 *
 * Sweeps BDIV and computes the nearest baud rate generator count (CD) for
 * each candidate, then programs the (BDIV, CD) pair that yields the lowest
 * baud error. Ties are resolved toward higher oversampling (larger BDIV + 1)
 * for better receiver margin. An invalid CD value is never written.
 *
 * baud = sel_clk / (CD * (BDIV + 1)); see CDNSUART user guide, chapter 4.4.
 *
 * @param dev UART device struct
 * @param baud The desired baud rate as a decimal value
 *
 * @return 0 if successful, failed otherwise
 */
static int set_baudrate(const struct device *dev, uint32_t baud)
{
	const struct uart_cdns_dev_config *dev_cfg = dev->config;
	uint32_t clk_freq = dev_cfg->sys_clk_freq;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t best_bdiv = 0U, best_cd = 0U, best_err = UINT32_MAX;

	if ((baud == 0U) || (clk_freq == 0U)) {
		return -EINVAL;
	}

	/* Covering case where input clock is so slow */
	if (clk_freq < 1000000U && baud > 4800U) {
		baud = 4800U;
	}

	/* Iterate high oversample -> low so equal-error ties keep the more
	 * robust (higher BDIV + 1) sampling point.
	 */
	for (uint32_t bdiv = CDNS_BDIV_MAX; bdiv >= CDNS_BDIV_MIN; bdiv--) {
		uint64_t span = (uint64_t)baud * (bdiv + 1U);
		uint32_t cd = (uint32_t)(((uint64_t)clk_freq + (span / 2U)) / span);
		uint32_t actual, err;

		if ((cd < CDNS_CD_MIN) || (cd > CDNS_CD_MAX)) {
			continue;
		}

		actual = clk_freq / (cd * (bdiv + 1U));
		err = (actual > baud) ? (actual - baud) : (baud - actual);

		if (err < best_err) {
			best_err = err;
			best_bdiv = bdiv;
			best_cd = cd;
			if (err == 0U) {
				break;
			}
		}
	}

	if (best_err == UINT32_MAX) {
		LOG_ERR("uart_cdns: no valid divider for %u baud @ %u Hz", baud, clk_freq);
		return -EINVAL;
	}

	if (((uint64_t)best_err * 1000U) / baud > 20U) {
		LOG_WRN("uart_cdns: baud %u off by %u bps (CD=%u BDIV=%u, clk %u Hz)",
			baud, best_err, best_cd, best_bdiv, clk_freq);
	}

	/*
	 * Set baud rate divisor and generator.
	 * -> This function is always called from a context in which
	 * the receiver/transmitter is disabled, the baud rate can
	 * be changed safely at this time.
	 */
	sys_write32(best_bdiv, reg_base + CDNS_BAUDDIV_OFFSET);
	sys_write32(best_cd, reg_base + CDNS_BAUDGEN_OFFSET);

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Trigger a UART callback based on a timer.
 *
 * @param timer Timer that triggered the callback
 */
static void uart_cdns_soft_isr(struct k_timer *timer)
{
	struct uart_cdns_dev_data_t *data = CONTAINER_OF(timer, struct uart_cdns_dev_data_t, timer);

	if (data->user_cb) {
		data->user_cb(data->dev, data->user_data);
	}
}

#endif

/**
 * @brief Initialize individual UART port
 *
 * This routine is called to reset the chip in a quiescent state.
 *
 * @param dev UART device struct
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_cdns_init(const struct device *dev)
{
	__maybe_unused const struct uart_cdns_dev_config *dev_cfg = dev->config;
	struct uart_cdns_dev_data_t *dev_data = dev->data;
	uint32_t reg_val;
	int err;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

#ifdef CONFIG_PINCTRL
	if (dev_cfg->pincfg != NULL) {
		err = pinctrl_apply_state(dev_cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}
	}
#endif

	/* Set RX/TX reset and disable RX/TX */
	sys_write32(CDNS_CR_STOPBRK | CDNS_CR_TX_DIS | CDNS_CR_RX_DIS | CDNS_CR_TXRST |
			    CDNS_CR_RXRST,
		    reg_base + CDNS_CR_OFFSET);

	/* Set initial character length / start/stop bit / parity configuration */
	reg_val = sys_read32(reg_base + CDNS_MR_OFFSET);
	reg_val &= (~(CDNS_MR_CHARLEN_MASK | CDNS_MR_STOPMODE_MASK | CDNS_MR_PARITY_MASK));
	reg_val |= CDNS_MR_CHARLEN_8_BIT | CDNS_MR_STOPMODE_1_BIT | CDNS_MR_PARITY_NONE;
	sys_write32(reg_val, reg_base + CDNS_MR_OFFSET);

	/* Set RX FIFO trigger at 1 data bytes. */
	sys_write32(0x01U, reg_base + CDNS_RXWM_OFFSET);

	/* Disable all interrupts, polling mode is default */
	sys_write32(CDNS_IXR_MASK, reg_base + CDNS_IDR_OFFSET);

	/* Set the baud rate */
	err = set_baudrate(dev, dev_data->baud_rate);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Clear any pending interrupt flags */
	sys_write32(CDNS_IXR_MASK, reg_base + CDNS_ISR_OFFSET);

	dev_data->dev = dev;
	k_timer_init(&dev_data->timer, &uart_cdns_soft_isr, NULL);

	/* Attach to & unmask the corresponding interrupt vector */
	dev_cfg->irq_config_func(dev);

#endif

	cdns_enable_uart(reg_base);

	return 0;
}

static int uart_cdns_poll_in(const struct device *dev, unsigned char *c)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t reg_val = sys_read32(reg_base + CDNS_SR_OFFSET);

	if ((reg_val & CDNS_SR_RXEMPTY) == 0) {
		*c = (unsigned char)sys_read32(reg_base + CDNS_FIFO_OFFSET);
		return 0;
	} else {
		return -1;
	}
}

static void uart_cdns_poll_out(const struct device *dev, unsigned char c)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	/* wait for transmitter to be ready to accept a character */
	while ((sys_read32(reg_base + CDNS_SR_OFFSET) & CDNS_SR_TXFULL) != 0) {
	}

	sys_write32((uint32_t)(c & 0xFF), reg_base + CDNS_FIFO_OFFSET);
}

static int uart_cdns_err_check(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t reg_val = sys_read32(reg_base + CDNS_ISR_OFFSET);
	int err = 0;

	if (reg_val & CDNS_IXR_RBRK) {
		err |= UART_BREAK;
	}

	if (reg_val & CDNS_IXR_TOVR) {
		err |= UART_ERROR_OVERRUN;
	}

	if (reg_val & CDNS_IXR_PARITY) {
		err |= UART_ERROR_PARITY;
	}

	if (reg_val & CDNS_IXR_FRAMING) {
		err |= UART_ERROR_FRAMING;
	}

	if (reg_val & CDNS_IXR_RXOVR) {
		err |= UART_ERROR_OVERRUN;
	}
	sys_write32(reg_val & (CDNS_IXR_RBRK | CDNS_IXR_TOVR | CDNS_IXR_TOUT | CDNS_IXR_PARITY |
			       CDNS_IXR_FRAMING | CDNS_IXR_RXOVR),
		    reg_base + CDNS_ISR_OFFSET);

	return err;
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
static inline bool uart_cdns_cfg2ll_parity(uint32_t *mode_reg, enum uart_config_parity parity)
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
	case UART_CFG_PARITY_ODD:
		*mode_reg |= CDNS_MR_PARITY_ODD;
		break;
	case UART_CFG_PARITY_SPACE:
		*mode_reg |= CDNS_MR_PARITY_SPACE;
		break;
	case UART_CFG_PARITY_MARK:
		*mode_reg |= CDNS_MR_PARITY_MARK;
		break;
	case UART_CFG_PARITY_NONE:
		*mode_reg |= CDNS_MR_PARITY_NONE;
		break;
	case UART_CFG_PARITY_EVEN:
	default:
		*mode_reg |= CDNS_MR_PARITY_EVEN;
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
static inline bool uart_cdns_cfg2ll_stopbits(uint32_t *mode_reg,
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
	case UART_CFG_STOP_BITS_1_5:
		*mode_reg |= CDNS_MR_STOPMODE_1_5_BIT;
		break;
	case UART_CFG_STOP_BITS_2:
		*mode_reg |= CDNS_MR_STOPMODE_2_BIT;
		break;
	case UART_CFG_STOP_BITS_1:
	default:
		*mode_reg |= CDNS_MR_STOPMODE_1_BIT;
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
static inline bool uart_cdns_cfg2ll_databits(uint32_t *mode_reg,
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
	case UART_CFG_DATA_BITS_7:
		*mode_reg |= CDNS_MR_CHARLEN_7_BIT;
		break;
	case UART_CFG_DATA_BITS_6:
		*mode_reg |= CDNS_MR_CHARLEN_6_BIT;
		break;
	case UART_CFG_DATA_BITS_5:
	case UART_CFG_DATA_BITS_9:
		/* Controller doesn't support 5 or 9 data bits */
		return false;
	case UART_CFG_DATA_BITS_8:
	default:
		*mode_reg |= CDNS_MR_CHARLEN_8_BIT;
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
static inline bool uart_cdns_cfg2ll_hwctrl(uint32_t *modemcr_reg,
					   enum uart_config_flow_control hwctrl)
{
	/*
	 * Translate the new flow control configuration to the modem
	 * control register's bit [5] (FCM):
	 *  0b : no flow control
	 *  1b : RTS/CTS
	 */

	if (hwctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		*modemcr_reg |= CDNS_MODEMCR_FCM_RTS_CTS;
	} else if (hwctrl == UART_CFG_FLOW_CTRL_NONE) {
		*modemcr_reg |= CDNS_MODEMCR_FCM_NONE;
	} else {
		/* Only no flow control or RTS/CTS is supported. */
		return false;
	}

	return true;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_cdns_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_cdns_dev_data_t *dev_data = dev->data;

	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t mode_reg = 0;
	uint32_t modemcr_reg = 0;
	int err = 0;

	/* Read the current mode register & modem control register values */
	mode_reg = sys_read32(reg_base + CDNS_MR_OFFSET);
	modemcr_reg = sys_read32(reg_base + CDNS_MODEMCR_OFFSET);

	/* Mask out all items that might be re-configured */
	mode_reg &= (~CDNS_MR_PARITY_MASK);
	mode_reg &= (~CDNS_MR_STOPMODE_MASK);
	mode_reg &= (~CDNS_MR_CHARLEN_MASK);
	modemcr_reg &= (~CDNS_MODEMCR_FCM_MASK);

	/* Assemble the updated registers, validity checks contained within */
	if ((!uart_cdns_cfg2ll_parity(&mode_reg, cfg->parity)) ||
	    (!uart_cdns_cfg2ll_stopbits(&mode_reg, cfg->stop_bits)) ||
	    (!uart_cdns_cfg2ll_databits(&mode_reg, cfg->data_bits)) ||
	    (!uart_cdns_cfg2ll_hwctrl(&modemcr_reg, cfg->flow_ctrl))) {
		return -ENOTSUP;
	}

	/* Disable the controller before modifying any config registers */
	cdns_disable_uart(reg_base);

	/* Set the baud rate */
	err = set_baudrate(dev, cfg->baudrate);
	if (err < 0) {
		goto out;
	}
	dev_data->baud_rate = cfg->baudrate;

	/* Write the two control registers */
	sys_write32(mode_reg, reg_base + CDNS_MR_OFFSET);
	sys_write32(modemcr_reg, reg_base + CDNS_MODEMCR_OFFSET);

out:
	/* Re-enable the controller */
	cdns_enable_uart(reg_base);
	return err;
};
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

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
static inline enum uart_config_parity uart_cdns_ll2cfg_parity(uint32_t mode_reg)
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

	switch ((mode_reg & CDNS_MR_PARITY_MASK)) {
	case CDNS_MR_PARITY_ODD:
		return UART_CFG_PARITY_ODD;
	case CDNS_MR_PARITY_SPACE:
		return UART_CFG_PARITY_SPACE;
	case CDNS_MR_PARITY_MARK:
		return UART_CFG_PARITY_MARK;
	case CDNS_MR_PARITY_NONE:
		return UART_CFG_PARITY_NONE;
	case CDNS_MR_PARITY_EVEN:
	default:
		return UART_CFG_PARITY_EVEN;
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
static inline enum uart_config_stop_bits uart_cdns_ll2cfg_stopbits(uint32_t mode_reg)
{
	/*
	 * Obtain the current stop bit configuration from the mode register's
	 * bits [7..6] (NBSTOP):
	 *  00b : 1 stop bit -> reset value
	 *  01b : 1.5 stop bits
	 *  10b : 2 stop bits
	 *  11b : reserved
	 */

	switch ((mode_reg & CDNS_MR_STOPMODE_MASK)) {
	case CDNS_MR_STOPMODE_1_5_BIT:
		return UART_CFG_STOP_BITS_1_5;
	case CDNS_MR_STOPMODE_2_BIT:
		return UART_CFG_STOP_BITS_2;
	case CDNS_MR_STOPMODE_1_BIT:
	default:
		return UART_CFG_STOP_BITS_1;
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
static inline enum uart_config_data_bits uart_cdns_ll2cfg_databits(uint32_t mode_reg)
{
	/*
	 * Obtain the current data bit configuration from the mode register's
	 * bits [2..1] (CHRL):
	 *  0xb : 8 data bits -> reset value
	 *  10b : 7 data bits
	 *  11b : 6 data bits
	 */

	switch ((mode_reg & CDNS_MR_CHARLEN_MASK)) {
	case CDNS_MR_CHARLEN_7_BIT:
		return UART_CFG_DATA_BITS_7;
	case CDNS_MR_CHARLEN_6_BIT:
		return UART_CFG_DATA_BITS_6;
	case CDNS_MR_CHARLEN_8_BIT:
	default:
		return UART_CFG_DATA_BITS_8;
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
static inline enum uart_config_flow_control uart_cdns_ll2cfg_hwctrl(uint32_t modemcr_reg)
{
	/*
	 * Obtain the current flow control configuration from the modem
	 * control register's bit [5] (FCM):
	 *  0b : no flow control -> reset value
	 *  1b : RTS/CTS
	 */

	if ((modemcr_reg & CDNS_MODEMCR_FCM_MASK) == CDNS_MODEMCR_FCM_RTS_CTS) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_cdns_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_cdns_dev_data_t *dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	/*
	 * Read the Mode & Modem control registers - they contain
	 * the current data / stop bit and parity settings (Mode
	 * Register) and the current flow control setting (Modem
	 * Control register).
	 */
	uint32_t mode_reg = sys_read32(reg_base + CDNS_MR_OFFSET);
	uint32_t modemcr_reg = sys_read32(reg_base + CDNS_MODEMCR_OFFSET);

	cfg->baudrate = dev_data->baud_rate;
	cfg->parity = uart_cdns_ll2cfg_parity(mode_reg);
	cfg->stop_bits = uart_cdns_ll2cfg_stopbits(mode_reg);
	cfg->data_bits = uart_cdns_ll2cfg_databits(mode_reg);
	cfg->flow_ctrl = uart_cdns_ll2cfg_hwctrl(modemcr_reg);

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#if CONFIG_UART_INTERRUPT_DRIVEN

static int uart_cdns_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	int count = 0;

	while (count < size && (sys_read32(reg_base + CDNS_SR_OFFSET) & CDNS_SR_TXFULL) == 0) {
		sys_write32((uint32_t)tx_data[count++], reg_base + CDNS_FIFO_OFFSET);
	}

	return count;
}

static int uart_cdns_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	int count = 0;

	while (count < size && (sys_read32(reg_base + CDNS_SR_OFFSET) & CDNS_SR_RXEMPTY) == 0) {
		rx_data[count++] = (uint8_t)sys_read32(reg_base + CDNS_FIFO_OFFSET);
	}

	return count;
}

static void uart_cdns_irq_tx_enable(const struct device *dev)
{
	struct uart_cdns_dev_data_t *dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	sys_write32((CDNS_IXR_TTRIG | CDNS_IXR_TXEMPTY), reg_base + CDNS_IER_OFFSET);
	if ((sys_read32(reg_base + CDNS_SR_OFFSET) & (CDNS_SR_TTRIG | CDNS_SR_TXEMPTY)) != 0) {
		/*
		 * Enabling TX empty interrupts does not cause an interrupt
		 * if the FIFO is already empty.
		 * Generate a soft interrupt and have it call the
		 * callback function in timer isr context.
		 */
		k_timer_start(&dev_data->timer, K_NO_WAIT, K_NO_WAIT);
	}
}

static void uart_cdns_irq_tx_disable(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	sys_write32((CDNS_IXR_TTRIG | CDNS_IXR_TXEMPTY), reg_base + CDNS_IDR_OFFSET);
}

static int uart_cdns_irq_tx_ready(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t reg_val = sys_read32(reg_base + CDNS_SR_OFFSET);

	if ((reg_val & (CDNS_SR_TTRIG | CDNS_SR_TXEMPTY)) == 0) {
		return 0;
	} else {
		return 1;
	}
}

static int uart_cdns_irq_tx_complete(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t reg_val = sys_read32(reg_base + CDNS_SR_OFFSET);

	return (reg_val & CDNS_SR_TXEMPTY) != 0;
}

static void uart_cdns_irq_rx_enable(const struct device *dev)
{
	struct uart_cdns_dev_data_t *dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	sys_write32(CDNS_IXR_RTRIG, reg_base + CDNS_IER_OFFSET);
	if ((sys_read32(reg_base + CDNS_SR_OFFSET) & CDNS_SR_RXEMPTY) == 0) {
		/*
		 * Enabling RX trigger interrupts does not cause an interrupt
		 * if the FIFO already contains RX data.
		 * Generate a soft interrupt and have it call the
		 * callback function in timer isr context.
		 */
		k_timer_start(&dev_data->timer, K_NO_WAIT, K_NO_WAIT);
	}
}

static void uart_cdns_irq_rx_disable(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	sys_write32(CDNS_IXR_RTRIG, reg_base + CDNS_IDR_OFFSET);
}

static int uart_cdns_irq_rx_ready(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t reg_val = sys_read32(reg_base + CDNS_SR_OFFSET);

	return (reg_val & CDNS_SR_RXEMPTY) == 0;
}

static void uart_cdns_irq_err_enable(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	sys_write32(CDNS_IXR_RBRK              /* [13] Receiver Break */
			    | CDNS_IXR_TOVR    /* [12] Transmitter FIFO Overflow */
			    | CDNS_IXR_TOUT    /* [8]  Receiver Timerout */
			    | CDNS_IXR_PARITY  /* [7]  Parity Error */
			    | CDNS_IXR_FRAMING /* [6]  Receiver Framing Error */
			    | CDNS_IXR_RXOVR,  /* [5]  Receiver Overflow Error */
		    reg_base + CDNS_IER_OFFSET);
}

static void uart_cdns_irq_err_disable(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	sys_write32(CDNS_IXR_RBRK              /* [13] Receiver Break */
			    | CDNS_IXR_TOVR    /* [12] Transmitter FIFO Overflow */
			    | CDNS_IXR_TOUT    /* [8]  Receiver Timerout */
			    | CDNS_IXR_PARITY  /* [7]  Parity Error */
			    | CDNS_IXR_FRAMING /* [6]  Receiver Framing Error */
			    | CDNS_IXR_RXOVR,  /* [5]  Receiver Overflow Error */
		    reg_base + CDNS_IDR_OFFSET);
}

static int uart_cdns_irq_is_pending(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t reg_imr = sys_read32(reg_base + CDNS_IMR_OFFSET);
	uint32_t reg_isr = sys_read32(reg_base + CDNS_ISR_OFFSET);

	if ((reg_imr & reg_isr) != 0) {
		return 1;
	} else {
		return 0;
	}
}

static void uart_cdns_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_cdns_dev_data_t *dev_data = dev->data;

	dev_data->user_cb = cb;
	dev_data->user_data = cb_data;
}

static void uart_cdns_isr(const struct device *dev)
{
	const struct uart_cdns_dev_data_t *data = dev->data;

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_cdns_driver_api) = {
	.poll_in = uart_cdns_poll_in,
	.poll_out = uart_cdns_poll_out,
	.err_check = uart_cdns_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_cdns_configure,
	.config_get = uart_cdns_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_cdns_fifo_fill,
	.fifo_read = uart_cdns_fifo_read,
	.irq_tx_enable = uart_cdns_irq_tx_enable,
	.irq_tx_disable = uart_cdns_irq_tx_disable,
	.irq_tx_ready = uart_cdns_irq_tx_ready,
	.irq_tx_complete = uart_cdns_irq_tx_complete,
	.irq_rx_enable = uart_cdns_irq_rx_enable,
	.irq_rx_disable = uart_cdns_irq_rx_disable,
	.irq_rx_ready = uart_cdns_irq_rx_ready,
	.irq_err_enable = uart_cdns_irq_err_enable,
	.irq_err_disable = uart_cdns_irq_err_disable,
	.irq_is_pending = uart_cdns_irq_is_pending,
	.irq_callback_set = uart_cdns_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define UART_CDNS_IRQ_CONF_FUNC_SET(port) .irq_config_func = uart_cdns_irq_config_##port,

#define UART_CDNS_IRQ_CONF_FUNC(port)                                                              \
	static void uart_cdns_irq_config_##port(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(port), DT_INST_IRQ(port, priority), uart_cdns_isr,        \
			    DEVICE_DT_INST_GET(port), 0);                                          \
		irq_enable(DT_INST_IRQN(port));                                                    \
	}

#else

#define UART_CDNS_IRQ_CONF_FUNC_SET(port)
#define UART_CDNS_IRQ_CONF_FUNC(port)

#endif /*CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_CDNS_DEV_DATA(port)                                                                   \
	static struct uart_cdns_dev_data_t uart_cdns_dev_data_##port = {                           \
		.baud_rate = DT_INST_PROP(port, current_speed),                                    \
	}

#define UART_CDNS_PINCTRL_DEFINE(port)                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(port, pinctrl_0),                                        \
		    (PINCTRL_DT_INST_DEFINE(port);), ())

#define UART_CDNS_PINCTRL_INIT(port)                                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(port, pinctrl_0),                                        \
		    (.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(port),), ())

#define UART_CDNS_DEV_CFG(port)                                                                    \
	static struct uart_cdns_dev_config uart_cdns_dev_cfg_##port = {                            \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(port)),                                           \
		.sys_clk_freq = DT_INST_PROP(port, clock_frequency),                               \
		UART_CDNS_IRQ_CONF_FUNC_SET(port) UART_CDNS_PINCTRL_INIT(port)}

#define UART_CDNS_INIT(port)                                                                       \
	DEVICE_DT_INST_DEFINE(port, uart_cdns_init, NULL, &uart_cdns_dev_data_##port,              \
			      &uart_cdns_dev_cfg_##port, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_cdns_driver_api)

#define UART_CDNS_INSTANTIATE(inst)                                                                \
	UART_CDNS_PINCTRL_DEFINE(inst)                                                             \
	UART_CDNS_IRQ_CONF_FUNC(inst);                                                             \
	UART_CDNS_DEV_DATA(inst);                                                                  \
	UART_CDNS_DEV_CFG(inst);                                                                   \
	UART_CDNS_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(UART_CDNS_INSTANTIATE)
