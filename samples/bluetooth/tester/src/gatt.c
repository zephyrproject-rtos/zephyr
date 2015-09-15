/* gatt.c - Bluetooth GATT Server Tester */

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

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <toolchain.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <misc/byteorder.h>
#include <misc/printk.h>

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define MAX_ATTRIBUTES 20
#define MAX_BUFFER_SIZE 512

static struct bt_gatt_attr gatt_db[MAX_ATTRIBUTES];

static struct {
	uint8_t len;
	uint8_t buf[MAX_BUFFER_SIZE];
} gatt_buf;

static void *gatt_buf_add(const void *data, size_t len)
{
	void *ptr;

	if ((len + gatt_buf.len) > ARRAY_SIZE(gatt_buf.buf)) {
		return NULL;
	}

	ptr = memcpy(gatt_buf.buf + gatt_buf.len, data, len);
	gatt_buf.len += len;

	printk("gatt_buf: %d/%d used\n", gatt_buf.len, MAX_BUFFER_SIZE);

	return ptr;
}

static struct bt_gatt_attr *gatt_db_add(const struct bt_gatt_attr *pattern)
{
	struct bt_gatt_attr *attr;
	static int i = 0;

	if (i == ARRAY_SIZE(gatt_db)) {
		return NULL;
	}

	attr = &gatt_db[i];
	memcpy(attr, pattern, sizeof(*attr));
	attr->handle = ++i;

	printk("gatt_db: attribute added, handle %x\n", attr->handle);

	return attr;
}

static struct bt_gatt_attr svc_pri = BT_GATT_PRIMARY_SERVICE(0x0000, NULL);
static struct bt_gatt_attr svc_sec = BT_GATT_SECONDARY_SERVICE(0x0000, NULL);

static void add_service(uint8_t *data, uint16_t len)
{
	const struct gatt_add_service_cmd *cmd = (void *) data;
	struct gatt_add_service_rp rp;
	struct bt_gatt_attr *attr_svc;
	struct bt_uuid uuid;
	uint8_t status;
	uint16_t val;

	switch (cmd->uuid_length) {
	case 0x02: /* UUID 16 */
		uuid.type = BT_UUID_16;
		memcpy(&val, cmd->uuid, sizeof(val));
		uuid.u16 = sys_le16_to_cpu(val);
		break;
	case 0x10: /* UUID 128*/
		uuid.type = BT_UUID_128;
		memcpy(&uuid.u128, cmd->uuid, sizeof(uuid.u128));
		break;
	default:
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	switch (cmd->type) {
	case GATT_SERVICE_PRIMARY:
		attr_svc = gatt_db_add(&svc_pri);
		break;
	case GATT_SERVICE_SECONDARY:
		attr_svc = gatt_db_add(&svc_sec);
		break;
	default:
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	if (!attr_svc) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	attr_svc->user_data = gatt_buf_add(&uuid, sizeof(uuid));
	if (!attr_svc->user_data) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	rp.svc_id = sys_cpu_to_le16(attr_svc->handle);
	status = BTP_STATUS_SUCCESS;

rsp:
	if (status != BTP_STATUS_SUCCESS) {
		tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_SERVICE,
			   CONTROLLER_INDEX, status);
	} else {
		tester_rsp_full(BTP_SERVICE_ID_GATT, GATT_ADD_SERVICE,
				CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));
	}
}

static void start_server(uint8_t *data, uint16_t len)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(gatt_db); i++) {
		if (!gatt_db[i].handle) {
			break;
		}
	}

	if (gatt_db[ARRAY_SIZE(gatt_db) - 1].handle) {
		i++;
	}

	bt_gatt_register(gatt_db, i);

	tester_rsp(BTP_SERVICE_ID_GATT, GATT_START_SERVER,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

void tester_handle_gatt(uint8_t opcode, uint8_t index, uint8_t *data,
			 uint16_t len)
{
	switch (opcode) {
	case GATT_ADD_SERVICE:
		add_service(data, len);
		return;
	case GATT_START_SERVER:
		start_server(data, len);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_GATT, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

uint8_t tester_init_gatt(void)
{
	memset(&gatt_buf, 0, sizeof(gatt_buf));
	memset(&gatt_db, 0, sizeof(gatt_db));

	return BTP_STATUS_SUCCESS;
}
