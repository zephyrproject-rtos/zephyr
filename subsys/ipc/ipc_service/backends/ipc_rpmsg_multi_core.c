/*
 * Copyright (c) 2021, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipc/ipc_service_backend.h>
#include <ipc/rpmsg_multi_instance.h>

#include <logging/log.h>
#include <sys/util_macro.h>
#include <zephyr.h>
#include <device.h>

#define DT_DRV_COMPAT ipc_rpmsg_mc

LOG_MODULE_REGISTER(ipc_rpmsg_multi_instance, CONFIG_IPC_SERVICE_LOG_LEVEL);

#define NUM_INSTANCES		DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)
#define NUM_ENDPOINTS		CONFIG_IPC_BACKEND_RPMSG_NUM_ENDPOINTS_PER_INSTANCE
#define WORK_QUEUE_STACK_SIZE	CONFIG_IPC_BACKEND_RPMSG_WORK_QUEUE_STACK_SIZE

K_THREAD_STACK_ARRAY_DEFINE(ipm_stack, NUM_INSTANCES, WORK_QUEUE_STACK_SIZE);

struct ipc_ept {
	struct rpmsg_mi_ept rpmsg_ep;
	struct ipc_service_cb cb;
	void *priv;
};

struct ipc_instance {
	struct ipc_ept endpoint[NUM_ENDPOINTS];
	struct rpmsg_mi_ctx_cfg ctx_cfg;
	struct rpmsg_mi_ctx ctx;
};

#define FOREACH_SHM(inst)							\
	{ .addr = DT_REG_ADDR(DT_INST_PHANDLE(inst, ipc_shm)),			\
	  .size = DT_REG_SIZE(DT_INST_PHANDLE(inst, ipc_shm)),			\
	},

static struct rpmsg_mi_ctx_shm_cfg shm[NUM_INSTANCES] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_SHM)
};

#define FOREACH_INST(inst)							\
	{ .ctx_cfg.name = DT_INST_PROP(inst, target),				\
	  .ctx_cfg.ipm_work_q_prio = DT_INST_PROP(inst, priority),		\
	  .ctx_cfg.ipm_thread_name = DT_INST_PROP(inst, target),		\
	  .ctx_cfg.ipm_rx_name = DT_INST_PROP_BY_PHANDLE(inst, ipm_rx, label),	\
	  .ctx_cfg.ipm_tx_name = DT_INST_PROP_BY_PHANDLE(inst, ipm_tx, label),	\
	  .ctx_cfg.ipm_tx_id = DT_INST_PROP(inst, ipm_tx_id),			\
	  .ctx_cfg.shm = &shm[inst],						\
	},

static struct ipc_instance instance[NUM_INSTANCES] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_INST)
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

static struct ipc_ept *get_available_ept_slot(struct ipc_instance *instance)
{
	for (size_t i = 0; i < NUM_ENDPOINTS; i++) {
		if (!(instance->endpoint[i].priv)) {
			return &instance->endpoint[i];
		}
	}
	return NULL;
}

static struct ipc_instance *get_instance(const char *target)
{
	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		if (strcmp(instance[i].ctx.name, target) == 0) {
			return &instance[i];
		}
	}
	return NULL;
}

static int register_ept(struct ipc_ept **ept, const struct ipc_ept_cfg *cfg)
{
	struct rpmsg_mi_ept_cfg ept_cfg = { 0 };
	struct ipc_instance *instance;

	if (!cfg || !ept) {
		return -EINVAL;
	}

	instance = get_instance(cfg->target_inst);
	if (!instance) {
		LOG_ERR("No instance %s", log_strdup(cfg->target_inst));
		return -ENODEV;
	}

	*ept = get_available_ept_slot(instance);
	if (!(*ept)) {
		LOG_ERR("No free slots to register endpoint %s on target %s",
			log_strdup(cfg->name), log_strdup(cfg->target_inst));
		return -EIO;
	}

	(*ept)->priv = cfg->priv;
	(*ept)->cb = cfg->cb;

	ept_cfg.cb = &cb;
	ept_cfg.priv = *ept;
	ept_cfg.name = cfg->name;

	if (rpmsg_mi_ept_register(&instance->ctx, &((*ept)->rpmsg_ep), &ept_cfg) < 0) {
		LOG_ERR("Register endpoint failed");
		return -EIO;
	}

	return 0;
}

const static struct ipc_service_backend backend = {
	.name = "RPMsg multi-core backend",
	.send = send,
	.register_endpoint = register_ept,
};

static int register_instance(unsigned int inst)
{
	struct rpmsg_mi_ctx_cfg *ctx_cfg = &instance[inst].ctx_cfg;

	ctx_cfg->ipm_stack_area = ipm_stack[inst];
	ctx_cfg->ipm_stack_size = K_THREAD_STACK_SIZEOF(ipm_stack[inst]);

	LOG_DBG("Registering instance %s\n", log_strdup(ctx_cfg->name));

	if (rpmsg_mi_ctx_init(&instance[inst].ctx, ctx_cfg) < 0) {
		LOG_ERR("Instance initialization failed");
		return -EIO;
	}

	return 0;
}

static int init_backend(const struct device *dev)
{
	ARG_UNUSED(dev);

	for (size_t inst = 0; inst < NUM_INSTANCES; inst++) {
		register_instance(inst);
	}

	return ipc_service_register_backend(&backend);
}

SYS_INIT(init_backend, POST_KERNEL, CONFIG_IPC_SERVICE_BACKEND_REG_PRIORITY);
