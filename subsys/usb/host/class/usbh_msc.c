/**
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  usbh_msc.c
 * @brief Mass Storage Class Driver
 *
 * Mass Storage Class driver implementation using BOT and SCSI transparent
 * command set. Implementation follows the Mass Storage Class Specification
 * (Mass_Storage_Specification_Overview_v1.4_2-19-2010.pdf) and
 * Mass Storage Class Bulk-Only Transport Specification
 * (usbmassbulk_10.pdf).
 */

#include <stdint.h>
#include "../usbh_ch9.h"
#include <zephyr/usb/class/usbh_msc.h>
#include <zephyr/usb/class/usb_msc.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/disk.h>

LOG_MODULE_REGISTER(usbh_msc, CONFIG_USBH_MSC_LOG_LEVEL);

/* data stage number in a control transfer */
#define CTRL_DATA_STAGE		          (2u)
/* number of bulk endpoints required for MSC */
#define MSC_ENDPOINTS_NUM	          (2u)
/* pipe number of bulk in endpoint */
#define BULK_IN_PIPE_NUM	          (2u)
/* pipe number of bulk out endpoint */
#define BULK_OUT_PIPE_NUM	          (3u)
/* data length of SCSI test unit ready command */
#define DATA_LEN_MODE_TEST_UNIT_READY     (0u)
/* data length of SCSI read capacity command */
#define DATA_LEN_READ_CAPACITY10          (8u)
/* data length of SCSI inquiry command */
#define DATA_LEN_INQUIRY	          (36u)
/* data length of SCSI request sense command */
#define DATA_LEN_REQUEST_SENSE	          (14u)
/* command block length in a Command Block Wrapper */
#define CBW_CB_LENGTH		          (16u)
/* length of command descriptor block in the Command Block field */
#define CBW_LENGTH		          (10u)
/* total length of Command Block Wrapper */
#define CBW_TOTAL_LENGTH	          (31u)
/* tag used by Command Block Wrapper */
#define CBW_TAG			          (0x20304050u)
/* total length of Command Status Wrapper */
#define CSW_TOTAL_LENGTH	          (13u)
/* timeout value for transfer request allocation */
#define MSC_REQ_TIMEOUT		          (1000u)
/* bit position used by drive state atomic variable */
#define DRIVE_READY		          (0u)
/* interface number used for BOT reset */
#define MSC_INTERFACE_NUM	          (0x0u)
/* default block size */
#define DEFAULT_BLOCK_SIZE	          (512u)
/* default LUN used */
#define DEFAULT_LUN		          (0u)

/* Drive data reported by the device */
struct drive {
	/* drive state */
	atomic_t state;

	/* get max LUN response data */
	uint8_t max_lun;

	/* inquiry command response data */
	uint8_t peripheral_qualifier;
	uint8_t peripheral_dev_type;
	uint8_t spc_version;
	uint8_t t10_vendor_id[8];
	uint8_t product_id[16];
	uint8_t product_revision[4];

	/* read capacity command response data */
	uint32_t total_blocks;
	uint32_t block_size;

	/* last request sense response data */
	bool sense_valid;
	uint8_t sense_key;
	uint8_t sense_asc;
	uint8_t sense_ascq;
};

/* Endpoint data reported by the device */
struct ep_data {
	uint8_t bulk_in_num;
	uint8_t bulk_out_num;
	uint16_t bulk_in_mps;
	uint16_t bulk_out_mps;
};

/* SCSI commmand states */
enum scsi_state {
	GET_MAX_LUN = 0,
	SCSI_INQUIRY,
	SCSI_TEST_UNIT_READY,
	SCSI_READ_CAPACITY,
	SCSI_REQ_SENSE,
	SCSI_COMPLETE,
};

/* Bulk Only Transport states */
enum bot_state {
	TX_CBW = 0,
	DATA_IN,
	DATA_OUT,
	RX_CSW,
	HANDLE_EP_STALL,
	REQ_SENSE,
	XFER_COMPLETE,
	XFER_FAILED,
};

/* Bulk Only Transport error states */
enum bot_error_state {
	BOT_SUCCESS = 0,
	BOT_FAIL,
	BOT_REQ_SENSE,
	BOT_DEV_REMOVED,
};

/* Global MSC handle to track all communications */
struct msc_handle {
	/* device instance */
	const struct device *dev;
	/* semaphore to signal class transfer events */
	struct k_sem msc_xfr_sem;
	/* data buffer pointer */
	uint8_t *buf;
	/* drive data structure */
	struct drive drv;
	/* current transfer state of the class */
	enum uhc_xfer_state_type msc_xfer_state;
	/* endpoint data structure */
	struct ep_data ep;
	/* device address */
	uint8_t dev_addr;
	/* LUN value */
	uint8_t lun;
	/* command block wrapper data structure */
	struct CBW cbw;
	/* command status wrapper data structure */
	struct CSW csw;
};

static struct msc_handle msc_handle;
static struct msc_handle *const p_msc_handle = &msc_handle;

/* Drive access mutex */
K_MUTEX_DEFINE(drv_access_mutex);

/* Class code structure used for registration with core layer */
static const struct usbh_class_code class_code = {
	/* device dlass code */
	.dclass = USB_BCC_MASS_STORAGE,
	/* class subclass code */
	.sub = SCSI_TRANSPARENT_SUBCLASS,
	/* class protocol code */
	.proto = BULK_ONLY_TRANSPORT_PROTOCOL};

