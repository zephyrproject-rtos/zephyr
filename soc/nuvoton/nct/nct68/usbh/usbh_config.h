/**************************************************************************//**
 * @file     usbh_config.h
 * @brief    USB Host core configuration file
 *
 * @note
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef  _USB_CONFIG_H_
#define  _USB_CONFIG_H_


#ifdef __ICCARM__
#define __inline    inline
#endif


#include <stdio.h>
#include <zephyr/kernel.h>

/*
 *  Debug messages...
 */
//#define USB_DEBUG                       /*!< Enable debug message \hideinitializer      */
//#define USB_VERBOSE_DEBUG
//#define DUMP_DEV_DESCRIPTORS


/*
 *  Static Memory Settings...
 */
#define DEV_MAX_NUM             8       /*!< Maximum number of connected devices \hideinitializer       */
#define URB_MAX_NUM             15      /*!< Maximum number of URBs in memory pool \hideinitializer     */
#define ED_MAX_NUM              15      /*!< Maximum number of OHCI EDs in memory pool \hideinitializer */
#define TD_MAX_NUM              32      /*!< Maximum number of OHCI TDs in memory pool \hideinitializer */

#define MAX_ENDPOINTS           16      /*!< Maximum number of endpoints per device \hideinitializer    */
#define MAX_DRIVER_PER_DEV      3       /*!< Maximum number of drivers for a device \hideinitializer    */
#define MAX_TD_PER_OHCI_URB     8       /*!< Maximum number of OHCI TDs per URB \hideinitializer        */
#define MAX_HUB_DEVICE          2       /*!< Maximum number of connected Hub devices \hideinitializer   */


#define ISO_FRAME_COUNT         4       /*!< Transfer frames per Isohronous TD \hideinitializer     */
#define OHCI_ISO_DELAY          8       /*!< Delay isochronous transfer frame time \hideinitializer     */

/*
 * Class driver support...
 */
#define SUPPORT_HUB_CLASS               /*!< Support Hub driver \hideinitializer      */


/*
 *  Debug/Warning/Information to be printed on console or not
 */
#define USB_error				printk
#ifdef USB_DEBUG
#define USB_debug               printk
#else
#define USB_debug(...)
#endif

#ifdef USB_VERBOSE_DEBUG
#define USB_warning             printk
#define USB_info                printk
#else
#define USB_warning(...)
#define USB_info(...)
#endif


/*---  CPU clock speed ---*/
#define HZ                      (84)

#define USB_SWAP16(x)           (((x>>8)&0xff)|((x&0xff)<<8))
#define USB_SWAP32(x)           (((x>>24)&0xff)|((x>>8)&0xff00)|((x&0xff00)<<8)|((x&0xff)<<24))


/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/

#endif  /* _USB_CONFIG_H_ */


