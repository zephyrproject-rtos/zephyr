/**
 *
 * \file
 *
 * \brief This module contains M2M host interface APIs implementation.
 *
 * Copyright (c) 2016-2017 Atmel Corporation. All rights reserved.
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

#include "common/include/nm_common.h"
#include "driver/source/nmbus.h"
#include "bsp/include/nm_bsp.h"
#include "m2m_hif.h"
#include "driver/include/m2m_types.h"
#include "driver/source/nmasic.h"
#include "driver/include/m2m_periph.h"

#if (defined NM_EDGE_INTERRUPT)&&(defined NM_LEVEL_INTERRUPT)
#error "only one type of interrupt NM_EDGE_INTERRUPT,NM_LEVEL_INTERRUPT"
#endif

#if !((defined NM_EDGE_INTERRUPT)||(defined NM_LEVEL_INTERRUPT))
#error "define interrupt type NM_EDGE_INTERRUPT,NM_LEVEL_INTERRUPT"
#endif

#ifndef CORTUS_APP
#define NMI_AHB_DATA_MEM_BASE  0x30000
#define NMI_AHB_SHARE_MEM_BASE 0xd0000

#define WIFI_HOST_RCV_CTRL_0	(0x1070)
#define WIFI_HOST_RCV_CTRL_1	(0x1084)
#define WIFI_HOST_RCV_CTRL_2    (0x1078)
#define WIFI_HOST_RCV_CTRL_3    (0x106c)
#define WIFI_HOST_RCV_CTRL_4	(0x150400)
#define WIFI_HOST_RCV_CTRL_5	(0x1088)

typedef struct {
 	uint8 u8ChipMode;
 	uint8 u8ChipSleep;
 	uint8 u8HifRXDone;
 	uint8 u8Interrupt;
 	uint32 u32RxAddr;
 	uint32 u32RxSize;
	tpfHifCallBack pfWifiCb;
	tpfHifCallBack pfIpCb;
	tpfHifCallBack pfOtaCb;
	tpfHifCallBack pfSigmaCb;
	tpfHifCallBack pfHifCb;
	tpfHifCallBack pfCryptoCb;
	tpfHifCallBack pfSslCb;
}tstrHifContext;

volatile tstrHifContext gstrHifCxt;

static void isr(void)
{
	gstrHifCxt.u8Interrupt++;
#ifdef NM_LEVEL_INTERRUPT
	nm_bsp_interrupt_ctrl(0);
#endif
}
static sint8 hif_set_rx_done(void)
{
	uint32 reg;
	sint8 ret = M2M_SUCCESS;

	gstrHifCxt.u8HifRXDone = 0;
#ifdef NM_EDGE_INTERRUPT
	nm_bsp_interrupt_ctrl(1);
#endif
	ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_0,&reg);
	if(ret != M2M_SUCCESS)goto ERR1;
	/* Set RX Done */
	reg |= NBIT1;
	ret = nm_write_reg(WIFI_HOST_RCV_CTRL_0,reg);
	if(ret != M2M_SUCCESS)goto ERR1;
#ifdef NM_LEVEL_INTERRUPT
	nm_bsp_interrupt_ctrl(1);
#endif
ERR1:
	return ret;

}
/**
*	@fn			static void m2m_hif_cb(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr)
*	@brief		WiFi call back function
*	@param [in]	u8OpCode
*					HIF Opcode type.
*	@param [in]	u16DataSize
*					HIF data length.
*	@param [in]	u32Addr
*					HIF address.
*	@param [in]	grp
*					HIF group type.
*	@author
*	@date
*	@version	1.0
*/
static void m2m_hif_cb(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr)
{


}
/**
*	@fn		NMI_API sint8 hif_chip_wake(void);
*	@brief	To Wakeup the chip.
*    @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/

sint8 hif_chip_wake(void)
{
	sint8 ret = M2M_SUCCESS;
	if(gstrHifCxt.u8HifRXDone)
	{
		/*chip already wake for the rx not done no need to send wake request*/
		return ret;
	}
	if(gstrHifCxt.u8ChipSleep == 0)
	{
		if(gstrHifCxt.u8ChipMode != M2M_NO_PS)
		{
			ret = chip_wake();
			if(ret != M2M_SUCCESS)goto ERR1;
		}
		else
		{
		}
	}
	gstrHifCxt.u8ChipSleep++;
