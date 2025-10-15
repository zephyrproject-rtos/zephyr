/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <math.h>
#include <zephyr/sys/printk.h>

#ifndef _ASMLANGUAGE
#include <xc.h>
#endif

#define DT_DRV_COMPAT microchip_dspic33_uart

/* UART Register Offsets */
#define UART_MODE    (0x00U) /* Offset for UART Mode register */
#define OFFSET_STA   (0x04U) /* Offset for UART Status register */
#define OFFSET_BRG   (0x08U) /* Offset for Baud Rate Generator register */
#define OFFSET_RXREG (0x0CU) /* Offset for UART Receive register */
#define OFFSET_TXREG (0x10U) /* Offset for UART Transmit register */

/* UART Control Bits */
#define BIT_UTXEN      (0x00000020U) /* UART Transmit Enable bit mask */
#define BIT_URXEN      (0x00000010U) /* UART Receive Enable bit mask */
#define BIT_UARTEN     (0x00008000U) /* UART Module Enable bit mask */
#define BIT_TXBF       (0x00100000U) /* UART Transmit Buffer Full status */
#define BIT_TXBE       (0x00200000U) /* UART TX buffer empty status bit mask */
#define BIT_RXBF       (0x00010000U) /* UART RX buffer full bit */
#define BIT_RXBE       (0x00020000U) /* UART Receive Buffer Empty status */
#define ERR_IRQ_ENABLE (0x00007F00U) /* UART Error interrupts enable mask */
#define ERR_IRQ_CLEAR  (0x00000017U) /* UART Error interrupts flag clear mask */
#define FRACTIONAL_BRG (0x08000000U) /* Fractional Baud Rate Generator enable */

/* Interrupt Level Select Bits */
#define BIT_UxTXWM (0x00000000U) /* UART TX interrupt level select bit */
#define BIT_UxRXWM (0x00000000U) /* UART RX interrupt level select bit */

/* Generic Masks */
#define BIT_MASK_RCVR (0xFFU) /* Mask for receiver buffer */

static struct k_spinlock lock;

struct uart_dspic_config {
	uint32_t base;
	uint32_t baudrate;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
	/* IRQ number */
	unsigned int rx_irq_num;
	unsigned int tx_irq_num;
	unsigned int err_irq_num;
#endif
};

static inline uint32_t CALCULATE_BRG(uint32_t baudrate)
{
	double sys_clk = (double)sys_clock_hw_cycles_per_sec();
	double divisor = (2.0 * (double)baudrate);

	/* Round up (ceil) as per formula */
	uint32_t brg = (uint32_t)ceil(sys_clk / divisor);

	return brg;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
struct uart_dspic_data {
	uart_irq_callback_user_data_t callback;
	void *user_data;
};

static void uart_dspic_isr(const struct device *dev)
{
	struct uart_dspic_data *data = dev->data;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);

	/* RX interrupt flag set check */
	if ((bool)arch_dspic_irq_isset(cfg->rx_irq_num)) {
		if ((bool)data->callback) {
			data->callback(dev, data->user_data);
		}
	}

	/* TX interrupt flag set check */
	if ((bool)arch_dspic_irq_isset(cfg->tx_irq_num)) {
		if ((bool)data->callback) {
			data->callback(dev, data->user_data);
		}
	}

	/* ERR interrupt flag set check */
	if ((bool)arch_dspic_irq_isset(cfg->err_irq_num)) {

		/* Clear error interrupt flag in uart status register */
		*UxSTA &= ~ERR_IRQ_CLEAR;
		if ((bool)data->callback) {
			data->callback(dev, data->user_data);
		}
	}
}

static void uart_dspic_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *user_data)
{
	k_spinlock_key_t key;
	struct uart_dspic_data *data = dev->data;

	/* Registering the callback function and user data */
	key = k_spin_lock(&lock);
	data->callback = cb;
	data->user_data = user_data;
	k_spin_unlock(&lock, key);
}

static void uart_dspic_irq_tx_enable(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;

	key = k_spin_lock(&lock);
	/* Enabling the UART transmit interrupt */
	irq_enable(cfg->tx_irq_num);
	k_spin_unlock(&lock, key);
}

