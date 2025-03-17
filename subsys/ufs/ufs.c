/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file implements core initialization
 * for Universal Flash Storage Host Controller driver
 *
 * Right now the implementation:
 *  - support UFS Host Controller Initialization
 *  - support Device configuration
 *  - support Command Transfer Requests
 *  - does not support Task Management
 *  - does not support RPMB request
 *  - does not support asynchronous transfer
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/ufshc/ufshc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ufs/ufs.h>
#include <zephyr/ufs/unipro.h>

LOG_MODULE_REGISTER(ufs, CONFIG_UFS_LOG_LEVEL);

/**
 * @brief Fill the UFS Protocol Information Unit (UPIU) header
 *
 * @param ufshc Pointer to the UFS host controller instance
 * @param trans_type Transaction type to be set in the UPIU header
 * @param upiu_dw0 UPIU flags and task tag, combined into a 32-bit word
 * @param query_task_mang_fn Task management function value to be set in the UPIU header
 * @param data_segment_len Length of the data segment, in little-endian format
 */
static void ufshc_fill_upiu_header(struct ufs_host_controller *ufshc,
				   uint8_t trans_type, uint32_t upiu_dw0,
				   uint8_t query_task_mang_fn,
				   uint16_t data_segment_len)
{
	struct ufshc_xfer_req_upiu *ucd_req_ptr = &ufshc->ucdl_base_addr->req_upiu;

	ucd_req_ptr->upiu_header.transaction_type = trans_type;
	ucd_req_ptr->upiu_header.flags = (uint8_t)(upiu_dw0 >> 8U);
	ucd_req_ptr->upiu_header.task_tag = (uint8_t)(upiu_dw0 >> 24U);
	ucd_req_ptr->upiu_header.query_task_mang_fn = query_task_mang_fn;
	ucd_req_ptr->upiu_header.data_segment_len = sys_cpu_to_be16(data_segment_len);
}

/**
 * @brief Fill UTP Transfer request descriptor header
 *
 * @param ufshc Pointer to the UFS host controller instance
 * @param slot Slot index to place the transfer request descriptor
 * @param data_direction Data transfer direction (e.g., read or write)
 * @param resp_upiu_len Length of the response UPIU (Protocol Information Unit)
 * @param prdt_len Length of the PRDT (Physical Region Descriptor Table) entry, or zero if none
 */
static void ufshc_fill_utp_trans_req_desc(struct ufs_host_controller *ufshc,
					  uint32_t slot, uint32_t data_direction,
					  uint32_t resp_upiu_len, uint32_t prdt_len)
{
	uint32_t dw0;
	uint32_t prdt_offset;
	uint32_t resp_upiu_offset;
	uint32_t resp_upiu_info;
	uint32_t prdt_info;
	struct ufshc_xfer_req_desc *utrd_ptr;

	/* Set the control word (dw0) with storage, data direction, and interrupt */
	dw0 = ((uint32_t)UFSHC_CT_UFS_STORAGE_MASK |
	       (uint32_t)data_direction |
	       (uint32_t)UFSHC_INTERRUPT_CMD_MASK);

	/* response UPIU offset and info */
	resp_upiu_offset = (uint32_t)((uintptr_t)&(ufshc->ucdl_base_addr->resp_upiu) -
				      (uintptr_t)(ufshc->ucdl_base_addr));
	resp_upiu_info = (uint32_t)((resp_upiu_offset >> 2U) << 16U) |
			 (uint32_t)((sizeof(ufshc->ucdl_base_addr->resp_upiu.upiu_header) +
			 resp_upiu_len) >> 2U);

	/* calculate PRDT offset and related information */
	if (prdt_len != 0U) {
		prdt_offset = (uint32_t)((uintptr_t)&(ufshc->ucdl_base_addr->prdt) -
					 (uintptr_t)ufshc->ucdl_base_addr);
		prdt_info = (uint32_t)((prdt_offset >> 2U) << 16U) | (uint32_t)prdt_len;
	} else {
		prdt_info = 0U;
	}

	/* fill the transfer request descriptor for the given slot */
	utrd_ptr = ((struct ufshc_xfer_req_desc *)ufshc->utrdl_base_addr) + slot;

	utrd_ptr->dw0_config = dw0;
	utrd_ptr->dw1_dunl = 0U;
	utrd_ptr->dw2_ocs = (uint32_t)OCS_INVALID_COMMAND_STATUS;
	utrd_ptr->dw3_dunu = 0U;
	utrd_ptr->dw4_utp_cmd_desc_base_addr_lo = (uint32_t)((uintptr_t)(ufshc->ucdl_base_addr));
	utrd_ptr->dw5_utp_cmd_desc_base_addr_up = 0U;
	utrd_ptr->dw6_resp_upiu_info = resp_upiu_info;
	utrd_ptr->dw7_prdt_info = prdt_info;
}

/**
 * @brief Find an available slot in the transfer request list (TRL)
 *
 * This function checks the Transfer Request List (TRL) of the UFS host controller to find
 * an available slot.
 *
 * @param ufshc Pointer to the UFS host controller private data structure
 * @param slot Pointer to a variable that will hold the index of the available slot
 */
static void ufshc_find_slot_in_trl(struct ufs_host_controller *ufshc, uint32_t *slot)
{
	uint32_t reg_data;
	uint32_t index;

	reg_data = ufshc_read_reg(ufshc, UFSHC_UTRLDBR);

	for (index = 0; index < ufshc->xfer_req_depth; index++) {
		if ((reg_data & ((uint32_t)1U << index)) == 0U) {
			*slot = index;
			break;
		}
	}
}

/**
 * @brief Fill the UPIU (UFS Protocol Information Unit) for SCSI commands
 *
 * @param ufshc Pointer to the UFS host controller private data structure
 * @param slot Slot index for the UFS transaction
 * @param pccb Pointer to the SCSI command structure (SCSI Control Block)
 */
static void ufshc_fill_scsi_cmd_upiu(struct ufs_host_controller *ufshc,
				     uint32_t slot, struct scsi_cmd *pccb)
{
	uint32_t data_direction;
	uint32_t prdt_entries;
	uint32_t data_cnt;
	uint8_t *localbuf;
	uint32_t index = 0U;
	uint32_t upiu_dw0, tmp_val;
	uint32_t task_tag = slot;
	struct ufshc_xfer_cmd_desc *ucd_ptr = ufshc->ucdl_base_addr;

	(void)memset(ucd_ptr, 0, sizeof(struct ufshc_xfer_cmd_desc));

	/* data direction */
	if (pccb->dma_dir == (uint8_t)PERIPHERAL_TO_MEMORY) {
		data_direction = UFSHC_DD_DEV_TO_MEM_MASK;
		upiu_dw0 = ((uint32_t)UFSHC_UPIU_FLAGS_READ << 8U) |
			    (task_tag << 24U);
	} else if (pccb->dma_dir == (uint8_t)MEMORY_TO_PERIPHERAL) {
		data_direction = UFSHC_DD_MEM_TO_DEV_MASK;
		upiu_dw0 = ((uint32_t)UFSHC_UPIU_FLAGS_WRITE << 8U) |
			    (task_tag << 24U);
	} else {
		data_direction = 0U;
		upiu_dw0 = (slot << 24U);
	}
	ufshc_fill_upiu_header(ufshc, (uint8_t)UFSHC_CMD_UPIU_TRANS_CODE,
			       upiu_dw0, 0U, 0U);
	ucd_ptr->req_upiu.upiu_header.lun = pccb->lun;

	/* fill scsi cmd upiu */
	(void)memcpy(ucd_ptr->req_upiu.cmd_upiu.scsi_cdb, pccb->cmd, pccb->cmdlen);
	ucd_ptr->req_upiu.cmd_upiu.exp_data_xfer_len = sys_cpu_to_be32((uint32_t)pccb->datalen);

	/* fill prdt entries */
	data_cnt = (uint32_t)(pccb->datalen);
	prdt_entries = data_cnt / UFSHC_256KB;

	/* iterate for full 256KB blocks */
	for (index = 0U; index < prdt_entries; index++) {
		if (pccb->pdata != NULL) {
			localbuf = &(pccb->pdata[(index * UFSHC_256KB)]);
		} else {
			localbuf = NULL;
		}
		ucd_ptr->prdt[index].bufaddr_lower = (uint32_t)((uintptr_t)localbuf);
		tmp_val = (uint32_t)((uint64_t)((uintptr_t)localbuf) >> 32U);
		ucd_ptr->prdt[index].bufaddr_upper = tmp_val;
		ucd_ptr->prdt[index].data_bytecount = UFSHC_256KB - 1U;
	}

	/* leftover (less than 256KB) */
	if ((data_cnt % UFSHC_256KB) != 0U) {
		if (pccb->pdata != NULL) {
			localbuf = &(pccb->pdata[(index * UFSHC_256KB)]);
		} else {
			localbuf = NULL;
		}
		ucd_ptr->prdt[index].bufaddr_lower = (uint32_t)((uintptr_t)localbuf);
		tmp_val = (uint32_t)((uint64_t)((uintptr_t)localbuf) >> 32U);
		ucd_ptr->prdt[index].bufaddr_upper = tmp_val;
		ucd_ptr->prdt[index].data_bytecount = (data_cnt % UFSHC_256KB) - 1U;
		prdt_entries = prdt_entries + 1U;
	}

	/* Perform cache flush to ensure correct data transfer */
	(void)sys_cache_data_flush_range((void *)pccb->pdata, data_cnt);
	(void)sys_cache_data_invd_range((void *)pccb->pdata, data_cnt);

	/* Fill the UTP (UFS Transport Protocol) request descriptor */
	ufshc_fill_utp_trans_req_desc(ufshc, slot, data_direction,
				      (uint32_t)(sizeof(ucd_ptr->resp_upiu.resp_upiu)),
				      prdt_entries);
}

/**
 * @brief Initialize the query request transaction fields for UPIU
 *
 * @param ufshc Pointer to the UFS host controller private data structure
 * @param cmd Query opcode (command for the query)
 * @param tsf_dw0 Query parameters packed into a 32-bit value (includes IDN, index, and selector)
 * @param value The query value to be sent
 * @param length The length of the query
 */
