/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_optee_driver
 * @{
 * @defgroup t_optee_driver test_optee_driver
 * @}
 */

#include <zephyr/arch/arm64/arm-smccc.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>

#include <zephyr/drivers/tee.h>
#include <optee_msg.h>
#include <optee_smc.h>
#include <optee_rpc_cmd.h>

/*
 * TODO: Test shm_register/shm_register API to work with huge
 * buffers (more than 512K) to ensure that optee_construct_page_list
 * builds correct page list.
 */

#define TEE_OPTEE_CAP_TZ BIT(0)

typedef void (*smc_cb_t)(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			 unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			 struct arm_smccc_res *res);

static struct test_call {
	int num;
	int pending;
	smc_cb_t smc_cb;
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t a4;
	uint32_t a5;
	uint32_t a6;
	uint32_t a7;
	k_tid_t th_id;
} t_call;

static struct test_call wait_call;
static struct test_call send_call;

void arm_smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		   struct arm_smccc_res *res)
{
	if (a0 == OPTEE_SMC_CALLS_UID) {
		res->a0 = OPTEE_MSG_UID_0;
		res->a1 = OPTEE_MSG_UID_1;
		res->a2 = OPTEE_MSG_UID_2;
		res->a3 = OPTEE_MSG_UID_3;
		return;
	}

	if (a0 == OPTEE_SMC_EXCHANGE_CAPABILITIES) {
		res->a1 = OPTEE_SMC_SEC_CAP_DYNAMIC_SHM;
		return;
	}
	if (a0 == OPTEE_SMC_GET_THREAD_COUNT) {
		res->a1 = 5;
		return;
	}
	if (t_call.pending && t_call.smc_cb) {
		t_call.smc_cb(a0, a1, a2, a3, a4, a5, a6, a7, res);
	}
	if (wait_call.pending && wait_call.smc_cb && (k_current_get() == wait_call.th_id)) {
		wait_call.smc_cb(a0, a1, a2, a3, a4, a5, a6, a7, res);
	}
	if (send_call.pending && send_call.smc_cb) {
		send_call.smc_cb(a0, a1, a2, a3, a4, a5, a6, a7, res);
	}
}

/* Allocate dummy arm_smccc_hvc function for the tests */
void arm_smccc_hvc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		   struct arm_smccc_res *res)
{
}

ZTEST(optee_test_suite, test_get_version)
{
	int ret;
	struct tee_version_info info;
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	ret = tee_get_version(dev, NULL);
	zassert_equal(ret, -EINVAL, "tee_get_version failed with code %d", ret);

	ret = tee_get_version(dev, &info);
	zassert_ok(ret, "tee_get_version failed with code %d", ret);
	zassert_equal(info.impl_id, 1, "Wrong impl_id");
	zassert_equal(info.impl_caps, TEE_OPTEE_CAP_TZ, "Wrong impl_caps");
	zassert_equal(info.gen_caps, TEE_GEN_CAP_GP | TEE_GEN_CAP_REG_MEM,
		      "Wrong gen_caps");

	ret = tee_get_version(dev, NULL);
	zassert_equal(ret, -EINVAL, "tee_get_version failed with code %d", ret);
}

void fast_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
	       unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
	       struct arm_smccc_res *res)
{
	t_call.a0 = a0;
	t_call.a1 = a1;
	t_call.a2 = a2;
	t_call.a3 = a3;
	t_call.a4 = a4;
	t_call.a5 = a5;
	t_call.a6 = a6;
	t_call.a7 = a7;

	res->a0 = OPTEE_SMC_RETURN_OK;
}

void fail_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
	       unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
	       struct arm_smccc_res *res)
{
	res->a0 = OPTEE_SMC_RETURN_EBUSY;
}

