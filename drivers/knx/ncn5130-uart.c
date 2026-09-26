/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * NCN5130 KNX TP1 Transceiver — Zephyr UART driver
 *
 * Implements the Physical Layer (L1) primitives consumed by the KNX L2 subsystem:
 *   Ph_Data__req / Ph_Data__con / Ph_Data__ind
 *   Ph_Reset__req / Ph_Reset__con
 *   Ph_Bus_Free / U_SetAddress__req
 *   platform_timer_eof_{init,start,stop}
 *
 * The driver is instantiated from the device tree:
 *   ncn5130 { compatible = "onsemi,ncn5130"; uart = <&usart1>; };
 *
 * Host link: 9-BIT UART, 38400 bps, async DMA.
 *
 * The board straps the transceiver for 9-bit mode (TREQ/MODE1/MODE2/SCK-UC2 to
 * GND, CSB-UC1 to VDD — datasheet Table 9, p.27).  The 9th bit is nominally
 * even parity over the 8 data bits, but the datasheet (p.27) is explicit that
 * it is more than a check:
 *
 *   "When NCN5130 detects an acceptance-window error or a pulse-duration
 *    error on the KNX bus, the parity bit is also encoded to indicate an
 *    error in that byte."
 *   "For internal register read and write services the parity bit is
 *    meaningless and should be ignored."
 *
 * So the transceiver deliberately violates even parity to flag a bus error.
 * The USART must therefore be configured as 9 data bits with hardware parity
 * DISABLED, and parity computed/checked in software — otherwise every flagged
 * octet raises a hardware parity error, Zephyr reports UART_RX_STOPPED and
 * disables reception, and the KNX interface dies until the next reboot.
 * That is exactly what the previous 8-data-bits + PARITY_EVEN configuration
 * did (it put the same 9 bits on the wire, so the link appeared to work).
 *
 * Consequence: buffers are uint16_t and the transfers use the uart_*_u16()
 * async variants (CONFIG_UART_WIDE_DATA).  Note the asymmetry in the Zephyr
 * API — uart_tx_u16()/uart_rx_enable_u16() take lengths in WORDS, while the
 * UART_RX_RDY event reports offset/len in BYTES.
 *
 * Also note: in 9-bit mode the transceiver NEVER emits U_FrameState.ind (p.41
 * and p.49 both restrict it to SPI / 8-bit UART), so this driver does not look
 * for it, and validating the FCS and the frame length is the Data Link Layer's
 * job (done in l_process_rx_frame()).
 *
 * RX: 1-word ping-pong buffers → per-word delivery to receive_word().
 *
 *     A larger ring buffer with an idle-line delivery timeout was tried
 *     to cut interrupt load, and reverted: bench-tested on a
 *     live, busy KNX bus it broke both reset bring-up and frame EOF
 *     detection. Zephyr's STM32 async UART driver, in the non-cyclic RX
 *     mode this board's DMA config uses, only reliably reports new data
 *     when the buffer fills completely (uart_stm32_dma_rx_cb(), "true since
 *     this function occurs when buffer is full") — its idle-timeout partial
 *     flush (uart_stm32_dma_rx_flush(..., STM32_ASYNC_STATUS_TIMEOUT)) fired
 *     constantly but reported zero new words almost every time (measured:
 *     3321 of 3351 UART_RX_RDY events carried 0 words). With a 128-word
 *     buffer that meant several real, independently-timed KNX frames
 *     accumulated before ever reaching receive_word(), delivered together in
 *     one tight loop — which also starves the byte-by-byte software EOF
 *     timer (s_rx_eof_timer, below), since it cannot elapse in real time
 *     while receive_word() is called back-to-back for already-buffered
 *     words. Net effect: U_Reset.ind could sit unseen for seconds, and
 *     l_process_rx_frame() had to silently trim multiple concatenated
 *     frames into one, dropping every telegram but the first. A per-word
 *     buffer sidesteps both: "buffer full" fires after every word, so
 *     delivery timing tracks the real bus again.
 * TX: all sends are serialized and blocking at the driver level; Ph_Data__con
 *     is called from Ph_Data__req after the UART TX completes (thread context).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/knx/knx_l1.h>
#include <zephyr/knx/knx_pkt.h>

LOG_MODULE_REGISTER(ncn5130, CONFIG_KNX_STACK_LOG_LEVEL);

/* -------- NCN5130 state constants -------- */

#define NCN_STATE_POWER_UP      0
#define NCN_STATE_SYNC          1
#define NCN_STATE_STOP          2
#define NCN_STATE_NORMAL        3
#define NCN_STATE_RESET         5
#define NCN_STATE_POWER_UP_STOP 4

#define BUS_BUSY           (-1)
#define BUS_FREE           0
#define BUS_RX_IN_PROGRESS 1
#define BUS_TX_IN_PROGRESS 3

#define CTRL_FRAME_MASK  0x53
#define CTRL_FRAME_VALUE 0x10

/*
 * End-of-frame silence threshold — KNX TP1 bus timing (9600 baud):
 *
 *   Inner-frame character spacing     = 13 KNX bit-times = 1354 µs
 *   Minimum inter-frame gap (tightest,
 *     high-priority/repeated frame)   = 50 KNX bit-times = 5208 µs
 *
 * The timer must fire AFTER the last inner-frame gap (> 1354 µs) but
 * BEFORE the next frame's CTRL byte could arrive (< 5208 µs).
 *
 * It must ALSO fire AFTER the NCN5130's own autonomous IACK — sent in
 * hardware, without host involvement, for every frame addressed to our
 * Individual Address once auto-ack is armed (U_SetAddress__req) — has
 * finished transmitting on the bus:
 *
 *   ACK start delay (t_as)   = 15 KNX bit-times = 1562 µs
 *   ACK character duration   = 26 KNX bit-times = 2708 µs
 *   Total frame-end -> auto-IACK done = 41 KNX bit-times = 4271 µs
 *
 * With the previous 24-bit-time (2500 µs) timeout, l_tx_thread_fn could
 * start a software TX (e.g. the T_ACK following a T_Data_Connected)
 * ~1770 µs BEFORE the NCN5130 finished transmitting its own auto-IACK.
 * Issuing U_L_DataStart.req while the transceiver was still physically
 * busy on the bus corrupted its internal state (crc/len_err,
 * rx_err/tx_err/temp_warn, garbled echo bytes), reliably burning all 6
 * TX retries and dropping the T_ACK — this is what caused ETS individual
 * address programming to loop forever (confirmed via NCN5130 datasheet
 * timing + on-target debug log correlation, not a random bus collision).
 *
 * 45 KNX bit-times = 4688 µs sits strictly between the two constraints:
 *   4271 µs (auto-IACK done) < 4688 µs < 5208 µs (earliest next frame).
 */
#define KNX_TP1_BAUD    9600U
#define EOF_BIT_TIMEOUT 45U
#define EOF_TIMER_US    (EOF_BIT_TIMEOUT * 1000000U / KNX_TP1_BAUD) /* 4688 µs */

/* -------- Driver config (UART device resolved at build time) -------- */

struct ncn5130_config {
	const struct device *uart_dev;
};

/* -------- Module-level state (singleton — one NCN5130 per board) -------- */

static const struct device *s_uart_dev;

/*
 * s_bus_state, s_ncn_state and s_tx_echo_remaining are read-modify-write
 * state shared between the UART ISR/callback and thread context (the L2 TX
 * thread via knx_l1_send_frame(), work-queue items, Ph_Data__req()).
 * "volatile" alone gives no atomicity — atomic_t does.
 */
static atomic_t s_bus_state = ATOMIC_INIT(BUS_BUSY);
static atomic_t s_ncn_state = ATOMIC_INIT(NCN_STATE_POWER_UP);
/*
 * TX echo byte counter.
 *
 * The NCN5130 echoes every transmitted KNX frame byte-for-byte back to the
 * host UART before sending L_Data.con.  Any data byte in that echo can
 * accidentally match an NCN5130 service pattern (e.g. 0x0B = L_Data.con,
 * 0x2B = U_StopMode.ind, 0x*7 = U_State.ind).
 *
 * Solution: count down the known echo length (= wire_len) byte-by-byte and
 * discard bytes unconditionally while the counter is non-zero.  Only after
 * all echo bytes are consumed do we look for L_Data.con / U_State.ind.
 */
static atomic_t s_tx_echo_remaining;

/* Count of received octets whose 9th bit disagreed with even parity, i.e.
 * octets the transceiver flagged as damaged on the KNX bus (or that the host
 * link corrupted).  Exposed for diagnostics / PID_ERROR_FLAGS.
 */
static volatile uint32_t s_rx_parity_errors;

/* Count of UART_RX_STOPPED events (framing / overrun on the host link). */
static volatile uint32_t s_rx_stopped_events;

/* Count of 0x03 octets that looked like U_Reset.ind but arrived while we were
 * already in NORMAL state — i.e. duplicates or stray KNX data octets seen
 * after a loss of frame synchronisation.  A climbing value means the receiver
 * is desynchronised, not that the transceiver is resetting.
 */
static volatile uint32_t s_reset_ind_ignored;

/* Total 9-bit words received. */
static volatile uint32_t s_rx_words;

/* Number of octets to observe before judging the health of the 9-bit path. */
#define NCN_RX_HEALTH_SAMPLE 64u
static volatile bool s_rx_health_reported;

/*
 * Octets received during a TX window after the expected echo was fully
 * consumed.  Reset at the start of each knx_l1_send_frame(); a non-zero value
 * afterwards means the echo-length model is wrong, and stray echo octets are
 * reaching the service decoders.
 */
static volatile uint32_t s_tx_stray_bytes;

/* Bring-up diagnostics: dump the raw receive stream.  ISR context, so this is
 * deliberately a single LOG_DBG per word and must not be left on in a build
 * that has to keep up with real bus traffic.
 */
#if defined(CONFIG_NCN5130_TRACE_RX)
#define NCN_TRACE_WORD(w, ok)                                                                      \
	LOG_DBG("rx %03x octet=%02x p9=%u calc=%u %s bus=%d ncn=%d", (unsigned int)(w),            \
		(unsigned int)((w) & 0xFFu), (unsigned int)(((w) >> 8) & 1u),                      \
		(unsigned int)knx_parity8((uint8_t)((w) & 0xFFu)), (ok) ? "OK" : "FLAGGED",        \
		(int)atomic_get(&s_bus_state), (int)atomic_get(&s_ncn_state))
#else
#define NCN_TRACE_WORD(w, ok)                                                                      \
	do {                                                                                       \
	} while (0)
#endif

/*
 * Two-byte NCN5130 response parser state.
 * U_SystemStat.ind is the only multi-byte service sent by the NCN5130 to the
 * host.  When the first byte (0x4B) arrives, s_rx_parse_state is set to
 * NCN_RX_SYSSTAT so the very next byte is consumed as the status byte rather
 * than being misrouted through the single-byte pattern matchers.
 */
enum {
	NCN_RX_IDLE = 0,
	NCN_RX_SYSSTAT = 1,
};
static volatile uint8_t s_rx_parse_state = NCN_RX_IDLE;

/*
 * TX command buffer.
 * Small commands (U_Reset, U_SetAddress, U_Ackn): ≤4 bytes.
 * KNX frame burst: 2 bytes per wire byte (cmd + data), plus U_L_DataOffset
 * commands (1 byte) every 63 wire bytes for extended frames.
 *
 * Max extended frame: 263 wire bytes → 263 × 2 + ceil(263/63) offset cmds
 * = 526 + 5 = 531 bytes. Round up to 544 for alignment.
 */
#define KNX_MAX_STANDARD_WIRE_BYTES 24u
/* KNX spec 3/2/2: LG field max = 254 (255 = reserved escape code).
 * Extended wire frame = CTRL+CTRLE+SA(2)+DA(2)+LG+TPCI+APCI/data(LG)+FCS
 *                     = 9 + LG  →  max 9 + 254 = 263 bytes.
 */
#define KNX_MAX_EXTENDED_WIRE_BYTES 263u
#define KNX_BURST_BUF_SIZE          544u

/*
 * The NCN5130 indexes frame octets with 6 bits, so anything past the first 64
 * needs U_L_DataOffset.req — which knx_l1_send_frame() deliberately refuses
 * (see its comment).  Keep the advertised APDU length inside that block, or the
 * device would publish a PID_MAX_APDU_LENGTH it cannot honour.
 *
 * extended wire frame = 9 + LG octets, and the driver's limit is on
 * raw_len = wire_len - 1 = 8 + LG, hence LG <= 55.
 */
#define KNX_L1_MAX_BLOCK0_RAW_BYTES 63u
BUILD_ASSERT(8u + KNX_MAX_APDU_OCTETS <= KNX_L1_MAX_BLOCK0_RAW_BYTES,
	     "KNX_MAX_APDU_OCTETS exceeds what this driver can transmit without "
	     "U_L_DataOffset.req support — implement extended-block TX first "
	     "before raising it.");
/* uint16_t: 9-bit mode. Bits 7..0 = octet, bit 8 = even parity (see file header). */
static uint16_t s_tx_buf[KNX_BURST_BUF_SIZE];

/* -------- 9th-bit (parity) helpers -------- */

/** Even parity over bits 7..0. */
static inline uint8_t knx_parity8(uint8_t b)
{
	b ^= (uint8_t)(b >> 4);
	b ^= (uint8_t)(b >> 2);
	b ^= (uint8_t)(b >> 1);
	return (uint8_t)(b & 1u);
}

/** Build a 9-bit TX word: octet in bits 7..0, even parity in bit 8. */
static inline uint16_t knx_word_encode(uint8_t octet)
{
	return (uint16_t)octet | ((uint16_t)knx_parity8(octet) << 8);
}

/**
 * True when the received word's 9th bit matches even parity over its octet.
 *
 * A mismatch means "do not trust this octet": either the host link corrupted
 * it, or the NCN5130 deliberately flagged a KNX bus error on it (datasheet
 * p.27).  Both call for discarding the frame.
 *
 * Caveat for later: the parity bit is meaningless for U_IntRegRd/U_IntRegWr
 * responses, so whoever adds internal-register support (needed for
 * ANAOUTCTRL / the watchdog) must bypass this check for those replies.
 */
static inline bool knx_word_parity_ok(uint16_t word)
{
	return knx_parity8((uint8_t)(word & 0xFFu)) == (uint8_t)((word >> 8) & 1u);
}

/* Serializes all UART TX calls; s_tx_done_sem signals UART_TX_DONE */
static K_MUTEX_DEFINE(s_tx_mutex);
static K_SEM_DEFINE(s_tx_done_sem, 0, 1);

/*
 * s_ldata_con_sem — given once per KNX frame by the L_Data.con byte handler
 * (receive_byte, 0x0B/0x8B).  The L2 TX thread waits on this after sending
 * the last frame byte; s_ldata_con_status carries p_ok, p_error (negative
 * L_Data.con — no device ACKed), or p_collision_detected (NCN5130 reported
 * a genuine tx_err/rx_err via U_State.ind — lost bus arbitration).
 */
static K_SEM_DEFINE(s_ldata_con_sem, 0, 1);
static volatile P_Status s_ldata_con_status;

/* TX index for U_L_DataCont.req — used by Ph_Data__req byte-by-byte path
 * (kept for Req_ack_char and any future manual NCN5130 control sequences).
 */
static uint8_t s_p_tx_index;

/* Ping-pong RX buffers — 1 word each for per-octet L1 processing */
static uint16_t s_rx_buf[2][1];
static uint8_t s_rx_buf_idx;

/* One-shot timer: fires after EOF_TIMER_US of bus silence while BUS_RX_IN_PROGRESS */
static struct k_timer s_rx_eof_timer;

/* Work item to defer U_Reset.ind processing out of ISR context */
static struct k_work s_reset_ind_work;

/* Work item to send U_ExitStopMode.req from thread context after U_StopMode.ind */
static struct k_work s_exit_stop_work;

#if CONFIG_NCN5130_STATS_INTERVAL_MS > 0
static struct k_timer s_stats_timer;

static void stats_timer_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	LOG_INF("ncn5130 rx stats: words=%u parity_err=%u rx_stopped=%u "
		"reset_ignored=%u bus=%d ncn=%d",
		(unsigned int)s_rx_words, (unsigned int)s_rx_parity_errors,
		(unsigned int)s_rx_stopped_events, (unsigned int)s_reset_ind_ignored,
		(int)atomic_get(&s_bus_state), (int)atomic_get(&s_ncn_state));
}
#endif

