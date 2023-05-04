/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2022 Intel Corp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nvme, CONFIG_NVME_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

#include "nvme.h"
#include "nvme_helpers.h"

static struct nvme_prp_list prp_list_pool[CONFIG_NVME_PRP_LIST_AMOUNT];
static sys_dlist_t free_prp_list;

static struct nvme_request request_pool[NVME_REQUEST_AMOUNT];
static sys_dlist_t free_request;
static sys_dlist_t pending_request;

static void request_timeout(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(request_timer, request_timeout);

void nvme_cmd_init(void)
{
	int idx;

	sys_dlist_init(&free_request);
	sys_dlist_init(&pending_request);
	sys_dlist_init(&free_prp_list);

	for (idx = 0; idx < NVME_REQUEST_AMOUNT; idx++) {
		sys_dlist_append(&free_request, &request_pool[idx].node);
	}

	for (idx = 0; idx < CONFIG_NVME_PRP_LIST_AMOUNT; idx++) {
		sys_dlist_append(&free_prp_list, &prp_list_pool[idx].node);
	}
}

static struct nvme_prp_list *nvme_prp_list_alloc(void)
{
	sys_dnode_t *node;

	node = sys_dlist_peek_head(&free_prp_list);
	if (!node) {
		LOG_ERR("Could not allocate PRP list");
		return NULL;
	}

	sys_dlist_remove(node);

	return CONTAINER_OF(node, struct nvme_prp_list, node);
}

static void nvme_prp_list_free(struct nvme_prp_list *prp_list)
{
	memset(prp_list, 0, sizeof(struct nvme_prp_list));
	sys_dlist_append(&free_prp_list, &prp_list->node);
}

void nvme_cmd_request_free(struct nvme_request *request)
{
	if (sys_dnode_is_linked(&request->node)) {
		sys_dlist_remove(&request->node);
	}

	if (request->prp_list != NULL) {
		nvme_prp_list_free(request->prp_list);
	}

	memset(request, 0, sizeof(struct nvme_request));
	sys_dlist_append(&free_request, &request->node);
}

struct nvme_request *nvme_cmd_request_alloc(void)
{
	sys_dnode_t *node;

	node = sys_dlist_peek_head(&free_request);
	if (!node) {
		LOG_ERR("Could not allocate request");
		return NULL;
	}

	sys_dlist_remove(node);

	return CONTAINER_OF(node, struct nvme_request, node);
}

static void nvme_cmd_register_request(struct nvme_request *request)
{
	sys_dlist_append(&pending_request, &request->node);

	request->req_start = k_uptime_get_32();

	if (!k_work_delayable_remaining_get(&request_timer)) {
		k_work_reschedule(&request_timer,
				  K_SECONDS(CONFIG_NVME_REQUEST_TIMEOUT));
	}
}

static void request_timeout(struct k_work *work)
{
	uint32_t current = k_uptime_get_32();
	struct nvme_request *request, *next;

	ARG_UNUSED(work);

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&pending_request,
					  request, next, node) {
		if ((int32_t)(request->req_start +
			      CONFIG_NVME_REQUEST_TIMEOUT - current) > 0) {
			break;
		}

		LOG_WRN("Request %p CID %u timed-out",
			request, request->cmd.cdw0.cid);

		/* ToDo:
		 * - check CSTS for fatal fault
		 * - reset hw otherwise if it's the case
		 * - or check completion for missed interruption
		 */

		if (request->cb_fn) {
			request->cb_fn(request->cb_arg, NULL);
		}

		nvme_cmd_request_free(request);
	}

	if (request) {
		k_work_reschedule(&request_timer,
				  K_SECONDS(request->req_start +
					    CONFIG_NVME_REQUEST_TIMEOUT -
					    current));
	}
}

