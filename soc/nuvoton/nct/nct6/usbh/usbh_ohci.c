
/**************************************************************************//**
 * @file     usbh_ohci.c
 * @brief    MCU USB Host OHCI driver
 *
 * @note
 * URB OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000 David Brownell <david-b@pacbell.net>
 *
 * [ Initialisation is based on Linus'  ]
 * [ uhci code and gregs ohci fragments ]
 * [ (C) Copyright 1999 Linus Torvalds  ]
 * [ (C) Copyright 1999 Gregory P. Smith]
 *
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "usbh_core.h"
#include "usbh_ohci.h"
#include "errno.h"
#include "hal_ohci.h"

#ifdef __ICCARM__
    #pragma data_alignment=256
    static uint8_t  g_ohci_hcca[256];
    #pragma data_alignment=4
#elif defined ( __GNUC__ )
    static uint8_t  g_ohci_hcca[256] __attribute__ ((aligned (256)));
#elif defined ( __CC_ARM )
    static __align(256) uint8_t  g_ohci_hcca[256];
#endif


USB_BUS_T           g_ohci_bus;
static OHCI_T       ohci;

char  _is_in_interrupt = 0;


/* For initializing controller (mask in an HCFS mode too) */
#define OHCI_CONTROL_INIT \
        ((OHCI_CTRL_CBSR & 0x3) | OHCI_CTRL_IE | OHCI_CTRL_PLE)


/* #define OHCI_UNLINK_TIMEOUT     (HZ / 10)  */ /* Linux */
#define OHCI_UNLINK_TIMEOUT    20             /* 30ms, modified by YCHuang */


static OHCI_DEVICE_T  g_ohci_dev_pool[DEV_MAX_NUM];
static uint8_t  ohci_dev_alloc_mark[DEV_MAX_NUM];

static void td_submit_urb(URB_T * urb);
static int  rh_submit_urb(URB_T *urb);
static ED_T *ep_add_ed(USB_DEV_T *usb_dev, uint32_t pipe, int interval, int load);
static int  ep_link(ED_T *edi);
static void  ep_rm_ed(USB_DEV_T *usb_dev, ED_T *ed);
static int  ep_unlink(ED_T *ed);
static int  rh_unlink_urb(URB_T *urb);


static int  cc_to_error[16] = {

    /* mapping of the OHCI CC status to error codes */
    /* No  Error  */               USB_OK,
    /* CRC Error  */               CC_ERR_CRC,
    /* Bit Stuff  */               CC_ERR_BITSTUFF,
    /* Data Togg  */               CC_ERR_DATA_TOGGLE,
    /* Stall      */               CC_ERR_STALL,
    /* DevNotResp */               CC_ERR_NORESPONSE,
    /* PIDCheck   */               CC_ERR_PID_CHECK,
    /* InvalidPID */               CC_ERR_INVALID_PID,
    /* DataOver   */               CC_ERR_DATAOVERRUN,
    /* DataUnder  */               CC_ERR_DATAUNDERRUN,
    CC_ERR_NOT_DEFINED,
    CC_ERR_NOT_DEFINED,
    /* BufferOver */               CC_ERR_BUFFEROVERRUN,
    /* BuffUnder  */               CC_ERR_BUFFERUNDERRUN,
    /* Not Access */               CC_ERR_NOT_ACCESS,
    /* Not Access */               CC_ERR_NOT_ACCESS
};


void dump_hcca()
{
    int  i = 0;
    for (i = 0; i < 32; i++)
        printf("HCCA %02d - 0x%x\n", i, ohci.hcca->int_table[i]);
}

static void TD_CompletionCode(uint8_t cc)
{
    switch (cc) {
    case TD_CC_NOERROR:
        USB_debug("NO ERROR");
        break;
    case TD_CC_CRC:
        USB_debug("CRC ERROR");
        break;
    case TD_CC_BITSTUFFING:
        USB_debug("BIT STUFFING");
        break;
    case TD_CC_DATATOGGLEM:
        USB_debug("DATA TOGGLE MISMATCH");
        break;
    case TD_CC_STALL:
        USB_debug("STALL");
        break;
    case TD_DEVNOTRESP:
        USB_debug("DEVICE NOT RESPONDING");
        break;
    case TD_PIDCHECKFAIL:
        USB_debug("PID CHECK FAILURE");
        break;
    case TD_UNEXPECTEDPID:
        USB_debug("UNEXPECTED PID");
        break;
    case TD_DATAOVERRUN:
        USB_debug("DATA OVERRUN");
        break;
    case TD_DATAUNDERRUN:
        USB_debug("DATA UNDERRUN");
        break;
    case TD_BUFFEROVERRUN:
        USB_debug("BUFFER OVERRUN");
        break;
    case TD_BUFFERUNDERRUN:
        USB_debug("BUFFER UNDERRUN");
        break;
    case TD_NOTACCESSED:
        USB_debug("NOT ACCESSED");
        break;
    }
    USB_debug(",  [0x%02x]\n", cc);
}


/*-------------------------------------------------------------------------*
 * URB support functions
 *-------------------------------------------------------------------------*/

/* free HCD-private data associated with this URB */
static void  urb_free_priv(URB_PRIV_T *urb_priv)
{
    int i;

    for (i = 0; i < urb_priv->length; i++) {
        if (urb_priv->td[i])
            ohci_free_td(urb_priv->td[i]);
    }
}


/*-------------------------------------------------------------------------*/

/* debug| print the main components of an URB
 * small: 0) header + data packets 1) just header */
#ifdef USB_VERBOSE_DEBUG
static void urb_print(URB_T * urb, char * str, int small)
{
    int         len;
    uint32_t    pipe;

    pipe = urb->pipe;

    if (!urb->dev || !urb->dev->bus) {
        USB_debug("%s URB: no dev\n", str);
        return;
    }

    USB_debug("%s - ", str);

    if (urb->status != 0) {
        USB_debug("URB -");
        USB_debug("dev:%d, ", usb_pipedevice (pipe));
        USB_debug("ep:%d<%c>, ", usb_pipeendpoint (pipe), usb_pipeout (pipe)? 'O': 'I');
        //USB_debug("type:%s\n     ", usb_pipetype (pipe) < 2? (usb_pipeint (pipe)? "INTR": "ISOC"):
        //                 (usb_pipecontrol (pipe)? "CTRL": "BULK"));
        USB_debug("flags:%04x, ", urb->transfer_flags);
        USB_debug("len:%d/%d, ", urb->actual_length, urb->transfer_buffer_length);
        USB_debug("stat:%d(%x)\n", urb->status, urb->status);
    }

    if (!small) {
        int i; //, len;

        if (usb_pipecontrol (pipe)) {
            switch (urb->setup_packet[1]) {
            case 0:
                USB_debug("GET_STATUS");
                break;
            case 1:
                USB_debug("CLEAR_FEATURE");
                break;
            case 3:
                USB_debug("SET_FEATURE");
                break;
            case 5:
                USB_debug("SET_ADDRESS");
                break;
            case 6:
                USB_debug("GET_DESCRIPTOR");
                break;
            case 7:
                USB_debug("SET_DESCRIPTOR");
                break;
            case 8:
                USB_debug("GET_CONFIGURATION");
                break;
            case 9:
                USB_debug("SET_CONFIGURATION");
                break;
            case 10:
                USB_debug("GET_INTERFACE");
                break;
            case 11:
                USB_debug("SET_INTERFACE");
                break;
            case 12:
                USB_debug("SYNC_FRAME");
                break;
            default:
                USB_debug("Command");
                break;
            }

            USB_debug("(8):");
            for (i = 0; i < 8 ; i++)
                USB_debug (" %02x", ((uint8_t *) urb->setup_packet) [i]);
            USB_debug("\n");
        }
        if (urb->transfer_buffer_length > 0 && urb->transfer_buffer) {
            USB_debug(": data(%d/%d):", urb->actual_length,
                      urb->transfer_buffer_length);
            len = usb_pipeout (pipe)?
                  urb->transfer_buffer_length: urb->actual_length;
            for (i = 0; i < 16 && i < len; i++)
                USB_debug (" %02x", ((uint8_t *) urb->transfer_buffer) [i]);
            USB_debug("%s stat:%d\n", i < len? "...": "", urb->status);
        }
        USB_debug("\n");
    }
    USB_debug("\n");
}
#endif


#if 0
/* just for debugging; prints non-empty branches of the int ed tree inclusive iso eds*/
static void  ep_print_int_eds()
{
    int     i, j;
    uint32_t  *ed_p;
    ED_T    *ed;

    for (i = 0; i < 32; i++) {
        j = 5;
        ed_p = &(ohci.hcca->int_table[i]);
        if (*ed_p == 0)
            continue;
        USB_debug("branch %d:", i);
        while ((*ed_p != 0) && (j--)) {
            ed = (ED_T *) (*ed_p);
            USB_debug(" ed:%4x;", ed->hwINFO);
            ed_p = &ed->hwNextED;
        }
        USB_debug("\n");
    }
}
#endif


/*-------------------------------------------------------------------------*
 * Interface functions (URB)
 *-------------------------------------------------------------------------*/
/* return a request to the completion handler */

