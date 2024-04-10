/*
 * Copyright (c) 2021, Linaro Limited.
 * Copyright (c) 2022-2023, Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atm_uart

/**
 * @brief Driver for UART on Atmosic
 *
 * UART has two wires for RX and TX, and does not provide CTS or RTS.
 */
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <errno.h>
#ifdef CONFIG_PM
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#endif

#include "compiler.h"
#include "at_pinmux.h"
#include "at_wrpr.h"

#if defined(CONFIG_SOC_SERIES_ATM33) || defined(CONFIG_SOC_SERIES_ATM34)
#include "at_apb_uart_regs_core_macro.h"
#endif

/* UART registers struct */
struct uart_atm {
	/* offset: 0x000 (r/w) data register    */
	volatile uint32_t data;
	/* offset: 0x004 (r/w) status register  */
	volatile uint32_t state;
	/* offset: 0x008 (r/w) control register */
	volatile uint32_t ctrl;
	union {
		/* offset: 0x00c (r/ ) interrupt status register */
		volatile uint32_t intstatus;
		/* offset: 0x00c ( /w) interrupt clear register  */
		volatile uint32_t intclear;
	};
	/* offset: 0x010 (r/w) baudrate divider register */
	volatile uint32_t bauddiv;
	/* offset: 0x014 (r/w) receive low watermark register */
	volatile uint32_t rx_lwm;
	/* offset: 0x018 (r/w) transmit low watermark register */
	volatile uint32_t tx_lwm;
	/* offset: 0x01c (r/) unoccupied spaces in rx fifo register */
	volatile uint32_t rx_fifo_spaces;
	/* offset: 0x020 (r/) unoccupied spaces in tx fifo register */
	volatile uint32_t tx_fifo_spaces;
	/* offset: 0x024 (r/w) flow control register */
	volatile uint32_t hw_flow_ovrd;
};

/* UART Bits */
/* CTRL Register */
#define UART_TX_EN (1 << 0)
#define UART_RX_EN (1 << 1)
#define UART_TX_IN_EN (1 << 2)
#define UART_RX_IN_EN (1 << 3)
#define UART_TX_OV_EN (1 << 4)
#define UART_RX_OV_EN (1 << 5)
#define UART_HS_TM_TX (1 << 6)

/* STATE Register */
#define UART_TX_BF (1 << 0)
#define UART_RX_BF (1 << 1)
#define UART_TX_B_OV (1 << 2)
#define UART_RX_B_OV (1 << 3)
#define UART_nRTS (1 << 4)
#define UART_nCTS (1 << 5)
#define UART_TX_RDY (1 << 7)

/* INTSTATUS Register */
#define UART_TX_IN (1 << 0)
#define UART_RX_IN (1 << 1)
#define UART_TX_OV_IN (1 << 2)
#define UART_RX_OV_IN (1 << 3)

/* FIFO Registers */
#define UART_FIFO_SIZE 16

/* HW_FLOW_OVRD Register */
#define UART_nRTS_VAL (1 << 0)
#define UART_nRTS_OVRD (1 << 1)
#define UART_nCTS_VAL (1 << 2)
#define UART_nCTS_OVRD (1 << 3)

typedef void (*set_callback_t)(void);