/* Connected function is called on device attachment */
int connected(struct usbh_contex *const uhs_ctx);

/* Request is callled once the transfer is completed */
int request(struct usbh_contex *const uhs_ctx, struct uhc_transfer *const xfer, int err);

/* Removed is called on device removal */
int removed(struct usbh_contex *const uhs_ctx);

/* Registering class with core layer */
USBH_DEFINE_CLASS(msc, class_code, request, connected, removed);

/* Device endpoint data is populated */
static int populate_device_metadata(struct uhc_data *data)
{
	memset((void *)&p_msc_handle->ep, 0, sizeof(struct ep_data));

	if (data->if_descriptor.bNumEndpoints != MSC_ENDPOINTS_NUM) {
		return -EINVAL;
	}

	if ((data->ep_descriptor[0].bmAttributes != USB_EP_TYPE_BULK) ||
	    (data->ep_descriptor[1].bmAttributes != USB_EP_TYPE_BULK)) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(data->ep_descriptor[0].bEndpointAddress)) {
		p_msc_handle->ep.bulk_in_num = data->ep_descriptor[0].bEndpointAddress;
		p_msc_handle->ep.bulk_in_mps = data->ep_descriptor[0].wMaxPacketSize;
		p_msc_handle->ep.bulk_out_num = data->ep_descriptor[1].bEndpointAddress;
		p_msc_handle->ep.bulk_out_mps = data->ep_descriptor[1].wMaxPacketSize;
	} else {
		p_msc_handle->ep.bulk_in_num = data->ep_descriptor[1].bEndpointAddress;
		p_msc_handle->ep.bulk_in_mps = data->ep_descriptor[1].wMaxPacketSize;
		p_msc_handle->ep.bulk_out_num = data->ep_descriptor[0].bEndpointAddress;
		p_msc_handle->ep.bulk_out_mps = data->ep_descriptor[0].wMaxPacketSize;
	}

	return 0;
}

/* Clear Endpoint feature request */
static int usbh_req_clear_feature(const struct device *dev, const uint8_t addr, const uint16_t ep)
{
	const uint8_t bmRequestType =
		(USB_REQTYPE_DIR_TO_DEVICE << 7) | USB_REQTYPE_RECIPIENT_ENDPOINT;
	const uint8_t bRequest = USB_SREQ_CLEAR_FEATURE;
	const uint16_t wValue = 0;
	const uint16_t wIndex = ep;

	return usbh_req_setup(dev, addr, bmRequestType, bRequest, wValue, wIndex, 0, NULL);
}

/* Request BOT reset */
static int req_msc_bot_reset(const struct device *dev, const uint8_t addr)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				      (USB_REQTYPE_TYPE_CLASS << 5) |
				      USB_REQTYPE_RECIPIENT_INTERFACE;

	return usbh_req_setup(dev, addr, bmRequestType, MSC_REQUEST_RESET, 0, MSC_INTERFACE_NUM, 0,
			      NULL);
}

/* Initiate reset recovery */
static int msc_initiate_reset_recovery(const struct device *dev, const uint8_t addr)
{
	int ret_val;

	p_msc_handle->msc_xfer_state = XFER_STARTED;
	ret_val = req_msc_bot_reset(dev, addr);
	if (ret_val) {
		return ret_val;
	}
	k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);
	p_msc_handle->msc_xfer_state = XFER_STARTED;
	ret_val = usbh_req_clear_feature(dev, addr, p_msc_handle->ep.bulk_in_num);
	if (ret_val) {
		return ret_val;
	}
	k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);
	p_msc_handle->msc_xfer_state = XFER_STARTED;
	ret_val = usbh_req_clear_feature(dev, addr, p_msc_handle->ep.bulk_out_num);
	if (ret_val) {
		return ret_val;
	}
	k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);

	return 0;
}

/* Request for bulk endpoint transfer */
static int msc_req_xfer(const struct device *dev, const uint8_t addr, const uint8_t ep_addr,
			const uint8_t ep_attrib, const uint16_t ep_mps, const uint16_t data_length,
			uint8_t *const data)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;

	xfer = uhc_xfer_alloc(dev, addr, ep_addr, ep_attrib, ep_mps, MSC_REQ_TIMEOUT, NULL);
	if (!xfer) {
		LOG_ERR("Transfer allocation failed");
		return -ENOMEM;
	}

	buf = uhc_xfer_buf_alloc(dev, xfer, data_length);
	if (!buf) {
		LOG_ERR("Buffer allocation failed");
		uhc_xfer_free(dev, xfer);
		return -ENOMEM;
	}

	if (USB_EP_DIR_IS_OUT(ep_addr) && data) {
		net_buf_add_mem(buf, data, data_length);
	}

	return uhc_ep_enqueue(dev, xfer);
}

/* Request bulk out transfer */
static int msc_tx_data(const struct device *dev, const uint8_t addr, const uint16_t data_length,
		       uint8_t *const data)
{
	return msc_req_xfer(dev, addr, p_msc_handle->ep.bulk_out_num, USB_EP_TYPE_BULK,
			    p_msc_handle->ep.bulk_out_mps, data_length, data);
}

/* Request bulk in transfer */
static int msc_rx_data(const struct device *dev, const uint8_t addr, const uint16_t data_length,
		       uint8_t *const data)
{
	return msc_req_xfer(dev, addr, p_msc_handle->ep.bulk_in_num, USB_EP_TYPE_BULK,
			    p_msc_handle->ep.bulk_in_mps, data_length, data);
}

