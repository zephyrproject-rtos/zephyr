
/**************************************************************************//**
 * @file     usbh_core.c
 * @brief    OHCI MCU USB Host Library core
 *
 * @note
 *
 * (C) Copyright Linus Torvalds 1999
 * (C) Copyright Johannes Erdfelt 1999
 * (C) Copyright Andreas Gal 1999
 * (C) Copyright Gregory P. Smith 1999
 * (C) Copyright Deti Fliegl 1999 (new USB architecture)
 * (C) Copyright Randy Dunlap 2000
 * (C) Copyright David Brownell 2000 (kernel hotplug, usb_device_id)
 * (C) Copyright Yggdrasil Computing, Inc. 2000
 *     (usb_device_id matching changes by Adam J. Richter)
 *
 * NOTE! This is not actually a driver at all, rather this is
 * just a collection of helper routines that implement the
 * generic USB things that the real drivers can use..
 *
 * Think of this as a "USB library" rather than anything else.
 * It should be considered a slave, with no callbacks. Callbacks
 * are evil.
 *
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "usbh_core.h"

#include "hal_ohci.h"

extern USB_DEV_T g_dev_pool[DEV_MAX_NUM];
extern uint8_t  dev_alloc_mark[DEV_MAX_NUM];

static volatile uint32_t  g_devmap;

/*
 * We have a per-interface "registered driver" list.
 */
LIST_HEAD(_UsbDriverList);
LIST_HEAD(_UsbBusList);


/**
  * @brief     Register a device driver to USB Host Core driver
  * @param[in] new_driver: The USB device driver.
  * @return    Success or not.
  * @retval    0  Success
  * @retval    otherwise  Failed
  */
int32_t USBH_RegisterDriver(USB_DRIVER_T *new_driver)
{
    /* Add it to the list of known drivers */
    list_add_tail(&new_driver->driver_list, &_UsbDriverList);
    return 0;
}

const USB_DEV_ID_T  *usb_match_id(USB_DEV_T *dev, USB_IF_DESC_T *intf,
                                  const USB_DEV_ID_T *id)
{
    /* proc_connectinfo in devio.c may call us with id == NULL. */
    if (id == NULL)
        return NULL;

    /*
     *  It is important to check that id->driver_info is nonzero,
     *  since an entry that is all zeroes except for a nonzero
     *  id->driver_info is the way to create an entry that indicates
     *  that the driver want to examine every device and interface.
     */
    while(1) {
        if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
                (id->idVendor != dev->descriptor.idVendor))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
                (id->idProduct != dev->descriptor.idProduct))
            goto NEXT_ID;

        /* No need to test id->bcdDevice_lo != 0, since 0 is never
           greater than any unsigned number. */
        if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
                (id->bcdDevice_lo > dev->descriptor.bcdDevice))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
                (id->bcdDevice_hi < dev->descriptor.bcdDevice))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
                (id->bDeviceClass != dev->descriptor.bDeviceClass))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
                (id->bDeviceSubClass!= dev->descriptor.bDeviceSubClass))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
                (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
                (id->bInterfaceClass != intf->bInterfaceClass))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
                (id->bInterfaceSubClass != intf->bInterfaceSubClass))
            goto NEXT_ID;

        if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
                (id->bInterfaceProtocol != intf->bInterfaceProtocol))
            goto NEXT_ID;
        return id;

NEXT_ID:
        if (id->driver_info) // have next match id
        {
            id ++;
        }
        else
        {
            return NULL;
        }
    }
    return NULL;
}


/*
 *  This entry point gets called for each new device.
 *
 *  We now walk the list of registered USB drivers,
 *  looking for one that will accept this interface.
 *
 *  "New Style" drivers use a table describing the devices and interfaces
 *  they handle.  Those tables are available to user mode tools deciding
 *  whether to load driver modules for a new device.
 *
 *  The probe return value is changed to be a private pointer.  This way
 *  the drivers don't have to dig around in our structures to set the
 *  private pointer if they only need one interface.
 *
 *  Returns: 0 if a driver accepted the interface, -1 otherwise
 */
static int  usb_find_interface_driver(USB_DEV_T *dev, USB_IF_DESC_T *intf)
{
    USB_LIST_T          *tmp;
    const USB_DEV_ID_T  *id;
    USB_DRIVER_T        *driver;
    int                 i, found;

    for (tmp = _UsbDriverList.next; tmp != &_UsbDriverList;) { /* search through the driver list */
        driver = list_entry(tmp, USB_DRIVER_T, driver_list);
        tmp = tmp->next;

        id = driver->id_table;
        id = usb_match_id(dev, intf, id);
        if (id) {
            if (driver->probe(dev, intf, id) == 0) {
                /*
                 *  Add driver to driver list of this device.
                 */
                for (found = 0, i = 0; i < dev->driver_cnt; i++)
                    if (dev->driver[i] == driver) found = 1;

                if (!found) {
                    if (dev->driver_cnt >= MAX_DRIVER_PER_DEV) {
                        USB_error("Driver overrun for one device!\n");
                        return USB_ERR_NOMEM;
                    }
                    dev->driver[dev->driver_cnt] = driver;
                    dev->driver_cnt++;
                    USB_debug("Deivce bind driver count %d\n", dev->driver_cnt);
                    return 0;
                }
            }
        }
    }
    if (dev->driver_cnt == 0)
        USB_warning("No matching driver!!\n");
    return -1;
}

