/**************************************************************************//**
 * @file     usbh_core.h
 * @brief    USB Host core driver header file
 *
 * @note
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef _USB_CORE_H_
#define _USB_CORE_H_

#include <stdint.h>
#include "usbh_config.h"
#include "usbh_list.h"
#include "usbh_err_code.h"


/*
 * Device and/or Interface Class codes
 */
#define USB_CLASS_PER_INTERFACE         0       /* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PHYSICAL              5
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_DATA                  10
#define USB_CLASS_APP_SPEC              0xfe
#define USB_CLASS_VENDOR_SPEC           0xff

/*
 * USB types
 */
#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

/*
 * USB recipients
 */
#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

/*
 * USB directions
 */
#define USB_DIR_OUT                     0
#define USB_DIR_IN                      0x80

/*
 * Descriptor types
 */
#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05

#define USB_DT_HID                      (USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT                   (USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL                 (USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB                      (USB_TYPE_CLASS | 0x09)

#define USB_DT_CS_DEVICE                (USB_TYPE_CLASS | USB_DT_DEVICE)
#define USB_DT_CS_CONFIG                (USB_TYPE_CLASS | USB_DT_CONFIG)
#define USB_DT_CS_STRING                (USB_TYPE_CLASS | USB_DT_STRING)
#define USB_DT_CS_INTERFACE             (USB_TYPE_CLASS | USB_DT_INTERFACE)
#define USB_DT_CS_ENDPOINT              (USB_TYPE_CLASS | USB_DT_ENDPOINT)


/*
 * Descriptor sizes per descriptor type
 */
#define USB_DT_DEVICE_SIZE              18
#define USB_DT_CONFIG_SIZE              9
#define USB_DT_INTERFACE_SIZE           9
#define USB_DT_ENDPOINT_SIZE            7
#define USB_DT_ENDPOINT_AUDIO_SIZE      9       /* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE          7
#define USB_DT_HID_SIZE                 9

/*
 * Endpoints
 */
#define USB_ENDPOINT_NUMBER_MASK        0x0f    /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK           0x80

#define USB_ENDPOINT_XFERTYPE_MASK      0x03    /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL       0
#define USB_ENDPOINT_XFER_ISOC          1
#define USB_ENDPOINT_XFER_BULK          2
#define USB_ENDPOINT_XFER_INT           3

/*
 * USB Packet IDs (PIDs)
 */
#define USB_PID_UNDEF_0                 0xf0
#define USB_PID_OUT                     0xe1
#define USB_PID_ACK                     0xd2
#define USB_PID_DATA0                   0xc3
#define USB_PID_PING                    0xb4     /* USB 2.0 */
#define USB_PID_SOF                     0xa5
#define USB_PID_NYET                    0x96     /* USB 2.0 */
#define USB_PID_DATA2                   0x87     /* USB 2.0 */
#define USB_PID_SPLIT                   0x78     /* USB 2.0 */
#define USB_PID_IN                      0x69
#define USB_PID_NAK                     0x5a
#define USB_PID_DATA1                   0x4b
#define USB_PID_PREAMBLE                0x3c     /* Token mode */
#define USB_PID_ERR                     0x3c     /* USB 2.0: handshake mode */
#define USB_PID_SETUP                   0x2d
#define USB_PID_STALL                   0x1e
#define USB_PID_MDATA                   0x0f     /* USB 2.0 */

/*
 * Standard requests
 */
#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C

/*
 * HID requests
 */
#define USB_REQ_GET_REPORT              0x01
#define USB_REQ_GET_IDLE                0x02
#define USB_REQ_GET_PROTOCOL            0x03
#define USB_REQ_SET_REPORT              0x09
#define USB_REQ_SET_IDLE                0x0A
#define USB_REQ_SET_PROTOCOL            0x0B


/*!< device descriptor structure  \hideinitializer   */
typedef struct __attribute__((__packed__)) {
    uint8_t  requesttype;      /*!< Characteristics of request \hideinitializer  */
    uint8_t  request;          /*!< Specific request \hideinitializer            */
    uint16_t value;            /*!< Word-sized field that varies according to request \hideinitializer */
    uint16_t index;            /*!< Word-sized field that varies according to request; typically used to pass an index or offset \hideinitializer */
    uint16_t length;           /*!< Number of bytes to transfer if there is a Data stage \hideinitializer */
} DEV_REQ_T;