struct uart_atm_config {
	volatile struct uart_atm *uart;
	uint32_t sys_clk_freq;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

/* Device data structure */
struct uart_atm_dev_data {
	uint32_t baudrate;
	bool hw_flow_control;
	set_callback_t config_pins;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
#ifdef CONFIG_PM
	bool tx_poll_stream_on;
	bool tx_int_stream_on;
	bool pm_constraint_on;
#endif
};

static const struct uart_driver_api uart_atm_driver_api;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_atm_isr(const struct device *dev);
#endif

#ifdef CONFIG_PM
static void uart_atm_pm_constraint_set(const struct device *dev)
{
	struct uart_atm_dev_data *data = dev->data;

	if (!data->pm_constraint_on) {
		data->pm_constraint_on = true;
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
	}
}

static void uart_atm_pm_constraint_release(const struct device *dev)
{
	struct uart_atm_dev_data *data = dev->data;

	if (data->pm_constraint_on) {
		data->pm_constraint_on = false;
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
	}
}
#endif /* CONFIG_PM */

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UART.
 *
 * @param dev UART device struct
 */
static void baudrate_set(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	struct uart_atm_dev_data *const dev_data = dev->data;
	const uint32_t sys_clk = dev_cfg->sys_clk_freq;
	const uint32_t baudrate = dev_data->baudrate;
	/*
	 * If baudrate and/or sys_clk_freq are 0 the configuration remains
	 * unchanged. It can be useful in case that Zephyr it is run via
	 * a bootloader that brings up the serial and sets the baudrate.
	 */
	if ((baudrate != 0U) && (sys_clk != 0U)) {
		/* calculate baud rate divisor */
		uint32_t bauddiv = (sys_clk + (baudrate / 2)) / baudrate;
		if (bauddiv == dev_cfg->uart->bauddiv) {
			return;
		}
#ifdef CMSDK_AT_UART_STATE__TX_IDLE__MASK
		uint32_t save_hw_flow_ovrd = dev_cfg->uart->hw_flow_ovrd;
		dev_cfg->uart->hw_flow_ovrd |= UART_nCTS_OVRD;
		while (!(dev_cfg->uart->state & UART_TX_RDY));
		dev_cfg->uart->hw_flow_ovrd = save_hw_flow_ovrd;
#endif
		dev_cfg->uart->bauddiv = bauddiv;
	}
}

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_atm_init(const struct device *dev)
{
	struct uart_atm_dev_data *const dev_data = dev->data;
	const struct uart_atm_config * const dev_cfg = dev->config;

	dev_data->config_pins();

	/* Set baud rate */
	baudrate_set(dev);

	/* Enable receiver and transmitter */
	dev_cfg->uart->ctrl = UART_RX_EN | UART_TX_EN;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->irq_config_func(dev);
#endif

	// TODO: what if user sets this but doesn't give RTS or CTS pin?
	if (dev_data->hw_flow_control) {
		dev_cfg->uart->hw_flow_ovrd = 0;
	}

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_atm_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	struct uart_atm_dev_data *const dev_data = dev->data;

	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOTSUP;
	}

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	if (cfg->parity != UART_CFG_PARITY_NONE) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		dev_cfg->uart->hw_flow_ovrd = (UART_nCTS_OVRD | UART_nRTS_OVRD);
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		dev_cfg->uart->hw_flow_ovrd = 0;
		break;
	default:
		return -ENOTSUP;
	}

	dev_data->baudrate = cfg->baudrate;
	baudrate_set(dev);

	return 0;
}

static int uart_atm_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	const struct uart_atm_dev_data *const dev_data = dev->data;
	const uint32_t hw_flow_reg = dev_cfg->uart->hw_flow_ovrd;

	cfg->stop_bits = UART_CFG_STOP_BITS_1;
	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->parity = UART_CFG_PARITY_NONE;

	if ((hw_flow_reg & (UART_nCTS_OVRD | UART_nRTS_OVRD)) == 0) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	} else {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	}

	cfg->baudrate = dev_data->baudrate;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int uart_atm_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_atm_config * const dev_cfg = dev->config;

	/* If the receiver is not ready returns -1 */
	if (!(dev_cfg->uart->state & UART_RX_BF)) {
		return -1;
	}

	/* got a character */
	*c = (unsigned char)dev_cfg->uart->data;

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_atm_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	unsigned int key;

	/* Wait for transmitter to be ready */
	for (;;) {
		if (!(dev_cfg->uart->state & UART_TX_BF)) {
			key = irq_lock();
			if (!(dev_cfg->uart->state & UART_TX_BF)) {
				break;
			}
			irq_unlock(key);
		}
	}

