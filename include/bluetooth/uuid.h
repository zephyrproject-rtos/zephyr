/* uuid.h - Bluetooth UUID definitions */

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
#ifndef __BT_UUID_H
#define __BT_UUID_H

#define BT_UUID_GAP				0x1800
#define BT_UUID_GATT				0x1801
#define BT_UUID_CTS				0x1805
#define BT_UUID_HRS				0x180d
#define BT_UUID_BAS				0x180f
#define BT_UUID_GATT_PRIMARY			0x2800
#define BT_UUID_GATT_SECONDARY			0x2801
#define BT_UUID_GATT_INCLUDE			0x2802
#define BT_UUID_GATT_CHRC			0x2803
#define BT_UUID_GATT_CEP			0x2900
#define BT_UUID_GATT_CUD			0x2901
#define BT_UUID_GATT_CCC			0x2902
#define BT_UUID_GAP_DEVICE_NAME			0x2a00
#define BT_UUID_GAP_APPEARANCE			0x2a01
#define BT_UUID_BATTERY_LEVEL			0x2a19
#define BT_UUID_CURRENT_TIME			0x2a2b
#define BT_UUID_HR_MEASUREMENT			0x2a37
#define BT_UUID_HR_BODY_SENSOR			0x2a38
#define BT_UUID_HR_CONTROL_POINT		0x2a39

/* Bluetooth UUID types */
enum bt_uuid_type {
	BT_UUID_16,
	BT_UUID_128,
};

/* Bluetooth UUID structure */
struct bt_uuid {
	uint8_t  type;
	union {
		uint16_t u16;
		uint8_t  u128[16];
	};
};

int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2);

#endif /* __BT_UUID_H */
