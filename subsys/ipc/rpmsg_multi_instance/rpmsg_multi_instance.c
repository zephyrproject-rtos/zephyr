/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipc/rpmsg_multi_instance.h>
#include <zephyr.h>
#include <device.h>
#include <logging/log.h>

#include <drivers/ipm.h>
#include <openamp/open_amp.h>
#include <cache.h>

#include "rpmsg_multi_instance.h"

LOG_MODULE_REGISTER(rpmsg_multi_instance, CONFIG_RPMSG_MULTI_INSTANCE_LOG_LEVEL);

static bool config_correct;
static int instance;

static void rpmsg_service_unbind(struct rpmsg_endpoint *p_ep)
{
	rpmsg_destroy_ept(p_ep);
}

static unsigned char virtio_get_status(struct virtio_device *p_vdev)
{
	struct  rpmsg_mi_ctx *ctx = metal_container_of(p_vdev, struct rpmsg_mi_ctx, vdev);
	unsigned char ret = VIRTIO_CONFIG_STATUS_DRIVER_OK;

	if (!IS_ENABLED(CONFIG_RPMSG_MULTI_INSTANCE_MASTER)) {
		sys_cache_data_range(&ctx->shm_status_reg_addr,
				     sizeof(ctx->shm_status_reg_addr), K_CACHE_INVD);
		ret = sys_read8(ctx->shm_status_reg_addr);
	}

	return ret;
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

#ifdef CONFIG_RPMSG_MULTI_INSTANCE_MASTER

static void virtio_set_status(struct virtio_device *p_vdev, unsigned char status)
{
	struct rpmsg_mi_ctx *ctx  = metal_container_of(p_vdev, struct rpmsg_mi_ctx, vdev);

	sys_write8(status, ctx->shm_status_reg_addr);
	sys_cache_data_range(&ctx->shm_status_reg_addr,
			     sizeof(ctx->shm_status_reg_addr), K_CACHE_WB);
}

static void virtio_set_features(struct virtio_device *vdev, uint32_t features)
{
	/* No need for implementation */
}
#endif /* CONFIG_RPMSG_MULTI_INSTANCE_MASTER */

static void virtio_notify(struct virtqueue *vq)
{
	struct rpmsg_mi_ctx *ctx = metal_container_of(vq->vq_dev, struct rpmsg_mi_ctx, vdev);
	int status;

	if (ctx) {
		status = ipm_send(ctx->ipm_tx_handle, 0, CONFIG_IPM_MSG_ID, NULL, 0);
		if (status != 0) {
			LOG_WRN("Failed to notify: %d", status);
		}
	}
}

const static struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.get_features = virtio_get_features,
#ifdef CONFIG_RPMSG_MULTI_INSTANCE_MASTER
	.set_status = virtio_set_status,
	.set_features = virtio_set_features,
#endif /* CONFIG_RPMSG_MULTI_INSTANCE_MASTER */
	.notify = virtio_notify,
};

static void ipm_callback_process(struct k_work *item)
{
	struct rpmsg_mi_ctx *ctx = CONTAINER_OF(item, struct rpmsg_mi_ctx, ipm_work);

	LOG_DBG("Process callback. Instance name: %s", ctx->name);
	virtqueue_notification(ctx->vq[VIRTQUEUE_ID]);
}

static void ipm_callback(const struct device *dev, void *context, uint32_t id, volatile void *data)
{
	ARG_UNUSED(dev);
	struct rpmsg_mi_ctx *ctx = (struct rpmsg_mi_ctx *)context;

	k_work_submit_to_queue(&ctx->ipm_work_q, &ctx->ipm_work);
}

int rpmsg_mi_configure_shm(struct rpmsg_mi_ctx *ctx, const struct rpmsg_mi_ctx_cfg *cfg)
{
	uint8_t vring_size = VRING_SIZE_GET(SHM_SIZE);
	uint32_t shm_addr = SHMEM_INST_ADDR_AUTOALLOC_GET(SHM_START_ADDR, SHM_SIZE, instance);
	uint32_t shm_size = SHMEM_INST_SIZE_AUTOALLOC_GET(SHM_SIZE);

	uint32_t shm_local_start_addr = shm_addr + VDEV_STATUS_SIZE;
	uint32_t shm_local_size = shm_size - VDEV_STATUS_SIZE;

	uint32_t rpmsg_reg_size = VRING_COUNT * VIRTQUEUE_SIZE_GET(vring_size);
	uint32_t vring_region_size = VRING_SIZE_COMPUTE(vring_size, VRING_ALIGNMENT);

	ctx->shm_status_reg_addr = shm_addr;
	ctx->shm_physmap[0] = shm_local_start_addr;

	ctx->shm_device.name = SHM_DEVICE_NAME;
	ctx->shm_device.bus = NULL;
	ctx->shm_device.num_regions = 1;

	ctx->shm_device.regions->virt = (void *)shm_local_start_addr;
	ctx->shm_device.regions->physmap = ctx->shm_physmap;

	ctx->shm_device.regions->size = shm_local_size;
	ctx->shm_device.regions->page_shift = 0xffffffff;
	ctx->shm_device.regions->page_mask = 0xffffffff;
	ctx->shm_device.regions->mem_flags = 0;

	ctx->shm_device.irq_num = 0;
	ctx->shm_device.irq_info = NULL;

	ctx->vring_rx_addr = shm_local_start_addr + rpmsg_reg_size;
	ctx->vring_tx_addr = ctx->vring_rx_addr + vring_region_size;

	return 0;
}