#ifdef CONFIG_PM
	struct uart_atm_dev_data *data = dev->data;

	/* If an interrupt transmission is in progress, the pm constraint
	 * is already managed by the call of uart_atm_irq_tx_[en|dis]able
	 */
	if (!data->tx_poll_stream_on && !data->tx_int_stream_on) {
		data->tx_poll_stream_on = true;

		/* Don't allow system to suspend until stream transmission has completed */
		uart_atm_pm_constraint_set(dev);

		/* Enable TX interrupt so we can release suspend constraint when done */
		dev_cfg->uart->ctrl |= UART_TX_IN_EN;
	}
#endif /* CONFIG_PM */

	/* Send a character */
	dev_cfg->uart->data = (uint32_t)c;
	irq_unlock(key);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param len Number of bytes to send
 *
 * @return the number of characters that have been read
 */
static int uart_atm_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	uint8_t num_tx = 0U;

	unsigned int key = irq_lock();

	while (len - num_tx > 0) {
		if (dev_cfg->uart->state & UART_TX_BF) {
			dev_cfg->uart->intclear = UART_TX_IN;
			break;
		}
		dev_cfg->uart->data = (uint32_t)tx_data[num_tx++];
	}

	irq_unlock(key);

	return num_tx;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rx_data Pointer to data container
 * @param size Container size in bytes
 *
 * @return the number of characters that have been read
 */
static int uart_atm_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	uint8_t num_rx = 0U;

	while (size - num_rx > 0) {
		/*
		 * When set, UART_RX_BF indicates that RX Buffer
		 * is not empty. We keep reading either until the buffer is empty
		 * or if the size is reached.
		 */
		if (!(dev_cfg->uart->state & UART_RX_BF)) {
			/* RX Buffer is empty, nothing more to read */
			dev_cfg->uart->intclear = UART_RX_IN;
			return num_rx;
		}
		rx_data[num_rx++] = (unsigned char)dev_cfg->uart->data;
	}

	/**
	 * Do not forget to clear the RX interrupt when the size is same as
	 * available data in the RX buffer.
	 */
	if (!(dev_cfg->uart->state & UART_RX_BF)) {
		dev_cfg->uart->intclear = UART_RX_IN;
	}

	return num_rx;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev UART device struct
 */
static void uart_atm_irq_tx_enable(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	unsigned int key = irq_lock();

#ifdef CONFIG_PM
	struct uart_atm_dev_data *data = dev->data;
	data->tx_poll_stream_on = false;
	data->tx_int_stream_on = true;
	uart_atm_pm_constraint_set(dev);
#endif

	dev_cfg->uart->ctrl |= UART_TX_IN_EN;
	/* The expectation is that TX is a level interrupt, active for as
	 * long as TX buffer is empty. But in CMSDK UART it's an edge
	 * interrupt, firing on a state change of TX buffer from full to
	 * empty. So, we need to "prime" it here by calling ISR directly,
	 * to get interrupt processing going, as there is no previous
	 * full state to allow a transition from full to empty buffer
	 * that will trigger a TX interrupt.
	 */
	uart_atm_isr(dev);
	irq_unlock(key);
}

/**
 * @brief Disable TX interrupt
 *
 * @param dev UART device struct
 */
static void uart_atm_irq_tx_disable(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	unsigned int key = irq_lock();

	dev_cfg->uart->ctrl &= ~UART_TX_IN_EN;
	/* Clear any pending TX interrupt after disabling it */
	dev_cfg->uart->intclear = UART_TX_IN;
#ifdef CONFIG_PM
	struct uart_atm_dev_data *data = dev->data;
	data->tx_int_stream_on = false;
	uart_atm_pm_constraint_release(dev);
#endif
	irq_unlock(key);
}

/**
 * @brief Verify if Tx interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_atm_irq_tx_ready(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	return !(dev_cfg->uart->state & UART_TX_BF);
}

/**
 * @brief Enable RX interrupt
 *
 * @param dev UART device struct
 */
static void uart_atm_irq_rx_enable(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	dev_cfg->uart->ctrl |= UART_RX_IN_EN;
}

/**
 * @brief Disable RX interrupt
 *
 * @param dev UART device struct
 */
