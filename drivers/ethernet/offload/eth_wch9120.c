/*
 * Copyright (c) 2026 Madhab Sharma <madhabsharma94@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * WCH CH9120 Ethernet Driver (UART-to-ethernet Offload chip).
 *
 * Based on reference implementation by Shifeng Li
 * https://github.com/libdriver/ch9120
 *
 */

#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(eth_ch9120, LOG_LEVEL_INF);

#define DT_DRV_COMPAT             wch_ch9120
#define CH9120_NODE               DT_INST(0, wch_ch9120)
#define CH9120_UART_PRE_DELAY     30
#define CH9120_BAUD_CONFIG        9600
#define ETH_CH9120_RX_BUF_SIZE    CONFIG_ETH_CH9120_RX_BUF_SIZE
#define CH9120_CONNECT_TIMEOUT_MS 5000

/* header bytes */
#define CH9120_HDR_0 0x57
#define CH9120_HDR_1 0xAB

/* command codes */
#define CH9120_CMD_SET_MODE        0x10
#define CH9120_CMD_SET_TARGET_IP   0x15
#define CH9120_CMD_SET_TARGET_PORT 0x16
#define CH9120_CMD_SET_DHCP        0x33
#define CH9120_CMD_GET_MAC         0x81

/* exit config mode sequence */
#define CH9120_CMD_SAVE  0x0d
#define CH9120_CMD_RESET 0x0e

/* mode values */
#define CH9120_MODE_TCP_CLIENT 0x01
#define CH9120_MODE_UDP_CLIENT 0x03

#define CH9120_CMD_SET_BAUD 0x21

enum ch9120_sock_state {
	CH9120_SOCK_CLOSED,
	CH9120_SOCK_OPEN,
	CH9120_SOCK_CONNECTING,
	CH9120_SOCK_CONNECTED,
	CH9120_SOCK_ERROR,
};

struct ch9120_socket {
	bool in_use;

	int type;
	int proto;
	int family;
	enum ch9120_sock_state state;
	struct net_sockaddr dst;

	struct ring_buf rx_buf;
	uint8_t rx_buf_data[ETH_CH9120_RX_BUF_SIZE];
	struct k_sem rx_sem;
	struct k_sem connect_sem;

	struct k_mutex lock;
};

struct ch9120_runtime {

	struct net_if *iface;
	uint8_t mac_addr[6];

	struct ch9120_socket sock;
	struct k_mutex drv_lock;
	struct gpio_callback tcp_cb_data;
};

struct ch9120_config {
	struct gpio_dt_spec cfg_gpio;
	struct gpio_dt_spec rst_gpio;
	struct gpio_dt_spec tcp_gpio;

	const struct device *uart_dev;
};

static struct ch9120_runtime ch9120_runtime_data;

static const struct ch9120_config ch9120_config_data = {
	.uart_dev = DEVICE_DT_GET(DT_PROP(CH9120_NODE, uart)),
	.cfg_gpio = GPIO_DT_SPEC_INST_GET(0, config_gpios),
	.rst_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
	.tcp_gpio = GPIO_DT_SPEC_INST_GET(0, tcpcs_gpios),
};

static const struct socket_op_vtable ch9120_socket_fd_op_vtable;

static bool ch9120_is_tcp_connected(void)
{
	const struct ch9120_config *cfg = &ch9120_config_data;

	return gpio_pin_get_dt(&cfg->tcp_gpio) == 1;
}

static void ch9120_tcp_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct ch9120_runtime *data = CONTAINER_OF(cb, struct ch9120_runtime, tcp_cb_data);
	struct ch9120_socket *sck = &data->sock;

	if (ch9120_is_tcp_connected()) {
		if (sck->state == CH9120_SOCK_CONNECTING) {
			k_sem_give(&sck->connect_sem);
		}
	} else {
		if (sck->state == CH9120_SOCK_CONNECTED) {
			LOG_ERR("TCP server forcefully disconnected (GPIO Trigger)!");
			sck->state = CH9120_SOCK_OPEN;
			k_sem_give(&sck->rx_sem);
		}
	}
}