/* Decode Command Status Wrapper */
static uint8_t decode_csw(void)
{
	uint8_t ret_val = CSW_STATUS_PHASE_ERROR;

	if (p_msc_handle->csw.Signature == CSW_Signature) {
		ret_val = CSW_STATUS_CMD_FAILED;
		if (p_msc_handle->csw.Tag == p_msc_handle->cbw.Tag) {
			ret_val = CSW_STATUS_PHASE_ERROR;
			if (p_msc_handle->csw.Status == 0) {
				ret_val = CSW_STATUS_CMD_PASSED;
			} else if (p_msc_handle->csw.Status == 1) {
				ret_val = CSW_STATUS_CMD_FAILED;
			}
		}
	}

	LOG_DBG("dCSWDataResidue = %d", p_msc_handle->csw.DataResidue);

	return ret_val;
}

/* Transfer Bulk Only Transport (BOT) command */
static enum bot_error_state xfer_msc_bot_cmd(const struct device *dev, const uint8_t addr)
{
	enum bot_error_state ret_val = BOT_FAIL;
	int req_ret;
	uint32_t data_len = 0;
	bool xfer_in_progress = true;
	enum bot_state bot_state = TX_CBW;
	uint16_t retry = 0;
	uint8_t csw_status = CSW_STATUS_CMD_FAILED;
	struct uhc_data *data = dev->data;
	/* maximum buffer size which can be requested */
	uint32_t max_buf_len = CONFIG_UHC_BUF_POOL_SIZE / 2;

	while (atomic_test_bit(&data->status, UHC_STATUS_DEV_CONN) && xfer_in_progress &&
		(retry <= CONFIG_MSC_BOT_MAX_RETRY)) {
		switch (bot_state) {
		case TX_CBW:
			p_msc_handle->msc_xfer_state = XFER_STARTED;
			data_len = CBW_TOTAL_LENGTH;
			msc_tx_data(dev, addr, data_len, (uint8_t *)&p_msc_handle->cbw);
			k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);
			if (p_msc_handle->msc_xfer_state == XFER_DONE) {
				retry = 0;
				bot_state = RX_CSW;
				if (p_msc_handle->cbw.DataLength != 0) {
					bot_state = DATA_OUT;
					if (p_msc_handle->cbw.Flags == USB_EP_DIR_IN) {
						bot_state = DATA_IN;
					}
				}
			} else if (p_msc_handle->msc_xfer_state == XFER_STALL) {
				bot_state = HANDLE_EP_STALL;
			} else {
				retry++;
			}
			break;

		case DATA_IN:
			if (p_msc_handle->cbw.DataLength) {
				p_msc_handle->msc_xfer_state = XFER_STARTED;
				data_len = p_msc_handle->cbw.DataLength;

				if (p_msc_handle->cbw.DataLength > max_buf_len) {
					data_len = max_buf_len;
				}

				req_ret = msc_rx_data(dev, addr, data_len, NULL);
				if (!req_ret) {
					k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);
					if (p_msc_handle->msc_xfer_state == XFER_DONE) {
						p_msc_handle->cbw.DataLength -= data_len;
						p_msc_handle->buf += data_len;
					} else if (p_msc_handle->msc_xfer_state == XFER_STALL) {
						bot_state = HANDLE_EP_STALL;
					}
				} else {
					bot_state = XFER_FAILED;
				}
			} else {
				bot_state = RX_CSW;
			}
			break;

		case DATA_OUT:
			if (p_msc_handle->cbw.DataLength) {
				p_msc_handle->msc_xfer_state = XFER_STARTED;
				data_len = p_msc_handle->cbw.DataLength;

				if (p_msc_handle->cbw.DataLength > max_buf_len) {
					data_len = max_buf_len;
				}

				req_ret = msc_tx_data(dev, addr, data_len, p_msc_handle->buf);
				if (!req_ret) {
					k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);
					if (p_msc_handle->msc_xfer_state == XFER_DONE) {
						p_msc_handle->cbw.DataLength -= data_len;
						p_msc_handle->buf += data_len;
						retry = 0;
					} else if (p_msc_handle->msc_xfer_state == XFER_STALL) {
						bot_state = HANDLE_EP_STALL;
					} else {
						retry++;
					}
				} else {
					bot_state = XFER_FAILED;
				}
			} else {
				bot_state = RX_CSW;
			}
			break;

		case RX_CSW:
			p_msc_handle->msc_xfer_state = XFER_STARTED;
			data_len = CSW_TOTAL_LENGTH;
			p_msc_handle->buf = (uint8_t *)&p_msc_handle->csw;

			req_ret = msc_rx_data(dev, addr, data_len, NULL);
			if (!req_ret) {
				k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);
				if (p_msc_handle->msc_xfer_state == XFER_DONE) {
					csw_status = decode_csw();
					bot_state = XFER_COMPLETE;
					if (csw_status == CSW_STATUS_CMD_FAILED) {
						bot_state = REQ_SENSE;
					} else if (csw_status == CSW_STATUS_PHASE_ERROR) {
						bot_state = HANDLE_EP_STALL;
					}
				} else if (p_msc_handle->msc_xfer_state == XFER_STALL) {
					bot_state = HANDLE_EP_STALL;
				}
			} else {
				bot_state = XFER_FAILED;
			}
			break;

		case HANDLE_EP_STALL:
			req_ret = msc_initiate_reset_recovery(dev, addr);
			if (req_ret) {
				LOG_ERR("Reset recovery failed");
			}
			xfer_in_progress = false;
			ret_val = BOT_FAIL;
			break;

		case REQ_SENSE:
			ret_val = BOT_REQ_SENSE;
			xfer_in_progress = false;
			break;

		case XFER_COMPLETE:
			xfer_in_progress = false;
			ret_val = BOT_SUCCESS;
			break;

		case XFER_FAILED:
		default:
			xfer_in_progress = false;
			ret_val = BOT_FAIL;
			break;
		}
	}

	/* update BOT status on device disconnection */
	if (!atomic_test_bit(&data->status, UHC_STATUS_DEV_CONN)) {
		ret_val = BOT_DEV_REMOVED;
	}

	return ret_val;
}