/*
 * This is a USB device descriptor.
 *
 * USB device information
 */

/* Everything but the endpoint maximums are arbitrary */
#define USB_MAXCONFIG           8
#define USB_ALTSETTINGALLOC     16
#define USB_MAXALTSETTING       128  /* Hard limit */
#define USB_MAXINTERFACES       32
#define USB_MAXENDPOINTS        32

/* All standard descriptors have these 2 fields in common */
typedef struct __attribute__((__packed__)) usb_descriptor_header {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
} USB_DESC_HDR_T;


/*
 * declaring data structures presented before their definition met
 */
struct usb_device;
struct urb_t;


/*-----------------------------------------------------------------------------------
 *  USB device descriptor
 */
/*!< Device descriptor structure \hideinitializer           */
typedef struct __attribute__((__packed__)) usb_device_descriptor {
    uint8_t  bLength;          /*!< Length of device descriptor \hideinitializer           */
    uint8_t  bDescriptorType;  /*!< Device descriptor type \hideinitializer                */
    uint16_t bcdUSB;           /*!< USB version number \hideinitializer                    */
    uint8_t  bDeviceClass;     /*!< Device class code \hideinitializer                     */
    uint8_t  bDeviceSubClass;  /*!< Device subclass code \hideinitializer                  */
    uint8_t  bDeviceProtocol;  /*!< Device protocol code \hideinitializer                  */
    uint8_t  bMaxPacketSize0;  /*!< Maximum packet size of control endpoint \hideinitializer */
    uint16_t idVendor;         /*!< Vendor ID \hideinitializer                             */
    uint16_t idProduct;        /*!< Product ID \hideinitializer                            */
    uint16_t bcdDevice;        /*!< Device ID \hideinitializer                             */
    uint8_t  iManufacturer;    /*!< Manufacture description string ID \hideinitializer     */
    uint8_t  iProduct;         /*!< Product description string ID \hideinitializer         */
    uint8_t  iSerialNumber;    /*!< Serial number description string ID \hideinitializer   */
    uint8_t  bNumConfigurations; /*!< Total number of configurations \hideinitializer      */
} USB_DEV_DESC_T;                       /*!< Device descriptor structure \hideinitializer           */


/*-----------------------------------------------------------------------------------
 *  USB endpoint descriptor
 */
/*! Endpoint descriptor structure \hideinitializer         */
typedef struct __attribute__((__packed__)) usb_endpoint_descriptor {
    uint8_t  bLength;          /*!< Length of endpoint descriptor \hideinitializer         */
    uint8_t  bDescriptorType;  /*!< Descriptor type \hideinitializer                       */
    uint8_t  bEndpointAddress; /*!< Endpoint address \hideinitializer                      */
    uint8_t  bmAttributes;     /*!< Endpoint attribute \hideinitializer                    */
    uint16_t wMaxPacketSize;   /*!< Maximum packet size \hideinitializer                   */
    uint8_t  bInterval;        /*!< Synchronous transfer interval \hideinitializer         */
    uint8_t  bRefresh;         /*!< Refresh \hideinitializer                               */
    uint8_t  bSynchAddress;    /*!< Sync address \hideinitializer                          */
} USB_EP_DESC_T;                        /*! Endpoint descriptor structure \hideinitializer         */


/*-----------------------------------------------------------------------------------
 *  USB interface descriptor
 */