/* -------- Blocking UART TX helper -------- */

/*
 * Serialize all UART TX calls through s_tx_mutex.
 * Blocks until UART_TX_DONE (or UART_TX_ABORTED) fires.
 * Safe to call from any thread context; must NOT be called from ISR.
 */
static int send_blocking(const uint8_t *data, size_t len)
{
	int ret;

	k_mutex_lock(&s_tx_mutex, K_FOREVER);
	for (size_t i = 0; i < len; i++) {
		s_tx_buf[i] = knx_word_encode(data[i]);
	}
	/* uart_tx_u16() takes a length in WORDS, not bytes. */
	ret = uart_tx_u16(s_uart_dev, s_tx_buf, len, SYS_FOREVER_US);
	if (ret == 0) {
		k_sem_take(&s_tx_done_sem, K_FOREVER);
	}
	k_mutex_unlock(&s_tx_mutex);
	return ret;
}

/* -------- NCN5130 command helpers -------- */

static void U_Reset__req_send(void)
{
	static const uint8_t cmd = 0x01;

	atomic_set(&s_ncn_state, NCN_STATE_RESET);
	send_blocking(&cmd, 1);
}

void U_SetAddress__req(unsigned char addr_low, unsigned char addr_high)
{
	uint8_t cmd[4] = {0xF1, addr_high, addr_low, 0x00};

	send_blocking(cmd, 4);
}

