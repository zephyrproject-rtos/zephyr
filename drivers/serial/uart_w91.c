/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <ipc/ipc_based_driver.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_telink);

/* Driver dts compatibility: telink,w91_uart */
#define DT_DRV_COMPAT telink_w91_uart

#define W91_UART_CB_THREAD_NAME "uart_irq_cb"

enum {
	IPC_DISPATCHER_UART_CONFIG = IPC_DISPATCHER_UART,
	IPC_DISPATCHER_UART_IRQ_EVENT,
	IPC_DISPATCHER_UART_TRANSMIT,
	IPC_DISPATCHER_UART_RECEIVE,
	IPC_DISPATCHER_UART_RESET,
	IPC_DISPATCHER_UART_GET_RX_BUF,
	IPC_DISPATCHER_UART_IRQ_RX_ENABLE,
	IPC_DISPATCHER_UART_IRQ_TX_ENABLE,
	IPC_DISPATCHER_UART_IRQ_RX_READY,
	IPC_DISPATCHER_UART_IRQ_TX_READY,
	IPC_DISPATCHER_UART_POLL_IN,
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

static struct k_thread uart_rx_thread_data;
K_THREAD_STACK_DEFINE(uart_rx_thread_stack, CONFIG_TELINK_W91_UART_RX_THREAD_STACK_SIZE);
K_SEM_DEFINE(uart_sem, 0, 1);

/* w91 UART data structure */
struct uart_w91_data {
	struct uart_config cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
	struct ipc_based_driver ipc;    /* ipc driver part */
};

/* W91 UART config structure */
struct uart_w91_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t baud_rate;
	bool hw_flow_control;
	uint8_t instance_id;
};

struct uart_w91_config_req {
	uint32_t baudrate;
	uint8_t parity;
	uint8_t stop_bits;
	uint8_t data_bits;
	uint8_t flow_ctrl;
};

/* W91 UART transcieve structures */
struct uart_tx_req {
	uint32_t tx_len;
	uint8_t *tx_buf;
};

struct uart_rx_req {
	uint32_t rx_len;
};

struct uart_rx_resp {
	int err;
	uint32_t rx_len;
	uint8_t *rx_buf;
};

/* API implementation: irq handler */
static void uart_w91_irq_handler(const struct device *dev)
{
#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	ARG_UNUSED(dev);
#else
	struct uart_w91_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, data->cb_data);
	}
#endif
}

/* APIs implementation: configure */
static size_t pack_uart_w91_configure(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct uart_w91_config_req *p_config_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_config_req->baudrate) + sizeof(p_config_req->parity) +
			sizeof(p_config_req->stop_bits) + sizeof(p_config_req->data_bits) +
			sizeof(p_config_req->flow_ctrl);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_CONFIG, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->baudrate);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->parity);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->stop_bits);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->data_bits);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->flow_ctrl);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_configure);

static int uart_w91_configure(const struct device *dev,
			      const struct uart_config *cfg)
{
	int err;
	struct uart_w91_config_req config_req;
	struct uart_w91_data *data = dev->data;

	config_req.baudrate = cfg->baudrate;

	/* check parity */
	if (cfg->parity == UART_CFG_PARITY_NONE) {
		config_req.parity = UART_PARITY_NONE;
	} else if (cfg->parity == UART_CFG_PARITY_ODD) {
		config_req.parity = UART_PARITY_ODD;
	} else if (cfg->parity == UART_CFG_PARITY_EVEN) {
		config_req.parity = UART_PARITY_EVEN;
	} else {
		return -ENOTSUP;
	}

	/* check stop bits */
	if (cfg->stop_bits == UART_CFG_STOP_BITS_1) {
		config_req.stop_bits = UART_STOP_BIT_1;
	} else if (cfg->stop_bits == UART_CFG_STOP_BITS_2) {
		config_req.stop_bits = UART_STOP_BIT_2;
	} else {
		return -ENOTSUP;
	}

	/* check data bits */
	if (cfg->data_bits == UART_CFG_DATA_BITS_5) {
		config_req.data_bits = UART_DATA_BITS_5;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_6) {
		config_req.data_bits = UART_DATA_BITS_6;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_7) {
		config_req.data_bits = UART_DATA_BITS_7;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_8) {
		config_req.data_bits = UART_DATA_BITS_8;
	} else {
		return -ENOTSUP;
	}

	/* check flow control */
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE &&
		cfg->flow_ctrl != UART_CFG_FLOW_CTRL_RTS_CTS) {
		return -ENOTSUP;
	}

	/* save configuration */
	data->cfg = *cfg;

	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		uart_w91_configure, &config_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: transmit */
static size_t pack_uart_w91_transmit(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct uart_tx_req *p_tx_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_tx_req->tx_len) + p_tx_req->tx_len;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_TRANSMIT, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_tx_req->tx_len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_tx_req->tx_buf,
					  p_tx_req->tx_len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_transmit);