static int sohci_return_urb(URB_T * urb)
{
    URB_PRIV_T  *urb_priv;

    urb_priv = &urb->urb_hcpriv;

    /* just to be sure */
    if (!urb->complete) {
        urb_free_priv(&urb->urb_hcpriv);
        return -1;
    }

#ifdef USB_VERBOSE_DEBUG
    urb_print (urb, "RET", usb_pipeout(urb->pipe));
#endif

    switch (usb_pipetype (urb->pipe)) {
    case PIPE_INTERRUPT:
        urb->complete(urb);    /* call complete and requeue URB */
        urb->actual_length = 0;

#if 1   // 2013.07.31
        if ((urb_priv->state != URB_DEL) && (urb->status == 0))
#else
        if (urb_priv->state != URB_DEL)
#endif
        {
            urb->status = USB_ERR_URB_PENDING;
            td_submit_urb(urb);
        }
        else if (urb_priv->state != URB_DEL) {
            urb->status = USB_ERR_URB_PENDING;
            td_submit_urb(urb);
        }
        break;

    case PIPE_ISOCHRONOUS:
#if 0
        for (urbt = urb->next; urbt && (urbt != urb); urbt = urbt->next);
        if (urbt) {
            /* send the reply and requeue URB */
            urb->complete(urb);
            urb->actual_length = 0;
            urb->status = USB_ERR_URB_PENDING;
            urb->start_frame = urb_priv->ed->last_iso + 1;
            if (urb_priv->state != URB_DEL) {
                for (i = 0; i < urb->number_of_packets; i++) {
                    urb->iso_frame_desc[i].actual_length = 0;
                    urb->iso_frame_desc[i].status = -USB_ERR_XDEV;
                }
                td_submit_urb (urb);
            }
        } else {
            /* unlink URB, call complete */
            urb_free_priv(&urb->urb_hcpriv);
            urb->complete (urb);
        }
#endif
        urb_free_priv(&urb->urb_hcpriv);
        urb->complete(urb);
        break;

    case PIPE_BULK:
    case PIPE_CONTROL: /* unlink URB, call complete */
        urb_free_priv(&urb->urb_hcpriv);
        urb->complete(urb);
        break;
    }
    return 0;
}


/*-------------------------------------------------------------------------*/

/* get a transfer request */

static int  sohci_submit_urb(URB_T * urb)
{
    URB_PRIV_T  *urb_priv;
    ED_T        *ed;
    uint32_t    pipe;
    int         i, size = 0;

    pipe = urb->pipe;

    USB_info("[OHCI] Enter sohci_submit_urb() ...\n");

    if (!urb->dev || !urb->dev->bus)
        return USB_ERR_NODEV;

    if (usb_endpoint_halted(urb->dev, usb_pipeendpoint (pipe), usb_pipeout (pipe)))
        return USB_ERR_PIPE;

#ifdef USB_VERBOSE_DEBUG
    urb_print (urb, "SUB", usb_pipein (pipe));
#endif

    /* handle a request to the virtual root hub */
    if (usb_pipedevice(pipe) == ohci.rh.devnum)
        return rh_submit_urb(urb);

    /* when controller's hung, permit only roothub cleanup attempts
     * such as powering down ports */
    if (ohci.disabled)
        return USB_ERR_SHUTDOWN;

    /* every endpoint has an ed, locate and fill it */
    ed = ep_add_ed(urb->dev, pipe, urb->interval, 1);
    if (!ed)
        return USB_ERR_NOMEM;

    /* for the private part of the URB we need the number of TDs (size) */
    switch (usb_pipetype (pipe)) {
    case PIPE_BULK: /* one TD for every 4096 Byte */
        size = (urb->transfer_buffer_length - 1) / 4096 + 1;
        break;

    case PIPE_ISOCHRONOUS: /* number of packets from URB */
        size = urb->number_of_packets / ISO_FRAME_COUNT;
        if (size <= 0)
            return USB_ERR_INVAL;

        for (i = 0; i < urb->number_of_packets; i++) {
            urb->iso_frame_desc[i].actual_length = 0;
            urb->iso_frame_desc[i].status = -USB_ERR_XDEV;
        }
        break;

    case PIPE_CONTROL: /* 1 TD for setup, 1 for ACK and 1 for every 4096 B */
        size = (urb->transfer_buffer_length == 0)? 2:
               (urb->transfer_buffer_length - 1) / 4096 + 3;
        break;

    case PIPE_INTERRUPT: /* one TD */
        size = 1;
        break;
    }

    if (size > MAX_TD_PER_OHCI_URB)
        return USB_ERR_NOMEM;

    /* allocate the private part of the URB */
    urb_priv = &(urb->urb_hcpriv);
    memset(urb_priv, 0, sizeof(URB_PRIV_T));

    for (i = 0; i < size; i++) {
        urb_priv->td[i] = ohci_alloc_td(urb->dev);
        if (!urb_priv->td[i]) {
            urb_free_priv(urb_priv);
            return USB_ERR_NOMEM;
        }
    }

    /* fill the private part of the URB */
    urb_priv->length = size;
    urb_priv->ed = ed;

    if (ed->state == ED_NEW || (ed->state & ED_DEL)) {
        urb_free_priv(urb_priv);
        return USB_ERR_INVAL;
    }

    if (usb_pipetype(pipe) == PIPE_ISOCHRONOUS) {
        if (urb->transfer_flags & USB_ISO_ASAP) {
            urb->start_frame = ((ed->state == ED_OPER) ? (ed->last_iso + 1) : (ohci.hcca->frame_no)) & 0xffff;
        }
    }

    urb->actual_length = 0;
    urb->status = USB_ERR_URB_PENDING;

    if (pipe == PIPE_INTERRUPT)
        USB_debug("submit: urb: %x, ed: %x\n", (int)urb, (int)ed);

    /* link the ed into a chain if is not already */
    if (ed->state != ED_OPER)
        ep_link(ed);

    /* fill the TDs and link it to the ed */
    td_submit_urb(urb);
    return 0;
}


/*-------------------------------------------------------------------------*/

/* deactivate all TDs and remove the private part of the URB */
/* interrupt callers must use async unlink mode */

