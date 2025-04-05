/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file implements requests to configure UFS device.
 * It includes functions for handling SCSI Generic (SG) requests
 * related to UFS operations querying attributes, flags, and descriptors.
 */

#include <zephyr/ufs/ufs_ops.h>

/**
 * @brief Handle query-based requests for UFS
 *
 * This function processes query-based IOCTL requests for the UFS device.
 * attribute, flag, and descriptor queries.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param arg Pointer to the SG IO request structure containing query parameters
 *
 * @return 0 on success, negative error code on failure
 */
static int32_t ufs_qry_ioctl_request(struct ufs_host_controller *ufshc,
				     struct sg_io_req *req)
{
	int32_t ret = 0;
	int32_t ioctl_id;
	struct ufs_sg_req *ufs_req = (struct ufs_sg_req *)req->request;
	struct ufs_qry_ioctl_req *qry_ioctl_req = ufs_req->req_qry_ioctl;
	bool write_opr;
	struct ufs_qry_ioctl_attr *attr_param;
	struct ufs_qry_ioctl_flag *flag_param;
	struct ufs_qry_ioctl_desc *desc_param;

	if (qry_ioctl_req == NULL) {
		return -EINVAL;
	}

	ioctl_id = (int32_t)(qry_ioctl_req->ioctl_id);

	/* read or write based on the data transfer direction */
	if (req->dxfer_dir == SG_DXFER_TO_DEV) {
		write_opr = true;
	} else if (req->dxfer_dir == SG_DXFER_FROM_DEV) {
		write_opr = false;
	} else {
		return -EINVAL;
	}

	/* Handle different types of queries */
	switch (ioctl_id) {
	case UFS_QRY_IOCTL_ATTR:
		attr_param = (struct ufs_qry_ioctl_attr *)&qry_ioctl_req->attr;
		ret = ufshc_rw_attributes(ufshc, write_opr,
					  attr_param->attr_id,
					  req->dxferp);
		break;
	case UFS_QRY_IOCTL_FLAG:
		flag_param = (struct ufs_qry_ioctl_flag *)&qry_ioctl_req->flag;
		ret = ufshc_rw_flags(ufshc, write_opr,
				     flag_param->flag_id, 0,
				     req->dxferp);
		break;
	case UFS_QRY_IOCTL_DESC:
		desc_param = (struct ufs_qry_ioctl_desc *)&qry_ioctl_req->desc;
		ret = ufshc_rw_descriptors(ufshc, write_opr,
					   desc_param->desc_id,
					   desc_param->index,
					   desc_param->param_offset,
					   req->dxferp,
					   (uint8_t)req->dxfer_len);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief Handle a SCSI Generic (SG) request for UFS
 *
 * This function dispatches an SG_IO request to the appropriate UFS operation.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param arg Argument for the request, which could be a UFS query or other data
 *
 * @return 0 on success, negative error code on failure
 */
int32_t ufs_sg_request(struct ufs_host_controller *ufshc, void *arg)
{
	int32_t ret;
	struct sg_io_req *req = (struct sg_io_req *)arg;
	struct ufs_sg_req *ufs_req;
	int32_t msgcode;

	if ((req == NULL) || (req->protocol != BSG_PROTOCOL_SCSI) ||
	    (req->subprotocol != BSG_SUB_PROTOCOL_SCSI_TRANSPORT) ||
	    (req->request == NULL) || (req->dxferp == NULL)) {
		ret = -EINVAL;
		goto out;
	}

	ufs_req = (struct ufs_sg_req *)req->request;
	msgcode = ufs_req->msgcode;

	/* Dispatch based on the message code */
	switch (msgcode) {
	case (int32_t)UFS_SG_QUERY_REQ:
		ret = ufs_qry_ioctl_request(ufshc, req);
		break;
	case (int32_t)UFS_SG_TASK_REQ:
		ret = -ENOTSUP;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

out:
	return ret;
}
