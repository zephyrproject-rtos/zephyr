/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The BK7258 mailbox (mbox0): report "powered up" to the peer and keep a
 * periodic heartbeat going.
 *
 * This is a requirement, not an optimisation. The peer supervises this image:
 * it waits 2 s for the power-up indication after releasing it and 6 s between
 * heartbeats, and resets the whole chip when they stop. An image that never
 * talks over the mailbox therefore reboots about 8 s after power-on with no
 * console output at all.
 *
 * The link also works in the other direction: received command blocks are
 * handed to a callback, and each one is acknowledged. Without the ACK the
 * peer's server socket stays busy and answers every later request with a
 * failure.
 *
 * Registers and protocol constants:
 *   ap/middleware/soc/bk7258_ap/soc/mbox0_struct.h        register layout
 *   ap/middleware/soc/common/hal/mbox0_hal.c:69-110       send/receive sequence
 *   ap/middleware/driver/mailbox/mbox0_drv.c:37,95        channel ownership
 *   ap/middleware/driver/mailbox/mbox0_adapter.c:73-129   message = addr + len
 *   ap/middleware/driver/mailbox/mailbox_channel.c:194    physical command block
 */

#include <soc.h>

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc_bk7258_ipc, CONFIG_SOC_LOG_LEVEL);

/*
 * Command block: mb_chnl_cmd_t = { hdr, param1, param2, param3 }, 16 bytes.
 *
 * It has to live in shared memory: the mailbox FIFO carries the *address* of
 * this block rather than the data (mbox0_adapter.c:119-120) and the peer reads
 * it
 * from there. A stack variable would be clobbered as soon as the function
 * returns.
 */
static uint32_t mb_cmd_buf[4];
static uint8_t mb_tx_seq;

/*
 * Heartbeat payload.
 *
 * An outgoing IPC command block is not a bare mb_chnl_cmd_t: it is wrapped in
 * an IPC header,
 *
 *       typedef union {
 *           struct {
 *               mb_chnl_hdr_t chnl_hdr;     // word0: cmd[7:0]
 *               void         *cmd_buff;     // word1: payload pointer
 *               u16           cmd_data_len; // word2 low 16 bits: length
 *           };
 *           mb_chnl_cmd_t mb_cmd;           // {hdr, param1, param2, param3}
 *       } ipc_cmd_t;                        // cp/mb_ipc_cmd.c:49-59
 *
 * so param1 is cmd_buff (a pointer) and param2 is cmd_data_len (a length).
 *
 * The peer's handler for the heartbeat indication (cp/mb_ipc_cmd.c:663-674)
 * only calls through when the declared payload length is at least 4 bytes.
 *
 * With cmd_data_len = 0 the heartbeat is silently dropped: the peer reports no
 * error and does not refresh its timestamp either, so the chip is reset 6 s
 * later with nothing in between pointing at the payload.
 *
 * The power-up case does not look at the length, so it goes through even with a
 * zero payload.
 *
 * The peer copies these 4 bytes away, so they have to be stable shared memory
 * and not a stack variable.
 */
static uint32_t mb_hb_param;

/* Make sure channel N is enabled. The peer configures every channel at boot;
 * this only acts as a fallback when it did not.
 */
static void mb_chn_ensure_enabled(uint32_t chn)
{
	uint32_t en = sys_read32(SOC_MBOX0_CHN_ADDR(chn, SOC_MBOX0_CHN_ENABLE));

	if ((en & BIT(0)) != 0U) {
		/* Already configured: leave it alone rather than disturb a FIFO
		 * that may be in use.
		 */
		return;
	}

	/*
	 * Fallback configuration: fifo_start / fifo_length, and the start
	 * address has to be written before the enable bit. Values copied from
	 * ap/middleware/driver/bk7258_ap/mbox0_fifo_cfg.h:
	 *   chn0: start=0 len=2, chn1: start=2 len=4, chn2: start=6 len=2
	 */
	static const uint8_t fifo_start[3] = {0U, 2U, 6U};
	static const uint8_t fifo_len[3] = {2U, 4U, 2U};

	if (chn >= 3U) {
		return;
	}

	sys_write32(fifo_start[chn] & 0x3FU, SOC_MBOX0_CHN_ADDR(chn, SOC_MBOX0_CHN_FIFO_CFG));
	sys_write32(BIT(0) | ((uint32_t)fifo_len[chn] << 1),
		    SOC_MBOX0_CHN_ADDR(chn, SOC_MBOX0_CHN_ENABLE));
}