static void ufshc_fill_query_req_upiu_tsf(struct ufs_host_controller *ufshc,
					  uint8_t cmd, uint32_t tsf_dw0,
					  uint32_t value, uint16_t length)
{
	struct ufshc_xfer_cmd_desc *ucd_ptr = ufshc->ucdl_base_addr;
	struct ufshc_xfer_req_upiu *req_upiu = &ucd_ptr->req_upiu;

	req_upiu->query_req_upiu.tsf.opcode = cmd;
	req_upiu->query_req_upiu.tsf.desc_id = (uint8_t)(tsf_dw0 >> 8U);
	req_upiu->query_req_upiu.tsf.index = (uint8_t)(tsf_dw0 >> 16U);
	req_upiu->query_req_upiu.tsf.selector = (uint8_t)(tsf_dw0 >> 24U);
	req_upiu->query_req_upiu.tsf.value = sys_cpu_to_be32(value);
	req_upiu->query_req_upiu.tsf.length = sys_cpu_to_be16(length);
}

/**
 * @brief Initialize the query request parameters for UPIU
 *
 * @param ufshc Pointer to the UFS host controller private data structure
 * @param slot The slot index for the UFS command
 * @param opcode Query opcode (command for the query)
 * @param idn Query descriptor ID
 * @param idx Query index
 * @param sel Query selector
 * @param val Query value
 * @param len Query length
 */
static void ufshc_fill_query_upiu(struct ufs_host_controller *ufshc, uint32_t slot,
				  int32_t opcode, uint8_t idn, uint8_t idx,
				  uint8_t sel, uint32_t val, uint16_t len)
{
	uint32_t task_tag = slot;
	uint8_t query_task_mang_fn = 0U;
	uint32_t upiu_dw0;
	uint32_t tsf_dw0 = 0U;
	struct ufshc_xfer_cmd_desc *ucd_ptr = ufshc->ucdl_base_addr;

	/* Determine the query type based on the provided opcode */
	switch (opcode) {
	case (int32_t)UFSHC_QRY_READ_DESC_CMD:
	case (int32_t)UFSHC_QRY_READ_ATTR_CMD:
	case (int32_t)UFSHC_QRY_READ_FLAG_CMD:
		query_task_mang_fn = UFSHC_QRY_READ;
		break;
	case (int32_t)UFSHC_QRY_WRITE_ATTR_CMD:
	case (int32_t)UFSHC_QRY_WRITE_DESC_CMD:
	case (int32_t)UFSHC_QRY_SET_FLAG_CMD:
		query_task_mang_fn = UFSHC_QRY_WRITE;
		break;
	default:
		query_task_mang_fn = 0;
		break;
	}

	/* Construct the UPIU header */
	upiu_dw0 = (task_tag << 24U);
	ufshc_fill_upiu_header(ufshc, (uint8_t)UFSHC_QRY_UPIU_TRANS_CODE,
			       upiu_dw0, query_task_mang_fn, 0U);

	/* Construct the transaction-specific data word (TSF) */
	tsf_dw0 = ((uint32_t)idn << 8U) |
		  ((uint32_t)idx << 16U) |
		  ((uint32_t)sel << 24U);
	ufshc_fill_query_req_upiu_tsf(ufshc, (uint8_t)opcode,
				      tsf_dw0, val, (uint16_t)len);

	/* Initialize the Response field with FAILURE */
	ucd_ptr->resp_upiu.upiu_header.response = 1U;

	/* Initialize the UTP transfer request descriptor */
	ufshc_fill_utp_trans_req_desc(ufshc, slot, 0U,
				      (uint32_t)(sizeof(ucd_ptr->resp_upiu.query_resp_upiu)), 0U);
}

/**
 * @brief Initialize the attribute UPIU request parameters.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param slot Slot number for the UPIU request.
 * @param opcode Opcode specifying the operation to perform (e.g., read, write).
 * @param idn Attribute identifier (IDN) to access.
 * @param index Index field for the attribute request.
 * @param selector Selector field for the attribute request.
 * @param attr_val Attribute value to be used in the request.
 */
static inline void ufshc_fill_attr_upiu(struct ufs_host_controller *ufshc, uint32_t slot,
					int32_t opcode, int32_t idn, uint8_t index,
					uint8_t selector, uint32_t attr_val)
{
	ufshc_fill_query_upiu(ufshc, slot, opcode,
			      (uint8_t)idn, index, selector,
			      attr_val, 0U);
}

/**
 * @brief Initialize the descriptor UPIU request parameters.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param slot Slot number for the UPIU request.
 * @param opcode Opcode specifying the operation to perform (e.g., read, write).
 * @param idn Descriptor identifier (IDN) to access.
 * @param index: Index field for the descriptor request.
 * @param selector Selector field for the descriptor request.
 * @param desc_len Length parameter of the descriptor to be used in the request.
 */
static inline void ufshc_fill_desc_upiu(struct ufs_host_controller *ufshc, uint32_t slot,
					int32_t opcode, int32_t idn, uint8_t index,
					uint8_t selector, int32_t desc_len)
{
	ufshc_fill_query_upiu(ufshc, slot, opcode,
			      (uint8_t)idn, index, selector, 0U, (uint16_t)desc_len);
}

/**
 * @brief Initialize the flag UPIU request parameters.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param slot Slot number for the UPIU request.
 * @param opcode Opcode specifying the flag operation to perform.
 * @param idn Flag identifier (IDN) to access.
 * @param index Index field for the flag request.
 * @param selector Selector field for the flag request.
 */
static inline void ufshc_fill_flag_upiu(struct ufs_host_controller *ufshc, uint32_t slot,
					int32_t opcode, int32_t idn,
					uint8_t index, uint8_t selector)
{
	ufshc_fill_query_upiu(ufshc, slot, opcode,
			      (uint8_t)idn, index, selector, 0U, 0U);
}

/**
 * @brief Fill NOP UPIU (No Operation) request parameters.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param slot Slot number for the UPIU request.
 */
static void ufshc_fill_nop_upiu(struct ufs_host_controller *ufshc, uint32_t slot)
{
	uint32_t upiu_dw0;
	uint32_t task_tag = slot;
	struct ufshc_xfer_cmd_desc *ucd_ptr = ufshc->ucdl_base_addr;

	/* Clear the command descriptor entry to ensure no residual data */
	(void)memset(ufshc->ucdl_base_addr, 0, sizeof(struct ufshc_xfer_cmd_desc));

	/* Construct the UPIU header */
	upiu_dw0 = (task_tag << 24U);
	ufshc_fill_upiu_header(ufshc, (uint8_t)UFSHC_NOP_UPIU_TRANS_CODE, upiu_dw0, 0, 0);

	/* Initialize the Response field with FAILURE */
	ucd_ptr->resp_upiu.upiu_header.response = 1U;

	/* Initialize the UTP transfer request descriptor */
	ufshc_fill_utp_trans_req_desc(ufshc, slot, 0U,
				      (uint32_t)(sizeof(ucd_ptr->resp_upiu.nop_in_upiu)), 0U);
}

