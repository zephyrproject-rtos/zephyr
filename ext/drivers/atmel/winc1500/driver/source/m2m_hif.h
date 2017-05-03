/**
 *
 * \file
 *
 * \brief This module contains M2M host interface APIs implementation.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _M2M_HIF_
#define _M2M_HIF_

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "common/include/nm_common.h"
/*!< Include depends on UNO Board is used or not*/
#ifdef ENABLE_UNO_BOARD
#include "m2m_uno_hif.h"
#endif

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#define M2M_HIF_MAX_PACKET_SIZE      (1600 - 4)
/*!< Maximum size of the buffer could be transferred between Host and Firmware.
*/

#define M2M_HIF_HDR_OFFSET (sizeof(tstrHifHdr) + 4)

/**
*	@struct		tstrHifHdr
*	@brief		Structure to hold HIF header
*/
typedef struct
{
    uint8   u8Gid;		/*!< Group ID */
    uint8   u8Opcode;	/*!< OP code */
    uint16  u16Length;	/*!< Payload length */
}tstrHifHdr;

#ifdef __cplusplus
     extern "C" {
#endif

/*!
@typedef typedef void (*tpfHifCallBack)(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr);
@brief	used to point to Wi-Fi call back function depend on Arduino project or other projects.
@param [in]	u8OpCode
				HIF Opcode type.
@param [in]	u16DataSize
				HIF data length.
@param [in]	u32Addr
				HIF address.
@param [in]	grp
				HIF group type.
*/
typedef void (*tpfHifCallBack)(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr);
/**
*   @fn			NMI_API sint8 hif_init(void * arg);
*   @brief
				To initialize HIF layer.
*   @param [in]	arg
*				Pointer to the arguments.
*   @return
				The function shall return ZERO for successful operation and a negative value otherwise.
*/
NMI_API sint8 hif_init(void * arg);
/**
*	@fn			NMI_API sint8 hif_deinit(void * arg);
*	@brief
				To Deinitialize HIF layer.
*   @param [in]	arg
*				Pointer to the arguments.
*    @return
				The function shall return ZERO for successful operation and a negative value otherwise.
*/
NMI_API sint8 hif_deinit(void * arg);
/**
*	@fn		NMI_API sint8 hif_send(uint8 u8Gid,uint8 u8Opcode,uint8 *pu8CtrlBuf,uint16 u16CtrlBufSize,
					   uint8 *pu8DataBuf,uint16 u16DataSize, uint16 u16DataOffset)
*	@brief	Send packet using host interface.

*	@param [in]	u8Gid
*				Group ID.
*	@param [in]	u8Opcode
*				Operation ID.
*	@param [in]	pu8CtrlBuf
*				Pointer to the Control buffer.
*	@param [in]	u16CtrlBufSize
				Control buffer size.
*	@param [in]	u16DataOffset
				Packet Data offset.
*	@param [in]	pu8DataBuf
*				Packet buffer Allocated by the caller.
*	@param [in]	u16DataSize
				Packet buffer size (including the HIF header).
*    @return	The function shall return ZERO for successful operation and a negative value otherwise.
*/
NMI_API sint8 hif_send(uint8 u8Gid,uint8 u8Opcode,uint8 *pu8CtrlBuf,uint16 u16CtrlBufSize,
					   uint8 *pu8DataBuf,uint16 u16DataSize, uint16 u16DataOffset);
/*
*	@fn		hif_receive
*	@brief	Host interface interrupt serviece routine
*	@param [in]	u32Addr
*				Receive start address
*	@param [out] pu8Buf
*				Pointer to receive buffer. Allocated by the caller
*	@param [in]	 u16Sz
*				Receive buffer size
*	@param [in]	isDone
*				If you don't need any more packets send True otherwise send false
*   @return
				The function shall return ZERO for successful operation and a negative value otherwise.
*/

NMI_API sint8 hif_receive(uint32 u32Addr, uint8 *pu8Buf, uint16 u16Sz, uint8 isDone);
/**
*	@fn			hif_register_cb
*	@brief
				To set Callback function for every  Component.

*	@param [in]	u8Grp
*				Group to which the Callback function should be set.

*	@param [in]	fn
*				function to be set to the specified group.
*   @return
				The function shall return ZERO for successful operation and a negative value otherwise.
*/
NMI_API sint8 hif_register_cb(uint8 u8Grp,tpfHifCallBack fn);
/**
*	@fn		NMI_API sint8 hif_chip_sleep(void);
*	@brief
				To make the chip sleep.
*   @return
				The function shall return ZERO for successful operation and a negative value otherwise.
*/
NMI_API sint8 hif_chip_sleep(void);
/**
*	@fn		NMI_API sint8 hif_chip_wake(void);
*	@brief
			To Wakeup the chip.
*   @return
			The function shall return ZERO for successful operation and a negative value otherwise.
*/

NMI_API sint8 hif_chip_wake(void);
/*!
@fn	\
			NMI_API void hif_set_sleep_mode(uint8 u8Pstype);

@brief
			Set the sleep mode of the HIF layer.

@param [in]	u8Pstype
				Sleep mode.

@return
			The function SHALL return 0 for success and a negative value otherwise.
*/

NMI_API void hif_set_sleep_mode(uint8 u8Pstype);
/*!
@fn	\
	NMI_API uint8 hif_get_sleep_mode(void);

@brief
	Get the sleep mode of the HIF layer.

@return
	The function SHALL return the sleep mode of the HIF layer.
*/

NMI_API uint8 hif_get_sleep_mode(void);

#ifdef CORTUS_APP
/**
*	@fn		hif_Resp_handler(uint8 *pu8Buffer, uint16 u16BufferSize)
*	@brief
				Response handler for HIF layer.

*	@param [in]	pu8Buffer
				Pointer to the buffer.

*	@param [in]	u16BufferSize
				Buffer size.

*   @return
			    The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8 hif_Resp_handler(uint8 *pu8Buffer, uint16 u16BufferSize);
#endif

/**
*	@fn		hif_handle_isr(void)
*	@brief
			Handle interrupt received from NMC1500 firmware.
*   @return
			The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8 hif_handle_isr(void);

#ifdef __cplusplus
}
#endif
#endif