ZTEST(optee_test_suite, test_fast_calls)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	/* Fail pass */
	ret = tee_open_session(dev, NULL, 0, NULL, &session_id);
	zassert_equal(ret, -EINVAL, "tee_open_session failed with code %d", ret);

	ret = tee_open_session(dev, NULL, 0, NULL, NULL);
	zassert_equal(ret, -EINVAL, "tee_open_session failed with code %d", ret);

	/* Happy pass */
	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
}

ZTEST(optee_test_suite, test_invoke_fn)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_invoke_func_arg invoke_arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	/* Fail pass */
	ret = tee_invoke_func(dev, NULL, 0, NULL);
	zassert_equal(ret, -EINVAL, "tee_invoke_fn failed with code %d", ret);

	t_call.smc_cb = fail_call;

	invoke_arg.func = 12;
	invoke_arg.session = 1;
	ret = tee_invoke_func(dev, &invoke_arg, 0, NULL);
	zassert_equal(ret, -EINVAL, "tee_invoke_fn failed with code %d", ret);

	t_call.smc_cb = fast_call;

	/* Happy pass */
	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	invoke_arg.func = 12;
	invoke_arg.session = 1;
	ret = tee_invoke_func(dev, &invoke_arg, 1, &param);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
}

ZTEST(optee_test_suite, test_cancel)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	ret = tee_cancel(dev, 1, 25);
	zassert_ok(ret, "tee_cancel failed with code %d", ret);

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
}

void normal_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		 unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		 struct arm_smccc_res *res)
{
	t_call.a0 = a0;
	t_call.a1 = a1;
	t_call.a2 = a2;
	t_call.a3 = a3;
	t_call.a4 = a4;
	t_call.a5 = a5;
	t_call.a6 = a6;
	t_call.a7 = a7;

	switch (t_call.num) {
	case 0:
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_ALLOC;
		res->a1 = a4;
		res->a2 = a5;
		res->a3 = a3;
		res->a4 = 0;
		res->a5 = 0;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_FREE;
		res->a1 = a1;
		res->a2 = a2;
		res->a3 = a3;
		res->a4 = a4;
		res->a5 = a5;
		break;
	case 2:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_FOREIGN_INTR;
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

ZTEST(optee_test_suite, test_normal_calls)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = normal_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	t_call.num = 0;

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
}

ZTEST(optee_test_suite, test_reg_unreg)
{
	int ret;
	int addr;
	struct tee_shm *shm = NULL;
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = normal_call;
	zassert_not_null(dev, "Unable to get dev");

	/* Fail pass */
	ret = tee_shm_register(dev, &addr, 1, 0, NULL);
	zassert_equal(ret, -EINVAL, "tee_shm_register failed with code %d", ret);
	t_call.num = 0;

	ret = tee_shm_register(dev, NULL, 1, 0, &shm);
	zassert_equal(ret, -ENOMEM, "tee_shm_register failed with code %d", ret);

	t_call.num = 0;
	ret = tee_shm_register(dev, &addr, 1, 0, NULL);
	zassert_equal(ret, -EINVAL, "tee_shm_register failed with code %d", ret);

	t_call.num = 0;
	ret = tee_shm_register(dev, &addr, 0, 0, &shm);
	zassert_equal(ret, 0, "tee_shm_register failed with code %d", ret);

	t_call.num = 0;
	ret = tee_shm_unregister(dev, NULL);
	zassert_equal(ret, -EINVAL, "tee_shm_unregister failed with code %d", ret);

	/* Happy pass */
	t_call.num = 0;
	ret = tee_shm_register(dev, &addr, 1, 0, &shm);
	zassert_ok(ret, "tee_shm_register failed with code %d", ret);

	t_call.num = 0;
	ret = tee_shm_unregister(dev, shm);
	zassert_ok(ret, "tee_shm_unregister failed with code %d", ret);

	t_call.num = 0;
	ret = tee_shm_alloc(dev, 1, 0, &shm);
	zassert_ok(ret, "tee_shm_alloc failed with code %d", ret);

	t_call.num = 0;
	ret = tee_shm_free(dev, shm);
	zassert_ok(ret, "tee_shm_free failed with code %d", ret);
	t_call.pending = 0;
}

static uint64_t regs_to_u64(uint32_t reg0, uint32_t reg1)
{
	return (uint64_t)(((uint64_t)reg0 << 32) | reg1);
}

static void u64_to_regs(uint64_t val, unsigned long *reg0, unsigned long *reg1)
{
	*reg0 = val >> 32;
	*reg1 = val;
}

uint64_t g_shm_ref;
uint64_t g_func_shm_ref;

void cmd_alloc_free_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			 unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			 struct arm_smccc_res *res)
{
	struct optee_msg_arg *arg;
	struct tee_shm *shm;

	t_call.a0 = a0;
	t_call.a1 = a1;
	t_call.a2 = a2;
	t_call.a3 = a3;
	t_call.a4 = a4;
	t_call.a5 = a5;
	t_call.a6 = a6;
	t_call.a7 = a7;

	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (t_call.num) {
	case 0:
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_ALLOC;
		res->a1 = 1;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		arg = (struct optee_msg_arg *)regs_to_u64(a1, a2);
		arg->cmd = OPTEE_RPC_CMD_SHM_ALLOC;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.b = 4096;
		arg->params[0].u.value.a = OPTEE_RPC_SHM_TYPE_KERNEL;
		res->a1 = a4;
		res->a2 = a5;
		g_shm_ref = regs_to_u64(a4, a5);
		break;
	case 2:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		printk("a1 %lx, a2 %lx a4 %lx a5 %lx\n", a1, a2);
		shm = (struct tee_shm *)regs_to_u64(a1, a2);
		arg = (struct optee_msg_arg *)shm->addr;
		arg->cmd = OPTEE_RPC_CMD_SHM_FREE;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.a = OPTEE_RPC_SHM_TYPE_KERNEL;
		arg->params[0].u.value.b = g_func_shm_ref;
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		break;
	case 3:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_FREE;
		break;
	case 4:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

ZTEST(optee_test_suite, test_func_shm_alloc)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_invoke_func_arg invoke_arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = cmd_alloc_free_call;

	invoke_arg.func = 12;
	invoke_arg.session = 1;
	ret = tee_invoke_func(dev, &invoke_arg, 1, &param);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = fast_call;

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);

	t_call.pending = 0;
}