static int  sohci_unlink_urb (URB_T * urb)
{
    if (!urb) /* just to be sure */
        return USB_ERR_INVAL;

    if (!urb->dev || !urb->dev->bus)
        return USB_ERR_NODEV;

#ifdef USB_VERBOSE_DEBUG
    urb_print (urb, "UNLINK", 1);
#endif

    /* handle a request to the virtual root hub */
    if (usb_pipedevice (urb->pipe) == ohci.rh.devnum)
        return rh_unlink_urb(urb);

    if (urb->status == USB_ERR_URB_PENDING) {
        if (!ohci.disabled) {
            URB_PRIV_T  * urb_priv;

            /* interrupt code may not sleep; it must use
             * async status return to unlink pending urbs.
             */
            if (!(urb->transfer_flags & USB_ASYNC_UNLINK) && _is_in_interrupt) {
                USB_error("bug in call to sohci_unlink_urb!!\n");
                return EWOULDBLOCK;
            }

            /* flag the urb and its TDs for deletion in some
             * upcoming SF interrupt delete list processing
             */
            urb_priv = &urb->urb_hcpriv;

            if (urb_priv->state == URB_DEL)
                return 0;

            urb_priv->state = URB_DEL;

            ep_rm_ed(urb->dev, urb_priv->ed);
            urb_priv->ed->state |= ED_URB_DEL;

            if (!(urb->transfer_flags & USB_ASYNC_UNLINK)) {
                /* wait until all TDs are deleted */
                usbh_mdelay(100);

                if (urb->status == USB_ERR_URB_PENDING) {
                    USB_warning("unlink URB timeout\n");
                    return USB_ERR_TIMEOUT;
                }

                // 2014.07.28, force free TDs
                urb_free_priv(&urb->urb_hcpriv);
            } else { /* (urb->transfer_flags & USB_ASYNC_UNLINK) */
                urb->status = USB_ERR_INPROGRESS;
            }
        } else { /* ohci->disabled */
            urb_free_priv(&urb->urb_hcpriv);
            if (urb->transfer_flags & USB_ASYNC_UNLINK) {
                urb->status = USB_ERR_CONNRESET;
                if (urb->complete)
                    urb->complete (urb);
            } else
                urb->status = USB_ERR_NOENT;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------------*/

/* allocate private data space for a usb device */

static int  sohci_alloc_dev(USB_DEV_T *usb_dev)
{
    int     i;
    OHCI_DEVICE_T  *ohcidev;

    for (i = 0; i < DEV_MAX_NUM; i++) {
        if (ohci_dev_alloc_mark[i] == 0) {
            ohci_dev_alloc_mark[i] = 1;
            ohcidev = &g_ohci_dev_pool[i];
            memset((char *)ohcidev, 0, sizeof (OHCI_DEVICE_T));
            usb_dev->hcpriv = ohcidev;
            return 0;
        }
    }
    USB_error("sohci_alloc_dev - no free memory!\n");
    return USB_ERR_NOMEM;
}

/*-------------------------------------------------------------------------*/

/* may be called from interrupt context */
/* frees private data space of usb device */

static int sohci_free_dev (USB_DEV_T * usb_dev)
{
    int    i, cnt = 0;
    ED_T   *ed;
    OHCI_DEVICE_T *dev;

    dev = usb_to_ohci(usb_dev);

    if (!dev)
        return 0;

    if (usb_dev->devnum >= 0) {
        /* driver disconnects should have unlinked all urbs
         * (freeing all the TDs, unlinking EDs) but we need
         * to defend against bugs that prevent that.
         */
        for (i = 0; i < NUM_EDS; i++) {
            ed = dev->edp[i];
            if (ed != NULL) {
                if (ed->state != ED_NEW) {
                    if (ed->state == ED_OPER) {
                        /* driver on that interface didn't unlink an urb */
                        USB_warning("driver dev %d ed 0x%x unfreed URB\n", usb_dev->devnum, i);
                        ep_unlink(ed);
                    }
                    ep_rm_ed(usb_dev, ed);
                    ed->state = ED_DEL;
                    cnt++;
                }
                ohci_free_ed(ed);
                dev->edp[i] = 0;
            }
        }

        /* if the controller is running, tds for those unlinked
         * urbs get freed by dl_del_list at the next SF interrupt
         */
        if (cnt > 0) {
            if (ohci.disabled) {
                /* FIXME: Something like this should kick in,
                 * though it's currently an exotic case ...
                 * the controller won't ever be touching
                 * these lists again!!
                 * dl_del_list (ohci, le16_to_cpu (ohci->hcca->frame_no) & 1);
                 */
                USB_warning("Warning! TD leak, %d\n", cnt);
            } else if (!_is_in_interrupt) {
                /* SF interrupt handler calls dl_del_list */
                usbh_mdelay(100);

                if (dev->ed_cnt) {
                    USB_error("Error! - free device %d timeout", usb_dev->devnum);
                    return USB_ERR_TIMEOUT;
                }
            } else {
                /* likely some interface's driver has a refcount bug */
                USB_error("Error! - devnum %d deletion in interrupt\nSystem Halt!!", usb_dev->devnum);
            }
        }
    }

    ohci_free_dev_td(usb_dev);

    for (i = 0; i < DEV_MAX_NUM; i++) {
        if (&g_ohci_dev_pool[i] == dev)
            ohci_dev_alloc_mark[i] = 0;
    }
    return 0;
}

/*-------------------------------------------------------------------------*/

/* tell us the current USB frame number */
static int  sohci_get_current_frame_number(USB_DEV_T *usb_dev)
{
    return ohci.hcca->frame_no;
}

/*-------------------------------------------------------------------------*/

USB_OP_T sohci_device_operations = {
    sohci_alloc_dev,
    sohci_free_dev,
    sohci_get_current_frame_number,
    sohci_submit_urb,
    sohci_unlink_urb
};

/*-------------------------------------------------------------------------*
 * ED handling functions
 *-------------------------------------------------------------------------*/

/* search for the right branch to insert an interrupt ed into the int tree
 * do some load balancing;
 * returns the branch and
 * sets the interval to interval = 2^integer (ld (interval)) */
static int  ep_int_ballance (int interval, int load)
{
    int    i, branch = 0;

    /* search for the least loaded interrupt endpoint branch of all 32 branches */
    for (i = 0; i < 32; i++)
        if (ohci.ohci_int_load [branch] > ohci.ohci_int_load [i])
            branch = i;

    branch = branch % interval;
    for (i = branch; i < 32; i += interval)
        ohci.ohci_int_load [i] += load;

    return branch;
}

/*-------------------------------------------------------------------------*/

/*  2^int( ld (inter)) */
/*  depends on <inter>, return a value of 1,2,4,8,16,or 32 */
static int  ep_2_n_interval(int inter)
{
    int    i;

    for (i = 0; ((inter >> i) > 1 ) && (i < 5); i++);
    return 1 << i;
}

/*-------------------------------------------------------------------------*/

/* the int tree is a binary tree
 * in order to process it sequentially the indexes of the branches have to be mapped
 * the mapping reverses the bits of a word of num_bits length */

static int  ep_rev(int num_bits, int word)
{
    int i, wout = 0;

    for (i = 0; i < num_bits; i++)
        wout |= (((word >> i) & 1) << (num_bits - i - 1));
    return wout;
}



/*-------------------------------------------------------------------------*/

/* link an ed into one of the HC chains */

static int  ep_link(ED_T *edi)
{
    int     int_branch;
    int     i;
    int     inter;
    int     interval;
    int     load;
    uint32_t  *ed_p;
    volatile ED_T *ed;

    ed = edi;
    ed->state = ED_OPER;
    switch (ed->type) {
    case PIPE_CONTROL:
        ed->hwNextED = 0;
        if (ohci.ed_controltail == NULL) {
            halUSBH_Set_HcControlHeadED((uint32_t)ed);
        }
        else {
            ohci.ed_controltail->hwNextED = (uint32_t)ed;
        }
        ed->ed_prev = ohci.ed_controltail;
        if (!ohci.ed_controltail && !ohci.ed_rm_list[0] && (!ohci.ed_rm_list[1])) {
            ohci.hc_control |= OHCI_CTRL_CLE;
            halUSBH_Set_HcControl(ohci.hc_control);
        }
        ohci.ed_controltail = edi;
        break;

    case PIPE_BULK:
        ed->hwNextED = 0;
        if (ohci.ed_bulktail == NULL) {
            halUSBH_Set_HcBulkHeadED((uint32_t)ed);
        }
        else {
            ohci.ed_bulktail->hwNextED = (uint32_t)ed;
        }
        ed->ed_prev = ohci.ed_bulktail;
        if (!ohci.ed_bulktail && !ohci.ed_rm_list[0] && (!ohci.ed_rm_list[1])) {
            ohci.hc_control |= OHCI_CTRL_BLE;
            halUSBH_Set_HcControl(ohci.hc_control);
        }
        ohci.ed_bulktail = edi;
        break;

    case PIPE_INTERRUPT:
        load = ed->int_load;
        interval = ep_2_n_interval(ed->int_period);
        ed->int_interval = interval;
        int_branch = ep_int_ballance(interval, load);
        ed->int_branch = int_branch;

        for (i = 0; i < ep_rev(6, interval); i += inter) {
            inter = 1;

            for (ed_p = &(ohci.hcca->int_table[ep_rev (5,i) + int_branch]);
                    (*ed_p != 0) && (((ED_T *) (*ed_p))->int_interval >= interval);
                    ed_p = &(((ED_T *) (*ed_p))->hwNextED))
                inter = ep_rev(6, ((ED_T *) (*ed_p))->int_interval);
            ed->hwNextED = *ed_p;
            *ed_p = (uint32_t)ed;
        }
        break;

    case PIPE_ISOCHRONOUS:
        ed->hwNextED = 0;
        ed->int_interval = 1;
        if (ohci.ed_isotail != NULL) {
            ohci.ed_isotail->hwNextED = (uint32_t)ed;
            ed->ed_prev = ohci.ed_isotail;
        } else {
            for ( i = 0; i < 32; i += inter) {
                inter = 1;
                for (ed_p = &(ohci.hcca->int_table[ep_rev (5, i)]);
                        (*ed_p != 0) && ed_p; ed_p = &(((ED_T *) (*ed_p))->hwNextED))
                    inter = ep_rev (6, ((ED_T *) (*ed_p))->int_interval);
                *ed_p = (uint32_t)ed;
            }
            ed->ed_prev = NULL;
        }
        ohci.ed_isotail = edi;
        break;
    }
    return 0;
}

/*-------------------------------------------------------------------------*/

/* unlink an ed from one of the HC chains.
 * just the link to the ed is unlinked.
 * the link from the ed still points to another operational ed or 0
 * so the HC can eventually finish the processing of the unlinked ed */

static int  ep_unlink(ED_T *ed)
{
    int     i;
    uint32_t  *ed_ptr;

    ed->hwINFO |= OHCI_ED_SKIP;
    switch (ed->type) {
    case PIPE_CONTROL:
        if (ed->ed_prev == NULL) {
            if (!ed->hwNextED) {
                ohci.hc_control &= ~OHCI_CTRL_CLE;
                halUSBH_Set_HcControl(ohci.hc_control);
            }
            halUSBH_Set_HcControlHeadED(ed->hwNextED);
        } else
            ed->ed_prev->hwNextED = ed->hwNextED;

        if (ohci.ed_controltail == ed)
            ohci.ed_controltail = ed->ed_prev;
        else
            ((ED_T *)(ed->hwNextED))->ed_prev = ed->ed_prev;
        break;

    case PIPE_BULK:
        if (ed->ed_prev == NULL) {
            if (!ed->hwNextED) {
                ohci.hc_control &= ~OHCI_CTRL_BLE;
                halUSBH_Set_HcControl(ohci.hc_control);
            }
            halUSBH_Set_HcBulkHeadED(ed->hwNextED);
        } else
            ed->ed_prev->hwNextED = ed->hwNextED;

        if (ohci.ed_bulktail == ed)
            ohci.ed_bulktail = ed->ed_prev;
        else
            ((ED_T *)(ed->hwNextED))->ed_prev = ed->ed_prev;
        break;

    case PIPE_ISOCHRONOUS:

        if (ohci.ed_isotail == ed)
            ohci.ed_isotail = ed->ed_prev;
        if (ed->hwNextED != 0)
            ((ED_T *)ed->hwNextED)->ed_prev = ed->ed_prev;

        if (ed->ed_prev != NULL) {
            ed->ed_prev->hwNextED = ed->hwNextED;
        }

    // Isochronous fall through ...

    case PIPE_INTERRUPT:
        for (i = 0; i < 32; i++) {
            for (ed_ptr = (uint32_t *)&(ohci.hcca->int_table[i]);
                    (*ed_ptr != 0); ed_ptr = (uint32_t *) &(((ED_T *) (*ed_ptr))->hwNextED)) {
                if (ed_ptr == (uint32_t *) &(((ED_T *) (*ed_ptr))->hwNextED))
                    break;

                if ((ED_T *)*ed_ptr == ed) {
                    *ed_ptr = (uint32_t) (((ED_T *) (*ed_ptr))->hwNextED);
                    break;
                }
            }
        }
        break;

    }
    ed->state = ED_UNLINK;
    return 0;
}


/*-------------------------------------------------------------------------*/

/* add/reinit an endpoint; this should be done once at the usb_set_configuration command,
 * but the USB stack is a little bit stateless  so we do it at every transaction
 * if the state of the ed is ED_NEW then a dummy td is added and the state is changed to ED_UNLINK
 * in all other cases the state is left unchanged
 * the ed info fields are setted anyway even though most of them should not change */

static ED_T  *ep_add_ed(USB_DEV_T *usb_dev, uint32_t pipe, int interval, int load)
{
    TD_T    *td;
    ED_T    *ed_ret;
    OHCI_DEVICE_T   *ohci_dev;
    int     ep_num;
    volatile ED_T *ed;

    ohci_dev = usb_to_ohci(usb_dev);
    ep_num = (((pipe) >> 15) & 0xf);
    if (pipe & 0x80)
        ep_num |= 0x10;

    if (ohci_dev->edp[ep_num] == NULL)
        ohci_dev->edp[ep_num] = ohci_alloc_ed();

    if (ohci_dev->edp[ep_num] == NULL) {
        return NULL;
    }

    ed = ed_ret = ohci_dev->edp[ep_num];

    if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL))
        return NULL;

    if (ed->state == ED_NEW) {
        ed->hwINFO = OHCI_ED_SKIP; /* skip ed */
        /* dummy td; end of td list for ed */
        td = ohci_alloc_td(usb_dev);
        if (!td)
            return NULL;

        ed->hwTailP = (uint32_t)td;
        ed->hwHeadP = ed->hwTailP;
        ed->state = ED_UNLINK;
        ed->type = usb_pipetype(pipe);
        usb_to_ohci (usb_dev)->ed_cnt++;
    }

    ohci.dev[usb_pipedevice(pipe)] = usb_dev;

    ed->hwINFO = (usb_pipedevice (pipe)
                  | usb_pipeendpoint (pipe) << 7
                  | (usb_pipeisoc (pipe)? 0x8000: 0)
                  | (usb_pipecontrol (pipe)? 0: (usb_pipeout (pipe)? 0x800: 0x1000))
                  | usb_pipeslow (pipe) << 13
                  | usb_maxpacket(usb_dev, pipe, usb_pipeout(pipe)) << 16);

    if ((ed->type == PIPE_INTERRUPT) && (ed->state == ED_UNLINK)) {
        ed->int_period = interval;
        ed->int_load = load;
    }
    return ed_ret;
}



/*-------------------------------------------------------------------------*/

/* request the removal of an endpoint
 * put the ep on the rm_list and request a stop of the bulk or ctrl list
 * real removal is done at the next start frame (SF) hardware interrupt */

static void  ep_rm_ed(USB_DEV_T *usb_dev, ED_T *ed)
{
    uint32_t  frame;

    if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL))
        return;

    ed->hwINFO |= OHCI_ED_SKIP;

    if (!ohci.disabled) {
        switch (ed->type) {
        case PIPE_CONTROL:    /* stop control list */
            ohci.hc_control &= ~OHCI_CTRL_CLE;
            halUSBH_Set_HcControl(ohci.hc_control);
            break;
        case PIPE_BULK:       /* stop bulk list */
            ohci.hc_control &= ~OHCI_CTRL_BLE;
            halUSBH_Set_HcControl(ohci.hc_control);
            break;
        }
    }
    frame = ohci.hcca->frame_no & 0x1;
    ed->ed_rm_list = ohci.ed_rm_list[frame];
    ohci.ed_rm_list[frame] = ed;

    if (!ohci.disabled) {
        /* enable SOF interrupt */
        halUSBH_Set_HcInterruptStatus(OHCI_INTR_SF);
        halUSBH_Set_HcInterruptEnable(OHCI_INTR_SF);
    }
}



