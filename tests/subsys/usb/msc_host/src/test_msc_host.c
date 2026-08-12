/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh_msc.h>
#include <zephyr/usb/usbh_msc_bot.h>
#include <zephyr/usb/usbh_msc_scsi_cmd.h>
#include <zephyr/ztest.h>

ZTEST(msc_host, test_subclass_scsi_supported)
{
	zassert_true(usbh_msc_subclass_scsi_supported(USBH_MSC_SC_UFI));
	zassert_true(usbh_msc_subclass_scsi_supported(USBH_MSC_SC_8070));
	zassert_true(usbh_msc_subclass_scsi_supported(USBH_MSC_SC_SCSI));
	zassert_false(usbh_msc_subclass_scsi_supported(USBH_MSC_SC_RBC));
}

ZTEST(msc_host, test_bot_read10_cbw)
{
	struct usb_msc_bot_cbw cbw;

	usb_msc_bot_fill_read10_cbw(&cbw, 0x42U, 0U, 1U, 1U, 512U);

	zassert_equal(sys_le32_to_cpu(cbw.dCBWSignature), USB_MSC_BOT_CBW_SIGNATURE);
	zassert_equal(sys_le32_to_cpu(cbw.dCBWTag), 0x42U);
	zassert_equal(sys_le32_to_cpu(cbw.dCBWDataTransferLength), 512U);
	zassert_equal(cbw.bmCBWFlags, USB_MSC_BOT_CBW_FLAG_DATA_IN);
	zassert_equal(cbw.bCBWLUN, 0U);
	zassert_equal(cbw.bCBWCBLength, 10U);
	zassert_equal(cbw.CBWCB[0], USB_SCSI_READ10);
	zassert_equal(sys_be32_to_cpu(*(uint32_t *)&cbw.CBWCB[2]), 1U);
	zassert_equal(sys_be16_to_cpu(*(uint16_t *)&cbw.CBWCB[7]), 1U);
}

ZTEST(msc_host, test_bot_write10_cbw)
{
	struct usb_msc_bot_cbw cbw;

	usb_msc_bot_fill_write10_cbw(&cbw, 7U, 2U, 100U, 4U, 512U);

	zassert_equal(sys_le32_to_cpu(cbw.dCBWTag), 7U);
	zassert_equal(sys_le32_to_cpu(cbw.dCBWDataTransferLength), 4U * 512U);
	zassert_equal(cbw.bmCBWFlags, 0U);
	zassert_equal(cbw.bCBWLUN, 2U);
	zassert_equal(cbw.CBWCB[0], USB_SCSI_WRITE10);
}

ZTEST(msc_host, test_bot_verify10_cbw)
{
	struct usb_msc_bot_cbw cbw;

	usb_msc_bot_fill_verify10_cbw(&cbw, 3U, 0U, 0U, 8U);

	zassert_equal(sys_le32_to_cpu(cbw.dCBWDataTransferLength), 0U);
	zassert_equal(cbw.bCBWCBLength, USB_SCSI_VERIFY10_CDB_LEN);
	zassert_equal(cbw.CBWCB[0], USB_SCSI_VERIFY10);
	zassert_equal(sys_be16_to_cpu(*(uint16_t *)&cbw.CBWCB[7]), 8U);
}

ZTEST_SUITE(msc_host, NULL, NULL, NULL, NULL, NULL);
