/* bluetooth.h - HCI core Bluetooth definitions */

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
#ifndef __BT_BLUETOOTH_H
#define __BT_BLUETOOTH_H

#include <bluetooth/buf.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>

/* HCI control APIs */

/* Reset the state of the controller (i.e. perform full HCI init */
int bt_hci_reset(void);

/** @brief Initialize the Bluetooth Subsystem.
 *
 *  Initialize Bluetooth. Must be the called before anything else.
 *  Caller shall be either task or a fiber.
 *
 *  @return zero in success or error code otherwise
 */
int bt_init(void);

/* HCI driver API */

/* Receive data from the controller/HCI driver */
void bt_recv(struct bt_buf *buf);

struct bt_driver {
	/* How much headroom is needed for HCI transport headers */
	size_t head_reserve;

	/* Open the HCI transport */
	int (*open)(void);

	/* Send data to HCI */
	int (*send)(struct bt_buf *buf);
};

/* Register a new HCI driver to the Bluetooth stack */
int bt_driver_register(struct bt_driver *drv);

/* Unregister a previously registered HCI driver */
void bt_driver_unregister(struct bt_driver *drv);

/* Advertising API */

struct bt_eir {
	uint8_t len;
	uint8_t type;
	uint8_t data[29];
} __packed;

/** @brief Start advertising
 *
 *  Set advertisement data,  scan response data, advertisement parameters
 *  and start advertising.
 *
 *  @param type advertising type
 *  @param ad data to be used in advertisement packets
 *  @param sd data to be used in scan response packets
 *
 *  @return zero in success or error code otherwise.
 */
int bt_start_advertising(uint8_t type, const struct bt_eir *ad,
			 const struct bt_eir *sd);
int bt_start_scanning(uint8_t scan_type, uint8_t scan_filter);
int bt_stop_scanning(void);

/** @brief Setting LE connection to peer
 *
 *  Allows initiate new LE link to remote peer using its address
 *
 *  @param peer address of peer's object containing LE address
 *
 *  @return zero in success or error code otherwise.
 */
int bt_connect_le(const bt_addr_le_t *peer);
int bt_disconnect(struct bt_conn *conn, uint8_t reason);

#endif /* __BT_BLUETOOTH_H */
