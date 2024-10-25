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

enum {
	IPC_DISPATCHER_UART_CONFIG = IPC_DISPATCHER_UART,
	IPC_DISPATCHER_UART_SEND,
	IPC_DISPATCHER_UART_READ,
	IPC_DISPATCHER_UART_IRQ_STATE_CHANGE,
	IPC_DISPATCHER_UART_IRQ_EVENT,
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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
enum uart_irq_type {
	/* RX irq */
	UART_IRQ_RX_ENABLE,
	UART_IRQ_RX_READY,
	UART_IRQ_RX_DONE,

	/* TX irq */
	UART_IRQ_TX_ENABLE,
	UART_IRQ_TX_DONE,

	/* ERR irq */
	UART_IRQ_ERR_ENABLE,
	UART_IRQ_ERR_OVERRUN,
	UART_IRQ_ERR_PARITY,
	UART_IRQ_ERR_FRAMING,
	UART_IRQ_ERR_LINE_BREAK,
};

struct uart_irq_status {
	bool rx_enable   : 1;
	bool tx_enable   : 1;
	bool rx_ready    : 1;
	bool tx_ready    : 1;
	bool tx_complete : 1;
};

struct uart_irq_state_change_req {
	uint8_t irq;
	uint8_t state;
};

#define UART_MSG_QUEUE_SIZE 16
#define MSG_SIZE            sizeof(const struct device *)

K_MSGQ_DEFINE(uart_irq_msgq, MSG_SIZE, UART_MSG_QUEUE_SIZE, 4);
static bool uart_irq_thread_initialized;

static struct k_thread uart_irq_thread_data;
K_THREAD_STACK_DEFINE(uart_irq_thread_stack, CONFIG_TELINK_W91_UART_IRQ_THREAD_STACK_SIZE);
#endif

/* w91 UART data structure */
struct uart_w91_data {
	struct uart_config cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct uart_irq_status irq_status;
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
	uint16_t len;
	uint8_t *buf;
};

struct uart_rx_resp {
	int err;
	uint16_t len;
	uint8_t *buf;
};

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
	int err = -ETIMEDOUT;
	struct uart_w91_config_req config_req = {};
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

/* APIs implementation: send */
static size_t pack_uart_w91_send(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct uart_tx_req *p_tx_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) + sizeof(p_tx_req->len) + p_tx_req->len;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_SEND, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_tx_req->len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_tx_req->buf, p_tx_req->len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_send);

static int uart_w91_send(const struct device *dev, const uint8_t *tx_data, int size)
{
	int err = -ETIMEDOUT;
	struct uart_tx_req tx_req = {
		.len = size,
		.buf = (uint8_t *)tx_data,
	};

	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, uart_w91_send, &tx_req,
			 &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* API implementation: read */
static size_t pack_uart_w91_read(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	uint16_t *p_rx_len_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) + sizeof(*p_rx_len_req);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_READ, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, *p_rx_len_req);
	}

	return pack_data_len;
}

static void unpack_uart_w91_read(void *unpack_data, const uint8_t *pack_data, size_t pack_data_len)
{
	struct uart_rx_resp *p_rx_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_rx_resp->len);

	size_t expect_len =
		sizeof(uint32_t) + sizeof(p_rx_resp->err) +
		sizeof(p_rx_resp->len) + p_rx_resp->len;

	if (expect_len != pack_data_len) {
		LOG_ERR("Invalid RX length (exp %d/ got %d)", expect_len, pack_data_len);
		return;
	}

	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_rx_resp->buf, p_rx_resp->len);
}

static int uart_w91_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_rx_resp rx_resp = {
		.err = -ETIMEDOUT,
		.buf = rx_data,
	};

	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, uart_w91_read, (uint16_t *)&size,
			&rx_resp, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return rx_resp.err;
}

/* API implementation: config_get */
static int uart_w91_config_get(const struct device *dev,
			       struct uart_config *cfg)
{
	struct uart_w91_data *data = dev->data;

	*cfg = data->cfg;

	return 0;
}

/* API implementation: poll_in */
static int uart_w91_poll_in(const struct device *dev, unsigned char *c)
{
	int err = uart_w91_read(dev, (uint8_t *)c, sizeof(*c));

	return err;
}

/* API implementation: poll_out */
static void uart_w91_poll_out(const struct device *dev, uint8_t c)
{
	int err = -ETIMEDOUT;

	err = uart_w91_send(dev, &c, sizeof(c));
	if (err) {
		LOG_ERR("tx (poll_out) failed(%d)\n", err);
	}
}

