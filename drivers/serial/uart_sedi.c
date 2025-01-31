/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include "sedi_driver_uart.h"

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sedi_isr(void *arg);
static void uart_sedi_cb(struct device *port);
#endif

#define DT_DRV_COMPAT intel_sedi_uart

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/*  UART IRQ handler declaration.  */
#define  UART_IRQ_HANDLER_DECL(n) \
	static void irq_config_uart_##n(const struct device *dev)

/* Setting configuration function. */
#define UART_CONFIG_IRQ_HANDLER_SET(n) \
	.uart_irq_config_func = irq_config_uart_##n
#define UART_IRQ_HANDLER_DEFINE(n)				       \
	static void irq_config_uart_##n(const struct device *dev)      \
	{							       \
		ARG_UNUSED(dev);				       \
		IRQ_CONNECT(DT_INST_IRQN(n),			       \
			    DT_INST_IRQ(n, priority), uart_sedi_isr,   \
			    DEVICE_DT_GET(DT_NODELABEL(uart##n)),      \
			    DT_INST_IRQ(n, sense));		       \
		irq_enable(DT_INST_IRQN(n));			       \
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
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),		      \
		.instance = DT_INST_PROP(n, peripheral_id),	      \
		.baud_rate = DT_INST_PROP(n, current_speed),	      \
		.hw_fc = DT_INST_PROP(n, hw_flow_control),	      \
		.line_ctrl = SEDI_UART_LC_8N1,			      \
		.mutex = &uart_##n##_mutex,			      \
		UART_CONFIG_IRQ_HANDLER_SET(n)			      \
	};							      \
								      \
	static struct uart_sedi_drv_data drv_data_##n;		      \
	PM_DEVICE_DT_INST_DEFINE(n,                                   \
			    uart_sedi_pm_action);		      \
	DEVICE_DT_INST_DEFINE(n,			              \
		      &uart_sedi_init,				      \
		      PM_DEVICE_DT_INST_GET(n),			      \
		      &drv_data_##n, &config_info_##n,		      \
		      PRE_KERNEL_1,				      \
		      CONFIG_SERIAL_INIT_PRIORITY, &api);	      \
	UART_IRQ_HANDLER_DEFINE(n)



/* Convenient macro to get the controller instance. */
#define GET_CONTROLLER_INSTANCE(dev)		 \
	(((const struct uart_sedi_config_info *) \
	  dev->config)->instance)

#define GET_MUTEX(dev)				 \
	(((const struct uart_sedi_config_info *) \
	  dev->config)->mutex)

struct uart_sedi_config_info {
	DEVICE_MMIO_ROM;
	/* Specifies the uart instance for configuration. */
	sedi_uart_t instance;

	/* Specifies the baudrate for the uart instance. */
	uint32_t baud_rate;

	/* Specifies the port line control settings */
	sedi_uart_lc_t line_ctrl;

	struct k_mutex *mutex;

	/* Enable / disable hardware flow control for UART. */
	bool hw_fc;

	/* UART irq configuration function when supporting interrupt
	 * mode.
	 */
	uart_irq_config_func_t uart_irq_config_func;
};


static int uart_sedi_init(const struct device *dev);

struct uart_sedi_drv_data {
	DEVICE_MMIO_RAM;
	uart_irq_callback_user_data_t user_cb;
	void *unsol_rx_usr_cb_param;
	uint32_t sync_rx_len;
	uint32_t sync_rx_status;
	void *user_data;
	void *usr_rx_buff;
	uint32_t usr_rx_size;
	uint8_t iir_cache;
	uint8_t busy_count;
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_busy_set(const struct device *dev)
{

	struct uart_sedi_drv_data *context = dev->data;

	context->busy_count++;

	if (context->busy_count == 1) {
		pm_device_busy_set(dev);
	}
}

static void uart_busy_clear(const struct device *dev)
{

	struct uart_sedi_drv_data *context = dev->data;

	context->busy_count--;

	if (context->busy_count == 0) {
		pm_device_busy_clear(dev);
	}
}
#endif

#ifdef CONFIG_PM_DEVICE

#ifndef CONFIG_UART_CONSOLE

static int uart_suspend_device(const struct device *dev)
{
	const struct uart_sedi_config_info *config = dev->config;

	if (pm_device_is_busy(dev)) {
		return -EBUSY;
	}

	int ret = sedi_uart_set_power(config->instance, SEDI_POWER_SUSPEND);

	if (ret != SEDI_DRIVER_OK) {
		return -EIO;
	}

	return 0;
}

static int uart_resume_device_from_suspend(const struct device *dev)
{
	const struct uart_sedi_config_info *config = dev->config;
	int ret;

	ret = sedi_uart_set_power(config->instance, SEDI_POWER_FULL);
	if (ret != SEDI_DRIVER_OK) {
		return -EIO;
	}

	return 0;
}

static int uart_sedi_pm_action(const struct device *dev,
		enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = uart_suspend_device(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = uart_resume_device_from_suspend(dev);
		break;

	default:
		ret = -ENOTSUP;
	}
	return ret;
}

#else

static int uart_sedi_pm_action(const struct device *dev,
		enum pm_device_action action)
{
	/* do nothing if using UART print log to avoid clock gating
	 * pm driver already handled power management for uart.
	 */
	return 0;
}

#endif /* CONFIG_UART_CONSOLE */

#endif /* CONFIG_PM_DEVICE */

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

#ifdef CONFIG_UART_LINE_CTRL
static int get_xfer_error(int bsp_err)
{
	int err;

	switch (bsp_err) {
	case SEDI_DRIVER_OK:
		err = 0;
		break;
	case SEDI_USART_ERROR_CANCELED:
		err = -ECANCELED;
		break;
	case SEDI_DRIVER_ERROR:
		err = -EIO;
		break;
	case SEDI_DRIVER_ERROR_PARAMETER:
		err = -EINVAL;
		break;
	case SEDI_DRIVER_ERROR_UNSUPPORTED:
		err = -ENOTSUP;
		break;
	default:
		err = -EFAULT;
	}
	return err;
}
#endif  /* CONFIG_UART_LINE_CTRL */

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

	if (status & SEDI_UART_RX_BI) {
		ret_status = UART_BREAK;
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

	uart_busy_set(dev);
	sedi_uart_irq_rx_enable(instance);
}

static void uart_sedi_irq_rx_disable(const struct device *dev)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	sedi_uart_irq_rx_disable(instance);
	uart_busy_clear(dev);
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
	struct device *dev = arg;
	struct uart_sedi_drv_data *drv_data = dev->data;

	if (drv_data->user_cb) {
		drv_data->user_cb(dev, drv_data->user_data);
	} else {
		uart_sedi_cb(dev);
	}
}

/* Called from generic callback of zephyr , set by set_cb. */
static void uart_sedi_cb(struct device *port)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(port);

	sedi_uart_isr_handler(instance);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
static int uart_sedi_line_ctrl_set(struct device *dev,
		uint32_t ctrl, uint32_t val)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);
	sedi_uart_config_t cfg;
	uint32_t mask;
	int ret;

	k_mutex_lock(GET_MUTEX(dev), K_FOREVER);
	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		sedi_uart_get_config(instance, &cfg);
		cfg.baud_rate = val;
		ret = sedi_uart_set_config(instance, &cfg);
		break;

	default:
		ret = -ENODEV;
	}
	k_mutex_unlock(GET_MUTEX(dev));
	ret = get_xfer_error(ret);
	return ret;
}