/*
 * Map KNX bus-level acknowledgment byte (FRAME_ACK/NACK/BUSY/NAK_BUSY) to
 * the NCN5130 U_Ackn.req command byte: 0b00010nba.
 */
static uint8_t u_ackn_from_frame(uint8_t frame_ack)
{
	bool nack = (frame_ack == FRAME_NACK || frame_ack == FRAME_NAK_BUSY);
	bool busy = (frame_ack == FRAME_BUSY || frame_ack == FRAME_NAK_BUSY);

	return (uint8_t)(0x10 | ((uint8_t)nack << 2) | ((uint8_t)busy << 1) | 1U);
}

/* -------- U_Reset.ind work handler (thread context) -------- */

static void reset_ind_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	LOG_DBG("U_Reset.ind — entering NORMAL");
	Ph_Reset__con(p_ok);

	/*
	 * Disable NCN5130 hardware retransmissions (U_SetRepetition.req).
	 *
	 * By default the NCN5130 retransmits up to 3 times on NACK and 3 times
	 * on BUSY.  Each retransmission generates a second echo of the TX frame
	 * back to the host UART.  The echo-countdown TX filter is calibrated for
	 * exactly ONE echo per knx_l1_send_frame() call.  A retry echo puts the
	 * same payload bytes on the host UART again; if position 9 of the frame
	 * happens to be 0x0B (= negative L_Data.con pattern), the retry echo
	 * prematurely triggers TX completion with p_error, causing spurious RX
	 * of the remaining echo bytes and a disconnection.
	 *
	 * Setting both retry counts to 0 guarantees exactly one echo per TX.
	 * The application-level retry loop in l_tx_thread_fn (6 retries with
	 * exponential backoff) provides equivalent reliability without corrupting
	 * the echo-byte state machine.
	 *
	 * Command: 0xF2  repCounters  0x00  0x00
	 *          repCounters = 0b0_bbb_0_nnn  (bbb=BUSY retries, nnn=NACK retries)
	 *          0x00 = bbb=0, nnn=0 → no hardware retransmissions
	 */
	static const uint8_t rep_cmd[4] = {0xF2u, 0x00u, 0x00u, 0x00u};

	send_blocking(rep_cmd, sizeof(rep_cmd));
	LOG_DBG("NCN5130: hardware retransmissions disabled (retry=0)");
}

