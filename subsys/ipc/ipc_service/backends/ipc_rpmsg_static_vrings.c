/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/ipc/ipc_static_vrings.h>
#include <zephyr/ipc/ipc_rpmsg.h>

#include <zephyr/drivers/mbox.h>
#include <zephyr/dt-bindings/ipc_service/static_vrings.h>

#include "ipc_rpmsg_static_vrings.h"

#define DT_DRV_COMPAT	zephyr_ipc_openamp_static_vrings

#define NUM_INSTANCES	DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

#define WQ_STACK_SIZE	CONFIG_IPC_SERVICE_BACKEND_RPMSG_WQ_STACK_SIZE

#define STATE_READY	(0)
#define STATE_BUSY	(1)
#define STATE_INITED	(2)

#if defined(CONFIG_THREAD_MAX_NAME_LEN)
#define THREAD_MAX_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_MAX_NAME_LEN 1
#endif

K_THREAD_STACK_ARRAY_DEFINE(mbox_stack, NUM_INSTANCES, WQ_STACK_SIZE);

struct backend_data_t {
	/* RPMsg */
	struct ipc_rpmsg_instance rpmsg_inst;

	/* Static VRINGs */
	struct ipc_static_vrings vr;

	/* MBOX WQ */
	struct k_work mbox_work;
	struct k_work_q mbox_wq;

	/* General */
	unsigned int role;
	atomic_t state;

	/* TX buffer size */
	int tx_buffer_size;
};

struct backend_config_t {
	unsigned int role;
	uintptr_t shm_addr;
	size_t shm_size;
	struct mbox_dt_spec mbox_tx;
	struct mbox_dt_spec mbox_rx;
	unsigned int wq_prio_type;
	unsigned int wq_prio;
	unsigned int id;
	unsigned int buffer_size;
};

static void rpmsg_service_unbind(struct rpmsg_endpoint *ep)
{
	rpmsg_destroy_ept(ep);
}

static struct ipc_rpmsg_ept *get_ept_slot_with_name(struct ipc_rpmsg_instance *rpmsg_inst,
						    const char *name)
{
	struct ipc_rpmsg_ept *rpmsg_ept;

	for (size_t i = 0; i < NUM_ENDPOINTS; i++) {
		rpmsg_ept = &rpmsg_inst->endpoint[i];

		if (strcmp(name, rpmsg_ept->name) == 0) {
			return &rpmsg_inst->endpoint[i];
		}
	}

	return NULL;
}

static struct ipc_rpmsg_ept *get_available_ept_slot(struct ipc_rpmsg_instance *rpmsg_inst)
{
	return get_ept_slot_with_name(rpmsg_inst, "");
}

static bool check_endpoints_freed(struct ipc_rpmsg_instance *rpmsg_inst)
{
	struct ipc_rpmsg_ept *rpmsg_ept;

	for (size_t i = 0; i < NUM_ENDPOINTS; i++) {
		rpmsg_ept = &rpmsg_inst->endpoint[i];

		if (rpmsg_ept->bound == true) {
			return false;
		}
	}

	return true;
}

/*
 * Returns:
 *  - true:  when the endpoint was already cached / registered
 *  - false: when the endpoint was never registered before
 *
 * Returns in **rpmsg_ept:
 *  - The endpoint with the name *name if it exists
 *  - The first endpoint slot available when the endpoint with name *name does
 *    not exist
 *  - NULL in case of error
 */
static bool get_ept(struct ipc_rpmsg_instance *rpmsg_inst,
		    struct ipc_rpmsg_ept **rpmsg_ept, const char *name)
{
	struct ipc_rpmsg_ept *ept;

	ept = get_ept_slot_with_name(rpmsg_inst, name);
	if (ept != NULL) {
		(*rpmsg_ept) = ept;
		return true;
	}

	ept = get_available_ept_slot(rpmsg_inst);
	if (ept != NULL) {
		(*rpmsg_ept) = ept;
		return false;
	}

	(*rpmsg_ept) = NULL;

	return false;
}