/*-------------------------------------------------------------------------*
 * TD handling functions
 *-------------------------------------------------------------------------*/
static void  iso_td_fill(uint32_t info, void *data, URB_T *urb, int index)
{
    volatile TD_T   *td, *td_pt;
    volatile URB_PRIV_T *urb_priv;
    int             len=0, i;
    uint32_t        bufferStart;

    urb_priv = &urb->urb_hcpriv;

    if (index >= urb_priv->length) {
        USB_error("Error! - internal OHCI error: TD index > length\n");
        return;
    }

    td_pt = urb_priv->td[index];

    /*- fill the old dummy TD -*/
    td = urb_priv->td[index] = (TD_T *)((uint32_t)urb_priv->ed->hwTailP & 0xfffffff0); /* find the tail of the td list of this URB */

    td->ed = urb_priv->ed;
    td->ed->last_iso = (info & 0xffff) + ISO_FRAME_COUNT - 1;
    info = (info & 0xffff0000) | ((info + OHCI_ISO_DELAY) & 0xffff);
    td->next_dl_td = NULL;
    td->index = index;
    td->urb = urb;

    bufferStart = (uint32_t)data + urb->iso_frame_desc[index * ISO_FRAME_COUNT].offset;

    /* caculate total length of frames to be filled in this TD */
    for (i = index * ISO_FRAME_COUNT; i < (index + 1) * ISO_FRAME_COUNT; i++)
        len += urb->iso_frame_desc[i].length;

    td->hwINFO = info;
    td->hwCBP = (uint32_t)((!bufferStart || !len) ? 0 : bufferStart) & 0xFFFFF000;
    td->hwBE = (uint32_t)((!bufferStart || !len ) ? 0 : (bufferStart + len -1));
    td->hwNextTD = (uint32_t)td_pt;
    td->is_iso_packet = 1;

#ifdef DELAY_INTERRUPT
    info = (info & 0xFF1FFFFF) | (3 << 21);  /* delay 3 frame */
#endif

    td->hwPSW[0] = (((uint32_t)data + urb->iso_frame_desc[index * ISO_FRAME_COUNT].offset) & 0x0FFF) | 0xE000;
    for (i = 0; i < ISO_FRAME_COUNT / 2; i++) {
        td->hwPSW[i] = (((uint32_t)data + urb->iso_frame_desc[index * ISO_FRAME_COUNT + i * 2].offset) & 0x0FFF) |
                       ((((uint32_t)data + urb->iso_frame_desc[index * ISO_FRAME_COUNT + i * 2 + 1].offset) & 0x0FFF) << 16);
        td->hwPSW[i] |= 0xE000E000;
    }

    USB_info("Frame:[%lx], CBP:[%lx], BE:[%lx], PSW0:[%lx], PSW1:[%lx]\n",
             (td->hwINFO & 0xffff), td->hwCBP, td->hwBE, td->hwPSW[0], td->hwPSW[1]);

    /* chaining the new dummy TD */
    td_pt->hwNextTD = 0;
    td->ed->hwTailP = td->hwNextTD;
}



/* prepare a TD */
static void  td_fill(uint32_t info, void *data, int len, URB_T *urb, int index)
{
    volatile TD_T  *td, *td_pt;
    volatile URB_PRIV_T *urb_priv;

    urb_priv = &urb->urb_hcpriv;

    if (index >= urb_priv->length) {
        USB_error("Error! - internal OHCI error: TD index > length\n");
        return;
    }

    td_pt = urb_priv->td[index];

    /*- fill the old dummy TD -*/
    td = urb_priv->td[index] = (TD_T *)((uint32_t)urb_priv->ed->hwTailP & 0xfffffff0); /* find the tail of the td list of this URB */
    td->ed = urb_priv->ed;      /* the endpoint(pipe) of this URB */
    td->next_dl_td = NULL;
    td->index = index;
    td->urb = urb;

    td->hwINFO = info;
    td->hwCBP = (uint32_t)((!data || !len) ? 0 : data);
    td->hwBE = (uint32_t)((!data || !len ) ? 0 : (uint32_t)data + len - 1);
    td->hwNextTD = (uint32_t)td_pt;
    td->is_iso_packet = 0;

#ifdef DELAY_INTERRUPT
    info = (info & 0xFF1FFFFF) | (3 << 21);  /* delay 3 frame */
#endif

    td_pt->hwNextTD = 0;
    td->ed->hwTailP = td->hwNextTD;

    //USB_info("td_fill ed:[%x] td:[%x] - hwINFO:%08x hwCBP:%08x\n         hwNextTD:%08x hwBE:%08x\n",
    //      (int)td->ed, (int)td, td->hwINFO, td->hwCBP, td->hwNextTD, td->hwBE);
}

/*-------------------------------------------------------------------------*/