static void ch9120_uart_flush(const struct device *uart_dev)
{
	uint8_t c;

	while (uart_poll_in(uart_dev, &c) == 0) {
		/* discard */
	}
}

static void ch9120_uart_flush_rx_fifo(const struct device *uart_dev)
{
	uint8_t c;

	while (uart_fifo_read(uart_dev, &c, 1) > 0) {
		/* discard */
	}
}

static int ch9120_send_cmd_wait(const struct ch9120_config *cfg, uint8_t cmd, const uint8_t *data,
				size_t len, k_timeout_t timeout)
{
	uint8_t ack;
	int ret = -EIO;
	k_timepoint_t deadline;
	uint8_t header[3] = {CH9120_HDR_0, CH9120_HDR_1, cmd};
	struct uart_config current_cfg;
	const struct device *uart_dev = cfg->uart_dev;

	uart_config_get(uart_dev, &current_cfg);

	if (current_cfg.baudrate != CH9120_BAUD_CONFIG) {
		struct uart_config cfg_conf = current_cfg;

		cfg_conf.baudrate = CH9120_BAUD_CONFIG;
		uart_configure(uart_dev, &cfg_conf);
	}

	/* Enter config mode */
	gpio_pin_set_dt(&cfg->cfg_gpio, 1);
	k_msleep(10);

	ch9120_uart_flush(uart_dev);

	for (int i = 0; i < 3; i++) {
		uart_poll_out(uart_dev, header[i]);
	}

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, data[i]);
	}

	k_msleep(CH9120_UART_PRE_DELAY);

	/* Wait for 0xAA Acknowledgment from CH9120 */
	deadline = sys_timepoint_calc(timeout);
	do {
		ret = uart_poll_in(uart_dev, &ack);
		if (ret == 0) {
			if (ack == 0xAA) {
				ret = 0;
				goto cleanup;
			}

			if (ack == 0xEE) {
				LOG_ERR("CH9120 rejected command 0x%02x", cmd);
				errno = EPROTO;
				ret = -1;
				goto cleanup;
			}
		}
		k_msleep(10);
	} while (!sys_timepoint_expired(deadline));

	LOG_ERR("Timeout waiting for ACK on cmd: 0x%02x", cmd);
cleanup:
	gpio_pin_set_dt(&cfg->cfg_gpio, 0);

	if (current_cfg.baudrate != CH9120_BAUD_CONFIG) {
		uart_configure(uart_dev, &current_cfg);
	}

	return ret;
}

static int ch9120_send_cmd_read(const struct ch9120_config *cfg, uint8_t cmd, const uint8_t *data,
				size_t len, uint8_t *read_buf, size_t read_len, k_timeout_t timeout)
{
	size_t indx;
	k_timepoint_t deadline;
	uint8_t header[3] = {CH9120_HDR_0, CH9120_HDR_1, cmd};
	struct uart_config current_cfg;
	const struct device *uart_dev = cfg->uart_dev;

	uart_config_get(uart_dev, &current_cfg);

	if (current_cfg.baudrate != CH9120_BAUD_CONFIG) {
		struct uart_config cfg_conf = current_cfg;

		cfg_conf.baudrate = CH9120_BAUD_CONFIG;
		uart_configure(uart_dev, &cfg_conf);
	}

	/* Enter config mode */
	gpio_pin_set_dt(&cfg->cfg_gpio, 1);
	k_msleep(10);

	ch9120_uart_flush(uart_dev);

	for (int i = 0; i < 3; i++) {
		uart_poll_out(uart_dev, header[i]);
	}

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, data[i]);
	}

	k_msleep(CH9120_UART_PRE_DELAY);

	indx = 0;
	deadline = sys_timepoint_calc(timeout);
	do {
		while (uart_poll_in(uart_dev, &read_buf[indx]) == 0) {
			indx++;
			if (indx >= read_len) {
				goto cleanup;
			}
		}
		k_msleep(10);
	} while (!sys_timepoint_expired(deadline));

	LOG_ERR("Timeout waiting for ACK on cmd: 0x%02x", cmd);
	errno = EIO;
