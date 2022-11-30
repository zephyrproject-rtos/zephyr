/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2022 Intel Corp.
 */

#ifndef ZEPHYR_DRIVERS_DISK_NVME_NVME_COMMAND_H_
#define ZEPHYR_DRIVERS_DISK_NVME_NVME_COMMAND_H_

#include <zephyr/sys/slist.h>

struct nvme_command {
	/* dword 0 */
	struct _cdw0 {
		uint8_t opc;		/* opcode */
		uint8_t fuse : 2;	/* fused operation */
		uint8_t rsvd : 4;	/* reserved */
		uint8_t psdt : 2;       /* PRP or SGL for Data Transfer */
		uint16_t cid;		/* command identifier */
	} cdw0;

	/* dword 1 */
	uint32_t nsid;		/* namespace identifier */

	/* dword 2-3 */
	uint32_t cdw2;
	uint32_t cdw3;

	/* dword 4-5 */
	uint64_t mptr;		/* metadata pointer */

	/* dword 6-7 and 8-9 */
	struct _dptr {
		uint64_t prp1;	/* prp entry 1 */
		uint64_t prp2;	/* prp entry 2 */
	} dptr;			/* data pointer */

	/* dword 10 */
	union {
		uint32_t cdw10;	/* command-specific */
		uint32_t ndt;	/* Number of Dwords in Data transfer */
	};

	/* dword 11 */
	union {
		uint32_t cdw11;	/* command-specific */
		uint32_t ndm;	/* Number of Dwords in Metadata transfer */
	};

	/* dword 12-15 */
	uint32_t cdw12;		/* command-specific */
	uint32_t cdw13;		/* command-specific */
	uint32_t cdw14;		/* command-specific */
	uint32_t cdw15;		/* command-specific */
};

struct nvme_completion {
	/* dword 0 */
	uint32_t	cdw0;	/* command-specific */

	/* dword 1 */
	uint32_t	rsvd;

	/* dword 2 */
	uint16_t	sqhd;	/* submission queue head pointer */
	uint16_t	sqid;	/* submission queue identifier */

	/* dword 3 */
	uint16_t	cid;	/* command identifier */
	uint16_t	status;
} __aligned(8);

struct nvme_completion_poll_status {
	int status;
	struct nvme_completion cpl;
	struct k_sem sem;
};

/* status code types */
enum nvme_status_code_type {
	NVME_SCT_GENERIC		= 0x0,
	NVME_SCT_COMMAND_SPECIFIC	= 0x1,
	NVME_SCT_MEDIA_ERROR		= 0x2,
	NVME_SCT_PATH_RELATED		= 0x3,
	/* 0x3-0x6 - reserved */
	NVME_SCT_VENDOR_SPECIFIC	= 0x7,
};

