/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Realtek Ameba LOGUART
 */

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(loguart_ameba, CONFIG_UART_LOG_LEVEL);

/*
 * Extract information from devicetree.
 *
 * This driver only supports one instance of this IP block, so the
 * instance number is always 0.
 */
#define DT_DRV_COMPAT realtek_ameba_loguart

/* Device config structure */
struct loguart_ameba_config {
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_config_func_t irq_config_func;
#endif
};

/* Device data structure */
struct loguart_ameba_data {
	struct uart_config config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
	bool tx_int_en;
	bool rx_int_en;
#endif
};

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int loguart_ameba_poll_in(const struct device *dev, unsigned char *c)
{
	ARG_UNUSED(dev);

	if (!LOGUART_Readable()) {
		return -1;
	}

	*c = LOGUART_GetChar(false);
	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void loguart_ameba_poll_out(const struct device *dev, unsigned char c)
{
	ARG_UNUSED(dev);
	LOGUART_PutChar(c);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int loguart_ameba_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	ARG_UNUSED(dev);

	uint8_t num_tx = 0U;
	unsigned int key;

	if (!LOGUART_Writable()) {
		return num_tx;
	}

	/* Lock interrupts to prevent nested interrupts or thread switch */

	key = irq_lock();

	while ((len - num_tx > 0) && LOGUART_Writable()) {
		LOGUART_PutChar((uint8_t)tx_data[num_tx++]);
	}

	irq_unlock(key);

	return num_tx;
}

static int loguart_ameba_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	ARG_UNUSED(dev);

	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && LOGUART_Readable()) {
		rx_data[num_rx++] = LOGUART_GetChar(false);
	}

	/* Clear timeout int flag */
	if (LOGUART_GetStatus(LOGUART_DEV) & LOGUART_BIT_TIMEOUT_INT) {
		LOGUART_INTClear(LOGUART_DEV, LOGUART_BIT_TOICF);
	}

	return num_rx;
}

static void loguart_ameba_irq_tx_enable(const struct device *dev)
{
	u32 sts;
	struct loguart_ameba_data *data = dev->data;

	/* Disable IRQ Interrupts and Save Previous Status. */
	sts = irq_disable_save();

	data->tx_int_en = true;
	/* KM4: TX_PATH1 */
	LOGUART_INTConfig(LOGUART_DEV, LOGUART_TX_EMPTY_PATH_1_INTR, ENABLE);

	/* Enable IRQ Interrupts according to Previous Status. */
	irq_enable_restore(sts);
}

static void loguart_ameba_irq_tx_disable(const struct device *dev)
{
	u32 sts;
	struct loguart_ameba_data *data = dev->data;

	/* Disable IRQ Interrupts and Save Previous Status. */
	sts = irq_disable_save();

	LOGUART_INTConfig(LOGUART_DEV, LOGUART_TX_EMPTY_PATH_1_INTR, DISABLE);
	data->tx_int_en = false;

	/* Enable IRQ Interrupts according to Previous Status. */
	irq_enable_restore(sts);
}

static int loguart_ameba_irq_tx_ready(const struct device *dev)
{
	struct loguart_ameba_data *data = dev->data;

	/* KM4: TX_PATH1 */
	return (LOGUART_GetStatus(LOGUART_DEV) & LOGUART_BIT_TP1F_EMPTY) && data->tx_int_en;
}

static int loguart_ameba_irq_tx_complete(const struct device *dev)
{
	return loguart_ameba_irq_tx_ready(dev);
}

static void loguart_ameba_irq_rx_enable(const struct device *dev)
{
	struct loguart_ameba_data *data = dev->data;

	data->rx_int_en = true;
	LOGUART_INTConfig(LOGUART_DEV, LOGUART_BIT_ERBI | LOGUART_BIT_ETOI, ENABLE);
}

static void loguart_ameba_irq_rx_disable(const struct device *dev)
{
	struct loguart_ameba_data *data = dev->data;

	data->rx_int_en = false;
	LOGUART_INTConfig(LOGUART_DEV, LOGUART_BIT_ERBI | LOGUART_BIT_ETOI, DISABLE);
}

static int loguart_ameba_irq_rx_ready(const struct device *dev)
{
	struct loguart_ameba_data *data = dev->data;

	return (LOGUART_GetStatus(LOGUART_DEV) &
		(LOGUART_BIT_DRDY | LOGUART_BIT_RXFIFO_INT | LOGUART_BIT_TIMEOUT_INT)) &&
	       data->rx_int_en;
}