cleanup:
	gpio_pin_set_dt(&cfg->cfg_gpio, 0);

	if (current_cfg.baudrate != CH9120_BAUD_CONFIG) {
		uart_configure(uart_dev, &current_cfg);
	}

	return (indx >= read_len) ? 0 : -1;
}

static int ch9120_wait_for_connection(struct ch9120_socket *sck, k_timeout_t timeout)
{
	int ret = k_sem_take(&sck->connect_sem, timeout);

	if (ret < 0) {
		errno = ETIMEDOUT;
		return -1;
	}

	return 0;
}

static int ch9120_configure_interrupt(void)
{
	int ret;
	struct ch9120_runtime *data = &ch9120_runtime_data;
	const struct ch9120_config *cfg = &ch9120_config_data;

	ret = gpio_pin_interrupt_configure_dt(&cfg->tcp_gpio, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Failed to configure TCP GPIO interrupt: %d", ret);
		return -1;
	}

	gpio_init_callback(&data->tcp_cb_data, ch9120_tcp_cb, BIT(cfg->tcp_gpio.pin));

	ret = gpio_add_callback(cfg->tcp_gpio.port, &data->tcp_cb_data);
	if (ret < 0) {
		LOG_ERR("Failed to add TCP GPIO callback: %d", ret);
		return -1;
	}

	return 0;
}

static void ch9120_uart_cb(const struct device *uart_dev, void *user_data)
{
	int rx;
	int ret;
	uint32_t claimed_len = 0;
	uint32_t total_size = 0;
	uint8_t *buf;
	struct ch9120_runtime *data = (struct ch9120_runtime *)user_data;
	struct ch9120_socket *sck = &data->sock;

	/* Check if TCP is dropped */
	if (sck->type == NET_SOCK_STREAM && !ch9120_is_tcp_connected() &&
	    sck->state == CH9120_SOCK_CONNECTED) {
		LOG_WRN("TCP connection lost");
		sck->state = CH9120_SOCK_OPEN;
		k_sem_give(&sck->rx_sem);
	}

	uart_irq_update(uart_dev);
	if (uart_irq_rx_ready(uart_dev) > 0) {

		while (true) {

			if (!claimed_len) {
				if (total_size > 0) {
					ret = ring_buf_put_finish(&sck->rx_buf, total_size);
					__ASSERT_NO_MSG(ret == 0);
					total_size = 0;
				}

				claimed_len = ring_buf_put_claim(&sck->rx_buf, &buf, UINT32_MAX);
			}

			if (!claimed_len) {
				LOG_ERR("Rx buffer doesn't have enough space");
				ch9120_uart_flush_rx_fifo(uart_dev);
				break;
			}

			rx = uart_fifo_read(uart_dev, buf, claimed_len);
			if (rx <= 0) {
				break;
			}

			buf += rx;
			total_size += rx;
			claimed_len -= rx;
		}
	}

	if (total_size > 0) {
		ret = ring_buf_put_finish(&sck->rx_buf, total_size);
		__ASSERT_NO_MSG(ret == 0);
		k_sem_give(&sck->rx_sem);
	} else {
		if (claimed_len > 0) {
			ring_buf_put_finish(&sck->rx_buf, 0);
		}
	}
}

static int ch9120_close(void *obj)
{
	struct ch9120_socket *sck = (struct ch9120_socket *)obj;
	const struct ch9120_config *cfg = &ch9120_config_data;

	if (sck == NULL) {
		LOG_ERR("%s: invalid socket received", __func__);
		return -1;
	}

	k_mutex_lock(&ch9120_runtime_data.drv_lock, K_FOREVER);
	if (!sck->in_use) {
		k_mutex_unlock(&ch9120_runtime_data.drv_lock);
		return -1;
	}

	sck->in_use = false;
	sck->state = CH9120_SOCK_CLOSED;

	ring_buf_reset(&sck->rx_buf);
	k_sem_reset(&sck->rx_sem);

	k_mutex_unlock(&ch9120_runtime_data.drv_lock);

	if (sck->type == NET_SOCK_STREAM) {
		gpio_remove_callback(cfg->tcp_gpio.port, &ch9120_runtime_data.tcp_cb_data);
		gpio_pin_interrupt_configure_dt(&cfg->tcp_gpio, GPIO_INT_DISABLE);
	}

	uart_irq_rx_disable(cfg->uart_dev);
	LOG_DBG("socket closed");
	return 0;
}