static void uart_atm_irq_rx_disable(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	dev_cfg->uart->ctrl &= ~UART_RX_IN_EN;
	/* Clear any pending RX interrupt after disabling it */
	dev_cfg->uart->intclear = UART_RX_IN;
}

/**
 * @brief Verify if Tx complete interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_atm_irq_tx_complete(const struct device *dev)
{
	return uart_atm_irq_tx_ready(dev);
}

/**
 * @brief Verify if Rx interrupt has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an interrupt is ready, 0 otherwise
 */
static int uart_atm_irq_rx_ready(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	return dev_cfg->uart->state & UART_RX_BF;
}

/**
 * @brief Enable error interrupt
 *
 * @param dev UART device struct
 */
static void uart_atm_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Disable error interrupt
 *
 * @param dev UART device struct
 */
static void uart_atm_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Verify if Tx or Rx interrupt is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if Tx or Rx interrupt is pending, 0 otherwise
 */
static int uart_atm_irq_is_pending(const struct device *dev)
{
	const struct uart_atm_config * const dev_cfg = dev->config;
	/*
	 * Check UART_RX_BF in case of race condition where interrupt cleared
	 * but FIFO not empty. When RX_LWM is 1 (default) the RX_IN interrupt
	 * is only asserted when FIFO was previously empty and a new byte is
	 * received
	 */
	return (dev_cfg->uart->intstatus & (UART_RX_IN | UART_TX_IN)) ||
	       (dev_cfg->uart->state & UART_RX_BF);
}

/**
 * @brief Update the interrupt status
 *
 * @param dev UART device struct
 *
 * @return always 1
 */
static int uart_atm_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for an Interrupt.
 *
 * @param dev UART device structure
 * @param cb Callback function pointer.
 */
static void uart_atm_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_atm_dev_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * Calls the callback function, if exists.
 *
 * @param arg argument to interrupt service routine.
 */
void uart_atm_isr(const struct device *dev)
{
	struct uart_atm_dev_data *data = dev->data;

#ifdef CONFIG_PM
	const struct uart_atm_config * const dev_cfg = dev->config;
	if ((dev_cfg->uart->ctrl & UART_TX_IN_EN) && (dev_cfg->uart->intstatus & UART_TX_IN)) {
		if (data->tx_poll_stream_on) {
			dev_cfg->uart->intclear = UART_TX_IN;
			if (dev_cfg->uart->tx_fifo_spaces == UART_FIFO_SIZE) {
				/* A poll stream transmission just completed.
				 * Allow system to suspend. */
				dev_cfg->uart->ctrl &= ~UART_TX_IN_EN;
				data->tx_poll_stream_on = false;
				uart_atm_pm_constraint_release(dev);
			}
		} else {
			/* Stream transmission was IRQ based.  Constraint will
			 * be released at the same time TX_IN is disabled.
			 */
		}
	}
#endif /* CONFIG_PM */

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_atm_driver_api = {
	.poll_in = uart_atm_poll_in,
	.poll_out = uart_atm_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_atm_configure,
	.config_get = uart_atm_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_atm_fifo_fill,
	.fifo_read = uart_atm_fifo_read,
	.irq_tx_enable = uart_atm_irq_tx_enable,
	.irq_tx_disable = uart_atm_irq_tx_disable,
	.irq_tx_ready = uart_atm_irq_tx_ready,
	.irq_rx_enable = uart_atm_irq_rx_enable,
	.irq_rx_disable = uart_atm_irq_rx_disable,
	.irq_tx_complete = uart_atm_irq_tx_complete,
	.irq_rx_ready = uart_atm_irq_rx_ready,
	.irq_err_enable = uart_atm_irq_err_enable,
	.irq_err_disable = uart_atm_irq_err_disable,
	.irq_is_pending = uart_atm_irq_is_pending,
	.irq_update = uart_atm_irq_update,
	.irq_callback_set = uart_atm_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define ATMOSIC_UART_IRQ_HANDLER_DECL(inst)				\
	static void uart_atm_irq_config_func_##inst(const struct device *dev);
#define ATMOSIC_UART_IRQ_HANDLER(inst)					\
static void uart_atm_irq_config_func_##inst(const struct device *dev)	\
{									\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, tx, irq),			\
		    DT_INST_IRQ_BY_NAME(inst, tx, priority),		\
		    uart_atm_isr,					\
		    DEVICE_DT_INST_GET(inst),				\
		    0);							\
	irq_enable(DT_INST_IRQ_BY_NAME(inst, tx, irq));			\
									\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, rx, irq),			\
		    DT_INST_IRQ_BY_NAME(inst, rx, priority),		\
		    uart_atm_isr,					\
		    DEVICE_DT_INST_GET(inst),				\
		    0);							\
	irq_enable(DT_INST_IRQ_BY_NAME(inst, rx, irq));			\
}
#define ATMOSIC_UART_IRQ_HANDLER_FUNC(inst)				\
	.irq_config_func = uart_atm_irq_config_func_##inst,