/* prepare all TDs of a transfer */
static void td_submit_urb(URB_T * urb)
{
    URB_PRIV_T  *urb_priv;
    void        *ctrl;
    void        *data;
    int         data_len;
    int         cnt = 0;
    uint32_t    info = 0;
    uint32_t    toggle = 0;

    urb_priv = &urb->urb_hcpriv;
    ctrl = urb->setup_packet;
    data = urb->transfer_buffer;
    data_len = urb->transfer_buffer_length;
    /* OHCI handles the DATA-toggles itself, we just use the USB-toggle bits for reseting */
    if (usb_gettoggle(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe))) {
        toggle = TD_T_TOGGLE;
    } else {
        toggle = TD_T_DATA0;
        usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe), 1);
    }

    urb_priv->td_cnt = 0;

    switch (usb_pipetype (urb->pipe)) {
    case PIPE_BULK:
        info = usb_pipeout(urb->pipe)? (TD_CC | TD_DP_OUT) : (TD_CC | TD_DP_IN);
        while(data_len > 4096) {
            td_fill(info | (cnt? TD_T_TOGGLE:toggle), data, 4096, urb, cnt);
            data = (void *)((uint32_t)data + 4096);
            data_len -= 4096;
            cnt++;
        }
        info = usb_pipeout(urb->pipe)? (TD_CC | TD_DP_OUT) : (TD_CC | TD_R | TD_DP_IN);
        td_fill(info | (cnt? TD_T_TOGGLE:toggle), data, data_len, urb, cnt);
        cnt++;
        halUSBH_Set_HcCommandStatus(OHCI_BLF);      /* start bulk list */
        break;

    case PIPE_INTERRUPT:
        info = usb_pipeout (urb->pipe)? (TD_CC | TD_DP_OUT | toggle) : (TD_CC | TD_R | TD_DP_IN | toggle);
        td_fill(info, data, data_len, urb, cnt++);
        break;

    case PIPE_CONTROL:
        info = TD_CC | TD_DP_SETUP | TD_T_DATA0;
        td_fill(info, ctrl, 8, urb, cnt++);
        if (data_len > 0) {
            info = usb_pipeout (urb->pipe)? (TD_CC | TD_R | TD_DP_OUT | TD_T_DATA1) : (TD_CC | TD_R | TD_DP_IN | TD_T_DATA1);
            td_fill(info, data, data_len, urb, cnt++);
        }
        info = usb_pipeout (urb->pipe)? (TD_CC | TD_DP_IN | TD_T_DATA1): (TD_CC | TD_DP_OUT | TD_T_DATA1);
        td_fill(info, NULL, 0, urb, cnt++);
        halUSBH_Set_HcCommandStatus(OHCI_CLF);          /* start Control list */
        break;

    case PIPE_ISOCHRONOUS:
        for (cnt = 0; cnt < urb->number_of_packets / ISO_FRAME_COUNT; cnt++) {
            iso_td_fill(TD_CC | ((ISO_FRAME_COUNT - 1) << 24) | ((urb->start_frame + cnt * ISO_FRAME_COUNT) & 0xffff ),
                        (uint8_t *) data, urb, cnt);
        }
        break;
    }
    if (urb_priv->length != cnt)
        USB_error("Error! - TD LENGTH %d != CNT %d", urb_priv->length, cnt);
}

/*-------------------------------------------------------------------------*
 * Done List handling functions
 *-------------------------------------------------------------------------*/


/* calculate the transfer length and update the urb */

static void dl_transfer_length(TD_T * td)
{
    uint32_t    tdBE, tdCBP;
    uint16_t    tdPSW;
    URB_T       *urb;
    URB_PRIV_T  *urb_priv;
    int         dlen = 0;
    int         cc = 0;

    urb = td->urb;
    urb_priv = &urb->urb_hcpriv;

    tdBE   = (uint32_t)td->hwBE;
    tdCBP  = (uint32_t)td->hwCBP;

    if (td->is_iso_packet) {
        int  i, index;


        for (i = 0; i < ISO_FRAME_COUNT; i++) {
            index = td->index * ISO_FRAME_COUNT + i;
            if (i % 2)
                tdPSW = td->hwPSW[i / 2] >> 16;
            else
                tdPSW = td->hwPSW[i / 2] & 0xffff;

            cc = (tdPSW >> 12) & 0xF;
            if (cc < 0xE) {
                if (usb_pipeout(urb->pipe))
                    dlen = urb->iso_frame_desc[index].length;
                else
                    dlen = tdPSW & 0x3ff;

                urb->actual_length += dlen;
                urb->iso_frame_desc[index].actual_length = dlen;
                if (!(urb->transfer_flags & USB_DISABLE_SPD) && (cc == TD_DATAUNDERRUN))
                    cc = TD_CC_NOERROR;
                urb->iso_frame_desc[index].status = cc_to_error[cc];
            }
        }
    } else {
        /* BULK, int, CONTROL DATA */
        if (!(usb_pipetype (urb->pipe) == PIPE_CONTROL &&
                ((td->index == 0) || (td->index == urb_priv->length - 1)))) {
            if (tdBE != 0) {
                if (td->hwCBP == 0)
                    urb->actual_length = (uint32_t)tdBE - (uint32_t)urb->transfer_buffer + 1;
                else
                    urb->actual_length = (uint32_t)tdCBP - (uint32_t)urb->transfer_buffer;
            }
        }
        USB_info("td:%x urb->actual_length:%d\n", (int)td, urb->actual_length);
    }
}


/* handle an urb that is being unlinked */

static void dl_del_urb(URB_T * urb)
{
    if (urb->transfer_flags & USB_ASYNC_UNLINK) {
        urb->status = USB_ERR_CONNRESET;
        if (urb->complete)
            urb->complete(urb);
    } else {
        urb->status = USB_ERR_NOENT;
    }
}


/*-------------------------------------------------------------------------*/

/* replies to the request have to be on a FIFO basis so
 * we reverse the reversed done-list */

static TD_T  *dl_reverse_done_list()
{
    uint32_t    td_list_hc;
    TD_T        *td_rev = NULL;
    TD_T        *td_list = NULL;
    URB_PRIV_T  *urb_priv;

    td_list_hc = (uint32_t)(ohci.hcca->done_head) & 0xfffffff0;  /* mask out WD bit also */
    ohci.hcca->done_head = 0;

    while (td_list_hc) {
        td_list = (TD_T *)td_list_hc;
        USB_info("TD done - 0x%08lx, DEV:[%d], ED:[%lu] PSW:[%lx]\n", (uint32_t)td_list, td_list->urb->dev->devnum, (td_list->ed->hwINFO>>7)&0xf, td_list->hwPSW[0] >> 12);

#ifdef USB_VERBOSE_DEBUG
        if (td_list->ed->hwHeadP & 0x1)
            USB_error("!!Halted - DEV:[%d], ED:[%d] PSW0:[%x] PSW1:[%x]\n",
                      td_list->urb->dev->devnum, (td_list->ed->hwINFO>>7)&0xf,
                      (td_list->hwPSW[0] >> 12) & 0xf, (td_list->hwPSW[0] >> 28) & 0xf);
#endif

        if (TD_CC_GET((uint32_t)td_list->hwINFO)) {
            urb_priv = &(td_list->urb->urb_hcpriv);

            if (td_list->ed->type == PIPE_ISOCHRONOUS) {
                if ((td_list->hwINFO & 0xE0000000) == 0x80000000) {
                    USB_error("!"); //("I N/A: %x\n",  td_list->hwINFO);
                    td_list->ed->last_iso += OHCI_ISO_DELAY;
                } else
                    USB_error("ISO! - CC:0x%x, PSW:0x%x 0x%x\n", td_list->hwINFO >> 28, td_list->hwPSW[0], td_list->hwPSW[1]);
            } else {
                USB_error("CC Error!! - DEV[%d] ED[%u] - ", td_list->urb->dev->devnum, (td_list->ed->hwINFO >> 7) & 0xf);
                TD_CompletionCode(TD_CC_GET((uint32_t)(td_list->hwINFO)));
            }

            if (td_list->ed->hwHeadP & 0x1) {
                if (urb_priv && ((td_list->index + 1) < urb_priv->length)) {
                    td_list->ed->hwHeadP = (urb_priv->td[urb_priv->length - 1]->hwNextTD & 0xfffffff0) |
                                           (td_list->ed->hwHeadP & 0x2);
                    urb_priv->td_cnt += urb_priv->length - td_list->index - 1;
                } else
                    td_list->ed->hwHeadP &= 0xfffffff2;
            }
        }

        td_list->next_dl_td = td_rev;
        td_rev = td_list;
        td_list_hc = (uint32_t)(td_list->hwNextTD) & 0xfffffff0;
    }  /* end of while */
    return td_list;
}

/*-------------------------------------------------------------------------*/

/* there are some pending requests to remove
 * - some of the eds (if ed->state & ED_DEL (set by sohci_free_dev)
 * - some URBs/TDs if urb_priv->state == URB_DEL */

static void  dl_del_list(uint32_t frame)
{
    ED_T        *ed;
    uint32_t    edINFO;
    uint32_t    tdINFO;
    TD_T        *td=NULL, *td_next=NULL, *tdHeadP=NULL, *tdTailP;
    uint32_t    *td_p;
    int         ctrl=0, bulk=0;

    for (ed = ohci.ed_rm_list[frame]; ed != NULL; ed = ed->ed_rm_list) {
        tdTailP = (TD_T *)((uint32_t)(ed->hwTailP) & 0xfffffff0);
        tdHeadP = (TD_T *)((uint32_t)(ed->hwHeadP) & 0xfffffff0);
        edINFO = (uint32_t)ed->hwINFO;
        td_p = &ed->hwHeadP;

        for (td = tdHeadP; td != tdTailP; td = td_next) {
            URB_T * urb = td->urb;
            URB_PRIV_T * urb_priv = &(td->urb->urb_hcpriv);

            td_next = (TD_T *)((uint32_t)(td->hwNextTD) & 0xfffffff0);
            if ((urb_priv->state == URB_DEL) || (ed->state & ED_DEL)) {
                tdINFO = (uint32_t)td->hwINFO;
                if (TD_CC_GET (tdINFO) < 0xE)
                    dl_transfer_length(td);
                *td_p = td->hwNextTD | (*td_p & 0x3);

                /* URB is done; clean up */
                if (++(urb_priv->td_cnt) == urb_priv->length)
                    dl_del_urb (urb);
            } else {
                td_p = &td->hwNextTD;
            }
        }

        if (ed->state & ED_DEL) {
            /* set by sohci_free_dev */
            OHCI_DEVICE_T * dev = usb_to_ohci (ohci.dev[edINFO & 0x7F]);
            ohci_free_td(tdTailP);       /* free dummy td */
            ed->hwINFO = OHCI_ED_SKIP;
            ed->state = ED_NEW;

            dev->ed_cnt--;         /* This may make task blocked in sohci_free_dev() to be unblocked */
        } else {
            ed->state &= ~ED_URB_DEL;
            tdHeadP = (TD_T *)((uint32_t)(ed->hwHeadP) & 0xfffffff0);
            if (tdHeadP == tdTailP) {
                if (ed->state == ED_OPER)
                    ep_unlink(ed);
                ohci_free_td(tdTailP);
                ed->hwINFO = OHCI_ED_SKIP;
                ed->state = ED_NEW;
                --(usb_to_ohci (ohci.dev[edINFO & 0x7F]))->ed_cnt;
            } else
                ed->hwINFO &= ~OHCI_ED_SKIP;
        }

        switch (ed->type) {
        case PIPE_CONTROL:
            ctrl = 1;
            break;
        case PIPE_BULK:
            bulk = 1;
            break;
        }
    }

    /* maybe reenable control and bulk lists */
    if (!ohci.disabled) {
        if (ctrl)       /* reset control list */
            halUSBH_Set_HcControlCurrentED(0);
        if (bulk)       /* reset bulk list */
            halUSBH_Set_HcBulkCurrentED(0);
        if (!ohci.ed_rm_list[!frame]) {
            if (ohci.ed_controltail)
                ohci.hc_control |= OHCI_CTRL_CLE;
            if (ohci.ed_bulktail)
                ohci.hc_control |= OHCI_CTRL_BLE;
            halUSBH_Set_HcControl(ohci.hc_control);
        }
    }

    ohci.ed_rm_list[frame] = NULL;
}


