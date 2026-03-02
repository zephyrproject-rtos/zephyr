/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/ipc/ipc_service_backend.h>

#include <zephyr/drivers/mbox.h>
#include <zephyr/dt-bindings/ipc_service/static_vrings.h>

#include "ipc_rpmsg_lite.h"

#define DT_DRV_COMPAT nxp_ipc_rpmsg_lite

#define NUM_INSTANCES DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

#define WQ_STACK_SIZE CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_WQ_STACK_SIZE

#define STATE_READY  (0)
#define STATE_BUSY   (1)
#define STATE_INITED (2)

#if defined(CONFIG_THREAD_MAX_NAME_LEN)
#define THREAD_MAX_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_MAX_NAME_LEN 1
#endif

/* MBOX Message Queue Buffer */
#define MBOX_MQ_ITEM_SIZE (sizeof(uint32_t)) /** Size of one Item in Message Queue */
#define MBOX_MQ_NO_ITEMS  (10)               /** Number of Items in Message Queue */

#define RPMSG_HEADER_SIZE 16U

K_THREAD_STACK_ARRAY_DEFINE(mbox_stack, NUM_INSTANCES, WQ_STACK_SIZE);

struct backend_data_t {
	/* IPC RPMSG-Lite Instance */
	struct ipc_rpmsg_lite_instance ipc_rpmsg_inst;

	/* General */
	unsigned int role;
	atomic_t state;

	/* TX buffer size */
	int tx_buffer_size;

	unsigned int link_id;

	/* MBOX work queue (per instance) */
	struct k_work mbox_work;
	struct k_work_q mbox_wq;

#if defined(CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_QUEUE)
	/* Queue-based notification */
	struct k_msgq inst_mq;
	char mq_buffer[MBOX_MQ_NO_ITEMS * MBOX_MQ_ITEM_SIZE];
#else
	/* Direct notification */
	uint32_t pending_vector_id;
#endif
};

struct backend_config_t {
	unsigned int role;
	unsigned int link_id;
	uintptr_t shm_addr;
	size_t shm_size;
	struct mbox_dt_spec mbox_tx;
	struct mbox_dt_spec mbox_rx;
	unsigned int wq_prio_type;
	unsigned int wq_prio;
	unsigned int id;
	unsigned int buffer_size;
	unsigned int buffer_count;
};

static const struct backend_config_t *g_inst_conf_ref[NUM_INSTANCES];
static struct backend_data_t *g_inst_data_ref[NUM_INSTANCES];

static void ipc_rpmsg_lite_destroy_ept(struct ipc_rpmsg_lite_ept *ept)
{
	rpmsg_lite_destroy_ept(ept->priv_data.priv_inst_ref, ept->ep);
}

static struct ipc_rpmsg_lite_ept *
get_ept_slot_with_name(struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst, const char *name)
{
	struct ipc_rpmsg_lite_ept *rpmsg_ept;

	for (size_t i = 0; i < NUM_ENDPOINTS; i++) {
		rpmsg_ept = &ipc_rpmsg_inst->endpoint[i];

		if (strcmp(name, rpmsg_ept->name) == 0) {
			return &ipc_rpmsg_inst->endpoint[i];
		}
	}

	return NULL;
}

static struct ipc_rpmsg_lite_ept *
get_available_ept_slot(struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst)
{
	return get_ept_slot_with_name(ipc_rpmsg_inst, "");
}