/* SCSI request to get max LUN */
static enum bot_error_state req_msc_scsi_get_max_lun(const struct device *dev, const uint8_t addr)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				      (USB_REQTYPE_TYPE_CLASS << 5) |
				      USB_REQTYPE_RECIPIENT_INTERFACE;
	const uint8_t bRequest = MSC_REQUEST_GET_MAX_LUN;
	const uint16_t wValue = 0;
	const uint16_t wIndex = 0;
	const uint16_t wLength = 1;
	enum bot_error_state ret_val = BOT_FAIL;
	uint8_t data;

	p_msc_handle->buf = &data;
	p_msc_handle->msc_xfer_state = XFER_STARTED;
	usbh_req_setup(dev, addr, bmRequestType, bRequest, wValue, wIndex, wLength, NULL);
	k_sem_take(&p_msc_handle->msc_xfr_sem, K_FOREVER);
	if (p_msc_handle->msc_xfer_state == XFER_DONE) {
		LOG_DBG("rx LUN data = %d", data);
		ret_val = BOT_SUCCESS;

		/* initialize respective drive data received from device */
		if ((data == 0) || (data == 1)) {
			p_msc_handle->drv.max_lun = 1;
		} else {
			p_msc_handle->drv.max_lun = data;
		}
	}

	return ret_val;
}

/* Inquire the drive */
static enum bot_error_state req_msc_scsi_inquiry(const struct device *dev, const uint8_t addr,
						   const uint8_t lun)
{
	enum bot_error_state ret_val;
	uint8_t data[DATA_LEN_INQUIRY];

	/* prepare CBW */
	memset((void *)&p_msc_handle->cbw, 0, sizeof(struct CBW));
	p_msc_handle->cbw.DataLength = DATA_LEN_INQUIRY;
	p_msc_handle->cbw.Flags = USB_EP_DIR_IN;
	p_msc_handle->cbw.CBLength = CBW_LENGTH;
	p_msc_handle->cbw.LUN = lun;
	p_msc_handle->cbw.Signature = CBW_Signature;
	p_msc_handle->cbw.Tag = CBW_TAG;
	p_msc_handle->cbw.CB[0] = INQUIRY;
	p_msc_handle->cbw.CB[1] = (lun << 5);
	p_msc_handle->cbw.CB[2] = 0U;
	p_msc_handle->cbw.CB[3] = 0U;
	p_msc_handle->cbw.CB[4] = 0x24U;
	p_msc_handle->cbw.CB[5] = 0U;

	/* initialize buffer pointer */
	p_msc_handle->buf = &data[0];

	/* clear CSW */
	memset((void *)&p_msc_handle->csw, 0, sizeof(struct CSW));

	/* transfer BOT command */
	ret_val = xfer_msc_bot_cmd(dev, addr);
	if (ret_val == BOT_SUCCESS) {
		LOG_HEXDUMP_DBG(&data[0], DATA_LEN_INQUIRY, "inquiry rx data");
		LOG_HEXDUMP_DBG((uint8_t *)&p_msc_handle->csw, CSW_TOTAL_LENGTH, "inquiry rx CSW");

		/* initialize respective drive data received from device */
		p_msc_handle->drv.peripheral_qualifier = data[0] & 0x1F;
		p_msc_handle->drv.peripheral_dev_type = data[0] >> 5;
		p_msc_handle->drv.spc_version = data[2];
		memcpy(p_msc_handle->drv.t10_vendor_id, &data[8], 8);
		memcpy(p_msc_handle->drv.product_id, &data[16], 16);
		memcpy(p_msc_handle->drv.product_revision, &data[32], 4);
	}

	return ret_val;
}

/* Request read capacity (10) information from the drive */
static enum bot_error_state req_msc_scsi_read_capacity(const struct device *dev,
							 const uint8_t addr, uint8_t lun)
{
	uint8_t data[DATA_LEN_READ_CAPACITY10];
	enum bot_error_state ret_val;

	/* prepare CBW */
	memset((void *)&p_msc_handle->cbw, 0, sizeof(struct CBW));
	p_msc_handle->cbw.DataLength = DATA_LEN_READ_CAPACITY10;
	p_msc_handle->cbw.Flags = USB_EP_DIR_IN;
	p_msc_handle->cbw.CBLength = CBW_LENGTH;
	p_msc_handle->cbw.LUN = lun;
	p_msc_handle->cbw.Signature = CBW_Signature;
	p_msc_handle->cbw.Tag = CBW_TAG;
	p_msc_handle->cbw.CB[0] = READ_CAPACITY;

	/* initialize buffer pointer */
	p_msc_handle->buf = &data[0];

	/* clear CSW */
	memset((void *)&p_msc_handle->csw, 0, sizeof(struct CSW));

	/* transfer BOT command */
	ret_val = xfer_msc_bot_cmd(dev, addr);
	if (ret_val == BOT_SUCCESS) {
		LOG_HEXDUMP_DBG(&data[0], DATA_LEN_READ_CAPACITY10, "read capacity rx data");
		LOG_HEXDUMP_DBG((uint8_t *)&p_msc_handle->csw, CSW_TOTAL_LENGTH,
				"read capacity rx CSW");

		/* initialize respective drive data received from device */
		p_msc_handle->drv.total_blocks =
			(data[3] | ((uint32_t)data[2] << 8U) | ((uint32_t)data[1] << 16U) |
			 ((uint32_t)data[0] << 24U));
		p_msc_handle->drv.block_size = (uint16_t)(data[7] | ((uint32_t)data[6] << 8U));
	}

	return ret_val;
}