/*-------------------------------------------------------------------------*/

/* td done list */

static void  dl_done_list(TD_T *td_list)
{
    TD_T        *td_list_next = NULL;
    ED_T        *ed;
    int         cc = 0;
    URB_T       *urb;
    URB_PRIV_T  *urb_priv;
    uint32_t    tdINFO, edHeadP, edTailP;

    while (td_list) {
        td_list_next = td_list->next_dl_td;

        urb = td_list->urb;
        urb_priv = &urb->urb_hcpriv;
        tdINFO = (uint32_t)(td_list->hwINFO);

        ed = td_list->ed;

        dl_transfer_length(td_list);

        /* error code of transfer */
        cc = TD_CC_GET (tdINFO);
        if (cc == TD_CC_STALL)
            usb_endpoint_halt(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe));

        if (!(urb->transfer_flags & USB_DISABLE_SPD) && (cc == TD_DATAUNDERRUN))
            cc = TD_CC_NOERROR;

        if (++(urb_priv->td_cnt) == urb_priv->length) {
            if ((ed->state & (ED_OPER | ED_UNLINK)) && (urb_priv->state != URB_DEL)) {
                urb->status = cc_to_error[cc];
                sohci_return_urb(urb);
            } else {
                dl_del_urb(urb);
            }
        }
        if (ed->state != ED_NEW) {
            edHeadP = (uint32_t)(ed->hwHeadP) & 0xfffffff0;
            edTailP = (uint32_t)(ed->hwTailP);

            /* unlink eds if they are not busy */
            if ((edHeadP == edTailP) && (ed->state == ED_OPER)) {
                ep_unlink(ed);
            }
        }
        td_list = td_list_next;
    }
}


/*-------------------------------------------------------------------------*
 * Virtual Root Hub
 *-------------------------------------------------------------------------*/

/* Device descriptor */
static uint8_t root_hub_dev_des[] = {
    0x12,       /*  uint8_t  bLength; */
    0x01,       /*  uint8_t  bDescriptorType; Device */
    0x10,       /*  uint16_t bcdUSB; v1.1 */
    0x01,
    0x09,       /*  uint8_t  bDeviceClass; HUB_CLASSCODE */
    0x00,       /*  uint8_t  bDeviceSubClass; */
    0x00,       /*  uint8_t  bDeviceProtocol; */
    0x08,       /*  uint8_t  bMaxPacketSize0; 8 Bytes */
    0x00,       /*  uint16_t idVendor; */
    0x00,
    0x00,       /*  uint16_t idProduct; */
    0x00,
    0x00,       /*  uint16_t bcdDevice; */
    0x00,
    0x00,       /*  uint8_t  iManufacturer; */
    0x02,       /*  uint8_t  iProduct; */
    0x01,       /*  uint8_t  iSerialNumber; */
    0x01        /*  uint8_t  bNumConfigurations; */
};


/* Configuration descriptor */
static uint8_t root_hub_config_des[] = {
    0x09,       /*  uint8_t  bLength; */
    0x02,       /*  uint8_t  bDescriptorType; Configuration */
    0x19,       /*  uint16_t wTotalLength; */
    0x00,
    0x01,       /*  uint8_t  bNumInterfaces; */
    0x01,       /*  uint8_t  bConfigurationValue; */
    0x00,       /*  uint8_t  iConfiguration; */
    0x40,       /*  uint8_t  bmAttributes;
                 Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
    0x00,       /*  uint8_t  MaxPower; */

    /* interface */
    0x09,       /*  uint8_t  if_bLength; */
    0x04,       /*  uint8_t  if_bDescriptorType; Interface */
    0x00,       /*  uint8_t  if_bInterfaceNumber; */
    0x00,       /*  uint8_t  if_bAlternateSetting; */
    0x01,       /*  uint8_t  if_bNumEndpoints; */
    0x09,       /*  uint8_t  if_bInterfaceClass; HUB_CLASSCODE */
    0x00,       /*  uint8_t  if_bInterfaceSubClass; */
    0x00,       /*  uint8_t  if_bInterfaceProtocol; */
    0x00,       /*  uint8_t  if_iInterface; */

    /* endpoint */
    0x07,       /*  uint8_t  ep_bLength; */
    0x05,       /*  uint8_t  ep_bDescriptorType; Endpoint */
    0x81,       /*  uint8_t  ep_bEndpointAddress; IN Endpoint 1 */
    0x03,       /*  uint8_t  ep_bmAttributes; Interrupt */
    0x02,       /*  uint16_t ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
    0x00,
    0xff        /*  uint8_t  ep_bInterval; 255 ms */
};

/* Hub class-specific descriptor is constructed dynamically */


/*-------------------------------------------------------------------------*/

/* prepare Interrupt pipe data; HUB INTERRUPT ENDPOINT */

static int  rh_send_irq(void *rh_data, int rh_len)
{
    int     num_ports;
    int     i;
    int     ret;
    int     len;
    uint8_t   data[8];

    num_ports = halUSBH_Get_HcRhDescriptorA() & RH_A_NDP;

    /* YCHuang: prepare the 'Hub change detected' bit, refer to USB 1.1 Spec. p.260 */
    *(uint8_t *)data = (halUSBH_Get_HcRhStatus() & (RH_HS_LPSC | RH_HS_OCIC)) ? 1: 0;
    ret = *(uint8_t *)data;

    /* YCHuang: prepare the 'Port change detected' bits, refer to USB 1.1 Spec. p.260 */
    for ( i = 0; i < num_ports; i++) {
        *(uint8_t *)(data + (i + 1) / 8) |= ((halUSBH_Get_HcRhPortStatus(i) &
                                              (RH_PS_CSC | RH_PS_PESC | RH_PS_PSSC | RH_PS_OCIC | RH_PS_PRSC))
                                             ? 1: 0) << ((i + 1) % 8);
        ret += *(uint8_t *)(data + (i + 1) / 8);
    }
    len = i/8 + 1;

    if (ret > 0) {
        memcpy(rh_data, data, min(len, min (rh_len, sizeof(data))));
        return len;
    }
    return 0;
}

/*-------------------------------------------------------------------------*/
static URB_T  *_OHCI_RH_IRQ;

/* Virtual Root Hub INTs are polled by this timer every "interval" ms */

void  ohci_int_timer_do(int ptr)
{
    int     len;
    URB_T   *urb = _OHCI_RH_IRQ;

    if (ohci.rh.send) {
        len = rh_send_irq(urb->transfer_buffer, urb->transfer_buffer_length);
        if (len > 0) {
            urb->actual_length = len;
#ifdef USB_VERBOSE_DEBUG
            urb_print(urb, "RET-t(rh)", usb_pipeout (urb->pipe));
#endif
            if (urb->complete)
                urb->complete(urb);  /* call the hub_irq() in hub.c */
        }
    }
}



/* for returning string descriptors in UTF-16LE */
static int  ascii2utf(char *ascii, uint8_t *utf, int utfmax)
{
    int retval;

    for (retval = 0; *ascii && utfmax > 1; utfmax -= 2, retval += 2) {
        *utf++ = *ascii++ & 0x7f;
        *utf++ = 0;
    }
    return retval;
}



/*
 *  root_hub_string is used by each host controller's root hub code,
 *  so that they're identified consistently throughout the system.
 */
static int usbh_root_hub_string(int id, int serial, char *type, uint8_t *data, int len)
{
    char buf[30];

    /* language ids */
    if (id == 0) {
        *data++ = 4;
        *data++ = 3;       /* 4 bytes data */
        *data++ = 0;
        *data++ = 0;       /* some language id */
        return 4;
        // serial number
    } else if (id == 1) {
        snprintf (buf, sizeof(buf), "%x", serial);
        /* product description */
    } else if (id == 2) {
        snprintf (buf, sizeof(buf), "USB %s Root Hub", type);
        /* id 3 == vendor description */
        /* unsupported IDs --> "stall" */
    } else
        return 0;

    data [0] = 2 + ascii2utf(buf, data + 2, len - 2);
    data [1] = 3;
    return data [0];
}


/*-------------------------------------------------------------------------*/

#define RH_OK(x)                len = (x); break
#define WR_RH_STAT(x)           halUSBH_Set_HcRhStatus(x)
#define WR_RH_PORTSTAT(x)       halUSBH_Set_HcRhPortStatus (wIndex-1, x)
#define RD_RH_STAT              halUSBH_Get_HcRhStatus()
#define RD_RH_PORTSTAT          halUSBH_Get_HcRhPortStatus(wIndex-1)