static bool check_endpoints_freed(struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst)
{
	struct ipc_rpmsg_lite_ept *rpmsg_ept;

	for (size_t i = 0; i < NUM_ENDPOINTS; i++) {
		rpmsg_ept = &ipc_rpmsg_inst->endpoint[i];

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
static bool get_ept(struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst,
		    struct ipc_rpmsg_lite_ept **ipc_rpmsg_ept, const char *name)
{
	struct ipc_rpmsg_lite_ept *ept;

	ept = get_ept_slot_with_name(ipc_rpmsg_inst, name);
	if (ept != NULL) {
		(*ipc_rpmsg_ept) = ept;
		return true;
	}

	ept = get_available_ept_slot(ipc_rpmsg_inst);
	if (ept != NULL) {
		(*ipc_rpmsg_ept) = ept;
		return false;
	}

	(*ipc_rpmsg_ept) = NULL;

	return false;
}

static void advertise_ept(struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst,
			  struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept, const char *name, uint32_t dest)
{
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
	ipc_rpmsg_ept->ep = rpmsg_lite_create_ept(ipc_rpmsg_inst->rpmsg_lite_inst, RL_ADDR_ANY,
						  ipc_rpmsg_inst->cb, ipc_rpmsg_ept,
						  &ipc_rpmsg_ept->ep_context);
#else
	ipc_rpmsg_ept->ep = rpmsg_lite_create_ept(ipc_rpmsg_inst->rpmsg_lite_inst, RL_ADDR_ANY,
						  ipc_rpmsg_inst->cb, ipc_rpmsg_ept);
#endif
	if (ipc_rpmsg_ept->ep == NULL) {
		return;
	}

	/* Announce endpoint creation */
	if (dest == RL_ADDR_ANY) {
		rpmsg_ns_announce(ipc_rpmsg_inst->rpmsg_lite_inst, ipc_rpmsg_ept->ep,
				  ipc_rpmsg_ept->name, RL_NS_CREATE);
	}

	ipc_rpmsg_ept->dest = dest;

	ipc_rpmsg_ept->bound = true;
	if (ipc_rpmsg_inst->bound_cb) {
		ipc_rpmsg_inst->bound_cb(ipc_rpmsg_ept);
	}
}

static void ns_bind_cb(uint32_t new_ept, const char *new_ept_name, uint32_t flags, void *user_data)
{
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst = user_data;
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;
	bool ept_cached;
	const char *name = new_ept_name;
	uint32_t dest = new_ept;

	if (ipc_rpmsg_inst == NULL) {
		return;
	}

	if (name == NULL || name[0] == '\0') {
		return;
	}

	k_mutex_lock(&ipc_rpmsg_inst->mtx, K_FOREVER);
	ept_cached = get_ept(ipc_rpmsg_inst, &ipc_rpmsg_ept, name);

	if (ipc_rpmsg_ept == NULL) {
		k_mutex_unlock(&ipc_rpmsg_inst->mtx);
		return;
	}

	if (ept_cached) {
		/*
		 * The endpoint was already registered by the HOST core. The
		 * endpoint can now be advertised to the REMOTE core.
		 */
		k_mutex_unlock(&ipc_rpmsg_inst->mtx);
		advertise_ept(ipc_rpmsg_inst, ipc_rpmsg_ept, name, dest);
	} else {
		/*
		 * The endpoint is not registered yet, this happens when the
		 * REMOTE core registers the endpoint before the HOST has
		 * had the chance to register it. Cache it saving name and
		 * destination address to be used by the next register_ept()
		 * call by the HOST core.
		 */
		strncpy(ipc_rpmsg_ept->name, name, sizeof(ipc_rpmsg_ept->name));
		ipc_rpmsg_ept->name[RPMSG_NAME_SIZE - 1] = '\0';
		ipc_rpmsg_ept->dest = dest;
		k_mutex_unlock(&ipc_rpmsg_inst->mtx);
	}
}

static void bound_cb(struct ipc_rpmsg_lite_ept *ept)
{
	struct ipc_rpmsg_lite_ept_priv *priv_data = &ept->priv_data;
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_lite_inst = priv_data->priv_inst_ref;

	rpmsg_lite_send(ipc_rpmsg_lite_inst->rpmsg_lite_inst, ept->ep, ept->dest, (char *)"", 0,
			RL_DONT_BLOCK);

	if (ept->cb && ept->cb->bound) {
		ept->cb->bound(ept->priv_data.priv);
	}
}

static int ept_cb(void *payload, uint32_t payload_len, uint32_t src, void *priv)
{
	struct ipc_rpmsg_lite_ept *ept;

	ept = (struct ipc_rpmsg_lite_ept *)priv;

	/* Reset hold flag and store buffer pointer */
	atomic_set(&ept->hold_last_buffer, 0);
	ept->last_rx_data = payload;

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
	if (payload_len == 0) {
		if (!ept->bound) {
			if (ept->dest == RL_ADDR_ANY) {
				ept->dest = src;
			}
			ept->bound = true;
			bound_cb(ept);
		}
		return RL_SUCCESS;
	}

	if (ept->cb && ept->cb->received) {
		ept->cb->received(payload, payload_len, ept->priv_data.priv);
	}

	/* Check if user called hold_rx_buffer during the callback */
	if (atomic_get(&ept->hold_last_buffer) == 1) {
		return RL_HOLD; /* User wants to hold this buffer */
	}

	return RL_SUCCESS;
}

/*****************************************************************************
 * RPMSG-Lite Platform Porting functions
 *
 * START
 ****************************************************************************/
void platform_notify(uint32_t vector_id)
{
	/*
	 * RPMSG-Lite multiplexes all IPC instances over a single MBOX channel.
	 * The vector_id payload encodes both the link_id (instance index) and
	 * the vq_id (virtqueue index) via RL_GET_LINK_ID() / RL_GET_Q_ID().
	 * Instance 0's MBOX TX channel is therefore used for all notifications
	 * regardless of which link_id triggered the call. The remote side
	 * demultiplexes using RL_GET_LINK_ID(vector_id) in mbox_callback().
	 */
	if (g_inst_conf_ref[0] == NULL || !g_inst_conf_ref[0]->mbox_tx.dev) {
		return;
	}

	struct mbox_msg msg = {0};

	msg.data = &vector_id;
	msg.size = sizeof(vector_id);

	mbox_send_dt(&g_inst_conf_ref[0]->mbox_tx, &msg);
}

int32_t platform_init_interrupt(uint32_t vector_id, void *isr_data)
{
	/* Register ISR to environment layer */
	env_register_isr(vector_id, isr_data);

	return 0;
}

int32_t platform_deinit_interrupt(uint32_t vector_id)
{
	/* Unregister ISR from environment layer */
	env_unregister_isr(vector_id);

	return 0;
}

int32_t platform_init(void)
{
	/* Not used */
	return 0;
}

int32_t platform_deinit(void)
{
	/* Not used */
	return 0;
}

uintptr_t platform_vatopa(void *addr)
{
	return ((uintptr_t)(char *)addr);
}

void *platform_patova(uintptr_t addr)
{
	return ((void *)(char *)addr);
}

int32_t platform_interrupt_enable(uint32_t vector_id)
{
	return (int32_t)vector_id;
}

int32_t platform_interrupt_disable(uint32_t vector_id)
{
	return (int32_t)vector_id;
}
/*****************************************************************************
 * RPMSG-Lite Platform Porting functions
 *
 * END
 ****************************************************************************/

static void mbox_callback_process(struct k_work *item)
{
	struct backend_data_t *data;
	struct virtqueue *vq;
	uint32_t msg_data;

	data = CONTAINER_OF(item, struct backend_data_t, mbox_work);

#if defined(CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_QUEUE)
	/* Queue-based: Process all pending messages */
	while (k_msgq_get(&data->inst_mq, &msg_data, K_NO_WAIT) == 0) {
		uint32_t vq_id = RL_GET_Q_ID(msg_data);

		/*
		 * RPMSG-Lite VQ numbering: vq_id 0 is used by the HOST (master)
		 * for TX notifications, vq_id 1 is used by the REMOTE (slave).
		 * Each side's receive queue is the peer's send queue, so the
		 * HOST processes rvq on vq_id 0 and the REMOTE on vq_id 1.
		 */
		if (data->role == ROLE_HOST) {
			vq = (vq_id == 0) ? data->ipc_rpmsg_inst.rpmsg_lite_inst->rvq
					  : data->ipc_rpmsg_inst.rpmsg_lite_inst->tvq;
		} else {
			vq = (vq_id == 1) ? data->ipc_rpmsg_inst.rpmsg_lite_inst->rvq
					  : data->ipc_rpmsg_inst.rpmsg_lite_inst->tvq;
		}

		virtqueue_notification(vq);
	}
#else
	/* Direct: Process the latest notification */
	msg_data = data->pending_vector_id;

	uint32_t vq_id = RL_GET_Q_ID(msg_data);

	/*
	 * RPMSG-Lite VQ numbering: vq_id 0 is used by the HOST (master)
	 * for TX notifications, vq_id 1 is used by the REMOTE (slave).
	 * Each side's receive queue is the peer's send queue, so the
	 * HOST processes rvq on vq_id 0 and the REMOTE on vq_id 1.
	 */
	if (data->role == ROLE_HOST) {
		vq = (vq_id == 0) ? data->ipc_rpmsg_inst.rpmsg_lite_inst->rvq
				  : data->ipc_rpmsg_inst.rpmsg_lite_inst->tvq;
	} else {
		vq = (vq_id == 1) ? data->ipc_rpmsg_inst.rpmsg_lite_inst->rvq
				  : data->ipc_rpmsg_inst.rpmsg_lite_inst->tvq;
	}

	virtqueue_notification(vq);
#endif
}

static void mbox_callback(const struct device *instance, uint32_t channel, void *user_data,
			  struct mbox_msg *msg_data)
{
	if (msg_data == NULL || msg_data->size < sizeof(uint32_t)) {
		return;
	}

	uint32_t vector_id = *(const uint32_t *)msg_data->data;
	uint32_t link_id = RL_GET_LINK_ID(vector_id);

	if (link_id >= NUM_INSTANCES) {
		return;
	}

	struct backend_data_t *data = g_inst_data_ref[link_id];

	if (data == NULL) {
		return;
	}

#if defined(CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_QUEUE)
	/* Queue-based: Put message in queue for ordered processing */
	if (k_msgq_put(&data->inst_mq, &vector_id, K_NO_WAIT) != 0) {
		/* Queue full - message lost */
		return;
	}
#else
	/* Direct: Store latest notification (may overwrite previous) */
	data->pending_vector_id = vector_id;
#endif

	k_work_submit_to_queue(&data->mbox_wq, &data->mbox_work);
}

static int mbox_init(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	struct k_work_queue_config wq_cfg = {.name = instance->name};
	int prio, err;

	prio = (conf->wq_prio_type == PRIO_COOP) ? K_PRIO_COOP(conf->wq_prio)
						 : K_PRIO_PREEMPT(conf->wq_prio);

	k_work_queue_init(&data->mbox_wq);
	k_work_queue_start(&data->mbox_wq, mbox_stack[conf->id], WQ_STACK_SIZE, prio, &wq_cfg);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		char name[THREAD_MAX_NAME_LEN];

		snprintk(name, sizeof(name), "mbox_wq #%d", conf->id);
		k_thread_name_set(&data->mbox_wq.thread, name);
	}

	/* Initialize per-instance work item */
	k_work_init(&data->mbox_work, mbox_callback_process);

#if defined(CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_QUEUE)
	/* Initialize per-instance message queue */
	k_msgq_init(&data->inst_mq, data->mq_buffer, sizeof(uint32_t), MBOX_MQ_NO_ITEMS);
#endif

	/*
	 * RPMSG-Lite uses a single shared MBOX channel for all IPC instances.
	 * The vector_id in the MBOX data payload encodes the link_id (instance)
	 * and vq_id, allowing mbox_callback() to dispatch to the correct
	 * per-instance work queue. Only instance 0 registers the single RX
	 * callback; all other instances rely on this shared interrupt path.
	 */
	if (conf->link_id == 0) {
		err = mbox_register_callback_dt(&conf->mbox_rx, mbox_callback, data);
		if (err != 0) {
			return err;
		}

		err = mbox_set_enabled_dt(&conf->mbox_rx, 1);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static int mbox_deinit(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	k_tid_t wq_thread;
	int err = 0;

	/*
	 * Only instance 0 owns the shared RX MBOX channel registration,
	 * so only it needs to disable the channel on teardown.
	 */
	if (conf->link_id == 0) {
		err = mbox_set_enabled_dt(&conf->mbox_rx, 0);
		if (err != 0) {
			return err;
		}
	}

	/* Drain and stop per-instance work queue */
	k_work_queue_drain(&data->mbox_wq, 1);

	wq_thread = k_work_queue_thread_get(&data->mbox_wq);
	k_thread_abort(wq_thread);

#if defined(CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_QUEUE)
	/* Clean up per-instance message queue */
	k_msgq_purge(&data->inst_mq);
	k_msgq_cleanup(&data->inst_mq);
#endif

	return 0;
}

static struct ipc_rpmsg_lite_ept *
register_ept_on_host(struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst, const struct ipc_ept_cfg *cfg)
{
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;
	bool ept_cached;

	k_mutex_lock(&ipc_rpmsg_inst->mtx, K_FOREVER);

	ept_cached = get_ept(ipc_rpmsg_inst, &ipc_rpmsg_ept, cfg->name);
	if (ipc_rpmsg_ept == NULL) {
		k_mutex_unlock(&ipc_rpmsg_inst->mtx);
		return NULL;
	}

	ipc_rpmsg_ept->cb = &cfg->cb;
	ipc_rpmsg_ept->priv_data.priv = cfg->priv;
	ipc_rpmsg_ept->priv_data.priv_inst_ref = ipc_rpmsg_inst;
	ipc_rpmsg_ept->bound = false;
	ipc_rpmsg_ept->ep_priv = ipc_rpmsg_ept;

	if (ept_cached) {
		/*
		 * The endpoint was cached in the NS bind callback. We can finally
		 * advertise it.
		 */
		k_mutex_unlock(&ipc_rpmsg_inst->mtx);
		advertise_ept(ipc_rpmsg_inst, ipc_rpmsg_ept, cfg->name, ipc_rpmsg_ept->dest);
	} else {
		/*
		 * There is no endpoint in the cache because the REMOTE has
		 * not registered the endpoint yet. Cache it.
		 */
		strncpy(ipc_rpmsg_ept->name, cfg->name, sizeof(ipc_rpmsg_ept->name));
		ipc_rpmsg_ept->name[RPMSG_NAME_SIZE - 1] = '\0';
		k_mutex_unlock(&ipc_rpmsg_inst->mtx);
	}

	return ipc_rpmsg_ept;
}

static struct ipc_rpmsg_lite_ept *
register_ept_on_remote(struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst,
		       const struct ipc_ept_cfg *cfg)
{
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;

	ipc_rpmsg_ept = get_available_ept_slot(ipc_rpmsg_inst);
	if (ipc_rpmsg_ept == NULL) {
		return NULL;
	}

	ipc_rpmsg_ept->cb = &cfg->cb;
	ipc_rpmsg_ept->priv_data.priv = cfg->priv;
	ipc_rpmsg_ept->priv_data.priv_inst_ref = ipc_rpmsg_inst;
	ipc_rpmsg_ept->bound = false;
	ipc_rpmsg_ept->ep_priv = ipc_rpmsg_ept;
	ipc_rpmsg_ept->dest = RL_ADDR_ANY;

	strncpy(ipc_rpmsg_ept->name, cfg->name, sizeof(ipc_rpmsg_ept->name));
	ipc_rpmsg_ept->name[RPMSG_NAME_SIZE - 1] = '\0';

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
	ipc_rpmsg_ept->ep = rpmsg_lite_create_ept(ipc_rpmsg_inst->rpmsg_lite_inst, RL_ADDR_ANY,
						  ipc_rpmsg_inst->cb, ipc_rpmsg_ept,
						  &ipc_rpmsg_ept->ep_context);
#else
	ipc_rpmsg_ept->ep = rpmsg_lite_create_ept(ipc_rpmsg_inst->rpmsg_lite_inst, RL_ADDR_ANY,
						  ipc_rpmsg_inst->cb, ipc_rpmsg_ept);
#endif
	if (ipc_rpmsg_ept->ep == NULL) {
		return NULL;
	}

	/* Announce endpoint creation */
	rpmsg_ns_announce(ipc_rpmsg_inst->rpmsg_lite_inst, ipc_rpmsg_ept->ep, ipc_rpmsg_ept->name,
			  RL_NS_CREATE);

	return ipc_rpmsg_ept;
}

static int register_ept(const struct device *instance, void **token, const struct ipc_ept_cfg *cfg)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_lite_instance *rpmsg_inst;
	struct ipc_rpmsg_lite_ept *rpmsg_ept;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	/* Empty name is not valid */
	if (cfg->name == NULL || cfg->name[0] == '\0') {
		return -EINVAL;
	}

	rpmsg_inst = &data->ipc_rpmsg_inst;

	rpmsg_ept = (data->role == ROLE_HOST) ? register_ept_on_host(rpmsg_inst, cfg)
					      : register_ept_on_remote(rpmsg_inst, cfg);
	if (rpmsg_ept == NULL) {
		return -EINVAL;
	}

	(*token) = rpmsg_ept;

	return 0;
}

static int deregister_ept(const struct device *instance, void *token)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;
	struct k_work_sync sync;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	ipc_rpmsg_ept = (struct ipc_rpmsg_lite_ept *)token;

	/* Endpoint is not registered with instance */
	if (!ipc_rpmsg_ept) {
		return -ENOENT;
	}

	/* Drain pending work items for THIS instance before tearing down channel */
	k_work_flush(&data->mbox_work, &sync);

	ipc_rpmsg_lite_destroy_ept(ipc_rpmsg_ept);

	memset(ipc_rpmsg_ept, 0, sizeof(struct ipc_rpmsg_lite_ept));

	return 0;
}

static int send(const struct device *instance, void *token, const void *msg, size_t len)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;
	int ret;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	/* Empty message is not allowed */
	if (len == 0) {
		return -EBADMSG;
	}

	ipc_rpmsg_ept = (struct ipc_rpmsg_lite_ept *)token;

	/* Endpoint is not registered with instance */
	if (!ipc_rpmsg_ept) {
		return -ENOENT;
	}

	struct ipc_rpmsg_lite_ept_priv *priv_data = &ipc_rpmsg_ept->priv_data;
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_lite_inst = priv_data->priv_inst_ref;

	ret = rpmsg_lite_send(ipc_rpmsg_lite_inst->rpmsg_lite_inst, ipc_rpmsg_ept->ep,
			      ipc_rpmsg_ept->dest, (char *)msg, len, RL_DONT_BLOCK);

	/* No buffers available */
	if (ret != RL_SUCCESS) {
		return -ENOMEM;
	}

	return ret;
}