static int uart_sedi_line_ctrl_get(struct device *dev,
		uint32_t ctrl, uint32_t *val)
{
	sedi_uart_t instance = GET_CONTROLLER_INSTANCE(dev);
	sedi_uart_config_t cfg;
	uint32_t mask;
	int ret;

	k_mutex_lock(GET_MUTEX(dev), K_FOREVER);
	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		ret = sedi_uart_get_config(instance, &cfg);
		*val = cfg.baud_rate;
		break;

	case UART_LINE_CTRL_LOOPBACK:
		ret = sedi_uart_get_loopback_mode(instance, (uint32_t *)val);
		break;

	case UART_LINE_CTRL_AFCE:
		ret = sedi_uart_get_config(instance, &cfg);
		*val = cfg.hw_fc;
		break;

	case UART_LINE_CTRL_LINE_STATUS_REPORT_MASK:
		mask = 0;
		*val = 0;
		ret = sedi_get_ln_status_report_mask(instance,
						     (uint32_t *)&mask);
		*val |= ((mask & SEDI_UART_RX_OE) ? UART_ERROR_OVERRUN : 0);
		*val |= ((mask & SEDI_UART_RX_PE) ? UART_ERROR_PARITY : 0);
		*val |= ((mask & SEDI_UART_RX_FE) ? UART_ERROR_FRAMING : 0);
		*val |= ((mask & SEDI_UART_RX_BI) ? UART_BREAK : 0);
		break;

	case UART_LINE_CTRL_RTS:
		ret = sedi_uart_read_rts(instance, (uint32_t *)val);
		break;

	case UART_LINE_CTRL_CTS:
		ret = sedi_uart_read_cts(instance, (uint32_t *)val);
		break;


	default:
		ret = -ENODEV;
	}
	k_mutex_unlock(GET_MUTEX(dev));
	ret = get_xfer_error(ret);
	return ret;
}

#endif /* CONFIG_UART_LINE_CTRL */

static DEVICE_API(uart, api) = {
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

};

static int uart_sedi_init(const struct device *dev)
{

	const struct uart_sedi_config_info *config = dev->config;
	sedi_uart_config_t cfg;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	sedi_uart_init(config->instance, (void *)DEVICE_MMIO_GET(dev));

	cfg.line_control = config->line_ctrl;
	cfg.baud_rate = config->baud_rate;
	cfg.hw_fc = config->hw_fc;

	/* Setting to full power and enabling clk. */
	sedi_uart_set_power(config->instance, SEDI_POWER_FULL);

	sedi_uart_set_config(config->instance, &cfg);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->uart_irq_config_func(dev);
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

DT_INST_FOREACH_STATUS_OKAY(UART_SEDI_DEVICE_INIT)