/* Dedicated command block for ACKs. It cannot be mb_cmd_buf: the heartbeat
 * overwrites that every 2 s and we do not control when the peer reads these
 * 16 bytes.
 */
static uint32_t mb_ack_buf[4];

/*
 * Push "address + length" into the peer FIFO.
 *
 * The mailbox never carries the data itself, only the address of the command
 * block (mbox0_adapter.c:73-129). Writing MAIL_TID is what actually triggers
 * the transfer, so the data has to be visible in memory first (DMB).
 */
static int mb_tx_addr(uint32_t addr, uint32_t len)
{
	/* Drop this one when the peer FIFO is full */
	if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CHN, SOC_MBOX0_CHN_FIFO_STAT)) &
	     SOC_MBOX0_FIFO_STAT_FULL_BIT) != 0U) {
		return -EBUSY;
	}

	/* The data has to be visible to the peer before the doorbell */
	barrier_dmem_fence_full();

	sys_write32(addr, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_TDATA0));
	sys_write32(len, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_TDATA1));
	sys_write32(SOC_MB_PEER_CHN, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_MAIL_TID));

	return 0;
}

/*
 * Automatic logical-channel ACK.
 *
 * The vendor mailbox_channel.c:441-449 says "RE-USE the cmd buffer for ACK",
 * which reads like an in-place reuse but is not: it ends up in
 * bk_mailbox_send_safe(), and bk_mailbox_send() in mbox0_adapter.c:73-127
 * memcpy's the content into a rotating slot of its own, mailbox_buff[dst][box],
 * then pushes the *slot address* into the FIFO:
 *
 *       mailbox_tx_box[dst] = (mailbox_tx_box[dst] + 1) % TX_BOX_NUM;
 *       memcpy(&mailbox_buff[dst][box], data, sizeof(mailbox_data_t));
 *       message.data[0] = (u32)&mailbox_buff[dst][box];
 *
 * So patching the buffer at addr and pushing it back is wrong: that is the
 * *sender's* transmit slot, which the sender will reuse for its next message
 * at any time (the slots rotate), and the header we write there can collide
 * with that next send. The correct move is to send back a copy, which is what
 * the small rotating ACK pool below is for.
 */
#define MB_ACK_POOL_NUM 8U
static uint32_t mb_ack_pool[MB_ACK_POOL_NUM][4];
static uint8_t mb_ack_pool_idx;

static void mb_auto_ack(uint32_t addr, uint32_t hdr)
{
	uint32_t *b = &mb_ack_pool[mb_ack_pool_idx][0];

	mb_ack_pool_idx = (mb_ack_pool_idx + 1U) % MB_ACK_POOL_NUM;

	/* Only the header changes: clear state[11:8] and set ACK_BOX in ctrl.
	 * cmd / tx_seq / logical_chnl must be preserved verbatim, because the
	 * peer claims this ACK by the latter two.
	 */
	b[0] = (hdr & 0xFFFFF0FFU) | (SOC_MB_CTRL_ACK_BOX << 12);
	b[1] = sys_read32(addr + 4U);
	b[2] = sys_read32(addr + 8U);
	b[3] = sys_read32(addr + 12U);

	/* Count only what was actually sent: a full peer FIFO drops the ACK
	 * silently, and that drop is the most common cause of the peer timing
	 * out waiting for an ACK, so the counter has to show it.
	 */
	(void)mb_tx_addr((uint32_t)b, SOC_MB_IPC_MSG_LEN);
}

static int mb_ack(uint32_t addr, uint32_t hdr, uint32_t w1, uint32_t w2, uint32_t w3);

/*
 * Drain our own receive FIFO and answer what has to be answered.
 *
 * The FIFO only holds an address (rdata0) and a length (rdata1); the real 16-byte
 * command block sits in shared memory at that address and has to be read from
 * there (mbox0_adapter.c:73-129).
 *
 * Order matters: read mail_sid first, because reading it is what starts the
 * receive and pops the entry, then take rdata0/rdata1. Reversing the order reads
 * leftovers from the previous entry.
 */