ERR1:
	return ret;
}
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

void hif_set_sleep_mode(uint8 u8Pstype)
{
	gstrHifCxt.u8ChipMode = u8Pstype;
}
/*!
@fn	\
	NMI_API uint8 hif_get_sleep_mode(void);

@brief
	Get the sleep mode of the HIF layer.

@return
	The function SHALL return the sleep mode of the HIF layer.
*/

uint8 hif_get_sleep_mode(void)
{
	return gstrHifCxt.u8ChipMode;
}

/**
*	@fn		NMI_API sint8 hif_chip_sleep_sc(void);
*	@brief	To clear the chip sleep but keep the chip sleep
*    @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/

sint8 hif_chip_sleep_sc(void)
{
	if(gstrHifCxt.u8ChipSleep >= 1)
	{
		gstrHifCxt.u8ChipSleep--;
	}
	return M2M_SUCCESS;
}
/**
*	@fn		NMI_API sint8 hif_chip_sleep(void);
*	@brief	To make the chip sleep.
*    @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/

sint8 hif_chip_sleep(void)
{
	sint8 ret = M2M_SUCCESS;

	if(gstrHifCxt.u8ChipSleep >= 1)
	{
		gstrHifCxt.u8ChipSleep--;
	}
	
	if(gstrHifCxt.u8ChipSleep == 0)
	{
		if(gstrHifCxt.u8ChipMode != M2M_NO_PS)
		{
			ret = chip_sleep();
			if(ret != M2M_SUCCESS)goto ERR1;

		}
		else
		{
		}
	}
ERR1:
	return ret;
}
/**
*   @fn		NMI_API sint8 hif_init(void * arg);
*   @brief	To initialize HIF layer.
*   @param [in]	arg
*				Pointer to the arguments.
*   @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/

sint8 hif_init(void * arg)
{
	m2m_memset((uint8*)&gstrHifCxt,0,sizeof(tstrHifContext));
	nm_bsp_register_isr(isr);
	hif_register_cb(M2M_REQ_GROUP_HIF,m2m_hif_cb);
	return M2M_SUCCESS;
}
/**
*	@fn		NMI_API sint8 hif_deinit(void * arg);
*	@brief	To De-initialize HIF layer.
*    @param [in]	arg
*				Pointer to the arguments.
*    @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_deinit(void * arg)
{
	sint8 ret = M2M_SUCCESS;
	ret = hif_chip_wake();
	m2m_memset((uint8*)&gstrHifCxt,0,sizeof(tstrHifContext));
	return ret;
}
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
*    @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/

sint8 hif_send(uint8 u8Gid,uint8 u8Opcode,uint8 *pu8CtrlBuf,uint16 u16CtrlBufSize,
			   uint8 *pu8DataBuf,uint16 u16DataSize, uint16 u16DataOffset)
{
	sint8		ret = M2M_ERR_SEND;
	volatile tstrHifHdr	strHif;

	strHif.u8Opcode		= u8Opcode&(~NBIT7);
	strHif.u8Gid		= u8Gid;
	strHif.u16Length	= M2M_HIF_HDR_OFFSET;
	if(pu8DataBuf != NULL)
	{
		strHif.u16Length += u16DataOffset + u16DataSize;
	}
	else
	{
		strHif.u16Length += u16CtrlBufSize;
	}
	ret = hif_chip_wake();
	if(ret == M2M_SUCCESS)
	{
		volatile uint32 reg, dma_addr = 0;
		volatile uint16 cnt = 0;
//#define OPTIMIZE_BUS 
/*please define in firmware also*/
#ifndef OPTIMIZE_BUS
		reg = 0UL;
		reg |= (uint32)u8Gid;
		reg |= ((uint32)u8Opcode<<8);
		reg |= ((uint32)strHif.u16Length<<16);
		ret = nm_write_reg(NMI_STATE_REG,reg);
		if(M2M_SUCCESS != ret) goto ERR1;

		reg = 0UL;
		reg |= NBIT1;
		ret = nm_write_reg(WIFI_HOST_RCV_CTRL_2, reg);
		if(M2M_SUCCESS != ret) goto ERR1;
