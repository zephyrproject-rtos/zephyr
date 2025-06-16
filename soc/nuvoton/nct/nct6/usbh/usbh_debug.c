
/**************************************************************************//**
 * @file     usbh_debug.c
 * @brief    MCU USB Host library
 *
 * @note
 * USB debug helper routines.
 *
 * I just want these out of the way where they aren't in your
 * face, but so that you can still use them..
 *
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "usbh_core.h"

/*
 * Parse and show the different USB descriptors.
 */
void  usbh_dump_device_descriptor(USB_DEV_DESC_T *desc)
{
    if (!desc) {
        USB_debug("Invalid USB device descriptor (NULL POINTER)\n");
        return;
    }
    USB_debug("  Length              = %2d%s\n", desc->bLength,
              desc->bLength == USB_DT_DEVICE_SIZE ? "" : " (!!!)");
    USB_debug("  DescriptorType      = %02x\n", desc->bDescriptorType);
    USB_debug("  USB version         = %x.%02x\n",
              desc->bcdUSB >> 8, desc->bcdUSB & 0xff);
    USB_debug("  Vendor:Product      = %04x:%04x\n",
              desc->idVendor, desc->idProduct);
    USB_debug("  MaxPacketSize0      = %d\n", desc->bMaxPacketSize0);
    USB_debug("  NumConfigurations   = %d\n", desc->bNumConfigurations);
    USB_debug("  Device version      = %x.%02x\n",
              desc->bcdDevice >> 8, desc->bcdDevice & 0xff);
    USB_debug("  Device Class:SubClass:Protocol = %02x:%02x:%02x\n",
              desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol);
    switch (desc->bDeviceClass) {
    case 0:
        USB_debug("    Per-interface classes\n");
        break;
    case USB_CLASS_AUDIO:
        USB_debug("    Audio device class\n");
        break;
    case USB_CLASS_COMM:
        USB_debug("    Communications class\n");
        break;
    case USB_CLASS_HID:
        USB_debug("    Human Interface Devices class\n");
        break;
    case USB_CLASS_PRINTER:
        USB_debug("    Printer device class\n");
        break;
    case USB_CLASS_MASS_STORAGE:
        USB_debug("    Mass Storage device class\n");
        break;
    case USB_CLASS_HUB:
        USB_debug("    Hub device class\n");
        break;
    case USB_CLASS_VENDOR_SPEC:
        USB_debug("    Vendor class\n");
        break;
    default:
        USB_debug("    Unknown class\n");
    }
}



void  usbh_dump_config_descriptor(USB_CONFIG_DESC_T *desc)
{
    USB_debug("Configuration:\n");
    USB_debug("  bLength             = %4d%s\n", desc->bLength,
              desc->bLength == USB_DT_CONFIG_SIZE ? "" : " (!!!)");
    USB_debug("  bDescriptorType     =   %02x\n", desc->bDescriptorType);
    USB_debug("  wTotalLength        = %04x\n", desc->wTotalLength);
    USB_debug("  bNumInterfaces      =   %02x\n", desc->bNumInterfaces);
    USB_debug("  bConfigurationValue =   %02x\n", desc->bConfigurationValue);
    USB_debug("  iConfiguration      =   %02x\n", desc->iConfiguration);
    USB_debug("  bmAttributes        =   %02x\n", desc->bmAttributes);
    USB_debug("  MaxPower            = %4dmA\n", desc->MaxPower * 2);
}



void  usbh_dump_iface_descriptor(USB_IF_DESC_T *desc)
{
    USB_debug("  Interface: %d\n", desc->bInterfaceNumber);
    USB_debug("  Alternate Setting: %2d\n", desc->bAlternateSetting);
    USB_debug("    bLength             = %4d%s\n", desc->bLength,
              desc->bLength == USB_DT_INTERFACE_SIZE ? "" : " (!!!)");
    USB_debug("    bDescriptorType     =   %02x\n", desc->bDescriptorType);
    USB_debug("    bInterfaceNumber    =   %02x\n", desc->bInterfaceNumber);
    USB_debug("    bAlternateSetting   =   %02x\n", desc->bAlternateSetting);
    USB_debug("    bNumEndpoints       =   %02x\n", desc->bNumEndpoints);
    USB_debug("    bInterface Class:SubClass:Protocol =   %02x:%02x:%02x\n",
              desc->bInterfaceClass, desc->bInterfaceSubClass, desc->bInterfaceProtocol);
    USB_debug("    iInterface          =   %02x\n", desc->iInterface);
}



void  usbh_dump_ep_descriptor(USB_EP_DESC_T *desc)
{
#ifdef USB_DEBUG
    char   *LengthCommentString = (desc->bLength ==
                                   USB_DT_ENDPOINT_AUDIO_SIZE) ? " (Audio)" : (desc->bLength ==
                                           USB_DT_ENDPOINT_SIZE) ? "" : " (!!!)";
    char   *EndpointType[4] = { "Control", "Isochronous", "Bulk", "Interrupt" };

    USB_debug("    Endpoint:\n");
    USB_debug("      bLength             = %4d%s\n",
              desc->bLength, LengthCommentString);
    USB_debug("      bDescriptorType     =   %02x\n", desc->bDescriptorType);
    USB_debug("      bEndpointAddress    =   %02x (%s)\n", desc->bEndpointAddress,
              (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
              USB_ENDPOINT_XFER_CONTROL ? "i/o" :
              (desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ? "in" : "out");
    USB_debug("      bmAttributes        =   %02x (%s)\n", desc->bmAttributes,
              EndpointType[USB_ENDPOINT_XFERTYPE_MASK & desc->bmAttributes]);
    USB_debug("      wMaxPacketSize      = %04x\n", desc->wMaxPacketSize);
    USB_debug("      bInterval           =   %02x\n", desc->bInterval);

    /* Audio extensions to the endpoint descriptor */
    if (desc->bLength == USB_DT_ENDPOINT_AUDIO_SIZE) {
        USB_debug("      bRefresh            =   %02x\n", desc->bRefresh);
        USB_debug("      bSynchAddress       =   %02x\n", desc->bSynchAddress);
    }
#endif
}



void  usbh_print_usb_string(USB_DEV_T *dev, char *id, int index)
{
    char   buf[256];

    if (!index)
        return;
    if (usbh_translate_string(dev, index, buf, 256) > 0)
        USB_debug("%s: %s\n", id, buf);
}



void  usbh_dump_urb(URB_T *purb)
{
    USB_debug ("urb                   :0x%x\n", (int)purb);
    USB_debug ("next                  :0x%x\n", (int)purb->next);
    USB_debug ("dev                   :0x%x\n", (int)purb->dev);
    USB_debug ("pipe                  :0x%08lx\n", purb->pipe);
    USB_debug ("status                :%d\n", purb->status);
    USB_debug ("transfer_flags        :0x%08lx\n", purb->transfer_flags);
    USB_debug ("transfer_buffer       :0x%x\n", (int)purb->transfer_buffer);
    USB_debug ("transfer_buffer_length:%d\n", purb->transfer_buffer_length);
    USB_debug ("actual_length         :%d\n", purb->actual_length);
    USB_debug ("setup_packet          :0x%x\n", (int)purb->setup_packet);
    USB_debug ("start_frame           :%d\n", purb->start_frame);
    USB_debug ("number_of_packets     :%d\n", purb->number_of_packets);
    USB_debug ("interval              :%d\n", purb->interval);
    USB_debug ("error_count           :%d\n", purb->error_count);
    USB_debug ("complete              :0x%x\n", (int)purb->complete);
}