static int send_nocopy(const struct device *instance, void *token, const void *msg, size_t len)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;
	void *tx_buf;

	/* Instance is not ready */
	if (atomic_get(&data->state) != STATE_INITED) {
		return -EBUSY;
	}

	/* Empty message is not allowed */
	if (len == 0) {
		return -EBADMSG;
	}

	ipc_rpmsg_ept = (struct ipc_rpmsg_lite_ept *)token;

	/* Endpoint is not registered with instance */
	if (!ipc_rpmsg_ept) {
		return -ENOENT;
	}

	struct ipc_rpmsg_lite_ept_priv *priv_data = &ipc_rpmsg_ept->priv_data;
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_lite_inst = priv_data->priv_inst_ref;

	tx_buf = (void *)(uintptr_t)msg;

	return rpmsg_lite_send_nocopy(ipc_rpmsg_lite_inst->rpmsg_lite_inst, ipc_rpmsg_ept->ep,
				      ipc_rpmsg_ept->dest, tx_buf, len);
}

static int open(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst;
	int err;

	if (!atomic_cas(&data->state, STATE_READY, STATE_BUSY)) {
		return -EALREADY;
	}

	err = mbox_init(instance);
	if (err != 0) {
		goto error;
	}

	ipc_rpmsg_inst = &data->ipc_rpmsg_inst;

	ipc_rpmsg_inst->bound_cb = bound_cb;
	ipc_rpmsg_inst->cb = ept_cb;

	if (conf->role == ROLE_HOST) {
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
		ipc_rpmsg_inst->rpmsg_lite_inst = rpmsg_lite_master_init(
			(void *)conf->shm_addr, conf->shm_size, conf->link_id, RL_NO_FLAGS,
			&ipc_rpmsg_inst->rpmsg_lite_context);
#elif defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
		ipc_rpmsg_inst->rpmsg_lite_inst = rpmsg_lite_master_init(
			(void *)conf->shm_addr, conf->link_id, RL_NO_FLAGS, NULL);
#else
		ipc_rpmsg_inst->rpmsg_lite_inst =
			rpmsg_lite_master_init((void *)conf->shm_addr, conf->link_id, RL_NO_FLAGS);
#endif
	} else {
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
		ipc_rpmsg_inst->rpmsg_lite_inst =
			rpmsg_lite_remote_init((void *)conf->shm_addr, conf->link_id, RL_NO_FLAGS,
					       &ipc_rpmsg_inst->rpmsg_lite_context);
#elif defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
		ipc_rpmsg_inst->rpmsg_lite_inst = rpmsg_lite_remote_init(
			(void *)conf->shm_addr, conf->link_id, RL_NO_FLAGS, NULL);
#else
		ipc_rpmsg_inst->rpmsg_lite_inst =
			rpmsg_lite_remote_init((void *)conf->shm_addr, conf->link_id, RL_NO_FLAGS);
#endif
		rpmsg_lite_wait_for_link_up(ipc_rpmsg_inst->rpmsg_lite_inst, RL_BLOCK);
	}

	if (ipc_rpmsg_inst->rpmsg_lite_inst == NULL) {
		err = -EINVAL;
		goto error;
	}

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
	ipc_rpmsg_inst->ns_handle =
		rpmsg_ns_bind(ipc_rpmsg_inst->rpmsg_lite_inst, ns_bind_cb, ipc_rpmsg_inst,
			      &ipc_rpmsg_inst->rpmsg_lite_ns_context);
#else
	ipc_rpmsg_inst->ns_handle =
		rpmsg_ns_bind(ipc_rpmsg_inst->rpmsg_lite_inst, ns_bind_cb, ipc_rpmsg_inst);
#endif /* RL_USE_STATIC_API */

	if (ipc_rpmsg_inst->ns_handle == NULL) {
		err = -EINVAL;
		goto error;
	}

#if defined(RL_ALLOW_CUSTOM_SHMEM_CONFIG) && (RL_ALLOW_CUSTOM_SHMEM_CONFIG == 1)
	/* User specified total buffer size in DT, subtract header for payload */
	if (conf->buffer_size <= RPMSG_HEADER_SIZE) {
		err = -EINVAL;
		goto error;
	}
	data->tx_buffer_size = conf->buffer_size - RPMSG_HEADER_SIZE;
#else
	/* Use compile-time default payload size */
	data->tx_buffer_size = RL_BUFFER_PAYLOAD_SIZE;
#endif

	atomic_set(&data->state, STATE_INITED);
	return 0;

error:
	/* Back to the ready state */
	atomic_set(&data->state, STATE_READY);
	return err;
}

