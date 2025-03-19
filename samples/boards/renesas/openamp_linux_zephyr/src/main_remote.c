/*
 * Copyright (c) 2024 EPAM Systems
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/mbox.h>

#include <openamp/open_amp.h>
#include <metal/device.h>
#include "resource_table.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(openamp_rsc_table);

#define SHM_DEVICE_NAME "shm"

#if !DT_HAS_CHOSEN(zephyr_ipc_shm)
#error "Sample requires definition of shared memory for rpmsg"
#endif

#define APP_EPT_ADDR 1024

#define SHUTDOWN_MSG (0xEF56A55A)

/* Constants derived from device tree */
#define SHM_NODE       DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE       DT_REG_SIZE(SHM_NODE)

#define RSC_TABLE_ADDR DT_REG_ADDR(DT_NODELABEL(rsctbl))

#define APP_TASK_STACK_SIZE (1024)

K_THREAD_STACK_DEFINE(thread_mng_stack, APP_TASK_STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_rp__client_stack, APP_TASK_STACK_SIZE);

static struct k_thread thread_mng_data;
static struct k_thread thread_rp__client_data;

const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_CHOSEN(zephyr_ipc), tx);
const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_CHOSEN(zephyr_ipc), rx);

static metal_phys_addr_t shm_physmap = CM33_TO_A55_ADDR_NS(SHM_START_ADDR);
static metal_phys_addr_t rsc_physmap = RSC_TABLE_ADDR;

struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.num_regions = 2,
	{
		{.virt = NULL}, /* shared memory */
		{.virt = NULL}, /* rsc_table memory */
	},
	.node = {NULL},
	.irq_num = 0,

};

struct rpmsg_rcv_msg {
	void *data;
	size_t len;
};

static struct metal_io_region *shm_io;
static struct rpmsg_virtio_shm_pool shpool;

static struct metal_io_region *rsc_io;
static struct rpmsg_virtio_device rvdev;

static void *rsc_table;
static struct rpmsg_device *rpdev;

static char rx_sc_msg[512];
static struct rpmsg_endpoint sc_ept;
static struct rpmsg_rcv_msg sc_msg = {.data = rx_sc_msg};

static K_SEM_DEFINE(data_sem, 0, 1);
static K_SEM_DEFINE(data_sc_sem, 0, 1);

static volatile int finish;

static void platform_mbox_callback(const struct device *dev, mbox_channel_id_t channel_id,
				   void *user_data, struct mbox_msg *data)
{
	k_sem_give(&data_sem);
}

static int rpmsg_recv_cs_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				  void *priv)
{
	memcpy(sc_msg.data, data, len);
	sc_msg.len = len;
	if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
		finish = 1;
	}
	k_sem_give(&data_sc_sem);

	return RPMSG_SUCCESS;
}

static void receive_message(unsigned char **msg, unsigned int *len)
{
	int status = k_sem_take(&data_sem, K_FOREVER);

	if (status == 0) {
		rproc_virtio_notified(rvdev.vdev, VRING1_ID);
	}
}

static void new_service_cb(struct rpmsg_device *rdev, const char *name, uint32_t src)
{
	LOG_INF("%s: message received from service %s", __func__, name);
}

int mailbox_notify(void *priv, uint32_t id)
{
	ARG_UNUSED(priv);

	mbox_send_dt(&tx_channel, NULL);
	return 0;
}

int platform_init(void)
{
	void *rsc_tab_addr;
	int rsc_size;
	struct metal_device *device;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	int status;

	status = metal_init(&metal_params);
	if (status) {
		LOG_ERR("metal_init: failed: %d", status);
		return -1;
	}

	status = metal_register_generic_device(&shm_device);
	if (status) {
		LOG_ERR("Couldn't register shared memory: %d", status);
		return -1;
	}

	status = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (status) {
		LOG_ERR("metal_device_open failed: %d", status);
		return -1;
	}

	/* declare shared memory region */
	metal_io_init(&device->regions[0], (void *)SHM_START_ADDR, &shm_physmap, SHM_SIZE, -1, 0,
		      NULL);

	shm_io = metal_device_io_region(device, 0);
	if (!shm_io) {
		LOG_ERR("Failed to get shm_io region");
		return -1;
	}

	/* declare resource table region */
	rsc_table_get(&rsc_tab_addr, &rsc_size);
	memcpy((void *)RSC_TABLE_ADDR, rsc_tab_addr, rsc_size);
	rsc_table = (struct st_resource_table *)RSC_TABLE_ADDR;

	metal_io_init(&device->regions[1], (void *)RSC_TABLE_ADDR, &rsc_physmap, rsc_size, -1, 0,
		      NULL);

	rsc_io = metal_device_io_region(device, 1);
	if (!rsc_io) {
		LOG_ERR("Failed to get rsc_io region");
		return -1;
	}

	/* Setup MBOX */
	if (!mbox_is_ready_dt(&tx_channel)) {
		LOG_ERR("MBOX TX device is not ready");
		return -1;
	}

	if (!mbox_is_ready_dt(&rx_channel)) {
		LOG_ERR("MBOX RX device is not ready");
		return -1;
	}

	mbox_register_callback_dt(&rx_channel, platform_mbox_callback, NULL);

	status = mbox_set_enabled_dt(&rx_channel, true);
	if (status) {
		LOG_ERR("mbox_set_enabled_dt failed");
		return -1;
	}

	return 0;
}

