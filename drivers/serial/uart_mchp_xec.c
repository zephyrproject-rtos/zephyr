/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020 Intel Corp.
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Microchip XEC UART Serial Driver
 *
 * This is the driver for the Microchip XEC MCU UART. It is mostly NS16550 compatible.
 *
 */

#define DT_DRV_COMPAT microchip_xec_uart

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_xec, CONFIG_UART_LOG_LEVEL);

#define XEC_UART_FIN_HZ            1843200u
#define XEC_UART_FIN_HS_HZ         48000000u
#define XEC_UART_BRG_ROUNDING_MULT 16u
#define XEC_UART_BRG_ROUNDING_MSK  0xfu
#define XEC_UART_BRG_ROUNDING_UP   8u
#define XEC_UART_FIN_HZ_RM         (XEC_UART_FIN_HZ * XEC_UART_BRG_ROUNDING_MULT)
#define XEC_UART_FIN_HS_HZ_RM      (XEC_UART_FIN_HS_HZ * XEC_UART_BRG_ROUNDING_MULT)
#define XEC_UART_BRG_HW_MULT       16u
#define XEC_UART_MAX_BAUD          115200u
#define XEC_UART_MAX_HS_BAUD       3000000u
#define XEC_UART_BRG_DIV_MSK       0x7fffu
#define XEC_UART_BRD_DIV_HS_POS    15

enum uart_xec_pm_policy_state_flag {
	UART_XEC_PM_POLICY_STATE_TX_FLAG,
	UART_XEC_PM_POLICY_STATE_RX_FLAG,
	UART_XEC_PM_POLICY_STATE_FLAG_COUNT,
};

/* device config */
struct uart_xec_device_config {
	mm_reg_t uart_base;
	uint32_t sys_clk_freq;
	uint8_t girq_id;
	uint8_t girq_pos;
	uint8_t enc_pcr;
	const struct pinctrl_dev_config *pcfg;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif
#ifdef CONFIG_PM_DEVICE
	struct gpio_dt_spec wakerx_gpio;
	bool wakeup_source;
#endif
};

/** Device data structure */
struct uart_xec_dev_data {
	struct uart_config uart_config;
	struct k_spinlock lock;

	uint8_t fcr_cache; /**< cache of FCR write only register */
	uint8_t iir_cache; /**< cache of IIR since it clears when read */
	volatile uint8_t data_byte;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *cb_data;                    /**< Callback function arg */
#endif
};

#ifdef CONFIG_PM_DEVICE
ATOMIC_DEFINE(pm_policy_state_flag, UART_XEC_PM_POLICY_STATE_FLAG_COUNT);
#endif

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
struct k_work_delayable rx_refresh_timeout_work;
#endif