/**
 * @brief Send a SCSI and UPIU command to the UFS host controller.
 *
 * This function sends a UPIU (Unified Protocol Information Unit) command
 * to the UFS host controller.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param slot_tag Slot tag for the transfer request.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int32_t ufshc_send_upiu_cmd(struct ufs_host_controller *ufshc, uint32_t slot_tag)
{
	int32_t err = 0;
	uint32_t read_reg;
	uint32_t events;
	k_timeout_t timeout_event;
	struct ufshc_xfer_req_desc *utrd_ptr;
	struct ufshc_xfer_cmd_desc *ucd_ptr = ufshc->ucdl_base_addr;
	struct ufshc_xfer_req_upiu *req_upiu = &ucd_ptr->req_upiu;
	struct ufshc_xfer_resp_upiu *resp_upiu = &ucd_ptr->resp_upiu;
	uint8_t req_trans_type = req_upiu->upiu_header.transaction_type;

	/* Check UTP Transfer Request List Run-Stop Register */
	read_reg = ufshc_read_reg(ufshc, UFSHC_UTRLRSR);
	if ((read_reg & UFSHC_UTRL_RUN) != UFSHC_UTRL_RUN) {
		err = -EIO;
		goto out;
	}

	/* Ensure cache coherence */
	if (ufshc->is_cache_coherent == 0U) {
		/* Flush the request UPIU header */
		(void)sys_cache_data_flush_range((void *)&req_upiu->upiu_header,
						 sizeof(req_upiu->upiu_header));

		/* Depending on the transaction type, flush additional data structures */
		if (req_trans_type == (uint8_t)UFSHC_NOP_UPIU_TRANS_CODE) {
			(void)sys_cache_data_flush_range((void *)&req_upiu->nop_out_upiu,
							 sizeof(req_upiu->nop_out_upiu));
		} else if (req_trans_type == (uint8_t)UFSHC_CMD_UPIU_TRANS_CODE) {
			(void)sys_cache_data_flush_range((void *)&req_upiu->cmd_upiu,
							 sizeof(req_upiu->cmd_upiu));
			(void)sys_cache_data_flush_range((void *)&ucd_ptr->prdt,
							 sizeof(ucd_ptr->prdt));
		} else if (req_trans_type == (uint8_t)UFSHC_QRY_UPIU_TRANS_CODE) {
			(void)sys_cache_data_flush_range((void *)&req_upiu->query_req_upiu,
							 sizeof(req_upiu->query_req_upiu));
		} else {
			LOG_ERR("ufs-send-upiu: invalid upiu request, transaction_type: %d",
				req_trans_type);
			err = -EINVAL;
			goto out;
		}

		/* Invalidate the response UPIU header */
		(void)sys_cache_data_invd_range((void *)&resp_upiu->upiu_header,
						sizeof(resp_upiu->upiu_header));

		/* Invalidate fields depending on the request transaction type */
		if (req_trans_type == (uint8_t)UFSHC_NOP_UPIU_TRANS_CODE) {
			(void)sys_cache_data_invd_range((void *)&resp_upiu->nop_in_upiu,
							sizeof(resp_upiu->nop_in_upiu));
		} else if (req_trans_type == (uint8_t)UFSHC_CMD_UPIU_TRANS_CODE) {
			(void)sys_cache_data_invd_range((void *)&resp_upiu->resp_upiu,
							sizeof(resp_upiu->resp_upiu));
		} else if (req_trans_type == (uint8_t)UFSHC_QRY_UPIU_TRANS_CODE) {
			(void)sys_cache_data_invd_range((void *)&resp_upiu->query_resp_upiu,
							sizeof(resp_upiu->query_resp_upiu));
		} else {
			/* do nothing.. code is not reached */
		}

		/* Invalidate the entire UCD (Command Descriptor) to ensure it is up-to-date */
		(void)sys_cache_data_invd_range((void *)ucd_ptr, sizeof(*(ucd_ptr)));
	}

	/* Clear the outstanding transfer request event */
	(void)k_event_clear(&ufshc->irq_event,
			    (uint32_t)UFS_UPIU_COMPLETION_EVENT);

	/* Mark this slot as having an outstanding transfer request */
	sys_set_bit((mem_addr_t)((uintptr_t)&ufshc->outstanding_xfer_reqs), slot_tag);

	/* Start the transfer */
	ufshc_write_reg(ufshc, UFSHC_UTRLDBR, ((uint32_t)1U << slot_tag));

	/* wait for completion of the transfer */
	timeout_event = K_USEC(UFS_TIMEOUT_US);
	events = k_event_wait(&ufshc->irq_event,
			      (uint32_t)UFS_UPIU_COMPLETION_EVENT,
			      false, timeout_event);
	if ((events & UFS_UPIU_COMPLETION_EVENT) == 0U) {
		LOG_ERR("ufs-send-upiu: cmd request timedout, tag %d", slot_tag);
		err = -ETIMEDOUT;
		goto out;
	}

	/* Get the transfer request descriptor for this slot */
	utrd_ptr = ((struct ufshc_xfer_req_desc *)ufshc->utrdl_base_addr) + slot_tag;

	/* Ensure cache coherence */
	if (ufshc->is_cache_coherent == 0U) {
		(void)sys_cache_data_invd_range((void *)&resp_upiu->upiu_header,
						sizeof(resp_upiu->upiu_header));
		if (req_trans_type == (uint8_t)UFSHC_NOP_UPIU_TRANS_CODE) {
			(void)sys_cache_data_invd_range((void *)&resp_upiu->nop_in_upiu,
							sizeof(resp_upiu->nop_in_upiu));
		} else if (req_trans_type == (uint8_t)UFSHC_CMD_UPIU_TRANS_CODE) {
			(void)sys_cache_data_invd_range((void *)&resp_upiu->resp_upiu,
							sizeof(resp_upiu->resp_upiu));
		} else if (req_trans_type == (uint8_t)UFSHC_QRY_UPIU_TRANS_CODE) {
			(void)sys_cache_data_invd_range((void *)&resp_upiu->query_resp_upiu,
							sizeof(resp_upiu->query_resp_upiu));
		} else {
			/* do nothing.. code is not reached */
		}

		(void)sys_cache_data_invd_range((void *)utrd_ptr, sizeof(*(utrd_ptr)));
	}

	/* Check if the transfer was successful by evaluating the status fields */
	if (utrd_ptr->dw2_ocs != 0U) {
		LOG_ERR("OCS error from controller = %x for tag %d", utrd_ptr->dw2_ocs, slot_tag);
		err = -EIO;
	} else {
		if (resp_upiu->upiu_header.response != 0U) {
			LOG_ERR("ufs-send-upiu: unexpected response: %x",
				resp_upiu->upiu_header.response);
			err = -EIO;
		} else if (resp_upiu->upiu_header.status != 0U) {
			LOG_ERR("ufs-send-upiu: unexpected status:%x",
				resp_upiu->upiu_header.status);
			err = -EIO;
		} else if (resp_upiu->upiu_header.task_tag !=
			   req_upiu->upiu_header.task_tag) {
			err = -EIO;
		} else {
			err = 0; /* No error, transfer successful */
		}
	}

out:
	return err;
}

/**
 * @brief Send a flag query request to the UFS host controller.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param opcode The flag query operation to perform.
 * @param idn The flag identifier (IDN) to access.
 * @param index The flag index to access.
 * @param flag_res Pointer to store the queried flag value (true or false).
 *
 * @return 0 on success, or a non-zero error code on failure.
 */
static int32_t ufshc_query_flag(struct ufs_host_controller *ufshc,
				int32_t opcode, int32_t idn, uint8_t index,
				bool *flag_res)
{
	int32_t err, selector = 0;
	uint32_t tmp_be32_val;
	uint32_t slot = 0U;
	struct ufshc_query_upiu *query_resp_ptr;

	/* Find an available slot in the transfer request list (TRL) */
	ufshc_find_slot_in_trl(ufshc, &slot);

	/* Clear any previous command descriptor to prepare for new transfer. */
	(void)memset(ufshc->ucdl_base_addr, 0, sizeof(struct ufshc_xfer_cmd_desc));

	/* Fill the UPIU with query details */
	ufshc_fill_flag_upiu(ufshc, slot, opcode, idn, index, (uint8_t)selector);

	/* Send the UPIU command and wait for the result */
	err = ufshc_send_upiu_cmd(ufshc, slot);
	if ((err == 0) && (flag_res != NULL)) {
		query_resp_ptr = &ufshc->ucdl_base_addr->resp_upiu.query_resp_upiu;
		tmp_be32_val = sys_cpu_to_be32(query_resp_ptr->tsf.value);
		if ((tmp_be32_val & 0x1U) != 0U) {
			*flag_res = true;
		} else {
			*flag_res = false;
		}
	}

	if (err != 0) {
		LOG_ERR("ufs-query-flag: Sending for idn %d failed, err = %d", idn, err);
	}

	return err;
}

/**
 * @brief Send a descriptor query request to the UFS host controller.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param opcode The descriptor operation code.
 * @param idn The descriptor identifier (IDN) to access.
 * @param index The index field to use in the query.
 * @param selector The selector field to use in the query.
 * @param desc_buf Pointer to the buffer to store the retrieved descriptor data.
 * @param desc_len Length of the descriptor data to be retrieved.
 *
 * @return 0 on success, or a non-zero error code on failure.
 */
static int32_t ufshc_query_descriptor(struct ufs_host_controller *ufshc,
				      int32_t opcode, int32_t idn,
				      uint8_t index, uint8_t selector,
				      uint8_t *desc_buf, int32_t desc_len)
{
	int32_t err;
	uint32_t slot = 0U;
	struct ufshc_xfer_cmd_desc *cmd_desc_ptr = ufshc->ucdl_base_addr;

	/* Find an available slot in the transfer request list (TRL) */
	ufshc_find_slot_in_trl(ufshc, &slot);

	/* Clear any previous command descriptor to prepare for new transfer. */
	(void)memset(ufshc->ucdl_base_addr, 0, sizeof(struct ufshc_xfer_cmd_desc));

	/* Fill the UPIU with query details */
	ufshc_fill_desc_upiu(ufshc, slot, opcode, idn, index, selector, desc_len);

	/* Send the UPIU command and wait for the result */
	err = ufshc_send_upiu_cmd(ufshc, slot);
	if ((err == 0) && (desc_buf != NULL) &&
	    (opcode == (int32_t)UFSHC_QRY_READ_DESC_CMD)) {
		(void)memcpy(desc_buf,
			     cmd_desc_ptr->resp_upiu.query_resp_upiu.data,
			     (size_t)desc_len);
	}

	if (err != 0) {
		LOG_ERR("ufs-query-desc: opcode 0x%02X for idn %d failed, index %d, err = %d",
			opcode, idn, index, err);
	}

	return err;
}

/**
 * @brief Send an attribute query request to the UFS host controller.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param opcode The attribute operation code.
 * @param idn The attribute identifier (IDN) to access.
 * @param index The index field to use in the query.
 * @param selector The selector field to use in the query.
 * @param attr_val Pointer to store the queried attribute value.
 *
 * @return 0 on success, or a non-zero error code on failure.
 */
static int32_t ufshc_query_attr(struct ufs_host_controller *ufshc,
				int32_t opcode, int32_t idn,
				uint8_t index, uint8_t selector,
				uint32_t *attr_val)
{
	int32_t err;
	uint32_t slot = 0U;
	struct ufshc_query_upiu *query_resp_ptr;

	if (attr_val == NULL) {
		return -EINVAL;
	}

	/* Find an available slot in the transfer request list (TRL) */
	ufshc_find_slot_in_trl(ufshc, &slot);

	/* Clear any previous command descriptor to prepare for new transfer. */
	(void)memset(ufshc->ucdl_base_addr, 0, sizeof(struct ufshc_xfer_cmd_desc));

	/* Fill the UPIU with query details */
	ufshc_fill_attr_upiu(ufshc, slot, opcode, idn, index, selector, *attr_val);

	/* Send the UPIU command and wait for the result */
	err = ufshc_send_upiu_cmd(ufshc, slot);
	if ((err == 0) && (opcode == (int32_t)UFSHC_QRY_READ_ATTR_CMD)) {
		query_resp_ptr = &ufshc->ucdl_base_addr->resp_upiu.query_resp_upiu;
		*attr_val = sys_cpu_to_be32(query_resp_ptr->tsf.value);
	}

	if (err != 0) {
		LOG_ERR("ufs-query-attr: opcode 0x%02X for idn %d failed, index %d, err = %d",
		opcode, idn, index, err);
	}

	return err;
}

/**
 * @brief Fill UIC command structure
 *
 * This function fills the provided UIC command structure with the values
 * for the command, MIB attribute, generation selection, attribute set type,
 * and MIB value, based on the function arguments.
 *
 * @param uic_cmd_ptr Pointer to the UIC command structure to be filled
 * @param mib_attr_gen_sel MIB attribute and generation selection value
 *                         (upper 16-bits for attribute, lower 16-bits for selection)
 * @param attr_set_type Attribute set type for the UIC command
 * @param mib_val MIB value for the UIC command
 * @param cmd UIC command value
 */
