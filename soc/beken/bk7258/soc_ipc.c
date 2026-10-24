/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal slave-side implementation of the BK7258 inter-core mailbox (mbox0):
 * report "powered up" to the system controller and keep a periodic heartbeat going.
 *
 * Why this file has to exist
 * --------------------------
 * After boot the system controller releases this core and enters a supervision state
 * machine (ap/middleware/driver/mailbox/mb_ipc_heartbeat.c:191-236):
 *
 * 1. It sets MB_IPC_START_CORE_FLAG and waits 2 s for MB_IPC_POWER_UP_FLAG.
 * Timing out only prints "IPC retry to start core1" and carries on.
 * 2. It then supervises the heartbeat: no heartbeat within
 * HB_TIMEOUT_MS = 2000*3 = 6 s triggers BK_ASSERT(false), which resets
 * the whole chip.
 *
 * So a Zephyr image that never talks across cores gets reset by the system controller
 * about 8 s after power-on, and the console shows a board rebooting over
 * and over with no output from this core at all.
 *
 * What is implemented here is the equivalent of the vendor slave task
 * (mb_ipc_heartbeat.c:341-350):
 * ipc_send_power_up();
 * while (1) { rtos_delay_milliseconds(2000); ipc_send_heart_beat(0); }
 *
 * This implementation covers both directions:
 * - Received messages are no longer dropped: the 16-byte command block is
 * parsed and handed to a callback (soc_bk7258_mbox_set_rx_cb).
 * - the system controller's flash operation notices (logical channel 0x4B) are ACKed
 * automatically.
 * - Every command we receive gets a logical-channel ACK. Without it
 * the system controller's mb_ipc server socket stays RX_BUSY forever: CONNECT succeeds yet
 * every later SEND fails, which is deeply misleading.
 * - Verified on hardware: three IPC_TEST_CMD round trips all passed, and the
 * outbound LOG channel makes strings we send appear on the system controller's console.
 *
 * Two layers check an ACK before the waiter is released (source-verified):
 * 1. mailbox_channel.c mb_phy_chnl_rx_ack_isr() compares the ACK's tx_seq and
 * logical_chnl with the physical channel's CURRENT outbound values. A
 * mismatch is dropped as stale (rx_fault_cnt |= 0x40000000), and in SDK
 * builds before commit 0e5fd0d22 it left tx_state BUSY for ever, which
 * deadlocks every later write on that physical channel.
 * 2. mb_ipc.c ipc_router_tx_cmpl_isr() then matches the head of the channel's
 * tx queue by hdr.cmd and (param1 & 0x00FFFFFF) -- tx_seq is already
 * cleared by then, so it plays no part in this layer.
 * Only when both match does ipc_socket_tx_cmpl_handler() release the sender's
 * tx_notify_sema. One physical channel carries several logical channels
 * (mb_ipc plus the LOG channel), so step 1 is inherently racy: the system controller can
 * send something else on the same physical channel before our ACK gets back.
 *
 * Registers and protocol constants cross-checked against:
 * ap/middleware/soc/bk7258_ap/soc/mbox0_struct.h (register layout)
 * ap/middleware/soc/common/hal/mbox0_hal.c:69-110 (send/recv sequence)
 * ap/middleware/driver/mailbox/mbox0_drv.c:37,95 (channel ownership)
 * ap/middleware/driver/mailbox/mbox0_adapter.c:73-129 (message = addr+len)
 * ap/middleware/driver/mailbox/mailbox_channel.c:194 (physical cmd block)
 */

#include <soc.h>

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc_bk7258_ipc, CONFIG_SOC_BK7258_LOG_LEVEL);

/*
 * Command block: mb_chnl_cmd_t = { hdr, param1, param2, param3 }, 16 bytes.
 *
 * It has to live in shared memory: the mailbox FIFO carries the *address* of
 * this block rather than the data (mbox0_adapter.c:119-120) and the system controller reads it
 * from there. A stack variable would be clobbered as soon as the function
 * returns.
 */
static uint32_t mb_cmd_buf[4];
static uint8_t mb_tx_seq;

/*
 * One ACK-gated slot per logical channel: an entry may only go out when the
 * previous one has been echoed back by the system controller. Skipping this made sends
 * overtake each other's acknowledgements, and the system controller's frame counts swung
 * between none and over a hundred between runs.
 */
static volatile bool mb_chn_ack_pending[256];
static volatile uint8_t mb_chn_seq[256];
static uint32_t mb_chn_since[256];

/*
 * How long the system controller takes to echo an entry back, measured in the interrupt
 * that receives the echo. Thread-side timings cannot answer this: a thread can
 * be descheduled in the middle of a call for as long as the scheduler likes, so
 * a long per-call time says nothing about the path. The interrupt runs to
 * completion, so the number here is the path's own.
 */
static uint32_t mb_chn_posted_cyc[256];
static volatile uint32_t mb_echo_n;
static volatile uint32_t mb_echo_cyc_total;
static volatile uint32_t mb_echo_cyc_max;