/**
  * @brief    Submit an URB to USB core for transfer data.
  * @param[in] urb The USB transfer request to be processed.
  * @return    Success or not.
  * @retval    0  Success
  * @retval    otherwise  Failed
  */
int32_t  USBH_SubmitUrb(URB_T *urb)
{
    if (urb && urb->dev)
        return urb->dev->bus->op->submit_urb(urb);
    else {
        return USB_ERR_NODEV;
    }
}


/**
  * @brief    Cancel an URB which has been submit to USB core.
  * @param[in]  urb The USB to be canceled.
  * @return    Success or not.
  * @retval    0  Success
  * @retval    otherwise  Failed
  */
int32_t  USBH_UnlinkUrb(URB_T *urb)
{
    if (urb && urb->dev)
        return urb->dev->bus->op->unlink_urb(urb);
    else
        return USB_ERR_NODEV;
}


/*-------------------------------------------------------------------*
 *                     COMPLETION HANDLERS                           *
 *-------------------------------------------------------------------*/

static volatile int _Event_UrbCompleted;

/*-------------------------------------------------------------------*
 * completion handler for compatibility wrappers (sync control/bulk) *
 *-------------------------------------------------------------------*/
static void ctrl_msg_complete(URB_T *urb)
{
    _Event_UrbCompleted = 1;
}

static int get_ctrl_msg_signal()
{
    return _Event_UrbCompleted;
}
static void clr_ctrl_msg_signal()
{
    _Event_UrbCompleted = 0;
}



/*-------------------------------------------------------------------*
 *                         COMPATIBILITY STUFF                       *
 *-------------------------------------------------------------------*/
/*  Starts urb and waits for completion or timeout */
static int  usb_start_wait_urb(URB_T *urb, int timeout, int* actual_length)
{
    volatile int  t0;
    int   status;

    timeout /= 10000;
    if (timeout == 0) timeout = 100;

    clr_ctrl_msg_signal();

    status = USBH_SubmitUrb(urb);
    if (status)
        return status;

#if 1
    for (t0 = 0; t0 < 0x100000; t0++) {
        if (get_ctrl_msg_signal())
            break;
    }

    if (t0 >= 0x100000)
        urb->status = USB_ERR_INPROGRESS;
    else
        urb->status = 0;
#else
    t0 = sysGetTicks(TIMER0);
    while (sysGetTicks(TIMER0) - t0 < timeout) {
        if (get_ctrl_msg_signal())
            break;
    }

    if (sysGetTicks(TIMER0) - t0 >= timeout)
        urb->status = USB_ERR_INPROGRESS;
    else
        urb->status = 0;
#endif

    if (urb->status == USB_ERR_INPROGRESS) {
        /* timeout */
        USB_warning("usb_control/bulk_msg: timeout\n");
        USBH_UnlinkUrb(urb);     /* remove urb safely */
        status = USB_ERR_TIMEOUT;
    } else
        status = urb->status;

    if (actual_length)
        *actual_length = urb->actual_length;

    return status;
}


/*-------------------------------------------------------------------*/
// returns status (negative) or length (positive)
static int  usb_internal_control_msg(USB_DEV_T *usb_dev, uint32_t pipe,
                                     DEV_REQ_T *cmd,  void *data, int len, int timeout)
{
    URB_T   urb;
    int     retv;
    int     length;

    FILL_CONTROL_URB(&urb, usb_dev, pipe, (uint8_t*)cmd, data, len,  /* build urb */
                     ctrl_msg_complete, 0);

    retv = usb_start_wait_urb(&urb, timeout, &length);
    if (retv < 0)
        return retv;
    else
        return length;
}


/**
 *  @brief  Execute a control transfer.
 *  @param[in] dev    Pointer to the usb device to send the message to.
 *  @param[in] pipe   Endpoint pipe to send the message to.
 *  @param[in] request   USB message request value
 *  @param[in] requesttype  USB message request type value
 *  @param[in] value  USB message value
 *  @param[in] index  USB message index value
 *  @param[in,out] data   Memory buffer of the data to be send with this transfer or to hold the data received from this transfer.
 *  @param[in] size   Length in bytes of the data to send.
 *  @param[in] timeout Time in millisecond to wait for the message to complete before timing out (if 0 the wait is forever).
 *  @return   Data transfer length or error code.
 *  @retval   >=0  The actual bytes of data be transferred.
 *  @retval   Otherwise  Failed
 *
 *  This function sends a simple control message to a specified endpoint
 *  and waits for the message to complete, or timeout.
 *
 *  If successful, it returns 0, otherwise a negative error number.
 *
 *  Don't use this function from within an interrupt context, like a
 *  bottom half handler.  If you need a asynchronous message, or need to send
 *  a message from within interrupt context, use USBH_SubmitUrb()
 */
