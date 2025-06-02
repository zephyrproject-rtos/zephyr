/******************************************************************************
 * @file     usbh_ohci.c
 * @brief    MCU USB Host OHCI driver
 *
 * @note
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "hal_ohci.h"


#include "usbh_core.h"
#include "usbh_hub.h"
#include "usbh_ohci.h"


#ifdef __ICCARM__
    #pragma data_alignment=32
    static ED_T  g_ed_pool[ED_MAX_NUM];
    #pragma data_alignment=32
    static TD_T  g_td_pool[TD_MAX_NUM];
    static URB_T  g_urb_pool[URB_MAX_NUM];
#elif defined ( __GNUC__ )
    static ED_T  g_ed_pool[ED_MAX_NUM] __attribute__ ((aligned (32)));
    static TD_T  g_td_pool[TD_MAX_NUM] __attribute__ ((aligned (32)));
    static URB_T  g_urb_pool[URB_MAX_NUM] __attribute__ ((aligned (32)));
#elif defined ( __CC_ARM )
    static __align(32) ED_T  g_ed_pool[ED_MAX_NUM];
    static __align(32) TD_T  g_td_pool[TD_MAX_NUM];
    static __align(32) URB_T  g_urb_pool[URB_MAX_NUM];
#endif

USB_DEV_T g_dev_pool[DEV_MAX_NUM];
static USB_HUB_T g_hub_pool[MAX_HUB_DEVICE];
static uint8_t  ed_alloc_mark[ED_MAX_NUM];
static uint8_t  td_alloc_mark[TD_MAX_NUM];
USB_DEV_T *     td_alloc_dev[TD_MAX_NUM];
uint8_t  dev_alloc_mark[DEV_MAX_NUM];
static uint8_t  urb_alloc_mark[URB_MAX_NUM];
static uint8_t  hub_alloc_mark[MAX_HUB_DEVICE];

int _td_cnt, _ed_cnt;

void usbh_init_memory()
{
    int     i;

    for (i = 0; i < ED_MAX_NUM; i++)
        ed_alloc_mark[i] = 0;
    _ed_cnt = ED_MAX_NUM;

    for (i = 0; i < TD_MAX_NUM; i++)
        td_alloc_mark[i] = 0;
    _td_cnt = TD_MAX_NUM;

    for (i = 0; i < DEV_MAX_NUM; i++)
        dev_alloc_mark[i] = 0;

    for (i = 0; i < URB_MAX_NUM; i++)
        urb_alloc_mark[i] = 0;

    for (i = 0; i < MAX_HUB_DEVICE; i++)
        hub_alloc_mark[i] = 0;
}

/**
  * @brief   Allocate an URB from USB Core driver internal URB pool.
  * @return  URB or NULL.
  * @retval  NULL  Out of URB.
  * @retval  Otherwise  The pointer refer to the newly allocated URB.
  */
URB_T * USBH_AllocUrb()
{
    int  i;

    for (i = 0; i < URB_MAX_NUM; i++) {
        if (urb_alloc_mark[i] == 0) {
            urb_alloc_mark[i] = 1;
            memset((char *)&g_urb_pool[i], 0, sizeof(URB_T));
            return &g_urb_pool[i];
        }
    }
    USB_error("USBH_AllocUrb failed!\n");
    return NULL;
}


/**
  * @brief    Free the URB allocated from USBH_AllocUrb()
  * @param[in] urb   The URB to be freed.
  * @return   None
  */
void USBH_FreeUrb(URB_T *urb)
{
    int  i;

    for (i = 0; i < URB_MAX_NUM; i++) {
        if (&g_urb_pool[i] == urb) {
            urb_alloc_mark[i] = 0;
            return;
        }
    }
    USB_error("USBH_FreeUrb - missed!\n");
}

void usbh_free_dev_urbs(USB_DEV_T *dev)
{
    int  i;

    for (i = 0; i < URB_MAX_NUM; i++) {
        if (urb_alloc_mark[i]) {
            if (g_urb_pool[i].dev == dev) {
                USBH_UnlinkUrb(&g_urb_pool[i]);
                urb_alloc_mark[i] = 0;
            }
        }
    }
}


ED_T * ohci_alloc_ed()
{
    int  i;

    for (i = 0; i < ED_MAX_NUM; i++) {
        if (ed_alloc_mark[i] == 0) {
            ed_alloc_mark[i] = 1;
            memset((char *)&g_ed_pool[i], 0, sizeof(ED_T));
            _ed_cnt--;
            return &g_ed_pool[i];
        }
    }
    USB_error("ohci_alloc_ed failed!\n");
    return NULL;
}