void ufshc_fill_uic_cmd(struct ufshc_uic_cmd *uic_cmd_ptr,
			uint32_t mib_attr_gen_sel, uint32_t mib_val,
			uint32_t attr_set_type, uint32_t cmd)
{
	uic_cmd_ptr->command = (uint8_t)cmd;
	uic_cmd_ptr->mib_attribute = (uint16_t)(mib_attr_gen_sel >> 16U);
	uic_cmd_ptr->gen_sel_index = (uint16_t)mib_attr_gen_sel;
	uic_cmd_ptr->attr_set_type = (uint8_t)attr_set_type;
	uic_cmd_ptr->mib_value = mib_val;
}

/**
 * @brief Check if the UFS host controller is ready to accept UIC commands
 *
 * This function checks the readiness of the UFS host controller to accept UIC
 * commands by inspecting the UCRDY (UFS Controller Ready) bit in the HCS register.
 *
 * @param ufshc Pointer to the UFS host controller instance
 *
 * @return true if the controller is ready to accept UIC commands, false otherwise.
 */
static bool ufshc_ready_for_uic_cmd(struct ufs_host_controller *ufshc)
{
	if ((ufshc_read_reg(ufshc, UFSHC_HCS) & UFSHC_HCS_UCRDY_MASK) != 0U) {
		return true;
	} else {
		return false;
	}
}

/**
 * @brief Send a UIC command to the UFS host controller
 *
 * This function sends a UIC (UFS Interface command) to the UFS host controller
 * and waits for its completion.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param uic_cmd Pointer to the UIC command structure to be sent.
 *
 * @return 0 on success, negative error code on failure
 */
int32_t ufshc_send_uic_cmd(struct ufs_host_controller *ufshc,
			   struct ufshc_uic_cmd *uic_cmd)
{
	int32_t ret;
	uint32_t arg1;
	uint32_t events;
	k_timeout_t timeout_event;

	/* Check if the UFS Host Controller is ready to accept a UIC command. */
	if (!ufshc_ready_for_uic_cmd(ufshc)) {
		LOG_ERR("Controller is not ready to accept UIC commands");
		ret = -EIO;
		goto out;
	}

	/* Clear any previous UIC command completion event. */
	(void)k_event_clear(&ufshc->irq_event,
			    (uint32_t)UFS_UIC_CMD_COMPLETION_EVENT);

	/* Clear the UIC command completion status bit to prepare for the new command */
	ufshc_write_reg(ufshc, UFSHC_IS, UFSHC_IS_UCCS_MASK);

	/* Prepare the UIC command arguments */
	arg1 = (((uint32_t)uic_cmd->mib_attribute << 16U) |
		 (uint32_t)uic_cmd->gen_sel_index);
	ufshc_write_reg(ufshc, UFSHC_UCMDARG1, arg1);
	ufshc_write_reg(ufshc, UFSHC_UCMDARG2,
			((uint32_t)uic_cmd->attr_set_type << 16U));
	if (uic_cmd->command == UFSHC_DME_SET_OPCODE) {
		ufshc_write_reg(ufshc, UFSHC_UCMDARG3, uic_cmd->mib_value);
	}

	/* Write the UIC command opcode to the command register. */
	ufshc_write_reg(ufshc, UFSHC_UICCMD, uic_cmd->command);

	/* Wait for the UIC command completion event */
	timeout_event = K_USEC(UFS_TIMEOUT_US);
	events = k_event_wait(&ufshc->irq_event,
			      (uint32_t)UFS_UIC_CMD_COMPLETION_EVENT,
			      false, timeout_event);
	if ((events & UFS_UIC_CMD_COMPLETION_EVENT) == 0U) {
		LOG_ERR("uic cmd 0x%x completion timeout", uic_cmd->command);
		ret = -ETIMEDOUT;
		goto out;
	}

	/* Retrieve the result code from the UFS Host Controller status register. */
	uic_cmd->result_code = (uint8_t)(ufshc_read_reg(ufshc, UFSHC_UCMDARG2) &
					 UFSHC_UCMDARG2_RESCODE_MASK);
	if (uic_cmd->result_code != 0U) {
		ret = -EIO;
	} else {
		if (uic_cmd->command == (uint32_t)UFSHC_DME_GET_OPCODE) {
			/* Retrieve the MIB value from the response register */
			uic_cmd->mib_value = ufshc_read_reg(ufshc, UFSHC_UCMDARG3);
		}
		ret = 0;
	}

out:
	return ret;
}

/**
 * @brief Change UFS TX/RX Mphy attributes
 *
 * @param ufshc: Pointer to the UFS host controller instance.
 * @param speed_gear The desired speed gear value for the configuration.
 * @param rx_term_cap The RX termination capability to be set.
 * @param tx_term_cap The TX termination capability to be set.
 *
 * @return 0 on success, negative error code on failure
 */
static int32_t ufshc_configure_TxRxAttributes(struct ufs_host_controller *ufshc,
	uint32_t speed_gear, uint32_t rx_term_cap, uint32_t tx_term_cap)
{
	struct ufshc_uic_cmd uic_cmd = {0};
	int32_t ret;
	uint32_t tx_gear, rx_gear;
	uint32_t tx_lanes, rx_lanes;
	uint32_t power_mode, rate;

	/* Extracting speed gear components */
	tx_gear = (uint8_t)speed_gear;
	rx_gear = (uint8_t)speed_gear;
	power_mode = (uint8_t)(speed_gear >> 8U);
	rate = (uint8_t)(speed_gear >> 16U);

	/* Configure TX Gear */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_TXGEAR << 16U),
			   tx_gear, 0U,
			   UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Configure RX Gear */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_RXGEAR << 16U),
			   rx_gear, 0U,
			   UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Configure TX Termination capability */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_TXTERMINATION << 16U),
			   tx_term_cap, 0U,
			   UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Configure RX Termination capability */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_RXTERMINATION << 16U),
			   rx_term_cap, 0U,
			   UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Check for high-speed configuration (Fast Mode) */
	if (power_mode == UFSHC_TX_RX_FAST) {
		/* Configure High-Speed Series based on rate */
		ufshc_fill_uic_cmd(&uic_cmd,
				   ((uint32_t)PA_HSSERIES << 16U),
				   rate, 0U,
				   UFSHC_DME_SET_OPCODE);
		ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
		if (ret != 0) {
			goto out;
		}

		/* Program initial ADAPT for Gear 4 */
		if (tx_gear == UFSHC_GEAR4) {
			ufshc_fill_uic_cmd(&uic_cmd,
					   ((uint32_t)PA_TXHSADAPTTYPE << 16U),
					   1U, 0U,
					   UFSHC_DME_SET_OPCODE);
			ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
			if (ret != 0) {
				goto out;
			}
		}
	}

	/* Retrieve and configure the active TX data lanes */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_CONNECTEDTXDATALANES << 16U),
			   0U, 0U,
			   UFSHC_DME_GET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}
	tx_lanes = uic_cmd.mib_value;

	/* Retrieve and configure the active RX data lanes */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_CONNECTEDRXDATALANES << 16U),
			   0U, 0U,
			   UFSHC_DME_GET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}
	rx_lanes = uic_cmd.mib_value;

	/* Configure the active TX data lanes */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_ACTIVETXDATALANES << 16U),
			   tx_lanes, 0U,
			   UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Configure the active RX data lanes */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_ACTIVERXDATALANES << 16U),
			   rx_lanes, 0U,
			   UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);
	if (ret != 0) {
		goto out;
	}

	/* Set the power mode */
	ufshc_fill_uic_cmd(&uic_cmd,
			   ((uint32_t)PA_PWRMODE << 16U),
			   power_mode, 0U,
			   UFSHC_DME_SET_OPCODE);
	ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);

out:
	return ret;
}

/**
 * @brief Configure the UFS speed gear setting
 *
 * This function sets the UFS host controller's speed gear by configuring the tx and rx
 * attributes (such as termination capabilities)
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param speed_gear Desired speed gear setting
 *
 * @return 0 on success, negative error code on failure
 */
int32_t ufshc_configure_speedgear(struct ufs_host_controller *ufshc, uint32_t speed_gear)
{
	int32_t ret = 0;
	uint32_t tx_term_cap;
	uint32_t rx_term_cap;
	uint32_t time_out;
	uint32_t event_status;
	uint32_t events;
	k_timeout_t timeout_event;

	/* Validate the requested speed gear */
	if ((speed_gear != UFSHC_PWM_G1) && (speed_gear != UFSHC_PWM_G2) &&
	    (speed_gear != UFSHC_PWM_G3) && (speed_gear != UFSHC_PWM_G4) &&
	    (speed_gear != UFSHC_HS_G1) && (speed_gear != UFSHC_HS_G2) &&
	    (speed_gear != UFSHC_HS_G3) && (speed_gear != UFSHC_HS_G4) &&
	    (speed_gear != UFSHC_HS_G1_B) && (speed_gear != UFSHC_HS_G2_B) &&
	    (speed_gear != UFSHC_HS_G3_B) && (speed_gear != UFSHC_HS_G4_B)) {
		ret = -EINVAL;
		goto out;
	}

	/* Set termination capabilities based on speed gear type */
	if ((speed_gear == UFSHC_PWM_G1) || (speed_gear == UFSHC_PWM_G2) ||
	    (speed_gear == UFSHC_PWM_G3) || (speed_gear == UFSHC_PWM_G4)) {
		/* Pulse Width Modulation (PWM) gears require termination */
		tx_term_cap = 1U;
		rx_term_cap = 1U;
	} else {
		/* High-Speed gears do not require termination */
		tx_term_cap = 0U;
		rx_term_cap = 0U;
	}

	/* Clear previous UIC power event flag */
	(void)k_event_clear(&ufshc->irq_event, (uint32_t)UFS_UIC_PWR_COMPLETION_EVENT);

	/* Configure the tx and rx attributes for the selected speed gear */
	ret = ufshc_configure_TxRxAttributes(ufshc, speed_gear,
					     rx_term_cap, tx_term_cap);
	if (ret != 0) {
		goto out;
	}

	/* Wait for UIC power completion event */
	timeout_event = K_USEC(UFS_TIMEOUT_US);
	events = k_event_wait(&ufshc->irq_event,
			      (uint32_t)UFS_UIC_PWR_COMPLETION_EVENT,
			      false, timeout_event);
	if ((events & UFS_UIC_PWR_COMPLETION_EVENT) == 0U) {
		ret = -ETIMEDOUT;
		goto out;
	}

	/* Check power mode status */
	time_out = UFS_TIMEOUT_US;
	ret = -EIO;
	while (time_out > 0U) {
		event_status = ufshc_read_reg(ufshc, UFSHC_HCS) & UFSHC_HCS_UPMCRS_MASK;
		if (event_status == UFSHC_PWR_MODE_VAL) {
			/* Desired power mode, configuration successful */
			ret = 0;
			break;
		}
		time_out--;
		(void)k_usleep(1);
	}

out:
	return ret;
}

