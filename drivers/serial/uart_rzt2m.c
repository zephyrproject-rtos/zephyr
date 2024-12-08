/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_rzt2m.h"
#include "zephyr/spinlock.h"
#include "zephyr/sys/printk.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#define DT_DRV_COMPAT renesas_rzt2m_uart

LOG_MODULE_REGISTER(uart_renesas_rzt2m, CONFIG_UART_LOG_LEVEL);

struct rzt2m_device_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pin_config;
	uart_irq_config_func_t irq_config_func;
};

struct rzt2m_device_data {
	struct uart_config uart_cfg;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *callback_data;
#endif
};

static int rzt2m_poll_in(const struct device *dev, unsigned char *c)
{
	if (!dev || !dev->config || !dev->data) {
		return -ENODEV;
	}

	const struct rzt2m_device_config *config = dev->config;
	struct rzt2m_device_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (FRSR_R(*FRSR(config->base)) == 0) {
		k_spin_unlock(&data->lock, key);
		return -1;
	}
	*c = *RDR(config->base) & RDR_MASK_RDAT;
	*CFCLR(config->base) |= CFCLR_MASK_RDRFC;

	if (FRSR_R(*FRSR(config->base)) == 0) {
		*FFCLR(config->base) |= FFCLR_MASK_DRC;
	}

	k_spin_unlock(&data->lock, key);
	return 0;
}

static void rzt2m_poll_out(const struct device *dev, unsigned char c)
{
	if (!dev || !dev->config || !dev->data) {
		return;
	}

	const struct rzt2m_device_config *config = dev->config;
	struct rzt2m_device_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int fifo_count = FTSR_T(*FTSR(config->base));

	while (fifo_count == MAX_FIFO_DEPTH) {
		fifo_count = FTSR_T(*FTSR(config->base));
	}

	*TDR(config->base) = c;

	/* Clear `Transmit data empty flag`. */
	*CFCLR(config->base) |= CFCLR_MASK_TDREC;

	k_spin_unlock(&data->lock, key);
}

static int rzt2m_err_check(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;

	uint32_t status = *CSR(config->base);
	uint32_t retval = 0;

	if (status & CSR_MASK_ORER) {
		retval |= UART_ERROR_OVERRUN;
	}
	if (status & CSR_MASK_FER) {
		retval |= UART_ERROR_FRAMING;
	}
	if (status & CSR_MASK_PER) {
		retval |= UART_ERROR_PARITY;
	}

	return retval;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_rzt2m_irq_tx_ready(const struct device *dev);

static int rzt2m_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct rzt2m_device_data *data = dev->data;
	const struct rzt2m_device_config *config = dev->config;
	int num_tx = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while ((size - num_tx > 0) && uart_rzt2m_irq_tx_ready(dev)) {
		*TDR(config->base) = (uint8_t)tx_data[num_tx++];
	}

	k_spin_unlock(&data->lock, key);
	return num_tx;
}

static int rzt2m_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct rzt2m_device_data *data = dev->data;
	const struct rzt2m_device_config *config = dev->config;
	int num_rx = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while (num_rx < size && (FRSR_R(*FRSR(config->base)))) {
		rx_data[num_rx++] = *RDR(config->base);
	}
	*CFCLR(config->base) = CFCLR_MASK_RDRFC;
	*FFCLR(config->base) = FFCLR_MASK_DRC;
	k_spin_unlock(&data->lock, key);
	return num_rx;
}

static void uart_rzt2m_irq_rx_enable(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;
	*CCR0(config->base) |= CCR0_MASK_RIE | CCR0_MASK_RE;
}

static void uart_rzt2m_irq_rx_disable(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;
	*CCR0(config->base) &= ~CCR0_MASK_RIE;
}

static void uart_rzt2m_irq_tx_enable(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;
	/* These bits must be set simultaneously. */
	*CCR0(config->base) |= CCR0_MASK_TE | CCR0_MASK_TIE | CCR0_MASK_TEIE;
}

static void uart_rzt2m_irq_tx_disable(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;
	*CCR0(config->base) &= ~(CCR0_MASK_TIE | CCR0_MASK_TEIE);
}

static int uart_rzt2m_irq_tx_ready(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;

	if (FTSR_T(*FTSR(config->base)) == MAX_FIFO_DEPTH ||
	    ((*CCR0(config->base) & CCR0_MASK_TIE) == 0)) {
		return 0;
	}

	return 1;
}

static int uart_rzt2m_irq_rx_ready(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;

	if (FRSR_R(*FRSR(config->base))) {
		return 1;
	}

	return 0;
}

