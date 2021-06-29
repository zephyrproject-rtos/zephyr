/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <device.h>
#include <drivers/uart.h>
#include "sedi_driver_uart.h"
#include "sedi_driver_ipc.h"
#include "sedi_driver_pm.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart, LOG_LEVEL_ERR);

#define LINE_CONTROL SEDI_UART_LC_8N1
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sedi_isr(void *arg);
static void uart_sedi_cb(const struct device *port);
#endif

#define DT_DRV_COMPAT intel_pse_uart

/* Helper macro to set flow control. */
#define UART_CONFIG_FLOW_CTRL_SET(n) \
	.hw_fc = DT_INST_PROP(n, hw_flow_control)


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/*  UART IRQ handler declaration.  */
#define  UART_IRQ_HANDLER_DECL(n) \
	static void irq_config_uart_##n(struct device *dev)

/* Setting configuration function. */
#define UART_CONFIG_IRQ_HANDLER_SET(n) \
	.uart_irq_config_func = irq_config_uart_##n
#define UART_IRQ_HANDLER_DEFINE(n)				     \
	static void irq_config_uart_##n(struct device *dev)	     \
	{							     \
		ARG_UNUSED(dev);				     \
		IRQ_CONNECT(DT_INST_IRQN(n),			     \
			    DT_INST_IRQ(n, priority), uart_sedi_isr, \
			    DEVICE_GET(uart_##n), 0);		     \
		irq_enable(DT_INST_IRQN(n));			     \
	}
#else /*CONFIG_UART_INTERRUPT_DRIVEN */
#define UART_IRQ_HANDLER_DECL(n)
#define UART_CONFIG_IRQ_HANDLER_SET(n) (0)

#define UART_IRQ_HANDLER_DEFINE(n)
#endif  /* !CONFIG_UART_INTERRUPT_DRIVEN */

/* Device init macro for UART instance. As multiple uart instances follow a
 * similar definition of data structures differing only in the instance
 * number.This macro makes adding instances simpler.
 */
#define UART_SEDI_DEVICE_INIT(n)				      \
	UART_IRQ_HANDLER_DECL(n);				      \
	static K_MUTEX_DEFINE(uart_##n##_mutex);		      \
	static K_SEM_DEFINE(uart_##n##_tx_sem, 1, 1);		      \
	static K_SEM_DEFINE(uart_##n##_rx_sem, 1, 1);		      \
	static K_SEM_DEFINE(uart_##n##_sync_read_sem, 0, 1);	      \
	static const struct uart_sedi_config_info config_info_##n = { \
		.instance = SEDI_UART_##n,			      \
		.baud_rate = DT_INST_PROP(n, current_speed),	      \
		UART_CONFIG_FLOW_CTRL_SET(n),			      \
		.line_ctrl = LINE_CONTROL,			      \
		.mutex = &uart_##n##_mutex,			      \
		.tx_sem = &uart_##n##_tx_sem,			      \
		.rx_sem = &uart_##n##_rx_sem,			      \
		.sync_read_sem = &uart_##n##_sync_read_sem,	      \
		UART_CONFIG_IRQ_HANDLER_SET(n)			      \
	};							      \
								      \
	static struct uart_sedi_drv_data drv_data_##n;		      \
	DEVICE_DEFINE(uart_##n, DT_INST_LABEL(n),		      \
		      &uart_sedi_init,				      \
		      NULL,					      \
		      &drv_data_##n, &config_info_##n,		      \
		      PRE_KERNEL_1,				      \
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);      \
	UART_IRQ_HANDLER_DEFINE(n)



/* Convenient macro to get the controller instance. */
#define GET_CONTROLLER_INSTANCE(dev)		 \
	(((const struct uart_sedi_config_info *) \
	  dev->config)->instance)

/* Convenient macro to get tx semamphore */
#define GET_TX_SEM(dev)				 \
	(((const struct uart_sedi_config_info *) \
	  dev->config)->tx_sem)

/* Convenient macro to get rx sempahore */
#define GET_RX_SEM(dev)				 \
	(((const struct uart_sedi_config_info *) \
	  dev->config)->rx_sem)

/* Convenient macro to get sync_read sempahore */
#define GET_SYNC_READ_SEM(dev)			 \
	(((const struct uart_sedi_config_info *) \
	  dev->config)->sync_read_sem)

#define GET_MUTEX(dev)				 \
	(((const struct uart_sedi_config_info *) \
	  dev->config)->mutex)

struct uart_sedi_config_info {
	/* Specifies the uart instance for configuration. */
	sedi_uart_t instance;

	/* Specifies the baudrate for the uart instance. */
	uint32_t baud_rate;

	/* Specifies the port line contorl settings */
	sedi_uart_lc_t line_ctrl;

	struct k_mutex *mutex;
	struct k_sem *tx_sem;
	struct k_sem *rx_sem;
	struct k_sem *sync_read_sem;
	/* Enable / disable hardware flow control for UART. */

	bool hw_fc;

	/* UART irq configuration function when supporting interrupt
	 * mode.
	 */
	uart_irq_config_func_t uart_irq_config_func;

};

static int uart_sedi_init(const struct device *dev);

struct uart_sedi_drv_data {
	uart_irq_callback_user_data_t user_cb;
	void *unsol_rx_usr_cb_param;
	uint32_t sync_rx_len;
	uint32_t sync_rx_status;
	void *user_data;
	void *usr_rx_buff;
	uint32_t usr_rx_size;

	/* PM callback for freq. change notifocation */
	sedi_pm_callback_config_t pm_rst;
	uint8_t iir_cache;
	int32_t busy_count;
};

#ifdef CONFIG_UART_ASYNC_API

int uart_sedi_callback_set(const struct device *dev, uart_callback_t callback, void *user_data)
{
	return -ENOTSUP;
}

int uart_sedi_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	return -ENOTSUP;
}
int uart_sedi_tx_abort(const struct device *dev)
{
	return -ENOTSUP;
}

int uart_sedi_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	return -ENOTSUP;
}

int uart_sedi_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	return -ENOTSUP;
}

int uart_sedi_rx_disable(const struct device *dev)
{
	return -ENOTSUP;
}

#endif

static int uart_sedi_poll_in(const struct device *dev, unsigned char *data)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);
	uint32_t status;
	int ret = 0;

	sedi_uart_get_status(instance, (uint32_t *) &status);

	/* In order to check if there is any data to read from UART
	 * controller we should check if the SEDI_UART_RX_BUSY bit from
	 * 'status' is not set. This bit is set only if there is any
	 * pending character to read.
	 */
	if (!(status & SEDI_UART_RX_BUSY)) {
		ret = -1;
	} else {
		if (sedi_uart_read(instance, data, (uint32_t *)&status)) {
			ret = -1;
		}
	}
	return ret;
}