#else
		reg = 0UL;
		reg |= NBIT1;
		reg |= ((u8Opcode & NBIT7) ? (NBIT2):(0)); /*Data = 1 or config*/
		reg |= (u8Gid == M2M_REQ_GROUP_IP) ? (NBIT3):(0); /*IP = 1 or non IP*/
		reg |= ((uint32)strHif.u16Length << 4); /*length of pkt max = 4096*/
		ret = nm_write_reg(WIFI_HOST_RCV_CTRL_2, reg);
		if(M2M_SUCCESS != ret) goto ERR1;
#endif
		dma_addr = 0;
		
		for(cnt = 0; cnt < 1000; cnt ++)
		{
			ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_2,(uint32 *)&reg);
			if(ret != M2M_SUCCESS) break;
			/*
			 * If it takes too long to get a response, the slow down to 
			 * avoid back-to-back register read operations.
			 */
			if(cnt >= 500) {
				if(cnt < 501) {
					M2M_INFO("Slowing down...\n");
				}
				nm_bsp_sleep(1);
			}
			if (!(reg & NBIT1))
			{
				ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_4,(uint32 *)&dma_addr);
				if(ret != M2M_SUCCESS) {
					/*in case of read error clear the DMA address and return error*/
					dma_addr = 0;
					goto ERR1;
				}
				/*in case of success break */
				break;
			}
		}

		if (dma_addr != 0)
		{
			volatile uint32	u32CurrAddr;
			u32CurrAddr = dma_addr;
			strHif.u16Length=NM_BSP_B_L_16(strHif.u16Length);
			ret = nm_write_block(u32CurrAddr, (uint8*)&strHif, M2M_HIF_HDR_OFFSET);
			if(M2M_SUCCESS != ret) goto ERR1;
			u32CurrAddr += M2M_HIF_HDR_OFFSET;
			if(pu8CtrlBuf != NULL)
			{
				ret = nm_write_block(u32CurrAddr, pu8CtrlBuf, u16CtrlBufSize);
				if(M2M_SUCCESS != ret) goto ERR1;
				u32CurrAddr += u16CtrlBufSize;
			}
			if(pu8DataBuf != NULL)
			{
				u32CurrAddr += (u16DataOffset - u16CtrlBufSize);
				ret = nm_write_block(u32CurrAddr, pu8DataBuf, u16DataSize);
				if(M2M_SUCCESS != ret) goto ERR1;
				u32CurrAddr += u16DataSize;
			}

			reg = dma_addr << 2;
			reg |= NBIT1;
			ret = nm_write_reg(WIFI_HOST_RCV_CTRL_3, reg);
			if(M2M_SUCCESS != ret) goto ERR1;
		}
		else
		{
			ret = hif_chip_sleep();
			M2M_DBG("Failed to alloc rx size %d\r",ret);
			ret = M2M_ERR_MEM_ALLOC;
			goto ERR2;
		}

	}
	else
	{
		M2M_ERR("(HIF)Fail to wakup the chip\n");
		goto ERR2;
	}
	/*actual sleep ret = M2M_SUCCESS*/
 	ret = hif_chip_sleep();
	return ret;
ERR1:
	/*reset the count but no actual sleep as it already bus error*/
	hif_chip_sleep_sc();