static void platform_deinit(void)
{
	mbox_set_enabled_dt(&rx_channel, false);

	metal_finish();
}

static void cleanup_system(void)
{
	struct fw_resource_table *rsc_tbl = (struct fw_resource_table *)RSC_TABLE_ADDR;

	rpmsg_deinit_vdev(&rvdev);
	rproc_virtio_remove_vdev(rvdev.vdev);
	/*
	 * Clean vdev status in rsc_table because it may not be cleared from
	 * master end. This is not default work behavior. By default rsc_table
	 * should be provided and managed by master.
	 */
	rsc_tbl->vdev.status = 0;
}

struct rpmsg_device *platform_create_rpmsg_vdev(unsigned int vdev_index, unsigned int role,
						void (*rst_cb)(struct virtio_device *vdev),
						rpmsg_ns_bind_cb ns_cb)
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

	/* Set gfeatures because they should be equal to dfeatures
	 * when create_ept is called. As rproc_virtio_create_vdev
	 * doesn't set them because its VIRTIO_DEVICE not DRIVER.
	 * Assume the virtio driver support all remote features.
	 */
	virtio_set_features(vdev, 0x1);

	/* wait master rpmsg init completion */
	rproc_virtio_wait_remote_ready(vdev);

	vring_rsc = rsc_table_get_vring0(rsc_table);

	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid, (void *)VRING_TX_ADDR_CM33,
				      rsc_io, vring_rsc->num, vring_rsc->align);
	if (ret) {
		LOG_ERR("failed to init vring 0");
		goto failed;
	}

	vring_rsc = rsc_table_get_vring1(rsc_table);

	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid, (void *)VRING_RX_ADDR_CM33,
				      rsc_io, vring_rsc->num, vring_rsc->align);
	if (ret) {
		LOG_ERR("failed to init vring 1");
		goto failed;
	}

	rpmsg_virtio_init_shm_pool(&shpool, NULL, SHM_SIZE);
	ret = rpmsg_init_vdev(&rvdev, vdev, ns_cb, shm_io, &shpool);

	if (ret) {
		LOG_ERR("failed rpmsg_init_vdev");
		goto failed;
	}

	return rpmsg_virtio_get_rpmsg_device(&rvdev);

failed:
	rproc_virtio_remove_vdev(vdev);

	return NULL;
}

void app_rpmsg_client_sample(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	unsigned int msg_cnt = 0;
	int ret = 0;

	k_sem_take(&data_sc_sem, K_FOREVER);

	LOG_INF("OpenAMP[remote] Linux sample client responder started");

	ret = rpmsg_create_ept(&sc_ept, rpdev, "rpmsg-service-0", APP_EPT_ADDR, RPMSG_ADDR_ANY,
			       rpmsg_recv_cs_callback, NULL);

	while (!finish) {
		k_sem_take(&data_sc_sem, K_FOREVER);
		msg_cnt++;
		rpmsg_send(&sc_ept, sc_msg.data, sc_msg.len);
	}

	rpmsg_destroy_ept(&sc_ept);
	k_sem_reset(&data_sc_sem);

	LOG_INF("OpenAMP Linux sample client responder ended");
}

void rpmsg_mng_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	unsigned char *msg;
	unsigned int len;
	int ret = 0;

	LOG_INF("OpenAMP[remote]  linux responder demo started");

	/* Initialize platform */
	rpdev = platform_create_rpmsg_vdev(0, VIRTIO_DEV_DEVICE, NULL, new_service_cb);
	if (!rpdev) {
		LOG_ERR("Failed to create rpmsg virtio device");
		ret = -1;
		goto task_end;
	}

	/* start the rpmsg clients */
	k_sem_give(&data_sc_sem);

	while (!finish) {
		receive_message(&msg, &len);
	}

task_end:
	cleanup_system();

	LOG_INF("OpenAMP demo ended");
}

int main(void)
{
	LOG_INF("Starting application..!");

	/* Initialize platform */
	int ret = platform_init();

	if (ret) {
		LOG_ERR("Failed to initialize platform");
		return -1;
	}

	while (1) {
		finish = 0;

		LOG_INF("Starting application threads!");
		k_thread_create(&thread_mng_data, thread_mng_stack, APP_TASK_STACK_SIZE,
				(k_thread_entry_t)rpmsg_mng_task, NULL, NULL, NULL, K_PRIO_COOP(8),
				0, K_NO_WAIT);
		k_thread_create(&thread_rp__client_data, thread_rp__client_stack,
				APP_TASK_STACK_SIZE, (k_thread_entry_t)app_rpmsg_client_sample,
				NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

		k_thread_join(&thread_mng_data, K_FOREVER);
		k_thread_join(&thread_rp__client_data, K_FOREVER);
	}

	platform_deinit();
	return 0;
}
