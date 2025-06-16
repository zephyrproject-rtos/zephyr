/**************************************************************************//**
 * @file     hid_core.c
 * @version  V1.00
 * $Revision: 12 $
 * $Date: 14/10/07 5:47p $
 * @brief    NCT6 MCU USB Host HID library core.
 *
 * @note
 * Copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "apiHID.h"

/** @addtogroup NCT6_Device_Driver NCT6 Device Driver
  @{
*/

/** @addtogroup NCT6_USBH_HID_Driver USB Host HID Driver
  @{
*/

/** @addtogroup NCT6_USBH_HID_EXPORTED_FUNCTIONS USB Host HID Driver Exported Functions
  @{
*/


/// @cond HIDDEN_SYMBOLS

#define USB_TIMEOUT             10000

extern HID_DEV_T *find_hid_deivce_by_udev(USB_DEV_T *udev);
extern int  find_hid_device(HID_DEV_T *hdev);
extern EP_INFO_T *hid_get_ep_info(USB_DEV_T *dev, int ifnum, uint16_t dir);


static int usb_snd_control_msg(HID_DEV_T *hdev, int requesttype, int request,
                               int value, int index, char *bytes, int size, int timeout)
{
    USB_DEV_T       *udev = hdev->udev;

    return USBH_SendCtrlMsg(udev, usb_sndctrlpipe(udev, 0),
                            request, requesttype, value, index, bytes, size, timeout);
}

int usb_rcv_control_msg(HID_DEV_T *hdev, int requesttype, int request,
                        int value, int index, char *bytes, int size, int timeout)
{
    USB_DEV_T       *udev = hdev->udev;

    return USBH_SendCtrlMsg(udev, usb_rcvctrlpipe(udev, 0),
                            request, requesttype, value, index, bytes, size, timeout);
}

/// @endcond


/**
 *  @brief  Get report descriptor request.
 *  @param[in]  hdev         HID device
 *  @param[out] desc_buf     The data buffer to store report descriptor.
 *  @param[in]  buf_max_len  The maximum length of desc_buf. This function will read more data
 *                           than buf_max_len.
 *  @return   Report descriptor length or error code.
 *  @retval   <0        Failed
 *  @retval   Otherwise  Length of report descriptor read.
 */
int32_t  HID_HidGetReportDescriptor(HID_DEV_T *hdev, uint8_t *desc_buf, int buf_max_len)
{
    int  len;

    if (buf_max_len < 9)
        return HID_RET_INVALID_PARAMETER;

    len = usb_rcv_control_msg(hdev,
                              USB_DIR_IN+1,
                              USB_REQ_GET_DESCRIPTOR,
                              (USB_DT_HID << 8) + 0, hdev->ifnum,
                              (char*)desc_buf, buf_max_len,
                              USB_TIMEOUT);

    if (len < 0) {
        HID_DBGMSG("failed to get HID descriptor.\r\n");
        return HID_RET_IO_ERR;
    }

    len = desc_buf[7] | (desc_buf[8] << 8);

    HID_DBGMSG("Report descriptor size is %d\r\n", len);

    if (buf_max_len < len)
        return HID_RET_INVALID_PARAMETER;

    len = usb_rcv_control_msg(hdev,
                              USB_DIR_IN+1,
                              USB_REQ_GET_DESCRIPTOR,
                              (USB_DT_REPORT << 8) + 0, hdev->ifnum,
                              (char*)desc_buf, len,
                              USB_TIMEOUT);

    if (len < 0) {
        HID_DBGMSG("failed to get HID descriptor.\r\n");
        return HID_RET_IO_ERR;
    }

    HID_DBGMSG("successfully initialised HID descriptor %d bytes.\r\n", len);

    return len;
}


