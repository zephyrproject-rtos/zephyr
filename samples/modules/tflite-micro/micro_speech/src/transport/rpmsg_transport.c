/*
 * Copyright (c) 2025, NXP
 * Author: Thong Phan <quang.thong2001@gmail.com>
 *
 * Heavily based on pwm_mcux_ftm.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rpmsg_transport.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rpmsg_transport);
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>

#include <metal/sys.h>
#include <metal/io.h>
#include <resource_table.h>
#include <addr_translation.h>

#if !DT_HAS_CHOSEN(zephyr_ipc_shm)
#error "Sample requires definition of shared memory for rpmsg"
#endif

/* Constants from device tree */
#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR      DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE            DT_REG_SIZE(SHM_NODE)
/* Stack size for the management thread */
#define APP_TASK_STACK_SIZE (1024)

#if CONFIG_IPM_MAX_DATA_SIZE > 0
#define IPM_SEND(dev, w, id, d, s) ipm_send(dev, w, id, d, s)
#else
#define IPM_SEND(dev, w, id, d, s) ipm_send(dev, w, id, NULL, 0)
#endif
/* Thread definitions */
K_THREAD_STACK_DEFINE(thread_mng_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_mng_data;
/* OpenAMP components */
static const struct device *const ipm_handle = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));
static metal_phys_addr_t shm_physmap = SHM_START_ADDR;
static metal_phys_addr_t rsc_tab_physmap;
static struct metal_io_region shm_io_data;
static struct metal_io_region rsc_io_data;
static struct metal_io_region *shm_io = &shm_io_data;
static struct metal_io_region *rsc_io = &rsc_io_data;
static struct rpmsg_virtio_device rvdev;
static void *rsc_table;
/* Public variables */
struct rpmsg_device *rpdev;
struct rpmsg_endpoint tty_ept;
K_MSGQ_DEFINE(tty_msgq, sizeof(struct rpmsg_rcv_msg), 16, 4);
/* 16 messages * 20ms per frames covers 320ms of data */
K_SEM_DEFINE(data_tty_ready_sem, 0, 1);
/* Internal semaphore, hidden from other files */
static K_SEM_DEFINE(data_sem, 0, 1);

static void platform_ipm_callback(const struct device *dev, void *context, uint32_t id,
				  volatile void *data)

{
	LOG_DBG("%s: msg received from mb %d", __func__, id);
	k_sem_give(&data_sem);
}

int rpmsg_recv_tty_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
			    void *priv)
{
	struct rpmsg_rcv_msg msg = {.data = data, .len = len};

	rpmsg_hold_rx_buffer(ept, data);

	if (k_msgq_put(&tty_msgq, &msg, K_NO_WAIT) != 0) {
		LOG_WRN("tty_msgq full, dropping frame len %zu\n", len);
		rpmsg_release_rx_buffer(ept, data);
	}
	return RPMSG_SUCCESS;
}

static void receive_message(unsigned char **msg, unsigned int *len)
{
	if (k_sem_take(&data_sem, K_FOREVER) == 0) {
		rproc_virtio_notified(rvdev.vdev, VRING1_ID);
	}
}

static void new_service_cb(struct rpmsg_device *rdev, const char *name, uint32_t src)
{
	LOG_ERR("%s: unexpected ns service receive for name %s", __func__, name);
}

static int mailbox_notify(void *priv, uint32_t id)
{
	ARG_UNUSED(priv);
	LOG_DBG("%s: msg received", __func__);
	IPM_SEND(ipm_handle, 0, id, &id, 4);
	return 0;
}

static int platform_init(void)
{
	int rsc_size;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	int status;

	status = metal_init(&metal_params);
	if (status) {
		LOG_ERR("metal_init: failed: %d", status);
		return -1;
	}

	metal_io_init(shm_io, (void *)SHM_START_ADDR, &shm_physmap, SHM_SIZE, -1, 0,
		      addr_translation_get_ops(shm_physmap));

	rsc_table_get(&rsc_table, &rsc_size);
	rsc_tab_physmap = (uintptr_t)rsc_table;
	metal_io_init(rsc_io, rsc_table, &rsc_tab_physmap, rsc_size, -1, 0, NULL);

	if (!device_is_ready(ipm_handle)) {
		LOG_ERR("IPM device is not ready");
		return -1;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_handle, 1);
	if (status) {
		LOG_ERR("ipm_set_enabled failed");
		return -1;
	}

	return 0;
}

static void cleanup_system(void)
{
	ipm_set_enabled(ipm_handle, 0);
	rpmsg_deinit_vdev(&rvdev);
	metal_finish();
}

static struct rpmsg_device *platform_create_rpmsg_vdev(rpmsg_ns_bind_cb ns_cb)
{
	struct fw_rsc_vdev_vring *vring_rsc;
	struct virtio_device *vdev;
	int ret;

	vdev = rproc_virtio_create_vdev(VIRTIO_DEV_DEVICE, VDEV_ID, rsc_table_to_vdev(rsc_table),
					rsc_io, NULL, mailbox_notify, NULL);
	if (!vdev) {
		LOG_ERR("failed to create vdev");
		return NULL;
	}

	rproc_virtio_wait_remote_ready(vdev);

	vring_rsc = rsc_table_get_vring0(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid, (void *)vring_rsc->da, rsc_io,
				      vring_rsc->num, vring_rsc->align);
	if (ret) {
		LOG_ERR("failed to init vring 0");
		goto failed;
	}

	vring_rsc = rsc_table_get_vring1(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid, (void *)vring_rsc->da, rsc_io,
				      vring_rsc->num, vring_rsc->align);
	if (ret) {
		LOG_ERR("failed to init vring 1");
		goto failed;
	}

	ret = rpmsg_init_vdev(&rvdev, vdev, ns_cb, shm_io, NULL);
	if (ret) {
		LOG_ERR("failed rpmsg_init_vdev");
		goto failed;
	}

	return rpmsg_virtio_get_rpmsg_device(&rvdev);

failed:
	rproc_virtio_remove_vdev(vdev);
	return NULL;
}

static void rpmsg_mng_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	unsigned char *msg;
	unsigned int len;
	int ret = 0;

	LOG_INF("Microspeech sample with OpenAMP started");

	ret = platform_init();
	if (ret) {
		LOG_ERR("Failed to initialize platform");
		goto task_end;
	}

	rpdev = platform_create_rpmsg_vdev(new_service_cb);
	if (!rpdev) {
		LOG_ERR("Failed to create rpmsg virtio device");
		goto task_end;
	}

	k_sem_give(&data_tty_ready_sem);

	while (1) {
		receive_message(&msg, &len);
	}

task_end:
	cleanup_system();
	LOG_INF("Microspeech sample with OpenAMP ended");
}

/* Public Function */
void rpmsg_transport_start(void)
{
	k_thread_create(&thread_mng_data, thread_mng_stack, APP_TASK_STACK_SIZE, rpmsg_mng_task,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
}
