/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_printer.h>
#include <usb_descriptor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_printer, CONFIG_USB_DEVICE_LOG_LEVEL);

/* Maximum packet size for endpoints */
#define USB_PRINTER_BULK_EP_MPS    64

/* Number of endpoints used by printer interface */
#define USB_PRINTER_NUM_EP         2

/* Common PCL printer commands */
#define PCL_RESET                "\x1B" "E"
#define PCL_ORIENTATION_PORT     "\x1B&l0O"
#define PCL_ORIENTATION_LAND     "\x1B&l1O"
#define PCL_FONT_COURIER        "\x1B(s0p12h0s0b4099T"

/* Printer interface configuration */
struct printer_cfg_data {
    struct usb_if_descriptor if0;
    struct usb_ep_descriptor if0_in_ep;
    struct usb_ep_descriptor if0_out_ep;
} __packed;

/* USB Printer class instance */
struct printer_dev_data {
    const struct usb_printer_config *config;
    struct usb_dev_data common;
    uint8_t port_status;
    uint8_t in_ep;
    uint8_t out_ep;
};

/* Printer class descriptor structure */
USBD_CLASS_DESCR_DEFINE(primary, 0) struct printer_cfg_data printer_desc = {
    /* Interface descriptor */
    .if0 = {
        .bLength = sizeof(struct usb_if_descriptor),
        .bDescriptorType = USB_DESC_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = USB_PRINTER_NUM_EP,
        .bInterfaceClass = USB_PRINTER_CLASS,
        .bInterfaceSubClass = USB_PRINTER_SUBCLASS,
        .bInterfaceProtocol = USB_PRINTER_PROTOCOL_BI,
        .iInterface = 0,
    },

    /* Data IN Endpoint */
    .if0_in_ep = {
        .bLength = sizeof(struct usb_ep_descriptor),
        .bDescriptorType = USB_DESC_ENDPOINT,
        .bEndpointAddress = AUTO_EP_IN,
        .bmAttributes = USB_DC_EP_BULK,
        .wMaxPacketSize = sys_cpu_to_le16(USB_PRINTER_BULK_EP_MPS),
        .bInterval = 0,
    },

    /* Data OUT Endpoint */
    .if0_out_ep = {
        .bLength = sizeof(struct usb_ep_descriptor),
        .bDescriptorType = USB_DESC_ENDPOINT,
        .bEndpointAddress = AUTO_EP_OUT,
        .bmAttributes = USB_DC_EP_BULK,
        .wMaxPacketSize = sys_cpu_to_le16(USB_PRINTER_BULK_EP_MPS),
        .bInterval = 0,
    },
};

static struct printer_dev_data printer_data;

/* Send data to host */
int usb_printer_send_data(const uint8_t *data, size_t len)
{
    uint32_t written;
    int ret;

    if (!data || !len) {
        return -EINVAL;
    }

    ret = usb_write(printer_data.in_ep, data, len, &written);
    if (ret) {
        return ret;
    }

    return written;
}

/* Update printer status */
int usb_printer_update_status(uint8_t status)
{
    printer_data.port_status = status;
    return 0;
}

/* Process PCL commands in the received data */
static void process_pcl_command(const uint8_t *data, size_t len)
{
    if (len < 2 || data[0] != 0x1B) {
        return; /* Not a PCL escape sequence */
    }

    switch (data[1]) {
    case 'E':  /* Printer reset */
        LOG_INF("PCL: Printer reset");
        printer_data.port_status &= ~USB_PRINTER_STATUS_ERROR;
        break;
        
    case '&':  /* PCL control sequence */
        if (len < 4) {
            return;
        }
        if (data[2] == 'l') {  /* Page control */
            switch (data[3]) {
            case '0':
                if (data[4] == 'O') {
                    LOG_INF("PCL: Portrait orientation");
                }
                break;
            case '1':
                if (data[4] == 'O') {
                    LOG_INF("PCL: Landscape orientation");
                }
                break;
            }
        }
        break;
    }
}