ERR2:
	/*logical error*/
	return ret;
}
/**
*	@fn		hif_isr
*	@brief	Host interface interrupt service routine
*	@author	M. Abdelmawla
*	@date	15 July 2012
*	@return	1 in case of interrupt received else 0 will be returned
*	@version	1.0
*/
static sint8 hif_isr(void)
{
	sint8 ret = M2M_SUCCESS;
	uint32 reg;
	volatile tstrHifHdr strHif;

	ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_0, &reg);
	if(M2M_SUCCESS == ret)
	{
		if(reg & 0x1)	/* New interrupt has been received */
		{
			uint16 size;

			nm_bsp_interrupt_ctrl(0);
			/*Clearing RX interrupt*/
			reg &= ~NBIT0;
			ret = nm_write_reg(WIFI_HOST_RCV_CTRL_0,reg);
			if(ret != M2M_SUCCESS)goto ERR1;
			gstrHifCxt.u8HifRXDone = 1;
			size = (uint16)((reg >> 2) & 0xfff);
			if (size > 0) {
				uint32 address = 0;
				/**
				start bus transfer
				**/
				ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_1, &address);
				if(M2M_SUCCESS != ret)
				{
					M2M_ERR("(hif) WIFI_HOST_RCV_CTRL_1 bus fail\n");
					nm_bsp_interrupt_ctrl(1);
					goto ERR1;
				}
				gstrHifCxt.u32RxAddr = address;
				gstrHifCxt.u32RxSize = size;
				ret = nm_read_block(address, (uint8*)&strHif, sizeof(tstrHifHdr));
				strHif.u16Length = NM_BSP_B_L_16(strHif.u16Length);
				if(M2M_SUCCESS != ret)
				{
					M2M_ERR("(hif) address bus fail\n");
					nm_bsp_interrupt_ctrl(1);
					goto ERR1;
				}
				if(strHif.u16Length != size)
				{
					if((size - strHif.u16Length) > 4)
					{
						M2M_ERR("(hif) Corrupted packet Size = %u <L = %u, G = %u, OP = %02X>\n",
							size, strHif.u16Length, strHif.u8Gid, strHif.u8Opcode);
						nm_bsp_interrupt_ctrl(1);
						ret = M2M_ERR_BUS_FAIL;
						goto ERR1;
					}
				}

				if(M2M_REQ_GROUP_WIFI == strHif.u8Gid)
				{
					if(gstrHifCxt.pfWifiCb)
						gstrHifCxt.pfWifiCb(strHif.u8Opcode,strHif.u16Length - M2M_HIF_HDR_OFFSET, address + M2M_HIF_HDR_OFFSET);
					else
						M2M_ERR("WIFI callback is not registered\n");

				}
				else if(M2M_REQ_GROUP_IP == strHif.u8Gid)
				{
					if(gstrHifCxt.pfIpCb)
						gstrHifCxt.pfIpCb(strHif.u8Opcode,strHif.u16Length - M2M_HIF_HDR_OFFSET, address + M2M_HIF_HDR_OFFSET);
					else
						M2M_ERR("Scoket callback is not registered\n");

				}
				else if(M2M_REQ_GROUP_OTA == strHif.u8Gid)
				{
					if(gstrHifCxt.pfOtaCb)
						gstrHifCxt.pfOtaCb(strHif.u8Opcode,strHif.u16Length - M2M_HIF_HDR_OFFSET, address + M2M_HIF_HDR_OFFSET);
					else
						M2M_ERR("Ota callback is not registered\n");

				}
				else if(M2M_REQ_GROUP_CRYPTO == strHif.u8Gid)
				{
					if(gstrHifCxt.pfCryptoCb)
						gstrHifCxt.pfCryptoCb(strHif.u8Opcode,strHif.u16Length - M2M_HIF_HDR_OFFSET, address + M2M_HIF_HDR_OFFSET);

					else
						M2M_ERR("Crypto callback is not registered\n");
				}
				else if(M2M_REQ_GROUP_SIGMA == strHif.u8Gid)
				{
					if(gstrHifCxt.pfSigmaCb)
						gstrHifCxt.pfSigmaCb(strHif.u8Opcode,strHif.u16Length - M2M_HIF_HDR_OFFSET, address + M2M_HIF_HDR_OFFSET);
					else
						M2M_ERR("Sigma callback is not registered\n");
				}
				else if(M2M_REQ_GROUP_SSL == strHif.u8Gid)
				{
				    if(gstrHifCxt.pfSslCb)
						gstrHifCxt.pfSslCb(strHif.u8Opcode,strHif.u16Length - M2M_HIF_HDR_OFFSET, address + M2M_HIF_HDR_OFFSET);
				}
				else
				{
					M2M_ERR("(hif) invalid group ID\n");
					ret = M2M_ERR_BUS_FAIL;
					goto ERR1;
				}
				if(gstrHifCxt.u8HifRXDone)
				{
					M2M_ERR("(hif) host app didn't set RX Done <%u><%X>\n", strHif.u8Gid, strHif.u8Opcode);
					ret = hif_set_rx_done();
					if(ret != M2M_SUCCESS) goto ERR1;
				}
			}
			else
			{
				M2M_ERR("(hif) Wrong Size\n");
				ret = M2M_ERR_RCV;
				goto ERR1;
			}
		}
		else
		{
#ifndef WIN32
			M2M_ERR("(hif) False interrupt %lx",reg);
			ret = M2M_ERR_FAIL;
			goto ERR1;
#else
#endif
		}
	}
	else
	{
		M2M_ERR("(hif) Fail to Read interrupt reg\n");
		goto ERR1;
	}

ERR1:
	return ret;
}

