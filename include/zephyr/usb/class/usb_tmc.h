#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_TMC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_TMC_H_

#include <stdint.h>
#include <zephyr/usb/usbd.h>

#define USB_TMC_CLASS           0xFE
#define USB_TMC_SUBCLASS        0x03
#define USB_TMC_PROTOCOL_BASE   0x00
#define USB_TMC_PROTOCOL_USB488 0x01 // USB488 = 0x01, base TMC = 0x00

/* USB TMC Header Message ID Values */
#define USBTMC_DEV_DEP_MSG_OUT            0x01U
#define USBTMC_REQUEST_DEV_DEP_MSG_IN     0x02U
#define USBTMC_DEV_DEP_MSG_IN             0x02U /* response uses same ID */
#define USBTMC_VENDOR_SPECIFIC_OUT        0x7EU
#define USBTMC_REQUEST_VENDOR_SPECIFIC_IN 0x7FU

/* USB TMC bRequest values (4.2.1)*/
#define USBTMC_REQ_INITIATE_ABORT_BULK_OUT    0x01U
#define USBTMC_REQ_CHECK_ABORT_BULK_OUT       0x02U
#define USBTMC_REQ_INITIATE_ABORT_BULK_IN     0x03U
#define USBTMC_REQ_CHECK_ABORT_BULK_IN_STATUS 0x04U
#define USBTMC_REQ_INITIATE_CLEAR             0x05U
#define USBTMC_REQ_CHECK_CLEAR_STATUS         0x6U
#define USBTMC_REQ_GET_CAPABILITIES           0x7U
#define USBTMC_REQ_INDICATOR_PULSE            0x40

/* USB TMC Status Values*/
#define USBTMC_STATUS_SUCCESS                  0x01U
#define USBTMC_STATUS_PENDING                  0x02U
#define USBTMC_STATUS_FAILED                   0x80U
#define USBTMC_STATUS_TRANSFER_NOT_IN_PROGRESS 0x81U
#define USBTMC_STATUS_SPLIT_NOT_IN_PROGRESS    0x82U
#define USBTMC_STATUS_SPLIT_IN_PROGRESS        0x83U

/* USB TMC Class Message header */
struct usb_tmc_msg_header {
	uint8_t MsgID;
	uint8_t bTag;
	uint8_t bTagInverse;
	uint8_t _reserved;
	uint32_t TransferSize;
	uint8_t bmTransferAttributes;
	uint8_t _reserved2[3];
	uint8_t data[];
} __packed;

/* USB TMC Descriptor*/
struct usb_tmc_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_ep_out;
	struct usb_ep_descriptor if0_ep_in;
	struct usb_desc_header nil_desc;
};

struct usb_tmc_capabilities {
	uint8_t Status;
	uint16_t bcdUSBTMC;
	uint8_t InterfaceCapabilities;
	uint8_t DeviceCapabilities;
} __packed;

struct usbd_tmc_data {
	struct usb_tmc_desc *desc;
	const struct usb_desc_header **fs_desc;
};

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_TMC_H_ */