#ifndef API_HID_H
#define API_HID_H

#include <stdint.h>
#include "usbh_core.h"

#define HID_DEBUG

#ifdef HID_DEBUG
#define HID_DBGMSG      printk
#else
#define HID_DBGMSG(...)
#endif


#define CONFIG_HID_MAX_DEV            3     /*!< Maximum number of HID device. */
#define HID_MAX_BUFFER_SIZE           64    /*!< HID interrupt in transfer buffer size */


/*
 * Constants
 */
#define HID_RET_OK                      0   /*!< Return with no errors. */
#define HID_RET_DEV_NOT_FOUND          -9   /*!< HID device not found or removed. */
#define HID_RET_IO_ERR                -11   /*!< USB transfer failed. */
#define HID_RET_INVALID_PARAMETER     -13   /*!< Invalid parameter. */
#define HID_RET_OUT_OF_MEMORY         -15   /*!< Out of memory. */
#define HID_RET_NOT_SUPPORTED         -17   /*!< Function not supported. */


#define HID_REPORT_GET                0x01  /*!< Get_Report_Request code. */
#define HID_GET_IDLE                  0x02  /*!< Get_Idle code. */
#define HID_GET_PROTOCOL              0x03  /*!< Get_Protocol code. */
#define HID_REPORT_SET                0x09  /*!< Set_Report_Request code. */
#define HID_SET_IDLE                  0x0A  /*!< Set_Idle code. */
#define HID_SET_PROTOCOL              0x0B  /*!< Set_Protocol code. */


/*
 * Report type
 */
#define RT_INPUT          1      /*!< Report type: Input    \ */
#define RT_OUTPUT         2      /*!< Report type: Output   \ */
#define RT_FEATURE        3      /*!< Report type: Feature  \ */



struct usbhid_dev;               /*!< HID device structure  \             */



typedef void (HID_IR_FUNC)(struct usbhid_dev *hdev, uint8_t *rdata, int data_len);      /*!< interrupt in callback function */
typedef void (HID_IW_FUNC)(struct usbhid_dev *hdev, uint8_t **wbuff, int *buff_size);   /*!< interrupt out callback function */



/*-----------------------------------------------------------------------------------
 *  HID device
 */
typedef struct usbhid_dev {             /*! HID device structure              */
    USB_DEV_T           *udev;          /*!< USB device pointer of HID_DEV_T  */
    USB_IF_DESC_T       if_desc;        /*!< USB interface descriptor         */
    int                 ifnum;          /*!< Interface numder                 */
    URB_T               *urbin;         /*!< Input URB                        */
    URB_T               *urbout;        /*!< Output URB                       */
    uint8_t             inbuf[HID_MAX_BUFFER_SIZE];  /*!< Input buffer        */
    HID_IR_FUNC         *read_func;     /*!< Interrupt-in callback function   */
    HID_IW_FUNC         *write_func;    /*!< Interrupt-out callback function  */
    struct usbhid_dev   *next;          /*!< Point to the next HID device     */
} HID_DEV_T;                            /*! HID device structure              */

#ifdef __cplusplus
extern "C" {
#endif
/**
  * @brief    Init USB Host HID driver.
  * @return   None
  */
extern void     USBH_HidInit(void);

/**
 *  @brief   Get a list of currently connected USB Hid devices.
 *  @return  List of HID devices.
 *  @retval  NULL       There's no HID device found.
 *  @retval  Otherwise  A list of connected HID devices.
 *
 *  The HID devices are chained by the "next" member of HID_DEV_T.
 */
extern HID_DEV_T * USBH_HidGetDeviceList(void);

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
extern int32_t  HID_HidGetReportDescriptor(HID_DEV_T *hdev, uint8_t *desc_buf, int buf_max_len);

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
extern int32_t  HID_HidGetReport(HID_DEV_T *hdev, int rtp_typ, int rtp_id, uint8_t *data, int len);

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
extern int32_t  HID_HidSetReport(HID_DEV_T *hdev, int rtp_typ, int rtp_id, uint8_t *data, int len);

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
extern int32_t  HID_HidGetIdle(HID_DEV_T *hdev, int rtp_id, uint8_t *idle_rate);

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
extern int32_t  HID_HidSetIdle(HID_DEV_T *hdev, int rtp_id, uint8_t idle_rate);

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
extern int32_t  HID_HidGetProtocol(HID_DEV_T *hdev, uint8_t *protocol);

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
extern int32_t  HID_HidSetProtocol(HID_DEV_T *hdev, uint8_t protocol);

/**
 *  @brief  Start purge the USB interrupt in transfer.
 *  @param[in] hdev       HID device
 *  @param[in] func       The interrupt in data receiver callback function.
 *  @return   Success or not.
 *  @retval    0          Success
 *  @retval    Otherwise  Failed
 */
extern int32_t  USBH_HidStartIntReadPipe(HID_DEV_T *hdev, HID_IR_FUNC *func);

/**
 *  @brief  Start purge the USB interrupt out transfer.
 *  @param[in] hdev       HID device
 *  @param[in] func       The interrupt in data transfer callback function.
 *  @return   Success or not.
 *  @retval   0           Success
 *  @retval   Otherwise   Failed
 */
extern int32_t  USBH_HidStartIntWritePipe(HID_DEV_T *hdev, HID_IW_FUNC *func);
#ifdef __cplusplus
}
#endif

#endif
