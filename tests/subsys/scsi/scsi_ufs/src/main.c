/*
 * Copyright 2025 (c) Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/ufs/ufs.h>
#include <zephyr/ztest.h>

#define UFS_NODE DT_ALIAS(ufs0)

BUILD_ASSERT(DT_NODE_HAS_STATUS(UFS_NODE, okay), "UFS node is disabled!");

#define SCSI_TEST_LUN 0

static struct scsi_host_info *tst_shost;
static struct scsi_device *tst_sdev;

static const struct device *const tst_ufshcdev = DEVICE_DT_GET_OR_NULL(UFS_NODE);
static struct ufs_host_controller *tst_ufshc;

#if IS_ENABLED(CONFIG_TEST_SCSI_UFS_RW)
#define SCSI_TEST_SECTOR_COUNT 3
#define SCSI_TEST_MAX_SECTOR_SIZE 4096
#define SCSI_TEST_BUF_SIZE (SCSI_TEST_SECTOR_COUNT * SCSI_TEST_MAX_SECTOR_SIZE)
static uint8_t wr_buf[SCSI_TEST_BUF_SIZE] __aligned(CONFIG_UFSHC_BUFFER_ALIGNMENT);
static uint8_t rd_buf[SCSI_TEST_BUF_SIZE] __aligned(CONFIG_UFSHC_BUFFER_ALIGNMENT);
#endif /* CONFIG_TEST_SCSI_UFS_RW */

/* Verify that SCSI Device Handle is Initialized */
ZTEST(scsi_stack, test_0_init)
{
	tst_sdev = scsi_device_lookup_by_host(tst_shost, SCSI_TEST_LUN);
	zassert_not_null(tst_sdev, "SCSI Device for SCSI_TEST_LUN is NULL");
}

/* Verify SCSI cmds using IOCTL */
ZTEST(scsi_stack, test_1_scsi_cmd)
{
	int32_t rc;

	if (tst_sdev == NULL) {
		ztest_test_skip();
		return;
	}

	rc = scsi_ioctl(tst_sdev, SCSI_IOCTL_TEST_UNIT_READY, NULL);
	zassert_equal(rc, 0, "SCSI_CMD - TUR failed: %d", rc);
}

/* Verify SCSI Generic (SG) IOCTL */
ZTEST(scsi_stack, test_2_scsi_sgio)
{
	int32_t rc;
	struct sg_io_req req;
	uint8_t tur_cmd[] = {SCSI_TST_U_RDY, 0, 0, 0, 0, 0};

	if (tst_sdev == NULL) {
		ztest_test_skip();
		return;
	}

	req.protocol = BSG_PROTOCOL_SCSI;
	req.subprotocol = BSG_SUB_PROTOCOL_SCSI_CMD;
	req.request = &tur_cmd[0];
	req.request_len = (uint32_t)sizeof(tur_cmd);
	req.dxfer_dir = (int32_t)PERIPHERAL_TO_PERIPHERAL;
	req.dxferp = NULL;
	rc = scsi_ioctl(tst_sdev, SG_IO, &req);
	zassert_equal(rc, 0, "SCSI_IOCTL - SGIO - TUR failed: %d", rc);
}

#if IS_ENABLED(CONFIG_TEST_SCSI_UFS_RW)
/* Verify that SCSI stack can read/write to UFS card */
ZTEST(scsi_stack, test_3_scsi_rw)
{
	int32_t rc;
	int32_t block_addr = 0;

	if (tst_sdev == NULL) {
		ztest_test_skip();
		return;
	}

	memset(wr_buf, 0xAD, SCSI_TEST_BUF_SIZE);
	memset(rd_buf, 0, SCSI_TEST_BUF_SIZE);

	rc = scsi_write(tst_sdev, block_addr, SCSI_TEST_SECTOR_COUNT, wr_buf);
	zassert_equal(rc, 0, "Write SCSI failed: %d", rc);
	rc = scsi_read(tst_sdev, block_addr, SCSI_TEST_SECTOR_COUNT, rd_buf);
	zassert_equal(rc, 0, "Read SCSI failed: %d", rc);
	zassert_mem_equal(wr_buf, rd_buf, SCSI_TEST_BUF_SIZE,
			"Read of erased area was not zero");
}
#endif /* CONFIG_TEST_SCSI_UFS_RW */

/*
 * Scsi UFS Device Setup
 * This setup must run first, to ensure UFS card is initialized.
 */
static void *scsi_test_setup(void)
{
	int32_t rc;

	zassert_true(device_is_ready(tst_ufshcdev), "UFSHC device is not ready");
	rc = ufs_init(tst_ufshcdev, &tst_ufshc);
	zassert_equal(rc, 0, "UFS initialization failed: %d", rc);
	tst_shost = tst_ufshc->host;

	return NULL;
}

ZTEST_SUITE(scsi_stack, NULL, scsi_test_setup, NULL, NULL, NULL);