/*! Interface descriptor structure \hideinitializer        */
typedef struct __attribute__((__packed__)) usb_interface_descriptor {
    uint8_t  bLength;          /*!< Length of interface descriptor \hideinitializer        */
    uint8_t  bDescriptorType;  /*!< Descriptor type \hideinitializer                       */
    uint8_t  bInterfaceNumber; /*!< Interface number \hideinitializer                      */
    uint8_t  bAlternateSetting;/*!< Alternate setting number \hideinitializer              */
    uint8_t  bNumEndpoints;    /*!< Number of endpoints \hideinitializer                   */
    uint8_t  bInterfaceClass;  /*!< Interface class code \hideinitializer                  */
    uint8_t  bInterfaceSubClass; /*!< Interface subclass code \hideinitializer             */
    uint8_t  bInterfaceProtocol; /*!< Interface protocol code \hideinitializer             */
    uint8_t  iInterface;       /*!< Interface ID \hideinitializer                          */
    USB_EP_DESC_T *endpoint;            /*!< Endpoint descriptor \hideinitializer                   */
} USB_IF_DESC_T;                        /*! Interface descriptor structure \hideinitializer        */


/*-----------------------------------------------------------------------------------
 *  Configuration descriptor
 */
/*! Configuration descriptor structure \hideinitializer    */
typedef struct __attribute__((__packed__)) usb_config_descriptor {
    uint8_t   bLength;         /*!< Length of configuration descriptor \hideinitializer    */
    uint8_t   bDescriptorType; /*!< Descriptor type \hideinitializer                       */
    uint16_t  wTotalLength;    /*!< Total length of this configuration \hideinitializer    */
    uint8_t   bNumInterfaces;  /*!< Total number of interfaces \hideinitializer            */
    uint8_t   bConfigurationValue; /*!< Configuration descriptor number \hideinitializer   */
    uint8_t   iConfiguration;  /*!< String descriptor ID \hideinitializer                  */
    uint8_t   bmAttributes;    /*!< Configuration characteristics \hideinitializer         */
    uint8_t   MaxPower;        /*!< Maximum power consumption \hideinitializer             */
} USB_CONFIG_DESC_T;                    /*! Configuration descriptor structure \hideinitializer    */


/* String descriptor */
typedef struct __attribute__((__packed__)) usb_string_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wData[1];
} USB_STR_DESC_T;


/*
 * Device table entry for "new style" table-driven USB drivers.
 * User mode code can read these tables to choose which modules to load.
 * Declare the table as __devinitdata, and as a MODULE_DEVICE_TABLE.
 *
 * With a device table provide bind() instead of probe().  Then the
 * third bind() parameter will point to a matching entry from this
 * table.  (Null value reserved.)
 *
 * Terminate the driver's table with an all-zeroes entry.
 * Init the fields you care about; zeroes are not used in comparisons.
 */
#define USB_DEVICE_ID_MATCH_VENDOR              0x0001
#define USB_DEVICE_ID_MATCH_PRODUCT             0x0002
#define USB_DEVICE_ID_MATCH_DEV_LO              0x0004
#define USB_DEVICE_ID_MATCH_DEV_HI              0x0008
#define USB_DEVICE_ID_MATCH_DEV_CLASS           0x0010
#define USB_DEVICE_ID_MATCH_DEV_SUBCLASS        0x0020
#define USB_DEVICE_ID_MATCH_DEV_PROTOCOL        0x0040
#define USB_DEVICE_ID_MATCH_INT_CLASS           0x0080
#define USB_DEVICE_ID_MATCH_INT_SUBCLASS        0x0100
#define USB_DEVICE_ID_MATCH_INT_PROTOCOL        0x0200

#define USB_DEVICE_ID_MATCH_DEVICE              (USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT)
#define USB_DEVICE_ID_MATCH_DEV_RANGE           (USB_DEVICE_ID_MATCH_DEV_LO | USB_DEVICE_ID_MATCH_DEV_HI)
#define USB_DEVICE_ID_MATCH_DEVICE_AND_VERSION  (USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_DEV_RANGE)
#define USB_DEVICE_ID_MATCH_DEV_INFO \
        (USB_DEVICE_ID_MATCH_DEV_CLASS | USB_DEVICE_ID_MATCH_DEV_SUBCLASS | USB_DEVICE_ID_MATCH_DEV_PROTOCOL)
#define USB_DEVICE_ID_MATCH_INT_INFO \
        (USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL)

/* Some useful macros */
#define USB_DEVICE(vend,prod,info) \
        { USB_DEVICE_ID_MATCH_DEVICE, vend, prod, 0, 0, 0, 0, 0, 0, 0, 0, info }