static int uart_w91_transmit(const struct device *dev,
			      const struct uart_tx_req *tx_req)
{
	int err = -1;
	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, uart_w91_transmit, tx_req,
				      &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	return err;
}

/* APIs implementation: get rx buf */
static size_t pack_uart_w91_wrap_poll_in(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct uart_rx_req *p_rx_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_rx_req->rx_len);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_POLL_IN, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_rx_req->rx_len);
	}

	return pack_data_len;
}

static void unpack_uart_w91_wrap_poll_in(void *unpack_data, const uint8_t *pack_data,
					   size_t pack_data_len)
{
	struct uart_rx_resp *p_rx_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->rx_len);
	size_t expect_len =
		sizeof(uint32_t) + sizeof(p_rx_resp->err) +
		sizeof(p_rx_resp->rx_len) + p_rx_resp->rx_len;

	if (expect_len != pack_data_len) {
		LOG_ERR("Invalid RX length (exp %d/ got %d)", expect_len, pack_data_len);
		return;
	}
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_rx_resp->rx_buf, p_rx_resp->rx_len);
}

static int uart_w91_wrap_poll_in(const struct device *dev,
							uint8_t *rx_buf, uint32_t len)
{
	struct uart_rx_resp rx_resp = {
		.err = -1,
		.rx_buf = rx_buf,
	};
	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, uart_w91_wrap_poll_in, &len,
				      &rx_resp, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (rx_resp.rx_len == 0) {
		rx_resp.err = -ENOENT;
	}

	return rx_resp.err;
}

/* APIs implementation: get rx buf */
static size_t pack_uart_w91_retrieve_rx_buf(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct uart_rx_req *p_rx_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_rx_req->rx_len);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_GET_RX_BUF, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_rx_req->rx_len);
	}

	return pack_data_len;
}

static void unpack_uart_w91_retrieve_rx_buf(void *unpack_data, const uint8_t *pack_data,
					   size_t pack_data_len)
{
	struct uart_rx_resp *p_rx_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->rx_len);
	size_t expect_len =
		sizeof(uint32_t) + sizeof(p_rx_resp->err) +
		sizeof(p_rx_resp->rx_len) + p_rx_resp->rx_len;

	if (expect_len != pack_data_len) {
		LOG_ERR("Invalid RX length (exp %d/ got %d)", expect_len, pack_data_len);
		return;
	}
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_rx_resp->rx_buf, p_rx_resp->rx_len);
}

static int uart_w91_retrieve_rx_buf(const struct device *dev,
							uint8_t *rx_buf, uint32_t len)
{
	struct uart_rx_resp rx_resp = {
		.err = -1,
		.rx_buf = rx_buf,
	};
	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, uart_w91_retrieve_rx_buf, &len,
				      &rx_resp, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (rx_resp.rx_len == 0) {
		rx_resp.err = -ENOENT;
	}

	return rx_resp.err;
}


/* APIs implementation: receive */
static size_t pack_uart_w91_receive(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct uart_rx_req *p_rx_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_rx_req->rx_len);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_RECEIVE, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_rx_req->rx_len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_receive);

static int uart_w91_receive(const struct device *dev, uint32_t len)
{
	int err = -1;
	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, uart_w91_receive, &len,
				      &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	return err;
}

/* APIs implementation: irq_tx_ready */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(uart_w91_tx_irq_ready, IPC_DISPATCHER_UART_IRQ_TX_READY);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_tx_irq_ready);

static int uart_w91_tx_irq_ready(const struct device *dev)
{
	int err;

	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		uart_w91_tx_irq_ready, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: irq_rx_enable */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(uart_w91_rx_irq_enable, IPC_DISPATCHER_UART_IRQ_RX_ENABLE);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_rx_irq_enable);

static int uart_w91_rx_irq_enable(const struct device *dev)
{
	int err = -1;

	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		uart_w91_rx_irq_enable, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: reset */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(uart_w91_reset, IPC_DISPATCHER_UART_RESET);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_reset);

static int uart_w91_reset(const struct device *dev)
{
	int err = -1;

	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		uart_w91_reset, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* API implementation: config_get */
static int uart_w91_config_get(const struct device *dev,
			       struct uart_config *cfg)
{
	struct uart_w91_data *data = dev->data;
	*cfg = data->cfg;
	return 0;
}

static void uart_rx_w91_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;

	while (1) {
		k_sem_take(&uart_sem, K_FOREVER);
		uart_w91_irq_handler(dev);

		/* reload RX IRQ*/
		int err = uart_w91_receive(dev, 1);

		if (err != 0) {
			LOG_ERR("RX re-enable failed(%d)", err);
		}
	}
}

/* APIs implementation: irq_callback */
static void uart_w91_irq_cb(const void *data, size_t len, void *param)
{
	const struct device *dev = param;
	struct uart_w91_data *dev_data = dev->data;

	k_sem_give(&uart_sem);
}

