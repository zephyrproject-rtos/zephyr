/*
 * Copyright (c) 2020-2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ipc/ipc_service_backend.h>
#include <ipc/rpmsg_multi_instance.h>

#include <logging/log.h>
#include <zephyr.h>
#include <device.h>

LOG_MODULE_REGISTER(ipc_rpmsg_multi_instance, CONFIG_IPC_SERVICE_LOG_LEVEL);

#define MASTER IS_ENABLED(CONFIG_RPMSG_MULTI_INSTANCE_MASTER)

#define NUM_INSTANCES  CONFIG_RPMSG_MULTI_INSTANCES_NO
#define NUM_ENDPOINTS  CONFIG_IPC_BACKEND_RPMSG_MI_NUM_ENDPOINTS_PER_INSTANCE

#define WORK_QUEUE_STACK_SIZE  CONFIG_IPC_BACKEND_RPMSG_MI_WORK_QUEUE_STACK_SIZE

#define PRIO_INIT_VAL          INT_MAX
#define INSTANCE_NAME_SIZE     16

K_THREAD_STACK_ARRAY_DEFINE(ipm_stack, NUM_INSTANCES, WORK_QUEUE_STACK_SIZE);
#define CAT_4(a, b, c, d) (a##b##c##d)

#define IPM_CH_NAME(name, sub) char *name[] = {                         \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_1_IPM_, sub, _NAME), \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_2_IPM_, sub, _NAME), \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_3_IPM_, sub, _NAME), \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_4_IPM_, sub, _NAME), \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_5_IPM_, sub, _NAME), \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_6_IPM_, sub, _NAME), \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_7_IPM_, sub, _NAME), \
		CAT_4(CONFIG, _RPMSG_MULTI_INSTANCE_8_IPM_, sub, _NAME), }

IPM_CH_NAME(ipm_rx_name, RX);
IPM_CH_NAME(ipm_tx_name, TX);

BUILD_ASSERT(ARRAY_SIZE(ipm_rx_name) >= NUM_INSTANCES, "Invalid configuration");
BUILD_ASSERT(ARRAY_SIZE(ipm_tx_name) >= NUM_INSTANCES, "Invalid configuration");

/* Internal definition of the structure ipc_ept*/
struct ipc_ept {
	const char *name;
	struct rpmsg_mi_ept rpmsg_ep;
	struct ipc_service_cb cb;
	void *priv;
};

struct ipc_rpmsg_mi_instances {
	char name[INSTANCE_NAME_SIZE];
	struct rpmsg_mi_ctx ctx;
	struct ipc_ept endpoints[NUM_ENDPOINTS];
	int prio;
	bool is_initialized;
};

static struct ipc_rpmsg_mi_instances instances[NUM_INSTANCES];

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
	if (!cfg || !ept) {
		return -EINVAL;
	}

	int i = get_available_instance(cfg);
	int e_idx = get_available_ept_slot(&instances[i]);

	if (i < 0) {
		LOG_ERR("Available instance not found");
		return -EIO;
	}

	/** Initialization of the instance context is performed only once.
	 *  When registering the first endpoint for the instance.
	 */
	if (!instances[i].is_initialized) {
		struct rpsmg_mi_ctx_cfg ctx_cfg = {0};

		snprintf(instances[i].name, INSTANCE_NAME_SIZE, "rpmsg_mi_%d", i);
		ctx_cfg.name = instances[i].name;
		ctx_cfg.ipm_stack_area = ipm_stack[i];
		ctx_cfg.ipm_stack_size = K_THREAD_STACK_SIZEOF(ipm_stack[i]);
		ctx_cfg.ipm_work_q_prio = cfg->prio;
		ctx_cfg.ipm_thread_name = instances[i].name;
		ctx_cfg.ipm_rx_name = ipm_rx_name[i];
		ctx_cfg.ipm_tx_name = ipm_tx_name[i];

		if (rpmsg_mi_ctx_init(&instances[i].ctx, &ctx_cfg) < 0) {
			LOG_ERR("Instance initialization failed");
			return -EIO;
		}
		instances[i].is_initialized = true;
	}

	if (e_idx < 0) {
		LOG_ERR("No free slots to register endpoint %s", log_strdup(cfg->name));
		return -EIO;
	}

	struct rpmsg_mi_ept_cfg ept_cfg = {0};

	instances[i].endpoints[e_idx].priv = cfg->priv;
	instances[i].endpoints[e_idx].cb = cfg->cb;

	ept_cfg.cb = &cb;
	ept_cfg.priv = &instances[i].endpoints[e_idx];
	ept_cfg.name = cfg->name;

	if (rpmsg_mi_ept_register(&instances[i].ctx,
				  &instances[i].endpoints[e_idx].rpmsg_ep, &ept_cfg) < 0) {
		LOG_ERR("Register endpoint failed");
		return -EIO;
	}

	instances[i].endpoints[e_idx].name = cfg->name;
	instances[i].prio = cfg->prio;
	*ept = &instances[i].endpoints[e_idx];

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