#define USB_DEVICE_VER(vend,prod,lo,hi,info) \
        { USB_DEVICE_ID_MATCH_DEVICE_AND_VERSION, vend, prod, lo, hi, 0, 0, 0, 0, 0, 0, info }

#define USB_DEVICE_INFO(cl,sc,pr,info) \
        { USB_DEVICE_ID_MATCH_DEV_INFO, 0, 0, 0, 0, cl, sc, pr, 0, 0, 0, info }

#define USB_INTERFACE_INFO(cl,sc,pr,info) \
        { USB_DEVICE_ID_MATCH_INT_INFO, 0, 0, 0, 0, 0, 0, 0, cl, sc, pr, info }

typedef struct usb_device_id {
    /* This bitmask is used to determine which of the following fields
     * are to be used for matching.
     */
    uint16_t  match_flags;

    /*
     * vendor/product codes are checked, if vendor is non-zero
     * Range is for device revision (bcdDevice), inclusive;
     * zero values here mean range isn't considered
     */
    uint16_t  idVendor;
    uint16_t  idProduct;
    uint16_t  bcdDevice_lo;
    uint16_t  bcdDevice_hi;

    /*
     * if device class != 0, these can be match criteria;
     * but only if this bDeviceClass value is non-zero
     */
    uint8_t   bDeviceClass;
    uint8_t   bDeviceSubClass;
    uint8_t   bDeviceProtocol;

    /*
     * if interface class != 0, these can be match criteria;
     * but only if this bInterfaceClass value is non-zero
     */
    uint8_t   bInterfaceClass;
    uint8_t   bInterfaceSubClass;
    uint8_t   bInterfaceProtocol;

    /*
     * for driver's use; not involved in driver matching.
     */
    uint32_t  driver_info;  // if 1: have next id struct data, 0: no next id struct data
} USB_DEV_ID_T;


typedef struct usb_driver {
    const char  *name;
    int   (*probe)(struct usb_device *dev, USB_IF_DESC_T *ifd, const USB_DEV_ID_T *id);
    void  (*disconnect)(struct usb_device *);
    const USB_DEV_ID_T  *id_table;
    void (*suspend)(struct usb_device *dev);
    void (*resume)(struct usb_device *dev);
    USB_LIST_T  driver_list;
} USB_DRIVER_T;


/*----------------------------------------------------------------------------*
 * New USB Structures                                                         *
 *----------------------------------------------------------------------------*/
/*
 * urb->transfer_flags:
 */
#define USB_DISABLE_SPD     0x0001
#define URB_SHORT_NOT_OK    USB_DISABLE_SPD
#define USB_ISO_ASAP        0x0002
#define USB_ASYNC_UNLINK    0x0008
#define USB_QUEUE_BULK      0x0010
#define USB_NO_FSBR         0x0020
#define USB_ZERO_PACKET     0x0040  // Finish bulk OUTs always with zero length packet
#define URB_NO_INTERRUPT    0x0080  /* HINT: no non-error interrupt needed */
/* ... less overhead for QUEUE_BULK */
#define USB_TIMEOUT_KILLED  0x1000  // only set by HCD!

#define URB_ZERO_PACKET     USB_ZERO_PACKET
#define URB_ISO_ASAP        USB_ISO_ASAP


struct ohci_ed_t;
struct ohci_td_t;

typedef struct {
    struct ohci_ed_t    *ed;
    uint16_t            length;       /* number of tds associated with this request */
    uint16_t            td_cnt;       /* number of tds already serviced */
    int                 state;
    struct ohci_td_t    *td[MAX_TD_PER_OHCI_URB];     /* list pointer to all corresponding TDs associated with this request */
}   URB_PRIV_T;


/*-----------------------------------------------------------------------------------
 *  URB isochronous descriptor structure
 */
typedef struct iso_pkt_t {              /*! Isochronous packet structure \hideinitializer          */
    uint32_t  offset;                   /*!< Start offset in transfer buffer \hideinitializer       */
    uint32_t  length;                   /*!< Length in transfer buffer \hideinitializer             */
    uint32_t  actual_length;            /*!< Actual transfer length \hideinitializer                */
    int     status;                     /*!< Transfer status \hideinitializer                       */
}   ISO_PACKET_DESCRIPTOR_T;            /*! Isochronous packet structure \hideinitializer          */

