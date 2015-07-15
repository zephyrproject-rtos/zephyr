/* hci_core.h - Bluetooth HCI core access */

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

#include <stdbool.h>
#include <arch/cpu.h>
#include <bluetooth/driver.h>

/* Enabling debug increases stack size requirement considerably */
#if defined(CONFIG_BLUETOOTH_DEBUG)
#define BT_STACK_DEBUG_EXTRA	512
#else
#define BT_STACK_DEBUG_EXTRA	0
#endif

#define BT_STACK(name, size) \
		char __stack name[(size) + BT_STACK_DEBUG_EXTRA]
#define BT_STACK_NOINIT(name, size) \
		char __noinit __stack name[(size) + BT_STACK_DEBUG_EXTRA]

/* LMP feature helpers */
#define lmp_bredr_capable(dev)	(!((dev).features[4] & BT_LMP_NO_BREDR))
#define lmp_le_capable(dev)	((dev).features[4] & BT_LMP_LE)

/* State tracking for the local Bluetooth controller */
struct bt_dev {
	/* Local Bluetooth Device Address */
	bt_addr_t		bdaddr;

	/* Controller version & manufacturer information */
	uint8_t			hci_version;
	uint16_t		hci_revision;
	uint16_t		manufacturer;

	/* BR/EDR features page 0 */
	uint8_t			features[8];

	/* LE features */
	uint8_t			le_features[8];

	/* Advertising state */
	uint8_t                 adv_enable;

	/* Scanning state */
	uint8_t			scan_enable;
	uint8_t			scan_filter;

	/* Controller buffer information */
	uint8_t			le_pkts;
	uint16_t		le_mtu;
	struct nano_sem		le_pkts_sem;

	/* Number of commands controller can accept */
	uint8_t			ncmd;
	struct nano_sem		ncmd_sem;

	/* Last sent HCI command */
	struct bt_buf		*sent_cmd;

	/* Queue for incoming HCI events & ACL data */
	struct nano_fifo	rx_queue;

	/* Queue for high priority HCI events which may unlock waiters
	 * in other fibers. Such events include Number of Completed
	 * Packets, as well as the Command Complete/Status events.
	 */
	struct nano_fifo	rx_prio_queue;

	/* Queue for outgoing HCI commands */
	struct nano_fifo	cmd_tx_queue;

	/* Registered HCI driver */
	struct bt_driver	*drv;
};

extern struct bt_dev bt_dev;

static inline int bt_addr_cmp(const bt_addr_t *a, const bt_addr_t *b)
{
	return memcmp(a, b, sizeof(*a));
}

static inline int bt_addr_le_cmp(const bt_addr_le_t *a, const bt_addr_le_t *b)
{
	return memcmp(a, b, sizeof(*a));
}

static inline void bt_addr_copy(bt_addr_t *dst, const bt_addr_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

static inline void bt_addr_le_copy(bt_addr_le_t *dst, const bt_addr_le_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

static inline bool bt_addr_le_is_rpa(const bt_addr_le_t *addr)
{
	if (addr->type != BT_ADDR_LE_RANDOM)
		return false;

	if ((addr->val[5] & 0xc0) == 0x40)
	       return true;

	return false;
}

static inline bool bt_addr_le_is_identity(const bt_addr_le_t *addr)
{
	if (addr->type == BT_ADDR_LE_PUBLIC)
		return true;

	/* Check for Random Static address type */
	if ((addr->val[5] & 0xc0) == 0xc0)
		return true;

	return false;
}

struct bt_buf *bt_hci_cmd_create(uint16_t opcode, uint8_t param_len);
int bt_hci_cmd_send(uint16_t opcode, struct bt_buf *buf);
int bt_hci_cmd_send_sync(uint16_t opcode, struct bt_buf *buf,
			 struct bt_buf **rsp);

/* The helper is only safe to be called from internal fibers as it's
 * not multi-threading safe
 */
#if defined(CONFIG_BLUETOOTH_DEBUG)
const char *bt_addr_str(const bt_addr_t *addr);
const char *bt_addr_le_str(const bt_addr_le_t *addr);
#endif

int bt_le_scan_update(void);