/*
 * Heartbeat payload.
 *
 * This is the second, and the subtlest, trap found on hardware on:
 * an outbound IPC command block is not a bare mb_chnl_cmd_t, it is wrapped in
 * an IPC header:
 *
 * typedef union {
 * struct {
 * mb_chnl_hdr_t chnl_hdr; // word0: cmd[7:0]
 * void *cmd_buff; // word1: payload pointer
 * u16 cmd_data_len; // word2 low 16 bits: length
 * };
 * mb_chnl_cmd_t mb_cmd; // {hdr, param1, param2, param3}
 * } ipc_cmd_t; // cp/mb_ipc_cmd.c:49-59
 *
 * so param1 is cmd_buff (a pointer) and param2 is cmd_data_len (a length).
 *
 * the system controller handles it like this (cp/mb_ipc_cmd.c:663-674):
 * case IPC_CPU1_HEART_BEAT_INDICATION:
 * if (chnl_cb->cmd_len >= sizeof(u32)) // cmd_len = cmd_data_len
 * mb_ipc_heartbeat_notify(...);
 *
 * With cmd_data_len = 0 the heartbeat is silently dropped: the system controller neither reports
 * an error nor refreshes its heartbeat timestamp, and 6 s later BK_ASSERT
 * resets the whole chip. The symptom is "POWER_UP was accepted (no IPC retry
 * warning) but we never survive 6 seconds", which is very easy to misread as
 * "the system controller does not recognise our core".
 *
 * The POWER_UP case does not look at cmd_len ("no params"), so it goes through
 * even with a zero payload, which is exactly why it hid the cmd_len problem
 * for so long.
 *
 * the system controller memcpy's these 4 bytes away, so they must be stable shared memory and not
 * a stack variable.
 */
static uint32_t mb_hb_param;

/* Make sure channel N is enabled. The system controller configures every channel at boot; this
 * only acts as a fallback when it did not.
 */
static void mb_chn_ensure_enabled(uint32_t chn)
{
	uint32_t en = sys_read32(SOC_MBOX0_CHN_ADDR(chn, SOC_MBOX0_CHN_ENABLE));

	if ((en & BIT(0)) != 0U) {
		/* the system controller already configured it: leave it alone rather than disturb a
		 * FIFO that may be in use.
		 */
		return;
	}

	/*
	 * Fallback configuration: fifo_start / fifo_length, and the start
	 * address has to be written before the enable bit. Values copied from
	 * ap/middleware/driver/bk7258_ap/mbox0_fifo_cfg.h:
	 * chn0: start=0 len=2, chn1: start=2 len=4, chn2: start=6 len=2
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

/* Receive callback (NULL means drain and discard) and statistics */
/*
 * Whoever wants to see arriving messages registers here.
 *
 * It is a list rather than a single slot because more than one thing needs it:
 * the flash proxy takes the flash channel's notices and a wireless driver needs
 * its own channel, and neither may silently displace the other. Every
 * subscriber sees every message and filters by logical channel itself.
 */
#define MB_RX_SUBS_MAX 4U
static soc_bk7258_mbox_rx_cb_t mb_rx_subs[MB_RX_SUBS_MAX];
static uint32_t mb_rx_count;
static uint32_t mb_ack_count;
static uint32_t mb_flash_notify_count;
static uint32_t mb_txack_count; /* ACKs we have sent back to the system controller */
static uint32_t mb_ack_tx_fail;
static uint32_t mb_ipc_rsp_sent;
static uint32_t mb_ipc_rsp_buf[4];

/* Dedicated command block for ACKs. It cannot be mb_cmd_buf: the heartbeat
 * overwrites that every 2 s and we do not control when the system controller reads these
 * 16 bytes.
 */
static uint32_t mb_ack_buf[4];

void soc_bk7258_mbox_set_rx_cb(soc_bk7258_mbox_rx_cb_t cb)
{
	if (cb == NULL) {
		return;
	}

	for (uint8_t i = 0U; i < MB_RX_SUBS_MAX; i++) {
		if (mb_rx_subs[i] == cb) {
			return; /* already listening */
		}
	}

	for (uint8_t i = 0U; i < MB_RX_SUBS_MAX; i++) {
		if (mb_rx_subs[i] == NULL) {
			mb_rx_subs[i] = cb;
			return;
		}
	}
}

uint32_t soc_bk7258_mbox_txack_count(void)
{
	return mb_txack_count;
}

void soc_bk7258_mbox_stats(uint32_t *rx, uint32_t *ack, uint32_t *flash_notify)
{
	if (rx != NULL) {
		*rx = mb_rx_count;
	}
	if (ack != NULL) {
		*ack = mb_ack_count;
	}
	if (flash_notify != NULL) {
		*flash_notify = mb_flash_notify_count;
	}
}

/*
 * Drain our own receive FIFO.
 *
 * The FIFO only holds two things: an address (rdata0) and a length (rdata1).
 * The real 16-byte command block sits in shared memory at that address and has
 * to be read from there (mbox0_adapter.c:73-129).
 *
 * Order matters: read mail_sid first, because reading it is what starts the
 * receive and pops the entry, then take rdata0/rdata1. Reversing the order
 * reads leftovers from the previous entry.
 */
/*
 * Push "address + length" into the system controller FIFO.
 *
 * The mailbox never carries the data itself, only the address of the command
 * block (mbox0_adapter.c:73-129). Writing MAIL_TID is what actually triggers
 * the transfer, so the data has to be visible in memory first (DMB).
 */
static int mb_tx_addr(uint32_t addr, uint32_t len)
{
	/* Drop this one when the system controller FIFO is full */
	if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CPU, SOC_MBOX0_CHN_FIFO_STAT)) &
	     SOC_MBOX0_FIFO_STAT_FULL_BIT) != 0U) {
		return -EBUSY;
	}

	/* The data has to be visible to the system controller before the doorbell */
	barrier_dmem_fence_full();

	sys_write32(addr, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_TDATA0));
	sys_write32(len, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_TDATA1));
	sys_write32(SOC_MB_PEER_CPU, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_MAIL_TID));

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
 * mailbox_tx_box[dst] = (mailbox_tx_box[dst] + 1) % TX_BOX_NUM;
 * memcpy(&mailbox_buff[dst][box], data, sizeof(mailbox_data_t));
 * message.data[0] = (u32)&mailbox_buff[dst][box];
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
	 * cmd / tx_seq / logical_chnl stay verbatim: the system controller's physical layer
	 * matches on tx_seq + logical_chnl, its router layer on cmd + param1.
	 */
	b[0] = (hdr & 0xFFFFF0FFU) | (SOC_MB_CTRL_ACK_BOX << 12);
	b[1] = sys_read32(addr + 4U);
	b[2] = sys_read32(addr + 8U);
	b[3] = sys_read32(addr + 12U);

	/* Count only what was actually sent: a full system-controller FIFO drops the ACK
	 * silently, and that drop is the most common cause of the system controller timing
	 * out waiting for an ACK, so the counter has to show it.
	 */
	if (mb_tx_addr((uint32_t)b, SOC_MB_IPC_MSG_LEN) == 0) {
		mb_txack_count++;
	} else {
		mb_ack_tx_fail++;
	}
}