int32_t  USBH_SendCtrlMsg(USB_DEV_T *dev, uint32_t pipe, uint8_t request,
                      uint8_t requesttype,  uint16_t value, uint16_t index, void *data,
                      uint16_t size, int timeout)
{
    DEV_REQ_T   dr;

    dr.requesttype = requesttype;
    dr.request = request;
    dr.value = value;
    dr.index = index;
    dr.length = size;
    return usb_internal_control_msg(dev, pipe, &dr, (uint8_t *)data, size, timeout);
}


/**
  * @brief  Execute a bulk transfer.
 *  @param[in] usb_dev  Pointer to the usb device to send the message to.
 *  @param[in] pipe   Endpoint pipe to send the message to.
 *  @param[in,out] data   Pointer to the data to send or to receive.
 *  @param[in] len    Length in bytes of the data to send or receive.
 *  @param[out] actual_length Actual length of data transferred.
 *  @param[in]  timeout  Time in millisecond to wait for the message to complete before timing out (if 0 the wait is forever).
 *  @return   Data transfer length or error code.
 *  @retval   >=0  The actual bytes of data be transferred.
 *  @retval   Otherwise  Failed
 *
 *  This function sends a simple bulk message to a specified endpoint
 *  and waits for the message to complete, or timeout.
 *
 *  If successful, it returns 0, otherwise a negative error number.
 *  The number of actual bytes transferred will be placed in the
 *  actual_timeout parameter.
 *
 *  Don't use this function from within an interrupt context, like a
 *  bottom half handler.  If you need a asynchronous message, or need to
 *  send a message from within interrupt context, use USBH_SubmitUrb()
 */
int32_t  USBH_SendBulkMsg(USB_DEV_T *usb_dev, uint32_t pipe,
                      void *data, int len, int *actual_length, int timeout)
{
    URB_T  urb;

    FILL_BULK_URB(&urb,usb_dev,pipe,(uint8_t *)data,len,   /* build urb */
                  ctrl_msg_complete, 0);

    return usb_start_wait_urb(&urb, timeout, actual_length);
}

int usb_maxpacket(USB_DEV_T *dev, uint32_t pipe, int out)
{
    int         i;
    int         ep_addr;

    ep_addr =  (((pipe) >> 15) & 0xf) | (pipe & 0x80);

    if ((ep_addr == 0) || (ep_addr == 0x80))
        return dev->ep_list[0].wMaxPacketSize;

    for (i = 0; i < dev->ep_list_cnt; i++) {
        if ((dev->ep_list[i].cfgno == dev->act_config) &&
                (dev->ep_list[i].bEndpointAddress == ep_addr)) {
            return dev->ep_list[i].wMaxPacketSize;
        }
    }
    USB_error("usb_maxpacket - endpoint %x not found!!\n", ep_addr);
    return 64;
}