/**
 * @brief Read the capabilities of the UFS host controller.
 *
 * This function reads the controller's capabilities register and extracts the
 * maximum number of transfer request slots supported by the UFS host controller.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 */
static inline void ufshc_host_capabilities(struct ufs_host_controller *ufshc)
{
	uint32_t capabilities = ufshc_read_reg(ufshc, UFSHC_HOST_CTRL_CAP);

	ufshc->xfer_req_depth = (capabilities & (uint32_t)UFSHC_TRANSFER_REQ_SLOT_MASK) + 1U;
}

/**
 * @brief Allocate memory for UFS host controller data structures.
 *
 * This function allocates the required DMA memory for two key data structures:
 * 1. The Command Descriptor array (aligned to 128-byte boundaries).
 * 2. The UTP Transfer Request Descriptor List (UTRDL) array (aligned to 1k-byte
 *    boundaries).
 *
 * If memory has already been allocated for these structures, the function
 * avoids redundant allocations.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 *
 * @return 0 on success, or a negative error code (e.g., -ENOMEM) on failure
 */
static int32_t ufshc_host_memory_alloc(struct ufs_host_controller *ufshc)
{
	size_t utrd_size, ucd_size;

	/* Allocate 1 memory to 1 Command Descriptor
	 * Should be aligned to 128 boundary.
	 */
	ucd_size = (sizeof(struct ufshc_xfer_cmd_desc));
	if (ufshc->ucdl_base_addr == NULL) {
		/* don't allocate memory if already allocated - earlier */
		ufshc->ucdl_base_addr = k_aligned_alloc(128, ucd_size);
		if (ufshc->ucdl_base_addr == NULL) {
			LOG_ERR("Command descriptor memory allocation failed");
			return -ENOMEM;
		}
	}
	(void)memset(ufshc->ucdl_base_addr, 0x0, ucd_size);

	/* Allocate memory to Transfer Request Descriptors based on xfer req depth
	 * Should be aligned to 1k boundary.
	 */
	utrd_size = (sizeof(struct ufshc_xfer_req_desc)) * (ufshc->xfer_req_depth);
	if (ufshc->utrdl_base_addr == NULL) {
		/* don't allocate memory if already allocated - earlier */
		ufshc->utrdl_base_addr = k_aligned_alloc(1024, utrd_size);
		if (ufshc->utrdl_base_addr == NULL) {
			LOG_ERR("Transfer Descriptor memory allocation failed");
			return -ENOMEM;
		}
	}
	(void)memset(ufshc->utrdl_base_addr, 0x0, utrd_size);

	return 0;
}

/**
 * @brief Handle the completion of SCSI and query commands.
 *
 * This function processes the completion of SCSI and query commands initiated
 * by the UFS host controller. It checks the doorbell register for completed
 * requests. Posts an interrupt event indicating the completion of UFS UPIU operations.
 *
 * @param ufshc The UFS host controller instance.
 */
static void ufshc_transfer_req_compl(struct ufs_host_controller *ufshc)
{
	uint32_t tr_doorbell;
	uint32_t completed_reqs;
	uint32_t task_tag;

	/* Read the transfer doorbell register to get completed requests status */
	tr_doorbell = ufshc_read_reg(ufshc, UFSHC_UTRLDBR);

	/* Identify which transfer requests have been completed */
	completed_reqs = (~tr_doorbell) & (ufshc->outstanding_xfer_reqs);

	/* Clear the completed requests from the outstanding transfer requests */
	ufshc->outstanding_xfer_reqs &= ~completed_reqs;

	if (completed_reqs != 0U) {
		for (task_tag = 0;
		     task_tag < (uint32_t)(ufshc->xfer_req_depth);
		     task_tag++) {
			(void)sys_test_and_clear_bit((mem_addr_t)((uintptr_t)&completed_reqs),
						     task_tag);
		}

		/* Post an event to indicate that a UPIU completion has occurred */
		(void)k_event_post(&ufshc->irq_event,
				   (uint32_t)UFS_UPIU_COMPLETION_EVENT);
	}
}

/**
 * @brief Handle completion of UIC command interrupts.
 *
 * This function processes the completion of UIC commands based on the provided
 * interrupt status. It handles the UIC command completion and power
 * status change events.
 *
 * @param ufshc The UFS host controller instance.
 * @param intr_status The interrupt status generated by the UFS host controller.
 */
static void ufshc_uic_cmd_compl(struct ufs_host_controller *ufshc, uint32_t intr_status)
{
	/* Check if the UIC command status interrupt is set */
	if ((intr_status & UFSHC_IS_UCCS_MASK) != 0U) {
		/* Post an event indicating UIC command completion */
		(void)k_event_post(&ufshc->irq_event,
				   (uint32_t)UFS_UIC_CMD_COMPLETION_EVENT);
	}

	/* Check if the power status interrupt is set */
	if ((intr_status & UFSHC_IS_PWR_STS_MASK) != 0U) {
		/* Post an event indicating power status change */
		(void)k_event_post(&ufshc->irq_event,
				   (uint32_t)UFS_UIC_PWR_COMPLETION_EVENT);
	}
}

/**
 * @brief Main interrupt service routine for UFS host controller.
 *
 * This function serves as the main interrupt service routine (ISR) for the UFS host
 * controller. UIC and UPIU-related interrupts are handled separately.
 *
 * @param: Pointer to the UFS host controller instance.
 */
static void ufshc_main_isr(const void *param)
{
	uint32_t intr_status, enabled_intr_status = 0;
	struct ufs_host_controller *ufshc = (struct ufs_host_controller *)param;

	/* Read the interrupt status register to get the current interrupt */
	intr_status = ufshc_read_reg(ufshc, UFSHC_IS);

	/* Read the enabled interrupts register and mask it with the current */
	enabled_intr_status = intr_status & ufshc_read_reg(ufshc, UFSHC_IE);

	/* Clear the interrupt status register to acknowledge the interrupts */
	ufshc_write_reg(ufshc, UFSHC_IS, intr_status);

	/* Handle UIC interrupt */
	if ((enabled_intr_status & UFSHCD_UIC_MASK) != 0U) {
		ufshc_uic_cmd_compl(ufshc, enabled_intr_status);
	}

	/* Handle UPIU interrupt */
	if ((enabled_intr_status & UFSHC_IS_UTRCS_MASK) != 0U) {
		ufshc_transfer_req_compl(ufshc);
	}
}

/**
 * @brief Enable Host Controller (HCE)
 *
 * This function enables the Host Controller Enable (HCE) bit,
 * and waits for it to be acknowledged by the hardware.
 *
 * @param ufshc Pointer to the ufs-host-controller instance.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int32_t ufshc_set_hce(struct ufs_host_controller *ufshc)
{
	int32_t ret  = 0;
	uint32_t time_out;
	uint32_t read_reg;

	/* Write to the register to enable HCE (Host Controller Enable) */
	ufshc_write_reg(ufshc, UFSHC_HCE, UFSHC_HCE_MASK);

	/* Wait for the HCE bit to be set */
	time_out = UFS_TIMEOUT_US;
	do {
		read_reg = ufshc_read_reg(ufshc, UFSHC_HCE);
		if ((read_reg & UFSHC_HCE_MASK) == UFSHC_HCE_MASK) {
			/* Success: HCE bit is set */
			break;
		}
		time_out = time_out - 1U;
		(void)k_usleep(1);
	} while (time_out != 0U);

	if (time_out == 0U) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

/**
 * @brief Enable specific interrupts in the UFS host controller
 *
 * @param ufshc Pointer to the ufs-host-controller instance.
 * @param intrs Interrupt bits to be enabled.
 */
static void ufshc_enable_intr(struct ufs_host_controller *ufshc, uint32_t intrs)
{
	uint32_t set = ufshc_read_reg(ufshc, UFSHC_IE);

	set |= intrs;
	ufshc_write_reg(ufshc, UFSHC_IE, set);
}

/**
 * @brief Enable the UFS host controller
 *
 * This function performs the full sequence to enable the UFS host controller:
 * 1. Enables the Host Controller Enable (HCE) bit.
 * 2. Waits for the card connection status to indicate that the UFS device is ready.
 * 3. Enables UIC-related interrupts.
 *
 * @param ufshc Pointer to the ufs-host-controller instance.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int32_t ufshc_host_enable(struct ufs_host_controller *ufshc)
{
	int32_t ret  = 0;
	uint32_t time_out;
	uint32_t read_reg;

	/* start controller initialization sequence by enabling the HCE */
	ret = ufshc_set_hce(ufshc);
	if (ret != 0) {
		goto out;
	}

	/* Check for Card Connection Status */
	time_out = UFS_TIMEOUT_US;
	do {
		read_reg = ufshc_read_reg(ufshc, UFSHC_HCS);
		if ((read_reg & UFSHC_HCS_CCS_MASK) == 0U) {
			/* Success: card is properly connected */
			break;
		}
		time_out = time_out - 1U;
		(void)k_usleep(1);
	} while (time_out != 0U);

	if (time_out == 0U) {
		ret = -ETIMEDOUT;
	}

	/* Enable UIC related interrupts after successful initialization */
	ufshc_enable_intr(ufshc, UFSHCD_UIC_MASK);

out:
	return ret;
}