static int uart_rzt2m_irq_is_pending(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;

	if ((*CSR(config->base) & (CSR_MASK_RDRF)) || (*FRSR(config->base) & FRSR_MASK_DR)) {
		return 1;
	}
	return 0;
}

static void uart_rzt2m_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct rzt2m_device_data *data = dev->data;

	data->callback = cb;
	data->callback_data = cb_data;
}

static int uart_rzt2m_irq_update(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;

	*CFCLR(config->base) = CFCLR_MASK_RDRFC;
	*FFCLR(config->base) = FFCLR_MASK_DRC;
	return 1;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, rzt2m_uart_api) = {
	.poll_in = rzt2m_poll_in,
	.poll_out = rzt2m_poll_out,
	.err_check = rzt2m_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = rzt2m_fifo_fill,
	.fifo_read = rzt2m_fifo_read,
	.irq_rx_enable = uart_rzt2m_irq_rx_enable,
	.irq_rx_disable = uart_rzt2m_irq_rx_disable,
	.irq_tx_enable = uart_rzt2m_irq_tx_enable,
	.irq_tx_disable = uart_rzt2m_irq_tx_disable,
	.irq_tx_ready = uart_rzt2m_irq_tx_ready,
	.irq_rx_ready = uart_rzt2m_irq_rx_ready,
	.irq_is_pending = uart_rzt2m_irq_is_pending,
	.irq_callback_set = uart_rzt2m_irq_callback_set,
	.irq_update = uart_rzt2m_irq_update,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int rzt2m_module_start(const struct device *dev)
{
	if (!dev || !dev->config || !dev->data) {
		return -ENODEV;
	}

	const struct rzt2m_device_config *config = dev->config;
	struct rzt2m_device_data *data = dev->data;
	int interface_id = BASE_TO_IFACE_ID(config->base);
	unsigned int irqkey = irq_lock();
	volatile uint32_t dummy;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (interface_id < 5) {
		/* Dummy-read at least one time as stated in 8.3.1 of the User's Manual: Hardware */
		*MSTPCRA &= ~(MSTPCRA_MASK_SCIx(interface_id));
		dummy = *MSTPCRA;
	} else {
		LOG_ERR("SCI modules in the secure domain on RZT2M are not supported.");
		return -ENOTSUP;
	}

	/* Dummy-read at least five times as stated in 8.3.1 of the User's Manual: Hardware */
	dummy = *RDR(config->base);
	dummy = *RDR(config->base);
	dummy = *RDR(config->base);
	dummy = *RDR(config->base);
	dummy = *RDR(config->base);

	k_spin_unlock(&data->lock, key);
	irq_unlock(irqkey);
	return 0;
}

static int rzt2m_uart_init(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;
	struct rzt2m_device_data *data = dev->data;
	uint32_t baud_setting = 0;
	uint32_t baud_settings[] = {CCR2_BAUD_SETTING_9600, CCR2_BAUD_SETTING_115200};

	rzt2m_unlock_prcrs(PRCRS_GPIO);
	rzt2m_unlock_prcrn(PRCRN_PRC1 | PRCRN_PRC2);

	/* The module needs to be started
	 * to allow any operation on the registers of Serial Communications Interface.
	 */
	int ret = rzt2m_module_start(dev);

	if (ret) {
		return ret;
	}

	/* Disable transmitter, receiver, interrupts. */
	*CCR0(config->base) = CCR0_DEFAULT_VALUE;
	while (*CCR0(config->base) & (CCR0_MASK_RE | CCR0_MASK_TE)) {
	}

	*CCR1(config->base) = CCR1_DEFAULT_VALUE;
	*CCR2(config->base) = CCR2_DEFAULT_VALUE;
	*CCR3(config->base) = CCR3_DEFAULT_VALUE;
	*CCR4(config->base) = CCR4_DEFAULT_VALUE;

	/* Configure pinmuxes */
	ret = pinctrl_apply_state(config->pin_config, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	*CFCLR(config->base) = CFCLR_ALL_FLAG_CLEAR;
	*FFCLR(config->base) = FFCLR_MASK_DRC;

	/* Use FIFO mode. */
	*CCR3(config->base) |= (CCR3_MASK_FM);

	switch (data->uart_cfg.stop_bits) {
	case UART_CFG_STOP_BITS_1:
		/* Default value, already set. */
		break;
	case UART_CFG_STOP_BITS_2:
		*CCR3(config->base) |= CCR3_MASK_STP;
		break;
	default:
		LOG_ERR("Selected bit stop length is not supported: %u.", data->uart_cfg.stop_bits);
		return -ENOTSUP;
	}

	switch (data->uart_cfg.data_bits) {
	case UART_CFG_DATA_BITS_7:
		*CCR3(config->base) |= CCR3_CHR_7BIT;
		break;
	case UART_CFG_DATA_BITS_8:
		*CCR3(config->base) |= CCR3_CHR_8BIT;
		break;
	default:
		LOG_ERR("Selected number of data bits is not supported: %u.",
			data->uart_cfg.data_bits);
		return -ENOTSUP;
	}

	if (data->uart_cfg.baudrate > ARRAY_SIZE(baud_settings)) {
		LOG_ERR("Selected baudrate variant is not supported: %u.", data->uart_cfg.baudrate);
		return -ENOTSUP;
	}
	baud_setting = baud_settings[data->uart_cfg.baudrate];

	*CCR2(config->base) &= ~(CCR2_MASK_BAUD_SETTING);
	*CCR2(config->base) |= (baud_setting & CCR2_MASK_BAUD_SETTING);

	*CCR1(config->base) |= (CCR1_MASK_NFEN | CCR1_MASK_SPB2DT | CCR1_MASK_SPB2IO);

	switch (data->uart_cfg.parity) {
	case UART_CFG_PARITY_NONE:
		/* Default value, already set. */
		break;
	case UART_CFG_PARITY_EVEN:
		*CCR1(config->base) |= CCR1_MASK_PE;
		break;
	case UART_CFG_PARITY_ODD:
		*CCR1(config->base) |= (CCR1_MASK_PE | CCR1_MASK_PM);
		break;
	default:
		LOG_ERR("Unsupported parity: %u", data->uart_cfg.parity);
	}

	/* Specify trigger thresholds and clear FIFOs. */
	*FCR(config->base) = FCR_MASK_TFRST | FCR_MASK_RFRST | FCR_TTRG_15 | FCR_RTRG_15;

	/* Enable the clock. */
	*CCR3(config->base) &= ~CCR3_MASK_CKE;
	*CCR3(config->base) |= CCR3_CKE_ENABLE;

	/* Clear status flags. */
	*CFCLR(config->base) = CFCLR_ALL_FLAG_CLEAR;
	*FFCLR(config->base) = FFCLR_MASK_DRC;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	/* Start transmitter and receiver. */
	*CCR0(config->base) |= (CCR0_MASK_TE | CCR0_MASK_RE);
	while (!(*CCR0(config->base) & CCR0_MASK_RE)) {
	}
	while (!(*CCR0(config->base) & CCR0_MASK_TE)) {
	}

	rzt2m_lock_prcrs(PRCRS_GPIO);
	rzt2m_lock_prcrn(PRCRN_PRC1 | PRCRN_PRC2);

	return 0;
}

static void uart_rzt2m_isr(const struct device *dev)
{
	const struct rzt2m_device_config *config = dev->config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct rzt2m_device_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	*CFCLR(config->base) = CFCLR_MASK_RDRFC;
	*FFCLR(config->base) = FFCLR_MASK_DRC;
}

#define UART_RZT2M_IRQ_CONNECT(n, irq_name)                                                        \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, irq_name, irq),                                 \
			    DT_INST_IRQ_BY_NAME(n, irq_name, priority), uart_rzt2m_isr,            \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ_BY_NAME(n, irq_name, flags));       \
		irq_enable(DT_INST_IRQ_BY_NAME(n, irq_name, irq));                                 \
	} while (false)

