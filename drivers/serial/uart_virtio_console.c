/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT virtio_console
#include <zephyr/device.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(virtio_console, CONFIG_UART_LOG_LEVEL);

struct virtconsole_config {
	const struct device *vdev;
};

struct _virtio_console_config {
	uint16_t cols;
	uint16_t rows;
	uint32_t max_nr_ports;
	uint32_t emerg_wr;
};
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
struct _virtio_console_control {
	uint32_t port;
	uint16_t event;
	uint16_t value;
	/* Device can give human-readable names to ports by sending */
	/* VIRTIO_CONSOLE_PORT_NAME immediately followed by a name */
	char name[CONFIG_UART_VIRTIO_CONSOLE_NAME_BUFSIZE];
};
struct _fifo_item_virtio_console_control {
	void *fifo_reserved;
	bool pending; /* true if message is awaiting transmission */
	struct _virtio_console_control msg;
};
#endif

enum _flags {
	RX_IRQ_ENABLED,
	TX_IRQ_ENABLED,
};

enum _virtio_feature_bits {
	VIRTIO_CONSOLE_F_SIZE,
	VIRTIO_CONSOLE_F_MULTIPORT,
	VIRTIO_CONSOLE_F_EMERG_WRITE
};

/* Virtqueues frequently used explicitly */
enum _named_virtqueues {
	VIRTQ_RX,
	VIRTQ_TX,
	VIRTQ_CONTROL_RX,
	VIRTQ_CONTROL_TX
};

enum virtconsole_tx_state {
	VIRTIO_CONSOLE_TX_READY,
	VIRTIO_CONSOLE_TX_BUSY,
	VIRTIO_CONSOLE_TX_UNAVAILABLE,
};

#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
enum _virtio_ctl_events {
	VIRTIO_CONSOLE_DEVICE_READY,
	VIRTIO_CONSOLE_DEVICE_ADD,
	VIRTIO_CONSOLE_DEVICE_REMOVE,
	VIRTIO_CONSOLE_PORT_READY,
	VIRTIO_CONSOLE_CONSOLE_PORT,
	VIRTIO_CONSOLE_RESIZE,
	VIRTIO_CONSOLE_PORT_OPEN,
	VIRTIO_CONSOLE_PORT_NAME
};

struct _ctl_cb_data {
	struct virtconsole_data *data;
	int buf_no;
};
#endif

/* This should be enough as QEMU only allows 31 */
#define VIRTIO_CONSOLE_MAX_PORTS 32

/* Allows virtconsole_recv_cb to know which virtqueue it was called by */
struct _rx_cb_data {
	struct virtconsole_data *data;
	uint16_t port;
};

/* Convert port numbers to virtqueue indices */
#define PORT_TO_RX_VQ_IDX(p) ((p) ? ((p + 1) * 2) : (VIRTQ_RX))
#define PORT_TO_TX_VQ_IDX(p) (PORT_TO_RX_VQ_IDX(p) + 1)

/* Convert virtqueue index to port number */
static int8_t vq_idx_to_port(uint16_t q)
{
	if (q % 2) { /* transmit queue (odd-numbered) */
		if (q == VIRTQ_TX) {
			return 0;
		} else if (q != VIRTQ_CONTROL_TX) {
			return (q / 2) - 1;
		}
	} else { /* receive queue (even-numbered) */
		if (q == VIRTQ_RX) {
			return 0;
		} else if (q != VIRTQ_CONTROL_RX) {
			return (q / 2) - 1;
		}
	}
	return -1; /* control queues are not assigned to any port */
}

struct virtconsole_data {
	const struct device *dev;
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	/* bitmask of ports to be used as console */
	uint32_t console_ports;
	int8_t n_console_ports;
	size_t txctlcurrent;
	struct _virtio_console_control rx_ctlbuf[CONFIG_UART_VIRTIO_CONSOLE_RX_CONTROL_BUFSIZE];
	struct _fifo_item_virtio_console_control
		tx_ctlbuf[CONFIG_UART_VIRTIO_CONSOLE_TX_CONTROL_BUFSIZE];
	struct k_fifo tx_ctlfifo;
	struct _ctl_cb_data ctl_cb_data[CONFIG_UART_VIRTIO_CONSOLE_RX_CONTROL_BUFSIZE];
	struct _rx_cb_data rx_cb_data[VIRTIO_CONSOLE_MAX_PORTS];
#else
	struct _rx_cb_data rx_cb_data[1];
#endif
	struct k_spinlock tx_lock;
	struct k_sem tx_done;
	char rxbuf[CONFIG_UART_VIRTIO_CONSOLE_RX_BUFSIZE],
		txbuf[CONFIG_UART_VIRTIO_CONSOLE_TX_BUFSIZE];
	atomic_t flags;
	atomic_t rx_started, rx_ready;
	size_t rxcurrent, txcurrent;
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
	struct _virtio_console_config *virtio_devcfg;
};