/* Request test unit ready information from the drive */
static enum bot_error_state req_msc_scsi_test_unit_ready(const struct device *dev,
							   const uint8_t addr, const uint8_t lun)
{
	enum bot_error_state ret_val;

	/* prepare CBW */
	memset((void *)&p_msc_handle->cbw, 0, sizeof(struct CBW));
	p_msc_handle->cbw.DataLength = DATA_LEN_MODE_TEST_UNIT_READY;
	p_msc_handle->cbw.Flags = USB_EP_DIR_OUT;
	p_msc_handle->cbw.CBLength = CBW_LENGTH;
	p_msc_handle->cbw.LUN = lun;
	p_msc_handle->cbw.Signature = CBW_Signature;
	p_msc_handle->cbw.Tag = CBW_TAG;
	p_msc_handle->cbw.CB[0] = TEST_UNIT_READY;

	/* clear CSW */
	memset((void *)&p_msc_handle->csw, 0, sizeof(struct CSW));

	/* transfer BOT command */
	ret_val = xfer_msc_bot_cmd(dev, addr);
	if (ret_val == BOT_SUCCESS) {
		LOG_HEXDUMP_DBG((uint8_t *)&p_msc_handle->csw, CSW_TOTAL_LENGTH,
				"test unit ready rx CSW");
	}

	return ret_val;
}

/* Request sense information from the drive */
static enum bot_error_state req_msc_scsi_request_sense(const struct device *dev,
							 const uint8_t addr, const uint8_t lun)
{
	enum bot_error_state ret_val;
	uint8_t data[DATA_LEN_REQUEST_SENSE];

	/* prepare CBW */
	memset((void *)&p_msc_handle->cbw, 0, sizeof(struct CBW));
	p_msc_handle->cbw.DataLength = DATA_LEN_REQUEST_SENSE;
	p_msc_handle->cbw.Flags = USB_EP_DIR_IN;
	p_msc_handle->cbw.CBLength = CBW_LENGTH;
	p_msc_handle->cbw.LUN = lun;
	p_msc_handle->cbw.Signature = CBW_Signature;
	p_msc_handle->cbw.Tag = CBW_TAG;
	p_msc_handle->cbw.CB[0] = REQUEST_SENSE;
	p_msc_handle->cbw.CB[1] = (lun << 5);
	p_msc_handle->cbw.CB[2] = 0U;
	p_msc_handle->cbw.CB[3] = 0U;
	p_msc_handle->cbw.CB[4] = DATA_LEN_REQUEST_SENSE;
	p_msc_handle->cbw.CB[5] = 0U;

	/* initialize buffer pointer */
	p_msc_handle->buf = &data[0];

	/* clear CSW */
	memset((void *)&p_msc_handle->csw, 0, sizeof(struct CSW));

	/* transfer BOT command */
	p_msc_handle->drv.sense_valid = false;
	ret_val = xfer_msc_bot_cmd(dev, addr);
	if (ret_val == BOT_SUCCESS) {
		LOG_HEXDUMP_DBG(&data[0], DATA_LEN_REQUEST_SENSE, "req sense rx data");
		LOG_HEXDUMP_DBG((uint8_t *)&p_msc_handle->csw, CSW_TOTAL_LENGTH,
				"req sense rx CSW");

		/* initialize respective drive data received from device */
		p_msc_handle->drv.sense_valid = true;
		p_msc_handle->drv.sense_key = data[2] & 0x0FU;
		p_msc_handle->drv.sense_asc = data[12];
		p_msc_handle->drv.sense_ascq = data[13];
		LOG_INF("sense_key = 0x%X, ", p_msc_handle->drv.sense_key);
		LOG_INF("sense_asc = 0x%X, ", p_msc_handle->drv.sense_asc);
		LOG_INF("sense_ascq = 0x%X", p_msc_handle->drv.sense_ascq);
	}

	return ret_val;
}