static void advertise_ept(struct ipc_rpmsg_instance *rpmsg_inst, struct ipc_rpmsg_ept *rpmsg_ept,
			  const char *name, uint32_t dest)
{
	struct rpmsg_device *rdev;
	int err;

	rdev = rpmsg_virtio_get_rpmsg_device(&rpmsg_inst->rvdev);

	err = rpmsg_create_ept(&rpmsg_ept->ep, rdev, name, RPMSG_ADDR_ANY,
			       dest, rpmsg_inst->cb, rpmsg_service_unbind);
	if (err != 0) {
		return;
	}

	rpmsg_ept->bound = true;
	if (rpmsg_inst->bound_cb) {
		rpmsg_inst->bound_cb(rpmsg_ept);
	}
}

static void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	struct ipc_rpmsg_instance *rpmsg_inst;
	struct rpmsg_virtio_device *p_rvdev;
	struct ipc_rpmsg_ept *rpmsg_ept;
	bool ept_cached;

	p_rvdev = CONTAINER_OF(rdev, struct rpmsg_virtio_device, rdev);
	rpmsg_inst = CONTAINER_OF(p_rvdev->shpool, struct ipc_rpmsg_instance, shm_pool);

	if (name == NULL || name[0] == '\0') {
		return;
	}

	k_mutex_lock(&rpmsg_inst->mtx, K_FOREVER);
	ept_cached = get_ept(rpmsg_inst, &rpmsg_ept, name);

	if (rpmsg_ept == NULL) {
		k_mutex_unlock(&rpmsg_inst->mtx);
		return;
	}

	if (ept_cached) {
		/*
		 * The endpoint was already registered by the HOST core. The
		 * endpoint can now be advertised to the REMOTE core.
		 */
		k_mutex_unlock(&rpmsg_inst->mtx);
		advertise_ept(rpmsg_inst, rpmsg_ept, name, dest);
	} else {
		/*
		 * The endpoint is not registered yet, this happens when the
		 * REMOTE core registers the endpoint before the HOST has
		 * had the chance to register it. Cache it saving name and
		 * destination address to be used by the next register_ept()
		 * call by the HOST core.
		 */
		strncpy(rpmsg_ept->name, name, sizeof(rpmsg_ept->name));
		rpmsg_ept->name[RPMSG_NAME_SIZE - 1] = '\0';
		rpmsg_ept->dest = dest;
		k_mutex_unlock(&rpmsg_inst->mtx);
	}
}

static void bound_cb(struct ipc_rpmsg_ept *ept)
{
	rpmsg_send(&ept->ep, (uint8_t *)"", 0);

	if (ept->cb->bound) {
		ept->cb->bound(ept->priv);
	}
}

