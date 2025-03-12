/*
 * Copyright 2025 (c) Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/ufs/ufs.h>
#include <zephyr/ufs/ufs_ops.h>
#include <zephyr/ztest.h>

#define UFS_NODE DT_ALIAS(ufs0)

BUILD_ASSERT(DT_NODE_HAS_STATUS(UFS_NODE, okay), "UFS node is disabled!");

static const struct device *const tst_ufshcdev = DEVICE_DT_GET_OR_NULL(UFS_NODE);
static struct ufs_host_controller *tst_ufshc;

/* UFS card initialization */
ZTEST(ufs_stack, test_0_init)
{
	int32_t ret;

	ret = ufs_init(tst_ufshcdev, &tst_ufshc);
	zassert_equal(ret, 0, "UFS initialization failed: %d", ret);
}

/* Read descriptor information from UFS card */
ZTEST(ufs_stack, test_1_ioctl_desc)
{
	int32_t ret;
	struct sg_io_req sg_req;
	struct ufs_sg_req ufs_req;
	struct ufs_qry_ioctl_req qry_ioctl_req;
	uint8_t lun_enable = 0;
	uint8_t test_lun = 0;

	sg_req.protocol = BSG_PROTOCOL_SCSI;
	sg_req.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;
	sg_req.request = &ufs_req;

	sg_req.dxfer_dir = SG_DXFER_FROM_DEV;
	sg_req.dxferp = (uint8_t *)(&lun_enable);
	sg_req.dxfer_len = (uint32_t)sizeof(lun_enable);

	ufs_req.msgcode = (int32_t)UFS_SG_QUERY_REQ;
	ufs_req.req_qry_ioctl = &qry_ioctl_req;

	qry_ioctl_req.ioctl_id = UFS_QRY_IOCTL_DESC;
	qry_ioctl_req.desc.desc_id = UFSHC_UNIT_DESC_IDN;
	qry_ioctl_req.desc.index = test_lun;
	qry_ioctl_req.desc.param_offset = UFSHC_UD_PARAM_LU_ENABLE;

	ret = ufs_sg_request(tst_ufshc, &sg_req);
	zassert_equal(ret, 0, "UFS IOCTL desc failed: %d", ret);

	TC_PRINT("Lun id:%d, lun_enable:%Xh\n", test_lun, lun_enable);
}

/* Read attribute value from UFS card */
ZTEST(ufs_stack, test_2_ioctl_attr)
{
	int32_t ret;
	struct sg_io_req sg_req;
	struct ufs_sg_req ufs_req;
	struct ufs_qry_ioctl_req qry_ioctl_req;
	uint32_t blun_attrval = 0U;

	sg_req.protocol = BSG_PROTOCOL_SCSI;
	sg_req.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;
	sg_req.request = &ufs_req;

	sg_req.dxfer_dir = SG_DXFER_FROM_DEV;
	sg_req.dxferp = &blun_attrval;

	ufs_req.msgcode = (int32_t)UFS_SG_QUERY_REQ;
	ufs_req.req_qry_ioctl = &qry_ioctl_req;

	qry_ioctl_req.ioctl_id = UFS_QRY_IOCTL_ATTR;
	qry_ioctl_req.attr.attr_id = UFSHC_BLUEN_ATTRID;

	ret = ufs_sg_request(tst_ufshc, &sg_req);
	zassert_equal(ret, 0, "UFS IOCTL attr failed: %d", ret);

	TC_PRINT("bootlun_attrval:%Xh\n", blun_attrval);
}

/* Read flag value from UFS card */
ZTEST(ufs_stack, test_3_ioctl_flag)
{
	int32_t ret;
	struct sg_io_req sg_req;
	struct ufs_sg_req ufs_req;
	struct ufs_qry_ioctl_req qry_ioctl_req;
	bool flag_val = false;

	sg_req.protocol = BSG_PROTOCOL_SCSI;
	sg_req.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;
	sg_req.request = &ufs_req;

	sg_req.dxfer_dir = SG_DXFER_FROM_DEV;
	sg_req.dxferp = &flag_val;

	ufs_req.msgcode = (int32_t)UFS_SG_QUERY_REQ;
	ufs_req.req_qry_ioctl = &qry_ioctl_req;

	qry_ioctl_req.ioctl_id = UFS_QRY_IOCTL_FLAG;
	qry_ioctl_req.flag.flag_id = UFSHC_FDEVINIT_FLAG_IDN;

	ret = ufs_sg_request(tst_ufshc, &sg_req);
	zassert_equal(ret, 0, "UFS IOCTL Flag failed: %d", ret);

	TC_PRINT("fdeviceinit_flag:%Xh\n", flag_val);
}

/* UFS device setup */
static void *ufs_test_setup(void)
{
	zassert_true(device_is_ready(tst_ufshcdev), "UFSHC device is not ready");

	return NULL;
}

ZTEST_SUITE(ufs_stack, NULL, ufs_test_setup, NULL, NULL, NULL);
