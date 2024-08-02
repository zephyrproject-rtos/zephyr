/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Intel SoC FPGA platform specific functions used by ARM SiP Services for
 * supporting EL3 communication from zephyr.
 */

#include <string.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_mailbox.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include <zephyr/drivers/sip_svc/sip_svc_driver.h>
#include <zephyr/internal/syscall_handler.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(intel_socfpga_agilex_sip_smc, CONFIG_ARM_SIP_SVC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT intel_socfpga_agilex_sip_smc

#define DT_SIP_SMC DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)

static bool intel_sip_smc_plat_func_id_valid(const struct device *dev, uint32_t command,
					     uint32_t func_id)
{
	ARG_UNUSED(dev);
	bool valid = false;

	if (command > SIP_SVC_PROTO_CMD_MAX) {
		return false;
	}

	if (command == SIP_SVC_PROTO_CMD_SYNC) {
		/* Synchronous SMC Function IDs */
		switch (func_id) {
		case SMC_FUNC_ID_GET_SVC_VERSION:
		case SMC_FUNC_ID_REG_READ:
		case SMC_FUNC_ID_REG_WRITE:
		case SMC_FUNC_ID_REG_UPDATE:
		case SMC_FUNC_ID_SET_HPS_BRIDGES:
		case SMC_FUNC_ID_RSU_UPDATE_ADDR:
			valid = true;
			break;
		default:
			valid = false;
			break;
		}
	} else if (command == SIP_SVC_PROTO_CMD_ASYNC) {
		/* Asynchronous SMC Function IDs */
		switch (func_id) {
		case SMC_FUNC_ID_MAILBOX_SEND_COMMAND:
		case SMC_FUNC_ID_MAILBOX_POLL_RESPONSE:
			valid = true;
			break;
		default:
			valid = false;
			break;
		}
	}

	return valid;
}

static uint32_t intel_sip_smc_plat_format_trans_id(const struct device *dev, uint32_t client_idx,
						   uint32_t trans_idx)
{
	ARG_UNUSED(dev);

	/* Combine the transaction id and client id to get the job id*/
	return (((client_idx & 0xF) << 4) | (trans_idx & 0xF));
}

static uint32_t intel_sip_smc_plat_get_trans_idx(const struct device *dev, uint32_t trans_id)
{
	ARG_UNUSED(dev);

	return (trans_id & 0xF);
}

static void intel_sip_smc_plat_update_trans_id(const struct device *dev,
					       struct sip_svc_request *request, uint32_t trans_id)
{
	ARG_UNUSED(dev);

	uint32_t *data;

	if (request == NULL) {
		LOG_ERR("request is empty");
		return;
	}

	/* Assign the trans id into intel smc header a1 */
	SMC_PLAT_PROTO_HEADER_SET_TRANS_ID(request->a1, trans_id);

	/* Assign the trans id into mailbox header */
	if ((void *)request->a2 != NULL) {
		data = (uint32_t *)request->a2;
		SIP_SVC_MB_HEADER_SET_TRANS_ID(data[0], trans_id);
	}
}

static void intel_sip_smc_plat_free_async_memory(const struct device *dev,
						 struct sip_svc_request *request)
{
	ARG_UNUSED(dev);

	/* Free mailbox command data dynamic memory space,
	 * this function will be called after sip_svc service
	 * process the async request.
	 */
	if (request->a2) {
		k_free((void *)request->a2);
	}
}

static int intel_sip_smc_plat_async_res_req(const struct device *dev, unsigned long *a0,
					    unsigned long *a1, unsigned long *a2, unsigned long *a3,
					    unsigned long *a4, unsigned long *a5, unsigned long *a6,
					    unsigned long *a7, char *buf, size_t size)
{
	ARG_UNUSED(dev);

	/* Fill in SMC parameter to read mailbox response */
	*a0 = SMC_FUNC_ID_MAILBOX_POLL_RESPONSE;
	*a1 = 0;
	*a2 = (unsigned long)buf;
	*a3 = size;

	return 0;
}

static int intel_sip_smc_plat_async_res_res(const struct device *dev, struct arm_smccc_res *res,
					    char *buf, size_t *size, uint32_t *trans_id)
{
	ARG_UNUSED(dev);
	uint32_t *resp = (uint32_t *)buf;

	__ASSERT((res && buf && size && trans_id), "invalid parameters\n");

	if (((long)res->a0) <= SMC_STATUS_OKAY) {
		/* Extract transaction id from mailbox response header */
		*trans_id = SIP_SVC_MB_HEADER_GET_TRANS_ID(resp[0]);
		/* The final length should include both header and body */
		*size = (SIP_SVC_MB_HEADER_GET_LENGTH(resp[0]) + 1) * 4;
	} else {
		LOG_INF("There is no valid polling response %ld", (long)res->a0);
		return -EINPROGRESS;
	}

	LOG_INF("Got a valid polling response");
	return 0;
}