/*-----------------------------------------------------------------------------------
 *  USB Request Block (URB) structure
 */
typedef struct urb_t {                  /*! URB structure \hideinitializer                         */
    URB_PRIV_T  urb_hcpriv;             /*!< private data for host controller \hideinitializer      */
    USB_LIST_T  urb_list;               /*!< list pointer to all active urbs \hideinitializer       */
    struct urb_t  *next;                /*!< pointer to next URB \hideinitializer                   */
    struct usb_device  *dev;            /*!< pointer to associated USB device \hideinitializer      */
    uint32_t    pipe;                   /*!< pipe information \hideinitializer                      */
    int         status;                 /*!< returned status \hideinitializer                       */
    uint32_t    transfer_flags;         /*!< USB_DISABLE_SPD | USB_ISO_ASAP | etc. \hideinitializer */
    void        *transfer_buffer;       /*!< associated data buffer \hideinitializer                */
    int         transfer_buffer_length; /*!< data buffer length \hideinitializer                    */
    int         actual_length;          /*!< actual data buffer length \hideinitializer             */
    uint8_t     *setup_packet;          /*!< setup packet (control only) \hideinitializer           */
    int         start_frame;            /*!< start frame (iso/irq only) \hideinitializer            */
    int         number_of_packets;      /*!< number of packets in this request (iso) \hideinitializer */
    int         interval;               /*!< polling interval (irq only) \hideinitializer           */
    int         error_count;            /*!< number of errors in this transfer (iso only) \hideinitializer */
    int         timeout;                /*!< timeout (in jiffies) \hideinitializer                  */
    void        *context;               /*!< USB Driver internal used \hideinitializer              */
    void (*complete)(struct urb_t *);   /*!< USB transfer complete callback function \hideinitializer */
    ISO_PACKET_DESCRIPTOR_T  iso_frame_desc[8]; /*!< isochronous transfer descriptor \hideinitializer */
}   URB_T;                              /*! URB structure \hideinitializer                         */


#define FILL_CONTROL_URB(a,aa,b,c,d,e,f,g) \
    do {\
        (a)->dev=aa;\
        (a)->pipe=b;\
        (a)->setup_packet=c;\
        (a)->transfer_buffer=d;\
        (a)->transfer_buffer_length=e;\
        (a)->complete=f;\
        (a)->context=g;\
    } while (0)


#define FILL_BULK_URB(a,aa,b,c,d,e,f) \
    do {\
        (a)->dev=aa;\
        (a)->pipe=b;\
        (a)->transfer_buffer=c;\
        (a)->transfer_buffer_length=d;\
        (a)->complete=e;\
        (a)->context=f;\
    } while (0)


#define FILL_INT_URB(a,aa,b,c,d,e,f,g) \
    do {\
        (a)->dev=aa;\
        (a)->pipe=b;\
        (a)->transfer_buffer=c;\
        (a)->transfer_buffer_length=d;\
        (a)->complete=e;\
        (a)->context=f;\
        (a)->interval=g;\
        (a)->start_frame=-1;\
    } while (0)


#define FILL_CONTROL_URB_TO(a,aa,b,c,d,e,f,g,h) \
    do {\
        (a)->dev=aa;\
        (a)->pipe=b;\
        (a)->setup_packet=c;\
        (a)->transfer_buffer=d;\
        (a)->transfer_buffer_length=e;\
        (a)->complete=f;\
        (a)->context=g;\
        (a)->timeout=h;\
    } while (0)


#define FILL_BULK_URB_TO(a,aa,b,c,d,e,f,g) \
    do {\
        (a)->dev=aa;\
        (a)->pipe=b;\
        (a)->transfer_buffer=c;\
        (a)->transfer_buffer_length=d;\
        (a)->complete=e;\
        (a)->context=f;\
        (a)->timeout=g;\
    } while (0)


typedef struct usb_operations {
    int (*allocate)(struct usb_device *);
    int (*deallocate)(struct usb_device *);
    int (*get_frame_number) (struct usb_device *usb_dev);
    int (*submit_urb)(URB_T *urb);
    int (*unlink_urb)(URB_T *urb);
} USB_OP_T;