static DEVICE_API(uart, uart_xec_driver_api);

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
static void uart_xec_pm_policy_state_lock_get(enum uart_xec_pm_policy_state_flag flag)
{
	if (atomic_test_and_set_bit(pm_policy_state_flag, flag) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void uart_xec_pm_policy_state_lock_put(enum uart_xec_pm_policy_state_flag flag)
{
	if (atomic_test_and_clear_bit(pm_policy_state_flag, flag) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#endif

/* Calculate the baud clock divisor given the desired BAUD rate.
 * Hardware design is divisor = Fin / (16 * baud_rate)
 * Fin is selectable as 1.8432 MHz (divisor b[15]=0) or 48 MHz (divisor b[15]=1)
 * We multiply Fin by 16 and look at the lower 4 bits to implement rounding.
 */
static uint32_t calc_baud_clock_divisor(uint32_t baud_rate)
{
	uint32_t fin = XEC_UART_FIN_HZ_RM;
	uint32_t bdiv = 0, d = 0;

	if (baud_rate > XEC_UART_MAX_BAUD) {
		fin = XEC_UART_FIN_HS_HZ_RM;
		if (baud_rate > XEC_UART_MAX_HS_BAUD) {
			baud_rate = XEC_UART_MAX_HS_BAUD;
		}
	}

	d = fin / (XEC_UART_BRG_HW_MULT * baud_rate);
	bdiv = d / XEC_UART_BRG_ROUNDING_MULT;
	if ((d & XEC_UART_BRG_ROUNDING_MSK) >= XEC_UART_BRG_ROUNDING_UP) {
		bdiv++;
	}

	bdiv &= XEC_UART_BRG_DIV_MSK;

	if (baud_rate > XEC_UART_MAX_BAUD) {
		bdiv |= BIT(XEC_UART_BRD_DIV_HS_POS);
	}

	return bdiv;
}

static void set_baud_rate(const struct device *dev, uint32_t baud_rate)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *const dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	uint32_t divisor; /* baud rate divisor */
	uint8_t lcr_cache;

	if ((baud_rate != 0U) && (dev_cfg->sys_clk_freq != 0U)) {
		divisor = calc_baud_clock_divisor(baud_rate);

		/* set the DLAB to access the baud rate divisor registers */
		lcr_cache = sys_read8(ub + XEC_UART_LCR_OFS);
		sys_write8(XEC_UART_LCR_DLAB_EN | lcr_cache, ub + XEC_UART_LCR_OFS);

		sys_write8((uint8_t)(divisor & 0xffu), ub + XEC_UART_BRGD_LSB_OFS);
		sys_write8((uint8_t)((divisor >> 8) & 0xffu), ub + XEC_UART_BRGD_MSB_OFS);

		/* restore the DLAB to access the baud rate divisor registers */
		sys_write8(lcr_cache, ub + XEC_UART_LCR_OFS);

		dev_data->uart_config.baudrate = baud_rate;
	}
}

/*
 * Configure UART.
 * MCHP XEC UART defaults to reset if external Host VCC_PWRGD is inactive.
 * We must change the UART reset signal to XEC VTR_PWRGD. Make sure UART
 * clock source is an internal clock and UART pins are not inverted.
 */
static int uart_xec_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_xec_dev_data *const dev_data = dev->data;
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	mm_reg_t ub = dev_cfg->uart_base;
	int ret = 0;
	uint8_t lcr = 0, temp8 = 0;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->fcr_cache = 0U;
	dev_data->iir_cache = 0U;

	/* XEC UART specific configuration and enable */
	temp8 = sys_read8(ub + XEC_UART_LD_CFG_OFS);
	temp8 &= ~(XEC_UART_LD_CFG_RESET_VCC | XEC_UART_LD_CFG_EXTCLK | XEC_UART_LD_CFG_INVERT);
	sys_write8(temp8, ub + XEC_UART_LD_CFG_OFS);

	/* set activate to enable clocks */
	soc_set_bit8(ub + XEC_UART_LD_ACT_OFS, XEC_UART_LD_ACTIVATE_POS);

	set_baud_rate(dev, cfg->baudrate);

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		lcr |= XEC_UART_LCR_WORD_LEN_SET(XEC_UART_LCR_WORD_LEN_5);
		break;
	case UART_CFG_DATA_BITS_6:
		lcr |= XEC_UART_LCR_WORD_LEN_SET(XEC_UART_LCR_WORD_LEN_6);
		break;
	case UART_CFG_DATA_BITS_7:
		lcr |= XEC_UART_LCR_WORD_LEN_SET(XEC_UART_LCR_WORD_LEN_7);
		break;
	case UART_CFG_DATA_BITS_8:
		lcr |= XEC_UART_LCR_WORD_LEN_SET(XEC_UART_LCR_WORD_LEN_8);
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		lcr |= XEC_UART_LCR_STOP_BIT_1;
		break;
	case UART_CFG_STOP_BITS_2:
		lcr |= XEC_UART_LCR_STOP_BIT_2;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		lcr |= XEC_UART_LCR_PARITY_SET(XEC_UART_LCR_PARITY_NONE);
		break;
	case UART_CFG_PARITY_ODD:
		lcr |= XEC_UART_LCR_PARITY_SET(XEC_UART_LCR_PARITY_ODD);
		break;
	case UART_CFG_PARITY_EVEN:
		lcr |= XEC_UART_LCR_PARITY_SET(XEC_UART_LCR_PARITY_EVEN);
		break;
	case UART_CFG_PARITY_MARK:
		lcr |= XEC_UART_LCR_PARITY_SET(XEC_UART_LCR_PARITY_MARK);
		break;
	case UART_CFG_PARITY_SPACE:
		lcr |= XEC_UART_LCR_PARITY_SET(XEC_UART_LCR_PARITY_SPACE);
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	dev_data->uart_config = *cfg;

	/* data bits, stop bits, parity, clear DLAB */
	sys_write8(lcr, ub + XEC_UART_LCR_OFS);

	/* modem control */
	temp8 = (XEC_UART_MCR_OUT2 | XEC_UART_MCR_RTSn | XEC_UART_MCR_DTRn);
	sys_write8(temp8, ub + XEC_UART_MCR_OFS);

	/*
	 * Program FIFO: enabled, mode 0
	 * generate the interrupt at 8th byte
	 * Clear TX and RX FIFO
	 */
	dev_data->fcr_cache = (XEC_UART_FCR_EXRF | XEC_UART_FCR_RX_FIFO_LVL_8 |
			       XEC_UART_FCR_CLR_RX_FIFO | XEC_UART_FCR_CLR_TX_FIFO);

	sys_write8(dev_data->fcr_cache, ub + XEC_UART_FCR_OFS);

	/* clear the port */
	if (soc_test_bit8(ub + XEC_UART_LSR_OFS, XEC_UART_LSR_DATA_RDY_POS) != 0) {
		dev_data->data_byte = sys_read8(ub + XEC_UART_RTXB_OFS);
	}

	/* disable interrupts  */
	sys_write8(0, ub + XEC_UART_IER_OFS);

out:
	k_spin_unlock(&dev_data->lock, key);

	return ret;
};

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_xec_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_xec_dev_data *data = dev->data;

	cfg->baudrate = data->uart_config.baudrate;
	cfg->parity = data->uart_config.parity;
	cfg->stop_bits = data->uart_config.stop_bits;
	cfg->data_bits = data->uart_config.data_bits;
	cfg->flow_ctrl = data->uart_config.flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_PM_DEVICE

static void uart_xec_wake_handler(const struct device *gpio, struct gpio_callback *cb,
				  uint32_t pins)
{
	/* Disable interrupts on UART RX pin to avoid repeated interrupts. */
	(void)gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1), GPIO_INT_DISABLE);
	/* Refresh console expired time */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

	uart_xec_pm_policy_state_lock_get(UART_XEC_PM_POLICY_STATE_RX_FLAG);
	k_work_reschedule(&rx_refresh_timeout_work, delay);
#endif
}