static int ept_cb(struct rpmsg_endpoint *ep, void *data, size_t len, uint32_t src, void *priv)
{
	struct ipc_rpmsg_ept *ept;

	ept = (struct ipc_rpmsg_ept *) priv;

	/*
	 * the remote processor has send a ns announcement, we use an empty
	 * message to advice the remote side that a local endpoint has been
	 * created and that the processor is ready to communicate with this
	 * endpoint
	 *
	 * ipc_rpmsg_register_ept
	 *  rpmsg_send_ns_message --------------> ns_bind_cb
	 *                                        bound_cb
	 *                ept_cb <--------------- rpmsg_send [empty message]
	 *              bound_cb
	 */
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

static int vr_shm_configure(struct ipc_static_vrings *vr, const struct backend_config_t *conf)
{
	unsigned int num_desc;

	num_desc = optimal_num_desc(conf->shm_size, conf->buffer_size);
	if (num_desc == 0) {
		return -ENOMEM;
	}

	/*
	 * conf->shm_addr  +--------------+  vr->status_reg_addr
	 *		   |    STATUS    |
	 *		   +--------------+  vr->shm_addr
	 *		   |              |
	 *		   |              |
	 *		   |   RX BUFS    |
	 *		   |              |
	 *		   |              |
	 *		   +--------------+
	 *		   |              |
	 *		   |              |
	 *		   |   TX BUFS    |
	 *		   |              |
	 *		   |              |
	 *		   +--------------+  vr->rx_addr (aligned)
	 *		   |   RX VRING   |
	 *		   +--------------+  vr->tx_addr (aligned)
	 *		   |   TX VRING   |
	 *		   +--------------+
	 */

	vr->shm_addr = ROUND_UP(conf->shm_addr + VDEV_STATUS_SIZE, MEM_ALIGNMENT);
	vr->shm_size = shm_size(num_desc, conf->buffer_size);

	vr->rx_addr = vr->shm_addr + VRING_COUNT * vq_ring_size(num_desc, conf->buffer_size);
	vr->tx_addr = ROUND_UP(vr->rx_addr + vring_size(num_desc, MEM_ALIGNMENT),
			       MEM_ALIGNMENT);

	vr->status_reg_addr = conf->shm_addr;

	vr->vring_size = num_desc;

	return 0;
}

static void virtio_notify_cb(struct virtqueue *vq, void *priv)
{
	struct backend_config_t *conf = priv;

	if (conf->mbox_tx.dev) {
		mbox_send_dt(&conf->mbox_tx, NULL);
	}
}

static void mbox_callback_process(struct k_work *item)
{
	struct backend_data_t *data;
	unsigned int vq_id;

	data = CONTAINER_OF(item, struct backend_data_t, mbox_work);
	vq_id = (data->role == ROLE_HOST) ? VIRTQUEUE_ID_HOST : VIRTQUEUE_ID_REMOTE;

	virtqueue_notification(data->vr.vq[vq_id]);
}

static void mbox_callback(const struct device *instance, uint32_t channel,
			  void *user_data, struct mbox_msg *msg_data)
{
	struct backend_data_t *data = user_data;

	k_work_submit_to_queue(&data->mbox_wq, &data->mbox_work);
}

static int mbox_init(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	struct k_work_queue_config wq_cfg = {.name = instance->name};
	int prio, err;

	prio = (conf->wq_prio_type == PRIO_COOP) ? K_PRIO_COOP(conf->wq_prio) :
						   K_PRIO_PREEMPT(conf->wq_prio);

	k_work_queue_init(&data->mbox_wq);
	k_work_queue_start(&data->mbox_wq, mbox_stack[conf->id], WQ_STACK_SIZE, prio, &wq_cfg);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		char name[THREAD_MAX_NAME_LEN];

		snprintk(name, sizeof(name), "mbox_wq #%d", conf->id);
		k_thread_name_set(&data->mbox_wq.thread, name);
	}

	k_work_init(&data->mbox_work, mbox_callback_process);

	err = mbox_register_callback_dt(&conf->mbox_rx, mbox_callback, data);
	if (err != 0) {
		return err;
	}

	return mbox_set_enabled_dt(&conf->mbox_rx, 1);
}

static int mbox_deinit(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	k_tid_t wq_thread;
	int err;

	err = mbox_set_enabled_dt(&conf->mbox_rx, 0);
	if (err != 0) {
		return err;
	}

	k_work_queue_drain(&data->mbox_wq, 1);

	wq_thread = k_work_queue_thread_get(&data->mbox_wq);
	k_thread_abort(wq_thread);

	return 0;
}

static struct ipc_rpmsg_ept *register_ept_on_host(struct ipc_rpmsg_instance *rpmsg_inst,
						  const struct ipc_ept_cfg *cfg)
{
	struct ipc_rpmsg_ept *rpmsg_ept;
	bool ept_cached;

	k_mutex_lock(&rpmsg_inst->mtx, K_FOREVER);

	ept_cached = get_ept(rpmsg_inst, &rpmsg_ept, cfg->name);
	if (rpmsg_ept == NULL) {
		k_mutex_unlock(&rpmsg_inst->mtx);
		return NULL;
	}

	rpmsg_ept->cb = &cfg->cb;
	rpmsg_ept->priv = cfg->priv;
	rpmsg_ept->bound = false;
	rpmsg_ept->ep.priv = rpmsg_ept;

	if (ept_cached) {
		/*
		 * The endpoint was cached in the NS bind callback. We can finally
		 * advertise it.
		 */
		k_mutex_unlock(&rpmsg_inst->mtx);
		advertise_ept(rpmsg_inst, rpmsg_ept, cfg->name, rpmsg_ept->dest);
	} else {
		/*
		 * There is no endpoint in the cache because the REMOTE has
		 * not registered the endpoint yet. Cache it.
		 */
		strncpy(rpmsg_ept->name, cfg->name, sizeof(rpmsg_ept->name));
		rpmsg_ept->name[RPMSG_NAME_SIZE - 1] = '\0';
		k_mutex_unlock(&rpmsg_inst->mtx);
	}

	return rpmsg_ept;
}