static void exit_stop_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	static const uint8_t cmd = 0x0Fu; /* U_ExitStopMode.req */

	LOG_DBG("NCN5130: sending U_ExitStopMode.req");
	send_blocking(&cmd, 1);
	/* NCN5130 will transition Sync→Normal and eventually send U_Reset.ind */
}

/* -------- RX EOF timer callback (ISR context) -------- */

/*
 * Called after EOF_BIT_TIMEOUT of bus silence while a frame was being
 * received.  Runs in k_timer ISR context.
 *
 * Race guard: if the TX thread started a transmission (BUS_TX_IN_PROGRESS)
 * while a bus frame from another device was being partially received, the
 * TX state overwrites BUS_RX_IN_PROGRESS.  The EOF timer for that
 * interrupted frame fires here with s_bus_state already set to
 * BUS_TX_IN_PROGRESS.  Setting BUS_FREE here would discard the TX echo
 * filter and let remaining echo bytes reach the normal frame detector,
 * producing a spurious RX.  Only transition to FREE if we actually are
 * still in the RX phase — done as a single CAS rather than a
 * read then a write, so a TX thread claiming the bus between the two can
 * never be undone by this callback.
 */
static void rx_eof_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	if (!atomic_cas(&s_bus_state, BUS_RX_IN_PROGRESS, BUS_FREE)) {
		return;
	}
	Ph_Data__ind(Ind_end_of_Frame, 0);
}

/* -------- Per-octet RX parser (called from UART callback = ISR) -------- */

/*
 * receive_word — called from the UART ISR for every received 9-bit word.
 *
 * @param word  Bits 7..0 = octet, bit 8 = the transceiver's parity/error bit.
 */