static int close(const struct device *instance)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_inst;
	int err;

	if (!atomic_cas(&data->state, STATE_INITED, STATE_BUSY)) {
		return -EALREADY;
	}

	ipc_rpmsg_inst = &data->ipc_rpmsg_inst;

	if (!check_endpoints_freed(ipc_rpmsg_inst)) {
		/* Restore state — endpoints still active, cannot close */
		atomic_set(&data->state, STATE_INITED);
		return -EBUSY;
	}

	err = rpmsg_lite_deinit(ipc_rpmsg_inst->rpmsg_lite_inst);
	if (err != 0) {
		goto error;
	}

	err = mbox_deinit(instance);
	if (err != 0) {
		goto error;
	}

	memset(ipc_rpmsg_inst, 0, sizeof(struct ipc_rpmsg_lite_instance));

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

static int get_tx_buffer(const struct device *instance, void *token, void **r_data, uint32_t *size,
			 k_timeout_t wait)
{
	struct backend_data_t *data = instance->data;
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;
	void *payload;

	ipc_rpmsg_ept = (struct ipc_rpmsg_lite_ept *)token;

	/* Endpoint is not registered with instance */
	if (!ipc_rpmsg_ept) {
		return -ENOENT;
	}

	if (!r_data || !size) {
		return -EINVAL;
	}

	uint32_t wait_time;

	if (K_TIMEOUT_EQ(wait, K_FOREVER)) {
		wait_time = RL_BLOCK;
	} else if (K_TIMEOUT_EQ(wait, K_NO_WAIT)) {
		wait_time = RL_DONT_BLOCK;
	} else {
		/* Convert Zephyr ticks to milliseconds for RPMSG-Lite.
		 * k_ticks_to_ms_floor32() may round sub-millisecond timeouts
		 * to 0, which RPMSG-Lite treats as RL_DONT_BLOCK; clamp to
		 * 1ms so any positive finite timeout is honoured.
		 */
		wait_time = k_ticks_to_ms_floor32(wait.ticks);
		if (wait_time == 0) {
			wait_time = 1U;
		}
	}

	/* The user requested a specific size */
	if ((*size) && (*size > data->tx_buffer_size)) {
		/* Too big to fit */
		*size = data->tx_buffer_size;
		return -ENOMEM;
	}

	struct ipc_rpmsg_lite_ept_priv *priv_data = &ipc_rpmsg_ept->priv_data;
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_lite_inst = priv_data->priv_inst_ref;

	payload = rpmsg_lite_alloc_tx_buffer(ipc_rpmsg_lite_inst->rpmsg_lite_inst, size, wait_time);

	/* This should really only be valid for K_NO_WAIT */
	if (!payload) {
		return -ENOBUFS;
	}

	(*r_data) = payload;

	return 0;
}