static struct ipc_rpmsg_ept *register_ept_on_remote(struct ipc_rpmsg_instance *rpmsg_inst,
						    const struct ipc_ept_cfg *cfg)
{
	struct ipc_rpmsg_ept *rpmsg_ept;
	int err;

	rpmsg_ept = get_available_ept_slot(rpmsg_inst);
	if (rpmsg_ept == NULL) {
		return NULL;
	}

	rpmsg_ept->cb = &cfg->cb;
	rpmsg_ept->priv = cfg->priv;
	rpmsg_ept->bound = false;
	rpmsg_ept->ep.priv = rpmsg_ept;

	strncpy(rpmsg_ept->name, cfg->name, sizeof(rpmsg_ept->name));
	rpmsg_ept->name[RPMSG_NAME_SIZE - 1] = '\0';

	err = ipc_rpmsg_register_ept(rpmsg_inst, RPMSG_REMOTE, rpmsg_ept);
	if (err != 0) {
		return NULL;
	}

	return rpmsg_ept;
}

static int register_ept(const struct device *instance, void **token,
			const struct ipc_ept_cfg *cfg)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_instance *rpmsg_inst;
	struct ipc_rpmsg_ept *rpmsg_ept;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	/* Empty name is not valid */
	if (cfg->name == NULL || cfg->name[0] == '\0') {
		return -EINVAL;
	}

	rpmsg_inst = &data->rpmsg_inst;

	rpmsg_ept = (data->role == ROLE_HOST) ?
			register_ept_on_host(rpmsg_inst, cfg) :
			register_ept_on_remote(rpmsg_inst, cfg);
	if (rpmsg_ept == NULL) {
		return -EINVAL;
	}

	(*token) = rpmsg_ept;

	return 0;
}

static int deregister_ept(const struct device *instance, void *token)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_ept *rpmsg_ept;
	static struct k_work_sync sync;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	rpmsg_ept = (struct ipc_rpmsg_ept *) token;

	/* Endpoint is not registered with instance */
	if (!rpmsg_ept) {
		return -ENOENT;
	}

	/* Drain pending work items before tearing down channel.
	 *
	 * Note: `k_work_flush` Faults on Cortex-M33 with "illegal use of EPSR"
	 * if `sync` is not declared static.
	 */
	k_work_flush(&data->mbox_work, &sync);

	rpmsg_destroy_ept(&rpmsg_ept->ep);

	memset(rpmsg_ept, 0, sizeof(struct ipc_rpmsg_ept));

	return 0;
}

static int send(const struct device *instance, void *token,
		const void *msg, size_t len)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_ept *rpmsg_ept;
	int ret;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	/* Empty message is not allowed */
	if (len == 0) {
		return -EBADMSG;
	}

	rpmsg_ept = (struct ipc_rpmsg_ept *) token;

	/* Endpoint is not registered with instance */
	if (!rpmsg_ept) {
		return -ENOENT;
	}

	ret = rpmsg_send(&rpmsg_ept->ep, msg, len);

	/* No buffers available */
	if (ret == RPMSG_ERR_NO_BUFF) {
		return -ENOMEM;
	}

	return ret;
}

static int send_nocopy(const struct device *instance, void *token,
		       const void *msg, size_t len)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_ept *rpmsg_ept;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	/* Empty message is not allowed */
	if (len == 0) {
		return -EBADMSG;
	}

	rpmsg_ept = (struct ipc_rpmsg_ept *) token;

	/* Endpoint is not registered with instance */
	if (!rpmsg_ept) {
		return -ENOENT;
	}

	return rpmsg_send_nocopy(&rpmsg_ept->ep, msg, len);
}