static int ept_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	struct rpmsg_mi_ept *mi_ep = (struct rpmsg_mi_ept *)priv;

	if (len == 0) {
		if (!mi_ep->bound) {
			LOG_DBG("Handshake done");
			rpmsg_send(ept, (uint8_t *) "", 0);
			mi_ep->bound = true;
			if (mi_ep->cb->bound) {
				mi_ep->cb->bound(mi_ep->priv);
				return 0;
			}
		}
		return 0;
	}

	if (mi_ep->cb->received) {
		mi_ep->cb->received(data, len, mi_ep->priv);
	}

	return 0;
}

static void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	sys_snode_t *node;
	struct rpmsg_virtio_device *p_rvdev = metal_container_of(rdev,
								 struct rpmsg_virtio_device, rdev);
	struct rpmsg_mi_ctx *ctx = metal_container_of(p_rvdev, struct rpmsg_mi_ctx, rvdev);

	LOG_DBG("bind_cb endpoint: %s, for instance: %s", name ? log_strdup(name) : "", ctx->name);

	SYS_SLIST_FOR_EACH_NODE(&ctx->endpoints, node) {
		struct rpmsg_mi_ept *ept = CONTAINER_OF(node, struct rpmsg_mi_ept, node);

		if (strcmp(name, ept->name) == 0) {
			LOG_DBG("Master - Create endpoint: %s", ept->name);

			int err = rpmsg_create_ept(&ept->ep, rdev, name, RPMSG_ADDR_ANY,
						   dest, ept_cb, rpmsg_service_unbind);
			if (err != 0) {
				LOG_ERR("Creating remote endpoint %s"
					" failed wirh error %d", name, err);
			} else {
				/* Notify the remote site that binding has occurred */
				rpmsg_send(&ept->ep, (uint8_t *)"", 0);

				ept->bound = true;
				ept->cb->bound(ept->priv);
			}

			break;
		}
	}
}