static int hold_rx_buffer(const struct device *instance, void *token, void *data)
{
	struct ipc_rpmsg_lite_ept *ept = (struct ipc_rpmsg_lite_ept *)token;

	/* Endpoint is not registered with instance */
	if (!ept) {
		return -ENOENT;
	}

	/* Verify this is the buffer currently being received */
	if (ept->last_rx_data != data) {
		return -EINVAL; /* Not the current buffer */
	}

	/* Set the flag - callback will return RL_HOLD */
	atomic_set(&ept->hold_last_buffer, 1);

	return 0;
}

static int release_rx_buffer(const struct device *instance, void *token, void *data)
{
	struct ipc_rpmsg_lite_ept *ipc_rpmsg_ept;

	ipc_rpmsg_ept = (struct ipc_rpmsg_lite_ept *)token;

	/* Endpoint is not registered with instance */
	if (!ipc_rpmsg_ept) {
		return -ENOENT;
	}

	struct ipc_rpmsg_lite_ept_priv *priv_data = &ipc_rpmsg_ept->priv_data;
	struct ipc_rpmsg_lite_instance *ipc_rpmsg_lite_inst = priv_data->priv_inst_ref;

	int ret = rpmsg_lite_release_rx_buffer(ipc_rpmsg_lite_inst->rpmsg_lite_inst, data);

	return (ret == RL_SUCCESS) ? 0 : -EIO;
}