/*
 * Console input forwarded by the system controller.
 *
 * This core has no console of its own, so input enters it through the system controller's
 * CLI: a line typed there as `ap_cmd <text>` is forwarded to this core on the
 * inbound LOG channel (0x4E) as MB_CMD_USER_INPUT, carrying the shell
 * protocol's user_cmd_t { hdr; u8 *buf; u16 len; } -- word1 is a pointer into
 * the system controller's memory, word2's low 16 bits is the length. Source:
 * cp/components/bk_cli/cli_main.c:1051 cli_cpu1_command()
 * cp/components/bk_cli/shell_task.c:2411 shell_cmd_forward()
 * ap/components/bk_cli/shell_mailbox_cp1.c:145 the vendor receiver
 *
 * The bytes are copied in the interrupt, not in the pump: the automatic ACK for
 * this message goes out from the same interrupt pass, and the system controller answers
 * ACK_STATE_COMPLETE for MB_CMD_USER_INPUT (shell_mailbox_cp1.c:161), i.e. "the
 * buffer is yours to reuse" -- copying afterwards would race the system controller reusing
 * it.
 */
#define MB_CON_RING_LEN 256U

static uint8_t mb_con_ring[MB_CON_RING_LEN];
static volatile uint16_t mb_con_in;
static volatile uint16_t mb_con_out;
static volatile uint32_t mb_con_rx_msgs;
static volatile uint32_t mb_con_dropped;
static volatile uint32_t mb_con_bad_ptr;

/*
 * Is an address supplied by the system controller safe to read?
 *
 * The system controller hands us raw pointers, and a zero or a garbled one would take this
 * core straight into a HardFault, so anything outside the shared SRAM window is
 * refused. Unlike the command block, the payload has no alignment requirement:
 * the system controller hands us `&inbuf[fwd_pos + 7]`, an arbitrary byte inside its own line
 * buffer, so only the range is checked here.
 */
static bool mb_sc_range_ok(uint32_t addr, uint32_t len)
{
	if ((len == 0U) || (addr < SOC_SRAM0_DATA_BASE)) {
		return false;
	}

	if (addr >= (SOC_SRAM0_DATA_BASE + 0xA0000U)) {
		return false;
	}

	return len <= ((SOC_SRAM0_DATA_BASE + 0xA0000U) - addr);
}

static void mb_con_take(uint32_t addr, uint32_t w2)
{
	uint32_t len = w2 & 0xFFFFU;
	const uint8_t *p = (const uint8_t *)addr;
	uint16_t free_bytes;
	uint32_t i;

	mb_con_rx_msgs++;

	if (!mb_sc_range_ok(addr, len) || (len > (MB_CON_RING_LEN - 1U))) {
		mb_con_bad_ptr++;
		return;
	}

	/*
	 * All or nothing: a partially queued command line would run as a
	 * mangled command, which is worse to debug than a line that visibly did
	 * not arrive.
	 */
	free_bytes = (uint16_t)((mb_con_out + MB_CON_RING_LEN - mb_con_in - 1U) % MB_CON_RING_LEN);

	if (free_bytes < len) {
		mb_con_dropped++;
		return;
	}

	for (i = 0U; i < len; i++) {
		mb_con_ring[mb_con_in] = p[i];
		mb_con_in = (uint16_t)((mb_con_in + 1U) % MB_CON_RING_LEN);
	}
}

int soc_bk7258_mbox_console_getc(void)
{
	unsigned int key = irq_lock();
	int c = -1;

	if (mb_con_out != mb_con_in) {
		c = (int)mb_con_ring[mb_con_out];
		mb_con_out = (uint16_t)((mb_con_out + 1U) % MB_CON_RING_LEN);
	}

	irq_unlock(key);

	return c;
}

void soc_bk7258_mbox_console_stats(uint32_t *msgs, uint32_t *dropped, uint32_t *bad_ptr)
{
	if (msgs != NULL) {
		*msgs = mb_con_rx_msgs;
	}
	if (dropped != NULL) {
		*dropped = mb_con_dropped;
	}
	if (bad_ptr != NULL) {
		*bad_ptr = mb_con_bad_ptr;
	}
}

/*
 * Receive ring.
 *
 * The ISR below has to answer the system controller within
 * microseconds: it measures the ACK against the tx_seq of whatever it sent
 * last, so the earlier the reply goes out the better. Printing and the user
 * callback therefore stay in thread context; the ISR only stashes what it
 * received.
 */