static void receive_word(uint16_t word)
{
	uint8_t byte = (uint8_t)(word & 0xFFu);
	bool parity_ok = knx_word_parity_ok(word);

	s_rx_words++;
	NCN_TRACE_WORD(word, parity_ok);

	/*
	 * One-shot health check on the 9-bit receive path.
	 *
	 * A correctly wired 9-bit link produces almost no 9th-bit mismatches on a
	 * healthy bus, because the bit IS the even parity of the octet.  A rate
	 * anywhere near 50 % does not mean a noisy bus — it means the 9th bit
	 * being tested is not a parity bit at all.  The overwhelmingly likely
	 * cause is a DMA moving 8-bit items instead of 16-bit ones, so bit 8 of
	 * each "word" is really bit 0 of the following octet (and half the
	 * octets are being dropped entirely).  That width comes from the
	 * devicetree channel-config cell, not from the driver — see the comment
	 * on the KNX UART node in the board DTS.
	 */
	if (!s_rx_health_reported && s_rx_words >= NCN_RX_HEALTH_SAMPLE) {
		s_rx_health_reported = true;
		if (s_rx_parity_errors * 4u >= s_rx_words) {
			LOG_ERR("9th-bit mismatch on %u of the first %u octets. The 9th "
				"bit is not behaving as parity — check that the KNX UART's "
				"'dmas' channel-config in the board DTS sets "
				"STM32_DMA_MEM_16BITS | STM32_DMA_PERIPH_16BITS, which "
				"9-bit mode requires.",
				(unsigned int)s_rx_parity_errors, (unsigned int)s_rx_words);
		} else {
			LOG_DBG("9-bit RX healthy: %u mismatches in the first %u octets",
				(unsigned int)s_rx_parity_errors, (unsigned int)s_rx_words);
		}
	}

	/*
	 * 9th-bit check.  A mismatch means the NCN5130 flagged an
	 * acceptance-window or pulse-duration error on this octet (datasheet
	 * p.27), or the host link corrupted it.  Either way the octet is not
	 * trustworthy.
	 *
	 * Mid-frame we report Ind_parity_error so L1 can mark the accumulator
	 * corrupt and drop the frame at EOF — the KNX spec's own primitive for
	 * this (Ph_Data_Ind_Class, spec 3/3/1).  Outside a frame a damaged
	 * service byte is simply discarded: acting on a corrupt NCN5130 service
	 * code is worse than ignoring it.
	 */
	if (!parity_ok) {
		s_rx_parity_errors++;

		int bus_state = (int)atomic_get(&s_bus_state);

		if (bus_state == BUS_TX_IN_PROGRESS) {
			if (atomic_get(&s_tx_echo_remaining) > 0) {
				/* Damaged echo octet: still consume it so the
				 * countdown stays aligned with the frame we sent.
				 */
				atomic_dec(&s_tx_echo_remaining);
			}
			return;
		}

		/*
		 * Keeping frame synchronisation matters more than the individual
		 * octet.  A damaged CTRL octet that is simply dropped leaves the
		 * receiver in BUS_FREE, so every DATA octet of that frame is then
		 * fed to the single-byte service matchers below — and those are
		 * loose: 0x03 becomes a false U_Reset.ind, 0x2B a false
		 * U_StopMode.ind, anything with bits[2:0] = 111 a false
		 * U_State.ind.  So enter the frame anyway and let it be discarded
		 * at EOF via Ind_parity_error.
		 */
		if (bus_state == BUS_FREE && (byte & CTRL_FRAME_MASK) == CTRL_FRAME_VALUE) {
			atomic_set(&s_bus_state, BUS_RX_IN_PROGRESS);
			k_timer_start(&s_rx_eof_timer, K_USEC(EOF_TIMER_US), K_NO_WAIT);
			/* start_of_Frame stores the octet; bit_error condemns the
			 * frame without storing it a second time.
			 */
			Ph_Data__ind(Ind_start_of_Frame, byte);
			Ph_Data__ind(Ind_bit_error, byte);
			return;
		}

		if (bus_state == BUS_RX_IN_PROGRESS) {
			k_timer_start(&s_rx_eof_timer, K_USEC(EOF_TIMER_US), K_NO_WAIT);
			Ph_Data__ind(Ind_parity_error, byte);
		}
		return;
	}

	/*
	 * Multi-byte response: consume U_SystemStat.ind second byte before any
	 * other pattern matching.  The first byte (0x4B) sets s_rx_parse_state;
	 * the very next byte from the NCN5130 is the status byte regardless of
	 * its value (it could match U_Reset.ind, L_Data.con, etc. by accident).
	 */
	if (s_rx_parse_state == NCN_RX_SYSSTAT) {
		s_rx_parse_state = NCN_RX_IDLE;
		LOG_DBG("U_SystemStat.ind status=0x%02x v20v=%d vdd2=%d vbus=%d vfilt=%d xtal=%d "
			"tw=%d mode=%u",
			byte, (byte & 0x80u) ? 1 : 0, (byte & 0x40u) ? 1 : 0,
			(byte & 0x20u) ? 1 : 0, (byte & 0x10u) ? 1 : 0, (byte & 0x08u) ? 1 : 0,
			(byte & 0x04u) ? 1 : 0, (byte & 0x03u));
		return;
	}

	/*
	 * TX echo filter — count-based guard for BUS_TX_IN_PROGRESS.
	 *
	 * The NCN5130 echoes the full transmitted KNX frame (exactly wire_len
	 * bytes) back to the host UART before sending L_Data.con.  Pattern
	 * matching on echo bytes is unsafe: any payload byte can accidentally
	 * match an NCN5130 service pattern:
	 *   data byte 0x0B  → L_Data.con (negative) — premature TX completion
	 *   data byte 0x2B  → U_StopMode.ind — puts NCN5130 into STOP
	 *   data byte 0x**7 → U_State.ind
	 *
	 * Instead, consume exactly s_tx_echo_remaining bytes by count (set to
	 * wire_len before the uart_tx call).  Only after the counter reaches 0
	 * do we look for L_Data.con and U_State.ind.
	 */
	if (atomic_get(&s_bus_state) == BUS_TX_IN_PROGRESS) {
		if (atomic_get(&s_tx_echo_remaining) > 0) {
			/* Echo byte — discard unconditionally, regardless of value. */
			atomic_dec(&s_tx_echo_remaining);
			return;
		}
		/*
		 * Echo bytes exhausted — look for the real completion signal.
		 *
		 * L_Data.con is the ONLY terminator trusted here, and deliberately
		 * so.  Its pattern leaves just bit 7 free, i.e. 2 of 256 byte
		 * values.  U_State.ind's pattern (bits[2:0] = 111) matches 32 of
		 * 256, which is far too loose inside a window that is full of echo
		 * octets: a frame carrying the Group Object flag byte 0x4F — a
		 * perfectly ordinary value — was reported as
		 * "U_State.ind rx_err temp_warn" and burned two TX retries, purely
		 * because the echo accounting was off by a couple of octets.
		 *
		 * Nothing is lost by ignoring it: with hardware retransmission
		 * disabled the transceiver always closes a transmission with
		 * L_Data.con (negative if no device acknowledged), and a frame it
		 * refuses to transmit at all is caught by the 100 ms timeout in
		 * knx_l1_send_frame().
		 *
		 * In 9-bit mode there is no U_FrameState.ind to consider — the
		 * datasheet restricts it to SPI / 8-bit UART (p.41, p.49).
		 */
		if ((byte & 0x7Fu) == 0x0Bu) {
			atomic_set(&s_bus_state, BUS_FREE);
			s_ldata_con_status = (byte & 0x80u) ? p_ok : p_error;
			k_sem_give(&s_ldata_con_sem);
			return;
		}

		/*
		 * Not L_Data.con either.  Our own confirmation is
		 * still pending, but at medium busload the device is required to
		 * receive while transmitting (03_02_01 §5.7.3.3) — a genuine
		 * incoming frame from another device can and does interleave here,
		 * arriving as a whole block before our own L_Data.con.  Confirmed
		 * on the bench: an 8-octet echo consumed exactly, followed by 13
		 * further octets (an ETS request frame), then L_Data.con.
		 * Treating every such octet as a "stray" and discarding it lost
		 * that frame outright.
		 *
		 * Hand it to the normal RX path instead.  Our own pending
		 * confirmation lives purely in s_ldata_con_sem, not in
		 * s_bus_state, so moving to BUS_RX_IN_PROGRESS here does not lose
		 * it: the BUS_FREE branch below still recognises L_Data.con
		 * (identical 0x7F mask check) once this RX frame completes and
		 * hands the bus back via rx_eof_cb().
		 */
		if ((byte & CTRL_FRAME_MASK) == CTRL_FRAME_VALUE) {
			atomic_set(&s_bus_state, BUS_RX_IN_PROGRESS);
			k_timer_start(&s_rx_eof_timer, K_USEC(EOF_TIMER_US), K_NO_WAIT);
			Ph_Data__ind(Ind_start_of_Frame, byte);
			return;
		}

		/*
		 * Genuinely unexplained: neither our echo, our L_Data.con, nor the
		 * start of a new frame.  Count it — a non-zero stray count
		 * reported by knx_l1_send_frame() is the signal that the echo
		 * length model itself is wrong — and log a U_State.ind-shaped one,
		 * using the datasheet's own flag names (Table 13: sc re te pe tw
		 * 1 1 1).
		 */
		s_tx_stray_bytes++;
		if ((byte & 0x07u) == 0x07u) {
			LOG_DBG("TX window: U_State.ind-shaped stray 0x%02x%s%s%s%s%s", byte,
				(byte & 0x80u) ? " sc(slave_collision)" : "",
				(byte & 0x40u) ? " re(host_tx_corrupt)" : "",
				(byte & 0x20u) ? " te(knx_tx_error)" : "",
				(byte & 0x10u) ? " pe(protocol_error)" : "",
				(byte & 0x08u) ? " tw(temp_warning)" : "");
		}
		return;
	}

	/*
	 * When a frame is in progress, all bytes are inner-frame characters.
	 * Restart the EOF timer on every byte to extend the deadline.
	 */
	if (atomic_get(&s_bus_state) == BUS_RX_IN_PROGRESS) {
		k_timer_start(&s_rx_eof_timer, K_USEC(EOF_TIMER_US), K_NO_WAIT);
		Ph_Data__ind(Ind_inner_Frame_char, byte);
		return;
	}

	/* ---- BUS_FREE: normal NCN5130 service byte decoding ---- */

	/* Start of a new KNX L_Data frame (standard or extended).
	 * Pattern: bit6=0, bit4=1, bits1:0=0  →  mask 0x53 == 0x10
	 */
	if ((byte & CTRL_FRAME_MASK) == CTRL_FRAME_VALUE) {
		atomic_set(&s_bus_state, BUS_RX_IN_PROGRESS);
		k_timer_start(&s_rx_eof_timer, K_USEC(EOF_TIMER_US), K_NO_WAIT);
		Ph_Data__ind(Ind_start_of_Frame, byte);
		return;
	}

	/*
	 * L_Data.con: pattern x000_1011 (datasheet Table 13).
	 * Bit 7 (z): 1 = positive confirmation (IACK received), 0 = negative.
	 */
	if ((byte & 0x7Fu) == 0x0Bu) {
		s_ldata_con_status = (byte & 0x80u) ? p_ok : p_error;
		k_sem_give(&s_ldata_con_sem);
		return;
	}

	/*
	 * U_Reset.ind — the transceiver announces that it has entered Normal
	 * State.  Defer the bring-up work to a work queue (it needs uart_tx).
	 *
	 * 0x03 carries NO distinguishing bits: it is a bare value, so any KNX
	 * data octet that happens to equal 0x03 lands here if the receiver has
	 * lost frame synchronisation.  Re-running the whole bring-up
	 * (U_SetAddress.req + U_SetRepetition.req) on such a false positive is
	 * both wrong and self-sustaining, so only act on the transition INTO
	 * Normal State.  A 0x03 received while we already believe we are in
	 * Normal State is either a duplicate indication or a stray data octet;
	 * either way, ignoring it is correct.
	 *
	 * Residual gap: a genuine transceiver reset while we think we are in
	 * Normal State is now missed, and auto-acknowledge would stay off until
	 * the next U_SetAddress.req ("Auto-acknowledge can only be deactivated
	 * by a Reset Service", datasheet p.36).  The proper discriminator is
	 * U_SystemState.req -> U_SystemStat.ind mode == 11, not yet
	 * implemented; s_reset_ind_ignored makes the situation visible until then.
	 */
	if (byte == 0x03u) {
		if (atomic_get(&s_ncn_state) != NCN_STATE_NORMAL) {
			atomic_set(&s_ncn_state, NCN_STATE_NORMAL);
			atomic_set(&s_bus_state, BUS_FREE);
			k_work_submit(&s_reset_ind_work);
		} else {
			s_reset_ind_ignored++;
		}
		return;
	}

	/* U_State.ind: 0bvwxyz111 */
	if ((byte & 0x07u) == 0x07u) {
		LOG_WRN("U_State.ind 0x%02x%s%s%s%s", byte, (byte & 0x40u) ? " rx_err" : "",
			(byte & 0x20u) ? " tx_err" : "", (byte & 0x10u) ? " proto_err" : "",
			(byte & 0x08u) ? " temp_warn" : "");
		return;
	}

	/*
	 * U_FrameState.ind (0bwxy1z011) is deliberately NOT decoded: it is an
	 * SPI / 8-bit-UART service only (datasheet p.41, p.49) and cannot occur
	 * on this link.  Its mask accepts 16 of 256 byte values, so matching it
	 * here would swallow legitimate traffic — see the TX path above.
	 */

	/* U_Configure.ind: 0b0vwxyz01 */
	if ((byte & 0x83u) == 0x01u) {
		return;
	}

	/*
	 * U_FrameEnd.ind (0xCB) — only emitted when frame-end-with-MARKER is
	 * enabled via U_Configure.req, which this driver does not do (yet).
	 * If MARKER is ever enabled, remember the de-stuffing rule from the
	 * datasheet (p.49): a data octet 0xCB is echoed as 0xCB 0xCB, so a lone
	 * 0xCB is the frame end while a doubled one is data.
	 */
	if (byte == 0xCBu) {
		return;
	}

	/* U_StopMode.ind — request exit from thread context (send_blocking != ISR) */
	if (byte == 0x2Bu) {
		atomic_set(&s_ncn_state, NCN_STATE_STOP);
		LOG_WRN("NCN5130 entered STOP — scheduling U_ExitStopMode.req");
		k_work_submit(&s_exit_stop_work);
		return;
	}

	/* U_SystemStat.ind first byte — arm parser for the status byte. */
	if (byte == 0x4Bu) {
		s_rx_parse_state = NCN_RX_SYSSTAT;
		return;
	}
}

