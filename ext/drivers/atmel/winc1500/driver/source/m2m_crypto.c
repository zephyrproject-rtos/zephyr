/**
 *
 * \file
 *
 * \brief WINC Crypto module.
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

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "driver/include/m2m_crypto.h"
#include "driver/source/nmbus.h"
#include "driver/source/nmasic.h"

#ifdef CONF_CRYPTO_HW

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

/*======*======*======*======*======*=======*
* WINC SHA256 HW Engine Register Definition * 
*======*======*======*======*======*========*/

#define SHA_BLOCK_SIZE											(64)

#define SHARED_MEM_BASE											(0xd0000)


#define SHA256_MEM_BASE											(0x180000UL)
#define SHA256_ENGINE_ADDR										(0x180000ul)

/* SHA256 Registers */
#define SHA256_CTRL												(SHA256_MEM_BASE+0x00)
#define SHA256_CTRL_START_CALC_MASK								(NBIT0)
#define SHA256_CTRL_START_CALC_SHIFT							(0)
#define SHA256_CTRL_PREPROCESS_MASK								(NBIT1)
#define SHA256_CTRL_PREPROCESS_SHIFT							(1)
#define SHA256_CTRL_HASH_HASH_MASK								(NBIT2)
#define SHA256_CTRL_HASH_HASH_SHIFT								(2)
#define SHA256_CTRL_INIT_SHA256_STATE_MASK						(NBIT3)
#define SHA256_CTRL_INIT_SHA256_STATE_SHIFT						(3)
#define SHA256_CTRL_WR_BACK_HASH_VALUE_MASK						(NBIT4)
#define SHA256_CTRL_WR_BACK_HASH_VALUE_SHIFT					(4)
#define SHA256_CTRL_FORCE_SHA256_QUIT_MASK						(NBIT5)
#define SHA256_CTRL_FORCE_SHA256_QUIT_SHIFT						(5)

#define SHA256_REGS_SHA256_CTRL_AHB_BYTE_REV_EN                 (NBIT6)
#define SHA256_REGS_SHA256_CTRL_RESERVED						(NBIT7)
#define SHA256_REGS_SHA256_CTRL_CORE_TO_AHB_CLK_RATIO			(NBIT8+ NBIT9+ NBIT10)
#define SHA256_REGS_SHA256_CTRL_CORE_TO_AHB_CLK_RATIO_MASK		(NBIT2+ NBIT1+ NBIT0)
#define SHA256_REGS_SHA256_CTRL_CORE_TO_AHB_CLK_RATIO_SHIFT		(8)
#define SHA256_REGS_SHA256_CTRL_RESERVED_11						(NBIT11)
#define SHA256_REGS_SHA256_CTRL_SHA1_CALC                       (NBIT12)
#define SHA256_REGS_SHA256_CTRL_PBKDF2_SHA1_CALC                (NBIT13)


#define SHA256_START_RD_ADDR									(SHA256_MEM_BASE+0x04UL)
#define SHA256_DATA_LENGTH										(SHA256_MEM_BASE+0x08UL)
#define SHA256_START_WR_ADDR									(SHA256_MEM_BASE+0x0cUL)
#define SHA256_COND_CHK_CTRL									(SHA256_MEM_BASE+0x10)
#define SHA256_COND_CHK_CTRL_HASH_VAL_COND_CHK_MASK				(NBIT1 | NBIT0)
#define SHA256_COND_CHK_CTRL_HASH_VAL_COND_CHK_SHIFT			(0)
#define SHA256_COND_CHK_CTRL_STEP_VAL_MASK						(NBIT6 | NBIT5 | NBIT4 | NBIT3 | NBIT2)
#define SHA256_COND_CHK_CTRL_STEP_VAL_SHIFT						(2)
#define SHA256_COND_CHK_CTRL_COND_CHK_RESULT_MASK				(NBIT7)
#define SHA256_COND_CHK_CTRL_COND_CHK_RESULT_SHIFT				(7)

#define SHA256_MOD_DATA_RANGE									(SHA256_MEM_BASE+0x14)
#define SHA256_MOD_DATA_RANGE_ST_BYTE_2_ADD_STP_MASK			(NBIT24-1)
#define SHA256_MOD_DATA_RANGE_ST_BYTE_2_ADD_STP_SHIFT			(0)
#define SHA256_MOD_DATA_RANGE_MOD_DATA_LEN_MASK					(NBIT24 | NBIT25| NBIT26)
#define SHA256_MOD_DATA_RANGE_MOD_DATA_LEN_SHIFT				(24)


#define SHA256_COND_CHK_STS_1									(SHA256_MEM_BASE+0x18)
#define SHA256_COND_CHK_STS_2									(SHA256_MEM_BASE+0x1c)
#define SHA256_DONE_INTR_ENABLE									(SHA256_MEM_BASE+0x20)
#define SHA256_DONE_INTR_STS									(SHA256_MEM_BASE+0x24)
#define SHA256_TARGET_HASH_H1									(SHA256_MEM_BASE+0x28)
#define SHA256_TARGET_HASH_H2									(SHA256_MEM_BASE+0x2c)
#define SHA256_TARGET_HASH_H3									(SHA256_MEM_BASE+0x30)
#define SHA256_TARGET_HASH_H4									(SHA256_MEM_BASE+0x34)
#define SHA256_TARGET_HASH_H5									(SHA256_MEM_BASE+0x38)
#define SHA256_TARGET_HASH_H6									(SHA256_MEM_BASE+0x3c)
#define SHA256_TARGET_HASH_H7									(SHA256_MEM_BASE+0x40)
#define SHA256_TARGET_HASH_H8									(SHA256_MEM_BASE+0x44)

