/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <ipc/ipc_based_driver.h>

/* Driver dts compatibility: telink,w91_uart */
#define DT_DRV_COMPAT telink_w91_uart

enum {
	IPC_DISPATCHER_UART_CONFIG = IPC_DISPATCHER_UART,
};

/* Parity type */
#define UART_PARITY_NONE   ((uint8_t)0u)
#define UART_PARITY_ODD    ((uint8_t)1u)
#define UART_PARITY_EVEN   ((uint8_t)2u)

/* Stop bits */
#define UART_STOP_BIT_1    ((uint8_t)0u)
#define UART_STOP_BIT_2    ((uint8_t)1u)

/* Stop bits */
#define UART_DATA_BITS_5    ((uint8_t)0u)
#define UART_DATA_BITS_6    ((uint8_t)1u)
#define UART_DATA_BITS_7    ((uint8_t)2u)
#define UART_DATA_BITS_8    ((uint8_t)3u)

/* w91 UART data structure */
struct uart_w91_data {
	struct uart_config cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
	struct ipc_based_driver ipc;    /* ipc driver part */
};

/* w91 UART config structure */
struct uart_w91_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t baud_rate;
	void (*pirq_connect)(void);
	bool hw_flow_control;
	uint8_t instance_id;            /* instance id */
};

struct uart_w91_config_req {
	uint32_t baudrate;
	uint8_t parity;
	uint8_t stop_bits;
	uint8_t data_bits;
	uint8_t flow_ctrl;
};

/* API implementation: configure */
static int uart_w91_configure(const struct device *dev,
			      const struct uart_config *cfg)
{
	/* TODO */
	return 0;
}

/* API implementation: config get */
static int uart_w91_config_get(const struct device *dev,
			       struct uart_config *cfg)
{
	/* TODO */
	return 0;
}

/* API implementation: driver initialization */
static int uart_w91_driver_init(const struct device *dev)
{
	int err = 0;
	const struct uart_w91_config *cfg = dev->config;

	/* configure pins */
	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	return 0;
}

/* API implementation: poll out */
static void uart_w91_poll_out(const struct device *dev, uint8_t c)
{
	/* TODO */
}

/* API implementation: poll in */
static int uart_w91_poll_in(const struct device *dev, unsigned char *c)
{
	/* TODO */
	return 0;
}

/* API implementation: err check */
static int uart_w91_err_check(const struct device *dev)
{
	/* TODO */
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* API implementation: fifo fill */
static int uart_w91_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data,
			      int size)
{
	/* TODO */
	return 0;
}

/* API implementation: fifo read */
static int uart_w91_fifo_read(const struct device *dev,
			      uint8_t *rx_data,
			      const int size)
{
	/* TODO */
	return 0;
}

/* API implementation: irq tx enable */
static void uart_w91_irq_tx_enable(const struct device *dev)
{
	/* TODO */
}

/* API implementation: irq tx disable */
static void uart_w91_irq_tx_disable(const struct device *dev)
{
	/* TODO */
}

/* API implementation: irq tx ready */
static int uart_w91_irq_tx_ready(const struct device *dev)
{
	/* TODO */
	return 0;
}

/* API implementation: irq tx complete */
static int uart_w91_irq_tx_complete(const struct device *dev)
{
	/* TODO */
	return 0;
}

/* API implementation: irq rx enable */
static void uart_w91_irq_rx_enable(const struct device *dev)
{
	/* TODO */
}

/* API implementation: irq rx disable */
static void uart_w91_irq_rx_disable(const struct device *dev)
{
	/* TODO */
}

/* API implementation: irq rx ready */
static int uart_w91_irq_rx_ready(const struct device *dev)
{
	/* TODO */
	return 0;
}

/* API implementation: irq err enable */
static void uart_w91_irq_err_enable(const struct device *dev)
{
	/* TODO */
}

/* API implementation: irq err disable*/
static void uart_w91_irq_err_disable(const struct device *dev)
{
	/* TODO */
}

/* API implementation: irq is pending */
static int uart_w91_irq_is_pending(const struct device *dev)
{
	/* TODO */
	return 0;
}

/* API implementation: irq update */
static int uart_w91_irq_update(const struct device *dev)
{
	/* TODO */
	return 1;
}

/* API implementation: irq callback set */
static void uart_w91_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	/* TODO */
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_w91_driver_api = {
	.poll_in = uart_w91_poll_in,
	.poll_out = uart_w91_poll_out,
	.err_check = uart_w91_err_check,
	.configure = uart_w91_configure,
	.config_get = uart_w91_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_w91_fifo_fill,
	.fifo_read = uart_w91_fifo_read,
	.irq_tx_enable = uart_w91_irq_tx_enable,
	.irq_tx_disable = uart_w91_irq_tx_disable,
	.irq_tx_ready = uart_w91_irq_tx_ready,
	.irq_tx_complete = uart_w91_irq_tx_complete,
	.irq_rx_enable = uart_w91_irq_rx_enable,
	.irq_rx_disable = uart_w91_irq_rx_disable,
	.irq_rx_ready = uart_w91_irq_rx_ready,
	.irq_err_enable = uart_w91_irq_err_enable,
	.irq_err_disable = uart_w91_irq_err_disable,
	.irq_is_pending = uart_w91_irq_is_pending,
	.irq_update = uart_w91_irq_update,
	.irq_callback_set = uart_w91_irq_callback_set,
#endif
};


#define UART_W91_INIT(n)                                        \
	                                                            \
	static void uart_w91_irq_connect_##n(void);                 \
	                                                            \
	PINCTRL_DT_INST_DEFINE(n);                                  \
	                                                            \
	static const struct uart_w91_config uart_w91_cfg_##n =      \
	{                                                           \
		.baud_rate = DT_INST_PROP(n, current_speed),            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),              \
		.pirq_connect = uart_w91_irq_connect_##n,               \
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control)     \
	};                                                          \
	                                                            \
	static struct uart_w91_data uart_w91_data_##n;              \
	                                                            \
	DEVICE_DT_INST_DEFINE(n, uart_w91_driver_init,              \
			      PM_DEVICE_DT_INST_GET(n),                     \
			      &uart_w91_data_##n,                           \
			      &uart_w91_cfg_##n,                            \
			      POST_KERNEL,                                  \
			      CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,  \
			      (void *)&uart_w91_driver_api);                \
	                                                            \
	static void uart_w91_irq_connect_##n(void)                  \
	{                                                           \
	}

DT_INST_FOREACH_STATUS_OKAY(UART_W91_INIT)