K_KERNEL_STACK_DEFINE(supp_stack, 8192);
#define TEE_NUM_PARAMS 5
static struct k_thread supp_thread_data;
volatile int supp_thread_ok;

void cmd_rpc_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		  unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		  struct arm_smccc_res *res)
{
	struct optee_msg_arg *arg;
	struct tee_shm *shm;

	t_call.a0 = a0;
	t_call.a1 = a1;
	t_call.a2 = a2;
	t_call.a3 = a3;
	t_call.a4 = a4;
	t_call.a5 = a5;
	t_call.a6 = a6;
	t_call.a7 = a7;

	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (t_call.num) {
	case 0:
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_ALLOC;
		res->a1 = 1;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		arg = (struct optee_msg_arg *)regs_to_u64(a1, a2);
		/* Fall back to supplicant branch */
		arg->cmd = 555;
		arg->num_params = 2;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.a = 1111;
		arg->params[0].u.value.b = 3;
		arg->params[1].attr = OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT;
		res->a1 = a4;
		res->a2 = a5;
		break;
	case 2:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		shm = (struct tee_shm *)regs_to_u64(a1, a2);
		arg = (struct optee_msg_arg *)shm->addr;
		zassert_equal(arg->params[1].attr, OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT,
			      "%s failed wrong attr %lx", __func__, arg->params[0].attr);
		zassert_equal(arg->params[1].u.value.a, 0x1234, "%s failed wrong a %lx",
			      __func__, arg->params[0].u.value.a);
		zassert_equal(arg->params[1].u.value.b, 0x5678, "%s failed wrong b %lx",
			      __func__, arg->params[0].u.value.b);
		res->a0 = OPTEE_SMC_RETURN_OK;
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

static void supp_thread_comm(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);
	struct tee_param params[TEE_NUM_PARAMS];
	unsigned int num_params = TEE_NUM_PARAMS;
	uint32_t func;
	int ret;

	supp_thread_ok = true;

	ret = tee_suppl_recv(dev, &func, &num_params, params);

	if (ret != 0) {
		printk("tee_suppl_recv failed with %d\n", ret);
		supp_thread_ok = false;
	}

	if (func != 555 || num_params != 2) {
		printk("Unexpected func & num_params %d %d\n", func, num_params);
		supp_thread_ok = false;
	}

	if (params[0].attr != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT ||
	    params[1].attr != OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT ||
	    params[0].a != 1111 ||
	    params[0].b != 3) {
		printk("Unexpected params %d %d %d\n", params[0].attr,
		       params[0].a, params[0].b);
		supp_thread_ok = false;
	}

	params[1].a = 0x1234;
	params[1].b = 0x5678;
	tee_suppl_send(dev, 0, 2, params);
}

ZTEST(optee_test_suite, test_func_rpc_supp_cmd)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_invoke_func_arg invoke_arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);
	k_tid_t tid;

	tid = k_thread_create(&supp_thread_data, supp_stack,
			      K_KERNEL_STACK_SIZEOF(supp_stack),
			      (k_thread_entry_t)supp_thread_comm, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1), 0, K_NO_WAIT);
	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = cmd_rpc_call;

	invoke_arg.func = 12;
	invoke_arg.session = 1;
	ret = tee_invoke_func(dev, &invoke_arg, 1, &param);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = fast_call;

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	zassert_true(supp_thread_ok, "supp_thread_comm failed");
	t_call.pending = 0;
	k_thread_abort(tid);
}