/**
 * @brief  HID class standard request Get_Report request. The Get_Report request
 *         allows the host to receive a report via the Control pipe.
 *
 * @param[in] hdev       HID device
 * @param[in] rtp_typ    Report type. Valid values are:
 *                       - \ref RT_INPUT
 *                       - \ref RT_OUTPUT
 *                       - \ref RT_FEATURE
 * @param[in] rtp_id     Report ID
 * @param[out] data      Buffer to store data retrieved from this report ID.
 * @param[in] len        Report length.
 * @return   Report length or error code.
 * @retval   >=0         The actual length of data obtained from this report ID.
 * @retval   Otherwise   Failed
 */
int32_t  HID_HidGetReport(HID_DEV_T *hdev, int rtp_typ, int rtp_id,
                          uint8_t *data, int len)
{
    len = usb_rcv_control_msg(hdev,
                              USB_DIR_IN + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                              HID_REPORT_GET,
                              rtp_id + (rtp_typ << 8),
                              hdev->ifnum,
                              (char *)data, len, USB_TIMEOUT);

    if (len < 0) {
        HID_DBGMSG("failed to get report!\r\n");
        return HID_RET_IO_ERR;
    }
    return len;
}


/**
 * @brief  HID class standard request Set_Report request. The Set_Report
 *         request allows the host to send a report to the device, possibly
 *         setting the state of input, output, or feature controls.
 *
 * @param[in] hdev       HID device
 * @param[in] rtp_typ    Report type. Valid values are:
 *                       - \ref RT_INPUT
 *                       - \ref RT_OUTPUT
 *                       - \ref RT_FEATURE
 * @param[in] rtp_id     Report ID
 * @param[out] data      Buffer store data to be send.
 * @param[in] len        Report length.
 * @return   Written length or error code.
 * @retval   >=0         The actual length of data written to this report ID.
 * @retval   Otherwise   Failed
 */
int32_t  HID_HidSetReport(HID_DEV_T *hdev, int rtp_typ, int rtp_id,
                          uint8_t *data, int len)
{
    len = usb_snd_control_msg(hdev,
                              USB_DIR_OUT + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                              HID_REPORT_SET,
                              rtp_id + (rtp_typ << 8),
                              hdev->ifnum,
                              (char *)data, len, USB_TIMEOUT);

    if (len < 0) {
        HID_DBGMSG("failed to set report!\r\n");
        return HID_RET_IO_ERR;
    }
    return len;
}


/**
 * @brief  HID class standard request Get_Idle request. The Get_Idle request
 *         reads the current idle rate for a particular Input report
 *
 * @param[in] hdev       HID device
 * @param[in] rtp_id     Report ID
 * @param[out] idle_rate An one byte buffer holds the reported idle rate.
 * @return   Success or not.
 * @retval   0           Success
 * @retval   Otherwise   Failed
 */
int32_t  HID_HidGetIdle(HID_DEV_T *hdev, int rtp_id, uint8_t *idle_rate)
{
    int   len;

    len = usb_rcv_control_msg(hdev,
                              USB_DIR_IN + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                              HID_GET_IDLE,
                              rtp_id,
                              hdev->ifnum,
                              (char *)idle_rate, 1, USB_TIMEOUT);

    if (len != 1) {
        HID_DBGMSG("failed to get idle rate! %d\r\n", len);
        return HID_RET_IO_ERR;
    }
    return HID_RET_OK;
}


/**
 * @brief  HID class standard request Set_Idle request. The Set_Idle request
 *         silences a particular report on the Interrupt In pipe until a
 *         new event occurs or the specified amount of time passes.
 *
 * @param[in] hdev       HID device
 * @param[in] rtp_id     Report ID
 * @param[out] idle_rate The idle rate to be set.
 * @return   Success or not.
 * @retval   0           Success
 * @retval   Otherwise   Failed
 */
int32_t  HID_HidSetIdle(HID_DEV_T *hdev, int rtp_id, uint8_t idle_rate)
{
    int       ret;
    uint16_t  wValue = idle_rate;

    ret = usb_snd_control_msg(hdev,
                              USB_DIR_OUT + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                              HID_GET_IDLE,
                              rtp_id + (wValue << 8),
                              hdev->ifnum,
                              NULL, 0, USB_TIMEOUT);

    if (ret < 0) {
        HID_DBGMSG("failed to set idle rate! %d\r\n", ret);
        return HID_RET_IO_ERR;
    }
    return HID_RET_OK;
}


