/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Intel SoC FPGA platform specific functions used by Arm SiP Services driver.
 */

#include <kernel.h>
#include <device.h>
#include <drivers/sip_svc/sip_svc.h>
#include <drivers/sip_svc/sip_svc_agilex_mailbox.h>
#include <drivers/sip_svc/sip_svc_agilex_smc.h>

bool sip_svc_plat_func_id_valid(uint32_t command, uint32_t func_id)
{
	if (command > SIP_SVC_PROTO_CMD_CANCEL)
		return false;

	if (command == SIP_SVC_PROTO_CMD_SYNC) {
		/* Synchronous SMC Function IDs */
		switch (func_id) {
		case SMC_FUNC_ID_GET_SVC_VERSION:
		case SMC_FUNC_ID_REG_READ:
		case SMC_FUNC_ID_REG_WRITE:
		case SMC_FUNC_ID_REG_UPDATE:
		case SMC_FUNC_ID_SET_HPS_BRIDGES:
			return true;
		default:
			break;
		}
	} else if (command == SIP_SVC_PROTO_CMD_ASYNC) {
		/* Asynchronous SMC Function IDs */
		switch (func_id) {
		case SMC_FUNC_ID_MAILBOX_SEND_COMMAND:
		case SMC_FUNC_ID_MAILBOX_POLL_RESPONSE:
			return true;
		default:
			break;
		}
	}

	return false;
}

void sip_svc_plat_update_trans_id(struct sip_svc_request *request,
				 uint32_t trans_id)
{
	uint32_t *data;

	if (!request)
		return;

	/* Assign the trans id into intel smc header a1 */
	SMC_PLAT_PROTO_HEADER_SET_TRANS_ID(request->a1, trans_id);

	/* Assign the trans id into mailbox header */
	if (request->a2) {
		data = (uint32_t *)request->a2;
		SIP_SVC_MB_HEADER_SET_TRANS_ID(data[0], trans_id);
	}
}

void sip_svc_plat_free_async_memory(struct sip_svc_request *request)
{
	/* Free mailbox command data dynamic memory space,
	 * this function will be called after sip_svc driver
	 * process the async request.
	 */
	if (request->a2) {
		k_free((void *)request->a2);
	}
}

int sip_svc_plat_async_res_req(unsigned long *a0, unsigned long *a1,
				unsigned long *a2, unsigned long *a3,
				unsigned long *a4, unsigned long *a5,
				unsigned long *a6, unsigned long *a7,
				char *buf, size_t size)
{
	/* Fill in SMC parameter to read mailbox response */
	*a0 = SMC_FUNC_ID_MAILBOX_POLL_RESPONSE;
	*a1 = 0;
	*a2 = (unsigned long) buf;
	*a3 = size;

	return 0;
}

int sip_svc_plat_async_res_res(struct arm_smccc_res *res,
				char *buf, size_t *size,
				uint32_t *trans_id)
{
	uint32_t *resp = (uint32_t *)buf;

	if (!res || !buf || !size || !trans_id) {
		return -EINVAL;
	}

	if (res->a0 == SMC_STATUS_OKAY) {
		/* Extract transaction id from mailbox response header */
		*trans_id = SIP_SVC_MB_HEADER_GET_TRANS_ID(resp[0]);
		/* The final length should includes both header and body */
		*size = (SIP_SVC_MB_HEADER_GET_LENGTH(resp[0]) + 1) * 4;
	} else if (res->a0 == SMC_STATUS_INVALID) {
		return -ENOTSUP;
	} else {
		return -EINPROGRESS;
	}

	return 0;
}

uint32_t sip_svc_plat_get_error_code(struct arm_smccc_res *res)
{
	if (res)
		return res->a0;
	else
		return SIP_SVC_ID_INVALID;
}