static void mb_pump(void)
{
	uint32_t guard = 8U; /* FIFO is 4 deep; 8 guards against a spin */

	while (guard-- != 0U) {
		if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_FIFO_STAT)) &
		     SOC_MBOX0_FIFO_STAT_EMPT_BIT) != 0U) {
			break;
		}

		(void)sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_MAIL_SID));
		uint32_t addr =
			sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_RDATA0));
		uint32_t len =
			sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_RDATA1));

		/*
		 * The address comes from the peer, so do not dereference it
		 * blindly. Only pointers inside SRAM (0x28000000..0x280A0000)
		 * are accepted: a zero or a garbled pointer would throw the AP
		 * straight into a HardFault, which shows up as "I sent a test
		 * command and the board vanished" and is very hard to trace.
		 */
		bool valid = (addr >= SOC_SRAM0_DATA_BASE) &&
			     (addr < (SOC_SRAM0_DATA_BASE + 0xA0000U)) && ((addr & 0x3U) == 0U);
		uint32_t w0 = 0U, w2 = 0U;

		if (valid) {
			w0 = sys_read32(addr + 0U);
			w2 = sys_read32(addr + 8U);
		}

		LOG_DBG("mbox0 rx: addr=%#x len=%u valid=%d hdr=%#x", addr, len, valid, w0);

		/*
		 * Answer before doing anything else with the entry: the peer
		 * blocks on this ACK and times out otherwise, and if it sends
		 * something else meanwhile the ACK is judged stale (tx_seq no
		 * longer matches) and dropped.
		 */
		if (valid && !SOC_MB_HDR_IS_ACK(w0) && !SOC_MB_HDR_IS_SYNC_TX(w0)) {
			if ((w0 >> 24) == SOC_MB_CHNL_PEER_FLASH) {
				/*
				 * Flash operation notice: this needs an
				 * *application-level* ACK with w1 set to
				 * IPC_FLASH_OP_ACK (2)
				 * (flash_notify.c:65-100).
				 */
				(void)mb_ack(addr, w0, SOC_MB_FLASH_OP_ACK, w2,
					     SOC_MB_ACK_STATE_COMPLETE);
			} else {
				mb_auto_ack(addr, w0);
			}
		}
	}
}

static int mb_ack(uint32_t addr, uint32_t hdr, uint32_t w1, uint32_t w2, uint32_t w3)
{
	/*
	 * An ACK is the received command block sent straight back with three
	 * changes:
	 *   (1) clear state[11:8] (leaving it set reads as CHNL_STATE_COM_FAIL)
	 *   (2) set CHNL_CTRL_ACK_BOX in ctrl[15:12] (bit 0 -> header bit 12)
	 *   (3) fill ack_data1 / ack_state, i.e. word1 / word3
	 * cmd[7:0], tx_seq[23:16] and logical_chnl[31:24] must be preserved
	 * verbatim: the peer's mb_phy_chnl_rx_ack_isr() claims the ACK by
	 * tx_seq plus logical_chnl.
	 *
	 * Why the ACK does not use "the other" physical box: mb_chnl_write does
	 * distinguish BOX0 (command) from BOX1 (ACK), but the code in
	 * bk_mailbox_send() that picks a buffer by box is commented out
	 * (mbox0_adapter.c:88-98). In practice both go through the same FIFO and
	 * are told apart only by that one bit in hdr.ctrl.
	 */
	uint32_t h = (hdr & 0xFFFFF0FFU) | (SOC_MB_CTRL_ACK_BOX << 12);

	if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CHN, SOC_MBOX0_CHN_FIFO_STAT)) &
	     SOC_MBOX0_FIFO_STAT_FULL_BIT) != 0U) {
		return -EBUSY;
	}

	mb_ack_buf[0] = h;
	mb_ack_buf[1] = w1;
	mb_ack_buf[2] = w2;
	mb_ack_buf[3] = w3;

	return mb_tx_addr((uint32_t)&mb_ack_buf[0], SOC_MB_IPC_MSG_LEN);
}