/* API implementation: err_check */
static int uart_w91_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* API implementation: fifo_fill */
static int uart_w91_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	int err = -ETIMEDOUT;
	struct uart_w91_data *data = dev->data;

	if (!data->irq_status.tx_enable || !data->irq_status.tx_ready) {
		LOG_ERR("tx (fifo_fill) is not ready\n");
		return -EACCES;
	}

	data->irq_status.tx_ready = false;
	data->irq_status.tx_complete = false;

	err = uart_w91_send(dev, tx_data, size);
	if (err) {
		LOG_ERR("tx (fifo_fill) failed(%d)\n", err);
		data->irq_status.tx_ready = true;

		return 0;
	}

	return size;
}

/* API implementation: fifo_read */
static int uart_w91_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	int err = -ETIMEDOUT;
	struct uart_w91_data *data = dev->data;

	if (!data->irq_status.rx_enable || !data->irq_status.rx_ready) {
		return 0;
	}

	err = uart_w91_read(dev, rx_data, size);
	if (err) {
		LOG_ERR("rx (fifo_read) failed(%d)\n", err);

		return 0;
	}

	return size;
}

/* APIs implementation: irq_state_change */
static size_t pack_uart_w91_irq_state_change(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct uart_irq_state_change_req *p_irq_state_change_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
		sizeof(p_irq_state_change_req->irq) + sizeof(p_irq_state_change_req->state);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_IRQ_STATE_CHANGE, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_irq_state_change_req->irq);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_irq_state_change_req->state);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(uart_w91_irq_state_change);

static int uart_w91_irq_state_change(const struct device *dev,
		enum uart_irq_type irq, bool isIrqEnable)
{
	int err = -ETIMEDOUT;
	struct uart_irq_state_change_req irq_state_change_req = {
		.irq = irq,
		.state = isIrqEnable,
	};