/* Return desired size for given virtqueue */
static uint16_t virtconsole_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *priv)
{
	ARG_UNUSED(q_size_max);
	ARG_UNUSED(priv);

	switch (q_index) {
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	case VIRTQ_CONTROL_RX:
		return CONFIG_UART_VIRTIO_CONSOLE_RX_CONTROL_BUFSIZE;
	case VIRTQ_CONTROL_TX:
		return CONFIG_UART_VIRTIO_CONSOLE_TX_CONTROL_BUFSIZE;
#endif
	default:
		return 1;
	}
}

static void virtconsole_recv_cb(void *priv, uint32_t len)
{
	struct _rx_cb_data *cbdata = priv;
	struct virtconsole_data *data = cbdata->data;

	atomic_set_bit(&(data->rx_ready), cbdata->port);
	if (atomic_test_bit(&data->flags, RX_IRQ_ENABLED) && data->irq_cb) {
		data->irq_cb(data->dev, data->irq_cb_data);
	}
}

static void virtconsole_recv_setup(const struct device *dev, uint16_t q_no, void *addr,
				   uint32_t len, void (*recv_cb)(void *priv, uint32_t rcv_len),
				   void *cb_data)
{
	if (q_no % 2) {
		return; /* This should not be called on tx queues (odd-numbered) */
	}
	const struct virtconsole_config *config = dev->config;
	struct virtconsole_data *data = dev->data;

	int port = vq_idx_to_port(q_no);

	if ((port >= 0) && (port < VIRTIO_CONSOLE_MAX_PORTS)) {
		atomic_set_bit(&(data->rx_started), port);
	}
	struct virtq *vq = virtio_get_virtqueue(config->vdev, q_no);

	if (vq == NULL) {
		LOG_ERR("could not access virtqueue %u", q_no);
		return;
	}
	struct virtq_buf vqbuf[] = {{.addr = addr, .len = len}};

	if (virtq_add_buffer_chain(vq, vqbuf, 1, 0, recv_cb, cb_data, K_NO_WAIT)) {
		LOG_ERR("could not set up virtqueue %u for receiving", q_no);
		return;
	}
	virtio_notify_virtqueue(config->vdev, q_no);
}
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
static void virtconsole_control_tx_flush(void *priv, uint32_t len)
{
	struct virtconsole_data *data = priv;
	const struct device *dev = data->dev;
	const struct virtconsole_config *config = dev->config;
	struct _fifo_item_virtio_console_control *item;
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_CONTROL_TX);

	if (vq == NULL) {
		LOG_ERR("could not access virtqueue 3");
		return;
	}
	int i = virtq_get_free_desc_count(vq);

	while ((i-- > 0) && (item = k_fifo_get(&data->tx_ctlfifo, K_NO_WAIT))) {
		struct virtq_buf vqbuf = {.addr = &item->msg, .len = sizeof(item->msg)};

		int ret = virtq_add_buffer_chain(vq, &vqbuf, 1, 1, virtconsole_control_tx_flush,
						 priv, K_NO_WAIT);

		if (ret) {
			LOG_ERR("could not send control message");
			return;
		}
		virtio_notify_virtqueue(config->vdev, VIRTQ_CONTROL_TX);
		item->pending = false;
	}
}