#define MB_RX_RING_NUM 16U

struct mb_rx_item {
	uint32_t w0;
	uint32_t w1;
	uint32_t w2;
	uint32_t w3;
	uint32_t nbytes;
};

static struct mb_rx_item mb_rx_ring[MB_RX_RING_NUM];
static volatile uint8_t mb_rx_in;
static volatile uint8_t mb_rx_out;
static volatile uint32_t mb_isr_count;
static volatile uint32_t mb_batch_last;
static volatile uint32_t mb_batch_max;
static volatile uint32_t mb_batch_multi;
static volatile uint32_t mb_saradc_skipped;

/*
 * Drain the FIFO and answer. Safe to call from the ISR: no printing, no
 * callback, no allocation.
 *
 * The FIFO only holds an address (rdata0) and a length (rdata1); the real
 * 16-byte command block sits in shared memory at that address and has to be
 * read from there (mbox0_adapter.c:73-129).
 *
 * Order matters: read mail_sid first, because reading it is what starts the
 * receive and pops the entry, then take rdata0/rdata1. Reversing the order
 * reads leftovers from the previous entry.
 */
static void mb_rx_fast(void)
{
	/* FIFO is 4 deep; 8 guards against a spin. */
	struct mb_rx_item batch[8];
	uint32_t addr[8], len[8];
	uint32_t n = 0U;
	uint32_t guard = 8U;

	while ((guard-- != 0U) && (n < ARRAY_SIZE(batch))) {
		if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_FIFO_STAT)) &
		     SOC_MBOX0_FIFO_STAT_EMPT_BIT) != 0U) {
			break;
		}

		/*
		 * Order matters: reading mail_sid is what starts the receive and
		 * pops the entry, so it has to come before rdata0/rdata1.
		 * Reversing the order reads leftovers from the previous entry.
		 */
		(void)sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_MAIL_SID));
		addr[n] = sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_RDATA0));
		len[n] = sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_RDATA1));
		batch[n].w0 = 0U;
		batch[n].w1 = 0U;
		batch[n].w2 = 0U;
		batch[n].w3 = 0U;
		batch[n].nbytes = len[n];

		/*
		 * The address comes from the system controller, so do not dereference it
		 * blindly. Only pointers inside SRAM (0x28000000..0x280A0000) are
		 * accepted: a zero or a garbled pointer would throw this core
		 * straight into a HardFault.
		 */
		bool valid = (addr[n] >= SOC_SRAM0_DATA_BASE) &&
			     (addr[n] < (SOC_SRAM0_DATA_BASE + 0xA0000U)) &&
			     ((addr[n] & 0x3U) == 0U);

		if (valid) {
			batch[n].w0 = sys_read32(addr[n] + 0U);
			batch[n].w1 = sys_read32(addr[n] + 4U);
			batch[n].w2 = sys_read32(addr[n] + 8U);
			batch[n].w3 = sys_read32(addr[n] + 12U);
		}

		/*
		 * Console input forwarded by the system controller. See mb_con_ring for why this
		 * is copied here rather than in the pump.
		 */
		if (valid && ((batch[n].w0 >> 24) == SOC_MB_CHNL_CP_LOG) &&
		    ((batch[n].w0 & 0xFFU) == SOC_MB_CMD_USER_INPUT) &&
		    (SOC_MB_HDR_IS_ACK(batch[n].w0) == 0U)) {
			mb_con_take(batch[n].w1, batch[n].w2);
		}

		mb_rx_count++;
		n++;
	}

	if (n == 0U) {
		return;
	}

	mb_batch_last = n;
	if (n > mb_batch_max) {
		mb_batch_max = n;
	}
	if (n > 1U) {
		mb_batch_multi++;
	}

	/*
	 * Answer each message with its own header.
	 *
	 * The system controller checks an ACK against the tx_seq and logical channel of what it
	 * sent last on the physical channel, while its router matches the ACK to
	 * a queued command by cmd and param1. Both have to hold, so the header is
	 * echoed rather than restamped: stamping with the last entry's values
	 * breaks every transaction on a different logical channel, and the vendor
	 * driver ends up doing the same thing anyway (it stamps the values of the
	 * message it has just received).
	 */
	for (uint32_t i = 0U; i < n; i++) {
		uint32_t w0 = batch[i].w0;

		if ((w0 == 0U) && (batch[i].w1 == 0U) && (batch[i].w2 == 0U) &&
		    (batch[i].w3 == 0U)) {
			continue; /* unreadable block */
		}

		if (SOC_MB_HDR_IS_ACK(w0)) {
			/*
			 * An echo of an entry this side sent. Release the slot
			 * here rather than in the reader: the reader only runs on
			 * its own tick, and a sender waiting for the slot would
			 * then be held to that tick, which puts a ceiling of a
			 * few tens of frames per second on the whole data path.
			 */
			uint8_t ch = (uint8_t)(w0 >> 24);
			uint8_t seq = (uint8_t)(w0 >> 16);

			if (mb_chn_ack_pending[ch] && (mb_chn_seq[ch] == seq)) {
				uint32_t d = k_cycle_get_32() - mb_chn_posted_cyc[ch];

				mb_chn_ack_pending[ch] = false;
				mb_echo_n++;
				mb_echo_cyc_total += d;
				if (d > mb_echo_cyc_max) {
					mb_echo_cyc_max = d;
				}
			}

			continue; /* never acknowledge an ACK */
		}

		if (SOC_MB_HDR_IS_SYNC_TX(w0)) {
			continue;
		}

		/*
		 * Every command is acknowledged, including the pushes that arrive on
		 * their own logical channels.
		 *
		 * The physical channel carries every logical channel heading for the
		 * same destination CPU, and it stays busy until the command it sent
		 * last is acknowledged. A command left unanswered therefore holds the
		 * channel until the system controller's busy watchdog gives up on it, which was
		 * long enough for the next command to be dropped: the system controller's
		 * mb_phy_chnl_start_tx() copies a command into its logical channel
		 * buffer and returns without sending when the physical channel is
		 * busy, and nothing resends it afterwards.
		 */

		/*
		 * Answer at the ipc level from here, alongside the mailbox ACK.
		 *
		 * Sending this from a thread callback instead made it arrive after the
		 * sender had given up: the system controller then reports it as unsolicited
		 * (run_state no longer has TX_IN_PROCESS) and the sender times out.
		 * The reply echoes the push: same tag, command with the response
		 * flag, same payload, only the dst/src bytes reversed for our
		 * direction.
		 */
		/*
		 * Every pushed command needs a response, whatever its user_cmd:
		 * mb_ipc_send() always sends hdr.cmd MB_IPC_SEND_CMD and always
		 * waits for the system controller's response before it releases the caller.
		 *
		 * A message that already carries the response flag is the server
		 * answering a request of ours, sent without waiting, and
		 * answering that lands outside the sender's window.
		 */
		if (((w0 >> 24) == SOC_MB_CHNL_CP_IPC) && ((w0 & 0xFFU) == 0x02U)) {
			mb_ipc_rsp_buf[0] = ((w0 & 0xFFU) | 0x80U) | ((mb_tx_seq++ & 0xFFU) << 16) |
					    (SOC_MB_CHNL_AP_IPC << 24);
			mb_ipc_rsp_buf[1] = ((batch[i].w1 & 0x0000FF00U) >> 8) |
					    ((batch[i].w1 & 0x000000FFU) << 8) |
					    (batch[i].w1 & 0x00FF0000U);
			mb_ipc_rsp_buf[2] = batch[i].w2;
			mb_ipc_rsp_buf[3] = batch[i].w3;

			if (mb_tx_addr((uint32_t)&mb_ipc_rsp_buf[0], SOC_MB_IPC_MSG_LEN) == 0) {
				mb_ipc_rsp_sent++;
			} else {
				mb_ack_tx_fail++;
			}
		}

		if ((w0 >> 24) == SOC_MB_CHNL_CP_FLASH) {
			/*
			 * Flash operation notice: this needs an
			 * *application-level* ACK with w1 set to
			 * IPC_FLASH_OP_ACK (2) (flash_notify.c:65-100).
			 */
			mb_flash_notify_count++;
			(void)soc_bk7258_mbox_ack(addr[i], w0, SOC_MB_FLASH_OP_ACK, batch[i].w2,
						  SOC_MB_ACK_STATE_COMPLETE);
		} else {
			mb_auto_ack(addr[i], w0);
		}
	}

	/* Hand them to thread context, dropping the oldest if the ring is full. */
	for (uint32_t i = 0U; i < n; i++) {
		uint8_t next = (uint8_t)((mb_rx_in + 1U) % MB_RX_RING_NUM);

		if (next == mb_rx_out) {
			mb_rx_out = (uint8_t)((mb_rx_out + 1U) % MB_RX_RING_NUM);
		}

		mb_rx_ring[mb_rx_in] = batch[i];
		mb_rx_in = next;
	}
}