/* request to virtual root hub */

static int  rh_submit_urb(URB_T *urb)
{
    uint32_t    pipe;
    DEV_REQ_T   *cmd;
    void        *data;
    int         leni;
    int         len = 0;
    int         status = TD_CC_NOERROR;
    uint32_t    datab[4];
    uint8_t     *data_buf;
    uint16_t    bmRType_bReq;
    uint16_t    wValue;
    uint16_t    wIndex;
    uint16_t    wLength;

    pipe = urb->pipe;
    cmd = (DEV_REQ_T *) urb->setup_packet;
    data = urb->transfer_buffer;
    leni = urb->transfer_buffer_length;
    data_buf = (uint8_t *) datab;

    if (usb_pipeint(pipe)) { /* URB interrupt pipe */
        ohci.rh.urb =  urb;
        ohci.rh.send = 1;
        ohci.rh.interval = urb->interval;
        _OHCI_RH_IRQ = urb;

        ohci_int_timer_do(0);

        urb->status = 0;
        return 0;
    }

    bmRType_bReq  = (cmd->requesttype << 8) | cmd->request;
    wValue        = cmd->value;
    wIndex        = cmd->index;
    wLength       = cmd->length;
    USB_info("rh_submit_urb, req = 0x%x len=%d\n", bmRType_bReq, wLength);

#ifdef  USB_VERBOSE_DEBUG   /* YCHuang: added for debug */
    USB_debug("Command - ");
    switch (bmRType_bReq & 0x80FF) {
    case RH_GET_STATUS:
        USB_debug("GET_STATUS\n");
        break;

    case RH_CLEAR_FEATURE:
        USB_debug("CLEAR_FEATURE\n");
        break;

    case RH_SET_FEATURE:
        USB_debug("SET_FEATURE\n");
        break;

    case RH_SET_ADDRESS:
        USB_debug("SET_ADDRESS\n");
        break;

    case RH_GET_DESCRIPTOR:
        USB_debug("GET_DESCRIPTOR\n");
        break;

    case RH_SET_DESCRIPTOR:
        USB_debug("SET_DESCRIPTOR\n");
        break;

    case RH_GET_CONFIGURATION:
        USB_debug("GET_CONFIGURATION\n");
        break;

    case RH_SET_CONFIGURATION:
        USB_debug("RH_SET_CONFIGURATION\n");
        break;

    case RH_GET_STATE:
        USB_debug("GET_STATE\n");
        break;

    case RH_GET_INTERFACE:
        USB_debug("GET_INTERFACE\n");
        break;

    case RH_SET_INTERFACE:
        USB_debug("SET_INTERFACE\n");
        break;

    case RH_SYNC_FRAME:
        USB_debug("SYNC_FRAME\n");
        break;

    case RH_SET_EP:
        USB_debug("SET_EP\n");
        break;
    }
#endif

    switch (bmRType_bReq) {
    /* Request Destination:
       without flags: Device,
       RH_INTERFACE: interface,
       RH_ENDPOINT: endpoint,
       RH_CLASS means HUB here,
       RH_OTHER | RH_CLASS  almost ever means HUB_PORT here
    */

    case RH_GET_STATUS:
        *(uint16_t *) data_buf = USB_SWAP16(1);
        RH_OK(2);

    case RH_GET_STATUS | RH_INTERFACE:
        *(uint16_t *) data_buf = 0;
        RH_OK(2);

    case RH_GET_STATUS | RH_ENDPOINT:
        *(uint16_t *) data_buf = 0;
        RH_OK(2);

    case RH_GET_STATUS | RH_CLASS:
        /* BIGBIG */
        data_buf[0] = RD_RH_STAT & 0xFF;
        data_buf[1] = (RD_RH_STAT >> 8) & 0xFF;
        data_buf[2] = (RD_RH_STAT >> 16) & 0xFF;
        data_buf[3] = (RD_RH_STAT >> 24) & 0xFF;
        *(uint32_t *) data_buf = RD_RH_STAT & ~(RH_HS_CRWE | RH_HS_DRWE);
        RH_OK(4);

    case RH_GET_STATUS | RH_OTHER | RH_CLASS:
        *(uint32_t *) data_buf = RD_RH_PORTSTAT;
        RH_OK(4);

    case RH_CLEAR_FEATURE | RH_ENDPOINT:
        switch (wValue) {
        case (RH_ENDPOINT_STALL):
            RH_OK(0);
        }
        break;

    case RH_CLEAR_FEATURE | RH_CLASS:
        switch (wValue) {
        case RH_C_HUB_LOCAL_POWER:
            RH_OK(0);
        case (RH_C_HUB_OVER_CURRENT):
            WR_RH_STAT(RH_HS_OCIC);
            RH_OK(0);
        }
        break;

    case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
        switch (wValue) {
        case (RH_PORT_ENABLE):
            WR_RH_PORTSTAT (RH_PS_CCS );
            RH_OK(0);
        case (RH_PORT_SUSPEND):
            WR_RH_PORTSTAT (RH_PS_POCI);
            RH_OK(0);
        case (RH_PORT_POWER):
            WR_RH_PORTSTAT (RH_PS_LSDA);
            RH_OK(0);
        case (RH_C_PORT_CONNECTION):
            WR_RH_PORTSTAT (RH_PS_CSC );
            RH_OK(0);
        case (RH_C_PORT_ENABLE):
            WR_RH_PORTSTAT (RH_PS_PESC);
            RH_OK(0);
        case (RH_C_PORT_SUSPEND):
            WR_RH_PORTSTAT (RH_PS_PSSC);
            RH_OK(0);
        case (RH_C_PORT_OVER_CURRENT):
            WR_RH_PORTSTAT (RH_PS_OCIC);
            RH_OK(0);
        case (RH_C_PORT_RESET):
            WR_RH_PORTSTAT (RH_PS_PRSC);
            RH_OK(0);
        }
        break;

    case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
        switch (wValue) {
        case (RH_PORT_SUSPEND):
            WR_RH_PORTSTAT (RH_PS_PSS );
            RH_OK(0);

        case (RH_PORT_RESET): /* BUG IN HUP CODE *********/
            if (RD_RH_PORTSTAT & RH_PS_CCS)
                WR_RH_PORTSTAT (RH_PS_PRS);
            RH_OK(0);

        case (RH_PORT_POWER):
            WR_RH_PORTSTAT (RH_PS_PPS );
            RH_OK(0);

        case (RH_PORT_ENABLE): /* BUG IN HUP CODE *********/
            if (RD_RH_PORTSTAT & RH_PS_CCS)
                WR_RH_PORTSTAT (RH_PS_PES );
            RH_OK(0);
        }
        break;

    case RH_SET_ADDRESS:
        ohci.rh.devnum = wValue;
        RH_OK(0);

    case RH_GET_DESCRIPTOR:
        switch ((wValue & 0xff00) >> 8) {
        case (0x01): /* device descriptor */
            len = min (leni, min (sizeof (root_hub_dev_des), wLength));
            data_buf = root_hub_dev_des;
            RH_OK(len);

        case (0x02): /* configuration descriptor */
            len = min (leni, min (sizeof (root_hub_config_des), wLength));
            data_buf = root_hub_config_des;
            RH_OK(len);

        case (0x03): /* string descriptors */
            len = usbh_root_hub_string(wValue & 0xff, 0x0, "OHCI", data, wLength);
            if (len > 0) {
                data_buf = data;
                RH_OK(min (leni, len));
            }
        // else fallthrough
        default:
            status = TD_CC_STALL;
        }
        break;

    case RH_GET_DESCRIPTOR | RH_CLASS: {
        uint32_t temp = halUSBH_Get_HcRhDescriptorA();

        data_buf [0] = 9;           // min length;
        data_buf [1] = 0x29;
        data_buf [2] = temp & RH_A_NDP;
        data_buf [3] = 0;
        if (temp & RH_A_PSM)        /* per-port power switching? */
            data_buf [3] |= 0x1;
        if (temp & RH_A_NOCP)       /* no overcurrent reporting? */
            data_buf [3] |= 0x10;
        else if (temp & RH_A_OCPM)  /* per-port overcurrent reporting? */
            data_buf [3] |= 0x8;

        datab [1] = 0;
        data_buf [5] = (temp & RH_A_POTPGT) >> 24;
        temp = halUSBH_Get_HcRhDescriptorB();
        data_buf [7] = temp & RH_B_DR;
        if (data_buf [2] < 7) {
            data_buf [8] = 0xff;
        } else {
            data_buf [0] += 2;
            data_buf [8] = (temp & RH_B_DR) >> 8;
            data_buf [10] = data_buf [9] = 0xff;
        }
        len = min (leni, min (data_buf [0], wLength));
        RH_OK(len);
    }

    case RH_GET_CONFIGURATION:
        *(uint8_t *) data_buf = 0x01;
        RH_OK(1);

    case RH_SET_CONFIGURATION:
        WR_RH_STAT(0x10000);  /* set global port power */
        RH_OK(0);

    default:
        //USB_error("Error! - unsupported root hub command\n");
        status = TD_CC_STALL;
    }

    len = min(len, leni);
    if (data != data_buf)
        memcpy(data, data_buf, len);
    urb->actual_length = len;
    urb->status = cc_to_error[status];

    urb->dev = NULL;
    if (urb->complete)
        urb->complete (urb);

    return 0;
}

/*-------------------------------------------------------------------------*/

