/*
 * Copyright (c) 2020-2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ipc/ipc_service_backend.h>
#include <ipc/rpmsg_multi_instance.h>

#include <logging/log.h>
#include <sys/util_macro.h>
#include <zephyr.h>
#include <device.h>

LOG_MODULE_REGISTER(ipc_rpmsg_multi_instance, CONFIG_IPC_SERVICE_LOG_LEVEL);

#define NUM_INSTANCES		CONFIG_RPMSG_MULTI_INSTANCES_NO
#define NUM_ENDPOINTS		CONFIG_IPC_BACKEND_RPMSG_MI_NUM_ENDPOINTS_PER_INSTANCE

#define WORK_QUEUE_STACK_SIZE	CONFIG_IPC_BACKEND_RPMSG_MI_WORK_QUEUE_STACK_SIZE

#define PRIO_INIT_VAL		INT_MAX
#define INSTANCE_NAME_SIZE	16
#define IPM_MSG_ID		0

#define CH_NAME(idx, sub)	(CONFIG_RPMSG_MULTI_INSTANCE_ ## idx ## _IPM_ ## sub ## _NAME)

static char *ipm_rx_name[] = {
	FOR_EACH_FIXED_ARG(CH_NAME, (,), RX, 0, 1, 2, 3, 4, 5, 6, 7),
};

static char *ipm_tx_name[] = {
	FOR_EACH_FIXED_ARG(CH_NAME, (,), TX, 0, 1, 2, 3, 4, 5, 6, 7),
};

BUILD_ASSERT(ARRAY_SIZE(ipm_rx_name) >= NUM_INSTANCES, "Invalid configuration");
BUILD_ASSERT(ARRAY_SIZE(ipm_tx_name) >= NUM_INSTANCES, "Invalid configuration");

K_THREAD_STACK_ARRAY_DEFINE(ipm_stack, NUM_INSTANCES, WORK_QUEUE_STACK_SIZE);

struct ipc_ept {
	struct rpmsg_mi_ept rpmsg_ep;
	struct ipc_service_cb cb;
	const char *name;
	void *priv;
};

struct ipc_rpmsg_mi_instances {
	struct ipc_ept endpoints[NUM_ENDPOINTS];
	char name[INSTANCE_NAME_SIZE];
	struct rpmsg_mi_ctx ctx;
	bool is_initialized;
	int prio;
};

static struct ipc_rpmsg_mi_instances instances[NUM_INSTANCES];

static struct rpmsg_mi_ctx_shm_cfg shm = {
	.addr   = SHM_START_ADDR,
	.size   = SHM_SIZE,
};

static void common_bound_cb(void *priv)
{
	struct ipc_ept *ept = (struct ipc_ept *)priv;

	if (ept->cb.bound) {
		ept->cb.bound(ept->priv);
	}
}

static void common_recv_cb(const void *data, size_t len, void *priv)
{
	struct ipc_ept *ept = (struct ipc_ept *)priv;

	if (ept->cb.received) {
		ept->cb.received(data, len, ept->priv);
	}
}

static struct rpmsg_mi_cb cb = {
	.bound = common_bound_cb,
	.received = common_recv_cb,
};

static int send(struct ipc_ept *ept, const void *data, size_t len)
{
	return rpmsg_mi_send(&ept->rpmsg_ep, data, len);
}

static int get_available_instance(const struct ipc_ept_cfg *cfg)
{
	/* Endpoints with the same priority are
	 * registered to the same instance.
	 */
	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		if (instances[i].prio == cfg->prio || instances[i].prio == PRIO_INIT_VAL) {
			return (int)i;
		}
	}
	return -ENOMEM;
}

static int get_available_ept_slot(struct ipc_rpmsg_mi_instances *instance)
{
	for (size_t i = 0; i < NUM_ENDPOINTS; i++) {
		if (!(instance->endpoints[i].name)) {
			return (int)i;
		}
	}
	return -ENOMEM;
}

static int register_ept(struct ipc_ept **ept, const struct ipc_ept_cfg *cfg)
{
	struct rpmsg_mi_ept_cfg ept_cfg = { 0 };
	int i_idx, e_idx;

	if (!cfg || !ept) {
		return -EINVAL;
	}

	i_idx = get_available_instance(cfg);
	if (i_idx < 0) {
		LOG_ERR("Available instance not found");
		return -EIO;
	}

	/* Initialization of the instance context is performed only once.
	 * When registering the first endpoint for the instance.
	 */
	if (!instances[i_idx].is_initialized) {
		struct rpmsg_mi_ctx_cfg ctx_cfg = { 0 };

		snprintf(instances[i_idx].name, INSTANCE_NAME_SIZE, "rpmsg_mi_%d", i_idx);

		ctx_cfg.name = instances[i_idx].name;
		ctx_cfg.ipm_stack_area = ipm_stack[i_idx];
		ctx_cfg.ipm_stack_size = K_THREAD_STACK_SIZEOF(ipm_stack[i_idx]);
		ctx_cfg.ipm_work_q_prio = cfg->prio;
		ctx_cfg.ipm_thread_name = instances[i_idx].name;
		ctx_cfg.ipm_rx_name = ipm_rx_name[i_idx];
		ctx_cfg.ipm_tx_name = ipm_tx_name[i_idx];
		ctx_cfg.ipm_tx_id = IPM_MSG_ID;

		ctx_cfg.shm = &shm;

		if (rpmsg_mi_ctx_init(&instances[i_idx].ctx, &ctx_cfg) < 0) {
			LOG_ERR("Instance initialization failed");
			return -EIO;
		}

		instances[i_idx].is_initialized = true;
	}

	e_idx = get_available_ept_slot(&instances[i_idx]);
	if (e_idx < 0) {
		LOG_ERR("No free slots to register endpoint %s", log_strdup(cfg->name));
		return -EIO;
	}

	instances[i_idx].endpoints[e_idx].priv = cfg->priv;
	instances[i_idx].endpoints[e_idx].cb = cfg->cb;

	ept_cfg.cb = &cb;
	ept_cfg.priv = &instances[i_idx].endpoints[e_idx];
	ept_cfg.name = cfg->name;

	if (rpmsg_mi_ept_register(&instances[i_idx].ctx,
				  &instances[i_idx].endpoints[e_idx].rpmsg_ep, &ept_cfg) < 0) {
		LOG_ERR("Register endpoint failed");
		return -EIO;
	}

	instances[i_idx].endpoints[e_idx].name = cfg->name;
	instances[i_idx].prio = cfg->prio;

	*ept = &instances[i_idx].endpoints[e_idx];

	return 0;
}

const static struct ipc_service_backend backend = {
	.name = "RPMsg multi-instace backend",
	.send = send,
	.register_endpoint = register_ept,
};

static int backend_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		instances[i].prio = PRIO_INIT_VAL;
	}

	return ipc_service_register_backend(&backend);
}

SYS_INIT(backend_init, POST_KERNEL, CONFIG_IPC_SERVICE_BACKEND_REG_PRIORITY);