/*======*======*======*======*======*=======*
* WINC BIGINT HW Engine Register Definition *
*======*======*======*======*======*========*/


#define BIGINT_ENGINE_ADDR							(0x180080ul)
#define BIGINT_VERSION								(BIGINT_ENGINE_ADDR + 0x00)

#define BIGINT_MISC_CTRL							(BIGINT_ENGINE_ADDR + 0x04)
#define BIGINT_MISC_CTRL_CTL_START					(NBIT0)
#define BIGINT_MISC_CTRL_CTL_RESET					(NBIT1)
#define BIGINT_MISC_CTRL_CTL_MSW_FIRST				(NBIT2)
#define BIGINT_MISC_CTRL_CTL_SWAP_BYTE_ORDER		(NBIT3)
#define BIGINT_MISC_CTRL_CTL_FORCE_BARRETT			(NBIT4)
#define BIGINT_MISC_CTRL_CTL_M_PRIME_VALID			(NBIT5)

#define BIGINT_M_PRIME								(BIGINT_ENGINE_ADDR + 0x08)

#define BIGINT_STATUS								(BIGINT_ENGINE_ADDR + 0x0C)
#define BIGINT_STATUS_STS_DONE						(NBIT0)

#define BIGINT_CLK_COUNT							(BIGINT_ENGINE_ADDR + 0x10)
#define BIGINT_ADDR_X								(BIGINT_ENGINE_ADDR + 0x14)
#define BIGINT_ADDR_E								(BIGINT_ENGINE_ADDR + 0x18)
#define BIGINT_ADDR_M								(BIGINT_ENGINE_ADDR + 0x1C)
#define BIGINT_ADDR_R								(BIGINT_ENGINE_ADDR + 0x20)
#define BIGINT_LENGTH								(BIGINT_ENGINE_ADDR + 0x24)

#define BIGINT_IRQ_STS								(BIGINT_ENGINE_ADDR + 0x28)
#define BIGINT_IRQ_STS_DONE							(NBIT0)
#define BIGINT_IRQ_STS_CHOOSE_MONT					(NBIT1)
#define BIGINT_IRQ_STS_M_READ						(NBIT2)
#define BIGINT_IRQ_STS_X_READ						(NBIT3)
#define BIGINT_IRQ_STS_START						(NBIT4)
#define BIGINT_IRQ_STS_PRECOMP_FINISH				(NBIT5)

#define BIGINT_IRQ_MASK								(BIGINT_ENGINE_ADDR + 0x2C)
#define BIGINT_IRQ_MASK_CTL_IRQ_MASK_START			(NBIT4)

#define ENABLE_FLIPPING			1




#define GET_UINT32(BUF,OFFSET)			(((uint32)((BUF)[OFFSET])) | ((uint32)(((BUF)[OFFSET + 1]) << 8))  | \
((uint32)(((BUF)[OFFSET + 2]) << 16)) | ((uint32)(((BUF)[OFFSET + 3]) << 24)))

#define PUTU32(VAL32,BUF,OFFSET)	\
do	\
{	\
	(BUF)[OFFSET	] = BYTE_3((VAL32));	\
	(BUF)[OFFSET +1	] = BYTE_2((VAL32));	\
	(BUF)[OFFSET +2	] = BYTE_1((VAL32));	\
	(BUF)[OFFSET +3	] = BYTE_0((VAL32));	\
}while(0)


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

/*!
@struct	\
	tstrHashContext
	
@brief
*/
typedef struct{
	uint32		au32HashState[M2M_SHA256_DIGEST_LEN/4];
	uint8		au8CurrentBlock[64];
	uint32		u32TotalLength;
	uint8		u8InitHashFlag;
}tstrSHA256HashCtxt;



/*======*======*======*======*======*=======*
*           SHA256 IMPLEMENTATION           *
*======*======*======*======*======*========*/

sint8 m2m_crypto_sha256_hash_init(tstrM2mSha256Ctxt *pstrSha256Ctxt)
{
	tstrSHA256HashCtxt	*pstrSHA256 = (tstrSHA256HashCtxt*)pstrSha256Ctxt;
	if(pstrSHA256 != NULL)
	{
		m2m_memset((uint8*)pstrSha256Ctxt, 0, sizeof(tstrM2mSha256Ctxt));
		pstrSHA256->u8InitHashFlag = 1;
	}
	return 0;
}