/**
 * @brief  HID class standard request Get_Protocol request. The Get_Protocol
 *         request reads which protocol is currently active (either the boot
 *         protocol or the report protocol.)
 *
 * @param[in] hdev       HID device
 * @param[out] protocol  An one byte buffer holds the protocol code.
 * @return   Success or not.
 * @retval   0           Success
 * @retval   Otherwise   Failed
 */
int32_t  HID_HidGetProtocol(HID_DEV_T *hdev, uint8_t *protocol)
{
    int   len;

    len = usb_rcv_control_msg(hdev,
                              USB_DIR_IN + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                              HID_GET_PROTOCOL,
                              0,
                              hdev->ifnum,
                              (char *)protocol, 1, USB_TIMEOUT);

    if (len != 1) {
        HID_DBGMSG("failed to get protocol! %d\r\n", len);
        return HID_RET_IO_ERR;
    }
    return HID_RET_OK;
}


/**
 * @brief  HID class standard request Set_Protocol request. The Set_Protocol
 *         switches between the boot protocol and the report protocol (or
 *         vice versa).
 *
 * @param[in] hdev       HID device
 * @param[in] protocol   The protocol to be set.
 * @return   Success or not.
 * @retval   0           Success
 * @retval   Otherwise   Failed
 */
int32_t  HID_HidSetProtocol(HID_DEV_T *hdev, uint8_t protocol)
{
    int     ret;

    ret = usb_snd_control_msg(hdev,
                              USB_DIR_OUT + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                              HID_SET_PROTOCOL,
                              protocol,
                              hdev->ifnum,
                              NULL, 0, USB_TIMEOUT);

    if (ret < 0) {
        HID_DBGMSG("failed to set protocol! %d\r\n", ret);
        return HID_RET_IO_ERR;
    }
    return HID_RET_OK;
}


/// @cond HIDDEN_SYMBOLS

/*
 * HID INT-in complete function
 */
static void  hid_read_irq(URB_T *urb)
{
    HID_DEV_T   *hdev;

    //HID_DBGMSG("hid_read_irq. %d\r\n", urb->actual_length);

    hdev = find_hid_deivce_by_udev(urb->dev);
    if (hdev == NULL)
        return;

    if (urb->status) {
        HID_DBGMSG("hid_read_irq - has error: 0x%x\r\n", urb->status);
        return;
    }

    if (hdev->read_func && urb->actual_length)
        hdev->read_func(hdev, urb->transfer_buffer, urb->actual_length);
}

/*
 * HID INT-out complete function
 */
static void  hid_write_irq(URB_T *urb)
{
    HID_DEV_T     *hdev;

    //HID_DBGMSG("hid_write_irq. %d\r\n", urb->actual_length);

    hdev = find_hid_deivce_by_udev(urb->dev);
    if (hdev == NULL)
        return;

    if (urb->status) {
        HID_DBGMSG("hid_write_irq - has error: 0x%x\r\n", urb->status);
        return;
    }

    if (hdev->write_func)
        hdev->write_func(hdev, (uint8_t **)&urb->transfer_buffer, &urb->transfer_buffer_length);
}


/// @endcond HIDDEN_SYMBOLS

/**
 *  @brief  Start purge the USB interrupt in transfer.
 *  @param[in] hdev       HID device
 *  @param[in] func       The interrupt in data receiver callback function.
 *  @return   Success or not.
 *  @retval    0          Success
 *  @retval    Otherwise  Failed
 */
