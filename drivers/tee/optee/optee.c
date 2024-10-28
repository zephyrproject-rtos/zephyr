/*
 * Copyright 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm-smccc.h>
#include <zephyr/drivers/tee.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/dlist.h>

#include "optee_msg.h"
#include "optee_rpc_cmd.h"
#include "optee_smc.h"
LOG_MODULE_REGISTER(optee);

#define DT_DRV_COMPAT linaro_optee_tz

/* amount of physical addresses that can be stored in one page */
#define OPTEE_NUMBER_OF_ADDR_PER_PAGE (OPTEE_MSG_NONCONTIG_PAGE_SIZE / sizeof(uint64_t))

/*
 * TEE Implementation ID
 */
#define TEE_IMPL_ID_OPTEE 1

/*
 * OP-TEE specific capabilities
 */
#define TEE_OPTEE_CAP_TZ  BIT(0)

struct optee_rpc_param {
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t a4;
	uint32_t a5;
	uint32_t a6;
	uint32_t a7;
};

typedef void (*smc_call_t)(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			   unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			   struct arm_smccc_res *res);

struct optee_driver_config {
	const char *method;
};

struct optee_notify {
	sys_dnode_t node;
	uint32_t key;
	struct k_sem wait;
};

struct optee_supp_req {
	sys_dnode_t link;

	bool in_queue;
	uint32_t func;
	uint32_t ret;
	size_t num_params;
	struct tee_param *param;

	struct k_sem complete;
};

struct optee_supp {
	/* Serializes access to this struct */
	struct k_mutex mutex;

	int req_id;
	sys_dlist_t reqs;
	struct optee_supp_req *current;
	struct k_sem reqs_c;
};

struct optee_driver_data {
	smc_call_t smc_call;

	sys_bitarray_t *notif_bitmap;

	sys_dlist_t notif;
	struct k_spinlock notif_lock;
	struct optee_supp supp;
	unsigned long sec_caps;
	struct k_sem call_sem;
};

/* Wrapping functions so function pointer can be used */
static void optee_smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}

static void optee_smccc_hvc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_hvc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}

static int param_to_msg_param(const struct tee_param *param, unsigned int num_param,
			      struct optee_msg_param *msg_param)
{
	int i;
	const struct tee_param *tp = param;
	struct optee_msg_param *mtp = msg_param;

	if (!param || !msg_param) {
		return -EINVAL;
	}