void cmd_shm_alloc_appl(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			struct arm_smccc_res *res)
{
	struct optee_msg_arg *arg;
	struct tee_shm *shm;

	t_call.a0 = a0;
	t_call.a1 = a1;
	t_call.a2 = a2;
	t_call.a3 = a3;
	t_call.a4 = a4;
	t_call.a5 = a5;
	t_call.a6 = a6;
	t_call.a7 = a7;

	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (t_call.num) {
	case 0:
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_ALLOC;
		res->a1 = 1;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		arg = (struct optee_msg_arg *)regs_to_u64(a1, a2);
		arg->cmd = OPTEE_RPC_CMD_SHM_ALLOC;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.b = 4096;
		arg->params[0].u.value.a = OPTEE_RPC_SHM_TYPE_APPL;
		res->a1 = a4;
		res->a2 = a5;
		g_shm_ref = regs_to_u64(a4, a5);
		break;
	case 2:
		res->a0 = OPTEE_SMC_RETURN_OK;
		break;
	case 3:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		shm = (struct tee_shm *)regs_to_u64(a1, a2);
		arg = (struct optee_msg_arg *)shm->addr;
		arg->cmd = OPTEE_RPC_CMD_SHM_FREE;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.a = OPTEE_RPC_SHM_TYPE_APPL;
		arg->params[0].u.value.b = g_shm_ref;
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		break;
	case 4:
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		res->a0 = OPTEE_SMC_RETURN_OK;
		break;
	case 5:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

static void supp_thread_alloc(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);
	struct tee_param params[TEE_NUM_PARAMS];
	void *mem;
	unsigned int num_params = TEE_NUM_PARAMS;
	uint32_t func;
	int ret;

	supp_thread_ok = true;

	ret = tee_suppl_recv(dev, &func, &num_params, params);

	if (ret != 0) {
		printk("tee_suppl_recv failed with %d\n", ret);
		supp_thread_ok = false;
		goto fail1;
	}

	if (func != OPTEE_RPC_CMD_SHM_ALLOC || num_params != 1) {
		printk("Unexpected func & num_params %d %d\n", func, num_params);
		supp_thread_ok = false;
		goto fail1;
	}

	if (params[0].attr != OPTEE_MSG_ATTR_TYPE_VALUE_INOUT ||
	    params[0].a != OPTEE_RPC_SHM_TYPE_APPL) {
		printk("Unexpected params %d %d\n", params[0].attr,
		       params[0].a);
		supp_thread_ok = false;
		goto fail1;
	}

	mem = k_malloc(params[0].b);

	if (!mem) {
		printk("k_malloc failed\n");
		supp_thread_ok = false;
		goto fail1;
	}

	params[0].c = (uint64_t)mem;
	tee_suppl_send(dev, 0, 1, params);

	ret = tee_suppl_recv(dev, &func, &num_params, params);
	if (func != OPTEE_RPC_CMD_SHM_FREE || num_params != 1) {
		printk("Unexpected func & num_params %d %d\n", func, num_params);
		supp_thread_ok = false;
		goto fail2;
	}

	tee_suppl_send(dev, 0, 1, params);
	k_free(mem);

	return;
fail1:
	tee_suppl_send(dev, -1, 1, params);
fail2:
	tee_suppl_send(dev, -1, 1, params);
}

ZTEST(optee_test_suite, test_func_rpc_shm_alloc_appl)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_invoke_func_arg invoke_arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);
	k_tid_t tid;

	tid = k_thread_create(&supp_thread_data, supp_stack,
			      K_KERNEL_STACK_SIZEOF(supp_stack),
			      (k_thread_entry_t)supp_thread_alloc, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1), 0, K_NO_WAIT);
	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = cmd_shm_alloc_appl;

	invoke_arg.func = 12;
	invoke_arg.session = 1;
	ret = tee_invoke_func(dev, &invoke_arg, 1, &param);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);
	zassert_true(supp_thread_ok, "supp_thread failed");

	t_call.num = 0;
	t_call.smc_cb = fast_call;

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
	k_thread_abort(tid);
}