static bool nvme_completion_is_retry(const struct nvme_completion *cpl)
{
	uint8_t sct, sc, dnr;

	sct = NVME_STATUS_GET_SCT(cpl->status);
	sc = NVME_STATUS_GET_SC(cpl->status);
	dnr = NVME_STATUS_GET_DNR(cpl->status);

	/*
	 * TODO: spec is not clear how commands that are aborted due
	 *  to TLER will be marked.  So for now, it seems
	 *  NAMESPACE_NOT_READY is the only case where we should
	 *  look at the DNR bit. Requests failed with ABORTED_BY_REQUEST
	 *  set the DNR bit correctly since the driver controls that.
	 */
	switch (sct) {
	case NVME_SCT_GENERIC:
		switch (sc) {
		case NVME_SC_ABORTED_BY_REQUEST:
		case NVME_SC_NAMESPACE_NOT_READY:
			if (dnr) {
				return false;
			}

			return true;
		case NVME_SC_INVALID_OPCODE:
		case NVME_SC_INVALID_FIELD:
		case NVME_SC_COMMAND_ID_CONFLICT:
		case NVME_SC_DATA_TRANSFER_ERROR:
		case NVME_SC_ABORTED_POWER_LOSS:
		case NVME_SC_INTERNAL_DEVICE_ERROR:
		case NVME_SC_ABORTED_SQ_DELETION:
		case NVME_SC_ABORTED_FAILED_FUSED:
		case NVME_SC_ABORTED_MISSING_FUSED:
		case NVME_SC_INVALID_NAMESPACE_OR_FORMAT:
		case NVME_SC_COMMAND_SEQUENCE_ERROR:
		case NVME_SC_LBA_OUT_OF_RANGE:
		case NVME_SC_CAPACITY_EXCEEDED:
		default:
			return false;
		}
	case NVME_SCT_COMMAND_SPECIFIC:
	case NVME_SCT_MEDIA_ERROR:
		return false;
	case NVME_SCT_PATH_RELATED:
		switch (sc) {
		case NVME_SC_INTERNAL_PATH_ERROR:
			if (dnr) {
				return false;
			}

			return true;
		default:
			return false;
		}
	case NVME_SCT_VENDOR_SPECIFIC:
	default:
		return false;
	}
}

static void nvme_cmd_request_complete(struct nvme_request *request,
				      struct nvme_completion *cpl)
{
	bool error, retriable, retry;

	error = nvme_completion_is_error(cpl);
	retriable = nvme_completion_is_retry(cpl);
	retry = error && retriable &&
		request->retries < CONFIG_NVME_RETRY_COUNT;

	if (retry) {
		LOG_DBG("CMD will be retried");
		request->qpair->num_retries++;
	}

	if (error &&
	    (!retriable || (request->retries >= CONFIG_NVME_RETRY_COUNT))) {
		LOG_DBG("CMD error");
		request->qpair->num_failures++;
	}

	if (cpl->cid != request->cmd.cdw0.cid) {
		LOG_ERR("cpl cid != cmd cid");
	}

	if (retry) {
		LOG_DBG("Retrying CMD");
		/* Let's remove it from pending... */
		sys_dlist_remove(&request->node);
		/* ...and re-submit, thus re-adding to pending */
		nvme_cmd_qpair_submit_request(request->qpair, request);
		request->retries++;
	} else {
		LOG_DBG("Request %p CMD complete on %p/%p",
			request, request->cb_fn, request->cb_arg);

		if (request->cb_fn) {
			request->cb_fn(request->cb_arg, cpl);
		}

		nvme_cmd_request_free(request);
	}
}