/* -------- UART async callback -------- */

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		k_sem_give(&s_tx_done_sem);
		break;

	case UART_RX_RDY: {
		/*
		 * Zephyr API asymmetry: uart_rx_enable_u16() takes a length in
		 * WORDS, but UART_RX_RDY still reports offset/len in BYTES (the
		 * STM32 driver's u16 wrappers just multiply the length by two and
		 * hand the buffer to the 8-bit path).  Convert back here.
		 */
		const uint16_t *words = (const uint16_t *)(evt->data.rx.buf + evt->data.rx.offset);
		size_t count = evt->data.rx.len / sizeof(uint16_t);

		for (size_t i = 0; i < count; i++) {
			receive_word(words[i]);
		}
		break;
	}

	case UART_RX_BUF_REQUEST:
		s_rx_buf_idx ^= 1u;
		/* Length in WORDS for the _u16 variant. */
		uart_rx_buf_rsp_u16(dev, s_rx_buf[s_rx_buf_idx], ARRAY_SIZE(s_rx_buf[0]));
		break;

	case UART_RX_STOPPED:
		/*
		 * Framing or overrun error on the host link.  Zephyr's STM32 driver
		 * leaves reception DISABLED after this, so it must be re-armed here
		 * or the KNX interface stays silent until the next reboot — which is
		 * exactly what used to happen, because this case was not handled at
		 * all.  (Parity errors used to land here too, on every octet the
		 * transceiver flagged; that source is gone now that hardware parity
		 * is disabled and the 9th bit is checked in software.)
		 */
		s_rx_stopped_events++;
		LOG_WRN("UART_RX_STOPPED reason=0x%x (total %u) — re-arming RX",
			evt->data.rx_stop.reason, (unsigned int)s_rx_stopped_events);
		/* Abandon any partially received frame. */
		if (atomic_cas(&s_bus_state, BUS_RX_IN_PROGRESS, BUS_FREE)) {
			k_timer_stop(&s_rx_eof_timer);
			Ph_Data__ind(Ind_framing_error, 0);
		}
		s_rx_parse_state = NCN_RX_IDLE;
		s_rx_buf_idx = 0u;
		if (uart_rx_enable_u16(dev, s_rx_buf[0], ARRAY_SIZE(s_rx_buf[0]), SYS_FOREVER_US) !=
		    0) {
			LOG_ERR("UART_RX_STOPPED: re-arming RX failed");
		}
		break;

	case UART_RX_BUF_RELEASED:
	case UART_RX_DISABLED:
		break;

	default:
		break;
	}
}

/* -------- Ph_Data.req (Physical Layer service — called from L2 TX thread) -------- */

void Ph_Data__req(Ph_Data_Req_Class p_class, uint8_t p_data)
{
	uint8_t cmd[2];
	size_t cmd_len;

	switch (p_class) {
	case Req_start_of_Frame:
		if (!atomic_cas(&s_bus_state, BUS_FREE, BUS_TX_IN_PROGRESS)) {
			Ph_Data__con(P_bus_not_free);
			return;
		}
		s_p_tx_index = 1;
		cmd[0] = 0x80u;
		cmd[1] = p_data;
		cmd_len = 2;
		break;

	case Req_inner_Frame_char:
		cmd[0] = (uint8_t)(0x80u | s_p_tx_index++);
		cmd[1] = p_data;
		cmd_len = 2;
		break;

	case Req_last_Frame_char:
		/* U_L_DataEnd: l = last_index + 1 = total wire bytes.
		 * s_p_tx_index is the position of the FCS byte; l must be s_p_tx_index+1.
		 */
		cmd[0] = (uint8_t)(0x40u | (s_p_tx_index + 1u));
		cmd[1] = p_data;
		cmd_len = 2;
		break;

	case Req_ack_char:
		cmd[0] = u_ackn_from_frame(p_data);
		cmd_len = 1;
		break;

	default:
		return;
	}
	send_blocking(cmd, cmd_len);
	/* Per-byte UART completion: notify L2 immediately so the byte loop
	 * can continue.  The bus-level L_Data.con (0x0B/0x8B from NCN5130)
	 * arrives later and is signalled via s_ldata_con_sem — the L2 TX
	 * thread waits on that after the last frame byte to get real status.
	 */
	Ph_Data__con(p_ok);
}