static void virtconsole_send_control_msg(const struct device *dev, uint32_t port, uint16_t event,
					 uint16_t value)
{
	const struct virtconsole_config *config = dev->config;
	struct virtconsole_data *data = dev->data;

	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_CONTROL_TX);

	if (vq == NULL) {
		LOG_ERR("could not access virtqueue 3");
		return;
	}
	struct _fifo_item_virtio_console_control *item = &(data->tx_ctlbuf[data->txctlcurrent]);
	struct _virtio_console_control *msg = &(item->msg);

	if (item->pending) {
		LOG_ERR("not enough free buffers for control message");
		return;
	}
	msg->port = sys_cpu_to_le32(port);
	msg->event = sys_cpu_to_le16(event);
	msg->value = sys_cpu_to_le16(value);
	struct virtq_buf vqbuf = {.addr = msg, .len = sizeof(*msg)};

	int ret = virtq_add_buffer_chain(vq, &vqbuf, 1, 1, virtconsole_control_tx_flush, data,
					 K_NO_WAIT);

	if (ret == -EBUSY) {
		/* put in FIFO to be sent later, mark buffer as occupied to prevent overwriting */
		k_fifo_put(&data->tx_ctlfifo, data->tx_ctlbuf + data->txctlcurrent);
		item->pending = true;
	} else if (ret == 0) {
		virtio_notify_virtqueue(config->vdev, VIRTQ_CONTROL_TX);
	} else {
		LOG_ERR("could not send control message");
		return;
	}
	data->txctlcurrent =
		(data->txctlcurrent + 1) % CONFIG_UART_VIRTIO_CONSOLE_TX_CONTROL_BUFSIZE;
}

static void virtconsole_control_recv_cb(void *priv, uint32_t len)
{
	struct _ctl_cb_data *ctld = priv;
	struct virtconsole_data *data = ctld->data;

	for (int i = 0; i < CONFIG_UART_VIRTIO_CONSOLE_RX_CONTROL_BUFSIZE; i++) {
		if (data->rx_ctlbuf[i].port == UINT32_MAX) {
			continue;
		}
		data->rx_ctlbuf[i].port = sys_le32_to_cpu(data->rx_ctlbuf[i].port);
		data->rx_ctlbuf[i].event = sys_le16_to_cpu(data->rx_ctlbuf[i].event);
		data->rx_ctlbuf[i].value = sys_le16_to_cpu(data->rx_ctlbuf[i].value);

		switch (data->rx_ctlbuf[i].event) {
		case VIRTIO_CONSOLE_DEVICE_ADD:
			virtconsole_send_control_msg(
				data->dev, data->rx_ctlbuf[i].port, VIRTIO_CONSOLE_PORT_READY,
				(data->rx_ctlbuf[i].port) < VIRTIO_CONSOLE_MAX_PORTS);
			break;
		case VIRTIO_CONSOLE_DEVICE_REMOVE: {
			int port = data->rx_ctlbuf[i].port;

			if ((port < VIRTIO_CONSOLE_MAX_PORTS) &&
			    IS_BIT_SET(data->console_ports, port)) {
				/* Remove console port (unset bit) */
				data->console_ports = ~(data->console_ports);
				data->console_ports |= BIT(port);
				data->console_ports = ~(data->console_ports);
				data->n_console_ports--;
			}
			break;
		}
		case VIRTIO_CONSOLE_CONSOLE_PORT: {
			int port = data->rx_ctlbuf[i].port;

			if ((port < VIRTIO_CONSOLE_MAX_PORTS) &&
			    !IS_BIT_SET(data->console_ports, port)) {
				data->console_ports |= BIT(port);
				data->n_console_ports++;
			}
			virtconsole_send_control_msg(data->dev, data->rx_ctlbuf[i].port,
						     VIRTIO_CONSOLE_PORT_OPEN, 1);

			if (atomic_test_bit(&data->flags, RX_IRQ_ENABLED)) {
				if (!atomic_test_bit(&(data->rx_started), port)) {
					uint16_t q_no = PORT_TO_RX_VQ_IDX(port);

					virtconsole_recv_setup(data->dev, q_no,
							       data->rxbuf + data->rxcurrent,
							       sizeof(char), virtconsole_recv_cb,
							       data->rx_cb_data + port);
				}
			}
			break;
		}
		case VIRTIO_CONSOLE_RESIZE:
			/* Terminal sizes are not supported by Zephyr and the */
			/* VIRTIO_CONSOLE_F_SIZE feature was not enabled */
			LOG_WRN("device tried to set console size");
			break;
		case VIRTIO_CONSOLE_PORT_OPEN:
			LOG_INF("port %u is ready", data->rx_ctlbuf[i].port);
			break;
		case VIRTIO_CONSOLE_PORT_NAME:
			LOG_INF("port %u is named \"%.*s\"", data->rx_ctlbuf[i].port,
				(int)ARRAY_SIZE(data->rx_ctlbuf[i].name), data->rx_ctlbuf[i].name);
			break;
		default:
			break;
		}
		data->rx_ctlbuf[i].port = UINT32_MAX;
		memset(&(data->rx_ctlbuf[i].name), 0, ARRAY_SIZE(data->rx_ctlbuf[i].name));
	}
	virtconsole_recv_setup(data->dev, VIRTQ_CONTROL_RX, &data->rx_ctlbuf[ctld->buf_no],
			       sizeof(struct _virtio_console_control), virtconsole_control_recv_cb,
			       ctld);
}
#endif
static int virtconsole_poll_in(const struct device *dev, unsigned char *c)
{
	struct virtconsole_data *data = dev->data;
	int ready = -1;
	int port = 0;

#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	int n_ports_checked = 0;

	for (; port < VIRTIO_CONSOLE_MAX_PORTS; port++) {
		if (!IS_BIT_SET(data->console_ports, port)) {
			continue;
		}
#endif
		uint16_t q_no = PORT_TO_RX_VQ_IDX(port);

		if (!atomic_test_bit(&(data->rx_started), port)) {
			virtconsole_recv_setup(dev, q_no, data->rxbuf + data->rxcurrent,
					       sizeof(char), virtconsole_recv_cb,
					       data->rx_cb_data + port);
		}
		if (atomic_test_and_clear_bit(&(data->rx_ready), port)) {
			ready = q_no;
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
			break;
#endif
		}
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
		if ((++n_ports_checked) >= data->n_console_ports) {
			break;
		}
	}
#endif
	if (ready == -1) {
		return -1;
	}
	if (c) {
		*c = data->rxbuf[data->rxcurrent];
	}
	data->rxcurrent = (data->rxcurrent + 1) % CONFIG_UART_VIRTIO_CONSOLE_RX_BUFSIZE;
	virtconsole_recv_setup(dev, ready, data->rxbuf + data->rxcurrent, sizeof(char),
			       virtconsole_recv_cb, data->rx_cb_data + port);
	return 0;
}

