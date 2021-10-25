/*
 * Copyright (c) 2020-2021, Nordic Semiconductor ASA
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipc/ipc_service_backend.h>
#include <ipc/ipc_rpmsg.h>
#include <ipc/ipc_static_vrings.h>

#include <logging/log.h>
#include <zephyr.h>
#include <cache.h>
#include <device.h>
#include <drivers/ipm.h>

#include "ipc_rpmsg_static_vrings_mi.h"

#define MI_BACKEND_DRIVER_NAME "MI_BACKEND"

LOG_MODULE_REGISTER(ipc_rpmsg_multi_instance, CONFIG_IPC_SERVICE_LOG_LEVEL);

#define WQ_STACK_SIZE	CONFIG_IPC_SERVICE_BACKEND_RPMSG_MI_WQ_STACK_SIZE

#define PRIO_INIT_VAL	INT_MAX
#define INST_NAME_SIZE	16
#define IPM_MSG_ID	0

#define CH_NAME(idx, sub) (CONFIG_IPC_SERVICE_BACKEND_RPMSG_MI_ ## idx ## _IPM_ ## sub ## _NAME)

static char *ipm_rx_name[] = {
	FOR_EACH_FIXED_ARG(CH_NAME, (,), RX, 0, 1, 2, 3, 4, 5, 6, 7),
};

static char *ipm_tx_name[] = {
	FOR_EACH_FIXED_ARG(CH_NAME, (,), TX, 0, 1, 2, 3, 4, 5, 6, 7),
};

BUILD_ASSERT(ARRAY_SIZE(ipm_rx_name) >= NUM_INSTANCES, "Invalid configuration");
BUILD_ASSERT(ARRAY_SIZE(ipm_tx_name) >= NUM_INSTANCES, "Invalid configuration");

K_THREAD_STACK_ARRAY_DEFINE(ipm_stack, NUM_INSTANCES, WQ_STACK_SIZE);

struct rpmsg_mi_instance {
	/* RPMsg */
	struct ipc_rpmsg_instance rpmsg_inst;

	/* Static VRINGs */
	struct ipc_static_vrings vr;

	/* General */
	char name[INST_NAME_SIZE];
	bool is_initialized;
	unsigned int id;

	/* IPM */
	const struct device *ipm_tx_handle;
	const struct device *ipm_rx_handle;
	struct k_work_q ipm_wq;
	struct k_work ipm_work;
	int priority;

	/* Role */
	unsigned int role;
};

struct {
	uintptr_t addr;
	size_t size;
	size_t instance;
} shm = {
	.addr = SHM_START_ADDR,
	.size = SHM_SIZE,
};

static struct rpmsg_mi_instance instance[NUM_INSTANCES];

static int send(const struct device *instance, void *token,
		const void *data, size_t len)
{
	struct ipc_rpmsg_ept *rpmsg_ept;

	rpmsg_ept = (struct ipc_rpmsg_ept *) token;

	return rpmsg_send(&rpmsg_ept->ep, data, len);
}

static struct rpmsg_mi_instance *get_available_instance(const struct ipc_ept_cfg *cfg)
{
	/* Endpoints with the same priority are registered to the same instance. */
	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		if (instance[i].priority == cfg->prio || instance[i].priority == PRIO_INIT_VAL) {
			return &instance[i];
		}
	}
	return NULL;
}

static struct ipc_rpmsg_ept *get_available_ept_slot(struct ipc_rpmsg_instance *rpmsg_instance)
{
	for (size_t i = 0; i < NUM_ENDPOINTS; i++) {
		if (rpmsg_instance->endpoint[i].name == NULL) {
			return &rpmsg_instance->endpoint[i];
		}
	}
	return NULL;
}

static void ipm_callback_process(struct k_work *item)
{
	struct rpmsg_mi_instance *instance;
	unsigned int id;

	instance = CONTAINER_OF(item, struct rpmsg_mi_instance, ipm_work);
	id = (instance->role == VIRTIO_DEV_MASTER) ?
		VIRTQUEUE_ID_MASTER : VIRTQUEUE_ID_REMOTE;

	virtqueue_notification(instance->vr.vq[id]);
}

static void ipm_callback(const struct device *dev, void *context, uint32_t id, volatile void *data)
{
	struct rpmsg_mi_instance *instance = (struct rpmsg_mi_instance *) context;

	k_work_submit_to_queue(&instance->ipm_wq, &instance->ipm_work);
}

static int ipm_setup(struct rpmsg_mi_instance *instance)
{
	instance->ipm_tx_handle = device_get_binding(ipm_tx_name[instance->id]);
	if (instance->ipm_tx_handle == NULL) {
		return -ENODEV;
	}

	instance->ipm_rx_handle = device_get_binding(ipm_rx_name[instance->id]);
	if (instance->ipm_rx_handle == NULL) {
		return -ENODEV;
	}

	k_work_queue_start(&instance->ipm_wq, ipm_stack[instance->id],
			   K_THREAD_STACK_SIZEOF(ipm_stack[instance->id]),
			   instance->priority, NULL);

	k_thread_name_set(&instance->ipm_wq.thread, instance->name);

	k_work_init(&instance->ipm_work, ipm_callback_process);

	ipm_register_callback(instance->ipm_rx_handle, ipm_callback, instance);

	return ipm_set_enabled(instance->ipm_rx_handle, 1);
}

