/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Derived from FreeBSD original driver made by Jim Harris
 * with contributions from Alexander Motin, Wojciech Macek, and Warner Losh
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

#ifdef CONFIG_NVME_LOG_LEVEL_DBG
struct nvme_status_string {
	uint16_t   sc;
	const char *str;
};

static struct nvme_status_string generic_status[] = {
	{ NVME_SC_SUCCESS, "SUCCESS" },
	{ NVME_SC_INVALID_OPCODE, "INVALID OPCODE" },
	{ NVME_SC_INVALID_FIELD, "INVALID_FIELD" },
	{ NVME_SC_COMMAND_ID_CONFLICT, "COMMAND ID CONFLICT" },
	{ NVME_SC_DATA_TRANSFER_ERROR, "DATA TRANSFER ERROR" },
	{ NVME_SC_ABORTED_POWER_LOSS, "ABORTED - POWER LOSS" },
	{ NVME_SC_INTERNAL_DEVICE_ERROR, "INTERNAL DEVICE ERROR" },
	{ NVME_SC_ABORTED_BY_REQUEST, "ABORTED - BY REQUEST" },
	{ NVME_SC_ABORTED_SQ_DELETION, "ABORTED - SQ DELETION" },
	{ NVME_SC_ABORTED_FAILED_FUSED, "ABORTED - FAILED FUSED" },
	{ NVME_SC_ABORTED_MISSING_FUSED, "ABORTED - MISSING FUSED" },
	{ NVME_SC_INVALID_NAMESPACE_OR_FORMAT, "INVALID NAMESPACE OR FORMAT" },
	{ NVME_SC_COMMAND_SEQUENCE_ERROR, "COMMAND SEQUENCE ERROR" },
	{ NVME_SC_INVALID_SGL_SEGMENT_DESCR, "INVALID SGL SEGMENT DESCRIPTOR" },
	{ NVME_SC_INVALID_NUMBER_OF_SGL_DESCR, "INVALID NUMBER OF SGL DESCRIPTORS" },
	{ NVME_SC_DATA_SGL_LENGTH_INVALID, "DATA SGL LENGTH INVALID" },
	{ NVME_SC_METADATA_SGL_LENGTH_INVALID, "METADATA SGL LENGTH INVALID" },
	{ NVME_SC_SGL_DESCRIPTOR_TYPE_INVALID, "SGL DESCRIPTOR TYPE INVALID" },
	{ NVME_SC_INVALID_USE_OF_CMB, "INVALID USE OF CONTROLLER MEMORY BUFFER" },
	{ NVME_SC_PRP_OFFSET_INVALID, "PRP OFFSET INVALID" },
	{ NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED, "ATOMIC WRITE UNIT EXCEEDED" },
	{ NVME_SC_OPERATION_DENIED, "OPERATION DENIED" },
	{ NVME_SC_SGL_OFFSET_INVALID, "SGL OFFSET INVALID" },
	{ NVME_SC_HOST_ID_INCONSISTENT_FORMAT, "HOST IDENTIFIER INCONSISTENT FORMAT" },
	{ NVME_SC_KEEP_ALIVE_TIMEOUT_EXPIRED, "KEEP ALIVE TIMEOUT EXPIRED" },
	{ NVME_SC_KEEP_ALIVE_TIMEOUT_INVALID, "KEEP ALIVE TIMEOUT INVALID" },
	{ NVME_SC_ABORTED_DUE_TO_PREEMPT, "COMMAND ABORTED DUE TO PREEMPT AND ABORT" },
	{ NVME_SC_SANITIZE_FAILED, "SANITIZE FAILED" },
	{ NVME_SC_SANITIZE_IN_PROGRESS, "SANITIZE IN PROGRESS" },
	{ NVME_SC_SGL_DATA_BLOCK_GRAN_INVALID, "SGL_DATA_BLOCK_GRANULARITY_INVALID" },
	{ NVME_SC_NOT_SUPPORTED_IN_CMB, "COMMAND NOT SUPPORTED FOR QUEUE IN CMB" },
	{ NVME_SC_NAMESPACE_IS_WRITE_PROTECTED, "NAMESPACE IS WRITE PROTECTED" },
	{ NVME_SC_COMMAND_INTERRUPTED, "COMMAND INTERRUPTED" },
	{ NVME_SC_TRANSIENT_TRANSPORT_ERROR, "TRANSIENT TRANSPORT ERROR" },
	{ NVME_SC_LBA_OUT_OF_RANGE, "LBA OUT OF RANGE" },
	{ NVME_SC_CAPACITY_EXCEEDED, "CAPACITY EXCEEDED" },
	{ NVME_SC_NAMESPACE_NOT_READY, "NAMESPACE NOT READY" },
	{ NVME_SC_RESERVATION_CONFLICT, "RESERVATION CONFLICT" },
	{ NVME_SC_FORMAT_IN_PROGRESS, "FORMAT IN PROGRESS" },
	{ 0xFFFF, "GENERIC" }
};