static int  usb_parse_endpoint(USB_DEV_T *dev, int cfg_value, USB_IF_DESC_T *ifp, uint8_t *buffer, int size)
{
    USB_EP_DESC_T   endpoint;
    USB_DESC_HDR_T  *header;
    EP_INFO_T       *ep_info;
    int             parsed = 0, numskipped;

    header = (USB_DESC_HDR_T *)buffer;
    memcpy((uint8_t *)&endpoint, buffer, sizeof(USB_EP_DESC_T));

    /* Everything should be fine being passed into here, but we sanity */
    if (header->bLength > size) {
        USB_error("Error! - ran out of descriptors parsing");
        return -1;
    }

    if (header->bDescriptorType != USB_DT_ENDPOINT) {
        USB_warning("Warning! - unexpected descriptor 0x%X, expecting endpoint descriptor, type 0x%X",
                    endpoint.bDescriptorType, USB_DT_ENDPOINT);
        return parsed;
    }

    if (dev->ep_list_cnt < MAX_ENDPOINTS) {
        ep_info = &dev->ep_list[dev->ep_list_cnt];
        ep_info->cfgno = cfg_value;
        ep_info->ifnum = ifp->bInterfaceNumber;
        ep_info->altno = ifp->bAlternateSetting;
        ep_info->bEndpointAddress = endpoint.bEndpointAddress;
        ep_info->bmAttributes = endpoint.bmAttributes;
        ep_info->bInterval = endpoint.bInterval;
        ep_info->wMaxPacketSize = endpoint.wMaxPacketSize;
        if ((ep_info->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
            if (ep_info->wMaxPacketSize > 64) {
                USB_debug("Endpoint %x wMaxPacketSize is %d bytes, force to change as 64 bytes!\n",
                          endpoint.bEndpointAddress, endpoint.wMaxPacketSize);
                ep_info->wMaxPacketSize = 64;
            }
        }
        dev->ep_list_cnt++;
        //USB_debug("Add endpoint 0x%x - %d %d %d, max_pkt = %d\n", ep_info->bEndpointAddress, ep_info->cfgno,
        //           ep_info->ifnum, ep_info->altno, ep_info->wMaxPacketSize);
    } else {
        USB_error("Too many endpoints!\n");
    }

    buffer += header->bLength;
    size -= header->bLength;
    parsed += header->bLength;

    /* Skip over the rest of the Class Specific or Vendor Specific descriptors */
    numskipped = 0;
    while (size >= sizeof(USB_DESC_HDR_T)) {
        header = (USB_DESC_HDR_T *)buffer;

        if (header->bLength < 2) {
            USB_error("Error! - invalid descriptor length of %d\n", header->bLength);
            return -1;
        }

        /* If we find another descriptor which is at or below us */
        /*  in the descriptor heirarchy then we're done  */
        if ((header->bDescriptorType == USB_DT_ENDPOINT) ||
                (header->bDescriptorType == USB_DT_INTERFACE) ||
                (header->bDescriptorType == USB_DT_CONFIG) ||
                (header->bDescriptorType == USB_DT_DEVICE))
            break;
        USB_info("skipping descriptor 0x%X\n", header->bDescriptorType);
        numskipped++;

        buffer += header->bLength;
        size -= header->bLength;
        parsed += header->bLength;
    }

    if (numskipped)
        USB_warning("Skipped %d class/vendor specific endpoint descriptors\n", numskipped);

    return parsed;
}


static int  usb_parse_interface(USB_DEV_T *dev, int cfg_value, uint8_t *buffer, int size)
{
    int     i, numskipped, retval, parsed = 0;
    USB_DESC_HDR_T  *header;
    USB_IF_DESC_T   ifp;

#ifdef USB_VERBOSE_DEBUG
    //HexDumpBuffer("usb_parse_interface", (uint8_t *)buffer, size);
#endif

    while (size > 0) {
        memcpy((char *)&ifp, buffer, USB_DT_INTERFACE_SIZE);

        /* Skip over the interface */
        buffer += ifp.bLength;
        parsed += ifp.bLength;
        size -= ifp.bLength;

        numskipped = 0;

        /*
         *  Skip over any interface, class or vendor descriptors
         */
        while (size >= sizeof(USB_DESC_HDR_T)) {
            header = (USB_DESC_HDR_T *)buffer;

            if (header->bLength < 2) {
                USB_error("Invalid descriptor length of %d\n", header->bLength);
                return -1;
            }

            /* If we find another descriptor which is at or below */
            /*  us in the descriptor heirarchy then return */
            if ((header->bDescriptorType == USB_DT_INTERFACE) ||
                    (header->bDescriptorType == USB_DT_ENDPOINT) ||
                    (header->bDescriptorType == USB_DT_CONFIG) ||
                    (header->bDescriptorType == USB_DT_DEVICE))
                break;

            USB_info("skipping descriptor 0x%X\n", header->bDescriptorType);
            numskipped++;
            buffer += header->bLength;
            parsed += header->bLength;
            size -= header->bLength;
        }

        if (numskipped) {
            USB_warning("skipped %d class/vendor specific interface descriptors\n", numskipped);
        }

        /* Did we hit an unexpected descriptor? */
        header = (USB_DESC_HDR_T *)buffer;
        if ((size >= sizeof(USB_DESC_HDR_T)) &&
                ((header->bDescriptorType == USB_DT_CONFIG) ||
                 (header->bDescriptorType == USB_DT_DEVICE))) {
            USB_warning("parsing interface - hit an unexpected descriptor!\n");
            return parsed;
        }

        if (ifp.bNumEndpoints > USB_MAXENDPOINTS) {
            USB_warning("Warning - illegal endpoint number %d\n", ifp.bNumEndpoints);
            return -1;
        }

        /* fixed by YCHuang, 2002.03.13, ifp.bNumEndpoints number may be zero */
        if (ifp.bNumEndpoints > 0) {
            for (i = 0; i < ifp.bNumEndpoints; i++) {
                header = (USB_DESC_HDR_T *)buffer;
                if (header->bLength > size) {
                    USB_error("Error - ran out of descriptors parsing");
                    return -1;
                }

#ifdef DUMP_DEV_DESCRIPTORS
                usbh_dump_ep_descriptor((USB_EP_DESC_T *)buffer);
#endif
                retval = usb_parse_endpoint(dev, cfg_value, &ifp, buffer, size);
                if (retval < 0)
                    return retval;

                buffer += retval;
                parsed += retval;
                size -= retval;
            }
        }

        /* We check to see if it's an alternate to this one */
        memcpy((char *)&ifp, buffer, USB_DT_INTERFACE_SIZE);
        if ((size < USB_DT_INTERFACE_SIZE) ||
                (ifp.bDescriptorType != USB_DT_INTERFACE) || !ifp.bAlternateSetting)
            return parsed;
    }   /* end of while */
    return parsed;
}


static int  usb_parse_configuration(USB_DEV_T *dev, USB_CONFIG_DESC_T *config, uint8_t *buffer)
{
    int    i, retval, size;
    USB_DESC_HDR_T  *header;

    memcpy(config, buffer, USB_DT_CONFIG_SIZE);
    size = config->wTotalLength;

    if (config->bNumInterfaces > USB_MAXINTERFACES) {
        USB_warning("Warning - too many interfaces\n");
        return -1;
    }

    buffer += config->bLength;
    size -= config->bLength;

    for (i = 0; i < config->bNumInterfaces; i++) {
        int numskipped;

        /* Skip over the rest of the Class Specific or Vendor */
        /*  Specific descriptors */
        numskipped = 0;
        while (size >= sizeof(USB_DESC_HDR_T)) {
            header = (USB_DESC_HDR_T *)buffer;

            if ((header->bLength > size) || (header->bLength < 2)) {
                USB_error("Error - invalid descriptor length of %d\n", header->bLength);
                return -1;
            }

            /* If we find another descriptor which is at or below */
            /*  us in the descriptor heirarchy then we're done  */
            if ((header->bDescriptorType == USB_DT_ENDPOINT) ||
                    (header->bDescriptorType == USB_DT_INTERFACE) ||
                    (header->bDescriptorType == USB_DT_CONFIG) ||
                    (header->bDescriptorType == USB_DT_DEVICE))
                break;

            USB_info("skipping descriptor 0x%X\n", header->bDescriptorType);
            numskipped++;

            buffer += header->bLength;
            size -= header->bLength;
        } /* end of while */

        if (numskipped) {
            USB_warning("skipped %d class/vendor specific endpoint descriptors\n", numskipped);
        }

#ifdef DUMP_DEV_DESCRIPTORS
        usbh_dump_iface_descriptor((USB_IF_DESC_T *)buffer);
#endif

        retval = usb_parse_interface(dev, config->bConfigurationValue, buffer, size);
        if (retval < 0)
            return retval;

        /* probe drivers... */
        usb_find_interface_driver(dev, (USB_IF_DESC_T *)buffer);

        buffer += retval;
        size -= retval;
    }
    return size;
}


/*
 *  Something got disconnected. Get rid of it, and all of its children.
 */
void usbh_disconnect_device(USB_DEV_T **pdev)
{
    USB_DEV_T       *dev = *pdev;
    USB_DRIVER_T    *driver;
    int             i;

    if (!dev)
        return;

    *pdev = NULL;

    USB_info("usbh_disconnect_device - USB disconnect on device %d\n", dev->devnum);

    usbh_free_dev_urbs(dev);

    for (i = 0; i < dev->driver_cnt; i++) {
        driver = dev->driver[i];
        driver->disconnect(dev);
    }

    /* Free up all the children.. */
    for (i = 0; i < USB_MAXCHILDREN; i++) {
        USB_DEV_T **child = dev->children + i;
        if (*child)
            usbh_disconnect_device(child);
    }

    /* Free the device number and remove the /proc/bus/usb entry */
    if (dev->devnum > 0) {
        g_devmap &= ~(1 << dev->devnum);
    }

    /* Free up the device itself */
    usbh_free_device(dev);

    usbh_mdelay(5);   // let Host Controller got some time to free all
}


/*
 *  Connect a new USB device. This basically just initializes
 *  the USB device information and sets up the topology - it's
 *  up to the low-level driver to reset the port and actually
 *  do the setup (the upper levels don't know how to do that).
 */
void usbh_connect_device(USB_DEV_T *dev)
{
    int  devnum;

    dev->descriptor.bMaxPacketSize0 = 8;  /* Start off at 8 bytes  */

    for (devnum = 1; devnum < 31; devnum++) {
        if ((g_devmap & (1 << devnum)) == 0)
            break;
    }
    if (devnum > 31) {
        USB_error("Serious devnum error!\n");
        devnum = 31;
    }

    g_devmap |= (1 << devnum);
    dev->devnum = devnum;
}


/*
 * These are the actual routines to send
 * and receive control messages.
 */

#define GET_TIMEOUT   1000                 /*!< USB transfer time-out setting */
#define SET_TIMEOUT   100                  /*!< USB transfer time-out setting */

int  usbh_set_address(USB_DEV_T *dev)
{
    return USBH_SendCtrlMsg(dev, usb_snddefctrl(dev), USB_REQ_SET_ADDRESS,
                            0, dev->devnum, 0, NULL, 0, HZ * GET_TIMEOUT);
}


/**
  * @brief  Get a descriptor from device.
  * @param[in] dev    The USB devise.
  * @param[in] type   Descriptor type.
  * @param[in] index  Index of the descriptor.
  * @param[out] buf    Buffer to store the descriptor.
  * @param[in] size   Available size of buf.
 *  @return   Length of the descriptor or error code.
 *  @retval   >=0  Length of the descriptor.
 *  @retval   Otherwise  Failed
  */
int32_t  USBH_GetDescriptor(USB_DEV_T *dev, uint8_t type, uint8_t index,
                        void *buf, int size)
{
    int    i = 5;
    int    result = 0;

    memset(buf, 0, size);     /* Make sure we parse really received data */

    while (i--) {
        if ((result = USBH_SendCtrlMsg(dev, usb_rcvctrlpipe(dev, 0),
                                       USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
                                       (type << 8) + index, 0, buf, size, HZ * GET_TIMEOUT)) > 0 || result == USB_ERR_PIPE)
            break;  /* retry if the returned length was 0; flaky device */
    }
    return result;
}

static int usbh_get_string_descriptor(USB_DEV_T *dev, uint16_t langid, uint8_t index,
                                      void *buf, int size)
{
    return USBH_SendCtrlMsg(dev, usb_rcvctrlpipe(dev, 0),
                            USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
                            (USB_DT_STRING << 8) + index, langid, buf, size, HZ * GET_TIMEOUT);
}


static int usbh_get_device_descriptor(USB_DEV_T *dev)
{
    int     ret;

    ret = USBH_GetDescriptor(dev, USB_DT_DEVICE, 0, &dev->descriptor, sizeof(dev->descriptor));
    return ret;
}


int  usbh_get_protocol(USB_DEV_T *dev, int ifnum)
{
    uint8_t type;
    int     ret;

    if ((ret = USBH_SendCtrlMsg(dev, usb_rcvctrlpipe(dev, 0),
                                USB_REQ_GET_PROTOCOL, USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                                0, ifnum, &type, 1, HZ * GET_TIMEOUT)) < 0)
        return ret;

    return type;
}


int  usbh_set_protocol(USB_DEV_T *dev, int ifnum, int protocol)
{
    return USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                            USB_REQ_SET_PROTOCOL, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                            protocol, ifnum, NULL, 0, HZ * SET_TIMEOUT);
}