/* generic command status codes */
enum nvme_generic_command_status_code {
	NVME_SC_SUCCESS				= 0x00,
	NVME_SC_INVALID_OPCODE			= 0x01,
	NVME_SC_INVALID_FIELD			= 0x02,
	NVME_SC_COMMAND_ID_CONFLICT		= 0x03,
	NVME_SC_DATA_TRANSFER_ERROR		= 0x04,
	NVME_SC_ABORTED_POWER_LOSS		= 0x05,
	NVME_SC_INTERNAL_DEVICE_ERROR		= 0x06,
	NVME_SC_ABORTED_BY_REQUEST		= 0x07,
	NVME_SC_ABORTED_SQ_DELETION		= 0x08,
	NVME_SC_ABORTED_FAILED_FUSED		= 0x09,
	NVME_SC_ABORTED_MISSING_FUSED		= 0x0a,
	NVME_SC_INVALID_NAMESPACE_OR_FORMAT	= 0x0b,
	NVME_SC_COMMAND_SEQUENCE_ERROR		= 0x0c,
	NVME_SC_INVALID_SGL_SEGMENT_DESCR	= 0x0d,
	NVME_SC_INVALID_NUMBER_OF_SGL_DESCR	= 0x0e,
	NVME_SC_DATA_SGL_LENGTH_INVALID		= 0x0f,
	NVME_SC_METADATA_SGL_LENGTH_INVALID	= 0x10,
	NVME_SC_SGL_DESCRIPTOR_TYPE_INVALID	= 0x11,
	NVME_SC_INVALID_USE_OF_CMB		= 0x12,
	NVME_SC_PRP_OFFSET_INVALID		= 0x13,
	NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED	= 0x14,
	NVME_SC_OPERATION_DENIED		= 0x15,
	NVME_SC_SGL_OFFSET_INVALID		= 0x16,
	/* 0x17 - reserved */
	NVME_SC_HOST_ID_INCONSISTENT_FORMAT	= 0x18,
	NVME_SC_KEEP_ALIVE_TIMEOUT_EXPIRED	= 0x19,
	NVME_SC_KEEP_ALIVE_TIMEOUT_INVALID	= 0x1a,
	NVME_SC_ABORTED_DUE_TO_PREEMPT		= 0x1b,
	NVME_SC_SANITIZE_FAILED			= 0x1c,
	NVME_SC_SANITIZE_IN_PROGRESS		= 0x1d,
	NVME_SC_SGL_DATA_BLOCK_GRAN_INVALID	= 0x1e,
	NVME_SC_NOT_SUPPORTED_IN_CMB		= 0x1f,
	NVME_SC_NAMESPACE_IS_WRITE_PROTECTED	= 0x20,
	NVME_SC_COMMAND_INTERRUPTED		= 0x21,
	NVME_SC_TRANSIENT_TRANSPORT_ERROR	= 0x22,

	NVME_SC_LBA_OUT_OF_RANGE		= 0x80,
	NVME_SC_CAPACITY_EXCEEDED		= 0x81,
	NVME_SC_NAMESPACE_NOT_READY		= 0x82,
	NVME_SC_RESERVATION_CONFLICT		= 0x83,
	NVME_SC_FORMAT_IN_PROGRESS		= 0x84,
};

/* command specific status codes */
enum nvme_command_specific_status_code {
	NVME_SC_COMPLETION_QUEUE_INVALID	= 0x00,
	NVME_SC_INVALID_QUEUE_IDENTIFIER	= 0x01,
	NVME_SC_MAXIMUM_QUEUE_SIZE_EXCEEDED	= 0x02,
	NVME_SC_ABORT_COMMAND_LIMIT_EXCEEDED	= 0x03,
	/* 0x04 - reserved */
	NVME_SC_ASYNC_EVENT_REQUEST_LIMIT_EXCEEDED = 0x05,
	NVME_SC_INVALID_FIRMWARE_SLOT		= 0x06,
	NVME_SC_INVALID_FIRMWARE_IMAGE		= 0x07,
	NVME_SC_INVALID_INTERRUPT_VECTOR	= 0x08,
	NVME_SC_INVALID_LOG_PAGE		= 0x09,
	NVME_SC_INVALID_FORMAT			= 0x0a,
	NVME_SC_FIRMWARE_REQUIRES_RESET		= 0x0b,
	NVME_SC_INVALID_QUEUE_DELETION		= 0x0c,
	NVME_SC_FEATURE_NOT_SAVEABLE		= 0x0d,
	NVME_SC_FEATURE_NOT_CHANGEABLE		= 0x0e,
	NVME_SC_FEATURE_NOT_NS_SPECIFIC		= 0x0f,
	NVME_SC_FW_ACT_REQUIRES_NVMS_RESET	= 0x10,
	NVME_SC_FW_ACT_REQUIRES_RESET		= 0x11,
	NVME_SC_FW_ACT_REQUIRES_TIME		= 0x12,
	NVME_SC_FW_ACT_PROHIBITED		= 0x13,
	NVME_SC_OVERLAPPING_RANGE		= 0x14,
	NVME_SC_NS_INSUFFICIENT_CAPACITY	= 0x15,
	NVME_SC_NS_ID_UNAVAILABLE		= 0x16,
	/* 0x17 - reserved */
	NVME_SC_NS_ALREADY_ATTACHED		= 0x18,
	NVME_SC_NS_IS_PRIVATE			= 0x19,
	NVME_SC_NS_NOT_ATTACHED			= 0x1a,
	NVME_SC_THIN_PROV_NOT_SUPPORTED		= 0x1b,
	NVME_SC_CTRLR_LIST_INVALID		= 0x1c,
	NVME_SC_SELF_TEST_IN_PROGRESS		= 0x1d,
	NVME_SC_BOOT_PART_WRITE_PROHIB		= 0x1e,
	NVME_SC_INVALID_CTRLR_ID		= 0x1f,
	NVME_SC_INVALID_SEC_CTRLR_STATE		= 0x20,
	NVME_SC_INVALID_NUM_OF_CTRLR_RESRC	= 0x21,
	NVME_SC_INVALID_RESOURCE_ID		= 0x22,
	NVME_SC_SANITIZE_PROHIBITED_WPMRE	= 0x23,
	NVME_SC_ANA_GROUP_ID_INVALID		= 0x24,
	NVME_SC_ANA_ATTACH_FAILED		= 0x25,