static void uart_sedi_poll_out(const struct device *dev,
			       unsigned char data)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_write(instance, data);
}


static int uart_sedi_err_check(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);
	uint32_t status;
	int ret_status = 0;

	sedi_uart_get_status(instance, (uint32_t *const)&status);
	if (status &  SEDI_UART_RX_OE) {
		ret_status = UART_ERROR_OVERRUN;
	}

	if (status & SEDI_UART_RX_PE) {
		ret_status = UART_ERROR_PARITY;
	}

	if (status & SEDI_UART_RX_FE) {
		ret_status = UART_ERROR_FRAMING;
	}

	return ret_status;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_sedi_fifo_fill(const struct device *dev, const uint8_t *tx_data,
			       int size)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	return sedi_uart_fifo_fill(instance, tx_data, size);
}

static int uart_sedi_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	return sedi_uart_fifo_read(instance, rx_data, size);
}

static void uart_sedi_irq_tx_enable(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_irq_tx_enable(instance);
}

static void uart_sedi_irq_tx_disable(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_irq_tx_disable(instance);
}

static int uart_sedi_irq_tx_ready(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	return sedi_uart_irq_tx_ready(instance);
}

static int uart_sedi_irq_tx_complete(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	return sedi_uart_is_tx_complete(instance);
}

static void uart_sedi_irq_rx_enable(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_irq_rx_enable(instance);
}

static void uart_sedi_irq_rx_disable(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_irq_rx_disable(instance);
}

static int uart_sedi_irq_rx_ready(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	return sedi_uart_is_irq_rx_ready(instance);
}

static void uart_sedi_irq_err_enable(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_irq_err_enable(instance);
}

static void uart_sedi_irq_err_disable(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_irq_err_disable(instance);
}

static int uart_sedi_irq_is_pending(const struct device *dev)
{

	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	return sedi_uart_is_irq_pending(instance);
}

static int uart_sedi_irq_update(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_update_irq_cache(instance);
	return 1;
}