static int drop_tx_buffer(const struct device *instance, void *token, const void *data)
{
	/* Not supported by RPMSG-Lite */
	return -ENOTSUP;
}

#if defined(RL_ALLOW_CUSTOM_SHMEM_CONFIG) && (RL_ALLOW_CUSTOM_SHMEM_CONFIG == 1)
uint32_t platform_get_custom_shmem_config(uint32_t link_id, rpmsg_platform_shmem_config_t *config)
{
	if (config == NULL) {
		return RL_ERR_PARAM;
	}

	if (link_id >= NUM_INSTANCES) {
		return RL_ERR_PARAM;
	}

	const struct backend_config_t *conf = g_inst_conf_ref[link_id];

	if (conf == NULL) {
		/* Use defaults if not initialized */
		config->buffer_payload_size = RL_BUFFER_PAYLOAD_SIZE;
		config->buffer_count = RL_BUFFER_COUNT;
		config->vring_size = VRING_SIZE;
		config->vring_align = VRING_ALIGN;
	} else {
		/* Use device tree config - RPMSG-Lite will validate */
		config->buffer_payload_size = conf->buffer_size - RPMSG_HEADER_SIZE;
		config->buffer_count = conf->buffer_count;
		config->vring_size = vring_size(conf->buffer_count, VRING_ALIGN);
		config->vring_align = VRING_ALIGN;
	}

	return RL_SUCCESS;
}

