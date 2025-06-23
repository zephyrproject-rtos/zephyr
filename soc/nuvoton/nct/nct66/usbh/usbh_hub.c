
/**************************************************************************//**
 * @file     usbh_hub.c
 * @brief    MCU USB Host Library Hub class driver
 *
 * @note
 * (C) Copyright 1999 Linus Torvalds
 * (C) Copyright 1999 Johannes Erdfelt
 * (C) Copyright 1999 Gregory P. Smith
 * (C) Copyright 2001 Brad Hards (bhards@bigpond.net.au)
 *
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "usbh_core.h"
#include "usbh_hub.h"


static LIST_HEAD(_HubEventList);       /* List of hubs needing servicing */

static int  usb_hub_events(void);

extern USB_HUB_T * usbh_get_hub_by_dev(USB_DEV_T *dev);

static int  usb_get_hub_descriptor(USB_DEV_T *dev, void *data, int size)
{
    return USBH_SendCtrlMsg(dev, usb_rcvctrlpipe(dev, 0),
                            USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
                            USB_DT_HUB << 8, 0, data, size, HZ);
}


static int  usb_clear_hub_feature(USB_DEV_T *dev, int feature)
{
    return USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                            USB_REQ_CLEAR_FEATURE, USB_RT_HUB, feature, 0, NULL, 0, HZ);
}


static int  usb_clear_port_feature(USB_DEV_T *dev, int port, int feature)
{
    return USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                            USB_REQ_CLEAR_FEATURE, USB_RT_PORT, feature, port, NULL, 0, HZ);
}


static int  usb_set_port_feature(USB_DEV_T *dev, int port, int feature)
{
    USB_info("usb_set_port_feature, port:%d of hub:%d\n", port, dev->devnum);
    return USBH_SendCtrlMsg(dev, usb_sndctrlpipe(dev, 0),
                            USB_REQ_SET_FEATURE, USB_RT_PORT, feature, port, NULL, 0, HZ);
}


static int  usb_get_hub_status(USB_DEV_T *dev, void *data)
{
    int             status;
    uint32_t        stack_buff[8];

    status = USBH_SendCtrlMsg(dev, usb_rcvctrlpipe(dev, 0),
                              USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
                              (void *)&stack_buff[0], sizeof(USB_HUB_STATUS_T), HZ*3);
    memcpy((uint8_t *)data, (uint8_t *)&stack_buff[0], sizeof(USB_HUB_STATUS_T));
    return status;
}


static int  usb_get_port_status(USB_DEV_T *dev, int port, void *data)
{
    int             status;
    uint32_t        stack_buff[8];

    status = USBH_SendCtrlMsg(dev, usb_rcvctrlpipe(dev, 0),
                              USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port,
                              (void *)&stack_buff[0], sizeof(USB_PORT_STATUS_T), HZ * 5);
    memcpy((uint8_t *)data, (uint8_t *)&stack_buff[0], sizeof(USB_PORT_STATUS_T));
    return status;
}

/**
  * @brief    Processed USB hub device events. User application must invoke this routine
  *           in the main while loop. Device enumeration is done in this routine.
  * @return   Have hub events or not.
  * @retval   0   No hub events
  * @retval   1   Have hub events
  */
int32_t  USBH_ProcessHubEvents()
{
    ohci_int_timer_do(0);
    return usb_hub_events();
}

/*
 * hub int-in complete function
 */
static void  hub_irq(URB_T *urb)
{
    USB_HUB_T  *hub = (USB_HUB_T *)urb->context;

    //USB_debug("hub_irq!!\n");

    /* Cause a hub reset after 10 consecutive errors */
    if (urb->status) {
        if (urb->status == USB_ERR_NOENT)
            return;
        USB_warning("nonzero status in irq %d\n", urb->status);
        if ((++hub->nerrors < 10) || hub->error)
            return;
        hub->error = urb->status;
    }
    hub->nerrors = 0;

    /* Something happened, let khubd figure it out */

    /* Add the hub to the event queue */
    if (list_empty(&hub->event_list)) {
        list_add(&hub->event_list, &_HubEventList);
    }
}