sint8 m2m_crypto_sha256_hash_update(tstrM2mSha256Ctxt *pstrSha256Ctxt, uint8 *pu8Data, uint16 u16DataLength)
{
	sint8	s8Ret = M2M_ERR_FAIL;
	tstrSHA256HashCtxt	*pstrSHA256 = (tstrSHA256HashCtxt*)pstrSha256Ctxt;
	if(pstrSHA256 != NULL)
	{
		uint32	u32ReadAddr;
		uint32	u32WriteAddr	= SHARED_MEM_BASE;
		uint32	u32Addr			= u32WriteAddr;
		uint32 	u32ResidualBytes;
		uint32 	u32NBlocks;
		uint32 	u32Offset;
		uint32 	u32CurrentBlock = 0;
		uint8	u8IsDone		= 0;

		/* Get the remaining bytes from the previous update (if the length is not block aligned). */
		u32ResidualBytes = pstrSHA256->u32TotalLength % SHA_BLOCK_SIZE;

		/* Update the total data length. */
		pstrSHA256->u32TotalLength += u16DataLength;

		if(u32ResidualBytes != 0)
		{
			if((u32ResidualBytes + u16DataLength) >= SHA_BLOCK_SIZE)
			{
				u32Offset = SHA_BLOCK_SIZE - u32ResidualBytes;
				m2m_memcpy(&pstrSHA256->au8CurrentBlock[u32ResidualBytes], pu8Data, u32Offset);
				pu8Data			+= u32Offset;
				u16DataLength	-= u32Offset;

				nm_write_block(u32Addr, pstrSHA256->au8CurrentBlock, SHA_BLOCK_SIZE);
				u32Addr += SHA_BLOCK_SIZE;
				u32CurrentBlock	 = 1;
			}
			else
			{
				m2m_memcpy(&pstrSHA256->au8CurrentBlock[u32ResidualBytes], pu8Data, u16DataLength);
				u16DataLength = 0;
			}
		}

		/* Get the number of HASH BLOCKs and the residual bytes. */
		u32NBlocks			= u16DataLength / SHA_BLOCK_SIZE;
		u32ResidualBytes	= u16DataLength % SHA_BLOCK_SIZE;

		if(u32NBlocks != 0)
		{
			nm_write_block(u32Addr, pu8Data, (uint16)(u32NBlocks * SHA_BLOCK_SIZE));
			pu8Data += (u32NBlocks * SHA_BLOCK_SIZE);
		}

		u32NBlocks += u32CurrentBlock;
		if(u32NBlocks != 0)
		{
			uint32	u32RegVal = 0;

			nm_write_reg(SHA256_CTRL, u32RegVal);
			u32RegVal |= SHA256_CTRL_FORCE_SHA256_QUIT_MASK;
			nm_write_reg(SHA256_CTRL, u32RegVal);

			if(pstrSHA256->u8InitHashFlag)
			{
				pstrSHA256->u8InitHashFlag = 0;
				u32RegVal |= SHA256_CTRL_INIT_SHA256_STATE_MASK;
			}

			u32ReadAddr = u32WriteAddr + (u32NBlocks * SHA_BLOCK_SIZE);
			nm_write_reg(SHA256_DATA_LENGTH, (u32NBlocks * SHA_BLOCK_SIZE));
			nm_write_reg(SHA256_START_RD_ADDR, u32WriteAddr);
			nm_write_reg(SHA256_START_WR_ADDR, u32ReadAddr);

			u32RegVal |= SHA256_CTRL_START_CALC_MASK;

			u32RegVal &= ~(0x7 << 8);
			u32RegVal |= (2 << 8);

			nm_write_reg(SHA256_CTRL, u32RegVal);

			/* 5.	Wait for done_intr */
			while(!u8IsDone)
			{
				u32RegVal = nm_read_reg(SHA256_DONE_INTR_STS);
				u8IsDone = u32RegVal & NBIT0;
			}
		}
		if(u32ResidualBytes != 0)
		{
			m2m_memcpy(pstrSHA256->au8CurrentBlock, pu8Data, u32ResidualBytes);
		}
		s8Ret = M2M_SUCCESS;
	}
	return s8Ret;
}


sint8 m2m_crypto_sha256_hash_finish(tstrM2mSha256Ctxt *pstrSha256Ctxt, uint8 *pu8Sha256Digest)
{
	sint8	s8Ret = M2M_ERR_FAIL;
	tstrSHA256HashCtxt	*pstrSHA256 = (tstrSHA256HashCtxt*)pstrSha256Ctxt;
	if(pstrSHA256 != NULL)
	{
		uint32	u32ReadAddr;
		uint32	u32WriteAddr	= SHARED_MEM_BASE;
		uint32	u32Addr			= u32WriteAddr;
		uint16	u16Offset;
		uint16 	u16PaddingLength;
		uint16	u16NBlocks		= 1;
		uint32	u32RegVal		= 0;
		uint32	u32Idx,u32ByteIdx;
		uint32	au32Digest[M2M_SHA256_DIGEST_LEN / 4];
		uint8	u8IsDone		= 0;

		nm_write_reg(SHA256_CTRL,u32RegVal);
		u32RegVal |= SHA256_CTRL_FORCE_SHA256_QUIT_MASK;
		nm_write_reg(SHA256_CTRL,u32RegVal);

		if(pstrSHA256->u8InitHashFlag)
		{
			pstrSHA256->u8InitHashFlag = 0;
			u32RegVal |= SHA256_CTRL_INIT_SHA256_STATE_MASK;
		}

		/* Calculate the offset of the last data byte in the current block. */
		u16Offset = (uint16)(pstrSHA256->u32TotalLength % SHA_BLOCK_SIZE);

		/* Add the padding byte 0x80. */
		pstrSHA256->au8CurrentBlock[u16Offset ++] = 0x80;

		/* Calculate the required padding to complete
		one Hash Block Size.
		*/
		u16PaddingLength = SHA_BLOCK_SIZE - u16Offset;
		m2m_memset(&pstrSHA256->au8CurrentBlock[u16Offset], 0, u16PaddingLength);

		/* If the padding count is not enough to hold 64-bit representation of
		the total input message length, one padding block is required.
		*/
		if(u16PaddingLength < 8)
		{
			nm_write_block(u32Addr, pstrSHA256->au8CurrentBlock, SHA_BLOCK_SIZE);
			u32Addr += SHA_BLOCK_SIZE;
			m2m_memset(pstrSHA256->au8CurrentBlock, 0, SHA_BLOCK_SIZE);
			u16NBlocks ++;
		}

		/* pack the length at the end of the padding block */
		PUTU32(pstrSHA256->u32TotalLength << 3, pstrSHA256->au8CurrentBlock, (SHA_BLOCK_SIZE - 4));

		u32ReadAddr = u32WriteAddr + (u16NBlocks * SHA_BLOCK_SIZE);
		nm_write_block(u32Addr, pstrSHA256->au8CurrentBlock, SHA_BLOCK_SIZE);
		nm_write_reg(SHA256_DATA_LENGTH, (u16NBlocks * SHA_BLOCK_SIZE));
		nm_write_reg(SHA256_START_RD_ADDR, u32WriteAddr);
		nm_write_reg(SHA256_START_WR_ADDR, u32ReadAddr);

		u32RegVal |= SHA256_CTRL_START_CALC_MASK;
		u32RegVal |= SHA256_CTRL_WR_BACK_HASH_VALUE_MASK;
		u32RegVal &= ~(0x7UL << 8);
		u32RegVal |= (0x2UL << 8);

		nm_write_reg(SHA256_CTRL,u32RegVal);


		/* 5.	Wait for done_intr */
		while(!u8IsDone)
		{
			u32RegVal = nm_read_reg(SHA256_DONE_INTR_STS);
			u8IsDone = u32RegVal & NBIT0;
		}
		nm_read_block(u32ReadAddr, (uint8*)au32Digest, 32);
		
		/* Convert the output words to an array of bytes.
		*/
		u32ByteIdx = 0;
		for(u32Idx = 0; u32Idx < (M2M_SHA256_DIGEST_LEN / 4); u32Idx ++)
		{
			pu8Sha256Digest[u32ByteIdx ++] = BYTE_3(au32Digest[u32Idx]);
			pu8Sha256Digest[u32ByteIdx ++] = BYTE_2(au32Digest[u32Idx]);
			pu8Sha256Digest[u32ByteIdx ++] = BYTE_1(au32Digest[u32Idx]);
			pu8Sha256Digest[u32ByteIdx ++] = BYTE_0(au32Digest[u32Idx]);
		}
		s8Ret = M2M_SUCCESS;
	}
	return s8Ret;
}