static void loguart_ameba_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOGUART_INTConfig(LOGUART_DEV, LOGUART_BIT_ELSI, ENABLE);
}

static void loguart_ameba_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOGUART_INTConfig(LOGUART_DEV, LOGUART_BIT_ELSI, DISABLE);
}

static int loguart_ameba_irq_is_pending(const struct device *dev)
{
	struct loguart_ameba_data *data = dev->data;

	return ((LOGUART_GetStatus(LOGUART_DEV) & LOGUART_BIT_TP1F_EMPTY) && data->tx_int_en) ||
	       ((LOGUART_GetStatus(LOGUART_DEV) &
		 (LOGUART_BIT_DRDY | LOGUART_BIT_RXFIFO_INT | LOGUART_BIT_TIMEOUT_INT)) &&
		data->rx_int_en);
}

static int loguart_ameba_irq_update(const struct device *dev)
{
	return 1;
}

static void loguart_ameba_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct loguart_ameba_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0 on success
 */
static int loguart_ameba_init(const struct device *dev)
{
	LOGUART_RxCmd(LOGUART_DEV, DISABLE);
	LOGUART_INTCoreConfig(LOGUART_DEV, LOGUART_BIT_INTR_MASK_KM0, DISABLE);
	LOGUART_INTCoreConfig(LOGUART_DEV, LOGUART_BIT_INTR_MASK_KM4, ENABLE);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	const struct loguart_ameba_config *config = dev->config;

	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	LOGUART_RxCmd(LOGUART_DEV, ENABLE);

	return 0;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define AMEBA_LOGUART_IRQ_HANDLER_DECL                                                             \
	static void loguart_ameba_irq_config_func(const struct device *dev);
#define AMEBA_LOGUART_IRQ_HANDLER                                                                  \
	static void loguart_ameba_irq_config_func(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), loguart_ameba_isr,          \
			    DEVICE_DT_INST_GET(0), 0);                                             \
		irq_enable(DT_INST_IRQN(0));                                                       \
	}
#define AMEBA_LOGUART_IRQ_HANDLER_FUNC .irq_config_func = loguart_ameba_irq_config_func,
#else
#define AMEBA_LOGUART_IRQ_HANDLER_DECL /* Not used */
#define AMEBA_LOGUART_IRQ_HANDLER      /* Not used */
#define AMEBA_LOGUART_IRQ_HANDLER_FUNC /* Not used */
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)

static void loguart_ameba_isr(const struct device *dev)
{
	struct loguart_ameba_data *data = dev->data;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api loguart_ameba_driver_api = {
	.poll_in = loguart_ameba_poll_in,
	.poll_out = loguart_ameba_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = loguart_ameba_fifo_fill,
	.fifo_read = loguart_ameba_fifo_read,
	.irq_tx_enable = loguart_ameba_irq_tx_enable,
	.irq_tx_disable = loguart_ameba_irq_tx_disable,
	.irq_tx_ready = loguart_ameba_irq_tx_ready,
	.irq_rx_enable = loguart_ameba_irq_rx_enable,
	.irq_rx_disable = loguart_ameba_irq_rx_disable,
	.irq_tx_complete = loguart_ameba_irq_tx_complete,
	.irq_rx_ready = loguart_ameba_irq_rx_ready,
	.irq_err_enable = loguart_ameba_irq_err_enable,
	.irq_err_disable = loguart_ameba_irq_err_disable,
	.irq_is_pending = loguart_ameba_irq_is_pending,
	.irq_update = loguart_ameba_irq_update,
	.irq_callback_set = loguart_ameba_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

AMEBA_LOGUART_IRQ_HANDLER_DECL
AMEBA_LOGUART_IRQ_HANDLER

static const struct loguart_ameba_config loguart_config = {AMEBA_LOGUART_IRQ_HANDLER_FUNC};

static struct loguart_ameba_data loguart_data = {.config = {
							 .stop_bits = UART_CFG_STOP_BITS_1,
							 .data_bits = UART_CFG_DATA_BITS_8,
							 .baudrate = DT_INST_PROP(0, current_speed),
							 .parity = UART_CFG_PARITY_NONE,
							 .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
						 }};

DEVICE_DT_INST_DEFINE(0, loguart_ameba_init, NULL, &loguart_data, &loguart_config, PRE_KERNEL_1,
		      CONFIG_SERIAL_INIT_PRIORITY, &loguart_ameba_driver_api);