void ohci_free_ed(ED_T *ed_p)
{
    int  i;

    for (i = 0; i < ED_MAX_NUM; i++) {
        if (&g_ed_pool[i] == ed_p) {
            ed_alloc_mark[i] = 0;
            _ed_cnt++;
            return;
        }
    }
    USB_error("ohci_free_ed - missed!\n");
}


TD_T * ohci_alloc_td(USB_DEV_T *dev)
{
    int  i;

    for (i = 0; i < TD_MAX_NUM; i++) {
        if (td_alloc_mark[i] == 0) {
            td_alloc_mark[i] = 1;
            td_alloc_dev[i] = dev;
            memset((char *)&g_td_pool[i], 0, sizeof(TD_T));
            _td_cnt--;
            return &g_td_pool[i];
        }
    }
    USB_error("ohci_alloc_td failed!\n");
    return NULL;
}


void ohci_free_td(TD_T *td_p)
{
    int  i;

    for (i = 0; i < TD_MAX_NUM; i++) {
        if ((&g_td_pool[i] == td_p) && (td_alloc_mark[i])) {
            _td_cnt++;
            td_alloc_mark[i] = 0;
            return;
        }
    }
}


void ohci_free_dev_td(USB_DEV_T *dev)
{
    int   i;
    for (i = 0; i < TD_MAX_NUM; i++) {
        if ((td_alloc_mark[i]) && (td_alloc_dev[i] == dev)) {
            _td_cnt++;
            td_alloc_mark[i] = 0;
            return;
        }
    }
}


/*
 *  Only HC's should call usbh_alloc_device and usbh_free_device directly
 *  Anybody may use USB_IncreaseDeviceUser or usb_dec_dev_use
 */
USB_DEV_T  *usbh_alloc_device(USB_DEV_T *parent, USB_BUS_T *bus)
{
    int     i;
    USB_DEV_T  *dev;

    for (i = 0; i < DEV_MAX_NUM; i++) {
        if (dev_alloc_mark[i] == 0) {
            dev_alloc_mark[i] = 1;
            dev = &g_dev_pool[i];
            memset((char *)dev, 0, sizeof(*dev));
            dev->act_config = -1;
            dev->parent = parent;
            dev->bus = bus;
            if (dev->bus->op->allocate(dev) != 0) {
                dev_alloc_mark[i] = 0;
                return NULL;
            }
            return dev;
        }
    }
    USB_error("usbh_alloc_device failed!\n");
    return NULL;
}


void  usbh_free_device(USB_DEV_T *dev)
{
    int     i, j;
    OHCI_DEVICE_T  *ohcidev;

    dev->bus->op->deallocate(dev);
    for (i = 0; i < DEV_MAX_NUM; i++) {
        if (&g_dev_pool[i] == dev) {
            ohcidev = usb_to_ohci(dev);
            for (j = 0; j < NUM_EDS; j++) {
                if (ohcidev->edp[j] != NULL)
                    ohci_free_ed(ohcidev->edp[j]);
            }
            dev_alloc_mark[i] = 0;
            return;
        }
    }
    USB_error("usbh_free_device - missed!\n");
}


USB_HUB_T * usbh_alloc_hubdev()
{
    int  i;

    for (i = 0; i < MAX_HUB_DEVICE; i++) {
        if (hub_alloc_mark[i] == 0) {
            hub_alloc_mark[i] = 1;
            memset((char *)&g_hub_pool[i], 0, sizeof(USB_HUB_T));
            return &g_hub_pool[i];
        }
    }
    USB_error("usbh_alloc_hubdev failed!\n");
    return NULL;
}


void usbh_free_hubdev(USB_HUB_T *hub)
{
    int  i;

    for (i = 0; i < MAX_HUB_DEVICE; i++) {
        if (&g_hub_pool[i] == hub) {
            hub_alloc_mark[i] = 0;
            return;
        }
    }
    USB_error("ohci_free_ed - missed!\n");
}


USB_HUB_T * usbh_get_hub_by_dev(USB_DEV_T *dev)
{
    int   i;

    for (i = 0; i < MAX_HUB_DEVICE; i++) {
        if (hub_alloc_mark[i]) {
            if (g_hub_pool[i].dev == dev)
                return &g_hub_pool[i];
        }
    }
    return NULL;
}


/*-------------------------------------------------------------------------
 *  Milisecond delay
 *-------------------------------------------------------------------------*/

void  usbh_mdelay(uint32_t msec)
{
    volatile uint32_t  t0;
    volatile uint32_t  frame_no;

    if ((halUSBH_Get_HcControl() & 0xC0) == 0x40) {
        /* OHCI in operational state */
        for (t0 = 0; t0 < msec; t0++) {
            frame_no = halUSBH_Get_HcFmNumber();
            while (halUSBH_Get_HcFmNumber() == frame_no) ;
        }
    } else {
        for (t0 = 0; t0 < msec * 0x1000; t0++) ;
    }
}


/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/