	NVME_SC_CONFLICTING_ATTRIBUTES		= 0x80,
	NVME_SC_INVALID_PROTECTION_INFO		= 0x81,
	NVME_SC_ATTEMPTED_WRITE_TO_RO_PAGE	= 0x82,
};

/* media error status codes */
enum nvme_media_error_status_code {
	NVME_SC_WRITE_FAULTS			= 0x80,
	NVME_SC_UNRECOVERED_READ_ERROR		= 0x81,
	NVME_SC_GUARD_CHECK_ERROR		= 0x82,
	NVME_SC_APPLICATION_TAG_CHECK_ERROR	= 0x83,
	NVME_SC_REFERENCE_TAG_CHECK_ERROR	= 0x84,
	NVME_SC_COMPARE_FAILURE			= 0x85,
	NVME_SC_ACCESS_DENIED			= 0x86,
	NVME_SC_DEALLOCATED_OR_UNWRITTEN	= 0x87,
};

/* path related status codes */
enum nvme_path_related_status_code {
	NVME_SC_INTERNAL_PATH_ERROR		= 0x00,
	NVME_SC_ASYMMETRIC_ACCESS_PERSISTENT_LOSS = 0x01,
	NVME_SC_ASYMMETRIC_ACCESS_INACCESSIBLE	= 0x02,
	NVME_SC_ASYMMETRIC_ACCESS_TRANSITION	= 0x03,
	NVME_SC_CONTROLLER_PATHING_ERROR	= 0x60,
	NVME_SC_HOST_PATHING_ERROR		= 0x70,
	NVME_SC_COMMAND_ABOTHED_BY_HOST		= 0x71,
};

/* admin opcodes */
enum nvme_admin_opcode {
	NVME_OPC_DELETE_IO_SQ			= 0x00,
	NVME_OPC_CREATE_IO_SQ			= 0x01,
	NVME_OPC_GET_LOG_PAGE			= 0x02,
	/* 0x03 - reserved */
	NVME_OPC_DELETE_IO_CQ			= 0x04,
	NVME_OPC_CREATE_IO_CQ			= 0x05,
	NVME_OPC_IDENTIFY			= 0x06,
	/* 0x07 - reserved */
	NVME_OPC_ABORT				= 0x08,
	NVME_OPC_SET_FEATURES			= 0x09,
	NVME_OPC_GET_FEATURES			= 0x0a,
	/* 0x0b - reserved */
	NVME_OPC_ASYNC_EVENT_REQUEST		= 0x0c,
	NVME_OPC_NAMESPACE_MANAGEMENT		= 0x0d,
	/* 0x0e-0x0f - reserved */
	NVME_OPC_FIRMWARE_ACTIVATE		= 0x10,
	NVME_OPC_FIRMWARE_IMAGE_DOWNLOAD	= 0x11,
	/* 0x12-0x13 - reserved */
	NVME_OPC_DEVICE_SELF_TEST		= 0x14,
	NVME_OPC_NAMESPACE_ATTACHMENT		= 0x15,
	/* 0x16-0x17 - reserved */
	NVME_OPC_KEEP_ALIVE			= 0x18,
	NVME_OPC_DIRECTIVE_SEND			= 0x19,
	NVME_OPC_DIRECTIVE_RECEIVE		= 0x1a,
	/* 0x1b - reserved */
	NVME_OPC_VIRTUALIZATION_MANAGEMENT	= 0x1c,
	NVME_OPC_NVME_MI_SEND			= 0x1d,
	NVME_OPC_NVME_MI_RECEIVE		= 0x1e,
	/* 0x1f-0x7b - reserved */
	NVME_OPC_DOORBELL_BUFFER_CONFIG		= 0x7c,