int  usbh_set_idle(USB_DEV_T *dev, int ifnum, int duration, int report_id)
{
    return USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                            USB_REQ_SET_IDLE, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                            (duration << 8) | report_id, ifnum, NULL, 0, HZ * SET_TIMEOUT);
}


/**
  * @brief    Clear the halt state of an endpoint.
  * @param[in]  dev   The USB device.
  * @param[in]  pipe  Pipe description of the endpoint.
 *  @return   Success or not.
  * @retval   0    Success
  * @retval   Otherwise   Failed
  */
int32_t USBH_ClearHalt(USB_DEV_T *dev, int pipe)
{
    int         result;
    uint16_t    status;
    int         endp;

    endp = usb_pipeendpoint(pipe)|(usb_pipein(pipe)<<7);

    /*
    if (!usb_endpoint_halted(dev, endp & 0x0f, usb_endpoint_out(endp)))
        return 0;
    */

    result = USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                              USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0, endp, NULL, 0, HZ * SET_TIMEOUT);

    /* don't clear if failed */
    if (result < 0)
        return result;

    result = USBH_SendCtrlMsg(dev, usb_rcvctrlpipe(dev, 0),
                              USB_REQ_GET_STATUS, USB_DIR_IN | USB_RECIP_ENDPOINT, 0, endp,
                              &status, sizeof(status), HZ * SET_TIMEOUT);
    if (result < 0)
        return result;

    if (USB_SWAP16(status) & 1)
        return USB_ERR_PIPE;          /* still halted */

    usb_endpoint_running(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));

    /* toggle is reset on clear */

    usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 0);

    return 0;
}