static void mb_mbox_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Channel 1 owns bit 1 of the mailbox interrupt status. */
	if ((sys_read32(SOC_MBOX0_REG_ADDR(0x03U)) & BIT(1)) == 0U) {
		return;
	}

	mb_isr_count++;
	mb_rx_fast();
}

void soc_bk7258_mbox_echo_stats(uint32_t *n, uint32_t *avg_us, uint32_t *max_us)
{
	uint32_t cycles_per_us = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000U;
	uint32_t count = mb_echo_n;

	*n = count;
	*avg_us = count ? (mb_echo_cyc_total / count) / cycles_per_us : 0U;
	*max_us = mb_echo_cyc_max / cycles_per_us;
}

uint32_t soc_bk7258_mbox_irq_count(void)
{
	return mb_isr_count;
}

uint32_t soc_bk7258_mbox_batch_max(void)
{
	return mb_batch_max;
}

uint32_t soc_bk7258_mbox_batch_multi(void)
{
	return mb_batch_multi;
}

uint32_t soc_bk7258_mbox_saradc_skipped(void)
{
	return mb_saradc_skipped;
}

uint32_t soc_bk7258_mbox_ack_tx_fail(void)
{
	return mb_ack_tx_fail;
}

uint32_t soc_bk7258_mbox_ipc_rsp_sent(void)
{
	return mb_ipc_rsp_sent;
}