/*======*======*======*======*======*=======*
*             RSA IMPLEMENTATION            *
*======*======*======*======*======*========*/

static void FlipBuffer(uint8 *pu8InBuffer, uint8 *pu8OutBuffer, uint16 u16BufferSize)
{
	uint16	u16Idx;
	for(u16Idx = 0; u16Idx < u16BufferSize; u16Idx ++)
	{
#if ENABLE_FLIPPING == 1
		pu8OutBuffer[u16Idx] = pu8InBuffer[u16BufferSize - u16Idx - 1];
#else
		pu8OutBuffer[u16Idx] = pu8InBuffer[u16Idx];
#endif
	}
}

void BigInt_ModExp
(	
 uint8	*pu8X,	uint16	u16XSize,
 uint8	*pu8E,	uint16	u16ESize,
 uint8	*pu8M,	uint16	u16MSize,
 uint8	*pu8R,	uint16	u16RSize
 )
{
	uint32	u32Reg;
	uint8	au8Tmp[780] = {0};
	uint32	u32XAddr	= SHARED_MEM_BASE;
	uint32	u32MAddr;
	uint32	u32EAddr;
	uint32	u32RAddr;
	uint8	u8EMswBits	= 32;
	uint32	u32Mprime	= 0x7F;
	uint16	u16XSizeWords,u16ESizeWords;
	uint32	u32Exponent;

	u16XSizeWords = (u16XSize + 3) / 4;
	u16ESizeWords = (u16ESize + 3) / 4;
	
	u32MAddr	= u32XAddr + (u16XSizeWords * 4);
	u32EAddr 	= u32MAddr + (u16XSizeWords * 4);
	u32RAddr	= u32EAddr + (u16ESizeWords * 4);

	/* Reset the core.
	*/
	u32Reg	= 0;
	u32Reg	|= BIGINT_MISC_CTRL_CTL_RESET;
	u32Reg	= nm_read_reg(BIGINT_MISC_CTRL);
	u32Reg	&= ~BIGINT_MISC_CTRL_CTL_RESET;
	u32Reg = nm_read_reg(BIGINT_MISC_CTRL);

	nm_write_block(u32RAddr,au8Tmp, u16RSize);

	/* Write Input Operands to Chip Memory. 
	*/
	/*------- X -------*/
	FlipBuffer(pu8X,au8Tmp,u16XSize);
	nm_write_block(u32XAddr,au8Tmp,u16XSizeWords * 4);

	/*------- E -------*/
	m2m_memset(au8Tmp, 0, sizeof(au8Tmp));
	FlipBuffer(pu8E, au8Tmp, u16ESize);
	nm_write_block(u32EAddr, au8Tmp, u16ESizeWords * 4);
	u32Exponent = GET_UINT32(au8Tmp, (u16ESizeWords * 4) - 4);
	while((u32Exponent & NBIT31)== 0)
	{
		u32Exponent <<= 1;
		u8EMswBits --;
	}

	/*------- M -------*/
	m2m_memset(au8Tmp, 0, sizeof(au8Tmp));
	FlipBuffer(pu8M, au8Tmp, u16XSize);
	nm_write_block(u32MAddr, au8Tmp, u16XSizeWords * 4);

	/* Program the addresses of the input operands. 
	*/
	nm_write_reg(BIGINT_ADDR_X, u32XAddr);
	nm_write_reg(BIGINT_ADDR_E, u32EAddr);
	nm_write_reg(BIGINT_ADDR_M, u32MAddr);
	nm_write_reg(BIGINT_ADDR_R, u32RAddr);

	/* Mprime. 
	*/
	nm_write_reg(BIGINT_M_PRIME,u32Mprime);

	/* Length. 
	*/
	u32Reg	= (u16XSizeWords & 0xFF);
	u32Reg += ((u16ESizeWords & 0xFF) << 8);
	u32Reg += (u8EMswBits << 16);
	nm_write_reg(BIGINT_LENGTH,u32Reg);

	/* CTRL Register. 
	*/
	u32Reg = nm_read_reg(BIGINT_MISC_CTRL);
	u32Reg ^= BIGINT_MISC_CTRL_CTL_START;
	u32Reg |= BIGINT_MISC_CTRL_CTL_FORCE_BARRETT;
	//u32Reg |= BIGINT_MISC_CTRL_CTL_M_PRIME_VALID;
#if ENABLE_FLIPPING == 0
	u32Reg |= BIGINT_MISC_CTRL_CTL_MSW_FIRST;
#endif
	nm_write_reg(BIGINT_MISC_CTRL,u32Reg);

	/* Wait for computation to complete. */
	while(1)
	{
		u32Reg = nm_read_reg(BIGINT_IRQ_STS);
		if(u32Reg & BIGINT_IRQ_STS_DONE)
		{
			break;
		}
	}
	nm_write_reg(BIGINT_IRQ_STS,0);
	m2m_memset(au8Tmp, 0, sizeof(au8Tmp));
	nm_read_block(u32RAddr, au8Tmp, u16RSize);
	FlipBuffer(au8Tmp, pu8R, u16RSize);
}