void cmd_gettime_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		      unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		      struct arm_smccc_res *res)
{
	struct optee_msg_arg *arg;
	struct tee_shm *shm;

	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (t_call.num) {
	case 0:
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_ALLOC;
		res->a1 = 1;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		arg = (struct optee_msg_arg *)regs_to_u64(a1, a2);
		arg->cmd = OPTEE_RPC_CMD_GET_TIME;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT;
		res->a1 = a4;
		res->a2 = a5;
		g_shm_ref = regs_to_u64(a4, a5);
		break;
	case 2:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_FREE;
		shm = (struct tee_shm *)regs_to_u64(a1, a2);
		arg = (struct optee_msg_arg *)shm->addr;

		t_call.a6 = arg->params[0].u.value.a;
		t_call.a7 = arg->params[0].u.value.b;
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		break;
	case 3:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

#define TICKS 0xDEADBEEF
#define TEST_SEC 37359285
#define TEST_NSEC 590000000
extern void z_vrfy_sys_clock_tick_set(uint64_t tick);

ZTEST(optee_test_suite, test_gettime)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_invoke_func_arg invoke_arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = cmd_gettime_call;

	z_vrfy_sys_clock_tick_set(TICKS);

	invoke_arg.func = 12;
	invoke_arg.session = 1;
	ret = tee_invoke_func(dev, &invoke_arg, 1, &param);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);

	zassert_equal(t_call.a6, TEST_SEC, "Unexpected secs");
	zassert_equal(t_call.a7, TEST_NSEC, "Unexpected nsecs");

	t_call.num = 0;
	t_call.smc_cb = fast_call;

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
}

