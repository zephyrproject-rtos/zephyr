/**
 *
 * \file
 *
 * \brief This module contains common APIs declarations.
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

void m2m_memcpy(uint8* pDst,uint8* pSrc,uint32 sz)
{
	if(sz == 0) return;
	do
	{
		*pDst = *pSrc;
		pDst++;
		pSrc++;
	}while(--sz);
}
uint8 m2m_checksum(uint8* buf, int sz)
{
	uint8 cs = 0;
	while(--sz)
	{
		cs ^= *buf;
		buf++;
	}

	return cs;
}

void m2m_memset(uint8* pBuf,uint8 val,uint32 sz)
{
	if(sz == 0) return;
	do
	{
		*pBuf = val;
		pBuf++;
	}while(--sz);
}

uint16 m2m_strlen(uint8 * pcStr)
{
	uint16	u16StrLen = 0;
	while(*pcStr)
	{
		u16StrLen ++;
		pcStr++;
	}
	return u16StrLen;
}

uint8 m2m_strncmp(uint8 *pcS1, uint8 *pcS2, uint16 u16Len)
{
    for ( ; u16Len > 0; pcS1++, pcS2++, --u16Len)
	if (*pcS1 != *pcS2)
	    return ((*(uint8 *)pcS1 < *(uint8 *)pcS2) ? -1 : +1);
	else if (*pcS1 == '\0')
	    return 0;
    return 0;
}

/* Finds the occurance of pcStr in pcIn.
If pcStr is part of pcIn it returns a valid pointer to the start of pcStr within pcIn.
Otherwise a NULL Pointer is returned.
*/
uint8 * m2m_strstr(uint8 *pcIn, uint8 *pcStr)
{
    uint8 u8c;
    uint16 u16StrLen;

    u8c = *pcStr++;
    if (!u8c)
        return (uint8 *) pcIn;	// Trivial empty string case

    u16StrLen = m2m_strlen(pcStr);
    do {
        uint8 u8Sc;

        do {
            u8Sc = *pcIn++;
            if (!u8Sc)
                return (uint8 *) 0;
        } while (u8Sc != u8c);
    } while (m2m_strncmp(pcIn, pcStr, u16StrLen) != 0);

    return (uint8 *) (pcIn - 1);
}

sint8 m2m_memcmp(uint8 *pu8Buff1,uint8 *pu8Buff2 ,uint32 u32Size)
{
	uint32	i;
	sint8		s8Result = 0;
	for(i	 = 0 ; i < u32Size ; i++)
	{
		if(pu8Buff1[i] != pu8Buff2[i])
		{
			s8Result = 1;
			break;
		}
	}
	return s8Result;
}