	for (i = 0; i < num_param; i++, tp++, mtp++) {
		if (!tp || !mtp) {
			LOG_ERR("Wrong param on %d iteration", i);
			return -EINVAL;
		}

		switch (tp->attr) {
		case TEE_PARAM_ATTR_TYPE_NONE:
			mtp->attr = OPTEE_MSG_ATTR_TYPE_NONE;
			memset(&mtp->u, 0, sizeof(mtp->u));
			break;
		case TEE_PARAM_ATTR_TYPE_VALUE_INPUT:
		case TEE_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_PARAM_ATTR_TYPE_VALUE_INOUT:
			mtp->attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT + tp->attr -
				TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
			mtp->u.value.a = tp->a;
			mtp->u.value.b = tp->b;
			mtp->u.value.c = tp->c;
			break;
		case TEE_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_PARAM_ATTR_TYPE_MEMREF_INOUT:
			mtp->attr = OPTEE_MSG_ATTR_TYPE_RMEM_INPUT + tp->attr -
				TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
			mtp->u.rmem.shm_ref = tp->c;
			mtp->u.rmem.size = tp->b;
			mtp->u.rmem.offs = tp->a;
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static void msg_param_to_tmp_mem(struct tee_param *p, uint32_t attr,
				 const struct optee_msg_param *mp)
{
	struct tee_shm *shm = (struct tee_shm *)mp->u.tmem.shm_ref;

	p->attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT + attr - OPTEE_MSG_ATTR_TYPE_TMEM_INPUT;
	p->b = mp->u.tmem.size;

	if (!shm) {
		p->a = 0;
		p->c = 0;
		return;
	}

	p->a = mp->u.tmem.buf_ptr - k_mem_phys_addr(shm->addr);
	p->c = mp->u.tmem.shm_ref;
}

static int msg_param_to_param(struct tee_param *param, unsigned int num_param,
			      const struct optee_msg_param *msg_param)
{
	int i;
	struct tee_param *tp = param;
	const struct optee_msg_param *mtp = msg_param;

	if (!param || !msg_param) {
		return -EINVAL;
	}

	for (i = 0; i < num_param; i++, tp++, mtp++) {
		uint32_t attr = mtp->attr & OPTEE_MSG_ATTR_TYPE_MASK;

		if (!tp || !mtp) {
			LOG_ERR("Wrong param on %d iteration", i);
			return -EINVAL;
		}

		switch (attr) {
		case OPTEE_MSG_ATTR_TYPE_NONE:
			memset(tp, 0, sizeof(*tp));
			tp->attr = TEE_PARAM_ATTR_TYPE_NONE;
			break;
		case OPTEE_MSG_ATTR_TYPE_VALUE_INPUT:
		case OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_VALUE_INOUT:
			tp->attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT + attr -
				OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
			tp->a = mtp->u.value.a;
			tp->b = mtp->u.value.b;
			tp->c = mtp->u.value.c;
			break;
		case OPTEE_MSG_ATTR_TYPE_RMEM_INPUT:
		case OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_RMEM_INOUT:
			tp->attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT + attr -
				OPTEE_MSG_ATTR_TYPE_RMEM_INPUT;
			tp->b = mtp->u.rmem.size;

			if (!mtp->u.rmem.shm_ref) {
				tp->a = 0;
				tp->c = 0;
			} else {
				tp->a = mtp->u.rmem.offs;
				tp->c = mtp->u.rmem.shm_ref;
			}

			break;
		case OPTEE_MSG_ATTR_TYPE_TMEM_INPUT:
		case OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_TMEM_INOUT:
			msg_param_to_tmp_mem(tp, attr, mtp);
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static uint64_t regs_to_u64(uint32_t reg0, uint32_t reg1)
{
	return (uint64_t)(((uint64_t)reg0 << 32) | reg1);
}

static void u64_to_regs(uint64_t val, uint32_t *reg0, uint32_t *reg1)
{
	*reg0 = val >> 32;
	*reg1 = val;
}

static inline bool check_param_input(struct optee_msg_arg *arg)
{
	return arg->num_params == 1 &&
		 arg->params[0].attr == OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
}

static void *optee_construct_page_list(void *buf, uint32_t len, uint64_t *phys_buf);

static uint32_t optee_call_supp(const struct device *dev, uint32_t func, size_t num_params,
				struct tee_param *param)
{
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;
	struct optee_supp *supp = &data->supp;
	struct optee_supp_req *req;
	uint32_t ret;

	req = k_malloc(sizeof(*req));
	if (!req) {
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	k_sem_init(&req->complete, 0, 1);
	req->func = func;
	req->num_params = num_params;
	req->param = param;

	/* Insert the request in the request list */
	k_mutex_lock(&supp->mutex, K_FOREVER);
	sys_dlist_append(&supp->reqs, &req->link);
	k_mutex_unlock(&supp->mutex);

	/* Tell an event listener there's a new request */
	k_sem_give(&supp->reqs_c);

	/*
	 * Wait for supplicant to process and return result, once we've
	 * returned from k_sem_take(&req->c) successfully we have
	 * exclusive access again.
	 */

	k_sem_take(&req->complete, K_FOREVER);

	ret = req->ret;
	k_free(req);

	return ret;
}

static int cmd_alloc_suppl(const struct device *dev, size_t sz, struct tee_shm **shm)
{
	uint32_t ret;
	struct tee_param param;

	param.attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param.a = OPTEE_RPC_SHM_TYPE_APPL;
	param.b = sz;
	param.c = 0;

	ret = optee_call_supp(dev, OPTEE_RPC_CMD_SHM_ALLOC, 1, &param);

	if (ret) {
		return ret;
	}

	ret = tee_add_shm(dev, (void *)param.c, 0, param.b, 0, shm);

	return ret;
}

static void cmd_free_suppl(const struct device *dev, struct tee_shm *shm)
{
	struct tee_param param;

	param.attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param.a = OPTEE_RPC_SHM_TYPE_APPL;
	param.b = (uint64_t)shm;
	param.c = 0;

	optee_call_supp(dev, OPTEE_RPC_CMD_SHM_FREE, 1, &param);
	tee_rm_shm(dev, shm);
}

static void handle_cmd_alloc(const struct device *dev, struct optee_msg_arg *arg,
			     void **pages)
{
	int rc;
	struct tee_shm *shm = NULL;
	void *pl;
	uint64_t pl_phys_and_offset;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	if (!check_param_input(arg)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	switch (arg->params[0].u.value.a) {
	case OPTEE_RPC_SHM_TYPE_KERNEL:
		/* TODO handle situation when shm was allocated statically so buffer can be reused*/
		rc = tee_add_shm(dev, NULL, 0, arg->params[0].u.value.b, TEE_SHM_ALLOC, &shm);
		break;
	case OPTEE_RPC_SHM_TYPE_APPL:
		rc = cmd_alloc_suppl(dev, arg->params[0].u.value.b, &shm);
		break;
	default:
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	if (rc) {
		if (rc == -ENOMEM) {
			arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		} else {
			arg->ret = TEEC_ERROR_GENERIC;
		}
		return;
	}

	pl = optee_construct_page_list(shm->addr, shm->size, &pl_phys_and_offset);
	if (!pl) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		goto out;
	}

	*pages = pl;
	arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT | OPTEE_MSG_ATTR_NONCONTIG;
	arg->params[0].u.tmem.buf_ptr = pl_phys_and_offset;
	arg->params[0].u.tmem.size = shm->size;
	arg->params[0].u.tmem.shm_ref = (uint64_t)shm;
	arg->ret = TEEC_SUCCESS;
	return;
out:
	tee_shm_free(dev, shm);
}

static void handle_cmd_free(const struct device *dev, struct optee_msg_arg *arg)
{
	int rc = 0;

	if (!check_param_input(arg)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	switch (arg->params[0].u.value.a) {
	case OPTEE_RPC_SHM_TYPE_KERNEL:
		rc = tee_rm_shm(dev, (struct tee_shm *)arg->params[0].u.value.b);
		break;
	case OPTEE_RPC_SHM_TYPE_APPL:
		cmd_free_suppl(dev, (struct tee_shm *)arg->params[0].u.value.b);
		break;
	default:
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	if (rc) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}

	arg->ret = TEEC_SUCCESS;
}

static void handle_cmd_get_time(const struct device *dev, struct optee_msg_arg *arg)
{
	int64_t ticks;
	int64_t up_secs;
	int64_t up_nsecs;

	if (arg->num_params != 1 ||
	    (arg->params[0].attr & OPTEE_MSG_ATTR_TYPE_MASK)
	    != OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	ticks = k_uptime_ticks();

	up_secs = ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	up_nsecs = k_ticks_to_ns_floor64(ticks - up_secs * CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	arg->params[0].u.value.a = up_secs;
	arg->params[0].u.value.b = up_nsecs;

	arg->ret = TEEC_SUCCESS;
}

/* This should be called under notif_lock */
static inline bool key_is_pending(struct optee_driver_data *data, uint32_t key)
{
	struct optee_notify *iter;

	SYS_DLIST_FOR_EACH_CONTAINER(&data->notif, iter, node) {
		if (iter->key == key) {
			k_sem_give(&iter->wait);
			return true;
		}
	}

	return false;
}

static int optee_notif_send(const struct device *dev, uint32_t key)
{
	struct optee_driver_data *data = dev->data;
	k_spinlock_key_t sp_key;

	if (key > CONFIG_OPTEE_MAX_NOTIF) {
		return -EINVAL;
	}

	sp_key = k_spin_lock(&data->notif_lock);
	if (!key_is_pending(data, key)) {
		/* If nobody is waiting for key - set bit in the bitmap */
		sys_bitarray_set_bit(data->notif_bitmap, key);
	}
	k_spin_unlock(&data->notif_lock, sp_key);

	return 0;
}

static int optee_notif_wait(const struct device *dev, uint32_t key)
{
	int rc = 0;
	struct optee_driver_data *data = dev->data;
	struct optee_notify *entry;
	k_spinlock_key_t sp_key;
	int prev_val;

	if (key > CONFIG_OPTEE_MAX_NOTIF) {
		return -EINVAL;
	}

	entry = k_malloc(sizeof(*entry));
	if (!entry) {
		return -ENOMEM;
	}

	k_sem_init(&entry->wait, 0, 1);
	entry->key = key;

	sp_key = k_spin_lock(&data->notif_lock);

	/*
	 * If notif bit was set then SEND command was already received.
	 * Skipping wait.
	 */
	rc = sys_bitarray_test_and_clear_bit(data->notif_bitmap, key, &prev_val);
	if (rc || prev_val) {
		goto out;
	}

	/*
	 * If key is already registred, then skip.
	 */
	if (key_is_pending(data, key)) {
		rc = -EBUSY;
		goto out;
	}

	sys_dlist_append(&data->notif, &entry->node);

	k_spin_unlock(&data->notif_lock, sp_key);
	k_sem_take(&entry->wait, K_FOREVER);
	sp_key = k_spin_lock(&data->notif_lock);

	sys_dlist_remove(&entry->node);
out:
	k_spin_unlock(&data->notif_lock, sp_key);

	k_free(entry);

	return rc;
}

static void handle_cmd_notify(const struct device *dev, struct optee_msg_arg *arg)
{
	if (!check_param_input(arg)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	switch (arg->params[0].u.value.a) {
	case OPTEE_RPC_NOTIFICATION_SEND:
		if (optee_notif_send(dev, arg->params[0].u.value.b)) {
			goto err;
		}
		break;
	case OPTEE_RPC_NOTIFICATION_WAIT:
		if (optee_notif_wait(dev, arg->params[0].u.value.b)) {
			goto err;
		}
		break;
	default:
		goto err;
	}

	arg->ret = TEEC_SUCCESS;
	return;

err:
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_cmd_wait(const struct device *dev, struct optee_msg_arg *arg)
{
	if (!check_param_input(arg)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	k_sleep(K_MSEC(arg->params[0].u.value.a));

	arg->ret = TEEC_SUCCESS;
}

static void free_shm_pages(void **pages)
{
	/*
	 * Clean allocated pages if needed. Some function calls requires pages
	 * allocation which should be freed after processing new request.
	 * It is safe to free this list when another SHM op (e,g. another alloc
	 * or free) was received.
	 */
	if (*pages) {
		k_free(*pages);
		*pages = NULL;
	}
}

static void handle_rpc_supp_cmd(const struct device *dev, struct optee_msg_arg *arg)
{
	struct tee_param *params;
	int ret;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	params = k_malloc(sizeof(*params) * arg->num_params);
	if (!params) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}

	ret = msg_param_to_param(params, arg->num_params, arg->params);
	if (ret) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		goto out;
	}

	arg->ret = optee_call_supp(dev, arg->cmd, arg->num_params, params);

	ret = param_to_msg_param(params, arg->num_params, arg->params);
	if (ret) {
		arg->ret = TEEC_ERROR_GENERIC;
		arg->ret_origin = TEEC_ORIGIN_COMMS;
	}
out:
	k_free(params);
}

static uint32_t handle_func_rpc_call(const struct device *dev, struct tee_shm *shm,
				     void **pages)
{
	struct optee_msg_arg *arg = shm->addr;

	switch (arg->cmd) {
	case OPTEE_RPC_CMD_SHM_ALLOC:
		free_shm_pages(pages);
		handle_cmd_alloc(dev, arg, pages);
		break;
	case OPTEE_RPC_CMD_SHM_FREE:
		handle_cmd_free(dev, arg);
		break;
	case OPTEE_RPC_CMD_GET_TIME:
		handle_cmd_get_time(dev, arg);
		break;
	case OPTEE_RPC_CMD_NOTIFICATION:
		handle_cmd_notify(dev, arg);
		break;
	case OPTEE_RPC_CMD_SUSPEND:
		handle_cmd_wait(dev, arg);
		break;
	case OPTEE_RPC_CMD_I2C_TRANSFER:
		/* TODO: i2c transfer case is not implemented right now */
		return TEEC_ERROR_NOT_IMPLEMENTED;
	default:
		handle_rpc_supp_cmd(dev, arg);
		break;
	}

	return OPTEE_SMC_CALL_RETURN_FROM_RPC;
}

static void handle_rpc_call(const struct device *dev, struct optee_rpc_param *param,
			    void **pages)
{
	struct tee_shm *shm = NULL;
	uint32_t res = OPTEE_SMC_CALL_RETURN_FROM_RPC;

	switch (OPTEE_SMC_RETURN_GET_RPC_FUNC(param->a0)) {
	case OPTEE_SMC_RPC_FUNC_ALLOC:
		if (!tee_add_shm(dev, NULL, OPTEE_MSG_NONCONTIG_PAGE_SIZE,
				 param->a1,
				 TEE_SHM_ALLOC, &shm)) {
			u64_to_regs((uint64_t)k_mem_phys_addr(shm->addr), &param->a1, &param->a2);
			u64_to_regs((uint64_t)shm, &param->a4, &param->a5);
		} else {
			param->a1 = 0;
			param->a2 = 0;
			param->a4 = 0;
			param->a5 = 0;
		}
		break;
	case OPTEE_SMC_RPC_FUNC_FREE:
		shm = (struct tee_shm *)regs_to_u64(param->a1, param->a2);
		tee_rm_shm(dev, shm);
		break;
	case OPTEE_SMC_RPC_FUNC_FOREIGN_INTR:
		/* Foreign interrupt was raised */
		break;
	case OPTEE_SMC_RPC_FUNC_CMD:
		shm = (struct tee_shm *)regs_to_u64(param->a1, param->a2);
		res = handle_func_rpc_call(dev, shm, pages);
		break;
	default:
		break;
	}

	param->a0 = res;
}

static int optee_call(const struct device *dev, struct optee_msg_arg *arg)
{
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;
	struct optee_rpc_param param = {
		.a0 = OPTEE_SMC_CALL_WITH_ARG
	};
	void *pages = NULL;

	u64_to_regs((uint64_t)k_mem_phys_addr(arg), &param.a1, &param.a2);

	k_sem_take(&data->call_sem, K_FOREVER);
	while (true) {
		struct arm_smccc_res res;

		data->smc_call(param.a0, param.a1, param.a2, param.a3,
			       param.a4, param.a5, param.a6, param.a7, &res);

		if (OPTEE_SMC_RETURN_IS_RPC(res.a0)) {
			param.a0 = res.a0;
			param.a1 = res.a1;
			param.a2 = res.a2;
			param.a3 = res.a3;
			handle_rpc_call(dev, &param, &pages);
		} else {
			free_shm_pages(&pages);
			k_sem_give(&data->call_sem);
			return res.a0 == OPTEE_SMC_RETURN_OK ? TEEC_SUCCESS :
				TEEC_ERROR_BAD_PARAMETERS;
		}
	}
}

static int optee_get_version(const struct device *dev, struct tee_version_info *info)
{
	if (!info) {
		return -EINVAL;
	}

	/*
	 * TODO Version and capabilities should be requested from
	 * OP-TEE OS.
	 */

	info->impl_id = TEE_IMPL_ID_OPTEE;
	info->impl_caps = TEE_OPTEE_CAP_TZ;
	info->gen_caps = TEE_GEN_CAP_GP | TEE_GEN_CAP_REG_MEM;

	return 0;
}

static int optee_close_session(const struct device *dev, uint32_t session_id)
{
	int rc;
	struct tee_shm *shm;
	struct optee_msg_arg *marg;

	rc = tee_add_shm(dev, NULL, OPTEE_MSG_NONCONTIG_PAGE_SIZE,
			 OPTEE_MSG_GET_ARG_SIZE(0),
			 TEE_SHM_ALLOC, &shm);
	if (rc) {
		LOG_ERR("Unable to get shared memory, rc = %d", rc);
		return rc;
	}

	marg = shm->addr;
	marg->num_params = 0;
	marg->cmd = OPTEE_MSG_CMD_CLOSE_SESSION;
	marg->session = session_id;

	rc = optee_call(dev, marg);

	if (tee_rm_shm(dev, shm)) {
		LOG_ERR("Unable to free shared memory");
	}

	return rc;
}

static int optee_open_session(const struct device *dev, struct tee_open_session_arg *arg,
			      unsigned int num_param, struct tee_param *param,
			      uint32_t *session_id)
{
	int rc, ret;
	struct tee_shm *shm;
	struct optee_msg_arg *marg;

	if (!arg || !session_id) {
		return -EINVAL;
	}

	rc = tee_add_shm(dev, NULL, OPTEE_MSG_NONCONTIG_PAGE_SIZE,
			 OPTEE_MSG_GET_ARG_SIZE(num_param + 2),
			 TEE_SHM_ALLOC, &shm);
	if (rc) {
		LOG_ERR("Unable to get shared memory, rc = %d", rc);
		return rc;
	}

	marg = shm->addr;
	memset(marg, 0, OPTEE_MSG_GET_ARG_SIZE(num_param + 2));

	marg->num_params = num_param + 2;
	marg->cmd = OPTEE_MSG_CMD_OPEN_SESSION;
	marg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT | OPTEE_MSG_ATTR_META;
	marg->params[1].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT | OPTEE_MSG_ATTR_META;

	memcpy(&marg->params[0].u.value, arg->uuid, sizeof(arg->uuid));
	memcpy(&marg->params[1].u.value, arg->uuid, sizeof(arg->clnt_uuid));

	marg->params[1].u.value.c = arg->clnt_login;

	rc = param_to_msg_param(param, num_param, marg->params + 2);
	if (rc) {
		goto out;
	}

	arg->ret = optee_call(dev, marg);
	if (arg->ret) {
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		goto out;
	}

	rc = msg_param_to_param(param, num_param, marg->params);
	if (rc) {
		arg->ret = TEEC_ERROR_COMMUNICATION;
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		/*
		 * Ret is needed here only to print an error. Param conversion error
		 * should be returned from the function.
		 */
		ret = optee_close_session(dev, marg->session);
		if (ret) {
			LOG_ERR("Unable to close session: %d", ret);
		}
		goto out;
	}

	*session_id = marg->session;

	arg->ret = marg->ret;
	arg->ret_origin = marg->ret_origin;
out:
	ret = tee_rm_shm(dev, shm);
	if (ret) {
		LOG_ERR("Unable to free shared memory");
	}

	return (rc) ? rc : ret;
}

static int optee_cancel(const struct device *dev, uint32_t session_id, uint32_t cancel_id)
{
	int rc;
	struct tee_shm *shm;
	struct optee_msg_arg *marg;

	rc = tee_add_shm(dev, NULL, OPTEE_MSG_NONCONTIG_PAGE_SIZE,
			 OPTEE_MSG_GET_ARG_SIZE(0),
			 TEE_SHM_ALLOC, &shm);
	if (rc) {
		LOG_ERR("Unable to get shared memory, rc = %d", rc);
		return rc;
	}

	marg = shm->addr;
	marg->num_params = 0;
	marg->cmd = OPTEE_MSG_CMD_CANCEL;
	marg->cancel_id = cancel_id;
	marg->session = session_id;

	rc = optee_call(dev, marg);

	if (tee_rm_shm(dev, shm)) {
		LOG_ERR("Unable to free shared memory");
	}

	return rc;
}

static int optee_invoke_func(const struct device *dev, struct tee_invoke_func_arg *arg,
			     unsigned int num_param, struct tee_param *param)
{
	int rc, ret;
	struct tee_shm *shm;
	struct optee_msg_arg *marg;

	if (!arg) {
		return -EINVAL;
	}

	rc = tee_add_shm(dev, NULL, OPTEE_MSG_NONCONTIG_PAGE_SIZE,
			 OPTEE_MSG_GET_ARG_SIZE(num_param),
			 TEE_SHM_ALLOC, &shm);
	if (rc) {
		LOG_ERR("Unable to get shared memory, rc = %d", rc);
		return rc;
	}

	marg = shm->addr;
	memset(marg, 0, OPTEE_MSG_GET_ARG_SIZE(num_param));

	marg->num_params = num_param;
	marg->cmd = OPTEE_MSG_CMD_INVOKE_COMMAND;
	marg->func = arg->func;
	marg->session = arg->session;

	rc = param_to_msg_param(param, num_param, marg->params);
	if (rc) {
		goto out;
	}

	arg->ret = optee_call(dev, marg);
	if (arg->ret) {
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		goto out;
	}

	rc = msg_param_to_param(param, num_param, marg->params);
	if (rc) {
		arg->ret = TEEC_ERROR_COMMUNICATION;
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		goto out;
	}

	arg->ret = marg->ret;
	arg->ret_origin = marg->ret_origin;
out:
	ret = tee_rm_shm(dev, shm);
	if (ret) {
		LOG_ERR("Unable to free shared memory");
	}

	return (rc) ? rc : ret;
}

static void *optee_construct_page_list(void *buf, uint32_t len, uint64_t *phys_buf)
{
	const size_t page_size = OPTEE_MSG_NONCONTIG_PAGE_SIZE;
	const size_t num_pages_in_pl = OPTEE_NUMBER_OF_ADDR_PER_PAGE - 1;
	uint32_t page_offset = (uintptr_t)buf & (page_size - 1);

	uint8_t *buf_page;
	uint32_t num_pages;
	uint32_t list_size;

	/* see description of OPTEE_MSG_ATTR_NONCONTIG */
	struct {
		uint64_t pages[OPTEE_NUMBER_OF_ADDR_PER_PAGE - 1];
		uint64_t next_page;
	} *pl;

	BUILD_ASSERT(sizeof(*pl) == OPTEE_MSG_NONCONTIG_PAGE_SIZE);

	num_pages = ROUND_UP(page_offset + len, page_size) / page_size;
	list_size = DIV_ROUND_UP(num_pages, num_pages_in_pl) * page_size;

	pl = k_aligned_alloc(page_size, list_size);
	if (!pl) {
		return NULL;
	}

	memset(pl, 0, list_size);

	buf_page = (uint8_t *)ROUND_DOWN((uintptr_t)buf, page_size);

	for (uint32_t pl_idx = 0; pl_idx < list_size / page_size; pl_idx++) {
		for (uint32_t page_idx = 0; num_pages && page_idx < num_pages_in_pl; page_idx++) {
			pl[pl_idx].pages[page_idx] = k_mem_phys_addr(buf_page);
			buf_page += page_size;
			num_pages--;
		}

		if (!num_pages) {
			break;
		}

		pl[pl_idx].next_page = k_mem_phys_addr(pl + 1);
	}

	/* 12 least significant bits of optee_msg_param.u.tmem.buf_ptr should hold page offset
	 * of user buffer
	 */
	*phys_buf = k_mem_phys_addr(pl) | page_offset;

	return pl;
}

static int optee_shm_register(const struct device *dev, struct tee_shm *shm)
{
	struct tee_shm *shm_arg;
	struct optee_msg_arg *msg_arg;
	void *pl;
	uint64_t pl_phys_and_offset;
	int rc;

	rc = tee_add_shm(dev, NULL, OPTEE_MSG_NONCONTIG_PAGE_SIZE, OPTEE_MSG_GET_ARG_SIZE(1),
			 TEE_SHM_ALLOC, &shm_arg);
	if (rc) {
		return rc;
	}

	msg_arg = shm_arg->addr;

	memset(msg_arg, 0, OPTEE_MSG_GET_ARG_SIZE(1));

	pl = optee_construct_page_list(shm->addr, shm->size, &pl_phys_and_offset);
	if (!pl) {
		rc = -ENOMEM;
		goto out;
	}

	/* for this command op-tee os should support CFG_CORE_DYN_SHM */
	msg_arg->cmd = OPTEE_MSG_CMD_REGISTER_SHM;
	/* op-tee OS ingnore this cmd in case when TYPE_TMEM_OUTPUT and NONCONTIG aren't set */
	msg_arg->params->attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT | OPTEE_MSG_ATTR_NONCONTIG;
	msg_arg->num_params = 1;
	msg_arg->params->u.tmem.buf_ptr = pl_phys_and_offset;
	msg_arg->params->u.tmem.shm_ref = (uint64_t)shm;
	msg_arg->params->u.tmem.size = shm->size;

	if (optee_call(dev, msg_arg)) {
		rc = -EINVAL;
	}

	k_free(pl);
out:
	tee_rm_shm(dev, shm_arg);

	return rc;
}

static int optee_shm_unregister(const struct device *dev, struct tee_shm *shm)
{
	struct tee_shm *shm_arg;
	struct optee_msg_arg *msg_arg;
	int rc;

	rc = tee_add_shm(dev, NULL, OPTEE_MSG_NONCONTIG_PAGE_SIZE, OPTEE_MSG_GET_ARG_SIZE(1),
			 TEE_SHM_ALLOC, &shm_arg);
	if (rc) {
		return rc;
	}

	msg_arg = shm_arg->addr;

	memset(msg_arg, 0, OPTEE_MSG_GET_ARG_SIZE(1));

	msg_arg->cmd = OPTEE_MSG_CMD_UNREGISTER_SHM;
	msg_arg->num_params = 1;
	msg_arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_RMEM_INPUT;
	msg_arg->params[0].u.rmem.shm_ref = (uint64_t)shm;

	if (optee_call(dev, msg_arg)) {
		rc = -EINVAL;
	}

	tee_rm_shm(dev, shm_arg);
	return rc;
}

static int optee_suppl_recv(const struct device *dev, uint32_t *func, unsigned int *num_params,
			    struct tee_param *param)
{
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;
	struct optee_supp *supp = &data->supp;
	struct optee_supp_req *req = NULL;

	while (true) {
		k_mutex_lock(&supp->mutex, K_FOREVER);
		req = (struct optee_supp_req *)sys_dlist_peek_head(&supp->reqs);

		if (req) {
			if (supp->current) {
				LOG_ERR("Concurrent supp_recv calls are not supported");
				k_mutex_unlock(&supp->mutex);
				return -EBUSY;
			}

			if (*num_params < req->num_params) {
				LOG_ERR("Not enough space for params, need at least %lu",
					req->num_params);
				k_mutex_unlock(&supp->mutex);
				return -EINVAL;
			}

			supp->current = req;
			sys_dlist_remove(&req->link);
		}
		k_mutex_unlock(&supp->mutex);

		if (req) {
			break;
		}

		k_sem_take(&supp->reqs_c, K_FOREVER);
	}

	*func = req->func;
	*num_params = req->num_params;
	memcpy(param, req->param, sizeof(struct tee_param) * req->num_params);

	return 0;
}

static int optee_suppl_send(const struct device *dev, unsigned int ret, unsigned int num_params,
			    struct tee_param *param)
{
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;
	struct optee_supp *supp = &data->supp;
	struct optee_supp_req *req = NULL;
	size_t n;

	k_mutex_lock(&supp->mutex, K_FOREVER);
	if (supp->current && num_params >= supp->current->num_params) {
		req = supp->current;
		supp->current = NULL;
	} else {
		LOG_ERR("Invalid number of parameters, expected %lu got %u", req->num_params,
			num_params);
	}
	k_mutex_unlock(&supp->mutex);

	if (!req) {
		return -EINVAL;
	}

	/* Update out and in/out parameters */
	for (n = 0; n < req->num_params; n++) {
		struct tee_param *p = req->param + n;

		switch (p->attr & TEE_PARAM_ATTR_TYPE_MASK) {
		case TEE_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_PARAM_ATTR_TYPE_VALUE_INOUT:
			p->a = param[n].a;
			p->b = param[n].b;
			p->c = param[n].c;
			break;
		case TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_PARAM_ATTR_TYPE_MEMREF_INOUT:
			LOG_WRN("Memref params are not fully tested");
			p->a = param[n].a;
			p->b = param[n].b;
			p->c = param[n].c;
			break;
		default:
			break;
		}
	}
	req->ret = ret;

	/* Let the requesting thread continue */
	k_mutex_lock(&supp->mutex, K_FOREVER);
	supp->current = NULL;
	k_mutex_unlock(&supp->mutex);
	k_sem_give(&req->complete);

	return 0;
}

static int set_optee_method(const struct device *dev)
{
	const struct optee_driver_config *conf = dev->config;
	struct optee_driver_data *data = dev->data;

	if (!strcmp("hvc", conf->method)) {
		data->smc_call = optee_smccc_hvc;
	} else if (!strcmp("smc", conf->method)) {
		data->smc_call = optee_smccc_smc;
	} else {
		LOG_ERR("Invalid smc_call method");
		return -EINVAL;
	}

	return 0;
}

static bool optee_check_uid(const struct device *dev)
{
	struct arm_smccc_res res;
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;

	data->smc_call(OPTEE_SMC_CALLS_UID, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 == OPTEE_MSG_UID_0 && res.a1 == OPTEE_MSG_UID_1 &&
	    res.a2 == OPTEE_MSG_UID_2 && res.a3 == OPTEE_MSG_UID_3) {
		return true;
	}

	return false;
}

static void optee_get_revision(const struct device *dev)
{
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;
	struct arm_smccc_res res = { 0 };

	data->smc_call(OPTEE_SMC_CALL_GET_OS_REVISION, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a2) {
		LOG_INF("OPTEE revision %lu.%lu (%08lx)", res.a0,
			res.a1, res.a2);
	} else {
		LOG_INF("OPTEE revision %lu.%lu", res.a0, res.a1);
	}
}

static bool optee_exchange_caps(const struct device *dev, unsigned long *sec_caps)
{
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;
	struct arm_smccc_res res = { 0 };
	unsigned long a1 = 0;

	if (!IS_ENABLED(CONFIG_SMP) || arch_num_cpus() == 1) {
		a1 |= OPTEE_SMC_NSEC_CAP_UNIPROCESSOR;
	}

	data->smc_call(OPTEE_SMC_EXCHANGE_CAPABILITIES, a1, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 != OPTEE_SMC_RETURN_OK) {
		return false;
	}

	*sec_caps = res.a1;
	return true;
}

static unsigned long optee_get_thread_count(const struct device *dev, unsigned long *thread_count)
{
	struct optee_driver_data *data = (struct optee_driver_data *)dev->data;
	struct arm_smccc_res res = { 0 };
	unsigned long a1 = 0;

	data->smc_call(OPTEE_SMC_GET_THREAD_COUNT, a1, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 != OPTEE_SMC_RETURN_OK) {
		return false;
	}

	*thread_count = res.a1;
	return true;
}

static int optee_init(const struct device *dev)
{
	struct optee_driver_data *data = dev->data;
	unsigned long thread_count;

	if (set_optee_method(dev)) {
		return -ENOTSUP;
	}

	sys_dlist_init(&data->notif);
	k_mutex_init(&data->supp.mutex);
	k_sem_init(&data->supp.reqs_c, 0, 1);
	sys_dlist_init(&data->supp.reqs);

	if (!optee_check_uid(dev)) {
		LOG_ERR("OPTEE API UID mismatch");
		return -EINVAL;
	}

	optee_get_revision(dev);

	if (!optee_exchange_caps(dev, &data->sec_caps)) {
		LOG_ERR("OPTEE capabilities exchange failed\n");
		return -EINVAL;
	}

	if (!(data->sec_caps & OPTEE_SMC_SEC_CAP_DYNAMIC_SHM)) {
		LOG_ERR("OPTEE does not support dynamic shared memory");
		return -ENOTSUP;
	}

	if (!optee_get_thread_count(dev, &thread_count)) {
		LOG_ERR("OPTEE unable to get maximum thread count");
		return -ENOTSUP;
	}

	k_sem_init(&data->call_sem, thread_count, thread_count);

	return 0;
}

static const struct tee_driver_api optee_driver_api = {
	.get_version = optee_get_version,
	.open_session = optee_open_session,
	.close_session = optee_close_session,
	.cancel = optee_cancel,
	.invoke_func = optee_invoke_func,
	.shm_register = optee_shm_register,
	.shm_unregister = optee_shm_unregister,
	.suppl_recv = optee_suppl_recv,
	.suppl_send = optee_suppl_send,
};

/*
 * Bitmap of the ongoing notificatons, received from OP-TEE. Maximum number is
 * CONFIG_OPTEE_MAX_NOTIF. This bitmap is needed to handle case when SEND command
 * was received before WAIT command from OP-TEE. In this case WAIT will not create
 * locks.
 */
#define OPTEE_DT_DEVICE_INIT(inst)					\
	SYS_BITARRAY_DEFINE_STATIC(notif_bitmap_##inst, CONFIG_OPTEE_MAX_NOTIF); \
									\
	static struct optee_driver_config optee_config_##inst = {	\
		.method = DT_INST_PROP(inst, method)			\
	};								\
									\
	static struct optee_driver_data optee_data_##inst = {		\
		.notif_bitmap = &notif_bitmap_##inst			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst, optee_init, NULL, &optee_data_##inst, \
			      &optee_config_##inst, POST_KERNEL,	\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &optee_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(OPTEE_DT_DEVICE_INIT)