void cmd_suspend_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		      unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		      struct arm_smccc_res *res)
{
	struct optee_msg_arg *arg;
	struct tee_shm *shm;

	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (t_call.num) {
	case 0:
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_ALLOC;
		res->a1 = 1;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		arg = (struct optee_msg_arg *)regs_to_u64(a1, a2);
		arg->cmd = OPTEE_RPC_CMD_SUSPEND;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.a = t_call.a0;
		res->a1 = a4;
		res->a2 = a5;
		g_shm_ref = regs_to_u64(a4, a5);
		break;
	case 2:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_FREE;
		shm = (struct tee_shm *)regs_to_u64(a1, a2);
		arg = (struct optee_msg_arg *)shm->addr;

		t_call.a6 = arg->params[0].u.value.a;
		t_call.a7 = arg->params[0].u.value.b;
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		break;
	case 3:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

ZTEST(optee_test_suite, test_suspend)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_invoke_func_arg invoke_arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	t_call.num = 0;
	t_call.a0 = 4000; /* Set timeout 4000 ms */
	t_call.smc_cb = cmd_suspend_call;

	invoke_arg.func = 12;
	invoke_arg.session = 1;
	ret = tee_invoke_func(dev, &invoke_arg, 1, &param);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = fast_call;

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
}

void cmd_notify_alloc_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			   unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			   struct arm_smccc_res *res)
{
	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (t_call.num) {
	case 0:
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_ALLOC;
		res->a1 = 1;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		/* res->a1 = a4; */
		/* res->a2 = a5; */
		g_shm_ref = regs_to_u64(a4, a5);
		break;
	default:
		zassert_equal(a0, 0x32000004, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

void cmd_notify_free_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			  unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			  struct arm_smccc_res *res)
{
	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (t_call.num) {
	case 0:
		zassert_equal(a0, 0x32000004, "%s failed with ret %lx", __func__, a0);
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_FREE;
		break;
	case 1:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		res->a1 = a4;
		res->a2 = a5;
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
	}

	t_call.num++;
}

static struct test_call wait_call;
static struct test_call send_call;

void cmd_notify_wait_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			  unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			  struct arm_smccc_res *res)
{
	struct optee_msg_arg *arg;
	struct tee_shm *shm;

	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (wait_call.num) {
	case 0:
		zassert_equal(a0, 0x32000004, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		shm = (struct tee_shm *)g_shm_ref;
		arg = (struct optee_msg_arg *)shm->addr;
		arg->cmd = OPTEE_RPC_CMD_NOTIFICATION;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.a = OPTEE_RPC_NOTIFICATION_WAIT;
		arg->params[0].u.value.b = wait_call.a0; /* Set notification key */
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		wait_call.a6 = 1;
	}

	wait_call.num++;
}

void cmd_notify_send_call(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
			  unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
			  struct arm_smccc_res *res)
{
	struct optee_msg_arg *arg;
	struct tee_shm *shm;

	res->a1 = a1;
	res->a2 = a2;
	res->a3 = a3;
	res->a4 = a4;
	res->a5 = a5;

	switch (send_call.num) {
	case 0:
		zassert_equal(a0, 0x32000004, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_RPC_PREFIX | OPTEE_SMC_RPC_FUNC_CMD;
		shm = (struct tee_shm *)g_shm_ref;
		arg = (struct optee_msg_arg *)shm->addr;
		arg->cmd = OPTEE_RPC_CMD_NOTIFICATION;
		arg->num_params = 1;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
		arg->params[0].u.value.a = OPTEE_RPC_NOTIFICATION_SEND;
		arg->params[0].u.value.b = send_call.a0; /* Set notification key */
		u64_to_regs(g_shm_ref, &res->a1, &res->a2);
		break;
	default:
		zassert_equal(a0, 0x32000003, "%s failed with ret %lx", __func__, a0);
		res->a0 = OPTEE_SMC_RETURN_OK;
		send_call.a6 = 1;
	}

	send_call.num++;
}

static int test_invoke_fn(const struct device *const dev, uint32_t session_id)
{
	struct tee_invoke_func_arg invoke_arg = {};
	struct tee_param param = {};

	invoke_arg.func = 12;
	invoke_arg.session = session_id;

	return tee_invoke_func(dev, &invoke_arg, 1, &param);
}

static void wait_handler(void *arg0, void *arg1, void *arg2)
{
	int ret;
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* This expects wait_call.a0 to be set as key and wait_call.a4 as session_id */
	wait_call.pending = 1;
	wait_call.th_id = k_current_get();
	wait_call.num = 0;
	wait_call.a6 = 0; /* result */

	wait_call.smc_cb = cmd_notify_wait_call;

	ret = test_invoke_fn(dev, t_call.a4);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);
	wait_call.a6 = 1;
	wait_call.pending = 0;
}

#define WAIT_STACKSIZE 512
#define WAIT_PRIORITY  4

static K_THREAD_STACK_DEFINE(wait_stack, WAIT_STACKSIZE);
static struct k_thread wait_thread;

static void do_wait(int key, uint32_t session_id)
{
	wait_call.a0 = key;
	wait_call.a4 = session_id;
	k_thread_create(&wait_thread, wait_stack, WAIT_STACKSIZE, wait_handler,
			INT_TO_POINTER(1) /* key */,
			NULL, NULL, K_PRIO_COOP(WAIT_PRIORITY), 0, K_NO_WAIT);
}

ZTEST(optee_test_suite, test_notify)
{
	int ret;
	uint32_t session_id;
	struct tee_open_session_arg arg = {};
	struct tee_param param = {};
	const struct device *const dev = DEVICE_DT_GET_ONE(linaro_optee_tz);

	zassert_not_null(dev, "Unable to get dev");

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = fast_call;

	arg.uuid[0] = 111;
	arg.clnt_uuid[0] = 222;
	arg.clnt_login = TEEC_LOGIN_PUBLIC;
	param.attr = TEE_PARAM_ATTR_TYPE_NONE;
	param.a = 3333;
	ret = tee_open_session(dev, &arg, 1, &param, &session_id);
	zassert_ok(ret, "tee_open_session failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = cmd_notify_alloc_call;

	ret = test_invoke_fn(dev, session_id);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);
	t_call.pending = 0;

	/* Wait then send */
	do_wait(1, session_id);
	k_sleep(K_MSEC(100));

	send_call.pending = 1;
	send_call.num = 0;
	send_call.a0 = 1; /* key */
	send_call.smc_cb = cmd_notify_send_call;

	ret = test_invoke_fn(dev, session_id);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);
	send_call.pending = 0;

	k_sleep(K_MSEC(100));
	zassert_equal(wait_call.a6, 1, "Notify wait is still in progress");

	/* Test send then wait */
	send_call.pending = 1;
	send_call.num = 0;
	send_call.a0 = 2; /* key */
	send_call.smc_cb = cmd_notify_send_call;

	ret = test_invoke_fn(dev, session_id);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);
	send_call.pending = 0;

	wait_call.pending = 1;
	wait_call.th_id = k_current_get();
	wait_call.num = 0;
	wait_call.a0 = 2; /* key */

	wait_call.smc_cb = cmd_notify_wait_call;

	ret = test_invoke_fn(dev, session_id);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);

	wait_call.pending = 0;
	/* End of test section */

	t_call.pending = 1;
	t_call.num = 0;
	t_call.smc_cb = cmd_notify_free_call;

	ret = test_invoke_fn(dev, session_id);
	zassert_ok(ret, "tee_invoke_fn failed with code %d", ret);

	t_call.num = 0;
	t_call.smc_cb = fast_call;

	ret = tee_close_session(dev, session_id);
	zassert_ok(ret, "close_session failed with code %d", ret);
	t_call.pending = 0;
}

ZTEST_SUITE(optee_test_suite, NULL, NULL, NULL, NULL, NULL);
