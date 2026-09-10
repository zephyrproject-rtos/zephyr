/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/rpmsgfs.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/ipm.h>
#include <openamp/open_amp.h>
#include <resource_table.h>

/* Constants derived from device tree */
#define SHM_NODE       DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE       DT_REG_SIZE(SHM_NODE)

#define APP_TASK_STACK_SIZE (1024)
#define MNG_THREAD_PRIORITY 1

K_THREAD_STACK_DEFINE(thread_mng_stack, APP_TASK_STACK_SIZE);

/* All tests must use this structure to mount file system. After each test this structure is cleaned
 * to allow for running next tests unaffected by previous one.
 */
struct fs_mount_t testfs_mnt = {
	.type = FS_RPMSGFS,
	.mnt_point = "/sml",
	.fs_data = "/test",
	.storage_dev = NULL,
	.flags = 0,
};

static struct k_thread thread_mng_data;

static const struct device *const ipm_handle = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));

static metal_phys_addr_t shm_physmap = SHM_START_ADDR;
static metal_phys_addr_t rsc_tab_physmap;

static struct metal_io_region shm_io_data; /* shared memory */
static struct metal_io_region rsc_io_data; /* rsc_table memory */

struct rpmsg_rcv_msg {
	void *data;
	size_t len;
};

static struct metal_io_region *shm_io = &shm_io_data;

static struct metal_io_region *rsc_io = &rsc_io_data;
static struct rpmsg_virtio_device rvdev;

static void *rsc_table;
static struct rpmsg_device *rpdev;

static K_SEM_DEFINE(data_sem, 0, 1);
static K_SEM_DEFINE(rpdev_sem, 0, 1);

static void platform_ipm_callback(const struct device *dev, void *context, uint32_t id,
				  volatile void *data)
{
	k_sem_give(&data_sem);
}

static void new_service_cb(struct rpmsg_device *rdev, const char *name, uint32_t src)
{
	TC_PRINT("%s: unexpected ns service receive for name %s\n", __func__, name);
}

static int mailbox_notify(void *priv, uint32_t id)
{
	ARG_UNUSED(priv);

	ipm_send(ipm_handle, 0, id, NULL, 0);
	return 0;
}

static int platform_init(void)
{
	int status;

	/* setup IPM */
	if (!device_is_ready(ipm_handle)) {
		TC_PRINT("IPM device is not ready\n");
		return -1;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_handle, 1);
	if (status) {
		TC_PRINT("ipm_set_enabled failed\n");
		return -1;
	}

	return 0;
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
		TC_PRINT("failed to create vdev\n");
		return NULL;
	}

	/* wait master rpmsg init completion */
	rproc_virtio_wait_remote_ready(vdev);

	vring_rsc = rsc_table_get_vring0(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid, (void *)vring_rsc->da, rsc_io,
				      vring_rsc->num, vring_rsc->align);
	if (ret) {
		TC_PRINT("failed to init vring 0\n");
		goto failed;
	}

	vring_rsc = rsc_table_get_vring1(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid, (void *)vring_rsc->da, rsc_io,
				      vring_rsc->num, vring_rsc->align);
	if (ret) {
		TC_PRINT("failed to init vring 1\n");
		goto failed;
	}

	ret = rpmsg_init_vdev(&rvdev, vdev, ns_cb, shm_io, NULL);
	if (ret) {
		TC_PRINT("failed rpmsg_init_vdev\n");
		goto failed;
	}

	return rpmsg_virtio_get_rpmsg_device(&rvdev);

failed:
	rproc_virtio_remove_vdev(vdev);

	return NULL;
}

void rpmsg_rpdev_wait(void)
{
	if (!rpdev) {
		k_sem_take(&rpdev_sem, K_FOREVER);
	}
}

void rpmsg_mng_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	rpdev = platform_create_rpmsg_vdev(0, VIRTIO_DEV_DEVICE, NULL, new_service_cb);
	if (!rpdev) {
		TC_PRINT("Failed to create rpmsg virtio device\n");
		return;
	}

	rpmsgfs_init_rpmsg(rpdev);

	k_sem_give(&rpdev_sem);

	while (1) {
		int status = k_sem_take(&data_sem, K_FOREVER);

		if (status == 0) {
			rproc_virtio_notified(rvdev.vdev, VRING1_ID);
		}
	}
}

static int init_metal_and_rsc(void)
{
	int rsc_size;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	int status;

	status = metal_init(&metal_params);
	if (status) {
		TC_PRINT("metal_init: failed: %d\n", status);
		return -1;
	}

	/* declare shared memory region */
	metal_io_init(shm_io, (void *)SHM_START_ADDR, &shm_physmap, SHM_SIZE, -1, 0, NULL);

	/* declare resource table region */
	rsc_table_get(&rsc_table, &rsc_size);
	rsc_tab_physmap = (uintptr_t)rsc_table;

	metal_io_init(rsc_io, rsc_table, &rsc_tab_physmap, rsc_size, -1, 0, NULL);

	return 0;
}

static int init_rpmsg(void)
{
	/* Initialize platform */
	int ret = platform_init();

	if (ret) {
		TC_PRINT("Failed to initialize platform\n");
		return -1;
	}

	TC_PRINT("Starting rpmsg mng thread\n");
	k_tid_t tid = k_thread_create(&thread_mng_data, thread_mng_stack, APP_TASK_STACK_SIZE,
				      rpmsg_mng_task, NULL, NULL, NULL, MNG_THREAD_PRIORITY, 0,
				      K_FOREVER);
	k_thread_start(tid);

	return 0;
};

static void *reset(void)
{
	init_rpmsg();
	rpmsg_rpdev_wait();
	return NULL;
}

static void before_test(void *f)
{
	ARG_UNUSED(f);

	testfs_mnt.flags = 0;
}

static void after_test(void *f)
{
	ARG_UNUSED(f);

	/* Unmount file system */
	fs_unmount(&testfs_mnt);
}

ZTEST_SUITE(rpmsgfs, NULL, reset, before_test, after_test, NULL);
SYS_INIT(init_metal_and_rsc, PRE_KERNEL_1, 0);