/* Callback for OUT endpoint events */
static void printer_out_cb(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
    uint8_t buffer[USB_PRINTER_BULK_EP_MPS];
    uint32_t bytes_read;
    int ret;

    if (ep_status != USB_DC_EP_DATA_OUT) {
        return;
    }

    /* Read received data */
    ret = usb_read(printer_data.out_ep, buffer, sizeof(buffer), &bytes_read);
    if (ret) {
        LOG_ERR("Failed to read data: %d", ret);
        return;
    }

    /* Check for PCL commands */
    process_pcl_command(buffer, bytes_read);

    if (printer_data.config && printer_data.config->data_received) {
        printer_data.config->data_received(buffer, bytes_read);
    }

    /* Re-enable OUT endpoint for next transfer */
    usb_dc_ep_read_continue(printer_data.out_ep);
}

static void printer_status_cb(struct usb_cfg_data *cfg,
                            enum usb_dc_status_code status,
                            const uint8_t *param)
{
    ARG_UNUSED(param);
    ARG_UNUSED(cfg);

    switch (status) {
    case USB_DC_RESET:
        printer_data.port_status = USB_PRINTER_STATUS_SELECTED;
        break;

    case USB_DC_CONFIGURED:
        printer_data.in_ep = printer_desc.if0_in_ep.bEndpointAddress;
        printer_data.out_ep = printer_desc.if0_out_ep.bEndpointAddress;
        usb_dc_ep_callback_set(printer_data.out_ep, printer_out_cb);
        usb_dc_ep_read_continue(printer_data.out_ep);
        
        LOG_DBG("USB device configured");
        if (printer_data.config && printer_data.config->status_cb) {
            printer_data.config->status_cb(true);
        }
        break;

    case USB_DC_DISCONNECTED:
        LOG_DBG("USB device disconnected");
        if (printer_data.config && printer_data.config->status_cb) {
            printer_data.config->status_cb(false);
        }
        break;

    default:
        break;
    }
}

static int printer_class_handle_req(struct usb_setup_packet *setup,
                                  int32_t *len, uint8_t **data)
{
    struct printer_dev_data *dev_data = &printer_data;

    if (REQTYPE_GET_RECIP(setup->bmRequestType) != REQTYPE_RECIP_INTERFACE ||
        setup->wIndex != 0) {
        return -ENOTSUP;
    }

    switch (setup->bRequest) {
    case USB_PRINTER_GET_DEVICE_ID:
        if (dev_data->config && dev_data->config->device_id) {
            uint16_t id_len = strlen(dev_data->config->device_id);
            uint8_t *device_id = (uint8_t *)dev_data->config->device_id;

            *len = id_len + 2;
            device_id[0] = (id_len >> 8) & 0xFF;
            device_id[1] = id_len & 0xFF;
            *data = device_id;
            return 0;
        }
        break;

    case USB_PRINTER_GET_PORT_STATUS:
        *data = &dev_data->port_status;
        *len = 1;
        return 0;

    case USB_PRINTER_SOFT_RESET:
        if (setup->wValue == 0 && setup->wLength == 0) {
            LOG_DBG("Soft reset");
            return 0;
        }
        break;

    default:
        if (dev_data->config && dev_data->config->class_handler) {
            return dev_data->config->class_handler(setup, len, data);
        }
    }

    return -ENOTSUP;
}

/* USB Printer Device configuration */
static struct usb_cfg_data printer_config = {
    .usb_device_description = NULL,
    .interface_config = printer_status_cb,
    .interface_descriptor = &printer_desc.if0,
    .cb_usb_status = printer_status_cb,
    .interface = {
        .class_handler = printer_class_handle_req,
        .custom_handler = NULL,
        .vendor_handler = NULL,
    },
    .num_endpoints = USB_PRINTER_NUM_EP,
};

int usb_printer_init(const struct usb_printer_config *config)
{
    int ret;

    if (!config) {
        return -EINVAL;
    }

    printer_data.config = config;
    printer_data.port_status = USB_PRINTER_STATUS_SELECTED;

    /* Initialize the USB device stack */
    ret = usb_enable(&printer_config);
    if (ret < 0) {
        LOG_ERR("Failed to enable USB");
        return ret;
    }

    return 0;
}