/**
 * @brief Notify Unipro to perform link startup
 *
 * This function issues the DME_LINKSTARTUP command to the Unipro layer,
 * which triggers the initialization of the Unipro link startup procedure.
 * Once the Unipro link is successfully initialized, the device connected to
 * the host controller will be detected.
 *
 * This function attempts to issue the DME_LINKSTARTUP command, and retries
 * if necessary. If the link startup process times out, it returns a timeout error.
 *
 * @param ufshc ufs-host-controller instance
 *
 * @return 0 on success, non-zero value on failure.
 */
static int32_t ufshc_dme_link_startup(struct ufs_host_controller *ufshc)
{
	struct ufshc_uic_cmd uic_cmd = {0};
	int32_t ret;
	uint32_t time_out;
	uint32_t time_out_ulss;
	uint32_t read_reg;

	/* send DME_LINKSTARTUP command */
	ufshc_fill_uic_cmd(&uic_cmd, 0U, 0U, 0U, UFSHC_DME_LINKSTARTUP_OPCODE);

	time_out = 100U; /* 100 micro seconds */
	do {
		time_out = time_out - 1U;
		(void)k_usleep(1);
		ret = ufshc_send_uic_cmd(ufshc, &uic_cmd);

		/* Clear UIC error which trigger during LINKSTARTUP command */
		ufshc_write_reg(ufshc, UFSHC_IS, UFSHC_IS_UE_MASK);
		if (ret == 0) {
			/* Success: LINKSTARTUP command was sent successfully */
			break;
		}

		/* If the command failed, check if the ULSS (UFS Link Startup) bit is set */
		time_out_ulss = UFS_TIMEOUT_US;
		do {
			read_reg = ufshc_read_reg(ufshc, UFSHC_IS);
			if ((read_reg & UFSHC_IS_ULSS_MASK) == UFSHC_IS_ULSS_MASK) {
				/* Success: ULSS bit set, send link startup again */
				break;
			}
			time_out_ulss = time_out_ulss - 1U;
			(void)k_usleep(1);
		} while (time_out_ulss != 0U);

		if (time_out_ulss == 0U) {
			/* Timeout occurred while waiting for ULSS bit to be set */
			ret = -ETIMEDOUT;
			break;
		}

		/* prepare for next iteration */
		ufshc_write_reg(ufshc, UFSHC_IS, UFSHC_IS_ULSS_MASK);
	} while (time_out != 0U);

	if (ret != 0) {
		LOG_ERR("dme-link-startup: error code %d", ret);
	}

	if (time_out == 0U) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

/**
 * @brief Check if any device is connected to the host controller
 *
 * This function checks if a device is connected to the host controller by
 * inspecting the device presence (DP) flag in the UFS Host Controller status register.
 * It will time out if no device is detected within the specified timeout period.
 *
 * @param ufshc ufs-host-controller instance
 *
 * @return true if a device is present, false if no device is detected or after a timeout.
 */
static inline bool ufshc_is_device_present(struct ufs_host_controller *ufshc)
{
	uint32_t time_out;
	uint32_t read_reg;
	bool dp_flag = false;

	time_out = UFS_TIMEOUT_US; /* One Second */
	do {
		read_reg = ufshc_read_reg(ufshc, UFSHC_HCS);
		if ((read_reg & UFSHC_HCS_DP_MASK) != 0U) {
			dp_flag = true;
			break;
		}
		time_out = time_out - 1U;
		(void)k_usleep(1);
	} while (time_out != 0U);

	return dp_flag;
}

/**
 * @brief Initialize the Unipro link startup procedure
 *
 * This function performs the link startup procedure for the UFS host controller.
 * It involves the following steps:
 * - Perform PHY initialization.
 * - Issue the DME_LINKSTARTUP command to Unipro.
 * - Verify if the device is present.
 * - Retry the link startup in case of failure.
 *
 * The function retries the link startup procedure if the device is not detected
 * or if an error occurs. The maximum number of retries is controlled by the
 * UFSHC_DME_LINKSTARTUP_RETRIES constant.
 *
 * @param ufshc ufs-host-controller instance
 *
 * @return 0 on success, non-zero value on failure.
 */
static int32_t ufshc_link_startup(struct ufs_host_controller *ufshc)
{
	int32_t ret;
	int32_t retries = UFSHC_DME_LINKSTARTUP_RETRIES;

	/* perform phy initialization */
	ret = ufshc_variant_phy_initialization(ufshc->dev);
	if (ret != 0) {
		LOG_ERR("Phy setup failed (%d)", ret);
		goto out;
	}

	do {
		/* send DME link start up */
		ret = ufshc_dme_link_startup(ufshc);

		/* check if device is detected by inter-connect layer */
		if (ret == 0) {
			if (ufshc_is_device_present(ufshc) == false) {
				LOG_ERR("ufs-link-startup: Device is not present");
				ret = -ENXIO;
				goto out;
			}
		} else {
			/*
			 * DME link lost indication is only received when link is up,
			 * but we can't be sure if the link is up until link startup
			 * succeeds. So reset the local Uni-Pro and try again.
			 */
			if (ufshc_host_enable(ufshc) != 0) {
				goto out;
			}
		}

		retries--;
	} while ((ret != 0) && (retries != 0));

	if (ret != 0) {
		goto out;
	}

	/* Include any host controller configuration via UIC commands */
	ret = ufshc_variant_link_startup_notify(ufshc->dev, (uint8_t)POST_CHANGE);
	if (ret != 0) {
		goto out;
	}

out:
	return ret;
}

/**
 * @brief Check the status of UCRDY, UTRLRDY bits.
 *
 * This function checks if the UCRDY (UFS Ready), UTRLRDY (UFS Transport Layer Ready)
 * bits are set in the provided host controller status register.
 *
 * @param reg Register value of host controller status (HCS)
 *
 * @return 0 if the status bits are set correctly (ready) else a positive value (1).
 */
static inline int32_t ufshc_get_lists_status(uint32_t reg)
{
	if ((reg & UFSHCD_STATUS_READY) == UFSHCD_STATUS_READY) {
		return 0;
	} else {
		return 1;
	}
}

/**
 * @brief Enable the run-stop register to allow host controller operation
 *
 * This function enables the run-stop register (UTRLRSR) by writing a value that
 * allows the host controller to process UFS requests.
 *
 * @param ufshc Pointer to the UFS host controller instance
 */
static void ufshc_enable_run_stop_reg(struct ufs_host_controller *ufshc)
{
	ufshc_write_reg(ufshc, UFSHC_UTRLRSR, UFSHC_UTRL_RUN);
}

/**
 * @brief Initialize and make UFS controller operational
 *
 * This function brings the UFS host controller into an operational state by performing
 * the following actions:
 * 1. Enabling necessary interrupts.
 * 2. Programming the UTRL base address for the transport layer.
 * 3. Configuring the run-stop register to allow operations.
 *
 * @param ufshc Pointer to the UFS host controller instance
 *
 * @return 0 on success; a negative error code on failure.
 */
static int32_t ufshc_make_host_operational(struct ufs_host_controller *ufshc)
{
	int32_t err = 0;
	uint32_t reg;

	/* Enable required interrupts */
	ufshc_enable_intr(ufshc, UFSHCD_ENABLE_INTRS);

	/* Configure UTRL base address registers */
	ufshc_write_reg(ufshc, UFSHC_UTRLBA,
			(uint32_t)((uintptr_t)ufshc->utrdl_base_addr));
	ufshc_write_reg(ufshc, UFSHC_UTRLBAU,
			(uint32_t)(((uint64_t)((uintptr_t)ufshc->utrdl_base_addr)) >> 32U));

	/*
	 * UCRDY and UTRLRDY bits must be 1
	 */
	reg = ufshc_read_reg(ufshc, UFSHC_HCS);
	if ((ufshc_get_lists_status(reg)) == 0) {
		ufshc_enable_run_stop_reg(ufshc);
	} else {
		LOG_ERR("Host controller not ready to process requests");
		err = -EIO;
		goto out;
	}

out:
	return err;
}

/**
 * @brief Initializes the UFS Host Controller.
 *
 * This function performs the necessary steps to initialize the UFS host controller.
 *
 * @param ufshc Pointer to the UFS Host Controller instance.
 *
 * @return 0 on success, negative value on failure.
 */
static int32_t ufshc_host_initialize(struct ufs_host_controller *ufshc)
{
	int32_t err;

	/* Read capabilities registers */
	ufshc_host_capabilities(ufshc);

	/* Allocate memory for host memory space */
	err = ufshc_host_memory_alloc(ufshc);
	if (err != 0) {
		LOG_ERR("Memory allocation failed");
		goto out;
	}

	/*
	 * In order to avoid any spurious interrupt immediately after
	 * registering UFS controller interrupt handler, clear any pending UFS
	 * interrupt status and disable all the UFS interrupts.
	 */
	ufshc_write_reg(ufshc, UFSHC_IS, ufshc_read_reg(ufshc, UFSHC_IS));
	ufshc_write_reg(ufshc, UFSHC_IE, 0);

	/*
	 * Make sure that UFS interrupts are disabled and any pending interrupt
	 * status is cleared before registering UFS interrupt handler.
	 */
	if ((uint32_t)irq_connect_dynamic(ufshc->irq, 0, ufshc_main_isr,
					  (const void *)ufshc, 0) != (ufshc->irq)) {
		LOG_ERR("request irq failed");
		err = -ENOTSUP;
		goto out;
	} else {
		irq_enable(ufshc->irq);
	}

	/* Set HCE, card connection status, enable uic interrupts */
	err = ufshc_host_enable(ufshc);
	if (err != 0) {
		LOG_ERR("Host controller enable failed");
		goto out;
	}

	/* initialize unipro-phy, link start up */
	err = ufshc_link_startup(ufshc);
	if (err != 0) {
		goto out;
	}

	/* make host operational - enable transfer request */
	err = ufshc_make_host_operational(ufshc);
	if (err != 0) {
		goto out;
	}

out:
	return err;
}

/**
 * @brief Initialize UFS device and check transport layer readiness
 *
 * This function sends a NOP OUT UPIU to the UFS device and waits for the corresponding
 * NOP IN response. It checks if the UTP (Universal Transport Protocol) layer is
 * ready after initialization by polling the device's fDeviceInit flag.
 *
 * @param ufshc Pointer to the UFS host controller instance
 *
 * @return 0 on success, negative value on failure.
 */
static int32_t ufshc_card_initialize(struct ufs_host_controller *ufshc)
{
	int32_t err;
	bool flag_res = true;
	uint32_t timeout, slot = 0U;

	/* Send NOP OUT UPIU */
	/* find slot for transfer request */
	ufshc_find_slot_in_trl(ufshc, &slot);
	ufshc_fill_nop_upiu(ufshc, slot);
	err = ufshc_send_upiu_cmd(ufshc, slot);
	if (err != 0) {
		LOG_ERR("ufs-card-init: NOP OUT failed %d", err);
		goto out;
	}

	/* Write fdeviceinit flag */
	err = ufshc_query_flag(ufshc, (int32_t)UFSHC_QRY_SET_FLAG_CMD,
				(int32_t)UFSHC_FDEVINIT_FLAG_IDN, 0, NULL);
	if (err != 0) {
		LOG_ERR("ufs-card-init: setting fDeviceInit flag failed with error %d", err);
		goto out;
	}

	/* Poll fDeviceInit flag to be cleared */
	timeout = UFS_TIMEOUT_US;	/* One Second */
	do {
		err = ufshc_query_flag(ufshc,
				       (int32_t)UFSHC_QRY_READ_FLAG_CMD,
				       (int32_t)UFSHC_FDEVINIT_FLAG_IDN, 0,
				       &flag_res);
		if (flag_res == false) {
			break;
		}
		timeout = timeout - 1U;
		(void)k_usleep(1);
	} while (timeout != 0U);

	if ((err == 0) && (flag_res != false)) {
		LOG_ERR("ufs-card-init: fDeviceInit was not cleared by the device");
		err = -EBUSY;
	}

out:
	return err;
}

/**
 * @brief Reads UFS geometry descriptor
 *
 * This function reads the UFS Geometry descriptor from the device to determine
 * the maximum number of logical units (LUNs) supported by the UFS device.
 *
 * @param ufshc Pointer to the UFS host controller instance
 *
 * @return 0 on success, a negative error code on failure.
 */
static int32_t ufshc_read_device_geo_desc(struct ufs_host_controller *ufshc)
{
	int32_t err;
	uint8_t desc_buf[UFSHC_QRY_DESC_MAX_SIZE] = {0};
	struct ufs_dev_info *dev_info = &ufshc->dev_info;

	err = ufshc_query_descriptor(ufshc, (int32_t)UFSHC_QRY_READ_DESC_CMD,
				     (int32_t)UFSHC_GEOMETRY_DESC_IDN,
				     0, 0, desc_buf,
				     (int32_t)UFSHC_QRY_DESC_MAX_SIZE);
	if (err != 0) {
		LOG_ERR("ufs-read-desc: Failed reading Geometry Desc. err = %d", err);
		goto out;
	}

	if (desc_buf[UFSHC_GEO_DESC_PARAM_MAX_NUM_LUN] == 1U) {
		dev_info->max_lu_supported = 32U;
	} else {
		dev_info->max_lu_supported = 8U;
	}

out:
	return err;
}

/**
 * @brief Initializes UFS device information by reading descriptors
 *
 * This function reads the UFS Geometry descriptor and updates the device's
 * information structure to initialize the UFS device parameters, such as
 * the maximum number of supported LUNs.
 *
 * @param ufshc Pointer to the UFS host controller instance
 *
 * @return 0 on success, a negative error code on failure.
 */
static int32_t ufshc_read_device_info(struct ufs_host_controller *ufshc)
{
	int32_t ret;

	/* Init UFS geometry descriptor related parameters */
	ret = ufshc_read_device_geo_desc(ufshc);
	if (ret != 0) {
		goto out;
	}

out:
	return ret;
}

/**
 * @brief Retrieves information about UFS logical units (LUNs)
 *
 * This function queries the UFS Unit descriptor for each supported LUN to gather
 * detailed information such as LUN ID, block count, block size, and whether the LUN
 * is enabled.
 *
 * @param ufshc Pointer to the UFS host controller instance
 *
 * @return 0 on success, a negative error code on failure.
 */
static int32_t ufshc_get_lun_info(struct ufs_host_controller *ufshc)
{
	int32_t ret = -1;
	uint8_t desc_buf[UFSHC_QRY_DESC_MAX_SIZE] = {0};
	uint8_t Index, lun_id;
	uint8_t lun_enable;
	uint32_t TwoPwrblksize;
	uint64_t tmpval;

	struct ufs_dev_info *dev_info = &ufshc->dev_info;

	for (Index = 0U; Index < (dev_info->max_lu_supported); Index++) {

		/* Read the Unit Descriptor for Lun Index */
		ret = ufshc_query_descriptor(ufshc,
					     (int32_t)UFSHC_QRY_READ_DESC_CMD,
					     (int32_t)UFSHC_UNIT_DESC_IDN,
					     Index, 0, desc_buf,
					     (int32_t)UFSHC_QRY_DESC_MAX_SIZE);
		if (ret != 0) {
			goto out;
		}

		/* Check for LU Enable */
		lun_enable = (desc_buf[UFSHC_UD_PARAM_LU_ENABLE]);

		/* if found enabled -- then compute other fields */
		if (lun_enable == 1U) {
			dev_info->lun[Index].lun_enabled = true;

			lun_id = (desc_buf[UFSHC_UD_PARAM_UNIT_INDEX]);
			dev_info->lun[lun_id].lun_id = lun_id;

			/* compute 2^blocksize */
			TwoPwrblksize = (desc_buf[UFSHC_UD_PARAM_LOGICAL_BLKSZ]);
			tmpval = 1;
			for (uint32_t Idx = 0; Idx < TwoPwrblksize; Idx++) {
				tmpval *= 2U;
			}
			dev_info->lun[lun_id].block_size = (uint32_t)tmpval;

			/* get block count */
			tmpval = sys_get_be64(&desc_buf[UFSHC_UD_PARAM_LOGICAL_BLKCNT]);
			dev_info->lun[lun_id].block_count = tmpval;
		} else {
			dev_info->lun[Index].lun_enabled = false;
		}
	}

out:
	return ret;
}

/**
 * @brief Main entry point for executing SCSI requests.
 *
 * This function is called by the SCSI midlayer to handle SCSI requests.
 * It finds a free slot for the transfer request, prepares the command,
 * and sends it to the UFS Host Controller.
 *
 * @param sdev Pointer to the SCSI device.
 * @param cmd Pointer to the SCSI command structure.
 *
 * @return 0 on success, a negative error code on failure.
 */
static int32_t ufs_scsi_exec(struct scsi_device *sdev, struct scsi_cmd *pccb)
{
	int32_t err;
	uint32_t slot = 0U;
	struct scsi_host_info *shost = sdev->host;
	struct device *ufs_dev = shost->parent;
	struct ufs_host_controller *ufshc = ufs_dev->data;

	/* find slot for transfer request */
	ufshc_find_slot_in_trl(ufshc, &slot);
	ufshc_fill_scsi_cmd_upiu(ufshc, slot, pccb);
	err = ufshc_send_upiu_cmd(ufshc, slot);

	if (err != 0) {
		err = -EINVAL;
	}

	return err;
}

static const struct scsi_ops ufs_ops = {
	.exec		= ufs_scsi_exec,
};

/**
 * @brief Allocates a UFS SCSI Host Controller instance.
 *
 * This function allocates a new SCSI host instance for the given UFS
 * Host Controller if not already allocated. It initializes the host
 * structure and associates it with the UFS controller.
 *
 * @param ufshc Pointer to the UFS Host Controller structure.
 *
 * @return 0 on success, -ENOMEM if memory allocation fails.
 */
static int32_t ufshc_alloc_scsi_host(struct ufs_host_controller *ufshc)
{
	struct scsi_host_info *host;
	int32_t err = 0;

	if (ufshc->host == NULL) {
		/* allocate if not done earlier */
		host = scsi_host_alloc(&ufs_ops);
		if (host == NULL) {
			LOG_ERR("scsi-add-host: failed");
			err = -ENOMEM;
			goto out;
		}
		ufshc->host = host;
		host->parent = ufshc->dev;
		host->hostdata = (void *)ufshc;
	}

out:
	return err;
}

/**
 * @brief Probes and adds UFS logical units (LUs).
 *
 * This function probes the UFS device for supported logical units
 * and adds them to the SCSI subsystem if they are enabled.
 *
 * @param ufshc Pointer to the UFS Host Controller structure.
 *
 * @return 0.
 */
static int32_t ufshc_add_lus(struct ufs_host_controller *ufshc)
{
	struct ufs_dev_info *dev_info = &ufshc->dev_info;

	for (uint32_t i = 0; i < dev_info->max_lu_supported; i++) {
		if (dev_info->lun[i].lun_enabled == true) {
			(void)scsi_add_lun_host(ufshc->host, &dev_info->lun[i]);
		}
	}

	return 0;
}

/**
 * @brief Adds a SCSI link for the UFS controller.
 *
 * This function binds the UFS device to the SCSI subsystem. It allocates
 * the SCSI host instance and probes for logical units to be added
 * to the host.
 *
 * @param ufs_dev Pointer to the UFS device structure.
 *
 * @return 0 on success, a negative error code on failure.
 */
static int32_t ufs_scsi_bind(struct device *ufs_dev)
{
	int32_t err;
	struct ufs_host_controller *ufshc = ufs_dev->data;

	err = ufshc_alloc_scsi_host(ufshc);
	if (err != 0) {
		goto out;
	}
	(void)ufshc_add_lus(ufshc);

out:
	return err;
}

/**
 * @brief Initializes the UFS host controller and its associated resources
 *
 * This function performs the initialization of the UFS host controller
 * - Initializing the host and device
 * - Reading device-specific information and parameters
 * - Retrieving LUN (Logical Unit Number) information
 * - Binding the SCSI layer to the UFS device
 *
 * @param ufshc Pointer to the UFS host controller instance
 *
 * @return 0 on success, negative error code on failure.
 */
static int32_t ufshc_init(struct ufs_host_controller *ufshc)
{
	int32_t err;

	err = k_mutex_lock(&ufshc->ufs_lock, K_FOREVER);
	if (err != 0) {
		return err;
	}

	/* host initialization */
	err = ufshc_host_initialize(ufshc);
	if (err != 0) {
		goto out;
	}

	/* Check Device Initialization */
	err = ufshc_card_initialize(ufshc);
	if (err != 0) {
		goto out;
	}

	/* device param - UFS descriptors */
	err = ufshc_read_device_info(ufshc);
	if (err != 0) {
		goto out;
	}

	/* read LUN info */
	err = ufshc_get_lun_info(ufshc);
	if (err != 0) {
		LOG_ERR("Read LUN info failed");
		goto out;
	}

	/* register scsi_exec */
	err = ufs_scsi_bind(ufshc->dev);

	/* UFS initialization succeeded */
	if (err == 0) {
		ufshc->is_initialized = true;
	}

out:
	/* Unlock card mutex */
	(void)k_mutex_unlock(&ufshc->ufs_lock);

	return err;
}

/**
 * @brief Initialize the UFS host controller
 *
 * This function is responsible for initializing the UFS driver by linking the UFS
 * device structure to its corresponding host controller instance and performing
 * the necessary initialization steps.
 *
 * @param ufshc_dev Pointer to the device structure for the UFS host controller
 * @param ufshc Pointer to the UFS host controller structure that will be initialized
 *
 * @return 0 on success, negative error code on failure.
 */
int32_t ufs_init(const struct device *ufshc_dev, struct ufs_host_controller **ufshc)
{
	int32_t err;

	/* Validate input parameters */
	if ((ufshc_dev == NULL) || (ufshc == NULL)) {
		return -ENODEV;
	}

	/* Link the UFS device to the host controller structure */
	*ufshc = (struct ufs_host_controller *)ufshc_dev->data;

	/* Initialize the UFS host controller */
	err = ufshc_init(*ufshc);
	if (err != 0) {
		LOG_ERR("Initialization failed with error %d", err);
	}

	return err;
}

/**
 * @brief Send raw UPIU commands to the UFS host controller
 *
 * This function executes raw UPIU commands such as NOP, Query, and SCSI commands
 * by using the provided `msgcode`. The caller must fill the UPIU request content
 * correctly, as no further validation is performed. The function copies the response
 * to the `rsp` parameter if it's not NULL.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param msgcode  Message code, one of UPIU Transaction Codes Initiator to Target
 * @param req Pointer to the request data (raw UPIU command)
 * @param rsp Pointer to the response data (raw UPIU reply)
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_exec_raw_upiu_cmd(struct ufs_host_controller *ufshc,
				uint32_t msgcode, void *req, void *rsp)
{
	int32_t ret = 0;
	struct ufshc_xfer_req_upiu *req_upiu;
	struct ufshc_xfer_resp_upiu *rsp_upiu;
	struct ufshc_xfer_cmd_desc *ucd_ptr = ufshc->ucdl_base_addr;
	struct scsi_cmd *pccb;
	uint32_t slot = 0U;

	/* Find an available slot for the transfer request */
	ufshc_find_slot_in_trl(ufshc, &slot);

	switch (msgcode) {
	case UFSHC_NOP_UPIU_TRANS_CODE: /* NOP (No Operation) UPIU */
		rsp_upiu = (struct ufshc_xfer_resp_upiu *)rsp;
		/* Fill NOP UPIU fields */
		ufshc_fill_nop_upiu(ufshc, slot);
		/* Send the NOP UPIU command */
		ret = ufshc_send_upiu_cmd(ufshc, slot);
		if (rsp != NULL) {
			/* copy the response UPIU */
			(void)memcpy(rsp_upiu, &ucd_ptr->resp_upiu, sizeof(*rsp_upiu));
		}
		break;
	case UFSHC_QRY_UPIU_TRANS_CODE: /* Query UPIU */
		req_upiu = (struct ufshc_xfer_req_upiu *)req;
		rsp_upiu = (struct ufshc_xfer_resp_upiu *)rsp;
		/* Fill query UPIU fields */
		ufshc_fill_query_upiu(ufshc, slot,
				      (int32_t)req_upiu->query_req_upiu.tsf.opcode,
				      req_upiu->query_req_upiu.tsf.desc_id,
				      req_upiu->query_req_upiu.tsf.index,
				      req_upiu->query_req_upiu.tsf.selector,
				      req_upiu->query_req_upiu.tsf.value,
				      req_upiu->query_req_upiu.tsf.length);
		/* Send the query UPIU command */
		ret = ufshc_send_upiu_cmd(ufshc, slot);
		if (rsp != NULL) {
			/* Copy the response UPIU */
			(void)memcpy(rsp_upiu, &ucd_ptr->resp_upiu, sizeof(*rsp_upiu));
		}
		break;
	case UFSHC_CMD_UPIU_TRANS_CODE: /* SCSI command UPIU */
		pccb = (struct scsi_cmd *)req;
		/* Fill SCSI command-specific UPIU fields */
		ufshc_fill_scsi_cmd_upiu(ufshc, slot, pccb);
		/* Send the SCSI UPIU command */
		ret = ufshc_send_upiu_cmd(ufshc, slot);
		if (ret != 0) {
			ret = -EINVAL;
		}
		break;
	case UFSHC_TSK_UPIU_TRANS_CODE: /* Task management UPIU command */
		ret = -ENOTSUP;
		break;
	default:
		ret = -EINVAL;
		break;
	};

	return ret;
}

