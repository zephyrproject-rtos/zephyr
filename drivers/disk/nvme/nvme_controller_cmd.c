/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2022 Intel Corp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nvme, CONFIG_NVME_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

#include "nvme.h"
#include "nvme_helpers.h"

int nvme_ctrlr_cmd_identify_controller(struct nvme_controller *ctrlr,
				       nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;

	request = nvme_allocate_request_vaddr(
		&ctrlr->cdata, sizeof(struct nvme_controller_data),
		cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	memset(&request->cmd, 0, sizeof(request->cmd));
	request->cmd.cdw0.opc = NVME_OPC_IDENTIFY;
	request->cmd.cdw10 = sys_cpu_to_le32(1);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_identify_namespace(struct nvme_controller *ctrlr,
				      uint32_t nsid, void *payload,
				      nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;

	request = nvme_allocate_request_vaddr(
		payload, sizeof(struct nvme_namespace_data),
		cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	request->cmd.cdw0.opc = NVME_OPC_IDENTIFY;
	/*
	 * TODO: create an identify command data structure
	 */
	request->cmd.nsid = sys_cpu_to_le32(nsid);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_create_io_cq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;
	struct nvme_command *cmd;

	request = nvme_allocate_request_null(cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	cmd = &request->cmd;
	cmd->cdw0.opc = NVME_OPC_CREATE_IO_CQ;

	/*
	 * TODO: create a create io completion queue command data
	 *  structure.
	 */
	cmd->cdw10 = sys_cpu_to_le32(((io_queue->num_entries-1) << 16) |
				     io_queue->id);
	/* 0x3 = interrupts enabled | physically contiguous */
	cmd->cdw11 = sys_cpu_to_le32((io_queue->vector << 16) | 0x3);
	cmd->dptr.prp1 = sys_cpu_to_le64(io_queue->cpl_bus_addr);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_create_io_sq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;
	struct nvme_command *cmd;

	request = nvme_allocate_request_null(cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	cmd = &request->cmd;
	cmd->cdw0.opc = NVME_OPC_CREATE_IO_SQ;

	/*
	 * TODO: create a create io submission queue command data
	 *  structure.
	 */
	cmd->cdw10 = sys_cpu_to_le32(((io_queue->num_entries - 1) << 16) |
				     io_queue->id);
	/* 0x1 = physically contiguous */
	cmd->cdw11 = sys_cpu_to_le32((io_queue->id << 16) | 0x1);
	cmd->dptr.prp1 = sys_cpu_to_le64(io_queue->cmd_bus_addr);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_delete_io_cq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;
	struct nvme_command *cmd;

	request = nvme_allocate_request_null(cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	cmd = &request->cmd;
	cmd->cdw0.opc = NVME_OPC_DELETE_IO_CQ;

	/*
	 * TODO: create a delete io completion queue command data
	 *  structure.
	 */
	cmd->cdw10 = sys_cpu_to_le32(io_queue->id);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_delete_io_sq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;
	struct nvme_command *cmd;

	request = nvme_allocate_request_null(cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	cmd = &request->cmd;
	cmd->cdw0.opc = NVME_OPC_DELETE_IO_SQ;

	/*
	 * TODO: create a delete io submission queue command data
	 *  structure.
	 */
	cmd->cdw10 = sys_cpu_to_le32(io_queue->id);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_set_feature(struct nvme_controller *ctrlr,
			       uint8_t feature, uint32_t cdw11,
			       uint32_t cdw12, uint32_t cdw13,
			       uint32_t cdw14, uint32_t cdw15,
			       void *payload, uint32_t payload_size,
			       nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;
	struct nvme_command *cmd;

	request = nvme_allocate_request_null(cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	cmd = &request->cmd;
	cmd->cdw0.opc = NVME_OPC_SET_FEATURES;
	cmd->cdw10 = sys_cpu_to_le32(feature);
	cmd->cdw11 = sys_cpu_to_le32(cdw11);
	cmd->cdw12 = sys_cpu_to_le32(cdw12);
	cmd->cdw13 = sys_cpu_to_le32(cdw13);
	cmd->cdw14 = sys_cpu_to_le32(cdw14);
	cmd->cdw15 = sys_cpu_to_le32(cdw15);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_get_feature(struct nvme_controller *ctrlr,
			       uint8_t feature, uint32_t cdw11,
			       void *payload, uint32_t payload_size,
			       nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;
	struct nvme_command *cmd;

	request = nvme_allocate_request_null(cb_fn, cb_arg);
	if (!request) {
		return -ENOMEM;
	}

	cmd = &request->cmd;
	cmd->cdw0.opc = NVME_OPC_GET_FEATURES;
	cmd->cdw10 = sys_cpu_to_le32(feature);
	cmd->cdw11 = sys_cpu_to_le32(cdw11);

	return nvme_cmd_qpair_submit_request(ctrlr->adminq, request);
}

int nvme_ctrlr_cmd_set_num_queues(struct nvme_controller *ctrlr,
				  uint32_t num_queues,
				  nvme_cb_fn_t cb_fn, void *cb_arg)
{
	uint32_t cdw11;

	cdw11 = ((num_queues - 1) << 16) | (num_queues - 1);

	return nvme_ctrlr_cmd_set_feature(ctrlr, NVME_FEAT_NUMBER_OF_QUEUES,
					  cdw11, 0, 0, 0, 0, NULL, 0,
					  cb_fn, cb_arg);
}
