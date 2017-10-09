/**
 *
 * \file
 *
 * \brief This module contains NMC1000 UART protocol bus APIs implementation.
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

#include "common/include/nm_common.h"

#ifdef CONF_WINC_USE_UART

#include "driver/source/nmuart.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"

#define HDR_SZ  12

static uint8 get_cs(uint8* b, uint8 sz){
	int i;
	uint8 cs = 0;
	for(i = 0; i < sz; i++)
		cs ^= b[i];
	return cs;
}

/*
*	@fn			nm_uart_sync_cmd
*	@brief		Check COM Port
*	@return		M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author		Dina El Sissy
*	@date		13 AUG 2012
*	@version	1.0
*/
sint8 nm_uart_sync_cmd(void)
{
	tstrNmUartDefault strUart;
	sint8 s8Ret = -1;
	uint8 b [HDR_SZ+1];
	uint8 rsz;
	uint8 onchip = 0;

	/*read reg*/
	b[0] = 0x12;

	rsz = 1;
	strUart.pu8Buf = b;
	strUart.u16Sz = 1;

	if(M2M_SUCCESS == nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
	{
		strUart.u16Sz = rsz;
		if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
		{
			s8Ret = M2M_ERR_BUS_FAIL;
		}
	}
	else
	{
		M2M_ERR("failed to send cfg bytes\n");
		s8Ret = M2M_ERR_BUS_FAIL;
	}
	if (b[0] == 0x5a)
	{
		s8Ret = 0;
		onchip = 1;
		M2M_INFO("Built-in WINC1500 UART Found\n");
	}
	else if(b[0] == 0x5b)
	{
		s8Ret = 0;
		onchip = 0;
		M2M_INFO("WINC1500 Serial Bridge Found\n");
	}
	/*TODO: this should be the way we read the register since the cortus is little endian*/
	/**pu32RetVal = b[0] | ((uint32)b[1] << 8) | ((uint32)b[2] << 16) | ((uint32)b[3] << 24);*/
	if(s8Ret == M2M_SUCCESS)
		s8Ret = (sint8)onchip;
	return s8Ret;
}
 sint8 nm_uart_read_reg_with_ret(uint32 u32Addr, uint32* pu32RetVal)
{
	tstrNmUartDefault strUart;
	sint8 s8Ret = M2M_SUCCESS;
	uint8 b [HDR_SZ+1];
	uint8 rsz;

	/*read reg*/
	b[0] = 0xa5;
	b[1] = 0;
	b[2] = 0;
	b[3] = 0;
	b[4] = 0;
	b[5] = (uint8)(u32Addr & 0x000000ff);
	b[6] = (uint8)((u32Addr & 0x0000ff00)>>8);
	b[7] = (uint8)((u32Addr & 0x00ff0000)>>16);
	b[8] = (uint8)((u32Addr & 0xff000000)>>24);
	b[9] = 0;
	b[10] = 0;
	b[11] = 0;
	b[12] = 0;

	b[2] = get_cs(&b[1],HDR_SZ);

	rsz = 4;
	strUart.pu8Buf = b;
	strUart.u16Sz = sizeof(b);

	if(M2M_SUCCESS == nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
	{
		if(!nm_bus_get_chip_type())
		{
			strUart.u16Sz = 1;
			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
			{
				s8Ret = M2M_ERR_BUS_FAIL;
			}
			if(b[0] == 0xAC)
			{
				M2M_DBG("Successfully sent the command\n");
				strUart.u16Sz = rsz;
				if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
				{
					s8Ret = M2M_ERR_BUS_FAIL;
				}
			}
			else
			{
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
		else
		{
			strUart.u16Sz = rsz;
			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
			{
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
	}
	else
	{
		M2M_ERR("failed to send cfg bytes\n");
		s8Ret = M2M_ERR_BUS_FAIL;
	}
	/*TODO: this should be the way we read the register since the cortus is little endian*/
	/**pu32RetVal = b[0] | ((uint32)b[1] << 8) | ((uint32)b[2] << 16) | ((uint32)b[3] << 24);*/

	*pu32RetVal = ((uint32)b[0] << 24) | ((uint32)b[1] << 16) | ((uint32)b[2] << 8) | b[3];

	return s8Ret;
}

/*
*	@fn			nm_uart_read_reg
*	@brief		Read register
*	@param [in]	u32Addr
*				Register address
*	@return		Register value
*	@author		Dina El Sissy
*	@date		13 AUG 2012
*	@version	1.0
*/
uint32 nm_uart_read_reg(uint32 u32Addr)
{
	uint32 val;
	nm_uart_read_reg_with_ret(u32Addr , &val);
	return val;
}

/*
*	@fn			nm_uart_write_reg
*	@brief		write register
*	@param [in]	u32Addr
*				Register address
*	@param [in]	u32Val
*				Value to be written to the register
*	@return		M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author		Dina El Sissy
*	@date		13 AUG 2012
*	@version	1.0
*/
sint8 nm_uart_write_reg(uint32 u32Addr, uint32 u32Val)
{
	tstrNmUartDefault strUart;
	sint8 s8Ret = M2M_SUCCESS;
	uint8 b[HDR_SZ+1];

	/*write reg*/
	b[0] = 0xa5;
	b[1] = 1;
	b[2] = 0;
	b[3] = 0;
	b[4] = 0;
	b[5] = (uint8)(u32Addr & 0x000000ff);
	b[6] = (uint8)((u32Addr & 0x0000ff00)>>8);
	b[7] = (uint8)((u32Addr & 0x00ff0000)>>16);
	b[8] = (uint8)((u32Addr & 0xff000000)>>24);
	b[9] = (uint8)(u32Val & 0x000000ff);
	b[10] = (uint8)((u32Val & 0x0000ff00)>>8);
	b[11] = (uint8)((u32Val & 0x00ff0000)>>16);
	b[12] = (uint8)((u32Val & 0xff000000)>>24);

	b[2] = get_cs(&b[1],HDR_SZ);

	get_cs(&b[1],HDR_SZ);

	strUart.pu8Buf = b;
	strUart.u16Sz = sizeof(b);

	if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
	{
		M2M_ERR("write error\n");
		s8Ret = M2M_ERR_BUS_FAIL;
	}
	else
	{
		if(!nm_bus_get_chip_type())
		{
			//check for the ack from the SAMD21 for the packet reception.
			strUart.u16Sz = 1;
			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
			{
				s8Ret = M2M_ERR_BUS_FAIL;
			}
			if(b[0] == 0xAC)
			{
				M2M_DBG("Successfully sent the reg write command\n");
			}
			else
			{
				M2M_ERR("write error\n");
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
	}

	return s8Ret;
}


/**
*	@fn			nm_uart_read_block
*	@brief		Read block of data
*	@param [in]	u32Addr
*				Start address
*	@param [out]	puBuf
*				Pointer to a buffer used to return the read data
*	@param [in]	u16Sz
*				Number of bytes to read. The buffer size must be >= u16Sz
*	@return		M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author		Dina El Sissy
*	@date		13 AUG 2012
*	@version	1.0
*/
sint8 nm_uart_read_block(uint32 u32Addr, uint8 *pu8Buf, uint16 u16Sz)
{
	tstrNmUartDefault strUart;
	sint8 s8Ret = M2M_SUCCESS;
	uint8 au8Buf[HDR_SZ+1];

	au8Buf[0] = 0xa5;
	au8Buf[1] = 2;
	au8Buf[2] = 0;
	au8Buf[3] = (uint8)(u16Sz & 0x00ff);
	au8Buf[4] = (uint8)((u16Sz & 0xff00)>>8);
	au8Buf[5] = (uint8)(u32Addr & 0x000000ff);
	au8Buf[6] = (uint8)((u32Addr & 0x0000ff00)>>8);
	au8Buf[7] = (uint8)((u32Addr & 0x00ff0000)>>16);
	au8Buf[8] = (uint8)((u32Addr & 0xff000000)>>24);
	au8Buf[9] = 0;
	au8Buf[10] = 0;
	au8Buf[11] = 0;
	au8Buf[12] = 0;

	au8Buf[2] = get_cs(&au8Buf[1],HDR_SZ);

	strUart.pu8Buf = au8Buf;
	strUart.u16Sz = sizeof(au8Buf);

	if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
	{
		M2M_ERR("write error\n");
		s8Ret = M2M_ERR_BUS_FAIL;
	}
	else
	{
		if(!nm_bus_get_chip_type())
		{
			//check for the ack from the SAMD21 for the packet reception.
			strUart.u16Sz = 1;
			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
			{
				s8Ret = M2M_ERR_BUS_FAIL;
			}
			if(au8Buf[0] == 0xAC)
			{
				M2M_DBG("Successfully sent the block read command\n");
				strUart.pu8Buf = pu8Buf;
				strUart.u16Sz = u16Sz;

				if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
				{
					M2M_ERR("read error\n");
					s8Ret = M2M_ERR_BUS_FAIL;
				}
			}
			else
			{
				M2M_ERR("write error (Error sending the block read command)\n");
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
		else
		{
			strUart.pu8Buf = pu8Buf;
			strUart.u16Sz = u16Sz;

			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
			{
				M2M_ERR("read error\n");
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
	}

	return s8Ret;
}

/**
*	@fn			nm_uart_write_block
*	@brief		Write block of data
*	@param [in]	u32Addr
*				Start address
*	@param [in]	puBuf
*				Pointer to the buffer holding the data to be written
*	@param [in]	u16Sz
*				Number of bytes to write. The buffer size must be >= u16Sz
*	@return		M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author		Dina El Sissy
*	@date		13 AUG 2012
*	@version	1.0
*/
sint8 nm_uart_write_block(uint32 u32Addr, uint8 *puBuf, uint16 u16Sz)
{
	tstrNmUartDefault strUart;
	sint8 s8Ret = M2M_SUCCESS;
	static uint8 au8Buf[HDR_SZ+1];

	au8Buf[0] = 0xa5;
	au8Buf[1] = 3;
	au8Buf[2] = 0;
	au8Buf[3] = (uint8)(u16Sz & 0x00ff);
	au8Buf[4] = (uint8)((u16Sz & 0xff00)>>8);
	au8Buf[5] = (uint8)(u32Addr & 0x000000ff);
	au8Buf[6] = (uint8)((u32Addr & 0x0000ff00)>>8);
	au8Buf[7] = (uint8)((u32Addr & 0x00ff0000)>>16);
	au8Buf[8] = (uint8)((u32Addr & 0xff000000)>>24);
	au8Buf[9] = 0;
	au8Buf[10] = 0;
	au8Buf[11] = 0;
	au8Buf[12] = 0;

	au8Buf[2] = get_cs(&au8Buf[1],HDR_SZ);

	strUart.pu8Buf = au8Buf;
	strUart.u16Sz = sizeof(au8Buf);

	if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
	{
		M2M_ERR("write error\n");
		s8Ret = M2M_ERR_BUS_FAIL;
	}
	else
	{
		if(!nm_bus_get_chip_type())
		{
			//check for the ack from the SAMD21 for the packet reception.
			strUart.u16Sz = 1;
			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
			{
				s8Ret = M2M_ERR_BUS_FAIL;
			}
			if(au8Buf[0] == 0xAC)
			{
				M2M_DBG("Successfully sent the block Write command\n");
				strUart.pu8Buf = puBuf;
				strUart.u16Sz = u16Sz;

				if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
				{
					M2M_ERR("write error\n");
					s8Ret = M2M_ERR_BUS_FAIL;
				}
				else
				{
					//check for the ack from the SAMD21 for the payload reception.
					strUart.pu8Buf = au8Buf;
					strUart.u16Sz = 1;
					if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
					{
						s8Ret = M2M_ERR_BUS_FAIL;
					}
					if(au8Buf[0] == 0xAC)
					{
						M2M_DBG("Successfully sent the data payload\n");
					}
					else
					{
						M2M_ERR("write error\n");
						s8Ret = M2M_ERR_BUS_FAIL;
					}
				}
			}
			else
			{
				M2M_ERR("write error (Error sending the block write command)\n");
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
		else
		{
			strUart.pu8Buf = puBuf;
			strUart.u16Sz = u16Sz;

			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
			{
				M2M_ERR("write error\n");
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
	}
	return s8Ret;
}

/**
*	@fn			nm_uart_reconfigure
*	@brief		Reconfigures the UART interface
*	@param [in]	ptr
*				Pointer to a DWORD containing baudrate at this moment.
*	@return		M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@author		Viswanathan Murugesan
*	@date		22 OCT 2014
*	@version	1.0
*/
sint8 nm_uart_reconfigure(void *ptr)
{
	tstrNmUartDefault strUart;
	sint8 s8Ret = M2M_SUCCESS;
	uint8 b[HDR_SZ+1];

	/*write reg*/
	b[0] = 0xa5;
	b[1] = 5;
	b[2] = 0;
	b[3] = 0;
	b[4] = 0;
	b[5] = 0;
	b[6] = 0;
	b[7] = 0;
	b[8] = 0;
	b[9] = (uint8)((*(unsigned long *)ptr) & 0x000000ff);
	b[10] = (uint8)(((*(unsigned long *)ptr) & 0x0000ff00)>>8);
	b[11] = (uint8)(((*(unsigned long *)ptr) & 0x00ff0000)>>16);
	b[12] = (uint8)(((*(unsigned long *)ptr) & 0xff000000)>>24);

	b[2] = get_cs(&b[1],HDR_SZ);

	get_cs(&b[1],HDR_SZ);

	strUart.pu8Buf = b;
	strUart.u16Sz = sizeof(b);

	if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_W, &strUart))
	{
		M2M_ERR("write error\n");
		s8Ret = M2M_ERR_BUS_FAIL;
	}
	else
	{
		if(!nm_bus_get_chip_type())
		{
			//check for the ack from the SAMD21 for the packet reception.
			strUart.u16Sz = 1;
			if(M2M_SUCCESS != nm_bus_ioctl(NM_BUS_IOCTL_R, &strUart))
			{
				s8Ret = M2M_ERR_BUS_FAIL;
			}
			if(b[0] == 0xAC)
			{
				M2M_DBG("Successfully sent the UART reconfigure command\n");
			}
			else
			{
				M2M_ERR("write error\n");
				s8Ret = M2M_ERR_BUS_FAIL;
			}
		}
	}

	return s8Ret;
}
#endif
/* EOF */