static int ch9120_ioctl(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
	case ZFD_IOCTL_POLL_UPDATE:
		errno = EXDEV;
		return -1;
	default:
		errno = EINVAL;
	}

	return -1;
}

static int ch9120_connect(void *obj, const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	int ret = 0;
	uint8_t mode = CH9120_MODE_UDP_CLIENT;
	uint16_t dst_port = 0U;
	uint8_t dst_ip[4];
	uint8_t port_bytes[2];
	struct ch9120_socket *sck = (struct ch9120_socket *)obj;
	const struct ch9120_config *cfg = &ch9120_config_data;

	if (sck == NULL) {
		LOG_ERR("%s: invalid socket received", __func__);
		errno = EINVAL;
		return -1;
	}

	if (addr == NULL || addrlen < sizeof(struct sockaddr_in)) {
		errno = EINVAL;
		return -1;
	}

	if (addr->sa_family != NET_AF_INET) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	k_mutex_lock(&sck->lock, K_FOREVER);
	if (sck->state != CH9120_SOCK_OPEN) {
		k_mutex_unlock(&sck->lock);
		errno = EISCONN;
		return -1;
	}

	sck->state = CH9120_SOCK_CONNECTING;
	k_mutex_unlock(&sck->lock);

	if (sck->type == NET_SOCK_STREAM) {
		if (ch9120_configure_interrupt() < 0) {
			goto err;
		}
		mode = CH9120_MODE_TCP_CLIENT;
	}

	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_SET_MODE, &mode, 1, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to set mode : %d", ret);
		goto err;
	}

	dst_port = net_ntohs(net_sin(addr)->sin_port);
	sys_put_le16(dst_port, port_bytes);

	memcpy(dst_ip, &net_sin(addr)->sin_addr.s_addr, 4);

	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_SET_TARGET_IP, dst_ip, 4, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to send destination IP :%d", ret);
		goto err;
	}

	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_SET_TARGET_PORT, port_bytes, 2, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to set dst port: %d", ret);
		goto err;
	}

	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_SAVE, NULL, 0, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to save config :%d", ret);
		goto err;
	}

	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_RESET, NULL, 0, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to save config :%d", ret);
		goto err;
	}
	/* wait for the chip to boot up */
	k_msleep(1000);
	if (sck->type == NET_SOCK_STREAM) {
		ret = ch9120_wait_for_connection(sck, K_MSEC(CH9120_CONNECT_TIMEOUT_MS));
		if (ret < 0) {
			LOG_ERR("TCP connection timed out!");
			goto err;
		}
	}

	k_mutex_lock(&sck->lock, K_FOREVER);
	sck->state = CH9120_SOCK_CONNECTED;
	k_mutex_unlock(&sck->lock);

	memcpy(&sck->dst, addr, addrlen);

	uart_irq_rx_enable(cfg->uart_dev);

	return 0;

err:
	k_mutex_lock(&sck->lock, K_FOREVER);
	sck->in_use = false;
	sck->state = CH9120_SOCK_OPEN;
	k_mutex_unlock(&sck->lock);

	return ret;
}

static ssize_t ch9120_sendto(void *obj, const void *buf, size_t len, int flags,
			     const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	struct ch9120_socket *sck = (struct ch9120_socket *)obj;
	const struct ch9120_config *cfg = &ch9120_config_data;
	const uint8_t *data = buf;

	ARG_UNUSED(flags);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	k_mutex_lock(&sck->lock, K_FOREVER);

	if (sck->state != CH9120_SOCK_CONNECTED) {
		k_mutex_unlock(&sck->lock);
		errno = ENOTCONN;
		return -1;
	}

	if (len > ETH_CH9120_RX_BUF_SIZE) {
		len = ETH_CH9120_RX_BUF_SIZE;
	}

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(cfg->uart_dev, data[i]);
	}

	k_mutex_unlock(&sck->lock);

	return len;
}