static void shm_configure(struct rpmsg_mi_instance *instance)
{
	size_t vring_size, shm_size, shm_local_size;
	size_t rpmsg_reg_size, vring_reg_size;
	uintptr_t shm_addr, shm_local_addr;

	vring_size = VRING_SIZE_GET(shm.size);
	shm_addr = SHMEM_INST_ADDR_AUTOALLOC_GET(shm.addr, shm.size, shm.instance);
	shm_size = SHMEM_INST_SIZE_AUTOALLOC_GET(shm.size);

	shm_local_addr = shm_addr + VDEV_STATUS_SIZE;
	shm_local_size = shm_size - VDEV_STATUS_SIZE;

	rpmsg_reg_size = VRING_COUNT * VIRTQUEUE_SIZE_GET(vring_size);
	vring_reg_size = VRING_SIZE_COMPUTE(vring_size, VRING_ALIGNMENT);

	instance->vr.status_reg_addr = shm_addr;
	instance->vr.vring_size = vring_size;
	instance->vr.rx_addr = shm_local_addr + rpmsg_reg_size;
	instance->vr.tx_addr = instance->vr.rx_addr + vring_reg_size;
	instance->vr.shm_addr = shm_local_addr;
	instance->vr.shm_size = shm_local_size;
}

static void bound_cb(struct ipc_rpmsg_ept *ept)
{
	/* Notify the remote site that binding has occurred */
	rpmsg_send(&ept->ep, (uint8_t *)"", 0);

	if (ept->cb->bound) {
		ept->cb->bound(ept->priv);
	}
}

static int ept_cb(struct rpmsg_endpoint *ep, void *data, size_t len, uint32_t src, void *priv)
{
	struct ipc_rpmsg_ept *ept;

	ept = (struct ipc_rpmsg_ept *) priv;

	if (len == 0) {
		if (!ept->bound) {
			ept->bound = true;
			bound_cb(ept);
		}
		return RPMSG_SUCCESS;
	}

	if (ept->cb->received) {
		ept->cb->received(data, len, ept->priv);
	}

	return RPMSG_SUCCESS;
}

static void virtio_notify_cb(struct virtqueue *vq, void *priv)
{
	struct rpmsg_mi_instance *instance;

	instance = (struct rpmsg_mi_instance *) priv;

	if (instance) {
		ipm_send(instance->ipm_tx_handle, 0, IPM_MSG_ID, NULL, 0);
	}
}

static int init_instance(struct rpmsg_mi_instance *instance)
{
	int err = 0;

	/* Check if there is enough space in the SHM */
	if (SHMEM_INST_SIZE_AUTOALLOC_GET(shm.size) * NUM_INSTANCES > shm.size) {
		return -ENOMEM;
	}

	shm_configure(instance);

	instance->vr.notify_cb = virtio_notify_cb;
	instance->vr.priv = instance;

	ipc_static_vrings_init(&instance->vr, instance->role);
	if (err != 0) {
		return err;
	}

	err = ipm_setup(instance);
	if (err != 0) {
		return err;
	}

	shm.instance++;

	return 0;
}

static int register_ept(const struct device *dev,
			void **token,
			const struct ipc_ept_cfg *cfg)
{
	struct ipc_rpmsg_instance *rpmsg_instance;
	struct rpmsg_mi_instance *instance;
	struct ipc_rpmsg_ept *rpmsg_ept;
	int err;

	ARG_UNUSED(dev);

	if (!cfg || !token) {
		return -EINVAL;
	}

	instance = get_available_instance(cfg);
	if (instance == NULL) {
		return -ENODEV;
	}

	rpmsg_instance = &instance->rpmsg_inst;

	if (!instance->is_initialized) {
		snprintf(instance->name, INST_NAME_SIZE, "rpmsg_mi_%d", instance->id);

		instance->priority = cfg->prio;

		err = init_instance(instance);
		if (err) {
			return err;
		}

		rpmsg_instance->bound_cb = bound_cb;
		rpmsg_instance->cb = ept_cb;

		err = ipc_rpmsg_init(rpmsg_instance,
				     instance->role,
				     instance->vr.shm_io,
				     &instance->vr.vdev,
				     (void *) instance->vr.shm_device.regions->virt,
				     instance->vr.shm_device.regions->size, NULL);
		if (err != 0) {
			return err;
		}

		instance->is_initialized = true;
	}

	rpmsg_ept = get_available_ept_slot(rpmsg_instance);
	if (rpmsg_ept == NULL) {
		return -ENODEV;
	}

	rpmsg_ept->name = cfg->name;
	rpmsg_ept->cb = &cfg->cb;
	rpmsg_ept->priv = cfg->priv;
	rpmsg_ept->bound = false;
	rpmsg_ept->ep.priv = rpmsg_ept;

	err = ipc_rpmsg_register_ept(rpmsg_instance, instance->role, rpmsg_ept);
	if (err != 0) {
		return err;
	}

	(*token) = rpmsg_ept;

	return 0;
}

const static struct ipc_service_backend backend_ops = {
	.send = send,
	.register_endpoint = register_ept,
};

static int backend_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		instance[i].priority = PRIO_INIT_VAL;
		instance[i].id = i;
		instance[i].role = IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_RPMSG_MI_MASTER) ?
				   VIRTIO_DEV_MASTER : VIRTIO_DEV_SLAVE;
	}

	return 0;
}

DEVICE_DEFINE(mi_backend, MI_BACKEND_DRIVER_NAME, &backend_init, NULL, NULL,
	      NULL, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	      &backend_ops);