static bool thread_initialized;
/* API implementation: driver initialization */
static int uart_w91_driver_init(const struct device *dev)
{
	int status = 0;
	const struct uart_w91_config *cfg = dev->config;
	struct uart_w91_data *data = dev->data;

	ipc_based_driver_init(&data->ipc);

	/* once for all UART instances */
	if (!thread_initialized) {
		k_tid_t thread_id = k_thread_create(&uart_rx_thread_data,
			uart_rx_thread_stack, K_THREAD_STACK_SIZEOF(uart_rx_thread_stack),
			uart_rx_w91_thread, dev, NULL, NULL,
			CONFIG_TELINK_W91_UART_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
		if (k_thread_name_set(thread_id, W91_UART_CB_THREAD_NAME) != 0) {
			LOG_ERR("failed to set name to %d\n", (int)thread_id);
		}
	}
	thread_initialized = 1;

	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_IRQ_EVENT, inst),
		uart_w91_irq_cb, (void *)dev);
	struct uart_config *config = &data->cfg;

	/* configure pins */
	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		return status;
	}

	data->cfg.baudrate = cfg->baud_rate;
	data->cfg.data_bits = UART_CFG_DATA_BITS_8;
	data->cfg.parity = UART_CFG_PARITY_NONE;
	data->cfg.stop_bits = UART_CFG_STOP_BITS_1;

	status = uart_w91_configure(dev, config);

	return 0;
}

/* API implementation: poll_out */
static void uart_w91_poll_out(const struct device *dev, uint8_t c)
{
	struct uart_tx_req tx_req = {
		.tx_len = 1,
		.tx_buf = &c,
	};

	uart_w91_transmit(dev, &tx_req);
}

/* API implementation: poll_in */
static int uart_w91_poll_in(const struct device *dev, unsigned char *c)
{
	if (uart_w91_wrap_poll_in(dev, c, 1) != 0) {
		return -1;
	}

	return 0;
}

/* API implementation: err_check */
static int uart_w91_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* API implementation: fifo_fill */
static int uart_w91_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data,
			      int size)
{
	int err = -1;
	struct uart_tx_req tx_req = {
		.tx_len = size,
		.tx_buf = tx_data,
	};

	err = uart_w91_transmit(dev, &tx_req);
	if (err < 0) {
		return err;
	}

	return size;
}

/* API implementation: fifo_read */
static int uart_w91_fifo_read(const struct device *dev,
			      uint8_t *rx_data,
			      const int size)
{
	if (uart_w91_retrieve_rx_buf(dev, rx_data, size) != 0) {
		return 0;
	}
	return size;
}

/* API implementation: irq_tx_enable */
static void uart_w91_irq_tx_enable(const struct device *dev)
{
	uart_w91_irq_handler(dev);
}

/* API implementation: irq_tx_disable */
static void uart_w91_irq_tx_disable(const struct device *dev)
{
	uart_w91_reset(dev);
}

/* API implementation: irq_tx_ready */
static int uart_w91_irq_tx_ready(const struct device *dev)
{
	return uart_w91_tx_irq_ready(dev);
}

/* API implementation: irq_tx_complete */
static int uart_w91_irq_tx_complete(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/* API implementation: irq_rx_enable */
static void uart_w91_irq_rx_enable(const struct device *dev)
{
	uart_w91_rx_irq_enable(dev);
}

/* API implementation: irq_rx_disable */
static void uart_w91_irq_rx_disable(const struct device *dev)
{
	uart_w91_reset(dev);
}

/* API implementation: irq_rx_ready */
static int uart_w91_irq_rx_ready(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

/* API implementation: irq_err_enable */
static void uart_w91_irq_err_enable(const struct device *dev)
{
}

/* API implementation: irq_err_disable*/
static void uart_w91_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/* API implementation: irq_is_pending */
static int uart_w91_irq_is_pending(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/* API implementation: irq_update */
static int uart_w91_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

/* API implementation: irq_callback_set */
static void uart_w91_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_w91_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
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
	PINCTRL_DT_INST_DEFINE(n);                                  \
	                                                            \
	static const struct uart_w91_config uart_w91_cfg_##n =      \
	{                                                           \
		.baud_rate = DT_INST_PROP(n, current_speed),            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),              \
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control),     \
		.instance_id = n                                        \
	};                                                          \
	                                                            \
	static struct uart_w91_data uart_w91_data_##n;				    \
										    \
	DEVICE_DT_INST_DEFINE(n, uart_w91_driver_init,				    \
			      PM_DEVICE_DT_INST_GET(n),				    \
			      &uart_w91_data_##n,				    \
			      &uart_w91_cfg_##n,				    \
			      POST_KERNEL,					    \
			      CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,		\
			      (void *)&uart_w91_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_W91_INIT)