static void uart_dspic_irq_tx_disable(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;

	key = k_spin_lock(&lock);
	/* Disabling the UART transmit interrupt */
	irq_disable(cfg->tx_irq_num);
	k_spin_unlock(&lock, key);
}

static void uart_dspic_irq_rx_enable(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;

	key = k_spin_lock(&lock);
	/* Enabling the UART receiver interrupt */
	irq_enable(cfg->rx_irq_num);
	k_spin_unlock(&lock, key);
}

static void uart_dspic_irq_rx_disable(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;

	key = k_spin_lock(&lock);
	/* Disabling the UART receiver interrupt */
	irq_disable(cfg->rx_irq_num);
	k_spin_unlock(&lock, key);
}

static void uart_dspic_irq_err_enable(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;

	key = k_spin_lock(&lock);
	/* Enabling the UART Error interrupt */
	irq_enable(cfg->err_irq_num);
	k_spin_unlock(&lock, key);
}

static void uart_dspic_irq_err_disable(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;

	key = k_spin_lock(&lock);
	/* Disabling the UART Error interrupt */
	irq_disable(cfg->err_irq_num);
	k_spin_unlock(&lock, key);
}

static int uart_dspic_irq_tx_ready(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	int ret;

	key = k_spin_lock(&lock);
	/* Transmit buffer empty check */
	ret = (*UxSTA & BIT_TXBE) ? 1 : 0;
	k_spin_unlock(&lock, key);

	return ret;
}

static int uart_dspic_irq_rx_ready(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	int ret;

	key = k_spin_lock(&lock);
	/* Receiver buffer not full check */
	ret = (*UxSTA & BIT_RXBF) ? 0 : 1;
	k_spin_unlock(&lock, key);

	return ret;
}

static int uart_dspic_irq_is_pending(const struct device *dev)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	int ret;

	/* Interrupt pending check */
	key = k_spin_lock(&lock);
	ret = arch_dspic_irq_isset(cfg->rx_irq_num) | arch_dspic_irq_isset(cfg->tx_irq_num) |
	      arch_dspic_irq_isset(cfg->err_irq_num);
	k_spin_unlock(&lock, key);

	return ret;
}

static int uart_dspic_irq_update(const struct device *dev)
{
	(void)dev;
	return 1;
}

static int uart_dspic_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	volatile uint32_t *UxRXREG = (void *)(cfg->base + OFFSET_RXREG);
	int num_read = 0;

	/* Transmitting data of size bytes */
	while ((num_read < size) && (!(*UxSTA & BIT_RXBE))) {
		key = k_spin_lock(&lock);
		rx_data[num_read] = *UxRXREG & BIT_MASK_RCVR;
		num_read++;
		k_spin_unlock(&lock, key);
	}

	return num_read;
}

static int uart_dspic_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	volatile uint32_t *UxTXREG = (void *)(cfg->base + OFFSET_TXREG);
	int num_sent = 0;

	/* Receiving data of size bytes */
	while ((num_sent < size) && (!(*UxSTA & BIT_TXBF))) {
		key = k_spin_lock(&lock);
		*UxTXREG = tx_data[num_sent];
		num_sent++;
		k_spin_unlock(&lock, key);
	}

	return num_sent;
}
#endif

static void uart_dspic_poll_out(const struct device *dev, unsigned char c)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	volatile uint32_t *UxTXREG = (void *)(cfg->base + OFFSET_TXREG);

	/* Wait until there is space in the TX FIFO */
	while ((bool)(void *)((*UxSTA) & BIT_TXBF)) {
		;
	}
	key = k_spin_lock(&lock);
	*UxTXREG = c;
	k_spin_unlock(&lock, key);
}

static int uart_dspic_poll_in(const struct device *dev, unsigned char *c)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	volatile uint32_t *UxRXREG = (void *)(cfg->base + OFFSET_RXREG);
	int ret_val;

	key = k_spin_lock(&lock);

	if ((*UxSTA & BIT_RXBE) != 0U) {
	/* uart_poll_in api expects to return -1, if receiver buffer is empty */
		ret_val = -1;
	}

	else {
		*c = *UxRXREG & 0xFF;
		ret_val = 0;
	}

	k_spin_unlock(&lock, key);
	return ret_val;
}