/**
*	@fn		hif_handle_isr(void)
*	@brief	Handle interrupt received from NMC1500 firmware.
*   @return     The function SHALL return 0 for success and a negative value otherwise.
*/

sint8 hif_handle_isr(void)
{
	sint8 ret = M2M_SUCCESS;	
	while (gstrHifCxt.u8Interrupt) {
		/*must be at that place because of the race of interrupt increment and that decrement*/
		/*when the interrupt enabled*/
		gstrHifCxt.u8Interrupt--;
		while(1)
		{
			ret = hif_isr();
			if(ret == M2M_SUCCESS) {
				/*we will try forever untill we get that interrupt*/
				/*Fail return errors here due to bus errors (reading expected values)*/
				break;
			} else {
				M2M_ERR("(HIF) Fail to handle interrupt %d try Again..\n",ret);
			}
		}
	}

	return ret;
}
/*
*	@fn		hif_receive
*	@brief	Host interface interrupt serviece routine
*	@param [in]	u32Addr
*				Receive start address
*	@param [out]	pu8Buf
*				Pointer to receive buffer. Allocated by the caller
*	@param [in]	u16Sz
*				Receive buffer size
*	@param [in]	isDone
*				If you don't need any more packets send True otherwise send false
*    @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_receive(uint32 u32Addr, uint8 *pu8Buf, uint16 u16Sz, uint8 isDone)
{
	sint8 ret = M2M_SUCCESS;
	if((u32Addr == 0)||(pu8Buf == NULL) || (u16Sz == 0))
	{
		if(isDone)
		{			
			/* set RX done */
			ret = hif_set_rx_done();
		}
		else
		{
			ret = M2M_ERR_FAIL;
			M2M_ERR(" hif_receive: Invalid argument\n");
		}
		goto ERR1;
	}

	if(u16Sz > gstrHifCxt.u32RxSize)
	{
		ret = M2M_ERR_FAIL;
		M2M_ERR("APP Requested Size is larger than the recived buffer size <%u><%lu>\n",u16Sz, gstrHifCxt.u32RxSize);
		goto ERR1;
	}
	if((u32Addr < gstrHifCxt.u32RxAddr)||((u32Addr + u16Sz)>(gstrHifCxt.u32RxAddr + gstrHifCxt.u32RxSize)))
	{
		ret = M2M_ERR_FAIL;
		M2M_ERR("APP Requested Address beyond the recived buffer address and length\n");
		goto ERR1;
	}
	
	/* Receive the payload */
	ret = nm_read_block(u32Addr, pu8Buf, u16Sz);
	if(ret != M2M_SUCCESS)goto ERR1;

	/* check if this is the last packet */
	if((((gstrHifCxt.u32RxAddr + gstrHifCxt.u32RxSize) - (u32Addr + u16Sz)) <= 0) || isDone)
	{
		/* set RX done */
		ret = hif_set_rx_done();
	}

ERR1:
	return ret;
}

/**
*	@fn		hif_register_cb
*	@brief	To set Callback function for every compantent Component
*	@param [in]	u8Grp
*				Group to which the Callback function should be set.
*	@param [in]	fn
*				function to be set
*    @return		The function shall return ZERO for successful operation and a negative value otherwise.
*/

sint8 hif_register_cb(uint8 u8Grp,tpfHifCallBack fn)
{
	sint8 ret = M2M_SUCCESS;
	switch(u8Grp)
	{
		case M2M_REQ_GROUP_IP:
			gstrHifCxt.pfIpCb = fn;
			break;
		case M2M_REQ_GROUP_WIFI:
			gstrHifCxt.pfWifiCb = fn;
			break;
		case M2M_REQ_GROUP_OTA:
			gstrHifCxt.pfOtaCb = fn;
			break;
		case M2M_REQ_GROUP_HIF:
			gstrHifCxt.pfHifCb = fn;
			break;
		case M2M_REQ_GROUP_CRYPTO:
			gstrHifCxt.pfCryptoCb = fn;
			break;
		case M2M_REQ_GROUP_SIGMA:
			gstrHifCxt.pfSigmaCb = fn;
			break;
		case M2M_REQ_GROUP_SSL:
			gstrHifCxt.pfSslCb = fn;
			break;
		default:
			M2M_ERR("GRp ? %d\n",u8Grp);
			ret = M2M_ERR_FAIL;
			break;
	}
	return ret;
}

#endif