static struct nvme_status_string command_specific_status[] = {
	{ NVME_SC_COMPLETION_QUEUE_INVALID, "INVALID COMPLETION QUEUE" },
	{ NVME_SC_INVALID_QUEUE_IDENTIFIER, "INVALID QUEUE IDENTIFIER" },
	{ NVME_SC_MAXIMUM_QUEUE_SIZE_EXCEEDED, "MAX QUEUE SIZE EXCEEDED" },
	{ NVME_SC_ABORT_COMMAND_LIMIT_EXCEEDED, "ABORT CMD LIMIT EXCEEDED" },
	{ NVME_SC_ASYNC_EVENT_REQUEST_LIMIT_EXCEEDED, "ASYNC LIMIT EXCEEDED" },
	{ NVME_SC_INVALID_FIRMWARE_SLOT, "INVALID FIRMWARE SLOT" },
	{ NVME_SC_INVALID_FIRMWARE_IMAGE, "INVALID FIRMWARE IMAGE" },
	{ NVME_SC_INVALID_INTERRUPT_VECTOR, "INVALID INTERRUPT VECTOR" },
	{ NVME_SC_INVALID_LOG_PAGE, "INVALID LOG PAGE" },
	{ NVME_SC_INVALID_FORMAT, "INVALID FORMAT" },
	{ NVME_SC_FIRMWARE_REQUIRES_RESET, "FIRMWARE REQUIRES RESET" },
	{ NVME_SC_INVALID_QUEUE_DELETION, "INVALID QUEUE DELETION" },
	{ NVME_SC_FEATURE_NOT_SAVEABLE, "FEATURE IDENTIFIER NOT SAVEABLE" },
	{ NVME_SC_FEATURE_NOT_CHANGEABLE, "FEATURE NOT CHANGEABLE" },
	{ NVME_SC_FEATURE_NOT_NS_SPECIFIC, "FEATURE NOT NAMESPACE SPECIFIC" },
	{ NVME_SC_FW_ACT_REQUIRES_NVMS_RESET, "FIRMWARE ACTIVATION REQUIRES NVM SUBSYSTEM RESET" },
	{ NVME_SC_FW_ACT_REQUIRES_RESET, "FIRMWARE ACTIVATION REQUIRES RESET" },
	{ NVME_SC_FW_ACT_REQUIRES_TIME, "FIRMWARE ACTIVATION REQUIRES MAXIMUM TIME VIOLATION" },
	{ NVME_SC_FW_ACT_PROHIBITED, "FIRMWARE ACTIVATION PROHIBITED" },
	{ NVME_SC_OVERLAPPING_RANGE, "OVERLAPPING RANGE" },
	{ NVME_SC_NS_INSUFFICIENT_CAPACITY, "NAMESPACE INSUFFICIENT CAPACITY" },
	{ NVME_SC_NS_ID_UNAVAILABLE, "NAMESPACE IDENTIFIER UNAVAILABLE" },
	{ NVME_SC_NS_ALREADY_ATTACHED, "NAMESPACE ALREADY ATTACHED" },
	{ NVME_SC_NS_IS_PRIVATE, "NAMESPACE IS PRIVATE" },
	{ NVME_SC_NS_NOT_ATTACHED, "NS NOT ATTACHED" },
	{ NVME_SC_THIN_PROV_NOT_SUPPORTED, "THIN PROVISIONING NOT SUPPORTED" },
	{ NVME_SC_CTRLR_LIST_INVALID, "CONTROLLER LIST INVALID" },
	{ NVME_SC_SELF_TEST_IN_PROGRESS, "DEVICE SELF-TEST IN PROGRESS" },
	{ NVME_SC_BOOT_PART_WRITE_PROHIB, "BOOT PARTITION WRITE PROHIBITED" },
	{ NVME_SC_INVALID_CTRLR_ID, "INVALID CONTROLLER IDENTIFIER" },
	{ NVME_SC_INVALID_SEC_CTRLR_STATE, "INVALID SECONDARY CONTROLLER STATE" },
	{ NVME_SC_INVALID_NUM_OF_CTRLR_RESRC, "INVALID NUMBER OF CONTROLLER RESOURCES" },
	{ NVME_SC_INVALID_RESOURCE_ID, "INVALID RESOURCE IDENTIFIER" },
	{ NVME_SC_SANITIZE_PROHIBITED_WPMRE,
	  "SANITIZE PROHIBITED WRITE PERSISTENT MEMORY REGION ENABLED" },
	{ NVME_SC_ANA_GROUP_ID_INVALID, "ANA GROUP IDENTIFIED INVALID" },
	{ NVME_SC_ANA_ATTACH_FAILED, "ANA ATTACH FAILED" },
	{ NVME_SC_CONFLICTING_ATTRIBUTES, "CONFLICTING ATTRIBUTES" },
	{ NVME_SC_INVALID_PROTECTION_INFO, "INVALID PROTECTION INFO" },
	{ NVME_SC_ATTEMPTED_WRITE_TO_RO_PAGE, "WRITE TO RO PAGE" },
	{ 0xFFFF, "COMMAND SPECIFIC" }
};