static int uart_dspic_init(const struct device *dev)
{
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxCON = (void *)(cfg->base);
	volatile uint32_t *UxBRG = (void *)(cfg->base + OFFSET_BRG);
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	int ret = 0;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret == 0) {
		/* Setting the UART mode */
		*UxCON |= UART_MODE;

		/* Setting the baudrate */
		*UxCON |= FRACTIONAL_BRG;
		*UxBRG |= CALCULATE_BRG(cfg->baudrate);

		/* Enable UART */
		*UxCON |= BIT_UARTEN;
		*UxCON |= BIT_UTXEN;
		*UxCON |= BIT_URXEN;

		/* Selecting the transmit and receive interrupt level bit */
		*UxSTA |= BIT_UxTXWM;
		*UxSTA |= BIT_UxRXWM;

		/* Enabling the Error interrupts */
		*UxSTA |= ERR_IRQ_ENABLE;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		if (cfg->irq_config_func != NULL) {
			cfg->irq_config_func(dev);
		}
#endif
	}

	return ret;
}

static const struct uart_driver_api uart_dspic_api = {
	.poll_out = uart_dspic_poll_out,
	.poll_in = uart_dspic_poll_in,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_dspic_fifo_fill,
	.fifo_read = uart_dspic_fifo_read,
	.irq_rx_enable = uart_dspic_irq_rx_enable,
	.irq_rx_disable = uart_dspic_irq_rx_disable,
	.irq_tx_enable = uart_dspic_irq_tx_enable,
	.irq_tx_disable = uart_dspic_irq_tx_disable,
	.irq_err_enable = uart_dspic_irq_err_enable,
	.irq_err_disable = uart_dspic_irq_err_disable,
	.irq_callback_set = uart_dspic_irq_callback_set,
	.irq_tx_ready = uart_dspic_irq_tx_ready,
	.irq_rx_ready = uart_dspic_irq_rx_ready,
	.irq_is_pending = uart_dspic_irq_is_pending,
	.irq_update = uart_dspic_irq_update,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_DSPIC_IRQ_HANDLER_DECLARE(inst)                                                       \
	static void uart_dspic_irq_config_##inst(const struct device *dev);

#define UART_DSPIC_IRQ_HANDLER_DEFINE(inst)                                                        \
	static void uart_dspic_irq_config_##inst(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),                                      \
			    DT_INST_IRQ_BY_IDX(inst, 0, priority), uart_dspic_isr,                 \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 1, irq),                                      \
			    DT_INST_IRQ_BY_IDX(inst, 1, priority), uart_dspic_isr,                 \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 2, irq),                                      \
			    DT_INST_IRQ_BY_IDX(inst, 2, priority), uart_dspic_isr,                 \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 0, irq));                                      \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 1, irq));                                      \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 2, irq));                                      \
	}

#define UART_DSPIC_IRQ_CONFIG_FUNC(inst) .irq_config_func = uart_dspic_irq_config_##inst,
#else
#define UART_DSPIC_IRQ_HANDLER_DECLARE(inst)
#define UART_DSPIC_IRQ_HANDLER_DEFINE(inst)
#define UART_DSPIC_IRQ_CONFIG_FUNC(inst)
#endif

#define UART_DSPIC_INIT(inst)                                                                      \
	UART_DSPIC_IRQ_HANDLER_DECLARE(inst)                                                       \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct uart_dspic_config uart_dspic_config_##inst = {                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.baudrate = DT_INST_PROP(inst, current_speed),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		UART_DSPIC_IRQ_CONFIG_FUNC(inst).rx_irq_num = DT_INST_IRQ_BY_IDX(inst, 0, irq),    \
		.tx_irq_num = DT_INST_IRQ_BY_IDX(inst, 1, irq),                                    \
		.err_irq_num = DT_INST_IRQ_BY_IDX(inst, 2, irq),                                   \
	};                                                                                         \
	static struct uart_dspic_data uart_dspic_data_##inst;                                      \
	DEVICE_DT_INST_DEFINE(inst, uart_dspic_init, NULL, &uart_dspic_data_##inst,                \
			      &uart_dspic_config_##inst, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_dspic_api);                       \
	UART_DSPIC_IRQ_HANDLER_DEFINE(inst)

DT_INST_FOREACH_STATUS_OKAY(UART_DSPIC_INIT)