	NVME_OPC_FORMAT_NVM			= 0x80,
	NVME_OPC_SECURITY_SEND			= 0x81,
	NVME_OPC_SECURITY_RECEIVE		= 0x82,
	/* 0x83 - reserved */
	NVME_OPC_SANITIZE			= 0x84,
	/* 0x85 - reserved */
	NVME_OPC_GET_LBA_STATUS			= 0x86,
};

/* nvme nvm opcodes */
enum nvme_nvm_opcode {
	NVME_OPC_FLUSH				= 0x00,
	NVME_OPC_WRITE				= 0x01,
	NVME_OPC_READ				= 0x02,
	/* 0x03 - reserved */
	NVME_OPC_WRITE_UNCORRECTABLE		= 0x04,
	NVME_OPC_COMPARE			= 0x05,
	/* 0x06-0x07 - reserved */
	NVME_OPC_WRITE_ZEROES			= 0x08,
	NVME_OPC_DATASET_MANAGEMENT		= 0x09,
	/* 0x0a-0x0b - reserved */
	NVME_OPC_VERIFY				= 0x0c,
	NVME_OPC_RESERVATION_REGISTER		= 0x0d,
	NVME_OPC_RESERVATION_REPORT		= 0x0e,
	/* 0x0f-0x10 - reserved */
	NVME_OPC_RESERVATION_ACQUIRE		= 0x11,
	/* 0x12-0x14 - reserved */
	NVME_OPC_RESERVATION_RELEASE		= 0x15,
};

enum nvme_feature {
	/* 0x00 - reserved */
	NVME_FEAT_ARBITRATION			= 0x01,
	NVME_FEAT_POWER_MANAGEMENT		= 0x02,
	NVME_FEAT_LBA_RANGE_TYPE		= 0x03,
	NVME_FEAT_TEMPERATURE_THRESHOLD		= 0x04,
	NVME_FEAT_ERROR_RECOVERY		= 0x05,
	NVME_FEAT_VOLATILE_WRITE_CACHE		= 0x06,
	NVME_FEAT_NUMBER_OF_QUEUES		= 0x07,
	NVME_FEAT_INTERRUPT_COALESCING		= 0x08,
	NVME_FEAT_INTERRUPT_VECTOR_CONFIGURATION = 0x09,
	NVME_FEAT_WRITE_ATOMICITY		= 0x0A,
	NVME_FEAT_ASYNC_EVENT_CONFIGURATION	= 0x0B,
	NVME_FEAT_AUTONOMOUS_POWER_STATE_TRANSITION = 0x0C,
	NVME_FEAT_HOST_MEMORY_BUFFER		= 0x0D,
	NVME_FEAT_TIMESTAMP			= 0x0E,
	NVME_FEAT_KEEP_ALIVE_TIMER		= 0x0F,
	NVME_FEAT_HOST_CONTROLLED_THERMAL_MGMT	= 0x10,
	NVME_FEAT_NON_OP_POWER_STATE_CONFIG	= 0x11,
	NVME_FEAT_READ_RECOVERY_LEVEL_CONFIG	= 0x12,
	NVME_FEAT_PREDICTABLE_LATENCY_MODE_CONFIG = 0x13,
	NVME_FEAT_PREDICTABLE_LATENCY_MODE_WINDOW = 0x14,
	NVME_FEAT_LBA_STATUS_INFORMATION_ATTRIBUTES = 0x15,
	NVME_FEAT_HOST_BEHAVIOR_SUPPORT		= 0x16,
	NVME_FEAT_SANITIZE_CONFIG		= 0x17,
	NVME_FEAT_ENDURANCE_GROUP_EVENT_CONFIGURATION = 0x18,
	/* 0x19-0x77 - reserved */
	/* 0x78-0x7f - NVMe Management Interface */
	NVME_FEAT_SOFTWARE_PROGRESS_MARKER	= 0x80,
	NVME_FEAT_HOST_IDENTIFIER		= 0x81,
	NVME_FEAT_RESERVATION_NOTIFICATION_MASK	= 0x82,
	NVME_FEAT_RESERVATION_PERSISTENCE	= 0x83,
	NVME_FEAT_NAMESPACE_WRITE_PROTECTION_CONFIG = 0x84,
	/* 0x85-0xBF - command set specific (reserved) */
	/* 0xC0-0xFF - vendor specific */
};

