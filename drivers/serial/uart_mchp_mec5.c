/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020 Intel Corp.
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Microchip MEC5 ns16550 compatible UART Serial Driver
 */

#include "zephyr/devicetree.h"
#define DT_DRV_COMPAT microchip_mec5_uart

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>
#include <soc.h>

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_mec5, CONFIG_UART_LOG_LEVEL);

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_ecia_api.h>
#include <mec_uart_api.h>

enum uart_mec5_pm_policy_state_flag {
	UART_MEC5_PM_POLICY_STATE_TX_FLAG,
	UART_MEC5_PM_POLICY_STATE_RX_FLAG,
	UART_MEC5_PM_POLICY_STATE_FLAG_COUNT,
};

/* device config */
struct uart_mec5_device_config {
	struct uart_regs *base;
	uint32_t clock_freq;
	uint8_t use_ext_clk;
	uint8_t fifo_dis;
	uint8_t rx_fifo_trig;
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
struct uart_mec5_dev_data {
	struct uart_config ucfg;
	struct k_spinlock lock;

	enum mec_uart_ipend ipend;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;  /**< Callback function pointer */
	void *cb_data;	/**< Callback function arg */
#endif
};

#ifdef CONFIG_PM_DEVICE
	ATOMIC_DEFINE(pm_policy_state_flag, UART_MEC5_PM_POLICY_STATE_FLAG_COUNT);
#endif

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
	struct k_work_delayable rx_refresh_timeout_work;
#endif

static const struct uart_driver_api uart_mec5_driver_api;

#ifdef CONFIG_PM_DEVICE
static void uart_mec5_pm_policy_state_lock_get(enum uart_mec5_pm_policy_state_flag flag)
{
	if (atomic_test_and_set_bit(pm_policy_state_flag, flag) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void uart_mec5_pm_policy_state_lock_put(enum uart_mec5_pm_policy_state_flag flag)
{
	if (atomic_test_and_clear_bit(pm_policy_state_flag, flag) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#endif

static const uint8_t mec5_xlat_word_len[4] = {
	MEC_UART_WORD_LEN_5, MEC_UART_WORD_LEN_6,
	MEC_UART_WORD_LEN_7, MEC_UART_WORD_LEN_8,
};

static const uint8_t mec5_xlat_stop_bits[4] = {
	MEC_UART_STOP_BITS_1, MEC_UART_STOP_BITS_1,
	MEC_UART_STOP_BITS_2, MEC_UART_STOP_BITS_2,
};

static const uint8_t mec5_xlat_parity[5] = {
	(uint8_t)(MEC5_UART_CFG_PARITY_NONE >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_ODD >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_EVEN >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_MARK >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_SPACE >> MEC5_UART_CFG_PARITY_POS),
};

static int uart_mec5_xlat_cfg(const struct uart_config *cfg, uint32_t *cfg_word)
{
	uint32_t temp;

	if (!cfg || !cfg_word) {
		return -EINVAL;
	}

	*cfg_word = 0u;

	if (cfg->data_bits > UART_CFG_DATA_BITS_8) {
		return -EINVAL;
	}
	temp = mec5_xlat_word_len[cfg->data_bits];
	*cfg_word |= ((temp << MEC5_UART_CFG_WORD_LEN_POS) & MEC5_UART_CFG_WORD_LEN_MSK);

	if (cfg->stop_bits > UART_CFG_STOP_BITS_2) {
		return -EINVAL;
	}
	temp = mec5_xlat_stop_bits[cfg->stop_bits];
	*cfg_word |= ((temp << MEC5_UART_CFG_STOP_BITS_POS) & MEC5_UART_CFG_STOP_BITS_MSK);

	if (cfg->parity > UART_CFG_PARITY_SPACE) {
		return -EINVAL;
	}
	temp = mec5_xlat_parity[cfg->parity];
	*cfg_word |= ((temp << MEC5_UART_CFG_PARITY_POS) & MEC5_UART_CFG_PARITY_MSK);

	return 0;
}

/* Configure UART TX and RX FIFOs
 * Both FIFOs are fixed 16-byte.
 * Set RX threshold to 8 bytes.
 */
static uint32_t uart_mec5_fifo_config(uint32_t mcfg)
{
	uint32_t new_mcfg = mcfg;

	new_mcfg |= BIT(MEC5_UART_CFG_FIFO_EN_POS);
	new_mcfg |= MEC5_UART_CFG_RX_FIFO_TRIG_LVL_8;

	return new_mcfg;
}

/* Configure MEC5 UART based upon baud rate, parity, stop bits, data width,
 * and flow control specified in uart configuration structure.
 * Zephyr does not implement configuration items for UART FIFOs.
 * MEC5 TX and RX FIFOs are fixed at 16 bytes and we choose RX FIFO
 * interrupt threshold of 8 bytes.
 */
static int uart_mec5_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_mec5_dev_data * const dev_data = dev->data;
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_regs *base = dev_cfg->base;
	int ret = 0;
	uint32_t mcfg = 0;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->ipend = MEC_UART_IPEND_NONE;

	ret = uart_mec5_xlat_cfg(cfg, &mcfg);
	if (ret) {
		return ret;
	}

	mcfg = uart_mec5_fifo_config(mcfg);

	ret = mec_uart_init(base, cfg->baudrate, mcfg, 0);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	memcpy(&dev_data->ucfg, cfg, sizeof(struct uart_config));

	k_spin_unlock(&dev_data->lock, key);
	return ret;
};

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_mec5_config_get(const struct device *dev, struct uart_config *cfg)
{
	if (!dev || !cfg) {
		return -EINVAL;
	}

	struct uart_mec5_dev_data *data = dev->data;

	cfg->baudrate = data->ucfg.baudrate;
	cfg->parity = data->ucfg.parity;
	cfg->stop_bits = data->ucfg.stop_bits;
	cfg->data_bits = data->ucfg.data_bits;
	cfg->flow_ctrl = data->ucfg.flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_PM_DEVICE

static void uart_mec5_wake_handler(const struct device *gpio,
				   struct gpio_callback *cb, uint32_t pins)
{
	/* Disable interrupts on UART RX pin to avoid repeated interrupts. */
	(void)gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1),
					   GPIO_INT_DISABLE);
	/* Refresh console expired time */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

	uart_mec5_pm_policy_state_lock_get(UART_MEC5_PM_POLICY_STATE_RX_FLAG);
	k_work_reschedule(&rx_refresh_timeout_work, delay);
#endif
}

static int uart_mec5_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_regs *base = dev_cfg->base;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* TODO regs->ACTV = MCHP_UART_LD_ACTIVATE; */
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Enable UART wake interrupt */
		/* TODO regs->ACTV = 0; */
		if ((dev_cfg->wakeup_source) && (dev_cfg->wakerx_gpio.port != NULL)) {
			ret = gpio_pin_interrupt_configure_dt(&dev_cfg->wakerx_gpio,
							      (GPIO_INT_MODE_EDGE
							       | GPIO_INT_TRIG_LOW));
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
static void uart_mec5_rx_refresh_timeout(struct k_work *work)
{
	ARG_UNUSED(work);

	uart_mec5_pm_policy_state_lock_put(UART_MEC5_PM_POLICY_STATE_RX_FLAG);
}
#endif
#endif /* CONFIG_PM_DEVICE */

/**
 * @brief Initialize individual UART port
 *
 * This routine is called to reset the chip in a quiescent state.
 *
 * @param dev UART device struct
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_mec5_init(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;

	int ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		return ret;
	}

	ret = uart_mec5_configure(dev, &dev_data->ucfg);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_PM_DEVICE
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
		k_work_init_delayable(&rx_refresh_timeout_work, uart_mec5_rx_refresh_timeout);
#endif
	if ((dev_cfg->wakeup_source) && (dev_cfg->wakerx_gpio.port != NULL)) {
		static struct gpio_callback uart_mec5_wake_cb;

		gpio_init_callback(&uart_mec5_wake_cb, uart_mec5_wake_handler,
				   BIT(dev_cfg->wakerx_gpio.pin));

		ret = gpio_add_callback(dev_cfg->wakerx_gpio.port, &uart_mec5_wake_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add UART wake callback (err %d)", ret);
			return ret;
		}
	}
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
static int uart_mec5_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	ret = mec_uart_rx_byte(base, (uint8_t *)c);
	if (ret == MEC_RET_OK) {
		ret = 0;
	} else {
		ret = -1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
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
 */
static void uart_mec5_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);
	const uint8_t data = (uint8_t)c;

	mec_uart_tx(base, &data, 1);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if an error was received
 *
 * @param dev UART device struct
 *
 * @return one of UART_ERROR_OVERRUN, UART_ERROR_PARITY, UART_ERROR_FRAMING,
 * UART_BREAK if an error was detected, 0 otherwise.
 */
static int uart_mec5_err_check(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	uint8_t lsr;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	mec_uart_raw_status(base, MEC_UART_STS_REG_LINE, &lsr);
	lsr &= (UART_LSR_OVR_ERR_Msk | UART_LSR_PAR_ERR_Msk
		| UART_LSR_FR_ERR_Msk | UART_LSR_BREAK_Msk);
	ret = (int)(lsr >> 1);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
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
static int uart_mec5_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	int tx_fifo_size = 0, wlen = 0;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	tx_fifo_size = mec_uart_tx_fifo_size(base);

	if (mec_uart_is_tx_fifo_empty(base)) {
#ifdef CONFIG_PM_DEVICE
		uart_mec5_pm_policy_state_lock_get(UART_XEC_PM_POLICY_STATE_TX_FLAG);
#endif
		wlen = (size > tx_fifo_size) ? tx_fifo_size : size;

		mec_uart_tx(base, tx_data, (size_t)wlen);
	}

	k_spin_unlock(&dev_data->lock, key);

	return wlen;
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
static int uart_mec5_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	int i, ret;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	rx_fifo_size = mec_uart_rx_fifo_size(base);

	ret = mec_uart_rx_byte(base, *rx_data)

	for (i = 0; i < size; i++) {
		ret = mec_uart_rx_byte(base, *rx_data);
		if ((ret == MEC_RET_ERR_INVAL) || (ret == MEC_RET_ERR_NO_DATA)) {
			break;
		}
		rx_data++;
	}

	k_spin_unlock(&dev_data->lock, key);

	return i;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_mec5_irq_tx_enable(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	mec_uart_intr_mask(base, MEC_UART_IEN_FLAG_ETHREI, MEC_UART_IEN_FLAG_ETHREI);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_mec5_irq_tx_disable(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	mec_uart_intr_mask(base, MEC_UART_IEN_FLAG_ETHREI, 0);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_mec5_irq_tx_ready(const struct device *dev)
{
	struct uart_mec5_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);
	int ret = 0;

	if (dev_data->ipend == MEC_UART_IPEND_TX) {
		ret = 1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_mec5_irq_tx_complete(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	int ret = mec_uart_is_tx_empty(base);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_mec5_irq_rx_enable(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	mec_uart_intr_mask(base, MEC_UART_IEN_FLAG_ERDAI, MEC_UART_IEN_FLAG_ERDAI);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_mec5_irq_rx_disable(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	mec_uart_intr_mask(base, MEC_UART_IEN_FLAG_ERDAI, 0);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_mec5_irq_rx_ready(const struct device *dev)
{
	struct uart_mec5_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	int ret = 0;

	if (dev_data->ipend == MEC_UART_IPEND_RX_DATA) {
		ret = 1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Enable error interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_mec5_irq_err_enable(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	mec_uart_intr_mask(base, MEC_UART_IEN_FLAG_ELSI, MEC_UART_IEN_FLAG_ELSI);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static void uart_mec5_irq_err_disable(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	mec_uart_intr_mask(base, MEC_UART_IEN_FLAG_ELSI, 0);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_mec5_irq_is_pending(const struct device *dev)
{
	struct uart_mec5_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	int ret = 0;

	if (dev_data->ipend != MEC_UART_IPEND_NONE) {
		ret = 1;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct
 *
 * @return Always 1
 */
static int uart_mec5_irq_update(const struct device *dev)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->ipend = MEC_UART_IPEND_NONE;
	mec_uart_pending_status(base, &dev_data->ipend);

	k_spin_unlock(&dev_data->lock, key);

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void uart_mec5_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_mec5_dev_data * const dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static void uart_mec5_isr(const struct device *dev)
{
	struct uart_mec5_dev_data * const dev_data = dev->data;
#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_regs *base = dev_cfg->base;

	if (mec_uart_is_rx_data(base)) {
		k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

		uart_mec5_pm_policy_state_lock_get(UART_MEC5_PM_POLICY_STATE_RX_FLAG);
		k_work_reschedule(&rx_refresh_timeout_work, delay);
	}
#endif

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}

#ifdef CONFIG_PM_DEVICE
	if (uart_mec5_irq_tx_complete(dev)) {
		uart_mec5_pm_policy_state_lock_put(UART_MEC5_PM_POLICY_STATE_TX_FLAG);
	}
#endif /* CONFIG_PM */
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_MCHP_MEC5_LINE_CTRL

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev UART device struct
 * @param ctrl The line control to be manipulated
 * @param val Value to set the line control
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_mec5_line_ctrl_set(const struct device *dev,
				   uint32_t ctrl, uint32_t val)
{
	const struct uart_mec5_device_config * const dev_cfg = dev->config;
	struct uart_mec5_dev_data *dev_data = dev->data;
	struct uart_regs *base = dev_cfg->base;
	uint32_t mdc, chg;
	k_spinlock_key_t key;
	int ret = -ENOTSUP;
	uint8_t sel = MEC_UART_DTR_SELECT;

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		/* set_baud_rate(dev, val); */
		if (mec_uart_baud_rate_set(base, dev_cfg->clock_freq, val) == MEC_RET_OK) {
			ret = 0;
		} else {
			ret = -EIO;
		}
		break;

	case UART_LINE_CTRL_RTS:
		sel = MEC_UART_RTS_SELECT;
		__fallthrough; /* fall through */
	case UART_LINE_CTRL_DTR:
		key = k_spin_lock(&dev_data->lock);
		if (mec_uart_dtr_rts_set(base, sel, val) == MEC_RET_OK) {
			ret = 0;
		} else {
			ret = -EIO;
		}
		k_spin_unlock(&dev_data->lock, key);
		break;
	}

	return ret;
}

#endif /* CONFIG_UART_MCHP_MEC5_LINE_CTRL */

static const struct uart_driver_api uart_mec5_driver_api = {
	.poll_in = uart_mec5_poll_in,
	.poll_out = uart_mec5_poll_out,
	.err_check = uart_mec5_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_mec5_configure,
	.config_get = uart_mec5_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mec5_fifo_fill,
	.fifo_read = uart_mec5_fifo_read,
	.irq_tx_enable = uart_mec5_irq_tx_enable,
	.irq_tx_disable = uart_mec5_irq_tx_disable,
	.irq_tx_ready = uart_mec5_irq_tx_ready,
	.irq_tx_complete = uart_mec5_irq_tx_complete,
	.irq_rx_enable = uart_mec5_irq_rx_enable,
	.irq_rx_disable = uart_mec5_irq_rx_disable,
	.irq_rx_ready = uart_mec5_irq_rx_ready,
	.irq_err_enable = uart_mec5_irq_err_enable,
	.irq_err_disable = uart_mec5_irq_err_disable,
	.irq_is_pending = uart_mec5_irq_is_pending,
	.irq_update = uart_mec5_irq_update,
	.irq_callback_set = uart_mec5_irq_callback_set,
#endif
#ifdef CONFIG_UART_MCHP_MEC5_LINE_CTRL
	.line_ctrl_set = uart_mec5_line_ctrl_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define DEV_CONFIG_IRQ_FUNC_INIT(n) \
	.irq_config_func = irq_config_func##n,
#define UART_MEC5_IRQ_FUNC_DECLARE(n) \
	static void irq_config_func##n(const struct device *dev);
#define UART_MEC5_IRQ_FUNC_DEFINE(n)					\
	static void irq_config_func##n(const struct device *dev)	\
	{								\
		ARG_UNUSED(dev);					\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    uart_mec5_isr, DEVICE_DT_INST_GET(n),	\
			    0);						\
		irq_enable(DT_INST_IRQN(n));				\
	}
#else
/* !CONFIG_UART_INTERRUPT_DRIVEN */
#define DEV_CONFIG_IRQ_FUNC_INIT(n)
#define UART_MEC5_IRQ_FUNC_DECLARE(n)
#define UART_MEC5_IRQ_FUNC_DEFINE(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/* To enable wakeup on the UART, the DTS needs to have two entries defined
 * in the corresponding UART node in the DTS specifying it as a wake source
 * and specifying the UART_RX GPIO; example as below
 *
 *	wakerx-gpios = <&gpio_140_176 25 GPIO_ACTIVE_HIGH>;
 *	wakeup-source;
 */
#ifdef CONFIG_PM_DEVICE
#define MEC5_UART_PM_WAKEUP(n)						\
	.wakeup_source = (uint8_t)DT_INST_PROP_OR(n, wakeup_source, 0),	\
	.wakerx_gpio = GPIO_DT_SPEC_INST_GET_OR(n, wakerx_gpios, {0}),
#else
#define MEC5_UART_PM_WAKEUP(index) /* Not used */
#endif

#define DEV_DATA_FLOW_CTRL(n)						\
	DT_INST_PROP_OR(n, hw_flow_control, UART_CFG_FLOW_CTRL_NONE)

#define UART_MEC5_DEVICE_INIT(n)					\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	UART_MEC5_IRQ_FUNC_DECLARE(n);					\
									\
	static const struct uart_mec5_device_config uart_mec5_dev_cfg_##n = { \
		.base = (struct uart_regs *)(DT_INST_REG_ADDR(n)),	\
		.clock_freq = DT_INST_PROP(n, clock_frequency),		\
		.use_ext_clk = DT_INST_PROP_OR(n, use_extclk, 0),	\
		.fifo_dis = DT_INST_PROP_OR(n, fifo_mode_disable, 0),	\
		.rx_fifo_trig = DT_INST_ENUM_IDX_OR(n, rx_fifo_trig, 2),\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		MEC5_UART_PM_WAKEUP(n)					\
		DEV_CONFIG_IRQ_FUNC_INIT(n)				\
	};								\
	static struct uart_mec5_dev_data uart_mec5_dev_data_##n = {	\
		.ucfg.baudrate = DT_INST_PROP_OR(n, current_speed, 0),	\
		.ucfg.parity = UART_CFG_PARITY_NONE,			\
		.ucfg.stop_bits = UART_CFG_STOP_BITS_1,			\
		.ucfg.data_bits = UART_CFG_DATA_BITS_8,			\
		.ucfg.flow_ctrl = DEV_DATA_FLOW_CTRL(n),		\
	};								\
	PM_DEVICE_DT_INST_DEFINE(n, uart_mec5_pm_action);		\
	DEVICE_DT_INST_DEFINE(n, &uart_mec5_init,			\
			      PM_DEVICE_DT_INST_GET(n),			\
			      &uart_mec5_dev_data_##n,			\
			      &uart_mec5_dev_cfg_##n,			\
			      PRE_KERNEL_1,				\
			      CONFIG_SERIAL_INIT_PRIORITY,		\
			      &uart_mec5_driver_api);			\
	UART_MEC5_IRQ_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(UART_MEC5_DEVICE_INIT)
