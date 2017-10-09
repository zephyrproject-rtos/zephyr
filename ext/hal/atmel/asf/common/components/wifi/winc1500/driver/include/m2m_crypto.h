/**
 *
 * \file
 *
 * \brief WINC Crypto Application Interface.
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

#ifndef __M2M_CRYPTO_H__
#define __M2M_CRYPTO_H__


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


#include "common/include/nm_common.h"
#include "driver/include/m2m_types.h"
#include "driver/source/m2m_hif.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#define M2M_MAX_RSA_LEN					(256)
#define M2M_SHA256_DIGEST_LEN			32
#define M2M_SHA256_MAX_DATA				(M2M_BUFFER_MAX_SIZE - M2M_SHA256_CONTEXT_BUFF_LEN - M2M_HIF_HDR_OFFSET)
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

/*!
@struct	\
	tstrM2mSha256Ctxt

@brief
	SHA256 context data
*/
typedef struct sha256ctxt{
	uint32	au32Sha256CtxtBuff[M2M_SHA256_CONTEXT_BUFF_LEN/sizeof(uint32)];
} tstrM2mSha256Ctxt;



/*!
@enum	\
	tenuRsaSignStatus

@brief
	RSA Signature status: pass or fail.
	
@see
	m2m_crypto_rsa_sign_gen
*/
typedef enum{
	M2M_RSA_SIGN_OK,
	M2M_RSA_SIGN_FAIL
} tenuRsaSignStatus;

/*!
@typedef \
	tpfAppCryproCb

@brief			Crypto Calback function receiving the crypto related messages
@param [in]	u8MsgType
				Crypto command about which the notification is received.
@param [in]	pvResp
				A pointer to the result associated with the notification.  				
@param [in]	pvMsg
				A pointer to a buffer containing the notification parameters (if any). It should be
				Casted to the correct data type corresponding to the notification type.
@see
	m2m_crypto_init
	tenuM2mCryptoCmd
*/
typedef void (*tpfAppCryproCb) (uint8 u8MsgType,void * pvResp, void * pvMsg);

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
FUNCTION PROTOTYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


#ifdef __cplusplus
     extern "C" {
#endif
/*!
@fn	\
	sint8 m2m_crypto_init();
	
@brief	crypto initialization.

@param[in]	pfAppCryproCb
				Pointer to the Crypto Calback function receiving the crypto related messages.
@see
	tpfAppCryproCb
	
@return		
	The function returns @ref M2M_SUCCESS for successful operation and a negative value otherwise.
*/
sint8 m2m_crypto_init(tpfAppCryproCb pfAppCryproCb);
/*!
@fn	\
	sint8 m2m_sha256_hash_init(tstrM2mSha256Ctxt *psha256Ctxt);
	
@brief	SHA256 hash initialization

@param[in]	psha256Ctxt
				Pointer to a sha256 context allocated by the caller.				
@return		
	The function returns @ref M2M_SUCCESS for successful operation and a negative value otherwise.
*/
sint8 m2m_crypto_sha256_hash_init(tstrM2mSha256Ctxt *psha256Ctxt);


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
@pre SHA256 module should be initialized first through m2m_crypto_sha256_hash_init function.

@see m2m_crypto_sha256_hash_init

@return		
	The function returns @ref M2M_SUCCESS for successful operation and a negative value otherwise.

*/
sint8 m2m_crypto_sha256_hash_update(tstrM2mSha256Ctxt *psha256Ctxt, uint8 *pu8Data, uint16 u16DataLength);


/*!
@fn	\
	sint8 m2m_sha256_hash_finish(tstrM2mSha256Ctxt *psha256Ctxt, uint8 *pu8Sha256Digest);
	
@brief	SHA256 hash finalization

@param[in]	psha256Ctxt
				Pointer to a sha256 context allocated by the caller.
				
@param [in] pu8Sha256Digest
				Buffer allocated by the caller which will hold the resultant SHA256 Digest. It must be allocated no less than M2M_SHA256_DIGEST_LEN.
				
@return		
	The function returns @ref M2M_SUCCESS for successful operation and a negative value otherwise.
*/
sint8 m2m_crypto_sha256_hash_finish(tstrM2mSha256Ctxt *psha256Ctxt, uint8 *pu8Sha256Digest);


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
				
@return		
	The function returns @ref M2M_SUCCESS for successful operation and a negative value otherwise.
*/
sint8 m2m_crypto_rsa_sign_verify(uint8 *pu8N, uint16 u16NSize, uint8 *pu8E, uint16 u16ESize, uint8 *pu8SignedMsgHash, 
						  uint16 u16HashLength, uint8 *pu8RsaSignature);


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
				
@return		
	The function returns @ref M2M_SUCCESS for successful operation and a negative value otherwise.
*/
sint8 m2m_crypto_rsa_sign_gen(uint8 *pu8N, uint16 u16NSize, uint8 *pu8d, uint16 u16dSize, uint8 *pu8SignedMsgHash, 
					   uint16 u16HashLength, uint8 *pu8RsaSignature);
#ifdef __cplusplus
}
#endif


#endif /* __M2M_CRYPTO_H__ */