/*
 * Ph_Bus_L_Data_con_wait — called by the L2 TX thread after the last frame
 * byte to obtain the real bus-level confirmation from the NCN5130.
 *
 * Blocks until the NCN5130 sends L_Data.con (0x0B = negative, 0x8B = positive)
 * or the given timeout expires.  Returns p_ok on positive confirmation,
 * p_error on negative confirmation (no device ACKed the frame),
 * p_collision_detected on a genuine NCN5130 tx_err/rx_err (U_State.ind),
 * or p_transceiver_fault on timeout.
 */
P_Status Ph_Bus_L_Data_con_wait(k_timeout_t timeout)
{
	int ret = k_sem_take(&s_ldata_con_sem, timeout);

	if (ret != 0) {
		return p_transceiver_fault;
	}
	return s_ldata_con_status;
}

void Ph_Bus_Free(void)
{
	atomic_set(&s_bus_state, BUS_FREE);
}

/*
 * knx_l1_send_frame — burst-TX a complete KNX wire frame.
 *
 * Builds the interleaved NCN5130 command/data byte sequence in s_tx_buf,
 * then fires a single uart_tx() DMA burst.  Waits for UART_TX_DONE, then
 * waits for the bus-level L_Data.con (0x0B/0x8B) to arrive.
 *
 * Wire frame layout expected by caller:
 *   frame[0]     = CTRL byte
 *   frame[1..n-2] = SA, DA, AT|HC|LG, TPDU
 *   frame[n-1]   = FCS (already computed and appended by caller)
 *
 * Returns 0 on positive bus confirmation, -ECONNREFUSED on a negative
 * L_Data.con (frame sent correctly but no device on the bus ACKed it —
 * NOT a collision), -EIO on a genuine NCN5130-reported collision/transceiver
 * error (U_State.ind tx_err/rx_err — lost bus arbitration), -ETIMEDOUT on
 * no confirmation at all, -EBUSY if the bus was not free, -EMSGSIZE if len
 * exceeds the supported range for standard frames.
 */
int knx_l1_send_frame(const uint8_t *frame, uint16_t len)
{
	if (len < 8 || len > KNX_MAX_EXTENDED_WIRE_BYTES) {
		return -EMSGSIZE;
	}
	if (atomic_get(&s_ncn_state) != NCN_STATE_NORMAL) {
		LOG_WRN("%s: NCN5130 not in NORMAL state (%d)", __func__,
			(int)atomic_get(&s_ncn_state));
		return -EBUSY;
	}

	/*
	 * Claim the bus atomically, right here — before touching s_tx_buf at
	 * all.  The previous code checked s_bus_state !=
	 * BUS_FREE at this same point but only WROTE BUS_TX_IN_PROGRESS much
	 * later, after spending the time to build ~130 bytes of TX buffer
	 * under s_tx_mutex.  Across that whole window s_bus_state still read
	 * BUS_FREE, so the UART ISR could legitimately start receiving a
	 * frame from another device (BUS_RX_IN_PROGRESS) — and this function
	 * would then unconditionally overwrite that with BUS_TX_IN_PROGRESS,
	 * losing the in-flight RX frame with no trace.  The CAS closes the
	 * window: either it wins and the ISR sees a non-FREE state from here
	 * on, or the ISR already claimed BUS_RX_IN_PROGRESS and this CAS
	 * fails, giving -EBUSY instead of clobbering it.
	 */
	if (!atomic_cas(&s_bus_state, BUS_FREE, BUS_TX_IN_PROGRESS)) {
		return -EBUSY;
	}

	/*
	 * Build the interleaved NCN5130 command/data sequence.
	 *
	 * Indices 1..63 use U_L_DataCont directly (cmd = 0x80 | i). Per the
	 * datasheet's own command table, U_L_DataCont's index field is valid
	 * for i = 1..63 ONLY (cmd range 0x81-0xBF) — i = 0 is not a valid
	 * U_L_DataCont index: that command byte (0x80) is bit-for-bit
	 * identical to U_L_DataStart.req. The datasheet never documents
	 * whether the chip can tell the two apart once a frame is already
	 * in progress (e.g. after U_L_DataOffset.req selected block ≥ 1),
	 * so we cannot safely guess: reinterpreting it as a fresh
	 * U_L_DataStart.req would abort/corrupt the frame under
	 * construction, while blindly assuming it is accepted as index 0 of
	 * the new block — with no way to verify against real hardware here —
	 * risks silently shifting every following byte by one position and
	 * transmitting a corrupted frame onto the live KNX bus.  Fail closed
	 * instead of emitting an ambiguous byte; extended frames whose data
	 * stays within the first 64-byte block (raw_len ≤ 64, i.e. the vast
	 * majority of real KNX traffic) are unaffected.
	 */
	uint16_t pos = 0;
	uint16_t raw_len = len - 1u; /* bytes before FCS */

	if (raw_len > 64u) {
		LOG_ERR("%s: %u data bytes exceed the 64-byte block this "
			"driver can address unambiguously on the NCN5130 (see comment)",
			__func__, raw_len);
		atomic_set(&s_bus_state, BUS_FREE); /* release the claim taken above */
		return -ENOTSUP;
	}

	/*
	 * Hold the TX mutex across the buffer BUILD as well as the transfer.
	 * s_tx_buf is shared with send_blocking(), which is called from work items
	 * (U_SetAddress.req, U_SetRepetition.req, U_ExitStopMode.req) on a
	 * different thread — building here without the lock let a concurrent
	 * command overwrite the frame mid-construction or mid-DMA, putting garbage
	 * on the host link.
	 */
	k_mutex_lock(&s_tx_mutex, K_FOREVER);

	/* Every word gets its 9th bit (even parity) from knx_word_encode(). */

	/* Start byte (index 0) */
	s_tx_buf[pos++] = knx_word_encode(0x80u);
	s_tx_buf[pos++] = knx_word_encode(frame[0]);

	for (uint16_t i = 1; i < raw_len; i++) {
		s_tx_buf[pos++] = knx_word_encode((uint8_t)(0x80u | (uint8_t)(i & 0x3Fu)));
		s_tx_buf[pos++] = knx_word_encode(frame[i]);
	}

	/*
	 * U_L_DataEnd: l = raw_len = number of KNX data bytes EXCLUDING FCS.
	 *
	 * The NCN5130 datasheet says "l = last index + 1".  "Last index" is the
	 * index of the last U_L_DataCont byte (= raw_len - 1), so l = raw_len.
	 * The FCS is the EXTRA byte sent alongside U_L_DataEnd, not counted in l.
	 *
	 * Example: 9-byte wire frame (raw_len=8 data bytes + 1 FCS):
	 *   l = 8 → byte = 0x48     ← matches bare-metal NanoKnx
	 *   NOT l = 9 → 0x49 which the NCN5130 rejects with rx_err.
	 */
	if ((raw_len & 0x3Fu) == 0u) {
		s_tx_buf[pos++] = knx_word_encode((uint8_t)(0x08u | (uint8_t)(raw_len >> 6)));
	}
	/* l = raw_len */
	s_tx_buf[pos++] = knx_word_encode((uint8_t)(0x40u | (uint8_t)(raw_len & 0x3Fu)));
	s_tx_buf[pos++] = knx_word_encode(frame[raw_len]); /* FCS */

	s_tx_stray_bytes = 0u;
	atomic_set(&s_tx_echo_remaining, (int)len); /* NCN5130 echoes the KNX data octets */
	/* s_bus_state is already BUS_TX_IN_PROGRESS — claimed above, before the build. */

	/* pos counts WORDS, which is exactly what uart_tx_u16() wants. */
	int ret = uart_tx_u16(s_uart_dev, s_tx_buf, pos, SYS_FOREVER_US);

	if (ret == 0) {
		k_sem_take(&s_tx_done_sem, K_FOREVER);
	}
	k_mutex_unlock(&s_tx_mutex);

	if (ret != 0) {
		atomic_set(&s_bus_state, BUS_FREE);
		return ret;
	}

	P_Status con = Ph_Bus_L_Data_con_wait(K_MSEC(CONFIG_KNX_L_DATA_CON_TIMEOUT_MS));

	/*
	 * Echo accounting.  echo_left > 0 means the transceiver echoed FEWER
	 * octets than the frame length; stray > 0 means it echoed MORE (or
	 * unrelated traffic entered the window) and those octets reached the
	 * service decoders.  Either way the echo-length model needs revisiting —
	 * this is exactly how a payload octet of 0x4F got mistaken for
	 * "U_State.ind rx_err temp_warn" and burned two TX retries.
	 */
	if (atomic_get(&s_tx_echo_remaining) != 0 || s_tx_stray_bytes != 0u) {
		LOG_WRN("TX echo mismatch: len=%u echo_left=%u stray=%u con=%d", len,
			(unsigned int)atomic_get(&s_tx_echo_remaining),
			(unsigned int)s_tx_stray_bytes, con);
	} else {
		LOG_DBG("TX echo exact: len=%u con=%d", len, con);
	}

	if (con == p_ok) {
		return 0;
	} else if (con == p_transceiver_fault) {
		/*
		 * No L_Data.con arrived within the timeout — unlike every other
		 * exit path here, receive_word() never got to reset the bus
		 * state machine (it only does so when it actually recognizes
		 * U_State.ind or L_Data.con).  Without this, s_bus_state stays
		 * BUS_TX_IN_PROGRESS forever and every future knx_l1_send_frame()
		 * call — this retry and all later frames — fails immediately
		 * with -EBUSY.
		 *
		 * A confirmation for THIS frame may still arrive late, so drain the
		 * semaphore too: otherwise that late byte leaks a stale result into
		 * the next TX's wait.
		 */
		atomic_set(&s_bus_state, BUS_FREE);
		k_sem_reset(&s_ldata_con_sem);
		return -ETIMEDOUT;
	} else if (con == p_collision_detected) {
		return -EIO;
	}
	/* p_error: negative L_Data.con — frame sent, but no device ACKed it. */
	return -ECONNREFUSED;
}