/*
 * A queue is idle when all of its descriptors are free, so no descriptor can
 * refer to a TX buffer that is about to be rewritten. Console data queues are
 * created with a size of one, but checking the whole queue stays correct if
 * that ever changes.
 */
static enum virtconsole_tx_state
virtconsole_tx_queues_check(const struct virtconsole_config *config, uint32_t console_ports,
			    bool reclaim_used)
{
	uint32_t port = 0U;
	bool found = false;
	bool busy = false;

#ifndef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	ARG_UNUSED(console_ports);
#endif

#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	for (; port < VIRTIO_CONSOLE_MAX_PORTS; port++) {
		if (!IS_BIT_SET(console_ports, port)) {
			continue;
		}
#endif
		found = true;
		struct virtq *vq = virtio_get_virtqueue(config->vdev, PORT_TO_TX_VQ_IDX(port));

		if (vq == NULL) {
			return VIRTIO_CONSOLE_TX_UNAVAILABLE;
		} else if (reclaim_used) {
			virtq_process_used(vq);
		}

		if (virtq_get_free_desc_count(vq) != vq->num) {
			busy = true;
		}
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	}
#endif
	if (!found) {
		return VIRTIO_CONSOLE_TX_UNAVAILABLE;
	}

	return busy ? VIRTIO_CONSOLE_TX_BUSY : VIRTIO_CONSOLE_TX_READY;
}