static void mb_pump_once(void)
{
	unsigned int key;

	/* Works even when the interrupt is not wired up yet. */
	key = irq_lock();
	mb_rx_fast();
	irq_unlock(key);

	while (mb_rx_out != mb_rx_in) {
		struct mb_rx_item it = mb_rx_ring[mb_rx_out];

		mb_rx_out = (uint8_t)((mb_rx_out + 1U) % MB_RX_RING_NUM);

		/* An echo of what we sent: the ack bit of the header set, our
		 * tx_seq and our channel unchanged. It releases the slot so the
		 * next entry may go out. The bit is 12 (SOC_MB_HDR_IS_ACK), not
		 * 7: testing the wrong one leaves the slot held until the
		 * watchdog, and everything after that is turned away as busy.
		 */
		if (SOC_MB_HDR_IS_ACK(it.w0) != 0U) {
			uint8_t ch = (uint8_t)(it.w0 >> 24);
			uint8_t seq = (uint8_t)(it.w0 >> 16);

			if (mb_chn_ack_pending[ch] && mb_chn_seq[ch] == seq) {
				mb_chn_ack_pending[ch] = false;
			}
			continue;
		}

		/* A lost ack must cost one entry, not the channel: anything left
		 * pending far past the system controller's 200 ms watchdog is let go here.
		 */
		for (uint32_t ch = 0U; ch < 256U; ch++) {
			if (mb_chn_ack_pending[ch] &&
			    (k_uptime_get_32() - mb_chn_since[ch]) > 400U) {
				mb_chn_ack_pending[ch] = false;
			}
		}

		for (uint8_t i = 0U; i < MB_RX_SUBS_MAX; i++) {
			if (mb_rx_subs[i] != NULL) {
				mb_rx_subs[i](it.w0, it.w1, it.w2, it.w3, it.nbytes);
			}
		}
	}
}

/*
 * Called from three places: the driver thread, main(), and the heartbeat timer,
 * which runs in interrupt context. Two passes at once would take the same ring
 * entry twice and leave the other's read index behind, and an entry processed
 * twice or lost that way costs more than the entry: a lost echo holds a channel
 * busy until the watchdog, and enough of those and the system controller's own heartbeat
 * check fires. One pass runs at a time; a pass not taken now is taken by the
 * caller that is already running.
 */
static atomic_t mb_pump_active;

/*
 * The mailbox is read by pumping it, and somebody has to do that: the Wi-Fi
 * driver has a thread of its own for it, but an image that only uses the
 * Bluetooth channel has nothing -- and a caller waiting inside the host's own
 * bring-up cannot pump on its own, which is exactly how a HCI command times
 * out with the answer sitting in the ring. This gives the IPC layer a reader
 * of its own for the images that would otherwise have none; calling it twice
 * is harmless, the second call finds the thread already running.
 */
static struct k_thread mb_reader_thread;
static K_THREAD_STACK_DEFINE(mb_reader_stack, 1024);
static bool mb_reader_started;

static void mb_reader(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		uint32_t before;
		uint32_t after;
		uint32_t junk;

		soc_bk7258_mbox_stats(&before, &junk, &junk);
		soc_bk7258_mbox_pump();
		soc_bk7258_mbox_stats(&after, &junk, &junk);

		if (after == before) {
			k_sleep(K_MSEC(1));
		} else {
			k_yield();
		}
	}
}

void soc_bk7258_mbox_reader_start(void)
{
	if (mb_reader_started) {
		return;
	}

	mb_reader_started = true;

	k_thread_create(&mb_reader_thread, mb_reader_stack, K_THREAD_STACK_SIZEOF(mb_reader_stack),
			mb_reader, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_thread_name_set(&mb_reader_thread, "mbox_reader");
}

void soc_bk7258_mbox_pump(void)
{
	if (!atomic_cas(&mb_pump_active, 0, 1)) {
		return;
	}

	mb_pump_once();

	(void)atomic_set(&mb_pump_active, 0);
}

int soc_bk7258_mbox_post(const uint32_t *cmd, uint32_t len)
{
	if (cmd == NULL) {
		return -EINVAL;
	}

	return mb_tx_addr((uint32_t)cmd, len);
}

/*
 * Console output, this core -> the system controller.
 *
 * This is the only way this core can print anything: UART0 belongs to the
 * system controller, so
 * the string travels as a LOG message and the system controller prints it on its console verbatim.
 *
 * Two details are load bearing:
 *
 * 1. The slot holding the command block and the payload has to be static shared
 * memory. The mailbox carries the *address* of the block, and we do not
 * control when the system controller reads it, so a stack buffer would be gone by then.
 *
 * 2. The slots rotate rather than being checked for a free one. With a single
 * slot, the next line overwrites memory the system controller may not have read yet and it ends
 * up printing a mixture of two lines. The vendor rotates the same way
 * (bk_mailbox_send: mailbox_tx_box[dst] = (... + 1) % TX_BOX_NUM), and the system controller
 * consumes a message in microseconds, so four slots are plenty.
 */
#define MB_LOG_SLOTS 4U

struct mb_log_slot {
	uint32_t cmd[4];
	char buf[SOC_MB_LOG_LINE_LEN];
};

static struct mb_log_slot mb_log_slots[MB_LOG_SLOTS];
static uint8_t mb_log_next;

int soc_bk7258_mbox_log_write(const char *str, uint32_t len)
{
	struct mb_log_slot *s;

	if ((str == NULL) || (len == 0U)) {
		return 0;
	}

	if (len > SOC_MB_LOG_LINE_LEN) {
		len = SOC_MB_LOG_LINE_LEN;
	}

	s = &mb_log_slots[mb_log_next];
	mb_log_next = (uint8_t)((mb_log_next + 1U) % MB_LOG_SLOTS);

	memcpy(s->buf, str, len);

	/*
	 * The command number is MB_CMD_LOG_OUT and word2 low 16 bits is the
	 * length, matching log_cmd_t { hdr; u8 *buf; u16 len; u16 tag } on the system controller's
	 * side (ap/components/bk_cli/shell_drv.h:99-106).
	 *
	 * tx_seq comes from the one counter every outbound send shares. The system controller's
	 * physical layer claims an ACK by tx_seq plus logical channel, and one
	 * physical channel carries every logical channel aimed at the same CPU,
	 * so a second, independent counter here would let the two interleave and
	 * be discarded as stale.
	 */
	s->cmd[0] = SOC_MB_HDR_CH(SOC_MB_CHNL_LOG, SOC_MB_CMD_LOG_OUT, mb_tx_seq++);
	s->cmd[1] = (uint32_t)s->buf; /* == cmd_buff: payload pointer */
	s->cmd[2] = len;              /* == cmd_data_len: length */
	s->cmd[3] = 0U;

	return soc_bk7258_mbox_post(s->cmd, SOC_MB_IPC_MSG_LEN);
}

int soc_bk7258_mbox_ack(uint32_t addr, uint32_t hdr, uint32_t w1, uint32_t w2, uint32_t w3)
{
	/*
	 * An ACK is the received command block sent straight back with three
	 * changes:
	 * (1) clear state[11:8] (leaving it set reads as CHNL_STATE_COM_FAIL)
	 * (2) set CHNL_CTRL_ACK_BOX in ctrl[15:12] (bit 0 -> header bit 12)
	 * (3) fill ack_data1 / ack_state, i.e. word1 / word3
	 * cmd[7:0], tx_seq[23:16] and logical_chnl[31:24] must be preserved
	 * verbatim: the system controller's physical layer claims the ACK by tx_seq plus
	 * logical_chnl, and its router layer then by cmd plus param1[23:0].
	 *
	 * Why the ACK does not use "the other" physical box: mb_chnl_write does
	 * distinguish BOX0 (command) from BOX1 (ACK), but the code in
	 * bk_mailbox_send() that picks a buffer by box is commented out
	 * (mbox0_adapter.c:88-98). In practice both go through the same FIFO and
	 * are told apart only by that one bit in hdr.ctrl.
	 */
	uint32_t h = (hdr & 0xFFFFF0FFU) | (SOC_MB_CTRL_ACK_BOX << 12);

	if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CPU, SOC_MBOX0_CHN_FIFO_STAT)) &
	     SOC_MBOX0_FIFO_STAT_FULL_BIT) != 0U) {
		return -EBUSY;
	}

	mb_ack_buf[0] = h;
	mb_ack_buf[1] = w1;
	mb_ack_buf[2] = w2;
	mb_ack_buf[3] = w3;

	mb_ack_count++;

	return mb_tx_addr((uint32_t)&mb_ack_buf[0], SOC_MB_IPC_MSG_LEN);
}