static int uart_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	mm_reg_t ub = dev_cfg->uart_base;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		soc_set_bit8(ub + XEC_UART_LD_ACT_OFS, XEC_UART_LD_ACTIVATE_POS);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Enable UART wake interrupt */
		soc_clear_bit8(ub + XEC_UART_LD_ACT_OFS, XEC_UART_LD_ACTIVATE_POS);
		if ((dev_cfg->wakeup_source) && (dev_cfg->wakerx_gpio.port != NULL)) {
			ret = gpio_pin_interrupt_configure_dt(
				&dev_cfg->wakerx_gpio, GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_LOW);
			if (ret < 0) {
				LOG_ERR("Failed to configure UART wake interrupt (ret %d)", ret);
				return ret;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
static void uart_xec_rx_refresh_timeout(struct k_work *work)
{
	ARG_UNUSED(work);

	uart_xec_pm_policy_state_lock_put(UART_XEC_PM_POLICY_STATE_RX_FLAG);
}
#endif
#endif /* CONFIG_PM_DEVICE */

/* Initialize individual UART port
 * params: dev UART device struct
 * return: 0 if successful, failed otherwise
 * This routine is called to reset the chip in a quiescent state.
 */
static int uart_xec_init(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	int ret;

	soc_xec_pcr_sleep_en_clear(dev_cfg->enc_pcr);

	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = uart_xec_configure(dev, &dev_data->uart_config);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_PM_DEVICE
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	k_work_init_delayable(&rx_refresh_timeout_work, uart_xec_rx_refresh_timeout);
#endif
	if ((dev_cfg->wakeup_source) && (dev_cfg->wakerx_gpio.port != NULL)) {
		static struct gpio_callback uart_xec_wake_cb;

		gpio_init_callback(&uart_xec_wake_cb, uart_xec_wake_handler,
				   BIT(dev_cfg->wakerx_gpio.pin));

		ret = gpio_add_callback(dev_cfg->wakerx_gpio.port, &uart_xec_wake_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add UART wake callback (err %d)", ret);
			return ret;
		}
	}
#endif

	return 0;
}

/* Poll the device for input.
 * params:
 *  dev UART device struct
 *  c Pointer to character
 * return: 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_xec_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	int ret = -1;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if ((soc_test_bit8(ub + XEC_UART_LSR_OFS, XEC_UART_LSR_DATA_RDY_POS)) != 0) {
		*c = (unsigned char)sys_read8(ub + XEC_UART_RTXB_OFS);
		ret = 0;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/* Output a character in polled mode.
 * params:
 *  dev a pointer to UART device structure
 *  c unsigned character to be written to HW
 * return: None
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 * If the hardware flow control is enabled then the handshake signal CTS has to
 * be asserted in order to send a character.
 */
static void uart_xec_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

#ifdef XEC_HAS_UART_LSR2
	if ((sys_read8(ub + XEC_UART_IIR_OFS) & XEC_UART_IIR_FIFO_EN_MASK) != 0) {
		/* When the FIFO is disabled this bit is always 0 */
		while (soc_test_bit8(ub + XEC_UART_LSR2_OFS, XEC_UART_LSR2_TX_FIFO_FULL_POS) != 0) {
		}
	} else {
		while (soc_test_bit8(ub + XEC_UART_LSR_OFS, XEC_UART_LSR_THRE_POS) == 0) {
		}
	}
#else
	while (soc_test_bit8(ub + XEC_UART_LSR_OFS, XEC_UART_LSR_THRE_POS) == 0) {
	}
#endif

	sys_write8(c, ub + XEC_UART_RTXB_OFS);

	k_spin_unlock(&dev_data->lock, key);
}

/* Check if an error was received
 * params: dev UART device struct
 * return:  one of UART_ERROR_OVERRUN, UART_ERROR_PARITY, UART_ERROR_FRAMING,
 * UART_BREAK if an error was detected, 0 otherwise.
 */
static int uart_xec_err_check(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	uint32_t lsr = 0;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	lsr = (uint32_t)sys_read8(ub + XEC_UART_LSR_OFS) & XEC_UART_LSR_ANY_ERR;

	k_spin_unlock(&dev_data->lock, key);

	return (int)((lsr & XEC_UART_LSR_ANY_ERR) >> 1);
}

#if CONFIG_UART_INTERRUPT_DRIVEN

static bool uart_xec_ready_to_xmit(mm_reg_t base, bool fifo_enabled)
{
#ifdef XEC_HAS_UART_LSR2
	if (fifo_enabled == true) {
		if (soc_test_bit8(base + XEC_UART_LSR2_OFS, XEC_UART_LSR2_TX_FIFO_FULL_POS) == 0) {
			return true;
		} else {
			return false;
		}
	}
#endif
	if (soc_test_bit8(base + XEC_UART_LSR_OFS, XEC_UART_LSR_THRE_POS) != 0) {
		return true;
	}

	return false;
}

/* Fill FIFO with data
 * params:
 *   dev UART device struct
 *   tx_data Data to transmit
 *   size Number of bytes to send
 * return: Number of bytes sent
 * Do NOT block. If TX FIFO is full return 0.
 */
static int uart_xec_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	int i = 0;
	bool fifo_enabled = false;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if ((sys_read8(ub + XEC_UART_IIR_OFS) & XEC_UART_IIR_FIFO_EN_MASK) != 0) {
		fifo_enabled = true;
	}

	while (i < size) {
		if (uart_xec_ready_to_xmit(ub, fifo_enabled) == true) {
#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
			uart_xec_pm_policy_state_lock_get(UART_XEC_PM_POLICY_STATE_TX_FLAG);
#endif
			sys_write8(tx_data[i], ub + XEC_UART_RTXB_OFS);
			i++;
		} else {
			break;
		}
	}

	k_spin_unlock(&dev_data->lock, key);

	return i;
}