#else
#define ATMOSIC_UART_IRQ_HANDLER_DECL(inst) /* Not used */
#define ATMOSIC_UART_IRQ_HANDLER(inst) /* Not used */
#define ATMOSIC_UART_IRQ_HANDLER_FUNC(inst) /* Not used */
#endif

#define UART_SIG(n, sig) CONCAT(CONCAT(UART, DT_INST_PROP(n, instance)), _##sig)
#ifdef CONFIG_SOC_SERIES_ATMx2
#define UART_BASE(inst) CONCAT(CMSDK_UART, DT_INST_PROP(inst, instance))
#define CLK_ENABLE WRPR_CTRL__CLK_ENABLE
#elif defined(CONFIG_SOC_SERIES_ATM33) || defined(CONFIG_SOC_SERIES_ATM34)
#define UART_BASE(inst) CONCAT(CMSDK_AT_UART,				\
	CONCAT(DT_INST_PROP(inst, instance), _NONSECURE))
#define CLK_ENABLE (WRPR_CTRL__CLK_SEL | WRPR_CTRL__CLK_ENABLE)
#endif

#define ATMOSIC_UART_INIT(inst)						\
ATMOSIC_UART_IRQ_HANDLER_DECL(inst)					\
									\
static void uart_atm_config_pins##inst(void)				\
{									\
	WRPR_CTRL_SET(UART_BASE(inst), CLK_ENABLE);			\
	PIN_SELECT(DT_INST_PROP(inst, rx_pin), UART_SIG(inst, RX));	\
	PIN_SELECT(DT_INST_PROP(inst, tx_pin), UART_SIG(inst, TX));	\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, rts_pin), (		\
		PIN_SELECT(DT_INST_PROP(inst, rts_pin), UART_SIG(inst, RTS)); \
	));								\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cts_pin), (		\
		PIN_SELECT(DT_INST_PROP(inst, cts_pin), UART_SIG(inst, CTS)); \
	));								\
}									\
									\
static const struct uart_atm_config uart_atm_dev_cfg_##inst = {	\
	.uart = (volatile struct uart_atm *)DT_INST_REG_ADDR(inst),			\
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency),\
	ATMOSIC_UART_IRQ_HANDLER_FUNC(inst)				\
};									\
									\
static struct uart_atm_dev_data uart_atm_dev_data_##inst = {		\
	.baudrate = DT_INST_PROP(inst, current_speed),			\
	.hw_flow_control = DT_INST_PROP(inst, hw_flow_control),		\
	.config_pins = uart_atm_config_pins##inst,			\
};									\
									\
DEVICE_DT_INST_DEFINE(inst,						\
		      &uart_atm_init,					\
		      NULL,						\
		      &uart_atm_dev_data_##inst,			\
		      &uart_atm_dev_cfg_##inst,				\
		      PRE_KERNEL_1,					\
		      CONFIG_SERIAL_INIT_PRIORITY,			\
		      &uart_atm_driver_api);				\
									\
ATMOSIC_UART_IRQ_HANDLER(inst)

DT_INST_FOREACH_STATUS_OKAY(ATMOSIC_UART_INIT)