#if !defined(CONFIG_DCACHE_LINE_SIZE) || (CONFIG_DCACHE_LINE_SIZE == 0)
#define CACHE_LINE_SIZE				(64)
#else
#define CACHE_LINE_SIZE				CONFIG_DCACHE_LINE_SIZE
#endif

/* Assuming page size it always 4Kib
 * ToDo: define it accorditng to CONFIG_MMU_PAGE_SIZE
 */
#define NVME_PBAO_MASK 0xFFF

#define NVME_PRP_NEXT_PAGE(_addr) ((_addr & (~NVME_PBAO_MASK)) + 0x1000)

struct nvme_prp_list {
	uintptr_t prp[CONFIG_MMU_PAGE_SIZE / sizeof(uintptr_t)]
						__aligned(0x1000);
	sys_dnode_t node;
};

struct nvme_cmd_qpair {
	struct nvme_controller	*ctrlr;
	uint32_t		id;

	uint32_t		num_entries;

	uint32_t		sq_tdbl_off;
	uint32_t		cq_hdbl_off;

	uint32_t		phase;
	uint32_t		sq_head;
	uint32_t		sq_tail;
	uint32_t		cq_head;

	int64_t			num_cmds;
	int64_t			num_intr_handler_calls;
	int64_t			num_retries;
	int64_t			num_failures;
	int64_t			num_ignored;

	struct nvme_command	*cmd;
	struct nvme_completion	*cpl;

	uintptr_t		cmd_bus_addr;
	uintptr_t		cpl_bus_addr;

	uint16_t		vector;
} __aligned(CACHE_LINE_SIZE);

typedef void (*nvme_cb_fn_t)(void *, const struct nvme_completion *);

enum nvme_request_type {
	NVME_REQUEST_NULL	= 1,
	NVME_REQUEST_VADDR	= 2,
};

struct nvme_request {
	struct nvme_command		cmd;
	struct nvme_cmd_qpair		*qpair;

	uint32_t			type;
	uint32_t			req_start;
	int32_t				retries;

	void				*payload;
	uint32_t			payload_size;
	nvme_cb_fn_t			cb_fn;
	void				*cb_arg;

	struct nvme_prp_list		*prp_list;

	sys_dnode_t			node;
};

void nvme_cmd_init(void);

void nvme_completion_poll_cb(void *arg, const struct nvme_completion *cpl);

void nvme_cmd_request_free(struct nvme_request *request);

struct nvme_request *nvme_cmd_request_alloc(void);

int nvme_cmd_qpair_setup(struct nvme_cmd_qpair *qpair,
			 struct nvme_controller *ctrlr,
			 uint32_t id);

void nvme_cmd_qpair_reset(struct nvme_cmd_qpair *qpair);

int nvme_cmd_qpair_submit_request(struct nvme_cmd_qpair *qpair,
				  struct nvme_request *request);