#define MD5_DIGEST_SIZE			(16)
#define SHA1_DIGEST_SIZE		(20)

static const uint8 au8TEncodingMD5[] = 
{
	0x30, 0x20, 0x30, 0x0C, 0x06, 0x08, 0x2A, 0x86,
	0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05, 0x05, 0x00,
	0x04
};
/*!< Fixed part of the Encoding T for the MD5 hash algorithm.
*/


static const uint8 au8TEncodingSHA1[] = 
{
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x0E,
	0x03, 0x02, 0x1A, 0x05, 0x00, 0x04 
};
/*!< Fixed part of the Encoding T for the SHA-1 hash algorithm.
*/


static const uint8 au8TEncodingSHA2[] = 
{
	0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
	0x00, 0x04
};
/*!< Fixed part of the Encoding T for the SHA-2 hash algorithm.
*/


sint8 m2m_crypto_rsa_sign_verify(uint8 *pu8N, uint16 u16NSize, uint8 *pu8E, uint16 u16ESize, uint8 *pu8SignedMsgHash, 
						  uint16 u16HashLength, uint8 *pu8RsaSignature)
{
	sint8		s8Ret = M2M_RSA_SIGN_FAIL;

	if((pu8N != NULL) && (pu8E != NULL) && (pu8RsaSignature != NULL) && (pu8SignedMsgHash != NULL))
	{
		uint16	u16TLength, u16TEncodingLength;
		uint8	*pu8T;
		uint8	au8EM[512];

		/* Selection of correct T Encoding based on the hash size.
		*/
		if(u16HashLength == MD5_DIGEST_SIZE)
		{
			pu8T 				= (uint8*)au8TEncodingMD5;
			u16TEncodingLength	= sizeof(au8TEncodingMD5);
		}
		else if(u16HashLength == SHA1_DIGEST_SIZE)
		{
			pu8T 				= (uint8*)au8TEncodingSHA1;
			u16TEncodingLength	= sizeof(au8TEncodingSHA1);
		}
		else 
		{
			pu8T 				= (uint8*)au8TEncodingSHA2;
			u16TEncodingLength	= sizeof(au8TEncodingSHA2);
		}
		u16TLength = u16TEncodingLength + 1 + u16HashLength;
					
		/* If emLen < tLen + 11.
		*/
		if(u16NSize >= (u16TLength + 11))
		{
			uint32 	u32PSLength,u32Idx = 0;
	
			/*
			RSA verification
			*/
			BigInt_ModExp(pu8RsaSignature, u16NSize, pu8E, u16ESize, pu8N, u16NSize, au8EM, u16NSize);

			u32PSLength = u16NSize - u16TLength - 3;

			/* 
			The calculated EM must match the following pattern.
			*======*======*======*======*======*
			* 0x00 || 0x01 || PS || 0x00 || T  *	
			*======*======*======*======*======*
			Where PS is all 0xFF 
			T is defined based on the hash algorithm.
			*/
			if((au8EM[0] == 0x00) && (au8EM[1] == 0x01))
			{
				for(u32Idx = 2; au8EM[u32Idx] == 0xFF; u32Idx ++);
				if(u32Idx == (u32PSLength + 2))
				{
					if(au8EM[u32Idx ++] == 0x00)
					{
						if(!m2m_memcmp(&au8EM[u32Idx], pu8T, u16TEncodingLength))
						{
							u32Idx += u16TEncodingLength;
							if(au8EM[u32Idx ++] == u16HashLength)
								s8Ret = m2m_memcmp(&au8EM[u32Idx], pu8SignedMsgHash, u16HashLength);
						}
					}
				}
			}
		}
	}
	return s8Ret;
}