static int open(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_instance *rpmsg_inst;
	struct rpmsg_device *rdev;
	int err;

	if (!atomic_cas(&data->state, STATE_READY, STATE_BUSY)) {
		return -EALREADY;
	}

	err = vr_shm_configure(&data->vr, conf);
	if (err != 0) {
		goto error;
	}

	data->vr.notify_cb = virtio_notify_cb;
	data->vr.priv = (void *) conf;

	err = ipc_static_vrings_init(&data->vr, conf->role);
	if (err != 0) {
		goto error;
	}

	err = mbox_init(instance);
	if (err != 0) {
		goto error;
	}

	rpmsg_inst = &data->rpmsg_inst;

	rpmsg_inst->bound_cb = bound_cb;
	rpmsg_inst->cb = ept_cb;

	err = ipc_rpmsg_init(rpmsg_inst, data->role, conf->buffer_size,
			     &data->vr.shm_io, &data->vr.vdev,
			     (void *)data->vr.shm_addr,
			     data->vr.shm_size, ns_bind_cb);
	if (err != 0) {
		goto error;
	}

	rdev = rpmsg_virtio_get_rpmsg_device(&rpmsg_inst->rvdev);

	data->tx_buffer_size = rpmsg_virtio_get_buffer_size(rdev);
	if (data->tx_buffer_size < 0) {
		err = -EINVAL;
		goto error;
	}

	atomic_set(&data->state, STATE_INITED);
	return 0;

error:
	/* Back to the ready state */
	atomic_set(&data->state, STATE_READY);
	return err;

}

static int close(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_instance *rpmsg_inst;
	int err;

	if (!atomic_cas(&data->state, STATE_INITED, STATE_BUSY)) {
		return -EALREADY;
	}

	rpmsg_inst = &data->rpmsg_inst;

	if (!check_endpoints_freed(rpmsg_inst)) {
		return -EBUSY;
	}

	err = ipc_rpmsg_deinit(rpmsg_inst, data->role);
	if (err != 0) {
		goto error;
	}

	err = mbox_deinit(instance);
	if (err != 0) {
		goto error;
	}

	err = ipc_static_vrings_deinit(&data->vr, conf->role);
	if (err != 0) {
		goto error;
	}

	memset(&data->vr, 0, sizeof(struct ipc_static_vrings));
	memset(rpmsg_inst, 0, sizeof(struct ipc_rpmsg_instance));

	atomic_set(&data->state, STATE_READY);
	return 0;

error:
	/* Back to the inited state */
	atomic_set(&data->state, STATE_INITED);
	return err;
}

static int get_tx_buffer_size(const struct device *instance, void *token)
{
	struct backend_data_t *data = instance->data;

	return data->tx_buffer_size;
}

static int get_tx_buffer(const struct device *instance, void *token,
			 void **r_data, uint32_t *size, k_timeout_t wait)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_ept *rpmsg_ept;
	void *payload;

	rpmsg_ept = (struct ipc_rpmsg_ept *) token;

	/* Endpoint is not registered with instance */
	if (!rpmsg_ept) {
		return -ENOENT;
	}

	if (!r_data || !size) {
		return -EINVAL;
	}

	/* OpenAMP only supports a binary wait / no-wait */
	if (!K_TIMEOUT_EQ(wait, K_FOREVER) && !K_TIMEOUT_EQ(wait, K_NO_WAIT)) {
		return -ENOTSUP;
	}

	/* The user requested a specific size */
	if ((*size) && (*size > data->tx_buffer_size)) {
		/* Too big to fit */
		*size = data->tx_buffer_size;
		return -ENOMEM;
	}

	/*
	 * OpenAMP doesn't really have the concept of forever but instead it
	 * gives up after 15 seconds.  In that case, just keep retrying.
	 */
	do {
		payload = rpmsg_get_tx_payload_buffer(&rpmsg_ept->ep, size,
						      K_TIMEOUT_EQ(wait, K_FOREVER));
	} while ((!payload) && K_TIMEOUT_EQ(wait, K_FOREVER));

	/* This should really only be valid for K_NO_WAIT */
	if (!payload) {
		return -ENOBUFS;
	}

	(*r_data) = payload;

	return 0;
}

static int hold_rx_buffer(const struct device *instance, void *token,
			  void *data)
{
	struct ipc_rpmsg_ept *rpmsg_ept;

	rpmsg_ept = (struct ipc_rpmsg_ept *) token;

	/* Endpoint is not registered with instance */
	if (!rpmsg_ept) {
		return -ENOENT;
	}

	rpmsg_hold_rx_buffer(&rpmsg_ept->ep, data);

	return 0;
}

static int release_rx_buffer(const struct device *instance, void *token,
			     void *data)
{
	struct ipc_rpmsg_ept *rpmsg_ept;

	rpmsg_ept = (struct ipc_rpmsg_ept *) token;

	/* Endpoint is not registered with instance */
	if (!rpmsg_ept) {
		return -ENOENT;
	}

	rpmsg_release_rx_buffer(&rpmsg_ept->ep, data);

	return 0;
}