#define UART_RZT2M_CONFIG_FUNC(n)                                                                  \
	static void uart##n##_rzt2m_irq_config(const struct device *port)                          \
	{                                                                                          \
		UART_RZT2M_IRQ_CONNECT(n, rx_err);                                                 \
		UART_RZT2M_IRQ_CONNECT(n, rx);                                                     \
		UART_RZT2M_IRQ_CONNECT(n, tx);                                                     \
		UART_RZT2M_IRQ_CONNECT(n, tx_end);                                                 \
	}

#define UART_RZT2M_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct rzt2m_device_data rzt2m_uart_##n##data = {                                   \
		.uart_cfg =                                                                        \
			{                                                                          \
				.baudrate = DT_INST_ENUM_IDX(n, current_speed),                    \
				.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),    \
				.stop_bits =                                                       \
					DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),   \
				.data_bits =                                                       \
					DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),   \
			},                                                                         \
	};                                                                                         \
	UART_RZT2M_CONFIG_FUNC(n);                                                                 \
	static const struct rzt2m_device_config rzt2m_uart_##n##_config = {                        \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = uart##n##_rzt2m_irq_config,                                     \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(n)};                                  \
	DEVICE_DT_INST_DEFINE(n, rzt2m_uart_init, NULL, &rzt2m_uart_##n##data,                     \
			      &rzt2m_uart_##n##_config, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &rzt2m_uart_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RZT2M_INIT)