/**
  * @brief    Set USB device interface.
  * @param[in] dev   The USB device.
  * @param[in] interface  Interface number.
  * @param[in] alternate  Interface alternate setting number.
  * @return  Success or not.
  * @retval   0  Success
  * @retval   Otherwise  Failed
  */
int32_t USBH_SetInterface(USB_DEV_T *dev, char interface, char alternate)
{
    int       ret;

    if ((ret = USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                                USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE, alternate,
                                interface, NULL, 0, HZ * 5)) < 0)
        return ret;

    dev->act_iface = interface;
    dev->iface_alternate = alternate;
    dev->toggle[0] = 0;     /* 9.1.1.5 says to do this */
    dev->toggle[1] = 0;
    return 0;
}


/**
  * @brief   Select USB device configuration.
  * @param[in] dev  The USB device.
  * @param[in] configuration   Configuration number.
  * @return  Success or not.
  * @retval  0   Success
  * @retval  Otherwise   Failed
  */
int32_t  USBH_SetConfiguration(USB_DEV_T *dev, int configuration)
{
    int     ret;

    if ((ret = USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                                USB_REQ_SET_CONFIGURATION, 0, configuration, 0, NULL, 0, HZ * SET_TIMEOUT)) < 0)
        return ret;
    dev->act_config = configuration;
    dev->toggle[0] = 0;
    dev->toggle[1] = 0;
    return 0;
}


/*
 *  usbh_settle_configuration
 *
 *  1. get configuration descriptor
 *  2. set default configuration
 *  3. parse configuration
 *  4. probe USB device drivers by interface
 */
