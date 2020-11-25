/*
 * Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
 *
 * SPDX-License-Identifier: APACHE-2.0
 */

#ifndef USBH_CFG_MODULE_PRESENT
#define USBH_CFG_MODULE_PRESENT

/*  Number of host controllers                          */
/*  The number of host controllers that will be ...     */
/*  ... added to the stack.                             */
#define USBH_CFG_MAX_NBR_HC                               1u

/*  Maximum number of class driver supported            */
/*  The maximum number of class driver ...              */
/*  ... (MSC, HID, etc) you will use and add to the ... */
/*  ... core. The hub class is mandatory and must be ...*/
/*  ... accounted in the total number.                  */
#define USBH_CFG_MAX_NBR_CLASS_DRVS                       4u

/*  Maximum number of devices                           */
/*  The maximum number of devices that the USB host ... */
/*  ... stack can accept. This must be < 127.           */
#define USBH_CFG_MAX_NBR_DEVS                            10u

/*  Maximum number of configurations per device         */
/*  The maximum number of USB configurations that ...   */
/*  ... each device can contain. Most of the time, ...  */
/*  ... devices will have only one configuration.       */
#define USBH_CFG_MAX_NBR_CFGS                             1u

/*  Maximum number of interfaces per USB configuration. */
/*  The maximum number of interface per USB ...         */
/*  ... configurations that each device can contain. ...*/
/*  ... Simple USB devices will generally have only ... */
/*  ... one interface. If you plan to use composite ... */
/*  ... devices, you should increase this value.        */
#define USBH_CFG_MAX_NBR_IFS                              2u

/*  Max number of endpoints per interface.              */
/*  The maximum number of endpoints that an interface...*/
/*  ... can contain. The number of endpoints greatly ...*/
/*  ... depends on the device's class, but should ...   */
/*  ... generally be around two or three.               */
#define USBH_CFG_MAX_NBR_EPS                              3u

/*  Maximum configuration descriptor length             */
/*  The maximum length of the buffer that is used to ...*/
/*  ... hold the USB configuration descriptor.          */
#define USBH_CFG_MAX_CFG_DATA_LEN                       256u

/*  Maximum length of string descriptors                */
/*  The maximum length for string descriptors.          */
#define USBH_CFG_MAX_STR_LEN                            256u

/*  Timeout for standard request (ms)                   */
/*  Timeout in ms for standard requests to complete.    */
#define USBH_CFG_STD_REQ_TIMEOUT                       5000u

/*  Number of retries on stand requests fail            */
/*  Number of times the stack should retry to ...       */
/*  ... execute a standard request when it failed.      */
#define USBH_CFG_STD_REQ_RETRY                            3u

/*  Maximum number of isochronous descriptor            */
/*  The maximum number of isochronous descriptor ...    */
/*  ... that will be shared between all isochronous ... */
/*  ... endpoints.                                      */
#define USBH_CFG_MAX_ISOC_DESC                            1u

/*  Maximum number of extra urb                         */
/*  The maximum number of extra urb used for streaming. */
#define USBH_CFG_MAX_EXTRA_URB_PER_DEV                    1u

/*  Maximum number of USB hub                           */
/*  The maximum number of external and root hub that ...*/
/*  ... can be connected.                               */
#define USBH_CFG_MAX_HUBS                                 5u

/*  Maximum number of port per USB hub                  */
/*  The maximum number of supported ports per hub. ...  */
/*  ... See note #3.                                    */
#define USBH_CFG_MAX_HUB_PORTS                            7u

/* Maximum number of connection retries                 */
/* The maximum number of reset and connection ...       */
/* ... retries before tearing down a device             */
#define USBH_CFG_MAX_NUM_DEV_RECONN                       3u

/*
 * MASS STORAGE CLASS (MSC) CONFIGURATION
 */

/*  Maximum number of MSC devices                       */
/*  The maximum number of MSC devices that can be ...   */
/*  ... connected at the same time.                     */
#define USBH_MSC_CFG_MAX_DEV                              2u

#endif