sint8 m2m_crypto_rsa_sign_gen(uint8 *pu8N, uint16 u16NSize, uint8 *pu8d, uint16 u16dSize, uint8 *pu8SignedMsgHash, 
					   uint16 u16HashLength, uint8 *pu8RsaSignature)
{
	sint8		s8Ret = M2M_RSA_SIGN_FAIL;

	if((pu8N != NULL) && (pu8d != NULL) && (pu8RsaSignature != NULL) && (pu8SignedMsgHash != NULL))
	{
		uint16	u16TLength, u16TEncodingLength;
		uint8	*pu8T;
		uint8	au8EM[512];

		/* Selection of correct T Encoding based on the hash size.
		*/
		if(u16HashLength == MD5_DIGEST_SIZE)
		{
			pu8T 				= (uint8*)au8TEncodingMD5;
			u16TEncodingLength	= sizeof(au8TEncodingMD5);
		}
		else if(u16HashLength == SHA1_DIGEST_SIZE)
		{
			pu8T 				= (uint8*)au8TEncodingSHA1;
			u16TEncodingLength	= sizeof(au8TEncodingSHA1);
		}
		else 
		{
			pu8T 				= (uint8*)au8TEncodingSHA2;
			u16TEncodingLength	= sizeof(au8TEncodingSHA2);
		}
		u16TLength = u16TEncodingLength + 1 + u16HashLength;
					
		/* If emLen < tLen + 11.
		*/
		if(u16NSize >= (u16TLength + 11))
		{
			uint16 	u16PSLength = 0;
			uint16	u16Offset	= 0;	

			/* 
			The calculated EM must match the following pattern.
			*======*======*======*======*======*
			* 0x00 || 0x01 || PS || 0x00 || T  *	
			*======*======*======*======*======*
			Where PS is all 0xFF 
			T is defined based on the hash algorithm.
			*/
			au8EM[u16Offset ++]	= 0;
			au8EM[u16Offset ++]	= 1;
			u16PSLength = u16NSize - u16TLength - 3;
			m2m_memset(&au8EM[u16Offset], 0xFF, u16PSLength);
			u16Offset += u16PSLength;
			au8EM[u16Offset ++] = 0;
			m2m_memcpy(&au8EM[u16Offset], pu8T, u16TEncodingLength);
			u16Offset += u16TEncodingLength;
			au8EM[u16Offset ++] = u16HashLength;
			m2m_memcpy(&au8EM[u16Offset], pu8SignedMsgHash, u16HashLength);
			
			/*
			RSA Signature Generation
			*/
			BigInt_ModExp(au8EM, u16NSize, pu8d, u16dSize, pu8N, u16NSize, pu8RsaSignature, u16NSize);
			s8Ret = M2M_RSA_SIGN_OK;
		}
	}
	return s8Ret;
}

#endif /* CONF_CRYPTO */

#ifdef CONF_CRYPTO_SOFT

typedef struct {
	tpfAppCryproCb pfAppCryptoCb;
	uint8 * pu8Digest;
	uint8 * pu8Rsa;
	uint8 u8CryptoBusy;
}tstrCryptoCtxt;

typedef struct {
	uint8 au8N[M2M_MAX_RSA_LEN];
	uint8 au8E[M2M_MAX_RSA_LEN];
	uint8 au8Hash[M2M_SHA256_DIGEST_LEN];
	uint16 u16Nsz;
	uint16 u16Esz;
	uint16 u16Hsz;
	uint8 _pad16_[2];
}tstrRsaPayload;

static tstrCryptoCtxt gstrCryptoCtxt;