int nvme_cmd_identify_controller(struct nvme_controller *ctrlr,
				 void *payload,
				 nvme_cb_fn_t cb_fn,
				 void *cb_arg);

int nvme_ctrlr_cmd_identify_controller(struct nvme_controller *ctrlr,
				       nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_identify_namespace(struct nvme_controller *ctrlr,
				      uint32_t nsid, void *payload,
				      nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_create_io_cq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_create_io_sq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_delete_io_cq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_delete_io_sq(struct nvme_controller *ctrlr,
				struct nvme_cmd_qpair *io_queue,
				nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_set_feature(struct nvme_controller *ctrlr,
			       uint8_t feature, uint32_t cdw11,
			       uint32_t cdw12, uint32_t cdw13,
			       uint32_t cdw14, uint32_t cdw15,
			       void *payload, uint32_t payload_size,
			       nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_get_feature(struct nvme_controller *ctrlr,
			       uint8_t feature, uint32_t cdw11,
			       void *payload, uint32_t payload_size,
			       nvme_cb_fn_t cb_fn, void *cb_arg);

int nvme_ctrlr_cmd_set_num_queues(struct nvme_controller *ctrlr,
				  uint32_t num_queues,
				  nvme_cb_fn_t cb_fn, void *cb_arg);

static inline
struct nvme_request *nvme_allocate_request(nvme_cb_fn_t cb_fn, void *cb_arg)
{
	struct nvme_request *request;

	request = nvme_cmd_request_alloc();
	if (request != NULL) {
		request->cb_fn = cb_fn;
		request->cb_arg = cb_arg;
	}

	return request;
}

static inline
struct nvme_request *nvme_allocate_request_vaddr(void *payload,
						 uint32_t payload_size,
						 nvme_cb_fn_t cb_fn,
						 void *cb_arg)
{
	struct nvme_request *request;

	request = nvme_allocate_request(cb_fn, cb_arg);
	if (request != NULL) {
		request->type = NVME_REQUEST_VADDR;
		request->payload = payload;
		request->payload_size = payload_size;
	}

	return request;
}


static inline
struct nvme_request *nvme_allocate_request_null(nvme_cb_fn_t cb_fn,
						void *cb_arg)
{
	struct nvme_request *request;

	request = nvme_allocate_request(cb_fn, cb_arg);
	if (request != NULL) {
		request->type = NVME_REQUEST_NULL;
	}

	return request;
}

static inline void nvme_completion_swapbytes(struct nvme_completion *cpl)
{
#if _BYTE_ORDER != _LITTLE_ENDIAN
	cpl->cdw0 = sys_le32_to_cpu(cpl->cdw0);
	/* omit rsvd1 */
	cpl->sqhd = sys_le16_to_cpu(cpl->sqhd);
	cpl->sqid = sys_le16_to_cpu(cpl->sqid);
	/* omit cid */
	cpl->status = sys_le16_to_cpu(s->status);
#else
	ARG_UNUSED(cpl);
#endif
}

static inline
void nvme_completion_poll(struct nvme_completion_poll_status *status)
{
	k_sem_take(&status->sem, K_FOREVER);
}

#define NVME_CPL_STATUS_POLL_INIT(cpl_status)			\
	{							\
		.status = 0,					\
		.sem = Z_SEM_INITIALIZER(cpl_status.sem, 0, 1),	\
	}

static inline
void nvme_cpl_status_poll_init(struct nvme_completion_poll_status *status)
{
	status->status = 0;
	k_sem_init(&status->sem, 0, 1);
}

#define nvme_completion_is_error(cpl)			\
	((NVME_STATUS_GET_SC((cpl)->status) != 0) |	\
	 (NVME_STATUS_GET_SCT((cpl)->status) != 0))

static inline
bool nvme_cpl_status_is_error(struct nvme_completion_poll_status *status)
{
	return ((status->status != 0) ||
		nvme_completion_is_error(&status->cpl));
}

#endif /* ZEPHYR_DRIVERS_DISK_NVME_NVME_COMMAND_H_ */
