/*
 * Copyright (c) 2020, STMICROELECTRONICS
 * Copyright (c) 2025 Ayush Singh BeagleBoard.org.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <resource_table.h>
#include <zephyr/kernel.h>
#include <openamp/open_amp.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/drivers/console/console.h>

/* Constants derived from device tree */
#define SHM_NODE       DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE       DT_REG_SIZE(SHM_NODE)

struct rpmsg_buf {
	uint32_t pos;
	uint32_t buf_len;
	char *buf;
};

struct config {
	struct rpmsg_buf tx_buf;
	metal_phys_addr_t shm_physmap;
	struct rpmsg_endpoint tty_ept;
	struct rpmsg_virtio_device rvdev;
	struct metal_io_region shm_io_data;
	struct metal_io_region rsc_io_data;
	struct fw_resource_table *rsc_table;
	const struct device *const ipm_handle;
};

static struct config rpmsg_console_config = {
	.ipm_handle = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc)),
	.shm_physmap = SHM_START_ADDR,
};

static void platform_ipm_callback(const struct device *dev, void *context, uint32_t id,
				  volatile void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(context);
	ARG_UNUSED(id);
	ARG_UNUSED(data);
}

int platform_init(struct config *cfg)
{
	/* A pointer to rsc_tab_physmap is stored in metal_io_init. So probably needs to be
	 * static.
	 */
	static metal_phys_addr_t rsc_tab_physmap;

	int status;
	int rsc_size;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	status = metal_init(&metal_params);
	if (status) {
		return -1;
	}

	/* declare shared memory region */
	metal_io_init(&cfg->shm_io_data, (void *)SHM_START_ADDR, &cfg->shm_physmap, SHM_SIZE, -1, 0,
		      NULL);

	/* declare resource table region */
	rsc_table_get((void **)&cfg->rsc_table, &rsc_size);
	rsc_tab_physmap = (uintptr_t)cfg->rsc_table;

	metal_io_init(&cfg->rsc_io_data, cfg->rsc_table, &rsc_tab_physmap, rsc_size, -1, 0, NULL);

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

int mailbox_notify(void *priv, uint32_t id)
{
	ARG_UNUSED(priv);
	struct config *cfg = priv;

	ipm_send(cfg->ipm_handle, 0, id, &id, 4);

	return 0;
}

struct rpmsg_device *platform_create_rpmsg_vdev(struct config *cfg)
{
	struct fw_rsc_vdev_vring *vring_rsc;
	struct virtio_device *vdev;
	int ret;

	vdev = rproc_virtio_create_vdev(VIRTIO_DEV_DEVICE, VDEV_ID,
					rsc_table_to_vdev(cfg->rsc_table), &cfg->rsc_io_data, cfg,
					mailbox_notify, NULL);

	if (!vdev) {
		return NULL;
	}

	/* wait master rpmsg init completion */
	rproc_virtio_wait_remote_ready(vdev);

	vring_rsc = rsc_table_get_vring0(cfg->rsc_table);
	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid, (void *)vring_rsc->da,
				      &cfg->rsc_io_data, vring_rsc->num, vring_rsc->align);
	if (ret) {
		goto failed;
	}

	vring_rsc = rsc_table_get_vring1(cfg->rsc_table);
	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid, (void *)vring_rsc->da,
				      &cfg->rsc_io_data, vring_rsc->num, vring_rsc->align);
	if (ret) {
		goto failed;
	}

	ret = rpmsg_init_vdev(&cfg->rvdev, vdev, NULL, &cfg->shm_io_data, NULL);
	if (ret) {
		goto failed;
	}

	return rpmsg_virtio_get_rpmsg_device(&cfg->rvdev);

failed:
	rproc_virtio_remove_vdev(vdev);

	return NULL;
}

static int rpmsg_recv_tty_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				   void *priv)
{
	ARG_UNUSED(ept);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(src);
	ARG_UNUSED(priv);

	return RPMSG_SUCCESS;
}

int zephyr_rpmsg_init(struct config *cfg)
{
	struct rpmsg_device *rpdev;
	int ret;

	/* Initialize platform */
	ret = platform_init(cfg);
	if (ret) {
		return ret;
	}

	rpdev = platform_create_rpmsg_vdev(cfg);
	if (!rpdev) {
		return ret;
	}

	return rpmsg_create_ept(&cfg->tty_ept, rpdev, "rpmsg-tty", RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
				rpmsg_recv_tty_callback, NULL);
}