static void nvme_cmd_qpair_process_completion(struct nvme_cmd_qpair *qpair)
{
	struct nvme_request *request;
	struct nvme_completion cpl;
	int done = 0;

	if (qpair->num_intr_handler_calls == 0 && qpair->phase == 0) {
		LOG_WRN("Phase wrong for first interrupt call.");
	}

	qpair->num_intr_handler_calls++;

	while (1) {
		uint16_t status;

		status = sys_le16_to_cpu(qpair->cpl[qpair->cq_head].status);
		if (NVME_STATUS_GET_P(status) != qpair->phase) {
			break;
		}

		cpl = qpair->cpl[qpair->cq_head];
		nvme_completion_swapbytes(&cpl);

		if (NVME_STATUS_GET_P(status) != NVME_STATUS_GET_P(cpl.status)) {
			LOG_WRN("Phase unexpectedly inconsistent");
		}

		if (cpl.cid < NVME_REQUEST_AMOUNT) {
			request = &request_pool[cpl.cid];
		} else {
			request = NULL;
		}

		done++;
		if (request != NULL) {
			nvme_cmd_request_complete(request, &cpl);
			qpair->sq_head = cpl.sqhd;
		} else {
			LOG_ERR("cpl (cid = %u) does not map to cmd", cpl.cid);
		}

		qpair->cq_head++;
		if (qpair->cq_head == qpair->num_entries) {
			qpair->cq_head = 0;
			qpair->phase = !qpair->phase;
		}
	}

	if (done != 0) {
		mm_reg_t regs = DEVICE_MMIO_GET(qpair->ctrlr->dev);

		sys_write32(qpair->cq_head, regs + qpair->cq_hdbl_off);
	}
}

static void nvme_cmd_qpair_msi_handler(const void *arg)
{
	const struct nvme_cmd_qpair *qpair = arg;

	nvme_cmd_qpair_process_completion((struct nvme_cmd_qpair *)qpair);
}

int nvme_cmd_qpair_setup(struct nvme_cmd_qpair *qpair,
			 struct nvme_controller *ctrlr,
			 uint32_t id)
{
	const struct nvme_controller_config *nvme_ctrlr_cfg =
		ctrlr->dev->config;

	qpair->ctrlr = ctrlr;
	qpair->id = id;
	qpair->vector = qpair->id;

	qpair->num_cmds = 0;
	qpair->num_intr_handler_calls = 0;
	qpair->num_retries = 0;
	qpair->num_failures = 0;
	qpair->num_ignored = 0;

	qpair->cmd_bus_addr = (uintptr_t)qpair->cmd;
	qpair->cpl_bus_addr = (uintptr_t)qpair->cpl;

	qpair->sq_tdbl_off = nvme_mmio_offsetof(doorbell) +
		(qpair->id << (ctrlr->dstrd + 1));
	qpair->cq_hdbl_off = nvme_mmio_offsetof(doorbell) +
		(qpair->id << (ctrlr->dstrd + 1)) + (1 << ctrlr->dstrd);

	if (!pcie_msi_vector_connect(nvme_ctrlr_cfg->pcie->bdf,
				     &ctrlr->vectors[qpair->vector],
				     nvme_cmd_qpair_msi_handler, qpair, 0)) {
		LOG_ERR("Failed to connect MSI-X vector %u", qpair->id);
		return -EIO;
	}

	LOG_DBG("CMD Qpair created ID %u, %u entries - cmd/cpl addr "
		"0x%lx/0x%lx - sq/cq offsets %u/%u",
		qpair->id, qpair->num_entries, qpair->cmd_bus_addr,
		qpair->cpl_bus_addr, qpair->sq_tdbl_off, qpair->cq_hdbl_off);

	return 0;
}

void nvme_cmd_qpair_reset(struct nvme_cmd_qpair *qpair)
{
	qpair->sq_head = qpair->sq_tail = qpair->cq_head = 0;

	/*
	 * First time through the completion queue, HW will set phase
	 * bit on completions to 1.  So set this to 1 here, indicating
	 * we're looking for a 1 to know which entries have completed.
	 * we'll toggle the bit each time when the completion queue
	 * rolls over.
	 */
	qpair->phase = 1;

	memset(qpair->cmd, 0,
	       qpair->num_entries * sizeof(struct nvme_command));
	memset(qpair->cpl, 0,
	       qpair->num_entries * sizeof(struct nvme_completion));
}