typedef struct usb_bus {
    USB_OP_T  *op;                      /* Operations (specific to the HC) */
    struct usb_device  *root_hub;       /* Root hub */
    void   *hcpriv;                     /* Host Controller private data */
}   USB_BUS_T;


#define USB_MAXCHILDREN         (4)     /* This is arbitrary */

typedef struct ep_info_t {
    uint8_t     cfgno;
    uint8_t     ifnum;
    uint8_t     altno;
    uint8_t     bEndpointAddress;
    uint8_t     bmAttributes;
    uint8_t     bInterval;
    short       wMaxPacketSize;
}   EP_INFO_T;


#define USB_SPEED_UNKNOWN           0
#define USB_SPEED_LOW               1
#define USB_SPEED_FULL              2
#define USB_SPEED_HIGH              3

/*-----------------------------------------------------------------------------------
 *  USB device structure
 */
typedef struct usb_device {             /*! USB device structure  \hideinitializer                 */
    USB_DEV_DESC_T  descriptor;         /*!< Device descriptor. \hideinitializer                    */

    int     devnum;                     /*!< Device number on USB bus \hideinitializer              */
    int     slow;                       /*!< Is slow device. \hideinitializer                       */
    int     speed;                      /*!< Device speed. \hideinitializer                         */

    uint32_t    toggle[2];              /*!< toggle bit ([0] = IN, [1] = OUT) \hideinitializer      */
    uint32_t    halted[2];              /*!< endpoint halts;  one bit per endpoint # & direction; \hideinitializer */
    /*!< [0] = IN, [1] = OUT \hideinitializer */

    struct usb_device  *parent;         /*!< parent device \hideinitializer  \hideinitializer       */
    int                 hub_port;       /*!< The hub port that this device connected on \hideinitializer */
    USB_BUS_T  *bus;                    /*!< Bus we're part of \hideinitializer                     */

    signed char act_config;             /*!< Active configuration number \hideinitializer            */
    char        act_iface;              /*!< Active interface number \hideinitializer               */
    char        iface_alternate;        /*!< Active interface alternate setting \hideinitializer    */

    EP_INFO_T   ep_list[MAX_ENDPOINTS]; /*!< Endpoint list \hideinitializer                         */
    int         ep_list_cnt;            /*!< Total number of ep_list[] \hideinitializer             */

    int         have_langid;            /*!< whether string_langid is valid yet \hideinitializer    */
    int         string_langid;          /*!< language ID for strings \hideinitializer               */

    void        *hcpriv;                /*!< Host Controller private data \hideinitializer          */

    /*
     * Child devices - these can be either new devices
     * (if this is a hub device), or different instances
     * of this same device.
     *
     * Each instance needs its own set of data structures.
     */

    int     maxchild;                  /*!< Number of ports if hub \hideinitializer                */
    struct usb_device  *children[USB_MAXCHILDREN]; /*!< Child device list \hideinitializer         */

    USB_DRIVER_T    *driver[MAX_DRIVER_PER_DEV]; /*!< Driver list \hideinitializer                 */
    int             driver_cnt;        /*!< Total number of driver list \hideinitializer           */

} USB_DEV_T;                           /*! USB device structure  \hideinitializer                 */


/*@}*/ /* end of group NUC472_442_USBH_EXPORTED_STRUCT */