static ssize_t ch9120_recvfrom(void *obj, void *buf, size_t len, int flags,
			       struct net_sockaddr *addr, net_socklen_t *addrlen)
{
	struct ch9120_socket *sck = (struct ch9120_socket *)obj;
	uint32_t read;
	int ret;

	ARG_UNUSED(flags);

	if (addr != NULL && addrlen != NULL) {
		*addrlen = sizeof(sck->dst);
		memcpy(addr, &sck->dst, *addrlen);
	}

	ret = k_sem_take(&sck->rx_sem, K_FOREVER);
	if (ret < 0) {
		errno = EINTR;
		return -1;
	}

	if (sck->state != CH9120_SOCK_CONNECTED) {
		if (!ring_buf_is_empty(&sck->rx_buf)) {
			read = ring_buf_get(&sck->rx_buf, buf, len);
			return (ssize_t)read;
		}
		errno = ENOTCONN;
		return -1;
	}

	read = ring_buf_get(&sck->rx_buf, buf, len);
	if (read == 0) {
		errno = EAGAIN;
		return -1;
	}

	return (ssize_t)read;
}

static ssize_t ch9120_read(void *obj, void *buf, size_t sz)
{
	return ch9120_recvfrom(obj, buf, sz, 0, NULL, NULL);
}

static ssize_t ch9120_write(void *obj, const void *buf, size_t sz)
{
	return ch9120_sendto(obj, buf, sz, 0, NULL, 0);
}

static int socket_family_is_supported(int family)
{
	switch (family) {
	case NET_AF_INET:
		break;
	default:
		return -1;
	}

	return 0;
}

static int socket_type_is_supported(int type)
{
	switch (type) {
	case NET_SOCK_STREAM:
	case NET_SOCK_DGRAM:
		break;
	default:
		return -1;
	}

	return 0;
}

static int socket_proto_is_supported(int proto)
{
	switch (proto) {
	case NET_IPPROTO_TCP:
	case NET_IPPROTO_UDP:
		break;
	default:
		return -1;
	}

	return 0;
}

static bool ch9120_socket_is_supported(int family, int type, int proto)
{
	int ret;

	ret = socket_family_is_supported(family);
	if (ret < 0) {
		return false;
	}

	ret = socket_type_is_supported(type);
	if (ret < 0) {
		return false;
	}

	ret = socket_proto_is_supported(proto);
	if (ret < 0) {
		return false;
	}

	return true;
}

int ch9120_socket_create(int family, int type, int proto)
{
	int fd;
	struct ch9120_socket *sck = &ch9120_runtime_data.sock;

	k_mutex_lock(&ch9120_runtime_data.drv_lock, K_FOREVER);

	/* CH9120 only supports one mode of connection at a time */
	if (sck->in_use) {
		k_mutex_unlock(&ch9120_runtime_data.drv_lock);
		LOG_ERR("Failed to create socket, already in use");
		errno = EMFILE;
		return -1;
	}

	sck->family = family;
	sck->type = type;
	sck->proto = proto;
	sck->state = CH9120_SOCK_OPEN;

	sck->in_use = true;

	k_mutex_unlock(&ch9120_runtime_data.drv_lock);

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		k_mutex_lock(&ch9120_runtime_data.drv_lock, K_FOREVER);
		sck->in_use = false;
		k_mutex_unlock(&ch9120_runtime_data.drv_lock);
		return -1;
	}

	zvfs_finalize_typed_fd(fd, sck,
			       (const struct fd_op_vtable *)&ch9120_socket_fd_op_vtable.fd_vtable,
			       ZVFS_MODE_IFSOCK);

	return fd;
}