static int mb_send(uint32_t chnl, uint32_t cmd, uint32_t p1, uint32_t p2)
{
	/* Drop this tick when the destination FIFO is full: losing one heartbeat
	 * is harmless, the next one follows 2 s later.
	 */
	if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CHN, SOC_MBOX0_CHN_FIFO_STAT)) &
	     SOC_MBOX0_FIFO_STAT_FULL_BIT) != 0U) {
		return -EBUSY;
	}

	mb_tx_seq++;
	mb_cmd_buf[0] = SOC_MB_HDR_CH(chnl, cmd, mb_tx_seq);
	mb_cmd_buf[1] = p1; /* == ipc_cmd.cmd_buff     (payload pointer) */
	mb_cmd_buf[2] = p2; /* == ipc_cmd.cmd_data_len (length, low 16) */
	mb_cmd_buf[3] = 0U;

	/*
	 * The data has to be visible to the peer before the doorbell. A DMB is
	 * enough with no D-Cache; if a D-Cache is ever enabled, or the command
	 * block moves to a cacheable region, this has to become a cache clean.
	 */
	barrier_dmem_fence_full();

	sys_write32((uint32_t)&mb_cmd_buf[0],
		    SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_TDATA0));
	sys_write32(SOC_MB_IPC_MSG_LEN, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_TDATA1));
	/* Writing mail_tid = destination CPU is what actually triggers the send */
	sys_write32(SOC_MB_PEER_CHN, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CHN, SOC_MBOX0_CHN_MAIL_TID));

	return 0;
}

void soc_bk7258_ipc_power_up(void)
{
	int rc;

	mb_chn_ensure_enabled(SOC_MB_SELF_CHN);
	mb_pump();

	/* POWER_UP carries no payload: ipc_send_cmd(..., NULL, 0, ...) */
	rc = mb_send(SOC_MB_CHNL_HW_CTRL, SOC_MB_IPC_CMD_POWER_UP, 0U, 0U);
	if (rc != 0) {
		LOG_ERR("power-up indication not delivered: %d", rc);
	}
}

static int mb_heartbeat(void)
{
	mb_pump();

	/*
	 * HEART_BEAT: the vendor ipc_send_heart_beat(param) is
	 * ipc_send_cmd(..., (u8 *)&param, sizeof(param), ...), that is
	 * cmd_buff = &param and cmd_data_len = 4.
	 *
	 * cmd_data_len must be >= 4 or the peer drops it outright (see the comment on
	 * mb_hb_param).
	 */
	mb_hb_param = 0U;
	return mb_send(SOC_MB_CHNL_HW_CTRL, SOC_MB_IPC_CMD_HEARTBEAT, (uint32_t)&mb_hb_param,
		       sizeof(mb_hb_param));
}

/*
 * Heartbeat tick
 *
 * A k_timer rather than a thread: its callback runs in interrupt context, so a
 * wedged application thread does not stop it. The flip side is intended by the
 * vendor design: if the application really hangs, the heartbeat stops, the peer
 * resets the chip, and the mechanism acts as a watchdog.
 */

static struct k_timer mb_hb_timer;

static void mb_hb_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	int rc = mb_heartbeat();

	if (rc != 0) {
		/* Dropping one tick is harmless, the next one follows in 2 s. */
		LOG_DBG("heartbeat not delivered: %d", rc);
	}
}

static int soc_bk7258_ipc_init(void)
{
	/*
	 * The power-up notification is issued separately at PRE_KERNEL_1 (see
	 * soc.c), much earlier than this point. The peer waits 2 s, but the earlier
	 * this happens the better, so that no later step can push it past that
	 * window.
	 */
	k_timer_init(&mb_hb_timer, mb_hb_handler, NULL);
	/* Vendor MB_IPC_HEARTBEAT_TIME is 2000 ms; 3 missed ticks are tolerated */
	k_timer_start(&mb_hb_timer, K_MSEC(2000), K_MSEC(2000));

	LOG_INF("mbox0 heartbeat started: every 2000 ms (peer chn=%d, self chn=%d)",
		SOC_MB_PEER_CHN, SOC_MB_SELF_CHN);

	return 0;
}

SYS_INIT(soc_bk7258_ipc_init, POST_KERNEL, 0);