/**
 * @brief Read or write descriptors in the UFS host controller
 *
 * This function reads or writes the specified UFS descriptor based on the operation mode.
 * If writing, the contents of `param_buff` are written to the descriptor. If reading,
 * the descriptor contents are copied into `param_buff` starting at `param_offset`.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param write Boolean indicating whether to write (true) or read (false) the descriptors
 * @param idn Descriptor identifier
 * @param index Descriptor index
 * @param param_offset Offset for the parameters in the descriptor
 * @param param_buff Pointer to the buffer holding the parameters to read/write
 * @param param_size Size of the parameter buffer
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_rw_descriptors(struct ufs_host_controller *ufshc,
			     bool write, uint8_t idn, uint8_t index,
			     uint8_t param_offset, void *param_buff,
			     uint8_t param_size)
{
	int32_t ret;
	int32_t opcode = (int32_t)UFSHC_QRY_READ_DESC_CMD;
	uint8_t desc_buf[UFSHC_QRY_DESC_MAX_SIZE] = {0};

	/* Validate input parameters */
	if ((param_buff == NULL) || (param_size == 0U)) {
		ret = -EINVAL;
		goto out;
	}

	/* Handle write operation */
	if (write) {
		opcode = (int32_t)UFSHC_QRY_WRITE_DESC_CMD;
		(void)memcpy((void *)desc_buf, param_buff, param_size);
	}

	/* Perform the descriptor query operation (read or write) */
	ret = ufshc_query_descriptor(ufshc, opcode,
				     (int32_t)idn, index, 0,
				     desc_buf, (int32_t)UFSHC_QRY_DESC_MAX_SIZE);
	if ((ret == 0) && (write == false)) {
		/* copy from descriptor buffer to user buffer */
		(void)memcpy(param_buff,
			    (const void *)(&desc_buf[param_offset]),
			     param_size);
	}