	struct ipc_based_driver *ipc_data = &((struct uart_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		uart_w91_irq_state_change, &irq_state_change_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* API implementation: irq_tx_enable */
static void uart_w91_irq_tx_enable(const struct device *dev)
{
	int err;

	err = uart_w91_irq_state_change(dev, UART_IRQ_TX_ENABLE, true);
	if (err) {
		LOG_ERR("irq tx enable failed(%d)\n", err);
	}
}

/* API implementation: irq_tx_disable */
static void uart_w91_irq_tx_disable(const struct device *dev)
{
	int err;
	struct uart_w91_data *data = dev->data;

	err = uart_w91_irq_state_change(dev, UART_IRQ_TX_ENABLE, false);
	if (err) {
		LOG_ERR("irq tx disable failed(%d)\n", err);
		return;
	}

	data->irq_status.tx_enable = false;
}

/* API implementation: irq_tx_ready */
static int uart_w91_irq_tx_ready(const struct device *dev)
{
	struct uart_w91_data *data = dev->data;

	return (data->irq_status.tx_enable && data->irq_status.tx_ready);
}

/* API implementation: irq_tx_complete */
static int uart_w91_irq_tx_complete(const struct device *dev)
{
	struct uart_w91_data *data = dev->data;

	return data->irq_status.tx_complete;
}

/* API implementation: irq_rx_enable */
static void uart_w91_irq_rx_enable(const struct device *dev)
{
	int err;

	err = uart_w91_irq_state_change(dev, UART_IRQ_RX_ENABLE, true);
	if (err) {
		LOG_ERR("irq rx enable failed(%d)\n", err);
	}
}

/* API implementation: irq_rx_disable */
static void uart_w91_irq_rx_disable(const struct device *dev)
{
	int err;
	struct uart_w91_data *data = dev->data;

	err = uart_w91_irq_state_change(dev, UART_IRQ_RX_ENABLE, false);
	if (err) {
		LOG_ERR("irq rx disable failed(%d)\n", err);
		return;
	}

	data->irq_status.rx_enable = false;
}

/* API implementation: irq_rx_ready */
static int uart_w91_irq_rx_ready(const struct device *dev)
{
	struct uart_w91_data *data = dev->data;

	return (data->irq_status.rx_enable && data->irq_status.rx_ready);
}

/* API implementation: irq_err_enable */
static void uart_w91_irq_err_enable(const struct device *dev)
{
	int err;

	err = uart_w91_irq_state_change(dev, UART_IRQ_ERR_ENABLE, true);
	if (err) {
		LOG_ERR("irq err enable failed(%d)\n", err);
	}
}

/* API implementation: irq_err_disable*/
static void uart_w91_irq_err_disable(const struct device *dev)
{
	int err;

	err = uart_w91_irq_state_change(dev, UART_IRQ_ERR_ENABLE, false);
	if (err) {
		LOG_ERR("irq err disable failed(%d)\n", err);
	}
}

/* API implementation: irq_is_pending */
static int uart_w91_irq_is_pending(const struct device *dev)
{
	return (uart_w91_irq_rx_ready(dev) || uart_w91_irq_tx_ready(dev));
}

/* API implementation: irq_update */
static int uart_w91_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* nothing to be done */
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

/* APIs implementation: irq_callback */
static int unpack_uart_w91_irq(void *unpack_data, const uint8_t *pack_data, size_t pack_data_len)
{
	uint8_t *p_irq = unpack_data;
	size_t expect_len = sizeof(uint32_t) + sizeof(*p_irq);

	if (expect_len != pack_data_len) {
		LOG_ERR("Invalid irq length (exp %d/ got %d)", expect_len, pack_data_len);
		return -EINVAL;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, *p_irq);

	return 0;
}

static void uart_w91_irq_cb(const void *data, size_t len, void *param)
{
	const struct device *dev = param;
	struct uart_w91_data *dev_data = dev->data;
	uint8_t irq;

	if (unpack_uart_w91_irq(&irq, data, len)) {
		return;
	}

	switch (irq) {
	case UART_IRQ_RX_ENABLE:
		dev_data->irq_status.rx_enable = true;
		break;
	case UART_IRQ_RX_READY:
		dev_data->irq_status.rx_ready = true;
		break;
	case UART_IRQ_RX_DONE:
		dev_data->irq_status.rx_ready = false;
		break;
	case UART_IRQ_TX_ENABLE:
		dev_data->irq_status.tx_enable = true;
		break;
	case UART_IRQ_TX_DONE:
		dev_data->irq_status.tx_ready = true;
		dev_data->irq_status.tx_complete = true;
		break;
	case UART_IRQ_ERR_OVERRUN:
	case UART_IRQ_ERR_PARITY:
	case UART_IRQ_ERR_FRAMING:
	case UART_IRQ_ERR_LINE_BREAK:
		LOG_ERR("err irq set (type = %d)\n", irq);
		return;
	default:
		return;
	}

	k_msgq_put(&uart_irq_msgq, &dev, K_NO_WAIT);
}

static void uart_w91_irq_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev;

	while (1) {
		if (k_msgq_get(&uart_irq_msgq, &dev, K_FOREVER) != 0) {
			return;
		}

		struct uart_w91_data *data = dev->data;

		if (data->callback != NULL) {
			data->callback(dev, data->cb_data);
		}
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/* API implementation: driver initialization */
static int uart_w91_driver_init(const struct device *dev)
{
	int err;
	const struct uart_w91_config *cfg = dev->config;
	struct uart_w91_data *data = dev->data;
	struct uart_config *config = &data->cfg;
	uint8_t inst = ((struct uart_w91_config *)dev->config)->instance_id;

	ipc_based_driver_init(&data->ipc);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* once for all UART instances */
	if (!uart_irq_thread_initialized) {
		k_tid_t thread_id = k_thread_create(&uart_irq_thread_data,
			uart_irq_thread_stack, K_THREAD_STACK_SIZEOF(uart_irq_thread_stack),
			uart_w91_irq_thread, NULL, NULL, NULL,
			CONFIG_TELINK_W91_UART_IRQ_THREAD_PRIORITY, 0, K_NO_WAIT);

		if (k_thread_name_set(thread_id, "uart_irq_cb") != 0) {
			LOG_ERR("failed to set name to %d\n", (int)thread_id);
		}
	}

	uart_irq_thread_initialized = true;
	data->irq_status.tx_ready = true;

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_UART_IRQ_EVENT, inst),
			uart_w91_irq_cb, (void *)dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	/* configure pins */
	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	data->cfg.baudrate = cfg->baud_rate;
	data->cfg.data_bits = UART_CFG_DATA_BITS_8;
	data->cfg.parity = UART_CFG_PARITY_NONE;
	data->cfg.stop_bits = UART_CFG_STOP_BITS_1;

	err = uart_w91_configure(dev, config);

	return err;
}

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
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control),    \
		.instance_id = DT_INST_PROP(n, instance_id),            \
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
			      (void *)&uart_w91_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_W91_INIT)