static int rpmsg_console_tx(struct config *cfg, size_t len, const char buf[])
{
	size_t offset = 0;
	int ret;

	while (offset < len) {
		ret = rpmsg_send(&rpmsg_console_config.tty_ept, buf + offset, len - offset);
		/* rpmsg_send fails if no console is connected to it initially. If we do not call
		 * rproc_virtio_notified, the internal state never updates, and thus rpmsg_send will
		 * fail even if a serial read is done on the Linux side.
		 */
		rproc_virtio_notified(cfg->rvdev.vdev, VRING1_ID);
		if (ret < 0) {
			return ret;
		}

		offset += ret;
	}

	return 0;
}

static void rpmsg_tx_buf_init(struct config *cfg)
{
	cfg->tx_buf.buf = rpmsg_get_tx_payload_buffer(&cfg->tty_ept, &cfg->tx_buf.buf_len, true);
	cfg->tx_buf.pos = 0;
}

static void rpmsg_tx_buf_add(struct rpmsg_buf *buf, char c)
{
	buf->buf[buf->pos++] = c;
}

static bool rpmsg_tx_buf_append(struct rpmsg_buf *buf, size_t len, const char src[])
{
	if (buf->buf_len - buf->pos >= len) {
		memcpy(buf->buf + buf->pos, src, len);
		buf->pos += len;

		return true;
	}

	return false;
}

static void rpmsg_tx_buf_send(struct config *cfg)
{
	int ret;
	struct rpmsg_buf *buf = &cfg->tx_buf;

	ret = rpmsg_send_nocopy(&cfg->tty_ept, buf->buf, buf->pos);
	rproc_virtio_notified(cfg->rvdev.vdev, VRING1_ID);

	/* If send fails, the buffer is not released. We just drop the contents in such a case. */
	if (ret < 0) {
		buf->pos = 0;
	} else {
		buf->buf = NULL;
		buf->buf_len = 0;
		rpmsg_tx_buf_init(cfg);
	}
}

static bool rpmsg_tx_buf_full(struct rpmsg_buf *buf)
{
	return buf->buf_len <= buf->pos;
}

/*
 * This function performs internal buffering.
 *
 * The data is actually sent over rpmsg in the following cases:
 * 1. A newline character is encounterd.
 * 2. The buffer is full
 */
static int console_out(int c)
{
	static const char CRLF[] = "\r\n";
	bool ret = false;

	if (c == '\n') {
		ret = rpmsg_tx_buf_append(&rpmsg_console_config.tx_buf, ARRAY_SIZE(CRLF), CRLF);
		rpmsg_tx_buf_send(&rpmsg_console_config);

		/* Check if CRLF was sent with the prior buffer */
		if (!ret) {
			rpmsg_console_tx(&rpmsg_console_config, ARRAY_SIZE(CRLF), CRLF);
		}

		return c;
	}

	if (rpmsg_tx_buf_full(&rpmsg_console_config.tx_buf)) {
		rpmsg_tx_buf_send(&rpmsg_console_config);
	}

	rpmsg_tx_buf_add(&rpmsg_console_config.tx_buf, c);

	return c;
}

/**
 * @brief Install printk/stdout hook for UART console output
 */
static void rpmsg_console_hook_install(void)
{
#if defined(CONFIG_STDOUT_CONSOLE)
	__stdout_hook_install(console_out);
#endif
#if defined(CONFIG_PRINTK)
	__printk_hook_install(console_out);
#endif
}

/**
 * @brief Initialize one RPMSG as the console/debug port
 *
 * @return 0 if successful, otherwise failed.
 */
static int rpmsg_console_init(void)
{
	int ret;

	ret = zephyr_rpmsg_init(&rpmsg_console_config);
	if (ret < 0) {
		return ret;
	}

	rpmsg_tx_buf_init(&rpmsg_console_config);
	rpmsg_console_hook_install();

	return 0;
}

/* Need to be initialized after IPM */
SYS_INIT(rpmsg_console_init, POST_KERNEL, CONFIG_CONSOLE_INIT_PRIORITY);