/* Read data from FIFO
 * params:
 *   dev UART device struct
 *   rxData Data container
 *   size Container size
 * return: Number of bytes read
 * Do NOT block. If no data ready return 0
 */
static int uart_xec_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	int i = 0;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	while (i < size) {
		if (soc_test_bit8(ub + XEC_UART_LSR_OFS, XEC_UART_LSR_DATA_RDY_POS) != 0) {
			rx_data[i] = sys_read8(ub + XEC_UART_RTXB_OFS);
			i++;
		} else {
			break;
		}
	}

	k_spin_unlock(&dev_data->lock, key);

	return i;
}

/* Enable TX interrupt in IER
 * params: dev UART device struct
 */
static void uart_xec_irq_tx_enable(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	soc_set_bit8(ub + XEC_UART_IER_OFS, XEC_UART_IER_ETHREI_POS);

	k_spin_unlock(&dev_data->lock, key);
}

/* Disable TX interrupt in IER
 * params: dev UART device struct
 */
static void uart_xec_irq_tx_disable(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	soc_clear_bit8(ub + XEC_UART_IER_OFS, XEC_UART_IER_ETHREI_POS);

	k_spin_unlock(&dev_data->lock, key);
}

/* Check if Tx IRQ has been raised
 * params: dev UART device struct
 * return: 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xec_irq_tx_ready(const struct device *dev)
{
	struct uart_xec_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);
	int ret = 0;
	uint8_t iid = XEC_UART_IIR_INTID_GET(dev_data->iir_cache);

	if (iid == XEC_UART_IIR_INTID_THRE) {
		ret = 1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/* Check if nothing remains to be transmitted
 * params: dev UART device struct
 * return: 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_xec_irq_tx_complete(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	int ret = 0;
	uint8_t ier = 0, lsr = 0;
	uint8_t lsr_msk = (XEC_UART_LSR_THRE | XEC_UART_LSR_TEMT);
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	ier = sys_read8(ub + XEC_UART_IER_OFS);
	lsr = sys_read8(ub + XEC_UART_LSR_OFS);

	/* TX FIFO holding register empty interrupt enabled OR
	 * both TX holding and shift registers are empty.
	 */
	if (((ier & XEC_UART_IER_ETHREI) != 0) || ((lsr & lsr_msk) == lsr_msk)) {
		ret = 0;
	} else {
		ret = 1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/* Enable RX interrupt in IER
 * params: dev UART device struct
 */
static void uart_xec_irq_rx_enable(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	soc_set_bit8(ub + XEC_UART_IER_OFS, XEC_UART_IER_ERDAI_POS);

	k_spin_unlock(&dev_data->lock, key);
}

/* Disable RX interrupt in IER
 * params: dev UART device struct
 */
static void uart_xec_irq_rx_disable(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	soc_clear_bit8(ub + XEC_UART_IER_OFS, XEC_UART_IER_ERDAI_POS);

	k_spin_unlock(&dev_data->lock, key);
}

/*
 * Check if Rx IRQ has been raised
 * params:
 *   dev UART device struct
 * return:
 *   0 No RX IRQ has been raised
 *   1 RX IRQ has been raised
 *   -ENOSYS function not implemented
 *   -ENOTSUP if API is not enabled
 * Notes:
 *  MEC UART is NS16550 compatible and has two possible RX events signalling
 *  an interrupt. RX data is available and RX data is available but no bytes
 *  have been removed from the RX FIFO during the last frames. In both cases
 *  reading the RX buffer will clear the status.
 */
static int uart_xec_irq_rx_ready(const struct device *dev)
{
	struct uart_xec_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);
	int ret = 0;
	uint8_t iid = XEC_UART_IIR_INTID_GET(dev_data->iir_cache);

	if ((iid == XEC_UART_IIR_INTID_RXD) || (iid == XEC_UART_IIR_INTID_RXTM)) {
		ret = 1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/* Enable error interrupt in IER
 * params: dev UART device struct
 */
static void uart_xec_irq_err_enable(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	soc_set_bit8(ub + XEC_UART_IER_OFS, XEC_UART_IER_ELSI_POS);

	k_spin_unlock(&dev_data->lock, key);
}

/* Disable error interrupt in IER
 * params: dev UART device struct
 */
static void uart_xec_irq_err_disable(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	soc_clear_bit8(ub + XEC_UART_IER_OFS, XEC_UART_IER_ELSI_POS);

	k_spin_unlock(&dev_data->lock, key);
}

/* Check if any IRQ is pending
 * params: dev UART device struct
 * return:  1 if an IRQ is pending, 0 otherwise
 */
static int uart_xec_irq_is_pending(const struct device *dev)
{
	struct uart_xec_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);
	int ret = 0;

	if ((dev_data->iir_cache & BIT(XEC_UART_IIR_NOT_IPEND_POS)) == 0) {
		ret = 1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/* Update cached contents of IIR
 * param:  dev UART device struct
 * return: Always 1
 */
static int uart_xec_irq_update(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->iir_cache = sys_read8(ub + XEC_UART_IIR_OFS);

	k_spin_unlock(&dev_data->lock, key);

	return 1;
}

/* Set the callback function pointer for IRQ.
 * params:
 *  dev UART device struct
 *  cb Callback function pointer.
 *  cb_data pointer to opaque user data
 */
static void uart_xec_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_xec_dev_data *const dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;

	k_spin_unlock(&dev_data->lock, key);
}

/* UART interrupt service routine.
 * params: dev pointer to UART device structure
 * This simply calls the callback function, if one exists.
 * If Zephyr PM_DEVICE power management enabled and console input expired
 * timeout is enabled in the build we ask the kernel to run a helper function
 * in its work queue thread.
 */
static void uart_xec_isr(const struct device *dev)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *const dev_data = dev->data;

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
	mm_reg_t ub = dev_cfg->uart_base;
	uint8_t lsr = sys_read8(ub + XEC_UART_LSR_OFS);

	if ((lsr & XEC_UART_LSR_DATA_RDY) != 0) {
		k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

		uart_xec_pm_policy_state_lock_get(UART_XEC_PM_POLICY_STATE_RX_FLAG);
		k_work_reschedule(&rx_refresh_timeout_work, delay);
	}
#endif

	if (dev_data->cb != NULL) {
		dev_data->cb(dev, dev_data->cb_data);
	}

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
	if (uart_xec_irq_tx_complete(dev) != 0) {
		uart_xec_pm_policy_state_lock_put(UART_XEC_PM_POLICY_STATE_TX_FLAG);
	}
#endif /* CONFIG_PM */

	/* clear ECIA GIRQ R/W1C status bit after UART status cleared */
	soc_ecia_girq_status_clear(dev_cfg->girq_id, dev_cfg->girq_pos);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_XEC_LINE_CTRL

/* Manipulate line control for UART.
 * params:
 *  dev UART device struct
 *  ctrl The line control to be manipulated
 *  val Value to set the line control
 * return: 0 if successful, failed otherwise
 */
static int uart_xec_line_ctrl_set(const struct device *dev, uint32_t ctrl, uint32_t val)
{
	const struct uart_xec_device_config *const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	mm_reg_t ub = dev_cfg->uart_base;
	uint32_t mdc = 0, chg = 0;
	k_spinlock_key_t key;

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		set_baud_rate(dev, val);
		return 0;

	case UART_LINE_CTRL_RTS:
	case UART_LINE_CTRL_DTR:
		key = k_spin_lock(&dev_data->lock);

		mdc = (uint32_t)sys_read8(ub + XEC_UART_MCR_OFS);

		if (ctrl == UART_LINE_CTRL_RTS) {
			chg = MCR_RTS;
		} else {
			chg = MCR_DTR;
		}

		if (val) {
			mdc |= chg;
		} else {
			mdc &= ~(chg);
		}

		sys_write8(mdc, ub + XEC_UART_MCR_OFS);

		k_spin_unlock(&dev_data->lock, key);

		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_XEC_LINE_CTRL */

static DEVICE_API(uart, uart_xec_driver_api) = {
	.poll_in = uart_xec_poll_in,
	.poll_out = uart_xec_poll_out,
	.err_check = uart_xec_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_xec_configure,
	.config_get = uart_xec_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = uart_xec_fifo_fill,
	.fifo_read = uart_xec_fifo_read,
	.irq_tx_enable = uart_xec_irq_tx_enable,
	.irq_tx_disable = uart_xec_irq_tx_disable,
	.irq_tx_ready = uart_xec_irq_tx_ready,
	.irq_tx_complete = uart_xec_irq_tx_complete,
	.irq_rx_enable = uart_xec_irq_rx_enable,
	.irq_rx_disable = uart_xec_irq_rx_disable,
	.irq_rx_ready = uart_xec_irq_rx_ready,
	.irq_err_enable = uart_xec_irq_err_enable,
	.irq_err_disable = uart_xec_irq_err_disable,
	.irq_is_pending = uart_xec_irq_is_pending,
	.irq_update = uart_xec_irq_update,
	.irq_callback_set = uart_xec_irq_callback_set,

#endif

#ifdef CONFIG_UART_XEC_LINE_CTRL
	.line_ctrl_set = uart_xec_line_ctrl_set,
#endif
};

#define DEV_CONFIG_REG_INIT(n) .uart_base = (mm_reg_t)(DT_INST_REG_ADDR(n)),

#define DEV_CFG_GIRQ(inst)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0))
#define DEV_CFG_GIRQ_POS(inst) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, 0))

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define DEV_CONFIG_IRQ_FUNC_INIT(n)  .irq_config_func = irq_config_func##n,
#define UART_XEC_IRQ_FUNC_DECLARE(n) static void irq_config_func##n(const struct device *dev);
#define UART_XEC_IRQ_FUNC_DEFINE(n)                                                                \
	static void irq_config_func##n(const struct device *dev)                                   \
	{                                                                                          \
		const struct uart_xec_device_config *devcfg = dev->config;                         \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_xec_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
		soc_ecia_girq_ctrl(devcfg->girq_id, devcfg->girq_pos, 1u);                         \
	}
#else
/* !CONFIG_UART_INTERRUPT_DRIVEN */
#define DEV_CONFIG_IRQ_FUNC_INIT(n)
#define UART_XEC_IRQ_FUNC_DECLARE(n)
#define UART_XEC_IRQ_FUNC_DEFINE(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define DEV_DATA_FLOW_CTRL(n) DT_INST_PROP_OR(n, hw_flow_control, UART_CFG_FLOW_CTRL_NONE)

/* To enable wakeup on the UART, the DTS needs to have two entries defined
 * in the corresponding UART node in the DTS specifying it as a wake source
 * and specifying the UART_RX GPIO; example as below
 *
 *	wakerx-gpios = <&gpio_140_176 25 GPIO_ACTIVE_HIGH>;
 *	wakeup-source;
 */
#ifdef CONFIG_PM_DEVICE
#define XEC_UART_PM_WAKEUP(n)                                                                      \
	.wakeup_source = (uint8_t)DT_INST_PROP_OR(n, wakeup_source, 0),                            \
	.wakerx_gpio = GPIO_DT_SPEC_INST_GET_OR(n, wakerx_gpios, {0}),
#else
#define XEC_UART_PM_WAKEUP(index) /* Not used */
#endif

#define UART_XEC_DEVICE_INIT(n)                                                                    \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	UART_XEC_IRQ_FUNC_DECLARE(n);                                                              \
                                                                                                   \
	static const struct uart_xec_device_config uart_xec_dev_cfg_##n = {                        \
		DEV_CONFIG_REG_INIT(n).sys_clk_freq = DT_INST_PROP(n, clock_frequency),            \
		.girq_id = DEV_CFG_GIRQ(n),                                                        \
		.girq_pos = DEV_CFG_GIRQ_POS(n),                                                   \
		.enc_pcr = DT_INST_PROP(n, pcr_scr),                                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		XEC_UART_PM_WAKEUP(n) DEV_CONFIG_IRQ_FUNC_INIT(n)};                                \
	static struct uart_xec_dev_data uart_xec_dev_data_##n = {                                  \
		.uart_config.baudrate = DT_INST_PROP_OR(n, current_speed, 0),                      \
		.uart_config.parity = UART_CFG_PARITY_NONE,                                        \
		.uart_config.stop_bits = UART_CFG_STOP_BITS_1,                                     \
		.uart_config.data_bits = UART_CFG_DATA_BITS_8,                                     \
		.uart_config.flow_ctrl = DEV_DATA_FLOW_CTRL(n),                                    \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, uart_xec_pm_action);                                           \
	DEVICE_DT_INST_DEFINE(n, uart_xec_init, PM_DEVICE_DT_INST_GET(n), &uart_xec_dev_data_##n,  \
			      &uart_xec_dev_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_xec_driver_api);                                               \
	UART_XEC_IRQ_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(UART_XEC_DEVICE_INIT)