/*
 * Calling this entity a "pipe" is glorifying it. A USB pipe
 * is something embarrassingly simple: it basically consists
 * of the following information:
 *  - device number (7 bits)
 *  - endpoint number (4 bits)
 *  - current Data0/1 state (1 bit)
 *  - direction (1 bit)
 *  - speed (1 bit)
 *  - max packet size (2 bits: 8, 16, 32 or 64) [Historical; now gone.]
 *  - pipe type (2 bits: control, interrupt, bulk, isochronous)
 *
 * That's 18 bits. Really. Nothing more. And the USB people have
 * documented these eighteen bits as some kind of glorious
 * virtual data structure.
 *
 * Let's not fall in that trap. We'll just encode it as a simple
 * uint32_t. The encoding is:
 *
 *  - max size:         bits 0-1        (00 = 8, 01 = 16, 10 = 32, 11 = 64) [Historical; now gone.]
 *  - direction:        bit 7           (0 = Host-to-Device [Out], 1 = Device-to-Host [In])
 *  - device:           bits 8-14
 *  - endpoint:         bits 15-18
 *  - Data0/1:          bit 19
 *  - speed:            bit 26          (0 = Full, 1 = Low Speed)
 *  - pipe type:        bits 30-31      (00 = isochronous, 01 = interrupt, 10 = control, 11 = bulk)
 *
 * Why? Because it's arbitrary, and whatever encoding we select is really
 * up to us. This one happens to share a lot of bit positions with the UHCI
 * specification, so that much of the uhci driver can just mask the bits
 * appropriately.
 */

#define PIPE_ISOCHRONOUS                0
#define PIPE_INTERRUPT                  1
#define PIPE_CONTROL                    2
#define PIPE_BULK                       3

#define usb_packetid(pipe)      (((pipe) & USB_DIR_IN) ? USB_PID_IN : USB_PID_OUT)

#define usb_pipeout(pipe)       ((((pipe) >> 7) & 1) ^ 1)
#define usb_pipein(pipe)        (((pipe) >> 7) & 1)
#define usb_pipedevice(pipe)    (((pipe) >> 8) & 0x7f)
#define usb_pipe_endpdev(pipe)  (((pipe) >> 8) & 0x7ff)
#define usb_pipeendpoint(pipe)  (((pipe) >> 15) & 0xf)
#define usb_pipedata(pipe)      (((pipe) >> 19) & 1)
#define usb_pipeslow(pipe)      (((pipe) >> 26) & 1)
#define usb_pipetype(pipe)      (((pipe) >> 30) & 3)
#define usb_pipeisoc(pipe)      (usb_pipetype((pipe)) == PIPE_ISOCHRONOUS)
#define usb_pipeint(pipe)       (usb_pipetype((pipe)) == PIPE_INTERRUPT)
#define usb_pipecontrol(pipe)   (usb_pipetype((pipe)) == PIPE_CONTROL)
#define usb_pipebulk(pipe)      (usb_pipetype((pipe)) == PIPE_BULK)

#define PIPE_DEVEP_MASK         0x0007ff00

/* The D0/D1 toggle bits */
#define usb_gettoggle(dev, ep, out) (((dev)->toggle[out] >> ep) & 1)
#define usb_dotoggle(dev, ep, out)  ((dev)->toggle[out] ^= (1 << ep))
#define usb_settoggle(dev, ep, out, bit) ((dev)->toggle[out] = ((dev)->toggle[out] & ~(1 << ep)) | ((bit) << ep))

/* Endpoint halt control/status */
#define usb_endpoint_out(ep_dir)        (((ep_dir >> 7) & 1) ^ 1)
#define usb_endpoint_halt(dev, ep, out) ((dev)->halted[out] |= (1 << (ep)))
#define usb_endpoint_running(dev, ep, out) ((dev)->halted[out] &= ~(1 << (ep)))
#define usb_endpoint_halted(dev, ep, out) ((dev)->halted[out] & (1 << (ep)))

static __inline uint32_t __create_pipe(USB_DEV_T *dev, uint32_t endpoint)
{
    return (dev->devnum << 8) | (endpoint << 15) | (dev->slow << 26);
}

static __inline uint32_t __default_pipe(USB_DEV_T *dev)
{
    return (dev->slow << 26);
}

/* Create various pipes... */
#define usb_sndctrlpipe(dev,endpoint)   (0x80000000 | __create_pipe(dev,endpoint))
#define usb_rcvctrlpipe(dev,endpoint)   (0x80000000 | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndisocpipe(dev,endpoint)   (0x00000000 | __create_pipe(dev,endpoint))
#define usb_rcvisocpipe(dev,endpoint)   (0x00000000 | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndbulkpipe(dev,endpoint)   (0xC0000000 | __create_pipe(dev,endpoint))
#define usb_rcvbulkpipe(dev,endpoint)   (0xC0000000 | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndintpipe(dev,endpoint)    (0x40000000 | __create_pipe(dev,endpoint))
#define usb_rcvintpipe(dev,endpoint)    (0x40000000 | __create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_snddefctrl(dev)             (0x80000000 | __default_pipe(dev))
#define usb_rcvdefctrl(dev)             (0x80000000 | __default_pipe(dev) | USB_DIR_IN)