static int drop_tx_buffer(const struct device *instance, void *token,
			  const void *data)
{
	/* Not yet supported by OpenAMP */
	return -ENOTSUP;
}

const static struct ipc_service_backend backend_ops = {
	.open_instance = open,
	.close_instance = close,
	.register_endpoint = register_ept,
	.deregister_endpoint = deregister_ept,
	.send = send,
	.send_nocopy = send_nocopy,
	.drop_tx_buffer = drop_tx_buffer,
	.get_tx_buffer = get_tx_buffer,
	.get_tx_buffer_size = get_tx_buffer_size,
	.hold_rx_buffer = hold_rx_buffer,
	.release_rx_buffer = release_rx_buffer,
};

static int backend_init(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;

	data->role = conf->role;

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	__ASSERT((VDEV_STATUS_SIZE % sys_cache_data_line_size_get()) == 0U,
		  "VDEV status area must be aligned to the cache line");
	__ASSERT((MEM_ALIGNMENT % sys_cache_data_line_size_get()) == 0U,
		  "Static VRINGs must be aligned to the cache line");
	__ASSERT((conf->buffer_size % sys_cache_data_line_size_get()) == 0U,
		  "Buffers must be aligned to the cache line ");
#endif

	k_mutex_init(&data->rpmsg_inst.mtx);
	atomic_set(&data->state, STATE_READY);

	return 0;
}


#if defined(CONFIG_ARCH_POSIX)
#define BACKEND_PRE(i) extern char IPC##i##_shm_buffer[];
#define BACKEND_SHM_ADDR(i) (const uintptr_t)IPC##i##_shm_buffer
#else
#define BACKEND_PRE(i)
#define BACKEND_SHM_ADDR(i) DT_REG_ADDR(DT_INST_PHANDLE(i, memory_region))
#endif /* defined(CONFIG_ARCH_POSIX) */

#define DEFINE_BACKEND_DEVICE(i)							\
	BACKEND_PRE(i)									\
	static struct backend_config_t backend_config_##i = {				\
		.role = DT_ENUM_IDX_OR(DT_DRV_INST(i), role, ROLE_HOST),		\
		.shm_size = DT_REG_SIZE(DT_INST_PHANDLE(i, memory_region)),		\
		.shm_addr = BACKEND_SHM_ADDR(i),					\
		.mbox_tx = MBOX_DT_SPEC_INST_GET(i, tx),				\
		.mbox_rx = MBOX_DT_SPEC_INST_GET(i, rx),				\
		.wq_prio = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, zephyr_priority),	\
			   (DT_INST_PROP_BY_IDX(i, zephyr_priority, 0)),		\
			   (0)),							\
		.wq_prio_type = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, zephyr_priority),	\
			   (DT_INST_PROP_BY_IDX(i, zephyr_priority, 1)),		\
			   (PRIO_PREEMPT)),						\
		.buffer_size = DT_INST_PROP_OR(i, zephyr_buffer_size,			\
					       RPMSG_BUFFER_SIZE),			\
		.id = i,								\
	};										\
											\
	static struct backend_data_t backend_data_##i;					\
											\
	DEVICE_DT_INST_DEFINE(i,							\
			 &backend_init,							\
			 NULL,								\
			 &backend_data_##i,						\
			 &backend_config_##i,						\
			 POST_KERNEL,							\
			 CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY,			\
			 &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)

#define BACKEND_CONFIG_INIT(n) &backend_config_##n,

#if defined(CONFIG_IPC_SERVICE_BACKEND_RPMSG_SHMEM_RESET)
static int shared_memory_prepare(void)
{
	static const struct backend_config_t *config[] = {
		DT_INST_FOREACH_STATUS_OKAY(BACKEND_CONFIG_INIT)
	};

	for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
		if (config[i]->role == ROLE_HOST) {
			memset((void *) config[i]->shm_addr, 0, VDEV_STATUS_SIZE);
		}
	}

	return 0;
}

SYS_INIT(shared_memory_prepare, PRE_KERNEL_1, 1);
#endif /* CONFIG_IPC_SERVICE_BACKEND_RPMSG_SHMEM_RESET */