#endif /* RL_ALLOW_CUSTOM_SHMEM_CONFIG */

static const struct ipc_service_backend backend_ops = {
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

	__ASSERT(conf->link_id < NUM_INSTANCES, "link_id %u must be less than NUM_INSTANCES %d",
		 conf->link_id, NUM_INSTANCES);

	if (conf->link_id >= NUM_INSTANCES) {
		return -EINVAL;
	}
	g_inst_conf_ref[conf->link_id] = conf;
	g_inst_data_ref[conf->link_id] = data;

	data->role = conf->role;

	data->link_id = conf->link_id;

#if defined(CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_DIRECT)
	/* Initialize pending_vector_id for direct mode */
	data->pending_vector_id = 0;
#endif

	k_mutex_init(&data->ipc_rpmsg_inst.mtx);
	atomic_set(&data->state, STATE_READY);

	return 0;
}

#define BACKEND_PRE(i)
#define BACKEND_SHM_ADDR(i) DT_REG_ADDR(DT_INST_PHANDLE(i, memory_region))

#define DEFINE_BACKEND_DEVICE(i)                                                                   \
	BACKEND_PRE(i)                                                                             \
	static struct backend_config_t backend_config_##i = {                                      \
		.role = DT_ENUM_IDX_OR(DT_DRV_INST(i), role, ROLE_HOST),                           \
		.link_id = DT_INST_PROP_OR(i, link_id, i),                                         \
		.shm_size = DT_REG_SIZE(DT_INST_PHANDLE(i, memory_region)),                        \
		.shm_addr = BACKEND_SHM_ADDR(i),                                                   \
		.mbox_tx = MBOX_DT_SPEC_INST_GET(i, tx),                                           \
		.mbox_rx = MBOX_DT_SPEC_INST_GET(i, rx),                                           \
		.wq_prio = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, zephyr_priority),	\
			   (DT_INST_PROP_BY_IDX(i, zephyr_priority, 0)),		\
			   (0)),                           \
			 .wq_prio_type = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, zephyr_priority),	\
			   (DT_INST_PROP_BY_IDX(i, zephyr_priority, 1)),		\
			   (PRIO_PREEMPT)),       \
				  .buffer_size = DT_INST_PROP_OR(i, zephyr_buffer_size,            \
								 RL_BUFFER_PAYLOAD_SIZE +          \
									 RPMSG_HEADER_SIZE),       \
				  .buffer_count = DT_INST_PROP_OR(i, zephyr_buffer_count,          \
								  RL_BUFFER_COUNT),                \
				  .id = i,                                                         \
	};                                                                                         \
                                                                                                   \
	static struct backend_data_t backend_data_##i;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, &backend_init, NULL, &backend_data_##i, &backend_config_##i,      \
			      POST_KERNEL, CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY, &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)