static uint32_t intel_sip_smc_plat_get_error_code(const struct device *dev,
						  struct arm_smccc_res *res)
{
	ARG_UNUSED(dev);

	if (res != NULL) {
		return res->a0;
	} else {
		return SIP_SVC_ID_INVALID;
	}
}

static void intel_sip_secure_monitor_call(const struct device *dev, unsigned long function_id,
					  unsigned long arg0, unsigned long arg1,
					  unsigned long arg2, unsigned long arg3,
					  unsigned long arg4, unsigned long arg5,
					  unsigned long arg6, struct arm_smccc_res *res)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(res != NULL);
	uint64_t start, end;

	LOG_DBG("Before %s call", DT_PROP(DT_SIP_SMC, method));
	LOG_DBG("\tfunction_id       %08lx", function_id);
	LOG_DBG("\targ0              %08lx", arg0);
	LOG_DBG("\targ1              %08lx", arg1);
	LOG_DBG("\targ2              %08lx", arg2);
	LOG_DBG("\targ3              %08lx", arg3);
	LOG_DBG("\targ4              %08lx", arg4);
	LOG_DBG("\targ5              %08lx", arg5);
	LOG_DBG("\targ6              %08lx", arg6);

	start = k_cycle_get_64();
	arm_smccc_smc(function_id, arg0, arg1, arg2, arg3, arg4, arg5, arg6, res);
	end = k_cycle_get_64();

	LOG_INF("Time taken for %08lx is %08lld ns", function_id,
		k_cyc_to_ns_ceil64(end - start));

	LOG_DBG("After %s call", DT_PROP(DT_SIP_SMC, method));
	LOG_DBG("\tres->a0           %08lx", res->a0);
	LOG_DBG("\tres->a1           %08lx", res->a1);
	LOG_DBG("\tres->a2           %08lx", res->a2);
	LOG_DBG("\tres->a3           %08lx", res->a3);
	LOG_DBG("\tres->a4           %08lx", res->a4);
	LOG_DBG("\tres->a5           %08lx", res->a5);
	LOG_DBG("\tres->a6           %08lx", res->a6);
	LOG_DBG("\tres->a7           %08lx", res->a7);
}

static int arm_sip_smc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_INF("Supervisory call %s registered successfully", DT_PROP(DT_SIP_SMC, method));

	return 0;
}

static const struct svc_driver_api api = {
	.sip_supervisory_call = intel_sip_secure_monitor_call,
	.sip_svc_plat_get_trans_idx = intel_sip_smc_plat_get_trans_idx,
	.sip_svc_plat_format_trans_id = intel_sip_smc_plat_format_trans_id,
	.sip_svc_plat_func_id_valid = intel_sip_smc_plat_func_id_valid,
	.sip_svc_plat_update_trans_id = intel_sip_smc_plat_update_trans_id,
	.sip_svc_plat_get_error_code = intel_sip_smc_plat_get_error_code,
	.sip_svc_plat_async_res_req = intel_sip_smc_plat_async_res_req,
	.sip_svc_plat_async_res_res = intel_sip_smc_plat_async_res_res,
	.sip_svc_plat_free_async_memory = intel_sip_smc_plat_free_async_memory};

BUILD_ASSERT((DT_PROP(DT_SIP_SMC, zephyr_num_clients) != 0), "num-clients should not be zero");
BUILD_ASSERT((CONFIG_ARM_SIP_SVC_EL3_MAX_ALLOWED_TRANSACTIONS > 0),
	     "CONFIG_ARM_SIP_SVC_EL3_MAX_ALLOWED_TRANSACTIONS should be greater than 0");

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

SIP_SVC_CONTROLLER_DEFINE(0, DT_PROP(DT_SIP_SMC, method), DEVICE_DT_GET(DT_SIP_SMC),
			  DT_PROP(DT_SIP_SMC, zephyr_num_clients),
			  CONFIG_ARM_SIP_SVC_EL3_MAX_ALLOWED_TRANSACTIONS,
			  CONFIG_ARM_SIP_SVC_EL3_MAILBOX_RESPONSE_SIZE);

DEVICE_DT_DEFINE(DT_SIP_SMC, arm_sip_smc_init, NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_ARM_SIP_SVC_DRIVER_INIT_PRIORITY, &api);

#endif