static int  rh_unlink_urb(URB_T *urb)
{
    if (ohci.rh.urb == urb) {
        ohci.rh.send = 0;
        ohci.rh.urb = NULL;

        urb->dev = NULL;
        if (urb->transfer_flags & USB_ASYNC_UNLINK) {
            urb->status = USB_ERR_CONNRESET;
            if (urb->complete)
                urb->complete (urb);
        } else
            urb->status = USB_ERR_NOENT;
    }
    return 0;
}

/*-------------------------------------------------------------------------*
 * HC functions
 *-------------------------------------------------------------------------*/

/* reset the HC and BUS */

static int  ohci_reset()
{
    int    timeout = 30;

    /* Disable HC interrupts */
    halUSBH_Set_HcInterruptDisable(OHCI_INTR_MIE);

    /* Reset USB (needed by some controllers) */
    halUSBH_Set_HcControl(0);

    /* HC Reset requires max 10 ms delay */
    halUSBH_Set_HcCommandStatus(OHCI_HCR);

    while ((halUSBH_Get_HcCommandStatus() & OHCI_HCR) != 0) {
        if (--timeout == 0) {
            USB_error("Error! - USB HC reset timed out!\n");
            return -1;
        }
        usbh_mdelay(10);
    }
    halUSBH_Set_HcControl((halUSBH_Get_HcControl() & 0xffffff3f) | OHCI_USB_RESET);

    return 0;
}


/*-------------------------------------------------------------------------*/
void USBH_IRQHandler(void)
{
    int     ints;

    _is_in_interrupt = 1;

    ints = halUSBH_Get_HcInterruptStatus();

    //printf("[irq-0x%x] 0x%x, 0x%x\n", ints, USBH->HcRhPortStatus[0], USBH->HcRhPortStatus[1]);

    if ((ohci.hcca->done_head != 0) && !((uint32_t)(ohci.hcca->done_head) & 0x01)) {
        ints =  OHCI_INTR_WDH;
    }

    if (ints & OHCI_INTR_UE) {
        ohci.disabled++;
        USB_error("Error! - OHCI Unrecoverable Error, controller disabled\n");
        /* e.g. due to PCI Master/Target Abort */
        /*
         * FIXME: be optimistic, hope that bug won't repeat often.
         * Make some non-interrupt context restart the controller.
         * Count and limit the retries though; either hardware or
         * software errors can go forever...
         */
        ohci_reset();
    }

    if (ints & OHCI_INTR_OC) {
        USB_debug("Ownership change!!\n");
    }

    if (ints & OHCI_INTR_WDH) {
        dl_done_list(dl_reverse_done_list());
        halUSBH_Set_HcInterruptStatus(OHCI_INTR_WDH);
    }

    if (ints & OHCI_INTR_SO) {
        //*(uint32_t *)0x600000 = 0x1;
        //while (1);
        //USB_error("Error! - USB Schedule overrun, count:%d\n", (USBH->HcCommandStatus >> 16) & 0x3);
        halUSBH_Set_HcInterruptDisable(OHCI_INTR_SO);
        halUSBH_Set_HcInterruptStatus(OHCI_INTR_SO);
    }

    if (ints & OHCI_INTR_SF) {
        uint32_t frame = ohci.hcca->frame_no & 1;

        halUSBH_Set_HcInterruptDisable(OHCI_INTR_SF);
        if (ohci.ed_rm_list[!frame] != NULL) {
            dl_del_list(!frame);
        }
        if (ohci.ed_rm_list[frame] != NULL)
            halUSBH_Set_HcInterruptEnable(OHCI_INTR_SF);
    }

    if (ints & OHCI_INTR_RHSC) {
        halUSBH_Set_HcInterruptDisable(OHCI_INTR_RHSC);
        halUSBH_Set_HcInterruptStatus(OHCI_INTR_RHSC);
    }

    if (ints & OHCI_INTR_RD) {
        halUSBH_Set_HcInterruptDisable(OHCI_INTR_RD);
        halUSBH_Set_HcInterruptStatus(OHCI_INTR_RD);
    }

    ints &= ~OHCI_INTR_WDH;
    halUSBH_Set_HcInterruptEnable(OHCI_INTR_MIE);

    /* FIXME:  check URB timeouts */
    _is_in_interrupt = 0;
}


/*-------------------------------------------------------------------------*/

/* Start an OHCI controller, set the BUS operational
 * enable interrupts
 * connect the virtual root hub */

static int  hc_start ()
{
    uint32_t    mask;
    uint32_t    fminterval;
    int         i;
    USB_DEV_T   *usb_dev;

    ohci.disabled = 1;

    /* empty the interrupt branches */
    for (i = 0; i < NUM_INTS; i++)
        ohci.ohci_int_load[i] = 0;
    for (i = 0; i < NUM_INTS; i++)
        ohci.hcca->int_table[i] = 0;

    /* no EDs to remove */
    ohci.ed_rm_list [0] = NULL;
    ohci.ed_rm_list [1] = NULL;

    /* empty control and bulk lists */
    ohci.ed_isotail     = NULL;
    ohci.ed_controltail = NULL;
    ohci.ed_bulktail    = NULL;

    /* Tell the controller where the control and bulk lists are
     * The lists are empty now. */
    halUSBH_Set_HcControlHeadED(0);         /* control ED list head */
    halUSBH_Set_HcBulkHeadED(0);            /* bulk ED list head */

    halUSBH_Set_HcHCCA((uint32_t)ohci.hcca); /* a reset clears this */

    fminterval = 0x2edf;    /* 11,999 */

    /* periodic start 90% of frame interval */
    halUSBH_Set_HcPeriodicStart((fminterval * 9) / 10);

    /* set FSLargestDataPacket, 10,104 for 0x2edf frame interval */
    fminterval |= ((((fminterval - 210) * 6) / 7) << 16);
    halUSBH_Set_HcFmInterval(fminterval);

    halUSBH_Set_HcLSThreshold(0x628);

    /* start controller operations */
    ohci.hc_control = OHCI_CONTROL_INIT | OHCI_USB_OPER;
    ohci.disabled = 0;
    halUSBH_Set_HcControl(ohci.hc_control);

    /* Choose the interrupts we care about now, others later on demand */
    mask = OHCI_INTR_MIE | OHCI_INTR_UE | OHCI_INTR_WDH | OHCI_INTR_SO;
    halUSBH_Set_HcInterruptStatus(mask);
    halUSBH_Set_HcInterruptEnable(mask);

    /* global port power mode */
    halUSBH_RootHubInit();

    /* POTPGT delay is bits 24-31, in 2 ms units. */
    usbh_mdelay(halUSBH_Get_PowerOnToPowerGoodTime()<<1);

    /* connect the virtual root hub */
    ohci.rh.devnum = 0;
    usb_dev = usbh_alloc_device(NULL, ohci.bus);
    if (!usb_dev) {
        ohci.disabled = 1;
        return USB_ERR_NOMEM;
    }

    ohci.bus->root_hub = usb_dev;
    usbh_connect_device(usb_dev);
    if (usbh_settle_new_device(usb_dev) != 0) {
        usbh_free_device(usb_dev);
        ohci.disabled = 1;
        return USB_ERR_NODEV;
    }
    return 0;
}


#if 0
static void  hc_restart()
{
    int    temp;
    int    i;

    ohci.disabled = 1;
    if (ohci.bus->root_hub)
        usbh_disconnect_device(&ohci.bus->root_hub);

    /* empty the interrupt branches */
    for (i = 0; i < NUM_INTS; i++)
        ohci.ohci_int_load[i] = 0;
    for (i = 0; i < NUM_INTS; i++)
        ohci.hcca->int_table[i] = 0;

    /* no EDs to remove */
    ohci.ed_rm_list [0] = NULL;
    ohci.ed_rm_list [1] = NULL;

    /* empty control and bulk lists */
    ohci.ed_isotail     = NULL;
    ohci.ed_controltail = NULL;
    ohci.ed_bulktail    = NULL;

    if ((temp = ohci_reset()) < 0 || (temp = hc_start ()) < 0)
        USB_error("Error! - can't restart usb-%s, %d\n", "ohci->ohci_dev->slot_name", temp);
    else
        USB_error("Error! - restart usb-%s completed\n", "ohci->ohci_dev->slot_name");
}
#endif


/*-------------------------------------------------------------------------*/

int  usbh_init_ohci()
{
    int   i;

    if (sizeof(ED_T) != 32) {
        USB_error("Wrong ED_T size!!!\n");
        return -1;
    }

    if (sizeof(TD_T) != 64) {
        USB_error("Wrong TD_T size!!!\n");
        return -1;
    }

    memset((char *)&ohci, 0, sizeof(OHCI_T));
    ohci.hcca = (OHCI_HCCA_T *)g_ohci_hcca;
    memset((char *)ohci.hcca, 0, 256);

    for (i = 0; i < DEV_MAX_NUM; i++)
        ohci_dev_alloc_mark[i] = 0;

    ohci.disabled = 1;

    if (ohci_reset() < 0)
        return USB_ERR_NODEV;

    ohci.hc_control = OHCI_USB_RESET;
    halUSBH_Set_HcControl(ohci.hc_control);

    /*- HCD must wait 10ms for HC reset complete -*/
    usbh_mdelay(100);

    /*
     * OHCI bus ...
     */
    g_ohci_bus.op = &sohci_device_operations;
    g_ohci_bus.root_hub = NULL;
    g_ohci_bus.hcpriv = &ohci;
    ohci.bus = &g_ohci_bus;

    if (hc_start() < 0) {
        USB_error("Error! - can't start OHCI!\n");
        return USB_ERR_BUSY;
    }
    return 0;
}