static struct nvme_status_string media_error_status[] = {
	{ NVME_SC_WRITE_FAULTS, "WRITE FAULTS" },
	{ NVME_SC_UNRECOVERED_READ_ERROR, "UNRECOVERED READ ERROR" },
	{ NVME_SC_GUARD_CHECK_ERROR, "GUARD CHECK ERROR" },
	{ NVME_SC_APPLICATION_TAG_CHECK_ERROR, "APPLICATION TAG CHECK ERROR" },
	{ NVME_SC_REFERENCE_TAG_CHECK_ERROR, "REFERENCE TAG CHECK ERROR" },
	{ NVME_SC_COMPARE_FAILURE, "COMPARE FAILURE" },
	{ NVME_SC_ACCESS_DENIED, "ACCESS DENIED" },
	{ NVME_SC_DEALLOCATED_OR_UNWRITTEN, "DEALLOCATED OR UNWRITTEN LOGICAL BLOCK" },
	{ 0xFFFF, "MEDIA ERROR" }
};

static struct nvme_status_string path_related_status[] = {
	{ NVME_SC_INTERNAL_PATH_ERROR, "INTERNAL PATH ERROR" },
	{ NVME_SC_ASYMMETRIC_ACCESS_PERSISTENT_LOSS, "ASYMMETRIC ACCESS PERSISTENT LOSS" },
	{ NVME_SC_ASYMMETRIC_ACCESS_INACCESSIBLE, "ASYMMETRIC ACCESS INACCESSIBLE" },
	{ NVME_SC_ASYMMETRIC_ACCESS_TRANSITION, "ASYMMETRIC ACCESS TRANSITION" },
	{ NVME_SC_CONTROLLER_PATHING_ERROR, "CONTROLLER PATHING ERROR" },
	{ NVME_SC_HOST_PATHING_ERROR, "HOST PATHING ERROR" },
	{ NVME_SC_COMMAND_ABORTED_BY_HOST, "COMMAND ABORTED BY HOST" },
	{ 0xFFFF, "PATH RELATED" },
};

