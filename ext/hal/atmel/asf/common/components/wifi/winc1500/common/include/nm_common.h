/**
 *
 * \file
 *
 * \brief WINC Driver Common API Declarations.
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

#ifndef _NM_COMMON_H_
#define _NM_COMMON_H_

#include "bsp/include/nm_bsp.h"
#include "common/include/nm_debug.h"

/**@defgroup  CommonDefines CommonDefines
 * @ingroup WlanDefines
 */
/**@{*/
#define M2M_TIME_OUT_DELAY 10000

/*states*/
#define M2M_SUCCESS         ((sint8)0)
#define M2M_ERR_SEND		((sint8)-1)
#define M2M_ERR_RCV			((sint8)-2)
#define M2M_ERR_MEM_ALLOC	((sint8)-3)
#define M2M_ERR_TIME_OUT	((sint8)-4)
#define M2M_ERR_INIT        ((sint8)-5)
#define M2M_ERR_BUS_FAIL    ((sint8)-6)
#define M2M_NOT_YET			((sint8)-7)
#define M2M_ERR_FIRMWARE	((sint8)-8)
#define M2M_SPI_FAIL		((sint8)-9)
#define M2M_ERR_FIRMWARE_bURN	 ((sint8)-10)
#define M2M_ACK				((sint8)-11)
#define M2M_ERR_FAIL		((sint8)-12)
#define M2M_ERR_FW_VER_MISMATCH         ((sint8)-13)
#define M2M_ERR_SCAN_IN_PROGRESS         ((sint8)-14)
#define M2M_ERR_INVALID_ARG				 ((sint8)-15)
#define M2M_ERR_INVALID					((sint8)-16)

/*i2c MAASTER ERR*/
#define I2C_ERR_LARGE_ADDRESS 	  0xE1UL	/*the address exceed the max addressing mode in i2c flash*/
#define I2C_ERR_TX_ABRT 		  0xE2UL	/*NO ACK from slave*/
#define I2C_ERR_OVER_SIZE 		  0xE3UL	/**/
#define ERR_PREFIX_NMIS		      0xE4UL	/*wrong first four byte in flash NMIS*/
#define ERR_FIRMEWARE_EXCEED_SIZE 0xE5UL	/*Total size of firmware exceed the max size 256k*/
/**/
#define PROGRAM_START		0x26961735UL
#define BOOT_SUCCESS		0x10add09eUL
#define BOOT_START		    0x12345678UL


#define NBIT31				(0x80000000)
#define NBIT30				(0x40000000)
#define NBIT29				(0x20000000)
#define NBIT28				(0x10000000)
#define NBIT27				(0x08000000)
#define NBIT26				(0x04000000)
#define NBIT25				(0x02000000)
#define NBIT24				(0x01000000)
#define NBIT23				(0x00800000)
#define NBIT22				(0x00400000)
#define NBIT21				(0x00200000)
#define NBIT20				(0x00100000)
#define NBIT19				(0x00080000)
#define NBIT18				(0x00040000)
#define NBIT17				(0x00020000)
#define NBIT16				(0x00010000)
#define NBIT15				(0x00008000)
#define NBIT14				(0x00004000)
#define NBIT13				(0x00002000)
#define NBIT12				(0x00001000)
#define NBIT11				(0x00000800)
#define NBIT10				(0x00000400)
#define NBIT9				(0x00000200)
#define NBIT8				(0x00000100)
#define NBIT7				(0x00000080)
#define NBIT6				(0x00000040)
#define NBIT5				(0x00000020)
#define NBIT4				(0x00000010)
#define NBIT3				(0x00000008)
#define NBIT2				(0x00000004)
#define NBIT1				(0x00000002)
#define NBIT0				(0x00000001)

#define M2M_MAX(A,B)					((A) > (B) ? (A) : (B))
#define M2M_SEL(x,m1,m2,m3)				((x>1)?((x>2)?(m3):(m2)):(m1))
#define WORD_ALIGN(val) 				(((val) & 0x03) ? ((val) + 4 - ((val) & 0x03)) : (val))



#define DATA_PKT_OFFSET	4

#ifndef BIG_ENDIAN
#define BYTE_0(word)   					((uint8)(((word) >> 0 	) & 0x000000FFUL))
#define BYTE_1(word)  	 				((uint8)(((word) >> 8 	) & 0x000000FFUL))
#define BYTE_2(word)   					((uint8)(((word) >> 16) & 0x000000FFUL))
#define BYTE_3(word)   					((uint8)(((word) >> 24) & 0x000000FFUL))
#else
#define BYTE_0(word)   					((uint8)(((word) >> 24) & 0x000000FFUL))
#define BYTE_1(word)  	 				((uint8)(((word) >> 16) & 0x000000FFUL))
#define BYTE_2(word)   					((uint8)(((word) >> 8 	) & 0x000000FFUL))
#define BYTE_3(word)   					((uint8)(((word) >> 0 	) & 0x000000FFUL))
#endif

/**@}*/
#ifdef __cplusplus
     extern "C" {
 #endif
NMI_API void m2m_memcpy(uint8* pDst,uint8* pSrc,uint32 sz);
NMI_API void m2m_memset(uint8* pBuf,uint8 val,uint32 sz);
NMI_API uint16 m2m_strlen(uint8 * pcStr);
NMI_API sint8 m2m_memcmp(uint8 *pu8Buff1,uint8 *pu8Buff2 ,uint32 u32Size);
NMI_API uint8 m2m_strncmp(uint8 *pcS1, uint8 *pcS2, uint16 u16Len);
NMI_API uint8 * m2m_strstr(uint8 *pcIn, uint8 *pcStr);
NMI_API uint8 m2m_checksum(uint8* buf, int sz);

#ifdef __cplusplus
}
 #endif
#endif	/*_NM_COMMON_H_*/