/**
*	@fn			m2m_crypto_cb(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr)
*	@brief		WiFi call back function
*	@param [in]	u8OpCode
*					HIF Opcode type.
*	@param [in]	u16DataSize
*					HIF data length.
*	@param [in]	u32Addr
*					HIF address.
*	@author
*	@date
*	@version	1.0
*/
static void m2m_crypto_cb(uint8 u8OpCode, uint16 u16DataSize, uint32 u32Addr)
{
	sint8 ret = M2M_SUCCESS;
	gstrCryptoCtxt.u8CryptoBusy = 0;
	if(u8OpCode == M2M_CRYPTO_RESP_SHA256_INIT)
	{
		tstrM2mSha256Ctxt strCtxt;	
		if (hif_receive(u32Addr, (uint8*) &strCtxt,sizeof(tstrM2mSha256Ctxt), 0) == M2M_SUCCESS)
		{	
			tstrCyptoResp strResp;	
			if(hif_receive(u32Addr + sizeof(tstrM2mSha256Ctxt), (uint8*) &strResp,sizeof(tstrCyptoResp), 1) == M2M_SUCCESS)
			{
				if (gstrCryptoCtxt.pfAppCryptoCb)
					gstrCryptoCtxt.pfAppCryptoCb(u8OpCode,&strResp,&strCtxt);
			}
		}
	}
	else if(u8OpCode == M2M_CRYPTO_RESP_SHA256_UPDATE)
	{
		tstrM2mSha256Ctxt strCtxt;
		if (hif_receive(u32Addr, (uint8*) &strCtxt,sizeof(tstrM2mSha256Ctxt), 0) == M2M_SUCCESS)
		{
			tstrCyptoResp strResp;
			if (hif_receive(u32Addr + sizeof(tstrM2mSha256Ctxt), (uint8*) &strResp,sizeof(tstrCyptoResp), 1) == M2M_SUCCESS)
			{
				if (gstrCryptoCtxt.pfAppCryptoCb)
					gstrCryptoCtxt.pfAppCryptoCb(u8OpCode,&strResp,&strCtxt);
			}
		}

	}
	else if(u8OpCode == M2M_CRYPTO_RESP_SHA256_FINSIH)
	{
		tstrCyptoResp strResp;
		if (hif_receive(u32Addr + sizeof(tstrM2mSha256Ctxt), (uint8*) &strResp,sizeof(tstrCyptoResp), 0) == M2M_SUCCESS)
		{
			if (hif_receive(u32Addr + sizeof(tstrM2mSha256Ctxt) + sizeof(tstrCyptoResp), (uint8*)gstrCryptoCtxt.pu8Digest,M2M_SHA256_DIGEST_LEN, 1) == M2M_SUCCESS)
			{
				if (gstrCryptoCtxt.pfAppCryptoCb)
					gstrCryptoCtxt.pfAppCryptoCb(u8OpCode,&strResp,gstrCryptoCtxt.pu8Digest);
				
			}
		}
	}
	else if(u8OpCode == M2M_CRYPTO_RESP_RSA_SIGN_GEN)
	{
		tstrCyptoResp strResp;
		if (hif_receive(u32Addr + sizeof(tstrRsaPayload), (uint8*)&strResp,sizeof(tstrCyptoResp), 0) == M2M_SUCCESS)
		{
			if (hif_receive(u32Addr + sizeof(tstrRsaPayload) + sizeof(tstrCyptoResp), (uint8*)gstrCryptoCtxt.pu8Rsa,M2M_MAX_RSA_LEN, 0) == M2M_SUCCESS)
			{
				if (gstrCryptoCtxt.pfAppCryptoCb)
					gstrCryptoCtxt.pfAppCryptoCb(u8OpCode,&strResp,gstrCryptoCtxt.pu8Rsa);
			}
		}
	}
	else if(u8OpCode == M2M_CRYPTO_RESP_RSA_SIGN_VERIFY)
	{
		tstrCyptoResp strResp;
		if (hif_receive(u32Addr + sizeof(tstrRsaPayload), (uint8*)&strResp,sizeof(tstrCyptoResp), 1) == M2M_SUCCESS)
		{
			if (gstrCryptoCtxt.pfAppCryptoCb)
				gstrCryptoCtxt.pfAppCryptoCb(u8OpCode,&strResp,NULL);
		}
	}
	else 
	{
		M2M_ERR("u8Code %d ??\n",u8OpCode);
	}

}
/*!
@fn	\
	sint8 m2m_crypto_init();
	
@brief	crypto initialization

@param[in]	pfAppCryproCb

*/
sint8 m2m_crypto_init(tpfAppCryproCb pfAppCryproCb)
{
	sint8 ret = M2M_ERR_FAIL;
	m2m_memset((uint8*)&gstrCryptoCtxt,0,sizeof(tstrCryptoCtxt));
	if(pfAppCryproCb != NULL)
	{
		gstrCryptoCtxt.pfAppCryptoCb = pfAppCryproCb;
		ret = hif_register_cb(M2M_REQ_GROUP_CRYPTO,m2m_crypto_cb);
	}
	return ret;
}
/*!
@fn	\
	sint8 m2m_sha256_hash_init(tstrM2mSha256Ctxt *psha256Ctxt);
	
@brief	SHA256 hash initialization

@param[in]	psha256Ctxt
				Pointer to a sha256 context allocated by the caller.
*/
sint8 m2m_crypto_sha256_hash_init(tstrM2mSha256Ctxt *psha256Ctxt)
{
	sint8  ret = M2M_ERR_FAIL;
	if((psha256Ctxt != NULL)&&(!gstrCryptoCtxt.u8CryptoBusy))
	{
		ret = hif_send(M2M_REQ_GROUP_CRYPTO,M2M_CRYPTO_REQ_SHA256_INIT|M2M_REQ_DATA_PKT,(uint8*)psha256Ctxt,sizeof(tstrM2mSha256Ctxt),NULL,0,0);
	}
	return ret;
}


/*!
@fn	\
	sint8 m2m_sha256_hash_update(tstrM2mSha256Ctxt *psha256Ctxt, uint8 *pu8Data, uint16 u16DataLength);
	
@brief	SHA256 hash update

@param [in]	psha256Ctxt
				Pointer to the sha256 context.
				
@param [in]	pu8Data
				Buffer holding the data submitted to the hash.
				
@param [in]	u16DataLength
				Size of the data bufefr in bytes.
*/
sint8 m2m_crypto_sha256_hash_update(tstrM2mSha256Ctxt *psha256Ctxt, uint8 *pu8Data, uint16 u16DataLength)
{
	sint8  ret = M2M_ERR_FAIL;
	if((!gstrCryptoCtxt.u8CryptoBusy) && (psha256Ctxt != NULL) && (pu8Data != NULL) && (u16DataLength < M2M_SHA256_MAX_DATA))
	{
		ret = hif_send(M2M_REQ_GROUP_CRYPTO,M2M_CRYPTO_REQ_SHA256_UPDATE|M2M_REQ_DATA_PKT,(uint8*)psha256Ctxt,sizeof(tstrM2mSha256Ctxt),pu8Data,u16DataLength,sizeof(tstrM2mSha256Ctxt) + sizeof(tstrCyptoResp));
	}
	return ret;
	
}


/*!
@fn	\
	sint8 m2m_sha256_hash_finish(tstrM2mSha256Ctxt *psha256Ctxt, uint8 *pu8Sha256Digest);
	
@brief	SHA256 hash finalization

@param[in]	psha256Ctxt
				Pointer to a sha256 context allocated by the caller.
				
@param [in] pu8Sha256Digest
				Buffer allocated by the caller which will hold the resultant SHA256 Digest. It must be allocated no less than M2M_SHA256_DIGEST_LEN.
*/
sint8 m2m_crypto_sha256_hash_finish(tstrM2mSha256Ctxt *psha256Ctxt, uint8 *pu8Sha256Digest)
{
	sint8  ret = M2M_ERR_FAIL;
	if((!gstrCryptoCtxt.u8CryptoBusy) && (psha256Ctxt != NULL) && (pu8Sha256Digest != NULL))
	{
		gstrCryptoCtxt.pu8Digest = pu8Sha256Digest;
		ret = hif_send(M2M_REQ_GROUP_CRYPTO,M2M_CRYPTO_REQ_SHA256_FINSIH|M2M_REQ_DATA_PKT,(uint8*)psha256Ctxt,sizeof(tstrM2mSha256Ctxt),NULL,0,0);
	}
	return ret;
}