static const char *get_status_string(uint16_t sct, uint16_t sc)
{
	struct nvme_status_string *entry;

	switch (sct) {
	case NVME_SCT_GENERIC:
		entry = generic_status;
		break;
	case NVME_SCT_COMMAND_SPECIFIC:
		entry = command_specific_status;
		break;
	case NVME_SCT_MEDIA_ERROR:
		entry = media_error_status;
		break;
	case NVME_SCT_PATH_RELATED:
		entry = path_related_status;
		break;
	case NVME_SCT_VENDOR_SPECIFIC:
		return "VENDOR SPECIFIC";
	default:
		return "RESERVED";
	}

	while (entry->sc != 0xFFFF) {
		if (entry->sc == sc) {
			return entry->str;
		}

		entry++;
	}
	return entry->str;
}

void nvme_completion_print(const struct nvme_completion *cpl)
{
	uint8_t sct, sc, crd, m, dnr, p;

	sct = NVME_STATUS_GET_SCT(cpl->status);
	sc = NVME_STATUS_GET_SC(cpl->status);
	crd = NVME_STATUS_GET_CRD(cpl->status);
	m = NVME_STATUS_GET_M(cpl->status);
	dnr = NVME_STATUS_GET_DNR(cpl->status);
	p = NVME_STATUS_GET_P(cpl->status);

	LOG_DBG("%s (%02x/%02x) crd:%x m:%x dnr:%x p:%d "
		"sqid:%d cid:%d cdw0:%x\n",
		get_status_string(sct, sc), sct, sc, crd, m, dnr, p,
		cpl->sqid, cpl->cid, cpl->cdw0);
}

#endif /* CONFIG_NVME_LOG_LEVEL_DBG */

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
		sys_cpu_to_le64((uint64_t)p_addr);
	request->cmd.dptr.prp2 =
		sys_cpu_to_le64((uint64_t)(uintptr_t)&prp_list->prp);
	p_addr = NVME_PRP_NEXT_PAGE(p_addr);

	for (idx = 0; idx < n_prp; idx++) {
		prp_list->prp[idx] = (uint64_t)sys_cpu_to_le64(p_addr);
		p_addr = NVME_PRP_NEXT_PAGE(p_addr);
	}

	request->prp_list = prp_list;

	return 0;
}

static int compute_n_prp(uintptr_t addr, uint32_t size)
{
	int n_prp;

	/* See Common Command Format, Data Pointer (DPTR) field */

	n_prp = size / CONFIG_NVME_PRP_PAGE_SIZE;
	if (n_prp == 0) {
		n_prp = 1;
	}

	if (size != CONFIG_NVME_PRP_PAGE_SIZE) {
		size = size % CONFIG_NVME_PRP_PAGE_SIZE;
	}

	if (n_prp == 1) {
		if ((addr + (uintptr_t)size) > NVME_PRP_NEXT_PAGE(addr)) {
			n_prp++;
		}
	} else if (size > 0) {
		n_prp++;
	}

	return n_prp;
}

static int nvme_cmd_qpair_fill_dptr(struct nvme_cmd_qpair *qpair,
				    struct nvme_request *request)
{
	switch (request->type) {
	case NVME_REQUEST_NULL:
		break;
	case NVME_REQUEST_VADDR: {
		int n_prp;

		if (request->payload_size > qpair->ctrlr->max_xfer_size) {
			LOG_ERR("VADDR request's payload too big");
			return -EINVAL;
		}

		n_prp = compute_n_prp((uintptr_t)request->payload,
				      request->payload_size);
		if (n_prp <= 2) {
			request->cmd.dptr.prp1 =
				sys_cpu_to_le64((uint64_t)(uintptr_t)request->payload);
			if (n_prp == 2) {
				request->cmd.dptr.prp2 = (uint64_t)sys_cpu_to_le64(
					NVME_PRP_NEXT_PAGE(
						(uintptr_t)request->payload));
			} else {
				request->cmd.dptr.prp2 = 0;
			}

			break;
		}

		return nvme_cmd_qpair_fill_prp_list(qpair, request, n_prp);
	}
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