int rpmsg_mi_ctx_init(struct rpmsg_mi_ctx *ctx, const struct rpmsg_mi_ctx_cfg *cfg)
{
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	uint8_t vring_size = VRING_SIZE_GET(SHM_SIZE);
	struct metal_device *device;
	int err;

	if (!ctx || !cfg) {
		return -EINVAL;
	}

	if (!config_correct) {
		return -EIO;
	}

	LOG_DBG("RPMsg multiple instance initialization");

	/* Start IPM workqueue */
	k_work_queue_start(&ctx->ipm_work_q, cfg->ipm_stack_area,
			   cfg->ipm_stack_size, cfg->ipm_work_q_prio, NULL);
	k_thread_name_set(&ctx->ipm_work_q.thread, cfg->ipm_thread_name);
	k_work_init(&ctx->ipm_work, ipm_callback_process);

	ctx->name = cfg->name;
	sys_slist_init(&ctx->endpoints);

	/* Configure share memory */
	err = rpmsg_mi_configure_shm(ctx, cfg);
	if (err) {
		LOG_ERR("shmem configuration: failed - error code %d", err);
		return err;
	}

	/* Libmetal setup */
	err = metal_init(&metal_params);
	if (err) {
		LOG_ERR("metal_init: failed - error code %d", err);
		return err;
	}

	err = metal_register_generic_device(&ctx->shm_device);
	if (err) {
		LOG_ERR("Could not register shared memory device: %d", err);
		return err;
	}

	err = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (err) {
		LOG_ERR("metal_device_open failed: %d", err);
		return err;
	}

	ctx->shm_io = metal_device_io_region(device, 0);
	if (!ctx->shm_io) {
		LOG_ERR("metal_device_io_region failed to get region");
		return err;
	}

	 /* IPM setup. */
	ctx->ipm_tx_handle = device_get_binding(cfg->ipm_tx_name);
	if (!ctx->ipm_tx_handle) {
		LOG_ERR("Could not get TX IPM device handle");
		return -ENODEV;
	}

	ctx->ipm_rx_handle = device_get_binding(cfg->ipm_rx_name);
	if (!ctx->ipm_rx_handle) {
		LOG_ERR("Could not get RX IPM device handle");
		return -ENODEV;
	}

	/* Register IPM callback. This cb executes when msg has come. */
	ipm_register_callback(ctx->ipm_rx_handle, ipm_callback, ctx);

	ctx->vq[RPMSG_VQ_0] = virtqueue_allocate(vring_size);
	if (!ctx->vq[RPMSG_VQ_0]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[RPMSG_VQ_0]");
		return -ENOMEM;
	}

	ctx->vq[RPMSG_VQ_1] = virtqueue_allocate(vring_size);
	if (!ctx->vq[RPMSG_VQ_1]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[RPMSG_VQ_1]");
		return -ENOMEM;
	}

	ctx->rvrings[RPMSG_VQ_0].io = ctx->shm_io;
	ctx->rvrings[RPMSG_VQ_0].info.vaddr = (void *)ctx->vring_tx_addr;
	ctx->rvrings[RPMSG_VQ_0].info.num_descs = vring_size;
	ctx->rvrings[RPMSG_VQ_0].info.align = VRING_ALIGNMENT;
	ctx->rvrings[RPMSG_VQ_0].vq = ctx->vq[RPMSG_VQ_0];

	ctx->rvrings[RPMSG_VQ_1].io = ctx->shm_io;
	ctx->rvrings[RPMSG_VQ_1].info.vaddr = (void *)ctx->vring_rx_addr;
	ctx->rvrings[RPMSG_VQ_1].info.num_descs = vring_size;
	ctx->rvrings[RPMSG_VQ_1].info.align = VRING_ALIGNMENT;
	ctx->rvrings[RPMSG_VQ_1].vq = ctx->vq[RPMSG_VQ_1];

	ctx->vdev.role = IS_ENABLED(CONFIG_RPMSG_MULTI_INSTANCE_MASTER) ?
			 RPMSG_MASTER : RPMSG_REMOTE;

	ctx->vdev.vrings_num = VRING_COUNT;
	ctx->vdev.func = &dispatch;
	ctx->vdev.vrings_info = &ctx->rvrings[0];

	if (IS_ENABLED(CONFIG_RPMSG_MULTI_INSTANCE_MASTER)) {
		/* This step is only required if you are VirtIO device master.
		 * Initialize the shared buffers pool.
		 */
		rpmsg_virtio_init_shm_pool(&ctx->shpool, (void *) ctx->shm_device.regions->virt,
					   ctx->shm_device.regions->size);

		err = rpmsg_init_vdev(&ctx->rvdev, &ctx->vdev, ns_bind_cb,
				      ctx->shm_io, &ctx->shpool);
	} else {
		err = rpmsg_init_vdev(&ctx->rvdev, &ctx->vdev, NULL, ctx->shm_io, NULL);
	}


	if (err) {
		LOG_ERR("RPMSG vdev initialization failed %d", err);
		return err;
	}

	/* Get RPMsg device from RPMsg VirtIO device. */
	ctx->rdev = rpmsg_virtio_get_rpmsg_device(&ctx->rvdev);

	instance++;

	LOG_DBG("RPMsg multiple instance initialization done");

	return 0;
}

int rpmsg_mi_ept_register(struct rpmsg_mi_ctx *ctx, struct rpmsg_mi_ept *ept,
			  struct rpmsg_mi_ept_cfg *cfg)
{
	if (!ctx || !ept || !cfg) {
		return -EINVAL;
	}

	ept->cb = cfg->cb;
	ept->priv = cfg->priv;
	ept->ep.priv = ept;
	ept->bound = false;
	ept->name = cfg->name;

	sys_slist_append(&ctx->endpoints, &ept->node);

	if (!IS_ENABLED(CONFIG_RPMSG_MULTI_INSTANCE_MASTER)) {
		LOG_DBG("Remote - Create endpoint: %s", ept->name);

		int err = rpmsg_create_ept(&ept->ep, ctx->rdev, ept->name, RPMSG_ADDR_ANY,
					   RPMSG_ADDR_ANY, ept_cb, rpmsg_service_unbind);

		if (err != 0) {
			LOG_ERR("RPMSG endpoint create failed %d", err);
			return err;
		}
	}

	return 0;
}

int rpmsg_mi_send(struct rpmsg_mi_ept *ept, const void *data, size_t len)
{
	return rpmsg_send(&ept->ep, data, len);
}

static bool rpmsg_mi_config_verify(void)
{
	if (SHMEM_INST_SIZE_AUTOALLOC_GET(SHM_SIZE) * IPC_INSTANCE_COUNT > SHM_SIZE) {
		LOG_ERR("Not enough memory");
		return false;
	}
	return true;
}

static int rpmsg_mi_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("Initialization of RPMsg multiple instance");
	config_correct = rpmsg_mi_config_verify();

	return 0;
}

SYS_INIT(rpmsg_mi_init, POST_KERNEL, CONFIG_RPMSG_MULTI_INSTANCE_INIT_PRIORITY);