static void virtconsole_tx_done(void *opaque, uint32_t used_len)
{
	struct virtconsole_data *data = opaque;

	ARG_UNUSED(used_len);
	k_sem_give(&data->tx_done);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void virtconsole_tx_irq(void *opaque)
{
	struct virtconsole_data *data = opaque;

	if (atomic_test_bit(&data->flags, TX_IRQ_ENABLED) && (data->irq_cb != NULL)) {
		data->irq_cb(data->dev, data->irq_cb_data);
	}
}
#endif

static bool virtconsole_send_char(const struct device *dev, unsigned char c, bool wait)
{
	const struct virtconsole_config *config = dev->config;
	struct virtconsole_data *data = dev->data;
	bool in_isr = k_is_in_isr();
	bool can_yield = !in_isr && k_can_yield();
	bool poll_completion = wait && !in_isr && !can_yield;
	uint32_t port = 0U;
	int ret = 0;
	uint32_t ports = 0U;
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

	/*
	 * The spinlock protects only queue checks and non-blocking submissions.
	 * Polling output from a non-yielding thread reclaims completed descriptors
	 * directly. FIFO writes and ISR callers never wait for a busy queue.
	 */

	/*
	 * A buffer must not be rewritten while a descriptor referring to it is
	 * in flight. Wait until every TX queue is completely idle before
	 * writing txbuf[].
	 */
	for (;;) {
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
		/*
		 * Refresh the snapshot after every lock acquisition. A port added
		 * while this sender waits must be checked before txbuf[] is
		 * rewritten and must receive the character.
		 */
		ports = data->console_ports;
#endif
		enum virtconsole_tx_state state =
			virtconsole_tx_queues_check(config, ports, poll_completion);

		if (state == VIRTIO_CONSOLE_TX_READY) {
			break;
		} else if ((state == VIRTIO_CONSOLE_TX_UNAVAILABLE) || !wait || in_isr) {
			k_spin_unlock(&data->tx_lock, key);
			return false;
		}

		k_spin_unlock(&data->tx_lock, key);
		if (can_yield) {
			if (k_sem_take(&data->tx_done, K_FOREVER) != 0) {
				return false;
			}
		} else {
			arch_spin_relax();
		}

		key = k_spin_lock(&data->tx_lock);
	}

#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	for (; port < VIRTIO_CONSOLE_MAX_PORTS; port++) {
		if (!IS_BIT_SET(ports, port)) {
			continue;
		}
#endif
		uint16_t q_no = PORT_TO_TX_VQ_IDX(port);
		struct virtq *vq = virtio_get_virtqueue(config->vdev, q_no);

		if (vq == NULL) {
			ret = -ENODEV;
		} else {
			data->txbuf[data->txcurrent] = c;
			struct virtq_buf vqbuf = {.addr = data->txbuf + data->txcurrent,
						  .len = sizeof(char)};

			ret = virtq_add_buffer_chain(vq, &vqbuf, 1, 1, virtconsole_tx_done, data,
						     K_NO_WAIT);
			if (ret == 0) {
				virtio_notify_virtqueue(config->vdev, q_no);
			}
		}
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
		if (ret != 0) {
			break;
		}
	}
#endif
	if (ret == 0) {
		data->txcurrent = (data->txcurrent + 1U) % CONFIG_UART_VIRTIO_CONSOLE_TX_BUFSIZE;
	}

	k_spin_unlock(&data->tx_lock, key);

	return (ret == 0);
}

static void virtconsole_poll_out(const struct device *dev, unsigned char c)
{
	(void)virtconsole_send_char(dev, c, true);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int virtconsole_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	int i = 0;

	for (; i < size; i++) {
		if (!virtconsole_send_char(dev, tx_data[i], false)) {
			break;
		}
	}
	return i;
}
static int virtconsole_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	int i = 0;

	for (; i < size; i++) {
		if (virtconsole_poll_in(dev, rx_data + i) == -1) {
			break;
		}
	}
	return i;
}
static void virtconsole_irq_tx_enable(const struct device *dev)
{
	struct virtconsole_data *data = dev->data;

	atomic_set_bit(&data->flags, TX_IRQ_ENABLED);
	if (data->irq_cb != NULL) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}
static void virtconsole_irq_tx_disable(const struct device *dev)
{
	struct virtconsole_data *data = dev->data;

	atomic_clear_bit(&data->flags, TX_IRQ_ENABLED);
}
static int virtconsole_irq_tx_ready(const struct device *dev)
{
	const struct virtconsole_config *config = dev->config;
	struct virtconsole_data *data = dev->data;
	uint32_t ports = 0U;
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	ports = data->console_ports;
#endif
	bool ready = virtconsole_tx_queues_check(config, ports, false) == VIRTIO_CONSOLE_TX_READY;

	k_spin_unlock(&data->tx_lock, key);
	return ready;
}
static int virtconsole_irq_tx_complete(const struct device *dev)
{
	return virtconsole_irq_tx_ready(dev);
}
static void virtconsole_irq_rx_enable(const struct device *dev)
{
	struct virtconsole_data *data = dev->data;

	/* Start receiving characters immediately */
	virtconsole_poll_in(dev, NULL);
	atomic_set_bit(&data->flags, RX_IRQ_ENABLED);
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}
static int virtconsole_irq_rx_ready(const struct device *dev)
{
	struct virtconsole_data *data = dev->data;

	/* True if any port has characters ready to read */
	return atomic_get(&(data->rx_ready));
}
static int virtconsole_irq_is_pending(const struct device *dev)
{
	struct virtconsole_data *data = dev->data;

	return (virtconsole_irq_rx_ready(dev) != 0) ||
	       (atomic_test_bit(&data->flags, TX_IRQ_ENABLED) &&
		(virtconsole_irq_tx_ready(dev) > 0));
}
static void virtconsole_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct virtconsole_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = user_data;
}
#endif