int32_t USBH_HidStartIntReadPipe(HID_DEV_T *hdev, HID_IR_FUNC *func)
{
    EP_INFO_T   *ep_info;
    URB_T       *urb;
    uint32_t    pipe;
    USB_DEV_T   *udev;
    int         maxp, ret;

    if (!func)
        return HID_RET_INVALID_PARAMETER;

    udev = hdev->udev;

    if (hdev->urbin)
        return HID_RET_OUT_OF_MEMORY;

    ep_info = hid_get_ep_info(udev, hdev->ifnum, USB_DIR_IN);
    if (ep_info == NULL) {
        HID_DBGMSG("Interrupt-in endpoint not found in this device!\r\n");
        return HID_RET_NOT_SUPPORTED;
    }

    urb = USBH_AllocUrb();
    if (urb == NULL) {
        HID_DBGMSG("Failed to allocated URB!\r\n");
        return HID_RET_OUT_OF_MEMORY;
    }

    pipe = usb_rcvintpipe(udev, ep_info->bEndpointAddress);
    maxp = usb_maxpacket(udev, pipe, usb_pipeout(pipe));

    HID_DBGMSG("Endpoint 0x%x maximum packet size is %d.\r\n", ep_info->bEndpointAddress, maxp);

    FILL_INT_URB(urb, udev, pipe, &hdev->inbuf[0], maxp, hid_read_irq,
                 hdev, ep_info->bInterval);

    hdev->urbin = urb;
    hdev->read_func = func;

    ret = USBH_SubmitUrb(urb);
    if (ret) {
        HID_DBGMSG("Error - failed to submit interrupt read request (%d)", ret);
        USBH_FreeUrb(urb);
        hdev->urbin = NULL;
        return HID_RET_IO_ERR;
    }

    return HID_RET_OK;
}

/**
 *  @brief  Start purge the USB interrupt out transfer.
 *  @param[in] hdev       HID device
 *  @param[in] func       The interrupt in data transfer callback function.
 *  @return   Success or not.
 *  @retval   0           Success
 *  @retval   Otherwise   Failed
 */
int32_t USBH_HidStartIntWritePipe(HID_DEV_T *hdev, HID_IW_FUNC *func)
{
    EP_INFO_T   *ep_info;
    URB_T       *urb;
    uint32_t    pipe;
    USB_DEV_T   *udev;
    int         maxp, ret;

    if (!func)
        return HID_RET_INVALID_PARAMETER;

    udev = hdev->udev;

    if (hdev->urbout)
        return HID_RET_OUT_OF_MEMORY;

    ep_info = hid_get_ep_info(hdev->udev, hdev->ifnum, USB_DIR_OUT);
    if (ep_info == NULL) {
        HID_DBGMSG("Assigned endpoint address 0x%x not found in this device!\r\n", ep_info->bEndpointAddress);
        return HID_RET_INVALID_PARAMETER;
    }

    urb = USBH_AllocUrb();
    if (urb == NULL) {
        HID_DBGMSG("Failed to allocated URB!\r\n");
        return HID_RET_OUT_OF_MEMORY;
    }

    pipe = usb_sndintpipe(udev, ep_info->bEndpointAddress);
    maxp = usb_maxpacket(udev, pipe, usb_pipeout(pipe));

    HID_DBGMSG("Endpoint 0x%x maximum packet size is %d.\r\n", ep_info->bEndpointAddress, maxp);

    FILL_INT_URB(urb, udev, pipe, NULL, maxp, hid_write_irq,
                 hdev, ep_info->bInterval);

    func(hdev, (uint8_t **)&urb->transfer_buffer, &urb->transfer_buffer_length);

    hdev->urbout = urb;
    hdev->write_func = func;

    ret = USBH_SubmitUrb(urb);
    if (ret) {
        HID_DBGMSG("Error - failed to submit interrupt read request (%d)", ret);
        USBH_FreeUrb(urb);
        hdev->urbout = NULL;
        return HID_RET_IO_ERR;
    }

    return HID_RET_OK;
}


/*@}*/ /* end of group NCT6_USBH_HID_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group NCT6_USBH_HID_Driver */

/*@}*/ /* end of group NCT6_Device_Driver */

/*** (C) COPYRIGHT 2013 Nuvoton Technology Corp. ***/


