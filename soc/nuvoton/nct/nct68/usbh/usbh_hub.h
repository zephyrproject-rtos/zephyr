#ifndef _USB_HUB_H
#define _USB_HUB_H

/*
 * Hub request types
 */

#define USB_RT_HUB      (USB_TYPE_CLASS | USB_RECIP_DEVICE)   /* 0x20 */
#define USB_RT_PORT     (USB_TYPE_CLASS | USB_RECIP_OTHER)    /* 0x23 */

/*
 * Hub Class feature numbers
 * See USB 2.0 spec Table 11-17
 */
#define C_HUB_LOCAL_POWER               0
#define C_HUB_OVER_CURRENT              1

/*
 * Port feature numbers
 * See USB 2.0 spec Table 11-17
 */
#define USB_PORT_FEAT_CONNECTION        0
#define USB_PORT_FEAT_ENABLE            1
#define USB_PORT_FEAT_SUSPEND           2
#define USB_PORT_FEAT_OVER_CURRENT      3
#define USB_PORT_FEAT_RESET             4
#define USB_PORT_FEAT_POWER             8
#define USB_PORT_FEAT_LOWSPEED          9
#define USB_PORT_FEAT_HIGHSPEED         10
#define USB_PORT_FEAT_C_CONNECTION      16  /* connection change */
#define USB_PORT_FEAT_C_ENABLE          17
#define USB_PORT_FEAT_C_SUSPEND         18
#define USB_PORT_FEAT_C_OVER_CURRENT    19
#define USB_PORT_FEAT_C_RESET           20
#define USB_PORT_FEAT_TEST              21  /* USB 2.0 only */
#define USB_PORT_FEAT_INDICATOR         22  /* USB 2.0 only */

/*
 * Hub Status and Hub Change results
 * See USB 2.0 spec Table 11-19 and Table 11-20
 */
typedef struct usb_port_status {
    uint16_t  wPortStatus;
    uint16_t  wPortChange;
} USB_PORT_STATUS_T;

/*
 * wPortStatus bit field
 * See USB 2.0 spec Table 11-21
 */
#define USB_PORT_STAT_CONNECTION        0x0001
#define USB_PORT_STAT_ENABLE            0x0002
#define USB_PORT_STAT_SUSPEND           0x0004
#define USB_PORT_STAT_OVERCURRENT       0x0008
#define USB_PORT_STAT_RESET             0x0010
/* bits 5 for 7 are reserved */
#define USB_PORT_STAT_POWER             0x0100
#define USB_PORT_STAT_LOW_SPEED         0x0200
#define USB_PORT_STAT_HIGH_SPEED        0x0400
#define USB_PORT_STAT_TEST              0x0800
#define USB_PORT_STAT_INDICATOR         0x1000
/* bits 13 to 15 are reserved */

/*
 * wPortChange bit field
 * See USB 2.0 spec Table 11-22
 * Bits 0 to 4 shown, bits 5 to 15 are reserved
 */
#define USB_PORT_STAT_C_CONNECTION      0x0001
#define USB_PORT_STAT_C_ENABLE          0x0002
#define USB_PORT_STAT_C_SUSPEND         0x0004
#define USB_PORT_STAT_C_OVERCURRENT     0x0008
#define USB_PORT_STAT_C_RESET           0x0010

/*
 * wHubCharacteristics (masks)
 * See USB 2.0 spec Table 11-13, offset 3
 */
#define HUB_CHAR_LPSM                   0x0003 /* D1 .. D0 */
#define HUB_CHAR_COMPOUND               0x0004 /* D2       */
#define HUB_CHAR_OCPM                   0x0018 /* D4 .. D3 */
#define HUB_CHAR_TTTT                   0x0060 /* D6 .. D5 */
#define HUB_CHAR_PORTIND                0x0080 /* D7       */

typedef struct __attribute__((__packed__)) usb_hub_status {
    uint16_t wHubStatus;
    uint16_t wHubChange;
} USB_HUB_STATUS_T;

/*
 * Hub Status & Hub Change bit masks
 * See USB 2.0 spec Table 11-19 and Table 11-20
 * Bits 0 and 1 for wHubStatus and wHubChange
 * Bits 2 to 15 are reserved for both
 */
#define HUB_STATUS_LOCAL_POWER          0x0001
#define HUB_STATUS_OVERCURRENT          0x0002
#define HUB_CHANGE_LOCAL_POWER          0x0001
#define HUB_CHANGE_OVERCURRENT          0x0002


/*
 * This is a bit arbitrary.
 * From USB 2.0 spec Table 11-13, offset 7, a hub
 * can have up to 255 ports. The most ever reported
 * is 10. So we round it up to the next multiple of
 * eight. If increasing this, do so to a multiple
 * of eight to be safe
 */
#define MAX_PORTS_PER_HUB                   8

/*
 * Hub descriptor
 * See USB 2.0 spec Table 11-13
 */
typedef struct __attribute__((__packed__)) usb_hub_descriptor {
    uint8_t  bDescLength;
    uint8_t  bDescriptorType;
    uint8_t  bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t  bPwrOn2PwrGood;
    uint8_t  bHubContrCurrent;
    uint8_t  DeviceRemovable[MAX_PORTS_PER_HUB/8];
    uint8_t  PortPwrCtrlMask[MAX_PORTS_PER_HUB/8];
} USB_HUB_DESC_T;


typedef struct usb_hub {
    USB_DEV_T       *dev;
    URB_T           *urb;               /* Interrupt polling pipe */
    short           error;
    short           nerrors;
    USB_LIST_T      event_list;
    USB_HUB_DESC_T  descriptor;
    char            buffer[(USB_MAXCHILDREN + 1 + 7) / 8];
    /* add 1 bit for hub status change */
    /* and add 7 bits to round up to byte boundary */
} USB_HUB_T;

#ifdef __cplusplus
extern "C"
{
#endif
extern USB_HUB_T * usbh_alloc_hubdev(void);
extern void usbh_free_hubdev(USB_HUB_T *hub);
#ifdef __cplusplus
}
#endif


#endif /* _USB_HUB_H */