static int nvme_cmd_qpair_fill_prp_list(struct nvme_cmd_qpair *qpair,
					struct nvme_request *request,
					int n_prp)
{
	struct nvme_prp_list *prp_list;
	uintptr_t p_addr;
	int idx;

	prp_list = nvme_prp_list_alloc();
	if (prp_list == NULL) {
		return -ENOMEM;
	}

	p_addr = (uintptr_t)request->payload;
	request->cmd.dptr.prp1 =
		(uint64_t)sys_cpu_to_le64(p_addr);
	request->cmd.dptr.prp2 =
		(uint64_t)sys_cpu_to_le64(&prp_list->prp);
	p_addr = NVME_PRP_NEXT_PAGE(p_addr);

	for (idx = 0; idx < n_prp; idx++) {
		prp_list->prp[idx] = (uint64_t)sys_cpu_to_le64(p_addr);
		p_addr = NVME_PRP_NEXT_PAGE(p_addr);
	}

	request->prp_list = prp_list;

	return 0;
}

static int nvme_cmd_qpair_fill_dptr(struct nvme_cmd_qpair *qpair,
				    struct nvme_request *request)
{
	switch (request->type) {
	case NVME_REQUEST_NULL:
		break;
	case NVME_REQUEST_VADDR:
		int n_prp;

		if (request->payload_size > qpair->ctrlr->max_xfer_size) {
			LOG_ERR("VADDR request's payload too big");
			return -EINVAL;
		}

		n_prp = request->payload_size / qpair->ctrlr->page_size;
		if ((request->payload_size % qpair->ctrlr->page_size) ||
		    ((uintptr_t)request->payload & NVME_PBAO_MASK)) {
			n_prp++;
		}

		if (n_prp <= 2) {
			request->cmd.dptr.prp1 =
				(uint64_t)sys_cpu_to_le64(request->payload);
			if ((uintptr_t)request->payload & NVME_PBAO_MASK) {
				request->cmd.dptr.prp2 =
					NVME_PRP_NEXT_PAGE(
						(uintptr_t)request->payload);
			} else {
				request->cmd.dptr.prp2 = 0;
			}

			break;
		}

		return nvme_cmd_qpair_fill_prp_list(qpair, request, n_prp);
	default:
		break;
	}

	return 0;
}

int nvme_cmd_qpair_submit_request(struct nvme_cmd_qpair *qpair,
				  struct nvme_request *request)
{
	mm_reg_t regs = DEVICE_MMIO_GET(qpair->ctrlr->dev);
	int ret;

	request->qpair = qpair;

	request->cmd.cdw0.cid = sys_cpu_to_le16((uint16_t)(request -
							   request_pool));

	ret = nvme_cmd_qpair_fill_dptr(qpair, request);
	if (ret != 0) {
		nvme_cmd_request_free(request);
		return ret;
	}

	nvme_cmd_register_request(request);

	memcpy(&qpair->cmd[qpair->sq_tail],
	       &request->cmd, sizeof(request->cmd));

	qpair->sq_tail++;
	if (qpair->sq_tail == qpair->num_entries) {
		qpair->sq_tail = 0;
	}

	sys_write32(qpair->sq_tail, regs + qpair->sq_tdbl_off);
	qpair->num_cmds++;

	LOG_DBG("Request %p %llu submitted: CID %u - sq_tail %u",
		request, qpair->num_cmds, request->cmd.cdw0.cid,
		qpair->sq_tail - 1);
	return 0;
}

void
nvme_completion_poll_cb(void *arg, const struct nvme_completion *cpl)
{
	struct nvme_completion_poll_status *status = arg;

	if (cpl != NULL) {
		memcpy(&status->cpl, cpl, sizeof(*cpl));
	} else {
		status->status = -ETIMEDOUT;
	}

	k_sem_give(&status->sem);
}