/* Request write (10) information to the drive */
static enum bot_error_state req_msc_scsi_write(const struct device *dev, const uint8_t addr,
						 const uint8_t lun, const uint8_t *pbuf,
						 uint32_t storage_address, uint32_t length)
{
	enum bot_error_state ret_val;

	/* prepare CBW */
	memset((void *)&p_msc_handle->cbw, 0, sizeof(struct CBW));
	p_msc_handle->cbw.DataLength = length * p_msc_handle->drv.block_size;
	p_msc_handle->cbw.Flags = USB_EP_DIR_OUT;
	p_msc_handle->cbw.CBLength = CBW_LENGTH;
	p_msc_handle->cbw.LUN = lun;
	p_msc_handle->cbw.Signature = CBW_Signature;
	p_msc_handle->cbw.Tag = CBW_TAG;
	p_msc_handle->cbw.CB[0] = WRITE10;
	p_msc_handle->cbw.CB[2] = (((uint8_t *)(void *)&storage_address)[3]);
	p_msc_handle->cbw.CB[3] = (((uint8_t *)(void *)&storage_address)[2]);
	p_msc_handle->cbw.CB[4] = (((uint8_t *)(void *)&storage_address)[1]);
	p_msc_handle->cbw.CB[5] = (((uint8_t *)(void *)&storage_address)[0]);
	p_msc_handle->cbw.CB[7] = (((uint8_t *)(void *)&length)[1]);
	p_msc_handle->cbw.CB[8] = (((uint8_t *)(void *)&length)[0]);

	/* initialize buffer pointer */
	p_msc_handle->buf = (uint8_t *)pbuf;

	/* clear CSW */
	memset((void *)&p_msc_handle->csw, 0, sizeof(struct CSW));

	/* transfer BOT command */
	ret_val = xfer_msc_bot_cmd(dev, addr);

	return ret_val;
}

/* Request read (10) information from the drive */
static enum bot_error_state req_msc_scsi_read(const struct device *dev, const uint8_t addr,
						const uint8_t lun, uint8_t *pbuf,
						uint32_t storage_address, uint32_t length)
{
	enum bot_error_state ret_val;

	/* prepare CBW */
	memset((void *)&p_msc_handle->cbw, 0, sizeof(struct CBW));
	p_msc_handle->cbw.DataLength = length * p_msc_handle->drv.block_size;
	p_msc_handle->cbw.Flags = USB_EP_DIR_IN;
	p_msc_handle->cbw.CBLength = CBW_LENGTH;
	p_msc_handle->cbw.LUN = lun;
	p_msc_handle->cbw.Signature = CBW_Signature;
	p_msc_handle->cbw.Tag = CBW_TAG;
	p_msc_handle->cbw.CB[0] = READ10;
	p_msc_handle->cbw.CB[2] = storage_address >> 24;
	p_msc_handle->cbw.CB[3] = storage_address >> 16;
	p_msc_handle->cbw.CB[4] = storage_address >> 8;
	p_msc_handle->cbw.CB[5] = storage_address & 0xFF;
	p_msc_handle->cbw.CB[7] = length >> 8;
	p_msc_handle->cbw.CB[8] = length & 0xFF;

	/* initialize buffer pointer */
	p_msc_handle->buf = (uint8_t *)pbuf;

	/* clear CSW */
	memset((void *)&p_msc_handle->csw, 0, sizeof(struct CSW));

	/* transfer BOT command */
	ret_val = xfer_msc_bot_cmd(dev, addr);

	return ret_val;
}

/* Run all SCSI commands required during initialization */
static void run_all_init_msc_scsi_commands(const struct device *dev, const uint8_t addr,
					   uint8_t lun)
{
	enum bot_error_state ret_val;
	enum bot_error_state exit_val = BOT_FAIL;
	bool scsi_complete = false;
	enum scsi_state scsi_state = GET_MAX_LUN;
	uint16_t scsi_retry = 0;
	struct uhc_data *data = dev->data;

	while (!scsi_complete && (scsi_retry <= CONFIG_MSC_SCSI_MAX_RETRY) &&
		atomic_test_bit(&data->status, UHC_STATUS_DEV_CONN)) {
		switch (scsi_state) {
		case GET_MAX_LUN:
			ret_val = req_msc_scsi_get_max_lun(dev, addr);
			if (ret_val == BOT_SUCCESS) {
				scsi_state = SCSI_INQUIRY;
				scsi_retry = 0;
			} else {
				scsi_retry++;
			}
			break;
		case SCSI_INQUIRY:
			ret_val = req_msc_scsi_inquiry(dev, addr, lun);
			if (ret_val == BOT_SUCCESS) {
				scsi_state = SCSI_TEST_UNIT_READY;
				scsi_retry = 0;
			} else {
				scsi_retry++;
			}
			break;
		case SCSI_TEST_UNIT_READY:
			ret_val = req_msc_scsi_test_unit_ready(dev, addr, lun);
			if (ret_val == BOT_SUCCESS) {
				scsi_state = SCSI_READ_CAPACITY;
				scsi_retry = 0;
			} else if (ret_val == BOT_REQ_SENSE) {
				scsi_state = SCSI_REQ_SENSE;
				scsi_retry = 0;
			} else {
				scsi_retry++;
			}
			break;
		case SCSI_READ_CAPACITY:
			ret_val = req_msc_scsi_read_capacity(dev, addr, lun);
			if (ret_val == BOT_SUCCESS) {
				scsi_state = SCSI_COMPLETE;
				scsi_retry = 0;
			} else {
				scsi_retry++;
			}
			break;
		case SCSI_REQ_SENSE:
			ret_val = req_msc_scsi_request_sense(dev, addr, lun);
			if (ret_val == BOT_SUCCESS) {
				scsi_state = SCSI_READ_CAPACITY;
				scsi_retry = 0;
			} else {
				scsi_retry++;
			}
			break;
		case SCSI_COMPLETE:
			scsi_complete = true;
			exit_val = BOT_SUCCESS;
			break;
		default:
			exit_val = BOT_FAIL;
			scsi_complete = true;
			break;
		}
	}

	if (atomic_test_bit(&data->status, UHC_STATUS_DEV_CONN)) {
		if (exit_val == BOT_SUCCESS) {
			atomic_set_bit(&p_msc_handle->drv.state, DRIVE_READY);
			LOG_INF("SCSI commands passed during initialization");
		} else {
			LOG_ERR("SCSI commands failed during initialization");
		}
	}
}