int soc_bk7258_mbox_send(uint32_t chnl, uint32_t cmd, uint32_t p1, uint32_t p2)
{
	return soc_bk7258_mbox_post_cmd(chnl, cmd, p1, p2, 0U);
}

int soc_bk7258_mbox_post_cmd(uint32_t chnl, uint32_t cmd, uint32_t p1, uint32_t p2, uint32_t p3)
{
	/* Drop this tick when the destination FIFO is full: losing one heartbeat
	 * is harmless, the next one follows 2 s later.
	 */
	if ((sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CPU, SOC_MBOX0_CHN_FIFO_STAT)) &
	     SOC_MBOX0_FIFO_STAT_FULL_BIT) != 0U) {
		return -EBUSY;
	}

	/* One entry at a time per channel, so acknowledgements cannot overtake
	 * each other. Measured cost of the rule on the frame channel: the echo
	 * comes back in 165 us on average, so the channel can carry thousands of
	 * frames a second and the rule is not what holds the data path back.
	 * Letting several entries out at once changed nothing.
	 */
	if (mb_chn_ack_pending[chnl & 0xFFU]) {
		return -EBUSY; /* the previous entry on this channel is not acked yet */
	}

	mb_tx_seq++;
	mb_chn_ack_pending[chnl & 0xFFU] = true;
	mb_chn_seq[chnl & 0xFFU] = (uint8_t)mb_tx_seq;
	mb_chn_since[chnl & 0xFFU] = k_uptime_get_32();
	mb_chn_posted_cyc[chnl & 0xFFU] = k_cycle_get_32();
	mb_cmd_buf[0] = SOC_MB_HDR_CH(chnl, cmd, mb_tx_seq);
	mb_cmd_buf[1] = p1; /* == ipc_cmd.cmd_buff (payload pointer) */
	mb_cmd_buf[2] = p2; /* == ipc_cmd.cmd_data_len (length, low 16) */
	mb_cmd_buf[3] = p3;

	/*
	 * The data has to be visible to the system controller before the doorbell. A DMB is
	 * enough with no D-Cache; if a D-Cache is ever enabled, or the command
	 * block moves to a cacheable region, this has to become a cache clean.
	 */
	barrier_dmem_fence_full();

	sys_write32((uint32_t)&mb_cmd_buf[0],
		    SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_TDATA0));
	sys_write32(SOC_MB_IPC_MSG_LEN, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_TDATA1));
	/* Writing mail_tid = destination CPU is what actually triggers the send */
	sys_write32(SOC_MB_PEER_CPU, SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_MAIL_TID));

	return 0;
}

void soc_bk7258_ipc_power_up(void)
{
	int rc;

	mb_chn_ensure_enabled(SOC_MB_SELF_CPU);
	soc_bk7258_mbox_pump();
	/* POWER_UP: the vendor ipc_send_cmd(..., NULL, 0, ...), no payload */
	rc = soc_bk7258_mbox_send(SOC_MB_CHNL_HW_CTRL, SOC_MB_IPC_CMD_POWER_UP, 0U, 0U);

	printk("[ipc] power_up rc=%d chn1_en=%08x chn1_st=%08x chn0_st=%08x\n", rc,
	       sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_ENABLE)),
	       sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_FIFO_STAT)),
	       sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CPU, SOC_MBOX0_CHN_FIFO_STAT)));
}

