/*
 * Copyright (c) 2014-2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GREYBUS_I2C_H_
#define _GREYBUS_I2C_H_

#include <greybus/types.h>

#define GB_I2C_VERSION_MAJOR                0
#define GB_I2C_VERSION_MINOR                1

#define GB_I2C_PROTOCOL_VERSION             0x01
#define GB_I2C_PROTOCOL_FUNCTIONALITY       0x02
#define GB_I2C_PROTOCOL_TRANSFER            0x05

#define GB_I2C_FUNC_I2C                     0x00000001
#define GB_I2C_FUNC_10BIT_ADDR              0x00000002
#define GB_I2C_FUNC_SMBUS_PEC               0x00000008
#define GB_I2C_FUNC_NOSTART                 0x00000010
#define GB_I2C_FUNC_SMBUS_BLOCK_PROC_CALL   0x00008000
#define GB_I2C_FUNC_SMBUS_QUICK             0x00010000
#define GB_I2C_FUNC_SMBUS_READ_BYTE         0x00020000
#define GB_I2C_FUNC_SMBUS_WRITE_BYTE        0x00040000
#define GB_I2C_FUNC_SMBUS_READ_BYTE_DATA    0x00080000
#define GB_I2C_FUNC_SMBUS_WRITE_BYTE_DATA   0x00100000
#define GB_I2C_FUNC_SMBUS_READ_WORD_DATA    0x00200000
#define GB_I2C_FUNC_SMBUS_WRITE_WORD_DATA   0x00400000
#define GB_I2C_FUNC_SMBUS_PROC_CALL         0x00800000
#define GB_I2C_FUNC_SMBUS_READ_BLOCK_DATA   0x01000000
#define GB_I2C_FUNC_SMBUS_WRITE_BLOCK_DATA  0x02000000
#define GB_I2C_FUNC_SMBUS_READ_I2C_BLOCK    0x04000000
#define GB_I2C_FUNC_SMBUS_WRITE_I2C_BLOCK   0x08000000

#define GB_I2C_M_RD                         0x0001
#define GB_I2C_M_TEN                        0x0010
#define GB_I2C_M_RECV_LEN                   0x0400
#define GB_I2C_M_NOSTART                    0x4000


/* version request has no payload */
struct gb_i2c_proto_version_response {
	__u8	major;
	__u8	minor;
} __packed;

struct gb_i2c_functionality_rsp {
	__le32	functionality;
} __packed;

struct gb_i2c_transfer_desc {
	__le16	addr;
	__le16	flags;
	__le16	size;
} __packed;

struct gb_i2c_transfer_req {
	__le16	op_count;
	struct gb_i2c_transfer_desc desc[0];
} __packed;

struct gb_i2c_transfer_rsp {
	__u8	data[0];
} __packed;

#endif /* _GREYBUS_I2C_H_ */