out:
	return ret;
}

/**
 * @brief Send attribute query requests to the UFS host controller
 *
 * This function performs a read or write operation for a specific UFS attribute
 * identified by `idn`.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param write Boolean indicating whether to write (true) or read (false) the attribute
 * @param idn Attribute identifier
 * @param data Pointer to the attribute value to read/write
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_rw_attributes(struct ufs_host_controller *ufshc,
			    bool write, uint8_t idn, void *data)
{
	int32_t ret;
	int32_t opcode = (int32_t)UFSHC_QRY_READ_ATTR_CMD;
	uint32_t attr_value = 0;

	/* Validate input parameters */
	if (data == NULL) {
		ret = -EINVAL;
		goto out;
	}

	/* Handle write operation */
	if (write) {
		opcode = (int32_t)UFSHC_QRY_WRITE_ATTR_CMD;
		attr_value = *(uint32_t *)data;
	}

	/* Perform the attribute query operation (read or write) */
	ret = ufshc_query_attr(ufshc, opcode, (int32_t)idn, 0, 0, &attr_value);
	if ((ret == 0) && (write == false)) {
		/* copy attribute value */
		(void)memcpy(data, (const void *)&attr_value, sizeof(attr_value));
	}

out:
	return ret;
}

/**
 * @brief Read or write flags in the UFS host controller
 *
 * This function performs a read or write operation for a UFS flag specified by
 * `idn` and `index`. Supports setting, clearing, or reading the flag value.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param write Boolean indicating whether to write (true) or read (false) the flags
 * @param idn Flag identifier
 * @param index Flag index
 * @param data  Pointer to the data buffer for reading/writing the flag
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_rw_flags(struct ufs_host_controller *ufshc,
		       bool write, uint8_t idn, uint8_t index, void *data)
{
	int32_t ret;
	int32_t opcode = (int32_t)UFSHC_QRY_READ_FLAG_CMD;
	bool flag_value = false;

	/* Validate input parameters */
	if (data == NULL) {
		ret = -EINVAL;
		goto out;
	}

	/* Handle write flag operation */
	if (write) {
		flag_value = *(bool *)data;
		if (!flag_value) {
			/* Clear flag if value is false */
			opcode = (int32_t)UFSHC_QRY_CLR_FLAG_CMD;
		} else {
			/* Set flag if value is true */
			opcode = (int32_t)UFSHC_QRY_SET_FLAG_CMD;
		}
	}

	/* Perform the flag query operation (set, clear, or read) */
	ret = ufshc_query_flag(ufshc, opcode,
			       (int32_t)idn, index,
			       (bool *)(&flag_value));
	if ((ret == 0) && (write == false)) {
		/* copy flag value */
		(void)memcpy(data, (const void *)&flag_value, sizeof(flag_value));
	}

out:
	return ret;
}