int soc_bk7258_ipc_heartbeat(void)
{
	soc_bk7258_mbox_pump();

	/*
	 * HEART_BEAT: the vendor ipc_send_heart_beat(param) is
	 * ipc_send_cmd(..., (u8 *)&param, sizeof(param), ...), that is
	 * cmd_buff = &param and cmd_data_len = 4.
	 *
	 * cmd_data_len must be >= 4 or the system controller drops it outright (see the comment on
	 * mb_hb_param).
	 */
	mb_hb_param = 0U;
	return soc_bk7258_mbox_send(SOC_MB_CHNL_HW_CTRL, SOC_MB_IPC_CMD_HEARTBEAT,
				    (uint32_t)&mb_hb_param, sizeof(mb_hb_param));
}

/* Diagnostic counters for the heartbeat */
static uint32_t mb_hb_count;
static uint32_t mb_hb_busy;

/*
 * Heartbeat tick
 *
 * A k_timer rather than a thread: its callback runs in interrupt context, so a
 * wedged application thread does not stop it. The flip side is intended by the
 * vendor design: if the application really hangs, the heartbeat stops, the system controller
 * resets the chip, and the mechanism acts as a watchdog.
 */

static struct k_timer mb_hb_timer;

static void mb_hb_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	int rc = soc_bk7258_ipc_heartbeat();

	mb_hb_count++;
	if (rc != 0) {
		mb_hb_busy++;
	}

	/*
	 * Diagnostic: print whether the heartbeat actually went out.
	 * Reading the output:
	 * [hb] lines at all -> the timer runs and the send path is
	 * being executed
	 * [hb] with rc=-16 -> the FIFO stays full on the system controller
	 * side (it drains it)
	 * no [hb] at all -> the k_timer never started
	 * [hb] rc=0 but the system controller still reports a heartbeat timeout
	 * -> the command was not accepted by the system controller
	 *
	 * One line when the timer first fires, and one whenever the heartbeat
	 * does not go out. The first says the mechanism is alive; the second is
	 * the only case worth watching, since a heartbeat that stops reaching
	 * the system controller costs it six seconds later: its own watchdog
	 * resets the chip. The quiet path stays silent so the console is free
	 * for the frame counters.
	 */
	if ((rc != 0) || (mb_hb_count == 1U)) {
		printk("[hb] #%u rc=%d busy=%u chn0_st=%08x chn1_st=%08x\n", mb_hb_count, rc,
		       mb_hb_busy,
		       sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_PEER_CPU, SOC_MBOX0_CHN_FIFO_STAT)),
		       sys_read32(SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_FIFO_STAT)));
	}
}

static int soc_bk7258_ipc_init(void)
{
	/*
	 * The power-up notification is issued separately at PRE_KERNEL_1 (see
	 * soc.c), much earlier than this point. The system controller waits 2 s, but the earlier
	 * this happens the better, so that no later step can push it past that
	 * window.
	 */
	/*
	 * Enable the mailbox receive interrupt and let it do the answering.
	 *
	 * Bit 8 of the channel's own register at group offset 0 is the interrupt
	 * enable (mbox0_ll_set_reg_0x20_int_en); the low bits carry the FIFO
	 * start and must be preserved. The status register is not cleared here:
	 * draining the FIFO is what lowers it (mbox0_drv_isr_handler does the
	 * same).
	 */
	uint32_t cfg_addr = SOC_MBOX0_CHN_ADDR(SOC_MB_SELF_CPU, SOC_MBOX0_CHN_FIFO_CFG);

	sys_write32(sys_read32(cfg_addr) | BIT(8), cfg_addr);

	/*
	 * Second-level enable. Apart from the NVIC there is one enable bit per
	 * IRQ in the SYSTEM domain, and an interrupt is only reported to the
	 * CPU when that bit is set as well: IRQ N maps to bit N below 32 and to
	 * bit N - 32 at or above it, so the mailbox (IRQ 63) is bit 31. The
	 * vendor driver does the same, via
	 * sys_drv_int_group2_enable(1 << (INT_SRC_MAILBOX - 32)). The SYSTEM
	 * domain is shared, so this has to be read-modify-write.
	 */
	sys_write32(sys_read32(SOC_SYS_CPU1_INT_32_63_EN_ADDR) | BIT(SOC_IRQ_MAILBOX - 32),
		    SOC_SYS_CPU1_INT_32_63_EN_ADDR);

	IRQ_CONNECT(SOC_IRQ_MAILBOX, 0, mb_mbox_isr, NULL, 0);
	irq_enable(SOC_IRQ_MAILBOX);

	k_timer_init(&mb_hb_timer, mb_hb_handler, NULL);
	/*
	 * Vendor MB_IPC_HEARTBEAT_TIME is 2000 ms; the system controller
	 * tolerates 3 missed ticks.
	 */
	k_timer_start(&mb_hb_timer, K_MSEC(2000), K_MSEC(2000));

	LOG_INF("mbox0 heartbeat started: every 2000 ms to CPU%d (self chn=%d)", SOC_MB_PEER_CPU,
		SOC_MB_SELF_CPU);

	return 0;
}

SYS_INIT(soc_bk7258_ipc_init, POST_KERNEL, 0);
