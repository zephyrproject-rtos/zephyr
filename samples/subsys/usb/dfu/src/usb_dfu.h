/***************************************************************************
 *
 * Copyright(c) 2015,2016 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/

/**
 * @file
 * @brief USB DFU device class driver header file
 *
 * Header file for USB DFU device class driver
 */

#ifndef __USB_DFU_H__
#define __USB_DFU_H__

#include <device.h>
#include <usb/usb_common.h>

/* Intel vendor ID */
#define DFU_VENDOR_ID   0x8086

/* Product Id, random value */
#define DFU_PRODUCT_ID  0x48FC

#define DFU_MAX_XFER_SIZE 256

#define DFU_NUM_CONF    0x01   /* Number of configurations for the USB Device */
#define DFU_NUM_ITF     0x01   /* Number of interfaces in the configuration */
#define DFU_NUM_EP      0x00   /* Number of Endpoints in the interface */

/* Class specific request */
#define DFU_DETACH          0x00
#define DFU_DNLOAD          0x01
#define DFU_UPLOAD          0x02
#define DFU_GETSTATUS       0x03
#define DFU_CLRSTATUS       0x04
#define DFU_GETSTATE        0x05
#define DFU_ABORT           0x06


/* DFU attributes */
#define DFU_ATTR_CAN_DNLOAD 0x01
#define DFU_ATTR_CAN_UPLOAD 0x02
#define DFU_ATTR_MANIFESTATION_TOLERANT 0x4

#define DFU_DETACH_TIMEOUT  1000

/***** STATUS CODE ****/
enum dfu_status {
	statusOK,
	errTARGET,
	errFILE,
	errWRITE,
	errERASE,
	errCHECK_ERASED,
	errPROG,
	errVERIFY,
	errADDRESS,
	errNOTDONE,
	errFIRMWARE,
	errVENDOR,
	errUSB,
	errPOR,
	errUNKNOWN,
	errSTALLEDPKT
};

/**** STATES ****/
enum dfu_state {
	appIDLE,
	appDETACH,
	dfuIDLE,
	dfuDNLOAD_SYNC,
	dfuDNBUSY,
	dfuDNLOAD_IDLE,
	dfuMANIFEST_SYNC,
	dfuMANIFEST,
	dfuMANIFEST_WAIT_RST,
	dfuUPLOAD_IDLE,
	dfuERROR,
};

/* Number of DFU interface alternate settings. */
#define DFU_RUNTIME_ALTERNATE_SETTINGS  1
#define DFU_MODE_ALTERNATE_SETTINGS     3

/* Size in bytes of the configuration sent to the Host on
 * GetConfiguration() request
 * For DFU: CONF + ITF*ALT_SETTINGS + DFU)
 */
#define DFU_MODE_CONF_SIZE   (USB_CONFIGURATION_DESC_SIZE +    \
	USB_INTERFACE_DESC_SIZE * DFU_MODE_ALTERNATE_SETTINGS + \
	USB_DFU_DESC_SIZE)

#define DFU_RUNTIME_CONF_SIZE   (USB_CONFIGURATION_DESC_SIZE + \
	USB_INTERFACE_DESC_SIZE * DFU_RUNTIME_ALTERNATE_SETTINGS +    \
	USB_DFU_DESC_SIZE)

/* Alternate settings are used to access additional memory segments.
 * This example uses the alternate settings as an offset into flash.
 */
#define DFU_ALT_SETTING_OFFSET 0x6000

/**
 * @brief DFU class driver start routine
 *
 * @param flash_dev         Flash device driver to be used.
 * @param flash_base_addr   Flash address to write/read to/from.
 * @param flash_page_size   Flash page size.
 * @param flash_upload_size Flash upload data size.
 *
 * @return  N/A.
 */
int dfu_start(struct device *flash_dev, u32_t flash_base_addr,
		u32_t flash_page_size, u32_t flash_upload_size);

#endif /* __USB_DFU_H__ */