/*!
@fn	\
	sint8 m2m_rsa_sign_verify(uint8 *pu8N, uint16 u16NSize, uint8 *pu8E, uint16 u16ESize, uint8 *pu8SignedMsgHash, \
		uint16 u16HashLength, uint8 *pu8RsaSignature);
	
@brief	RSA Signature Verification

	The function shall request the RSA Signature verification from the WINC Firmware for the given message. The signed message shall be 
	compressed to the corresponding hash algorithm before calling this function.
	The hash type is identified by the given hash length. For example, if the hash length is 32 bytes, then it is SHA256.

@param[in]	pu8N
				RSA Key modulus n.
				
@param[in]	u16NSize
				Size of the RSA modulus n in bytes.
				
@param[in]	pu8E
				RSA public exponent.
				
@param[in]	u16ESize
				Size of the RSA public exponent in bytes.

@param[in]	pu8SignedMsgHash
				The hash digest of the signed message.
				
@param[in]	u16HashLength
				The length of the hash digest.
				
@param[out] pu8RsaSignature
				Signature value to be verified.
*/


sint8 m2m_crypto_rsa_sign_verify(uint8 *pu8N, uint16 u16NSize, uint8 *pu8E, uint16 u16ESize, uint8 *pu8SignedMsgHash, 
						  uint16 u16HashLength, uint8 *pu8RsaSignature)
{
	sint8 ret = M2M_ERR_FAIL;
	if((!gstrCryptoCtxt.u8CryptoBusy) && (pu8N != NULL) && (pu8E != NULL) && (pu8RsaSignature != NULL) && (pu8SignedMsgHash != NULL) 
	&& (u16NSize != 0) && (u16ESize != 0) && (u16HashLength != 0) && (pu8RsaSignature != NULL) )
	
	{
		tstrRsaPayload strRsa = {0};
		
		m2m_memcpy(strRsa.au8N,pu8N,u16NSize);
		m2m_memcpy(strRsa.au8E,pu8E,u16ESize);
		m2m_memcpy(strRsa.au8Hash,pu8SignedMsgHash,u16HashLength);
		
		strRsa.u16Esz = u16ESize;
		strRsa.u16Hsz = u16HashLength;
		strRsa.u16Nsz = u16NSize;
		
		ret = hif_send(M2M_REQ_GROUP_CRYPTO,M2M_CRYPTO_REQ_RSA_SIGN_VERIFY|M2M_REQ_DATA_PKT,(uint8*)&strRsa,sizeof(tstrRsaPayload),NULL,0,0);
		
	}
	return ret;
}


/*!
@fn	\
	sint8 m2m_rsa_sign_gen(uint8 *pu8N, uint16 u16NSize, uint8 *pu8d, uint16 u16dSize, uint8 *pu8SignedMsgHash, \
		uint16 u16HashLength, uint8 *pu8RsaSignature);
	
@brief	RSA Signature Generation

	The function shall request the RSA Signature generation from the WINC Firmware for the given message. The signed message shall be 
	compressed to the corresponding hash algorithm before calling this function.
	The hash type is identified by the given hash length. For example, if the hash length is 32 bytes, then it is SHA256.

@param[in]	pu8N
				RSA Key modulus n.
				
@param[in]	u16NSize
				Size of the RSA modulus n in bytes.
				
@param[in]	pu8d
				RSA private exponent.
				
@param[in]	u16dSize
				Size of the RSA private exponent in bytes.

@param[in]	pu8SignedMsgHash
				The hash digest of the signed message.
				
@param[in]	u16HashLength
				The length of the hash digest.
				
@param[out] pu8RsaSignature
				Pointer to a user buffer allocated by teh caller shall hold the generated signature.
*/
sint8 m2m_crypto_rsa_sign_gen(uint8 *pu8N, uint16 u16NSize, uint8 *pu8d, uint16 u16dSize, uint8 *pu8SignedMsgHash, 
					   uint16 u16HashLength, uint8 *pu8RsaSignature)
{
	sint8 ret = M2M_ERR_FAIL;
	if((!gstrCryptoCtxt.u8CryptoBusy) && (pu8N != NULL) && (pu8d != NULL) && (pu8RsaSignature != NULL) && (pu8SignedMsgHash != NULL)
	&& (u16NSize != 0) && (u16dSize != 0) && (u16HashLength != 0) && (pu8RsaSignature != NULL))
	
	{
		tstrRsaPayload strRsa = {0};
		
		m2m_memcpy(strRsa.au8N,pu8N,u16NSize);
		m2m_memcpy(strRsa.au8E,pu8d,u16dSize);
		m2m_memcpy(strRsa.au8Hash,pu8SignedMsgHash,u16HashLength);
		
		strRsa.u16Esz = u16dSize;
		strRsa.u16Hsz = u16HashLength;
		strRsa.u16Nsz = u16NSize;
		
		gstrCryptoCtxt.pu8Rsa = pu8RsaSignature;
		ret = hif_send(M2M_REQ_GROUP_CRYPTO,M2M_CRYPTO_REQ_RSA_SIGN_GEN|M2M_REQ_DATA_PKT,(uint8*)&strRsa,sizeof(tstrRsaPayload),NULL,0,0);
		
	}
	return ret;			   
}

#endif