/* Initialize Mass Storage Class data structures */
static int msc_init(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	int ret_val;

	p_msc_handle->dev = dev;
	p_msc_handle->dev_addr = 1; /* dev_addr */
	p_msc_handle->lun = DEFAULT_LUN;

	atomic_clear_bit(&p_msc_handle->drv.state, DRIVE_READY);

	ret_val = populate_device_metadata(data);
	if (ret_val) {
		LOG_ERR("mass storage class meta data initialization failed");
		return ret_val;
	}

	k_sem_init(&p_msc_handle->msc_xfr_sem, 0, 1);
	p_msc_handle->msc_xfer_state = XFER_IDLE;
	p_msc_handle->buf = NULL;

	memset((void *)&p_msc_handle->cbw, 0, sizeof(struct CBW));
	memset((void *)&p_msc_handle->csw, 0, sizeof(struct CSW));
	memset((void *)&p_msc_handle->drv, 0, sizeof(struct drive));

	/* open bulk endpoint pipes */
	ret_val = uhc_pipe_open(dev, BULK_IN_PIPE_NUM, p_msc_handle->ep.bulk_in_num,
				USB_EP_TYPE_BULK, p_msc_handle->ep.bulk_in_mps);
	if (!ret_val) {
		ret_val = uhc_pipe_open(dev, BULK_OUT_PIPE_NUM, p_msc_handle->ep.bulk_out_num,
				USB_EP_TYPE_BULK, p_msc_handle->ep.bulk_out_mps);
	}

	if (!ret_val) {
		run_all_init_msc_scsi_commands(p_msc_handle->dev, p_msc_handle->dev_addr,
					       p_msc_handle->lun);
	} else {
		LOG_ERR("mass storage class pipe opening failed");
	}

	return ret_val;
}

/* Connected function is called on device attachment */
int connected(struct usbh_contex *const uhs_ctx)
{
	const struct device *dev = uhs_ctx->dev;

	return msc_init(dev);
}

static void msc_copy_data(uint8_t *data, uint16_t len)
{
	if (p_msc_handle->buf) {
		memcpy(p_msc_handle->buf, data, len);
	}
}

/* Request is callled once the transfer is completed */
int request(struct usbh_contex *const uhs_ctx, struct uhc_transfer *const xfer, int err)
{
	uint8_t stage = 0;
	const struct device *dev = uhs_ctx->dev;

	while (!k_fifo_is_empty(&xfer->done)) {
		struct net_buf *buf;

		stage++;
		buf = net_buf_get(&xfer->done, K_NO_WAIT);
		if (buf) {
			if (!err) {
				if (USB_EP_DIR_IS_IN(xfer->ep) &&
				    (xfer->attrib || stage == CTRL_DATA_STAGE)) {
					msc_copy_data(buf->data, buf->size);
				}
			}
			uhc_xfer_buf_free(dev, buf);
		}
	}

	/* inform msc state machine */
	if (p_msc_handle->msc_xfer_state == XFER_STARTED) {
		if (err) {
			p_msc_handle->msc_xfer_state = XFER_ERROR;
		} else {
			p_msc_handle->msc_xfer_state = XFER_DONE;
		}
		k_sem_give(&p_msc_handle->msc_xfr_sem);
	}

	return uhc_xfer_free(dev, xfer);
}

/* Deinitialize Mass Storage Class data structure */
static int msc_deinit(const struct device *dev)
{
	memset((void *)p_msc_handle, 0, sizeof(struct msc_handle));
	atomic_clear_bit(&p_msc_handle->drv.state, DRIVE_READY);

	return 0;
}

/* Removed is called on device removal */
int removed(struct usbh_contex *const uhs_ctx)
{
	const struct device *dev = uhs_ctx->dev;

	msc_deinit(dev);

	return 0;
}

/* Check the status of drive */
static inline bool is_drive_ready(void)
{
	return atomic_test_bit(&p_msc_handle->drv.state, DRIVE_READY);
}

/* Issue disk access read command to the drive */
int usbh_disk_access_read(const uint8_t lun, uint8_t *data_buf, uint32_t start_sector,
			  uint32_t num_sector)
{
	enum bot_error_state err_val;
	int ret_val = -EIO;
	uint16_t scsi_retry = 0;
	bool scsi_complete = false;
	uint8_t *org_data_buf = data_buf;

	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (!is_drive_ready()) {
		LOG_ERR("Drive is not ready");
		k_mutex_unlock(&drv_access_mutex);
		return -EIO;
	}

	if (!data_buf) {
		LOG_ERR("Invalid data buffer");
		k_mutex_unlock(&drv_access_mutex);
		return -EINVAL;
	}

	while (!scsi_complete && (scsi_retry <= CONFIG_MSC_SCSI_MAX_RETRY)) {
		err_val = req_msc_scsi_read(p_msc_handle->dev, p_msc_handle->dev_addr, lun,
					    data_buf, start_sector, num_sector);
		if (err_val == BOT_SUCCESS) {
			ret_val = 0;
			scsi_complete = true;
		} else if (err_val == BOT_REQ_SENSE) {
			req_msc_scsi_request_sense(p_msc_handle->dev, p_msc_handle->dev_addr, lun);
			scsi_complete = true;
			ret_val = -EIO;
		} else if (err_val == BOT_DEV_REMOVED) {
			ret_val = -EIO;
			scsi_complete = true;
		} else {
			scsi_retry++;
			/* rewind buffer in case of partial data transfer success */
			data_buf = org_data_buf;
		}
	}

	k_mutex_unlock(&drv_access_mutex);

	return ret_val;
}