static void usb_hub_power_on(USB_HUB_T *hub)
{
    int   i;

    /* Enable power to the ports */
    for (i = 0; i < hub->descriptor.bNbrPorts; i++) {
        USB_info("enable port:%d of hub:%d\n", i+1, hub->dev->devnum);
        usb_set_port_feature(hub->dev, i + 1, USB_PORT_FEAT_POWER);
    }

    /* Wait for power to be enabled */
    usbh_mdelay(hub->descriptor.bPwrOn2PwrGood * 2);
}


static int usb_hub_configure(USB_HUB_T *hub, EP_INFO_T *ep_info)
{
    USB_DEV_T           *dev = hub->dev;
    USB_HUB_STATUS_T    hubstatus;
    uint32_t            pipe;
    int                 maxp, ret;

    USB_info("[HUB] Enter usb_hub_configure()... hub:%d\n", hub->dev->devnum);

    /* Request the entire hub descriptor. */
    ret = usb_get_hub_descriptor(dev, &hub->descriptor, sizeof(USB_HUB_DESC_T));

    /* <hub->descriptor> is large enough for a hub with 127 ports;
     * the hub can/will return fewer bytes here. */
    if (ret < 0) {
        USB_error("Erro - Unable to get hub descriptor (err = %d)\n", ret);
        return ret;
    }
    if (hub->descriptor.bNbrPorts > USB_MAXCHILDREN) {
        return -1;
    }
    dev->maxchild = hub->descriptor.bNbrPorts;

#ifdef USB_VERBOSE_DEBUG
    USB_info("%d port%s detected\n", hub->descriptor.bNbrPorts, (hub->descriptor.bNbrPorts == 1) ? "" : "s");

    /* D2: Identifying a Compound Device */
    if (hub->descriptor.wHubCharacteristics & HUB_CHAR_COMPOUND) {
        USB_info("part of a compound device\n");
    } else {
        USB_info("standalone hub\n");
    }

    /* D1..D0: Logical Power Switching Mode */
    switch (hub->descriptor.wHubCharacteristics & HUB_CHAR_LPSM) {
    case 0x00:
        USB_info("ganged power switching\n");
        break;
    case 0x01:
        USB_info("individual port power switching\n");
        break;
    case 0x02:
    case 0x03:
        USB_info("unknown reserved power switching mode\n");
        break;
    }

    /* D4..D3: Over-current Protection Mode */
    switch (hub->descriptor.wHubCharacteristics & HUB_CHAR_OCPM) {
    case 0x00:
        USB_info("global over-current protection\n");
        break;
    case 0x08:
        USB_info("individual port over-current protection\n");
        break;
    case 0x10:
    case 0x18:
        USB_info("no over-current protection\n");
        break;
    }

    switch (dev->descriptor.bDeviceProtocol) {
    case 0:
        break;
    case 1:
        USB_debug("Single TT, ");
        break;
    case 2:
        USB_debug("TT per port, ");
        break;
    default:
        USB_debug("Unrecognized hub protocol %d", dev->descriptor.bDeviceProtocol);
        break;
    }

    USB_info("power on to power good time: %dms\n", hub->descriptor.bPwrOn2PwrGood * 2);
    USB_info("hub controller current requirement: %dmA\n", hub->descriptor.bHubContrCurrent);
#endif

    ret = usb_get_hub_status(dev, &hubstatus);
    if (ret < 0) {
        USB_error("Unable to get hub %d status (err = %d)\n", hub->dev->devnum, ret);
        return ret;
    }

    hubstatus.wHubStatus = USB_SWAP16(hubstatus.wHubStatus);

#ifdef USB_VERBOSE_DEBUG
    /* Hub status bit 0, Local Power Source */
    if (hubstatus.wHubStatus & HUB_STATUS_LOCAL_POWER) {
        USB_info("local power source is lost (inactive)\n");
    } else {
        USB_info("local power source is good\n");
    }

    /* Hub status bit 1, Over-current Indicator */
    if (hubstatus.wHubStatus & HUB_STATUS_OVERCURRENT) {
        USB_info("!! over-current\n");
    } else {
        USB_info("No over-current.\n");
    }
#endif

    /* Start the interrupt endpoint */
    pipe = usb_rcvintpipe(dev, ep_info->bEndpointAddress);
    maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

    if (maxp > sizeof(hub->buffer))
        maxp = sizeof(hub->buffer);

    hub->urb = USBH_AllocUrb();
    if (!hub->urb) {
        USB_error("Error - couldn't allocate interrupt urb");
        return USB_ERR_NOMEM;
    }

#if 1   /* YCHuang 2012.06.01 */
    if (ep_info->bInterval < 16)
        ep_info->bInterval = 16;
#endif
    FILL_INT_URB(hub->urb, dev, pipe, hub->buffer, maxp, hub_irq,
                 hub, ep_info->bInterval);
    ret = USBH_SubmitUrb(hub->urb);
    if (ret) {
        USB_error("Error - USBH_SubmitUrb failed (%d)", ret);
        USBH_FreeUrb(hub->urb);
        return ret;
    }

    if (g_ohci_bus.root_hub != hub->dev)
        usb_hub_power_on(hub);
    return 0;
}