static int ch9120_init(const struct device *dev)
{
	int ret;
	uint8_t enable_flag;
	uint32_t target_baud = DT_PROP(DT_PHANDLE(CH9120_NODE, uart), current_speed);
	uint8_t baud_bytes[4];
	struct ch9120_runtime *data = dev->data;
	const struct ch9120_config *cfg = dev->config;

	struct uart_config uart_cfg = {
		.baudrate = CH9120_BAUD_CONFIG,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	/* Initialise UART*/
	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("UART device not ready");
		errno = ENODEV;
		return -1;
	}

	ret = uart_configure(cfg->uart_dev, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure UART : %d", ret);
		return ret;
	}

	/* Initialise gpios */
	if (!gpio_is_ready_dt(&cfg->rst_gpio) || !gpio_is_ready_dt(&cfg->cfg_gpio) ||
	    !gpio_is_ready_dt(&cfg->tcp_gpio)) {

		LOG_ERR("GPIOs not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->cfg_gpio, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&cfg->tcp_gpio, GPIO_INPUT);
	gpio_pin_configure_dt(&cfg->rst_gpio, GPIO_OUTPUT_ACTIVE);

	k_msleep(10);
	/* Initialize the Chip */
	gpio_pin_set_dt(&cfg->rst_gpio, 0);
	k_msleep(500);

	enable_flag = 0x01;
	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_SET_DHCP, &enable_flag, 1, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to set dhcp:%d", ret);
		return -1;
	}
	k_msleep(500);

	sys_put_le32(target_baud, baud_bytes);
	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_SET_BAUD, baud_bytes, 4, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to set baud rate: %d", ret);
		return ret;
	}

	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_SAVE, NULL, 0, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to save config :%d", ret);
		return -1;
	}
	k_msleep(500);

	ret = ch9120_send_cmd_wait(cfg, CH9120_CMD_RESET, NULL, 0, K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to reset the chip :%d", ret);
		return -1;
	}
	k_msleep(1000);

	uart_cfg.baudrate = target_baud;
	ret = uart_configure(cfg->uart_dev, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure Zephyr UART to %d", target_baud);
		return ret;
	}
	LOG_INF("Data UART speed successfully increased to %d bps!", target_baud);

	ret = ch9120_send_cmd_read(cfg, CH9120_CMD_GET_MAC, NULL, 0, data->mac_addr,
				   sizeof(data->mac_addr), K_MSEC(1000));
	if (ret < 0) {
		LOG_ERR("Failed to read mac :%d", ret);
		return -1;
	}

	ret = uart_irq_callback_user_data_set(cfg->uart_dev, ch9120_uart_cb, (void *)dev->data);
	if (ret < 0) {
		LOG_ERR("Couldn't set UART callback");
		return ret;
	}

	/* Initialise driver state */
	data->sock.in_use = false;
	k_mutex_init(&data->drv_lock);

	k_mutex_init(&(data->sock.lock));
	k_sem_init(&(data->sock.rx_sem), 0, K_SEM_MAX_LIMIT);
	k_sem_init(&(data->sock.connect_sem), 0, 1);
	ring_buf_init(&(data->sock.rx_buf), sizeof(data->sock.rx_buf_data), data->sock.rx_buf_data);

	return 0;
}

static void ch9120_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct ch9120_runtime *data = dev->data;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	data->iface = iface;
	net_if_socket_offload_set(iface, ch9120_socket_create);
}

static struct offloaded_if_api ch9120_if_apis = {
	.iface_api.init = ch9120_iface_init,
};

/*
 * NOTE: Non-blocking I/O (O_NONBLOCK / SO_RCVTIMEO) is not yet
 * supported.Adding non-blocking support is tracked as future work.
 */
static const struct socket_op_vtable ch9120_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = ch9120_read,
		.write = ch9120_write,
		.close = ch9120_close,
		.ioctl = ch9120_ioctl,
	},
	.bind = NULL,
	.connect = ch9120_connect,
	.listen = NULL,
	.accept = NULL,
	.sendto = ch9120_sendto,
	.sendmsg = NULL,
	.recvfrom = ch9120_recvfrom,
	.recvmsg = NULL,
	.getsockopt = NULL,
	.setsockopt = NULL,
};

NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, ch9120_init, NULL, &ch9120_runtime_data, &ch9120_config_data,
				  CONFIG_ETH_INIT_PRIORITY, &ch9120_if_apis, NET_ETH_MTU);

NET_SOCKET_OFFLOAD_REGISTER(ch9120, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_INET,
			    ch9120_socket_is_supported, ch9120_socket_create);