static void uart_sedi_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *user_data)
{
	struct uart_sedi_drv_data *drv_data = dev->data;

	drv_data->user_cb = cb;
	drv_data->user_data = user_data;

}

static void uart_sedi_isr(void *arg)
{
	const struct device *dev = arg;
	struct uart_sedi_drv_data *drv_data = dev->data;

	if (drv_data->user_cb) {
		drv_data->user_cb(dev, drv_data->user_data);
	} else {
		uart_sedi_cb(dev);
	}
}


/* Called from generic callback of zephyr , set by set_cb. */
static void uart_sedi_cb(const struct device *port)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(port);

	sedi_uart_isr_handler(instance);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
static int uart_sedi_line_ctrl_set(const struct device *dev, uint32_t ctrl, uint32_t val)
{
	return -ENOTSUP;
}

static int uart_sedi_line_ctrl_get(const struct device *dev, uint32_t ctrl, uint32_t *val)
{
	return -ENOTSUP;
}

#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD
static int uart_sedi_drv_cmd(const struct device *dev, uint32_t cmd, uint32_t p)
{
	return -ENOTSUP;
}
#endif /* CONFIG_UART_DRV_CMD */

static const struct uart_driver_api api = {
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_sedi_callback_set,
	.tx = uart_sedi_tx,
	.tx_abort = uart_sedi_tx_abort,
	.rx_enable = uart_sedi_rx_enable,
	.rx_buf_rsp = uart_sedi_rx_buf_rsp,
	.rx_disable = uart_sedi_rx_disable,
#endif
	.poll_in = uart_sedi_poll_in,
	.poll_out = uart_sedi_poll_out,
	.err_check = uart_sedi_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sedi_fifo_fill,
	.fifo_read = uart_sedi_fifo_read,
	.irq_tx_enable = uart_sedi_irq_tx_enable,
	.irq_tx_disable = uart_sedi_irq_tx_disable,
	.irq_tx_ready = uart_sedi_irq_tx_ready,
	.irq_tx_complete = uart_sedi_irq_tx_complete,
	.irq_rx_enable = uart_sedi_irq_rx_enable,
	.irq_rx_disable = uart_sedi_irq_rx_disable,
	.irq_rx_ready = uart_sedi_irq_rx_ready,
	.irq_err_enable = uart_sedi_irq_err_enable,
	.irq_err_disable = uart_sedi_irq_err_disable,
	.irq_is_pending = uart_sedi_irq_is_pending,
	.irq_update = uart_sedi_irq_update,
	.irq_callback_set = uart_sedi_irq_callback_set,
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = uart_sedi_line_ctrl_set,
	.line_ctrl_get = uart_sedi_line_ctrl_get,
#endif  /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = uart_sedi_drv_cmd,
#endif  /* CONFIG_UART_DRV_CMD */
};

void uart_sedi_clk_change_cb(uint32_t core_ferq, uint32_t hbw_freq,
			     void *ctx)
{
	const struct uart_sedi_config_info *config =
		(const struct uart_sedi_config_info *) (ctx);

	sedi_uart_set_baud_rate(config->instance, config->baud_rate, hbw_freq);
}

static int uart_sedi_init(const struct device *dev)
{

	const struct uart_sedi_config_info *config = dev->config;
	sedi_uart_config_t cfg = { 0 };
	struct uart_sedi_drv_data *drv_data = dev->data;

	if (DEV_PSE_OWNED !=
	    sedi_get_dev_ownership(PSE_DEV_UART0 + config->instance)) {
		return -ENODEV;
	}

	cfg.line_control = config->line_ctrl;
	cfg.baud_rate = config->baud_rate;
	cfg.hw_fc = config->hw_fc;
	cfg.clk_speed_hz = sedi_pm_get_hbw_clock();

	/* Setting to full power and enabling clk. */
	sedi_uart_set_power(config->instance, SEDI_POWER_FULL);

	sedi_uart_set_config(config->instance, &cfg);

	drv_data->pm_rst.func.clk_change_cb = uart_sedi_clk_change_cb;
	drv_data->pm_rst.ctx = (void *) config;
	drv_data->pm_rst.type = CALLBACK_TYPE_CLOCK_CHANGE;
	drv_data->pm_rst.pri = CALLBACK_PRI_NORMAL;
	sedi_pm_register_callback(&drv_data->pm_rst);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	config->uart_irq_config_func(dev);
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

DT_INST_FOREACH_STATUS_OKAY(UART_SEDI_DEVICE_INIT)