void Ph_Reset__req(void)
{
	U_Reset__req_send();
}

/* -------- Zephyr driver initialisation -------- */

static int ncn5130_init(const struct device *dev)
{
	const struct ncn5130_config *cfg = dev->config;

	/*
	 * 9 data bits, hardware parity DISABLED.
	 *
	 * The NCN5130 is strapped for 9-bit UART (datasheet Table 9, p.27) and
	 * deliberately corrupts the 9th bit to flag a KNX bus error on an octet
	 * (p.27).  Enabling the STM32's parity checker would turn every flagged
	 * octet into a hardware parity error and a UART_RX_STOPPED, so the 9th
	 * bit is generated and validated in software instead — see the file
	 * header and knx_word_encode()/knx_word_parity_ok().
	 *
	 * Zephyr's STM32 driver accepts exactly this combination and rejects
	 * 9 data bits together with a parity setting (uart_stm32.c, ll2cfg).
	 */
	static const struct uart_config uart_cfg = {
		.baudrate = 38400,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_9,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};
	int ret;

	s_uart_dev = cfg->uart_dev;
	if (!device_is_ready(s_uart_dev)) {
		LOG_ERR("NCN5130: UART device not ready");
		return -ENODEV;
	}

	ret = uart_configure(s_uart_dev, &uart_cfg);
	if (ret != 0) {
		LOG_ERR("NCN5130: uart_configure failed: %d", ret);
		return ret;
	}

	ret = uart_callback_set(s_uart_dev, uart_cb, NULL);
	if (ret != 0) {
		LOG_ERR("NCN5130: uart_callback_set failed: %d", ret);
		return ret;
	}

	k_timer_init(&s_rx_eof_timer, rx_eof_cb, NULL);
	k_work_init(&s_reset_ind_work, reset_ind_work_fn);
	k_work_init(&s_exit_stop_work, exit_stop_work_fn);
#if CONFIG_NCN5130_STATS_INTERVAL_MS > 0
	k_timer_init(&s_stats_timer, stats_timer_cb, NULL);
	k_timer_start(&s_stats_timer, K_MSEC(CONFIG_NCN5130_STATS_INTERVAL_MS),
		      K_MSEC(CONFIG_NCN5130_STATS_INTERVAL_MS));
#endif

	/* Length in WORDS for the _u16 variant. */
	ret = uart_rx_enable_u16(s_uart_dev, s_rx_buf[0], ARRAY_SIZE(s_rx_buf[0]), SYS_FOREVER_US);
	if (ret != 0) {
		LOG_ERR("NCN5130: uart_rx_enable_u16 failed: %d", ret);
		return ret;
	}

	LOG_DBG("NCN5130 initialized on %s (9-bit UART, 38400 bps)", s_uart_dev->name);

	return 0;
}

#define DT_DRV_COMPAT onnn_ncn5130

static const struct ncn5130_config ncn5130_cfg_0 = {
	.uart_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, uart)),
};

DEVICE_DT_INST_DEFINE(0, ncn5130_init, NULL, NULL, &ncn5130_cfg_0, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
