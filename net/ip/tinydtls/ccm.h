/* dtls -- a very basic DTLS implementation
 *
 * Copyright (C) 2011--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _DTLS_CCM_H_
#define _DTLS_CCM_H_

#include "aes/rijndael.h"

/* implementation of Counter Mode CBC-MAC, RFC 3610 */

#define DTLS_CCM_BLOCKSIZE  16	/**< size of hmac blocks */
#define DTLS_CCM_MAX        16	/**< max number of bytes in digest */
#define DTLS_CCM_NONCE_SIZE 12	/**< size of nonce */

/** 
 * Authenticates and encrypts a message using AES in CCM mode. Please
 * see also RFC 3610 for the meaning of \p M, \p L, \p lm and \p la.
 * 
 * \param ctx The initialized rijndael_ctx object to be used for AES operations.
 * \param M   The number of authentication octets.
 * \param L   The number of bytes used to encode the message length.
 * \param N   The nonce value to use. You must provide \c DTLS_CCM_BLOCKSIZE 
 *            nonce octets, although only the first \c 16 - \p L are used.
 * \param msg The message to encrypt. The first \p la octets are additional
 *            authentication data that will be cleartext. Note that the 
 *            encryption operation modifies the contents of \p msg and adds 
 *            \p M bytes MAC. Therefore, the buffer must be at least
 *            \p lm + \p M bytes large.
 * \param lm  The actual length of \p msg.
 * \param aad A pointer to the additional authentication data (can be \c NULL if
 *            \p la is zero).
 * \param la  The number of additional authentication octets (may be zero).
 * \return FIXME
 */
long int
dtls_ccm_encrypt_message(rijndael_ctx *ctx, size_t M, size_t L, 
			 unsigned char nonce[DTLS_CCM_BLOCKSIZE], 
			 unsigned char *msg, size_t lm, 
			 const unsigned char *aad, size_t la);

long int
dtls_ccm_decrypt_message(rijndael_ctx *ctx, size_t M, size_t L, 
			 unsigned char nonce[DTLS_CCM_BLOCKSIZE], 
			 unsigned char *msg, size_t lm, 
			 const unsigned char *aad, size_t la);

#endif /* _DTLS_CCM_H_ */
