/*
 * Copyright (c) 2025 Ayush Singh, BeagleBoard.org
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <resource_table.h>
#include <openamp/open_amp.h>
#include <zephyr/drivers/ipm.h>

#define DT_DRV_COMPAT rpmsg_uart

#define RX_BUF_SIZE CONFIG_UART_RPMSG_RX_BUF_SIZE

struct uart_rpmsg_config {
	size_t shm_size;
	const metal_phys_addr_t shm_physmap;
	const struct device *const ipm_handle;
};

struct uart_rpmsg_data {
	struct rpmsg_endpoint tty_ept;
	struct metal_io_region shm_io_data;
	struct fw_resource_table *rsc_table;
	metal_phys_addr_t rsc_tab_physmap;
	struct metal_io_region rsc_io_data;
	struct rpmsg_virtio_device rvdev;

	uint8_t rx_rb_buf[RX_BUF_SIZE];
	struct ring_buf rx_rb;
	struct k_spinlock rx_lock;
};

static void platform_ipm_callback(const struct device *dev, void *context, uint32_t id,
				  volatile void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(context);
	ARG_UNUSED(id);
	ARG_UNUSED(data);
}

static int platform_init(const struct device *dev)
{
	int status, rsc_size;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	struct uart_rpmsg_data *data = dev->data;
	const struct uart_rpmsg_config *cfg = dev->config;

	status = metal_init(&metal_params);
	if (status) {
		return -1;
	}

	/* declare shared memory region */
	metal_io_init(&data->shm_io_data, (void *)cfg->shm_physmap, &cfg->shm_physmap,
		      cfg->shm_size, -1, 0, NULL);

	/* declare resource table region */
	rsc_table_get((void **)&data->rsc_table, &rsc_size);
	data->rsc_tab_physmap = (uintptr_t)data->rsc_table;

	metal_io_init(&data->rsc_io_data, data->rsc_table, &data->rsc_tab_physmap, rsc_size, -1, 0,
		      NULL);

	/* setup IPM */
	if (!device_is_ready(cfg->ipm_handle)) {
		return -ENODEV;
	}

	ipm_register_callback(cfg->ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(cfg->ipm_handle, 1);
	if (status) {
		return -1;
	}

	return 0;
}

static int mailbox_notify(void *priv, uint32_t id)
{
	struct uart_rpmsg_config *cfg = priv;

	ipm_send(cfg->ipm_handle, 0, id, &id, 4);

	return 0;
}

static struct rpmsg_device *platform_create_rpmsg_vdev(const struct device *dev)
{
	int ret;
	struct virtio_device *vdev;
	struct fw_rsc_vdev_vring *vring_rsc;
	struct uart_rpmsg_data *data = dev->data;
	const struct uart_rpmsg_config *cfg = dev->config;

	vdev = rproc_virtio_create_vdev(VIRTIO_DEV_DEVICE, VDEV_ID,
					rsc_table_to_vdev(data->rsc_table), &data->rsc_io_data,
					(void *)cfg, mailbox_notify, NULL);

	if (!vdev) {
		return NULL;
	}

	/* wait master rpmsg init completion */
	rproc_virtio_wait_remote_ready(vdev);

	vring_rsc = rsc_table_get_vring0(data->rsc_table);
	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid, (void *)vring_rsc->da,
				      &data->rsc_io_data, vring_rsc->num, vring_rsc->align);
	if (ret) {
		goto failed;
	}

	vring_rsc = rsc_table_get_vring1(data->rsc_table);
	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid, (void *)vring_rsc->da,
				      &data->rsc_io_data, vring_rsc->num, vring_rsc->align);
	if (ret) {
		goto failed;
	}

	ret = rpmsg_init_vdev(&data->rvdev, vdev, NULL, &data->shm_io_data, NULL);
	if (ret) {
		goto failed;
	}

	return rpmsg_virtio_get_rpmsg_device(&data->rvdev);

failed:
	rproc_virtio_remove_vdev(vdev);

	return NULL;
}

static int rpmsg_recv_tty_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				   void *priv)
{
	k_spinlock_key_t key;
	struct uart_rpmsg_data *priv_data = priv;

	ARG_UNUSED(ept);
	ARG_UNUSED(src);

	key = k_spin_lock(&priv_data->rx_lock);
	ring_buf_put(&priv_data->rx_rb, data, len);
	k_spin_unlock(&priv_data->rx_lock, key);

	return RPMSG_SUCCESS;
}

static int uart_rpmsg_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_rpmsg_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t read;

	key = k_spin_lock(&data->rx_lock);
	read = ring_buf_get(&data->rx_rb, p_char, 1);
	k_spin_unlock(&data->rx_lock, key);

	return (read > 0) ? 0 : 1;
}

static void uart_rpmsg_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_rpmsg_data *data = dev->data;

	rpmsg_send(&data->tty_ept, &c, sizeof(c));
	rproc_virtio_notified(data->rvdev.vdev, VRING1_ID);
}

static DEVICE_API(uart, uart_rpmsg_driver_api) = {
	.poll_in = uart_rpmsg_poll_in,
	.poll_out = uart_rpmsg_poll_out,
};

static int uart_rpmsg_init(const struct device *dev)
{
	int ret;
	struct rpmsg_device *rpdev;
	struct uart_rpmsg_data *data = dev->data;

	/* Initialize platform */
	ret = platform_init(dev);
	if (ret) {
		return ret;
	}

	rpdev = platform_create_rpmsg_vdev(dev);
	if (!rpdev) {
		return ret;
	}

	ring_buf_init(&data->rx_rb, sizeof(data->rx_rb_buf), data->rx_rb_buf);

	data->tty_ept.priv = data;
	return rpmsg_create_ept(&data->tty_ept, rpdev, "rpmsg-tty", RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
				rpmsg_recv_tty_callback, NULL);
}

#define UART_RPMSG_INIT(n)                                                                         \
	static struct uart_rpmsg_data uart_rpmsg_data_##n = {};                                    \
	static const struct uart_rpmsg_config uart_rpmsg_config_##n = {                            \
		.shm_size = DT_REG_SIZE(DT_INST_PHANDLE(n, ipc_shm)),                              \
		.shm_physmap = DT_REG_ADDR(DT_INST_PHANDLE(n, ipc_shm)),                           \
		.ipm_handle = DEVICE_DT_GET(DT_INST_PHANDLE(n, ipc)),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, uart_rpmsg_init, NULL, &uart_rpmsg_data_##n,                      \
			      &uart_rpmsg_config_##n, POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_rpmsg_driver_api)

DT_INST_FOREACH_STATUS_OKAY(UART_RPMSG_INIT)