static int virtconsole_init(const struct device *dev)
{
	const struct virtconsole_config *config = dev->config;
	struct virtconsole_data *data = dev->data;

	data->dev = dev;
	for (int i = 0; i < ARRAY_SIZE(data->rx_cb_data); i++) {
		data->rx_cb_data[i].data = data;
		data->rx_cb_data[i].port = i;
	}
	size_t n_queues = 2;

	__maybe_unused bool multiport =
		virtio_read_device_feature_bit(config->vdev, VIRTIO_CONSOLE_F_MULTIPORT);

	data->virtio_devcfg = virtio_get_device_specific_config(config->vdev);

	if (data->virtio_devcfg == NULL) {
		LOG_WRN("could not get device-specific config");
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
		LOG_WRN("disabling multiport feature");
		multiport = false;
#endif
	}
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	if (multiport) {
		if (virtio_write_driver_feature_bit(config->vdev, VIRTIO_CONSOLE_F_MULTIPORT, 1)) {
			multiport = false;
			LOG_WRN("could not enable multiport feature");
		}
		if (virtio_commit_feature_bits(config->vdev)) {
			multiport = false;
			LOG_WRN("could not commit feature bits; disabling multiport feature");
		} else {
			n_queues = (sys_le16_to_cpu(data->virtio_devcfg->max_nr_ports) + 1) * 2;
		}
	}
	if (!multiport) {
		/* If the multiport feature is off, use the default */
		data->n_console_ports = 1;
		data->console_ports = 1; /* Enable port 0 */
	}
#endif
	int ret = virtio_init_virtqueues(config->vdev, n_queues, virtconsole_enum_queues_cb, NULL);

	if (ret) {
		LOG_ERR("error initializing virtqueues!");
		return ret;
	}
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	for (uint16_t q_no = VIRTQ_TX; q_no < n_queues; q_no += 2U) {
		if (q_no == VIRTQ_CONTROL_TX) {
			continue;
		}
		struct virtq *vq = virtio_get_virtqueue(config->vdev, q_no);

		vq->irq_cb = virtconsole_tx_irq;
		vq->irq_cb_data = data;
	}
#endif
	virtio_finalize_init(config->vdev);
#ifdef CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT
	if (multiport) {
		k_fifo_init(&data->tx_ctlfifo);
		for (int i = 0; i < CONFIG_UART_VIRTIO_CONSOLE_RX_CONTROL_BUFSIZE; i++) {
			data->ctl_cb_data[i].data = data;
			data->ctl_cb_data[i].buf_no = i;
			data->rx_ctlbuf[i].port = UINT32_MAX;
			virtconsole_recv_setup(data->dev, VIRTQ_CONTROL_RX, &data->rx_ctlbuf[i],
					       sizeof(struct _virtio_console_control),
					       virtconsole_control_recv_cb, &data->ctl_cb_data[i]);
		}
		virtconsole_send_control_msg(dev, 0, VIRTIO_CONSOLE_DEVICE_READY, 1);
	}
#endif
	return 0;
}

static DEVICE_API(uart, virtconsole_api) = {
	.poll_in = virtconsole_poll_in,
	.poll_out = virtconsole_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = virtconsole_fifo_fill,
	.fifo_read = virtconsole_fifo_read,
	.irq_tx_enable = virtconsole_irq_tx_enable,
	.irq_tx_disable = virtconsole_irq_tx_disable,
	.irq_tx_ready = virtconsole_irq_tx_ready,
	.irq_tx_complete = virtconsole_irq_tx_complete,
	.irq_rx_enable = virtconsole_irq_rx_enable,
	.irq_rx_ready = virtconsole_irq_rx_ready,
	.irq_is_pending = virtconsole_irq_is_pending,
	.irq_callback_set = virtconsole_irq_callback_set,
#endif
};

#define VIRTIO_CONSOLE_DEFINE(inst)                                                                \
	static struct virtconsole_data virtconsole_data_##inst = {                                 \
		.tx_done = Z_SEM_INITIALIZER(virtconsole_data_##inst.tx_done, 0, 1),               \
	};                                                                                         \
	static const struct virtconsole_config virtconsole_config_##inst = {                       \
		.vdev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, virtconsole_init, NULL, &virtconsole_data_##inst,              \
			      &virtconsole_config_##inst, POST_KERNEL,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, &virtconsole_api);

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_CONSOLE_DEFINE)