static int  usbh_settle_configuration(USB_DEV_T *dev)
{
    int         result;
    uint32_t    cfgno, length;
    uint8_t     *buffer;
    USB_CONFIG_DESC_T *desc;
    uint32_t    stack_buff[256];

    if (dev->descriptor.bNumConfigurations > USB_MAXCONFIG) {
        USB_warning("Too many configurations\n");
        return USB_ERR_INVAL;
    }

    if (dev->descriptor.bNumConfigurations == 0) {
    //if (dev->descriptor.bNumConfigurations != 1) {
        //USB_warning("This driver does not support multi-configuration devices!\n");
        USB_warning("Not enough configurations\n");
        return USB_ERR_INVAL;
    }

    buffer = (uint8_t *)&stack_buff[0];
    desc = (USB_CONFIG_DESC_T *)buffer;

    for (cfgno = 0; cfgno < dev->descriptor.bNumConfigurations; cfgno++) {
        /* We grab the first 8 bytes so we know how long the whole */
        /*  configuration is */
        result = USBH_GetDescriptor(dev, USB_DT_CONFIG, cfgno, buffer, 8);
        if (result < 8) {
            if (result < 0)
                USB_error("Unable to get descriptor\n");
            else {
                USB_error("Config descriptor too short (expected %i, got %i)\n", 8, result);
                result = USB_ERR_INVAL;
            }
            goto err;
        }

        /* Get the full buffer */
        length = desc->wTotalLength;

        if (length > 256) {
            length = 256;
            USB_error("Config descriptor is too large. Read 256 bytes only. This may cause lost of information!\n");
        }

        /* Now that we know the length, get the whole thing */
        result = USBH_GetDescriptor(dev, USB_DT_CONFIG, cfgno, buffer, length);
        if (result < 0) {
            USB_error("Couldn't get all of config descriptors\n");
            goto err;
        }

        if (result < length) {
            USB_error("Config descriptor too short (expected %u, got %d)\n", length, result);
            result = USB_ERR_INVAL;
            goto err;
        }

        /*
         * Set first configuration as the default configuration
         */
        if (cfgno == 0) {
            result = USBH_SetConfiguration(dev, desc->bConfigurationValue);
            if (result) {
                USB_error("Failed to set device %d default configuration (error=%d)\n", dev->devnum, result);
                goto err;
            }
        }

#ifdef DUMP_DEV_DESCRIPTORS
        usbh_dump_config_descriptor(desc);
#endif

        result = usb_parse_configuration(dev, desc, buffer);
        if (result > 0) {
            USB_warning("Descriptor data left\n");
        } else if (result < 0) {
            USB_warning("usb_parse_configuration error\n");
            result = USB_ERR_INVAL;
            goto err;
        }
    }

    return 0;

err:
    dev->descriptor.bNumConfigurations = cfgno;
    return result;
}

/*
 * usbh_translate_string:
 *      returns string length (> 0) or error (< 0)
 */
int  usbh_translate_string(USB_DEV_T *dev, int index, char *buf, int size)
{
    int         err = 0;
    uint32_t    u, idx;
    uint8_t     tbuf[256];

    if (size == 0 || !buf || !index)
        return USB_ERR_INVAL;
    buf[0] = 0;

    /* get langid for strings if it's not yet known */
    if (!dev->have_langid) {
        err = usbh_get_string_descriptor(dev, 0, 0, tbuf, 4);
        if (err < 0) {
            USB_error("usbh_translate_string - error getting string descriptor 0 (error=%d)\n", err);
            goto errout;
        } else if (tbuf[0] < 4) {
            USB_error("usbh_translate_string - string descriptor 0 too short\n");
            err = USB_ERR_INVAL;
            goto errout;
        } else {
            dev->have_langid = -1;
            dev->string_langid = tbuf[2] | (tbuf[3]<< 8);
            /* always use the first langid listed */
            USB_info("USB device number %d default language ID 0x%x\n",
                     dev->devnum, dev->string_langid);
        }
    }

    /* YCHuang, added for W99683, prevent halt */
    if ((dev->descriptor.idVendor == 0x416) && (dev->descriptor.idProduct == 0x9683)) {
        USB_warning("??? Skip usbh_get_string_descriptor, 255\n");
        goto errout;
    }


    /*
     * Just ask for a maximum length string and then take the length
     * that was returned.
     */
    err = usbh_get_string_descriptor(dev, dev->string_langid, index, tbuf, 255);
    if (err < 0)
        goto errout;

    size--;                     /* leave room for trailing NULL char in output buffer */
    for (idx = 0, u = 2; u < err; u += 2) {
        if (idx >= size)
            break;
        if (tbuf[u+1])           /* high byte */
            buf[idx++] = '?';    /* non-ASCII character */
        else
            buf[idx++] = tbuf[u];
    }
    buf[idx] = 0;
    err = idx;

errout:
    return err;
}



/*
 *  By the time we get here, the device has gotten a new device ID
 *  and is in the default state. We need to identify the thing and
 *  get the ball rolling..
 *
 *  Returns 0 for success, != 0 for error.
 */