static int  hub_probe(USB_DEV_T *dev, USB_IF_DESC_T *ifd, const USB_DEV_ID_T *id)
{
    EP_INFO_T   *ep_info;
    USB_HUB_T   *hub;
    int         ifnum;
    int         i;

    USB_debug("hub_probe - dev=0x%x\n", (int)dev);

    ifnum = ifd->bInterfaceNumber;

    if (dev->descriptor.bDeviceClass != 0x09)
        return USB_ERR_NODEV;

    ep_info = NULL;
    for (i = 0; i < dev->ep_list_cnt; i++) {
        if ((dev->ep_list[i].ifnum == ifnum) &&
                ((dev->ep_list[i].bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)) {
            ep_info = &dev->ep_list[i];
            break;
        }
    }
    if (ep_info == NULL) {
        USB_error("hub int ep not found!\n");
        return USB_ERR_NODEV;
    }

    /* Output endpoint? Curiousier and curiousier.. */
    if (!(ep_info->bEndpointAddress & USB_DIR_IN)) {
        USB_error("Error - Device #%d is hub class, but has output endpoint?\n", dev->devnum);
        return USB_ERR_NODEV;
    }

    /* If it's not an interrupt endpoint, we'd better punt! */
    if ((ep_info->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_INT) {
        USB_error("Error - Device #%d is hub class, but has endpoint other than interrupt?\n", dev->devnum);
        return USB_ERR_NODEV;
    }

    /* We found a hub */
    USB_debug("USB hub found\n");

    hub = usbh_alloc_hubdev();
    if (!hub)
        return USB_ERR_NOMEM;

    INIT_LIST_HEAD(&hub->event_list);
    hub->dev = dev;

    if (usb_hub_configure(hub, ep_info) == 0)
        return 0;

    USB_error("Error - hub configuration failed for device #%d\n", dev->devnum);

    /* Delete it and then reset it */
    list_del(&hub->event_list);
    INIT_LIST_HEAD(&hub->event_list);

    usbh_free_hubdev(hub);
    return USB_ERR_NODEV;
}


static void  hub_disconnect(USB_DEV_T *dev)
{
    USB_HUB_T  *hub;

    hub = usbh_get_hub_by_dev(dev);

    if (hub == NULL) {
        USB_warning("hub_disconnect - hub device not found!\n");
        return;
    }

    /* Delete it and then reset it */
    list_del(&hub->event_list);
    INIT_LIST_HEAD(&hub->event_list);

    if (hub->urb) {
        USBH_UnlinkUrb(hub->urb);
        USBH_FreeUrb(hub->urb);
        hub->urb = NULL;
    }
    usbh_free_hubdev(hub);
}


#define HUB_RESET_TRIES             2       /*!< Hub port reset device retry times */
#define HUB_PROBE_TRIES             3       /*!< Hub probe device retry times      */
#define HUB_SHORT_RESET_TIME        150     /*!< Shortest port reset time          */
#define HUB_LONG_RESET_TIME         500     /*!< Longest port reset time           */
#define HUB_RESET_TIMEOUT           3000    /*!< Total maximum port reset time     */

static int  usb_hub_port_wait_reset(USB_DEV_T *hub, int port,
                                    USB_DEV_T *dev, uint32_t delay)
{
    int         delay_time, ret;
    uint16_t    portchange, portstatus;
    USB_PORT_STATUS_T  portsts;

    for (delay_time = 0; delay_time < HUB_RESET_TIMEOUT; delay_time += delay) {
        /* wait to give the device a chance to reset */
        usbh_mdelay(delay);

        /* read and decode port status */
        ret = usb_get_port_status(hub, port + 1, &portsts);
        if (ret < 0) {
            USB_error("Error - get_port_status(%d) failed (err = %d)\n", port + 1, ret);
            return -1;
        }

        portstatus = portsts.wPortStatus;
        portchange = portsts.wPortChange;

        USB_info("port %d of hub %d, portstatus %x, change %x, %s\n", port + 1, hub->devnum,
                 portstatus, portchange,
                 portstatus & (1 << USB_PORT_FEAT_LOWSPEED) ? "1.5 Mb/s" : "12 Mb/s");

        if ((portchange & USB_PORT_STAT_C_CONNECTION) ||
                !(portstatus & USB_PORT_STAT_CONNECTION))
            return -1;

        /* if we have finished resetting, then break out of the loop */
        if (!(portstatus & USB_PORT_STAT_RESET) &&
                (portstatus & USB_PORT_STAT_ENABLE)) {
            if (portstatus & USB_PORT_STAT_HIGH_SPEED) {
                USB_debug("Device is high speed!\n");
                dev->speed = USB_SPEED_HIGH;
            } else if (portstatus & USB_PORT_STAT_LOW_SPEED) {
                USB_debug("Device is low speed!\n");
                dev->speed = USB_SPEED_LOW;
            } else {
                USB_debug("Device is full speed!\n");
                dev->speed = USB_SPEED_FULL;
            }

            dev->slow = (portstatus & USB_PORT_STAT_LOW_SPEED) ? 1 : 0;
            return 0;
        }

        /* switch to the long delay after two short delay failures */
        if (delay_time >= 2 * HUB_SHORT_RESET_TIME)
            delay = HUB_LONG_RESET_TIME;

        USB_info("port %d of hub %d not reset yet, waiting %lums\n", port + 1, hub->devnum, delay);
    }
    return -1;
}


static int  usb_hub_port_reset(USB_DEV_T *hub, int port,
                               USB_DEV_T *dev, uint32_t delay)
{
    int    i;

    USB_info("usb_hub_port_reset: hub:%d, port:%d dev:%x\n", hub->devnum, port+1, (int)dev);

    /* Reset the port */
    for (i = 0; i < HUB_RESET_TRIES; i++) { /* retry loop */
        usb_set_port_feature(hub, port + 1, USB_PORT_FEAT_RESET);
        /* return success if the port reset OK */
        if (!usb_hub_port_wait_reset(hub, port, dev, delay)) {
            usb_clear_port_feature(hub, port + 1, USB_PORT_FEAT_C_RESET);
            return 0;
        }

        USB_error("port %d of hub %d not enabled, %dth trying reset again...\n", port + 1, hub->devnum, i);
        delay = HUB_LONG_RESET_TIME;
    }

    USB_error("Cannot enable port %i of hub %d, disabling port.\n", port + 1, hub->devnum);
    USB_error("Error - Maybe the USB cable is bad?\n");
    return -1;
}


void  usb_hub_port_disable(USB_DEV_T *hub, int port)
{
    int    ret;

    ret = usb_clear_port_feature(hub, port + 1, USB_PORT_FEAT_ENABLE);
    if (ret)
        USB_error("cannot disable port %d of hub %d (err = %d)\n",
                  port + 1, hub->devnum, ret);
}


static void  usb_hub_port_connect_change(USB_HUB_T *hubstate, USB_DEV_T *hub, int port,
        USB_PORT_STATUS_T *portsts)
{
    USB_DEV_T   *dev;
    uint16_t    portstatus; // portchange;
    uint32_t    delay = HUB_SHORT_RESET_TIME;
    int         i;

    portstatus = portsts->wPortStatus;
    //portchange = portsts->wPortChange;

    //USB_info("usb_hub_port_connect_change - port %d, portstatus %x, change %x, %s\n", port + 1, portstatus,
    //       portchange, portstatus & (1 << USB_PORT_FEAT_LOWSPEED) ? "1.5 Mb/s" : "12 Mb/s");

    /* Clear the connection change status */
    usb_clear_port_feature(hub, port + 1, USB_PORT_FEAT_C_CONNECTION);

    /* Disconnect any existing devices under this port */
    if (hub->children[port])
        usbh_disconnect_device(&hub->children[port]);

    /* Return now if nothing is connected */
    if (!(portstatus & USB_PORT_STAT_CONNECTION)) {
        if (portstatus & USB_PORT_STAT_ENABLE)
            usb_hub_port_disable(hub, port);
        return;
    }

    if (portstatus & USB_PORT_STAT_LOW_SPEED) {
        usbh_mdelay(400);
        delay = HUB_LONG_RESET_TIME;
    }

    for (i = 0; i < HUB_PROBE_TRIES; i++) {
        /* Allocate a new device struct */
        dev = usbh_alloc_device(hub, hub->bus);
        if (!dev) {
            USB_error("Error - couldn't allocate usb_device\n");
            break;
        }
        dev->hub_port = port;
        hub->children[port] = dev;

        if (usb_hub_port_reset(hub, port, dev, delay)) {
            usbh_free_device(dev);
            break;
        }

        /* Find a new device ID for it */
        usbh_connect_device(dev);

        USB_debug("USB new device connect, assigned device number %d\n", dev->devnum);

        /* Run it through the hoops (find a driver, etc) */
        if (usbh_settle_new_device(dev) == 0)
            return;     // OK.

        /* Free the configuration if there was an error */
        usbh_free_device(dev);

        /* Switch to a long reset time */
        delay = HUB_LONG_RESET_TIME;
    }

    hub->children[port] = NULL;
    usb_hub_port_disable(hub, port);
}


int  usb_hub_events(void)
{
    USB_LIST_T  *tmp;
    USB_DEV_T   *dev;
    USB_HUB_T   *hub;
    USB_HUB_STATUS_T  hubsts;
    uint16_t    hubchange;
    uint16_t    irq_data;
    int         i, ret;

    if (list_empty(&_HubEventList))
        return 0;

    /*
     *  We restart the list everytime to avoid a deadlock with
     *  deleting hubs downstream from this one. This should be
     *  safe since we delete the hub from the event list.
     *  Not the most efficient, but avoids deadlocks.
     */
    while (1) {
        if (list_empty(&_HubEventList))
            break;

        /* Grab the next entry from the beginning of the list */
        tmp = _HubEventList.next;

        hub = list_entry(tmp, USB_HUB_T, event_list);
        dev = hub->dev;

        list_del(tmp);
        INIT_LIST_HEAD(tmp);

        if (hub->error) {
            USB_error("hub error %d!!\n", hub->error);
        }

        /* added by YCHuang */
        if (hub->urb->transfer_buffer_length == 1)
            irq_data = *(uint8_t *)hub->urb->transfer_buffer;
        else {
            /* BIGBIG */
            //irq_data = *(uint16_t *)hub->urb->transfer_buffer;  /* FIX DEBUG, not verified */
            irq_data = (*((uint8_t *)hub->urb->transfer_buffer+1) << 8) |
                       *(uint8_t *)hub->urb->transfer_buffer;
        }

        for (i = 0; i < hub->descriptor.bNbrPorts; i++) {
            USB_PORT_STATUS_T  portsts;
            uint16_t    portstatus, portchange;

            if (!((irq_data >> (i+1)) & 0x01))
                continue;

            USB_info("usb_hub_events - hub:%d, get port status...\n", hub->dev->devnum);
            ret = usb_get_port_status(dev, i+1, &portsts);
            if (ret < 0) {
                USB_error("Error - get_hub %d port %d status failed (err = %d)\n", hub->dev->devnum, i+1, ret);
                continue;
            }

            portstatus = portsts.wPortStatus;
            portchange = portsts.wPortChange;
            USB_debug("portstatus = %x, portchange = %x\n", portstatus, portchange);

            if (portchange & USB_PORT_STAT_C_CONNECTION) {
                USB_info("port %d of hub %d connection change\n", i + 1, hub->dev->devnum);
                usb_hub_port_connect_change(hub, dev, i, &portsts);
            } else if (portchange & USB_PORT_STAT_C_ENABLE) {
                USB_info("port %d of hub %d enable change, status %x\n", i + 1, portstatus, hub->dev->devnum);
                usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_C_ENABLE);

                /*
                 *  EM interference sometimes causes bad shielded USB
                 *  devices to be shutdown by the hub, this hack enables
                 *  them again. Works at least with mouse driver.
                 */
                if (!(portstatus & USB_PORT_STAT_ENABLE) &&
                        (portstatus & USB_PORT_STAT_CONNECTION) &&
                        (dev->children[i])) {
                    USB_error("Error - already running port %i disabled by hub (EMI?), re-enabling...\n",  i + 1);
                    usb_hub_port_connect_change(hub, dev, i, &portsts);
                }
            }

            if (portchange & USB_PORT_STAT_C_SUSPEND) {
                USB_info("port %d of hub %d suspend change\n", i + 1, hub->dev->devnum);
                usb_clear_port_feature(dev, i + 1,  USB_PORT_FEAT_C_SUSPEND);
            }

            if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
                USB_warning("!! port %d of hub %d over-current change\n", i + 1, hub->dev->devnum);
                usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_C_OVER_CURRENT);
                usb_hub_power_on(hub);
            }

            if (portchange & USB_PORT_STAT_C_RESET) {
                USB_info("port %d of hub %d reset change\n", i + 1, hub->dev->devnum);
                usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_C_RESET);
            }
        } /* end for i */

        /* deal with hub status changes */
        if (usb_get_hub_status(dev, &hubsts) < 0)
            USB_error("Error - get_hub_status failed\n");
        else {
            //hubstatus = USB_SWAP16(hubsts.wHubStatus);
            hubchange = USB_SWAP16(hubsts.wHubChange);
            if (hubchange & HUB_CHANGE_LOCAL_POWER) {
                USB_debug("hub power change\n");
                usb_clear_hub_feature(dev, C_HUB_LOCAL_POWER);
            }
            if (hubchange & HUB_CHANGE_OVERCURRENT) {
                USB_error("!!hub overcurrent change\n");
                usbh_mdelay(500);   /* Cool down */
                usb_clear_hub_feature(dev, C_HUB_OVER_CURRENT);
                usb_hub_power_on(hub);
            }
        }

    } /* end while (1) */

    return 1;
}


static USB_DEV_ID_T  hub_id_table = {
    USB_DEVICE_ID_MATCH_INT_CLASS,     /* match_flags */
    0, 0, 0, 0, 0, 0, 0,
    USB_CLASS_HUB,                     /* bInterfaceClass */
    0, 0, 0
};


static USB_DRIVER_T  hub_driver = {
    "hub driver",
    hub_probe,
    hub_disconnect,
    &hub_id_table,
    NULL,                       /* suspend */
    NULL,                       /* resume */
    {NULL,NULL}                 /* driver_list */
};


int  usbh_init_hub_driver(void)
{
    USBH_RegisterDriver(&hub_driver);
    return 0;
}


/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/