extern USB_LIST_T  usb_driver_list;
extern USB_LIST_T  usb_bus_list;

/*-------------------------------------------------------------------------
 *   Global Variables
 *-------------------------------------------------------------------------*/
extern USB_BUS_T    g_ohci_bus;


#ifdef __cplusplus
extern "C"
{
#endif

extern int32_t  USBH_Open(void);
extern int32_t  USBH_ProcessHubEvents(void);
extern URB_T  * USBH_AllocUrb(void);
extern void     USBH_FreeUrb(URB_T *);
extern int32_t  USBH_SubmitUrb(URB_T *urb);
extern int32_t  USBH_UnlinkUrb(URB_T *urb);
extern int32_t  USBH_SendCtrlMsg(USB_DEV_T *dev, uint32_t pipe, uint8_t request, uint8_t requesttype,  uint16_t value, uint16_t index, void *data, uint16_t size, int timeout);
extern int32_t  USBH_SendBulkMsg(USB_DEV_T *usb_dev, uint32_t pipe, void *data, int len, int *actual_length, int timeout);
extern int32_t  USBH_RegisterDriver(USB_DRIVER_T *new_driver);
extern int32_t  USBH_GetDescriptor(USB_DEV_T *dev, uint8_t type, uint8_t index, void *buf, int size);
extern int32_t  USBH_SetConfiguration(USB_DEV_T *dev, int configuration);
extern int32_t  USBH_SetInterface(USB_DEV_T *dev, char interface, char alternate);
extern int32_t  USBH_ClearHalt(USB_DEV_T *dev, int pipe);
extern int32_t  USBH_Suspend(void);
extern int32_t  USBH_Resume(void);
extern int32_t  USBH_Close(void);


/* USB core library internal APIs */
extern void usbh_init_memory(void);
extern int  usbh_init_ohci(void);
extern int  usbh_init_hub_driver(void);
extern void ohci_irq(void);
extern void ohci_int_timer_do(int);
extern void usbh_mdelay(uint32_t msec);

extern void usbh_connect_device(USB_DEV_T *dev);
extern void usbh_disconnect_device(USB_DEV_T **pdev);
extern int  usbh_settle_new_device(USB_DEV_T *dev);

extern int usb_maxpacket(USB_DEV_T *dev, uint32_t pipe, int out);

extern USB_DEV_T  *usbh_alloc_device(USB_DEV_T *parent, USB_BUS_T *bus);
extern void  usbh_free_device(USB_DEV_T *dev);

extern struct ohci_ed_t * ohci_alloc_ed(void);
extern void ohci_free_ed(struct ohci_ed_t *ed_p);
extern struct ohci_td_t * ohci_alloc_td(USB_DEV_T *dev);
extern void ohci_free_td(struct ohci_td_t *td_p);
extern void ohci_free_dev_td(USB_DEV_T *dev);
extern struct ohci_itd_t * ohci_alloc_idd(void);
extern void ohci_free_itd(struct ohci_itd_t *itd_p);
extern void usbh_free_dev_urbs(USB_DEV_T *dev);
extern int  usbh_translate_string(USB_DEV_T *dev, int index, char *buf, int size);

extern void  usbh_dump_device_descriptor(USB_DEV_DESC_T *desc);
extern void  usbh_dump_config_descriptor(USB_CONFIG_DESC_T *desc);
extern void  usbh_dump_iface_descriptor(USB_IF_DESC_T *desc);
extern void  usbh_dump_ep_descriptor(USB_EP_DESC_T *desc);
extern void  usbh_dump_urb(URB_T *purb);
extern void  usbh_print_usb_string(USB_DEV_T *dev, char *id, int index);

#ifdef __cplusplus
}
#endif


#endif  /* _USB_CORE_H_ */

/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/