/* Issue disk access write command to the drive */
int usbh_disk_access_write(const uint8_t lun, const uint8_t *data_buf, uint32_t start_sector,
			   uint32_t num_sector)
{
	enum bot_error_state err_val;
	int ret_val = -EIO;
	uint16_t scsi_retry = 0;
	bool scsi_complete = false;
	const uint8_t *org_data_buf = data_buf;

	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (!is_drive_ready()) {
		LOG_ERR("Drive is not ready");
		k_mutex_unlock(&drv_access_mutex);
		return -EIO;
	}

	if (!data_buf) {
		LOG_ERR("Invalid data buffer");
		k_mutex_unlock(&drv_access_mutex);
		return -EINVAL;
	}

	while (!scsi_complete && (scsi_retry <= CONFIG_MSC_SCSI_MAX_RETRY)) {
		err_val = req_msc_scsi_write(p_msc_handle->dev, p_msc_handle->dev_addr, lun,
					     data_buf, start_sector, num_sector);
		if (err_val == BOT_SUCCESS) {
			ret_val = 0;
			scsi_complete = true;
		} else if (err_val == BOT_REQ_SENSE) {
			req_msc_scsi_request_sense(p_msc_handle->dev, p_msc_handle->dev_addr, lun);
			scsi_complete = true;
			ret_val = -EIO;
		} else if (err_val == BOT_DEV_REMOVED) {
			ret_val = -EIO;
			scsi_complete = true;
		} else {
			scsi_retry++;
			/* rewind buffer in case of partial data transfer success */
			data_buf = org_data_buf;
		}
	}

	k_mutex_unlock(&drv_access_mutex);

	return ret_val;
}

/* DISKIO function to check the status of drive */
int usbh_disk_access_status(uint8_t pdrv)
{
	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (is_drive_ready()) {
		k_mutex_unlock(&drv_access_mutex);
		return 0;
	}

	k_mutex_unlock(&drv_access_mutex);
	return -EIO;
}

/* DISKIO function to initialize drive */
int usbh_disk_access_init(uint8_t pdrv)
{
	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (!is_drive_ready()) {
		k_mutex_unlock(&drv_access_mutex);
		return -EIO;
	}
	k_mutex_unlock(&drv_access_mutex);

	return 0;
}

/* DISKIO function to perform ioctl operations */
int usbh_disk_access_ioctl(uint8_t pdrv, uint8_t cmd, void *buf)
{
	int ret = 0;

	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (!is_drive_ready()) {
		LOG_ERR("Drive is not ready");
		k_mutex_unlock(&drv_access_mutex);
		return -EIO;
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;

	case DISK_IOCTL_GET_SECTOR_COUNT:
		/* retrieves number of available sectors */
		*(uint32_t *)buf = p_msc_handle->drv.total_blocks;
		break;

	case DISK_IOCTL_GET_SECTOR_SIZE:
		/* FatFS's GET_SECTOR_SIZE is supposed to return a 16-bit number */
		*(uint16_t *)buf = (uint16_t)p_msc_handle->drv.block_size;
		break;

	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		/* retrieves erase block size in unit of sector */
		*(uint32_t *)buf = p_msc_handle->drv.block_size / DEFAULT_BLOCK_SIZE;
		break;

	default:
		ret = -EIO;
		break;
	}

	k_mutex_unlock(&drv_access_mutex);

	return ret;
}

int usbh_msc_scsi_get_max_lun(uint8_t *max_lun)
{
	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (!is_drive_ready()) {
		LOG_ERR("Drive is not ready");
		k_mutex_unlock(&drv_access_mutex);
		return -EIO;
	}

	*max_lun = p_msc_handle->drv.max_lun;
	k_mutex_unlock(&drv_access_mutex);

	return 0;
}

int usbh_msc_scsi_get_inquiry(uint8_t *t10_vendor_id, uint8_t *product_id,
			      uint8_t *product_revision)
{
	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (!is_drive_ready()) {
		LOG_ERR("Drive is not ready");
		k_mutex_unlock(&drv_access_mutex);
		return -EIO;
	}

	memcpy(t10_vendor_id, p_msc_handle->drv.t10_vendor_id, 8);
	memcpy(product_id, p_msc_handle->drv.product_id, 16);
	memcpy(product_revision, p_msc_handle->drv.product_revision, 4);
	k_mutex_unlock(&drv_access_mutex);

	return 0;
}

int usbh_msc_scsi_get_read_capacity(uint32_t *total_blocks, uint32_t *block_size)
{
	k_mutex_lock(&drv_access_mutex, K_FOREVER);
	if (!is_drive_ready()) {
		LOG_ERR("Drive is not ready");
		k_mutex_unlock(&drv_access_mutex);
		return -EIO;
	}

	*total_blocks = p_msc_handle->drv.total_blocks;
	*block_size = p_msc_handle->drv.block_size;
	k_mutex_unlock(&drv_access_mutex);

	return 0;
}
