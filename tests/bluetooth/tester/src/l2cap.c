/* l2cap.c - Bluetooth L2CAP Tester */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <bluetooth/bluetooth.h>

#include "bttester.h"

#define CONTROLLER_INDEX 0

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint8_t cmds[1];
	struct l2cap_read_supported_commands_rp *rp = (void *) cmds;

	memset(cmds, 0, sizeof(cmds));

	tester_set_bit(cmds, L2CAP_READ_SUPPORTED_COMMANDS);

	tester_send(BTP_SERVICE_ID_L2CAP, L2CAP_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, (uint8_t *) rp, sizeof(cmds));
}

void tester_handle_l2cap(uint8_t opcode, uint8_t index, uint8_t *data,
			 uint16_t len)
{
	switch (opcode) {
	case L2CAP_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_L2CAP, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}
