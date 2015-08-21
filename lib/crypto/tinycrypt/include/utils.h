/* utils.h - TinyCrypt interface to platform-dependent run-time operations */

/*
 *  Copyright (C) 2015 by Intel Corporation, All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *    - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    - Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief Interface to platform-dependent run-time operations.
 *
 */

#ifndef __TC_UTILS_H__
#define __TC_UTILS_H__

#include <stdint.h>

#define TC_SUCCESS 1
#define TC_FAIL 0
#define TC_RESEED_REQ -1

/**
 * @brief Copy the the buffer 'from' to the buffer 'to'.
 *
 * @return returns TC_FAIL (0) if:
 *         from_len > to_len.
 */
uint32_t _copy(uint8_t *to, uint32_t to_len, const uint8_t *from, uint32_t from_len);

/**
 * @brief Set the value 'val' into the buffer 'to', 'len' times.
 */
void _set(uint8_t *to, uint8_t val, uint32_t len);

/**
 * @brief This is an AES specific doubling function, which utilizes the
 * finite field used by AES.
 */
uint8_t _double_byte(uint8_t a);

#endif