int  usbh_settle_new_device(USB_DEV_T *dev)
{
    int     err;

    USB_info("[USBH] Enter usbh_settle_new_device() ...\n");
    /*
     *  USB v1.1 5.5.3
     *  We read the first 8 bytes from the device descriptor to get to
     *  the bMaxPacketSize0 field. Then we set the maximum packet size
     *  for the control pipe, and retrieve the rest
     */
    dev->ep_list[0].bEndpointAddress = 0;;
    dev->ep_list[0].wMaxPacketSize = 8;
    dev->ep_list_cnt = 1;

    err = usbh_set_address(dev);
    if (err < 0) {
        USB_error("USB device not accepting new address=%d (error=%d)\n", dev->devnum, err);
        g_devmap &= ~(1 << dev->devnum);
        dev->devnum = -1;
        return 1;
    }

    usbh_mdelay(10);    /* Let the SET_ADDRESS settle */

    memset((char *)&dev->descriptor, 0, 8);

    err = USBH_GetDescriptor(dev, USB_DT_DEVICE, 0, &dev->descriptor, 8);
    if (err < 8) {
        USB_debug("USBH_GetDescriptor failed!!\n");
        if (err < 0)
            USB_error("USB device not responding, giving up (error=%d)\n", err);
        else
            USB_error("USB device descriptor short read (expected %i, got %i)\n", 8, err);
        g_devmap &= ~(1 << dev->devnum);
        dev->devnum = -1;
        return 1;
    }

    dev->ep_list[0].wMaxPacketSize = dev->descriptor.bMaxPacketSize0;

    err = usbh_get_device_descriptor(dev);
    if (err < sizeof(dev->descriptor)) {
        if (err < 0)
            USB_error("unable to get device descriptor (error=%d)\n", err);
        else
            USB_error("USB device descriptor short read (expected %i, got %i)\n",
                      sizeof(dev->descriptor), err);

        g_devmap &= ~(1 << dev->devnum);
        dev->devnum = -1;
        return USB_ERR_INVAL;
    }

#ifdef DUMP_DEV_DESCRIPTORS
    usbh_dump_device_descriptor(&dev->descriptor);
#endif

    err = usbh_settle_configuration(dev);
    if (err < 0) {
        USB_error("Unable to get device %d configuration (error=%d)\n", dev->devnum, err);
        g_devmap &= ~(1 << dev->devnum);
        dev->devnum = -1;
        usbh_free_device(dev);
        return 1;
    }

#ifdef USB_VERBOSE_DEBUG
    if (dev->descriptor.iManufacturer)
        usbh_print_usb_string(dev, "Manufacturer", dev->descriptor.iManufacturer);
    if (dev->descriptor.iProduct)
        usbh_print_usb_string(dev, "Product", dev->descriptor.iProduct);
    if (dev->descriptor.iSerialNumber)
        usbh_print_usb_string(dev, "SerialNumber", dev->descriptor.iSerialNumber);
#endif
    return 0;
}


/**
  * @brief    Open USB Host controller function
  * @return   Success or not.
  * @retval   0   Success
  * @retval   Otherwise  Failed
  */
int32_t USBH_Open(void)
{
    usbh_init_memory();

    /*
     * Init global variables
     */
    g_devmap = 0;

    usbh_init_hub_driver();

#ifdef SUPPORT_HID_CLASS
#endif

#ifdef SUPPORT_UMAS_CLASS
#endif

#ifdef SUPPORT_UVC_CLASS
#endif

#ifdef SUPPORT_UAC_CLASS
#endif

#ifdef SUPPORT_CDC_CLASS
#endif

    halUSBH_Open();
    if (usbh_init_ohci() < 0) {
        USB_debug("OHCI init failed!\n");
        return -1;
    }

    return 0;
}


/**
  * @brief    Disable USB Host controller function
  * @return   Success or not.
  * @retval   0   Success
  * @retval   Otherwise   Failed
  */
int32_t USBH_Close(void)
{
    halUSBH_Close();

    usbh_init_memory();

    return 0;
}

/**
  * @brief    Suspend USB Host Controller and devices
  * @return   Success or not.
  * @retval   0  Success
  * @retval   Otherwise   Failed
  */
int32_t USBH_Suspend(void)
{
    int         i;
    USB_DEV_T   *dev;

    /* Set feature Device Remote Wakeup to all devices */
    for (i = 0; i < DEV_MAX_NUM; i++) {
        if (dev_alloc_mark[i]) {
            dev = &g_dev_pool[i];
            USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                             USB_REQ_SET_FEATURE, 0x00, 0x0001, 0x0000, NULL, 0x0000, HZ * SET_TIMEOUT);
        }
    }

    /* set port suspend if connected */
    halUSBH_SuspendAllRhPort();

    /* enable Device Remote Wakeup */
    /* enable USBH RHSC interrupt for system wakeup */
    halUSBH_RemoteWkup_EN();

    /* set Host Controller enter suspend state */
    halUSBH_SuspendHostControl();

    return 0;
}


/**
  * @brief    Resume USB Host controller and devices
  * @return   Success or not.
  * @retval   0   Success
  * @retval   Otherwise   Failed
  */
int32_t USBH_Resume(void)
{
    halUSBH_ResumeHostControl();

    halUSBH_ResumeAllRhPort();

    return 0;
}

/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/

