/* bttester.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define IPC_MTU 32

#define SERVICE_ID_CORE		0
#define SERVICE_ID_GAP		1

#define STATUS_SUCCESS		0x00
#define STATUS_FAILED		0x01
#define STATUS_UNKNOWN_CMD	0x02
#define STATUS_NOT_READY	0x03

struct ipc_hdr {
	uint8_t  service;
	uint8_t  opcode;
	uint16_t len;
	uint8_t  data[0];
} __packed;

#define OP_STATUS		0x00
struct ipc_status {
	uint8_t code;
} __packed;

/* Core Service */
#define OP_REGISTER_SERVICE		0x01
struct cmd_register_service {
	uint8_t id;
} __packed;

/* GAP Service */
#define OP_GAP_START_ADV		0x01
struct cmd_start_advertising {
	uint8_t adv_type;
} __packed;

/* no commands yet */

void tester_init(void);
void tester_rsp(uint8_t service, uint8_t opcode, uint8_t status);

void tester_handle_gap(uint8_t opcode, uint8_t *data, uint16_t len);
uint8_t tester_init_gap(void);
