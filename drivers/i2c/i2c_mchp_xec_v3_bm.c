/*
 * Copyright (c) 2026, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Microchip XEC I2Cv3 byte-mode I2C driver.
 *
 * This driver programs the v3.8 I2C-SMBus controller through its legacy
 * "byte mode" core: a START/address is issued through the Control
 * Register (CR) and every subsequent byte generates a single PIN
 * interrupt (nine I2C clocks). The interrupt service routine advances a
 * per-byte state machine that follows the byte-mode transfer sequences
 * in Tables 6-5 .. 6-9 of the "I2C-SMBus controller v3.8" datasheet.
 * The network layer FSM and DMA are NOT used.
 *
 * Two device flavours share this file:
 *   microchip,xec-i2c-v3          - the physical controller block. Owns
 *                                   the registers, runtime state and the
 *                                   PIN interrupt.
 *   microchip,xec-i2c-v3-port     - a virtual I2C bus mapped onto one of
 *                                   the controller's 16 pin-mux ports.
 *                                   Devices hang off the port node; the
 *                                   Zephyr i2c_driver_api is bound here.
 *
 * Several port nodes may reference one controller. They are serialized
 * on the controller's k_sem lock, and a transfer routed to a port whose
 * pin-mux/frequency is not the one currently programmed triggers a full
 * controller re-program (PCR reset) onto that port before the transfer.
 *
 * Synchronous transfers block the calling thread on a semaphore that the
 * ISR gives at completion. Asynchronous (CONFIG_I2C_CALLBACK) transfers
 * return immediately; the ISR hands completion to a custom kernel work
 * queue -- shared by all controller instances -- which invokes the user
 * callback outside interrupt context.
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/mchp_xec_i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2c_mchp_xec_v3_bm, CONFIG_I2C_LOG_LEVEL);

#include "i2c_mchp_xec_regs.h"

/* Control-register byte constants.
 *
 * CR is write-only at offset 0. The byte-mode datasheet gives these as
 * polled-mode values (ENI=0); this driver is interrupt-driven so ENI is
 * OR'd into the commands that must continue producing per-byte PIN
 * interrupts. The terminal STOP deliberately leaves ENI clear so no
 * spurious interrupt fires after the bus is released.
 *
 *   BM_CR_START     0xCD  PIN|ESO|ENI|STA|ACK  START + clock address byte
 *   BM_CR_RPT_START 0x4D  ESO|ENI|STA|ACK      repeated START (PIN not
 *                                              written; address byte is
 *                                              clocked by the following
 *                                              Data Register write)
 *   BM_CR_NACK      0x48  ESO|ENI              arm a NACK (ACK=0) ahead of
 *                                              the final read byte
 *   BM_CR_STOP      0xC3  PIN|ESO|STO|ACK      STOP, release bus, ENI off
 *   BM_CR_DFLT      0xC1  PIN|ESO|ACK          idle: serial on, auto-ACK
 */
#define BM_CR_ENI BIT(XEC_I2C_CR_ENI_POS)
#define BM_CR_START                                                                                \
	(BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BM_CR_ENI | BIT(XEC_I2C_CR_STA_POS) | \
	 BIT(XEC_I2C_CR_ACK_POS))
#define BM_CR_RPT_START                                                                            \
	(BIT(XEC_I2C_CR_ESO_POS) | BM_CR_ENI | BIT(XEC_I2C_CR_STA_POS) | BIT(XEC_I2C_CR_ACK_POS))
#define BM_CR_NACK (BIT(XEC_I2C_CR_ESO_POS) | BM_CR_ENI)
#define BM_CR_STOP                                                                                 \
	(BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_STO_POS) |             \
	 BIT(XEC_I2C_CR_ACK_POS))
#define BM_CR_DFLT (BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ACK_POS))

/* Status-register bit helpers (RO at offset 0). */
#define BM_SR_NBB BIT(XEC_I2C_SR_NBB_POS)
#define BM_SR_LAB BIT(XEC_I2C_SR_LAB_POS)
#define BM_SR_LRB BIT(XEC_I2C_SR_LRB_AD0_POS) /* Last Received Bit: target ACK, 0=ACK 1=NACK */
#define BM_SR_BER BIT(XEC_I2C_SR_BER_POS)
#define BM_SR_PIN BIT(XEC_I2C_SR_PIN_POS)

/* "Bus idle and controller healthy" pattern: PIN=1 (no service), NBB=1
 * (bus free), no error bits. Anything else on transfer entry drives the
 * bit-bang recovery path. Matches the v3-NL sibling driver.
 */
#define BM_SR_IDLE 0x81U

/* End-of-transaction signalling. This controller does NOT interrupt on
 * STOP when it is the host. Instead the Completion register's IDLE status
 * (bit 29) latches when SR.NBB transitions 0->1 -- i.e. after the STOP is
 * generated and both SCL/SDA are released and pulled high. CFG.IDLE_IEN
 * (bit 29) gates that latch onto the interrupt output.
 *
 * IDLE_IEN must only be enabled while NBB==0 (mid-transaction, after START
 * and before the STOP has completed). Enabling it while NBB==1 fires the
 * interrupt immediately; and the Idle Scaling register's FAIR_IDLE_DELAY
 * (offset 0x24, ~31.75 us at 100 kHz / 16 MHz baud) means NBB stays 1 for
 * a while *after* the CR write that requests a START, so enabling IDLE_IEN
 * before/at START would latch IDLE before the transaction even begins.
 * The driver therefore enables IDLE_IEN only when it writes the terminal
 * STOP, and disables it again in the IDLE interrupt.
 */
#define BM_CMPL_IDLE    BIT(XEC_I2C_CMPL_IDLE_POS)
#define BM_CFG_IDLE_IEN BIT(XEC_I2C_CFG_IDLE_IEN_POS)

/* BBCR (bit-bang control) modes on v3.8 -- see the v3-NL driver for the
 * full description. Live-readback keeps the pins on the I2C engine while
 * exposing the true SCL/SDA line state; the *_LOW / RELEASED values drive
 * the recovery clock burst and STOP.
 */
#define BM_BBCR_SCL_IN      BIT(XEC_I2C_BBCR_SCL_IN_POS)
#define BM_BBCR_SDA_IN      BIT(XEC_I2C_BBCR_SDA_IN_POS)
#define BM_BBCR_LIVE_RD     0x80U
#define BM_BBCR_BB_RELEASED 0x01U
#define BM_BBCR_BB_SCL_LOW  0x03U
#define BM_BBCR_BB_SDA_LOW  0x05U

/* Recovery timing: nine ~100 kHz clocks plus a STOP, retried against a
 * stuck slave; SCL-stuck-low polled for 10 ms.
 */
#define BM_BB_HALF_PERIOD_US   5U
#define BM_BB_POLL_INTERVAL_US 1000U
#define BM_BB_SCL_POLL_LOOPS   10U
#define BM_BB_SDA_RECOV_LOOPS  10U

/* Legacy I2C control-register default: ESO+ACK+PIN. PIN=1 clears any
 * latent PIN assertion left by the core.
 */
#define BM_CR_RESET_DFLT                                                                           \
	(BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ACK_POS) | BIT(XEC_I2C_CR_PIN_POS))

#define BM_RWBIT_READ 1U

#define BM_INVALID_PORT 0xFFU

#define BM_TIMEOUT K_MSEC(1000)

#ifdef CONFIG_I2C_TARGET
/* Target (peripheral) mode. The received address byte carries the R/W
 * direction in bit 0; the default byte clocked out when the application
 * has no data for a target-transmit is 0. Target-mode CR commands:
 *   BM_CR_TGT_ARM   PIN|ESO|ACK|ENI  auto-ACK own address, interrupt on PIN
 *   BM_CR_TGT_NACK  PIN|ESO|ENI      ACK cleared: NACK the next received byte
 * (ACK must be re-armed before the next transaction or the controller would
 * NACK its own target address.)
 */
#define BM_TGT_RW_POS    0U
#define BM_TGT_DFLT_DATA 0U
#define BM_CR_TGT_ARM                                                                              \
	(BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BIT(XEC_I2C_CR_ACK_POS) | BM_CR_ENI)
#define BM_CR_TGT_NACK (BIT(XEC_I2C_CR_PIN_POS) | BIT(XEC_I2C_CR_ESO_POS) | BM_CR_ENI)

#define BM_SR_AAT BIT(XEC_I2C_SR_AAT_POS)
#define BM_SR_STO BIT(XEC_I2C_SR_STO_POS)
#endif /* CONFIG_I2C_TARGET */

/* Per-byte transfer state driven by the PIN interrupt. */
enum bm_state {
	BM_STATE_IDLE,  /* no transfer in flight                          */
	BM_STATE_START, /* START/address issued, awaiting address-phase PIN */
	BM_STATE_WRITE, /* host transmitter, streaming data bytes         */
	BM_STATE_READ,  /* host receiver, clocking data bytes             */
	BM_STATE_STOP,  /* terminal STOP issued, awaiting IDLE interrupt   */
	BM_STATE_DONE,  /* terminal, completion signalled                 */
};

/* Controller bus-clock / timing rows for a 16 MHz baud clock. Values are
 * the named constants from i2c_mchp_xec_regs.h; MR1 mirrors the v3-NL
 * driver's initialisation of the reserved-1 register.
 */
struct bm_timing {
	uint32_t data_timing;
	uint32_t idle_scaling;
	uint32_t timeout_scaling;
	uint16_t bus_clock;
	uint8_t rpt_start_hold_tm;
	uint8_t mr1;
};

static const struct bm_timing bm_timing_tbl[] = {
	{
		/* 100 kHz */
		.data_timing = XEC_I2C_SMB_DATA_TM_100K,
		.idle_scaling = XEC_I2C_SMB_IDLE_SC_100K,
		.timeout_scaling = XEC_I2C_SMB_TMO_SC_100K,
		.bus_clock = XEC_I2C_SMB_BUS_CLK_100K,
		.rpt_start_hold_tm = XEC_I2C_SMB_RSHT_100K,
		.mr1 = XEC_I2C_MR0_TM_BAUD16M,
	},
	{
		/* 400 kHz */
		.data_timing = XEC_I2C_SMB_DATA_TM_400K,
		.idle_scaling = XEC_I2C_SMB_IDLE_SC_400K,
		.timeout_scaling = XEC_I2C_SMB_TMO_SC_400K,
		.bus_clock = XEC_I2C_SMB_BUS_CLK_400K,
		.rpt_start_hold_tm = XEC_I2C_SMB_RSHT_400K,
		.mr1 = XEC_I2C_MR0_TM_BAUD16M,
	},
	{
		/* 1 MHz */
		.data_timing = XEC_I2C_SMB_DATA_TM_1M,
		.idle_scaling = XEC_I2C_SMB_IDLE_SC_1M,
		.timeout_scaling = XEC_I2C_SMB_TMO_SC_1M,
		.bus_clock = XEC_I2C_SMB_BUS_CLK_1M,
		.rpt_start_hold_tm = XEC_I2C_SMB_RSHT_1M,
		.mr1 = XEC_I2C_MR0_TM_BAUD16M,
	},
};

struct xec_i2c_v3_bm_xcfg {
	uintptr_t base;
	uint32_t dflt_freq;
	uint16_t enc_pcr;
	uint8_t girq;
	uint8_t girq_pos;
	void (*irq_connect)(void);
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	/* Statically allocated per-controller staging buffer for target
	 * receive: bytes the external host writes are accumulated here and
	 * handed to buf_write_received() in one call at STOP.
	 */
	uint8_t *tgt_rx_buf;
	uint16_t tgt_rx_buf_size;
#endif
};

struct xec_i2c_v3_bm_xdat {
	const struct device *ctrl_dev;
	struct k_sem lock; /* serializes ports sharing this controller */
	struct k_sem sync; /* ISR -> thread completion for sync transfers */
	enum bm_state state;
	int err;
	uint32_t active_freq;
	uint8_t active_port;
	/* Current transfer. buf/blen/bpos track the message currently on
	 * the wire; rx_total/rx_done track the whole contiguous read run so
	 * the terminating NACK lands on the run's final byte even when the
	 * run spans several i2c_msg buffers.
	 */
	struct i2c_msg *msgs;
	uint8_t num_msgs;
	uint8_t msg_idx;
	uint16_t addr;
	bool dir_read;
	uint8_t *buf;
	uint32_t blen;
	uint32_t bpos;
	uint32_t rx_total;
	uint32_t rx_done;
#ifdef CONFIG_I2C_CALLBACK
	struct k_work kw; /* async completion dispatch */
	i2c_callback_t cb;
	void *cb_user_data;
	const struct device *cb_dev;
	struct k_work_delayable timeout_dwork; /* async completion watchdog */
	atomic_t async_done;                   /* claim: completion vs watchdog */
#endif
#ifdef CONFIG_I2C_TARGET
	/* Target (peripheral) mode. Registered targets occupy the two OA
	 * slots; the controller is pinned to target_port while attached, and
	 * all controller-mode (master) entry points return -EBUSY. active_slot
	 * is the OA slot matched by the in-progress transaction; target_read
	 * tracks its direction.
	 */
	struct i2c_target_config *targets[XEC_I2C_MAX_TARGETS];
	const struct device *target_port;
	bool target_attached;
	bool target_read;
	uint8_t active_slot;
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	uint16_t tgt_rx_pos; /* bytes staged in cfg->tgt_rx_buf this transaction */
	uint8_t *tgt_tx_buf; /* buffer returned by buf_read_requested()          */
	uint32_t tgt_tx_len; /* its length                                       */
	uint32_t tgt_tx_pos; /* next byte to transmit                            */
#endif
#endif /* CONFIG_I2C_TARGET */
#ifdef CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE
	volatile uint32_t capidx;
	volatile uint8_t capture[CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE_SIZE] __aligned(4);
#endif
};

struct xec_i2c_v3_bm_port_xcfg {
	const struct device *parent;
	const struct pinctrl_dev_config *pcfg;
	uint32_t bitrate;
	uint8_t port_id;
	bool is_default;
};

#ifdef CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE
static void xec_i2c_bm_cap_init(struct xec_i2c_v3_bm_xdat *xdat)
{
	xdat->capidx = 0;
	memset((void *)xdat->capture, 0, CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE_SIZE);
}

static void xec_i2c_bm_cap_update(struct xec_i2c_v3_bm_xdat *xdat, uint8_t capval)
{
	if (xdat->capidx >= CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE_SIZE) {
		return;
	}

	xdat->capture[xdat->capidx++] = capval;
}

int mchp_xec_i2c_bm_clear_capture(const struct device *port_dev)
{
	if (port_dev == NULL) {
		return -EINVAL;
	}

	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	struct xec_i2c_v3_bm_xdat *const xdat = pc->parent->data;

	if (xdat->state != BM_STATE_IDLE) {
		return -EBUSY;
	}

	k_sem_take(&xdat->lock, K_FOREVER);
	xec_i2c_bm_cap_init(xdat);
	k_sem_give(&xdat->lock);

	return 0;
}

int mchp_xec_i2c_bm_copy_capture(const struct device *port_dev, uint8_t *capdest,
				 size_t capdest_size)
{
	if ((port_dev == NULL) || (capdest == NULL)) {
		return -EINVAL;
	}

	if (capdest_size == 0) {
		return 0;
	}

	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	struct xec_i2c_v3_bm_xdat *const xdat = pc->parent->data;

	if (xdat->state != BM_STATE_IDLE) {
		return -EBUSY;
	}

	k_sem_take(&xdat->lock, K_FOREVER);

	size_t n = (capdest_size < CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE_SIZE)
			   ? capdest_size
			   : CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE_SIZE;

	memcpy(capdest, (const void *)xdat->capture, n);

	k_sem_give(&xdat->lock);

	return 0;
}
#else
static void xec_i2c_bm_cap_init(struct xec_i2c_v3_bm_xdat *xdat)
{
}

static void xec_i2c_bm_cap_update(struct xec_i2c_v3_bm_xdat *xdat, uint8_t capval)
{
}

int mchp_xec_i2c_bm_clear_capture(const struct device *port_dev)
{
	return -ENOSYS;
}

int mchp_xec_i2c_bm_copy_capture(const struct device *i2c_nl_dev, uint8_t *capdest,
				 size_t capdest_size)
{
	return -ENOSYS;
}
#endif /* CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE */

#ifdef CONFIG_I2C_CALLBACK
/* One kernel work queue shared by every controller instance; each instance
 * owns its own k_work item (data->kw). The handler dispatches the completion
 * callback for whichever instance signalled. Used only by the asynchronous
 * (transfer_cb) path -- synchronous transfers complete on the caller's thread
 * via data->sync -- so the whole thing is compiled out when CONFIG_I2C_CALLBACK
 * is disabled.
 */
K_THREAD_STACK_DEFINE(xec_i2c_v3_bm_q_stack, CONFIG_I2C_MCHP_XEC_V3_BM_KWQ_STACK_SIZE);
static struct k_work_q xec_i2c_v3_bm_work_q;
static bool xec_i2c_v3_bm_wq_started;
#endif

/* ---- helpers ---- */

static inline uint8_t bm_addr_byte(uint16_t addr, bool read)
{
	/* Hardware is 7-bit only; LSB carries the R/W bit. */
	return (uint8_t)(((addr & XEC_I2C_TARGET_ADDR_MSK) << 1) | (read ? BM_RWBIT_READ : 0U));
}

static inline bool bm_msg_is_read(const struct i2c_msg *m)
{
	return (m->flags & I2C_MSG_READ) != 0U;
}

/* Write-1-to-clear the named status bits in the Completion register while
 * preserving its read/write control bits[5:2]. The completion register mixes
 * RW1C status (IDLE, BER, ...) with RW enables (DTEN/HCEN/TCEN/BIDEN) in one
 * word, so a bare sys_write32 of a status constant would also write 0 into
 * those enables -- harmless only while they are unused. Read-modify-write:
 * OR the current RW control bits back in, and mask `bits` to the RW1C field
 * (RW1C bits written 0 are left unchanged; RO bits are ignored on write).
 */
static inline void bm_cmpl_clear(uintptr_t base, uint32_t bits)
{
	uint32_t rw = sys_read32(base + XEC_I2C_CMPL_OFS) & XEC_I2C_CMPL_RW_MSK;

	sys_write32(rw | (bits & XEC_I2C_CMPL_RW1C_MSK), base + XEC_I2C_CMPL_OFS);
}

static const struct bm_timing *bm_timing_for(uint32_t freqhz)
{
	if (freqhz <= KHZ(100)) {
		return &bm_timing_tbl[0];
	}
	if (freqhz <= KHZ(400)) {
		return &bm_timing_tbl[1];
	}
	return &bm_timing_tbl[2];
}

/* True when the current message is the last of the transfer (or is
 * explicitly flagged with STOP): the run terminates with a STOP rather
 * than a repeated START.
 */
static bool bm_stop_here(const struct xec_i2c_v3_bm_xdat *xdat)
{
	return ((xdat->msg_idx + 1U) >= xdat->num_msgs) ||
	       ((xdat->msgs[xdat->msg_idx].flags & I2C_MSG_STOP) != 0U);
}

/* Total bytes in the contiguous read run beginning at msg_idx: successive
 * read messages without a RESTART flag are one address segment on the
 * wire, so the final NACK is computed against the run, not one message.
 */
static uint32_t bm_run_read_total(const struct xec_i2c_v3_bm_xdat *xdat)
{
	uint32_t total = 0;

	for (uint8_t i = xdat->msg_idx; i < xdat->num_msgs; i++) {
		total += xdat->msgs[i].len;
		/* A message flagged I2C_MSG_STOP ends its group (and thus the
		 * read run) -- keep this consistent with bm_stop_here(), which
		 * issues the STOP there. Without this, a mid-array STOP on a
		 * read would over-count the run and mis-time the terminal NACK.
		 */
		if ((xdat->msgs[i].flags & I2C_MSG_STOP) != 0U) {
			break;
		}
		if ((i + 1U) >= xdat->num_msgs) {
			break;
		}
		const struct i2c_msg *nx = &xdat->msgs[i + 1U];

		if ((nx->flags & I2C_MSG_RESTART) != 0U || !bm_msg_is_read(nx)) {
			break;
		}
	}

	return total;
}

/* Full controller programming: PCR reset, port select, timing, enable.
 * The byte-mode PIN interrupt is enabled through CR.ENI per transfer, so
 * unlike the v3-NL sibling this leaves all CFG interrupt-enable bits off.
 * Must be called with no transfer in flight (lock held, or pre-transfer).
 */
static int xec_i2c_v3_bm_program_ctrl(const struct device *ctrl, uint32_t freqhz, uint8_t port)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *data = ctrl->data;
	const struct bm_timing *tm = bm_timing_for(freqhz);
	uintptr_t base = cfg->base;

	soc_ecia_girq_ctrl(cfg->girq, cfg->girq_pos, MCHP_MEC_ECIA_GIRQ_DIS);

	sys_write32(0U, base + XEC_I2C_CFG_OFS);

	soc_xec_pcr_reset_en(cfg->enc_pcr);
	k_busy_wait(10U);

	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);

	/* PIN=1 clears any latent assertion in the legacy core. */
	sys_write8(BIT(XEC_I2C_CR_PIN_POS), base + XEC_I2C_CR_OFS);

	/* Port select, digital filter on, general-call disabled. No CFG
	 * interrupt-enable bits: byte-mode interrupts arrive via CR.ENI.
	 * ENAB is asserted last, after timing has been written.
	 */
	sys_write32(XEC_I2C_CFG_PORT_SET(port) | BIT(XEC_I2C_CFG_FEN_POS) |
			    BIT(XEC_I2C_CFG_GC_DIS_POS),
		    base + XEC_I2C_CFG_OFS);

	/* Clear any latched completion status before re-enabling the GIRQ. */
	sys_write32(sys_read32(base + XEC_I2C_CMPL_OFS), base + XEC_I2C_CMPL_OFS);

	sys_write32(tm->data_timing, base + XEC_I2C_DT_OFS);
	sys_write32(tm->idle_scaling, base + XEC_I2C_ISC_OFS);
	sys_write32(tm->timeout_scaling, base + XEC_I2C_TMOUT_SC_OFS);
	sys_write32((uint32_t)tm->bus_clock, base + XEC_I2C_BCLK_OFS);
	soc_mmcr_mask_set8(base + XEC_I2C_RSHT_OFS, tm->rpt_start_hold_tm, XEC_I2C_RSHT_MSK);
	soc_mmcr_mask_set8(base + XEC_I2C_MR0_OFS, tm->mr1, XEC_I2C_MR0_TM_MSK);

	sys_write8(BM_CR_RESET_DFLT, base + XEC_I2C_CR_OFS);
	sys_set_bit(base + XEC_I2C_CFG_OFS, XEC_I2C_CFG_ENAB_POS);
	k_busy_wait(20U);

	/* Live-readback so recovery reads pick up the true line state. */
	sys_write8(BM_BBCR_LIVE_RD, base + XEC_I2C_BBCR_OFS);

	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	soc_ecia_girq_ctrl(cfg->girq, cfg->girq_pos, MCHP_MEC_ECIA_GIRQ_EN);

	data->active_freq = freqhz;
	data->active_port = port;

	return 0;
}

/* Route the controller onto this port. The v3.8 core requires a full PCR
 * reset (via program_ctrl) when the port mux or frequency changes; a bare
 * CFG.PORT RMW leaves the core FSM stale. No-op when already on the port.
 */
static int xec_i2c_v3_bm_apply_port(const struct device *port_dev)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *data = ctrl->data;
	uint32_t freq;
	int rc;

	if (data->active_port == pc->port_id) {
		return 0;
	}

	rc = pinctrl_apply_state(pc->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc != 0) {
		LOG_ERR("pinctrl_apply_state(%s)=%d", port_dev->name, rc);
		return rc;
	}

	freq = (pc->bitrate != 0U) ? pc->bitrate : cfg->dflt_freq;
	return xec_i2c_v3_bm_program_ctrl(ctrl, freq, pc->port_id);
}

/* Hard PCR reset and re-arm on the current (freq, port); invalidate the
 * active port so the next transfer re-applies pinctrl.
 */
static void xec_i2c_v3_bm_abort(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *data = ctrl->data;
	uint32_t freq = (data->active_freq != 0U) ? data->active_freq : cfg->dflt_freq;
	uint8_t port = (data->active_port == BM_INVALID_PORT) ? 0U : data->active_port;

	/* program_ctrl rewrites CFG (IDLE_IEN off) and CR, clearing any
	 * pending PIN/IDLE interrupt source left by the aborted transfer.
	 */
	(void)xec_i2c_v3_bm_program_ctrl(ctrl, freq, port);
	data->state = BM_STATE_IDLE;
	data->active_port = BM_INVALID_PORT;
}

static void xec_i2c_v3_bm_bb_clock_burst(uintptr_t base)
{
	for (uint32_t i = 0; i < 9U; i++) {
		sys_write8(BM_BBCR_BB_SCL_LOW, base + XEC_I2C_BBCR_OFS);
		k_busy_wait(BM_BB_HALF_PERIOD_US);
		sys_write8(BM_BBCR_BB_RELEASED, base + XEC_I2C_BBCR_OFS);
		k_busy_wait(BM_BB_HALF_PERIOD_US);
	}
}

static void xec_i2c_v3_bm_bb_stop(uintptr_t base)
{
	sys_write8(BM_BBCR_BB_SDA_LOW, base + XEC_I2C_BBCR_OFS);
	k_busy_wait(BM_BB_HALF_PERIOD_US);
	sys_write8(BM_BBCR_BB_RELEASED, base + XEC_I2C_BBCR_OFS);
	k_busy_wait(BM_BB_HALF_PERIOD_US);
}

/* Recover a stuck bus: reset the controller, and if SDA is held low drive
 * nine SCL clocks + a STOP (up to 10 rounds), then reset again. Returns
 * -EIO if SCL is stuck low or the lines will not release. Called with the
 * controller lock held. Adapted from the v3-NL driver.
 */
static int xec_i2c_v3_bm_bus_recover(const struct device *ctrl, uint32_t freq, uint8_t port)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	uintptr_t base = cfg->base;
	uint8_t bbcr = 0;
	int rc;

	rc = xec_i2c_v3_bm_program_ctrl(ctrl, freq, port);
	if (rc != 0) {
		return rc;
	}
	if (sys_read8(base + XEC_I2C_SR_OFS) == BM_SR_IDLE) {
		return 0;
	}

	for (uint32_t i = 0; i < BM_BB_SCL_POLL_LOOPS; i++) {
		bbcr = sys_read8(base + XEC_I2C_BBCR_OFS);
		if ((bbcr & BM_BBCR_SCL_IN) != 0U) {
			break;
		}
		k_busy_wait(BM_BB_POLL_INTERVAL_US);
	}
	if ((bbcr & BM_BBCR_SCL_IN) == 0U) {
		LOG_ERR("i2c-recover: SCL stuck low");
		return -EIO;
	}

	if ((bbcr & BM_BBCR_SDA_IN) == 0U) {
		sys_write8(BM_BBCR_BB_RELEASED, base + XEC_I2C_BBCR_OFS);
		k_busy_wait(BM_BB_HALF_PERIOD_US);

		for (uint32_t i = 0; i < BM_BB_SDA_RECOV_LOOPS; i++) {
			xec_i2c_v3_bm_bb_clock_burst(base);
			xec_i2c_v3_bm_bb_stop(base);
			bbcr = sys_read8(base + XEC_I2C_BBCR_OFS);
			if ((bbcr & BM_BBCR_SDA_IN) != 0U) {
				break;
			}
		}
	}

	sys_write8(BM_BBCR_LIVE_RD, base + XEC_I2C_BBCR_OFS);

	(void)xec_i2c_v3_bm_program_ctrl(ctrl, freq, port);

	bbcr = sys_read8(base + XEC_I2C_BBCR_OFS);
	if ((bbcr & (BM_BBCR_SCL_IN | BM_BBCR_SDA_IN)) != (BM_BBCR_SCL_IN | BM_BBCR_SDA_IN)) {
		LOG_ERR("i2c-recover: SCL=%u SDA=%u still not both high",
			(bbcr & BM_BBCR_SCL_IN) ? 1U : 0U, (bbcr & BM_BBCR_SDA_IN) ? 1U : 0U);
		return -EIO;
	}

	return 0;
}

/* ---- transfer completion ------------------------------------------------ */

static void xec_i2c_v3_bm_finish(const struct device *ctrl)
{
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;

	xdat->state = BM_STATE_DONE;

#ifdef CONFIG_I2C_CALLBACK
	if (xdat->cb != NULL) {
		k_work_submit_to_queue(&xec_i2c_v3_bm_work_q, &xdat->kw);
		return;
	}
#endif
	k_sem_give(&xdat->sync);
}

/* A terminal STOP has just been written (and, for reads, the final byte
 * read out). The controller does not interrupt on STOP, so enable the IDLE
 * interrupt and wait: NBB is still 0 here (mid-STOP), which is the only
 * safe time to enable IDLE_IEN. Completion is signalled from the IDLE
 * interrupt once NBB goes 0->1. The stale IDLE latch was cleared at group
 * arm, so if the STOP completes before this enable the latch is simply
 * already set and enabling IDLE_IEN fires the pending interrupt -- no race,
 * no miss.
 */
static void xec_i2c_v3_bm_wait_idle(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;

	xdat->state = BM_STATE_STOP;
	sys_set_bit(cfg->base + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
}

/* NACK-style failure: record the error, STOP, and complete cleanly through
 * the IDLE interrupt (the STOP returns the bus to idle normally, so a
 * following transfer -- e.g. the next address of a bus scan -- sees an idle
 * bus and need not run recovery).
 */
static void xec_i2c_v3_bm_fail(const struct device *ctrl, int err)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;

	xdat->err = err;
	sys_write8(BM_CR_STOP, cfg->base + XEC_I2C_CR_OFS);
	xec_i2c_v3_bm_wait_idle(ctrl);
}

/* Abnormal termination (bus error / lost arbitration): the bus may not
 * return to a clean idle, so record the error, attempt a STOP, and signal
 * immediately without waiting for IDLE. The next transfer's SR!=IDLE check
 * runs bus recovery if needed.
 */
static void xec_i2c_v3_bm_error(const struct device *ctrl, int err)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;

	xdat->err = err;
	sys_write8(BM_CR_STOP, cfg->base + XEC_I2C_CR_OFS);
	xec_i2c_v3_bm_finish(ctrl);
}

/* Begin the next message via a repeated START (direction/RESTART change).
 * Per Table 6-8 the repeated-START command does not clock the Data
 * Register, so the address byte is loaded immediately afterwards.
 */
static void xec_i2c_v3_bm_restart_next(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;
	struct i2c_msg *m;

	xdat->msg_idx++;
	m = &xdat->msgs[xdat->msg_idx];
	xdat->buf = m->buf;
	xdat->blen = m->len;
	xdat->bpos = 0;
	xdat->dir_read = bm_msg_is_read(m);
	xdat->state = BM_STATE_START;

	sys_write8(BM_CR_RPT_START, base + XEC_I2C_CR_OFS);
	sys_write8(bm_addr_byte(xdat->addr, xdat->dir_read), base + XEC_I2C_DATA_OFS);
}

/* Arm the initial START for the STOP-delimited group beginning at the
 * current xdat->msg_idx. The port must already be applied and the bus
 * idle (guaranteed by the caller: the pre-transfer SR check for the first
 * group, or the IDLE interrupt that completed the previous group). Loads
 * the address+R/W and issues START, which clocks the address byte out
 * (Tables 6-6/6-7).
 */
static void bm_arm_group(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;
	struct i2c_msg *m = &xdat->msgs[xdat->msg_idx];

	xdat->buf = m->buf;
	xdat->blen = m->len;
	xdat->bpos = 0;
	xdat->rx_total = 0;
	xdat->rx_done = 0;
	xdat->dir_read = bm_msg_is_read(m);
	xdat->err = 0;
	xdat->state = BM_STATE_START;

	/* Clear any stale IDLE latch (e.g. from a prior error path that
	 * STOPped without waiting for IDLE) so it cannot fire the instant
	 * this group enables IDLE_IEN at its own STOP. IDLE_IEN itself stays
	 * off here -- NBB==1 now, and it must not be enabled until STOP.
	 */
	bm_cmpl_clear(base, BM_CMPL_IDLE);

	sys_write8(bm_addr_byte(xdat->addr, xdat->dir_read), base + XEC_I2C_DATA_OFS);
	sys_write8(BM_CR_START, base + XEC_I2C_CR_OFS);
}

/* Called from the WRITE ISR path when the current write buffer is drained.
 * Ends the group with a STOP, continues into a same-direction non-RESTART
 * message without a bus operation, or issues a repeated START. Skips
 * zero-length continuation messages so an empty mid-run buffer cannot cause
 * an out-of-bounds access or a spurious byte on the wire.
 */
static void bm_write_advance(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;

	for (;;) {
		if (bm_stop_here(xdat)) {
			sys_write8(BM_CR_STOP, base + XEC_I2C_CR_OFS);
			xec_i2c_v3_bm_wait_idle(ctrl);
			return;
		}

		struct i2c_msg *nx = &xdat->msgs[xdat->msg_idx + 1U];

		if ((nx->flags & I2C_MSG_RESTART) != 0U || bm_msg_is_read(nx)) {
			xec_i2c_v3_bm_restart_next(ctrl);
			return;
		}

		/* Same-direction continuation: advance to the next buffer. */
		xdat->msg_idx++;
		xdat->buf = nx->buf;
		xdat->blen = nx->len;
		xdat->bpos = 0;

		if (xdat->blen != 0U) {
			xdat->state = BM_STATE_WRITE;
			sys_write8(xdat->buf[0], base + XEC_I2C_DATA_OFS);
			return;
		}
		/* Zero-length continuation: re-evaluate from the new message. */
	}
}

#ifdef CONFIG_I2C_TARGET
/* ---- target (peripheral) mode ------------------------------------------- */

/* Index of a free Own-Address slot, or -1 when both are occupied. */
static int bm_tgt_find_free_slot(const struct xec_i2c_v3_bm_xdat *xdat)
{
	for (int i = 0; i < XEC_I2C_MAX_TARGETS; i++) {
		if (xdat->targets[i] == NULL) {
			return i;
		}
	}
	return -1;
}

/* OA slot whose registered address matches addr7, or -1. */
static int bm_tgt_find_slot_by_addr(const struct xec_i2c_v3_bm_xdat *xdat, uint8_t addr7)
{
	for (int i = 0; i < XEC_I2C_MAX_TARGETS; i++) {
		if (xdat->targets[i] != NULL &&
		    (xdat->targets[i]->address & XEC_I2C_TARGET_ADDR_MSK) == addr7) {
			return i;
		}
	}
	return -1;
}

/* OA slot holding this exact config pointer, or -1. */
static int bm_tgt_find_slot_by_cfg(const struct xec_i2c_v3_bm_xdat *xdat,
				   const struct i2c_target_config *tcfg)
{
	for (int i = 0; i < XEC_I2C_MAX_TARGETS; i++) {
		if (xdat->targets[i] == tcfg) {
			return i;
		}
	}
	return -1;
}

/* Reprogram both Own-Address slots from the registered targets. Empty slots
 * read back as address 0, which never matches a real transaction here.
 */
static void bm_tgt_program_oa(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uint32_t oa = 0;

	for (int i = 0; i < XEC_I2C_MAX_TARGETS; i++) {
		if (xdat->targets[i] != NULL) {
			oa |= XEC_I2C_OA_SET(i,
					     xdat->targets[i]->address & XEC_I2C_TARGET_ADDR_MSK);
		}
	}
	sys_write32(oa, cfg->base + XEC_I2C_OA_OFS);
}

/* Re-arm to auto-ACK the own address and interrupt on every byte (PIN). */
static inline void bm_tgt_restart(uintptr_t base)
{
	sys_write8(BM_CR_TGT_ARM, base + XEC_I2C_CR_OFS);
}

/* Clear ACK so the controller NACKs the next byte (write declined/overflow). */
static inline void bm_tgt_nack_next(uintptr_t base)
{
	sys_write8(BM_CR_TGT_NACK, base + XEC_I2C_CR_OFS);
}

/* Address-phase interrupt: the matched address byte is in DATA. Reading it
 * clears AAT/SAD and releases SCL; bit 0 carries the R/W direction.
 */
static void bm_tgt_addr(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;
	uint8_t rx = sys_read8(base + XEC_I2C_DATA_OFS);
	uint8_t addr7 = (uint8_t)((rx >> 1) & XEC_I2C_TARGET_ADDR_MSK);
	int slot = bm_tgt_find_slot_by_addr(xdat, addr7);
	struct i2c_target_config *tcfg = NULL;
	const struct i2c_target_callbacks *cbs = NULL;

	if (slot >= 0) {
		xdat->active_slot = (uint8_t)slot;
		tcfg = xdat->targets[slot];
		cbs = tcfg->callbacks;
	}

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	/* A repeated START after a write (combined write-then-read) reaches the
	 * address phase with staged RX bytes and no intervening STOP: deliver
	 * them before turning the bus around.
	 */
	if (!xdat->target_read && xdat->tgt_rx_pos > 0U && cbs != NULL &&
	    cbs->buf_write_received != NULL) {
		cbs->buf_write_received(tcfg, cfg->tgt_rx_buf, xdat->tgt_rx_pos);
	}
	xdat->tgt_rx_pos = 0U;
#endif

	if ((rx & BIT(BM_TGT_RW_POS)) != 0U) {
		/* Target transmitter: the external host reads from us. Preload
		 * the first byte; writing DATA releases SCL to clock it out.
		 */
		uint8_t val = BM_TGT_DFLT_DATA;

		xdat->target_read = true;
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
		xdat->tgt_tx_buf = NULL;
		xdat->tgt_tx_len = 0U;
		xdat->tgt_tx_pos = 0U;
		if (cbs != NULL && cbs->buf_read_requested != NULL) {
			uint8_t *p = NULL;
			uint32_t l = 0U;

			if (cbs->buf_read_requested(tcfg, &p, &l) == 0 && p != NULL) {
				xdat->tgt_tx_buf = p;
				xdat->tgt_tx_len = l;
			}
		}
		if (xdat->tgt_tx_buf != NULL && xdat->tgt_tx_pos < xdat->tgt_tx_len) {
			val = xdat->tgt_tx_buf[xdat->tgt_tx_pos++];
		}
#else
		if (cbs != NULL && cbs->read_requested != NULL) {
			(void)cbs->read_requested(tcfg, &val);
		}
#endif
		sys_write8(val, base + XEC_I2C_DATA_OFS);
	} else {
		/* Target receiver: the external host writes to us. Reading the
		 * address above already released SCL for the first data byte.
		 */
		xdat->target_read = false;
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
		xdat->tgt_rx_pos = 0U;
#else
		if (cbs == NULL || cbs->write_requested == NULL ||
		    cbs->write_requested(tcfg) != 0) {
			bm_tgt_nack_next(base);
		}
#endif
	}
}

/* Data-phase interrupt, target receiver: a byte from the external host is in
 * DATA. Reading it releases SCL for the next byte.
 */
static void bm_tgt_rx(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;
	uint8_t val = sys_read8(base + XEC_I2C_DATA_OFS);

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	if (xdat->tgt_rx_pos < cfg->tgt_rx_buf_size) {
		cfg->tgt_rx_buf[xdat->tgt_rx_pos++] = val;
	} else {
		bm_tgt_nack_next(base);
	}
#else
	struct i2c_target_config *tcfg = xdat->targets[xdat->active_slot];
	const struct i2c_target_callbacks *cbs = (tcfg != NULL) ? tcfg->callbacks : NULL;

	if (cbs == NULL || cbs->write_received == NULL || cbs->write_received(tcfg, val) != 0) {
		bm_tgt_nack_next(base);
	}
#endif
}

/* Data-phase interrupt, target transmitter: SR.LRB reflects the external
 * host's ACK/NACK of the byte we just sent. NACK => the host is done reading;
 * arm IDLE_IEN to catch the STOP and clock a dummy byte to release PIN.
 * Otherwise fetch and send the next byte.
 */
static void bm_tgt_tx(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;
	uint8_t val = BM_TGT_DFLT_DATA;

	if ((sys_read8(base + XEC_I2C_SR_OFS) & BM_SR_LRB) != 0U) {
		/* Host NACKed the final read byte and will now issue STOP. Clear
		 * any stale CMPL.IDLE latch before arming IDLE_IEN: IDLE latches
		 * on every STOP's NBB 0->1 edge regardless of IDLE_IEN, and the
		 * preceding write transaction's STO-branch RW1C can race that
		 * edge and leave IDLE set. If armed with a stale latch, the next
		 * interrupt takes the IDLE branch while NBB is still clear (bus
		 * busy), disarming IDLE_IEN before this transaction's real STOP
		 * and losing its stop callback. At this point the host has only
		 * NACKed (STOP not yet issued, NBB still 0), so any latch here is
		 * stale by definition -- clearing it cannot drop a real STOP.
		 */
		bm_cmpl_clear(base, BM_CMPL_IDLE);
		sys_set_bit(base + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
		sys_write8(BM_TGT_DFLT_DATA, base + XEC_I2C_DATA_OFS);
		return;
	}

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	if (xdat->tgt_tx_buf != NULL && xdat->tgt_tx_pos < xdat->tgt_tx_len) {
		val = xdat->tgt_tx_buf[xdat->tgt_tx_pos++];
	}
#else
	struct i2c_target_config *tcfg = xdat->targets[xdat->active_slot];
	const struct i2c_target_callbacks *cbs = (tcfg != NULL) ? tcfg->callbacks : NULL;

	if (cbs != NULL && cbs->read_processed != NULL) {
		(void)cbs->read_processed(tcfg, &val);
	}
#endif
	sys_write8(val, base + XEC_I2C_DATA_OFS);
}

/* Target-mode interrupt service. Dispatched from the top of the controller
 * ISR while a target is attached; master ops are -EBUSY then, so the
 * controller state machine is dormant and the two paths never interleave.
 */
static void xec_i2c_v3_bm_isr_target(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;
	uint8_t sr = sys_read8(base + XEC_I2C_SR_OFS);
	uint32_t cmpl = sys_read32(base + XEC_I2C_CMPL_OFS);
	uint32_t cfgr = sys_read32(base + XEC_I2C_CFG_OFS);
	struct i2c_target_config *tcfg = xdat->targets[xdat->active_slot];
	const struct i2c_target_callbacks *cbs = (tcfg != NULL) ? tcfg->callbacks : NULL;

	xec_i2c_bm_cap_update(xdat, 0xB0U);

	/* Target-transmit external STOP: after the host NACKs the final read
	 * byte and issues STOP, NBB goes 0->1 and CMPL.IDLE latches (IDLE_IEN
	 * was armed in bm_tgt_tx). Deliver the stop and re-arm.
	 */
	if ((cfgr & BM_CFG_IDLE_IEN) != 0U && (cmpl & BM_CMPL_IDLE) != 0U) {
		xec_i2c_bm_cap_update(xdat, 0xB1U);
		sys_clear_bit(base + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
		bm_cmpl_clear(base, BM_CMPL_IDLE);
		/* IDLE latches on NBB 0->1 after the host's STOP releases the
		 * lines. Only then is the read transaction truly over: deliver
		 * stop and re-arm. A latch without NBB set is spurious -- just
		 * clear it and let the transfer continue.
		 */
		if ((sr & BM_SR_NBB) != 0U) {
			xec_i2c_bm_cap_update(xdat, 0xB2U);
			if (cbs != NULL && cbs->stop != NULL) {
				xec_i2c_bm_cap_update(xdat, 0xB3U);
				cbs->stop(tcfg);
			}
			bm_tgt_restart(base);
		}
		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
		return;
	}

	/* External STOP after a write, or a bus error: end the transaction. */
	if ((sr & (BM_SR_BER | BM_SR_STO)) != 0U) {
		xec_i2c_bm_cap_update(xdat, 0xB4U);
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
		if (!xdat->target_read && xdat->tgt_rx_pos > 0U && cbs != NULL &&
		    cbs->buf_write_received != NULL) {
			xec_i2c_bm_cap_update(xdat, 0xB5U);
			cbs->buf_write_received(tcfg, cfg->tgt_rx_buf, xdat->tgt_rx_pos);
		}
		xdat->tgt_rx_pos = 0U;
#endif
		if (cbs != NULL && cbs->stop != NULL) {
			xec_i2c_bm_cap_update(xdat, 0xB6U);
			cbs->stop(tcfg);
		}
		bm_cmpl_clear(base, cmpl);
		bm_tgt_restart(base);
		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
		xec_i2c_bm_cap_update(xdat, 0xB7U);
		return;
	}

	/* Address phase: AAT set with PIN clear (byte complete, addressed). */
	if ((sr & (BM_SR_AAT | BM_SR_PIN)) == BM_SR_AAT) {
		xec_i2c_bm_cap_update(xdat, 0xB8U);
		bm_tgt_addr(ctrl);
		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
		return;
	}

	/* Data phase. */
	if (xdat->target_read) {
		xec_i2c_bm_cap_update(xdat, 0xB9U);
		bm_tgt_tx(ctrl);
	} else {
		xec_i2c_bm_cap_update(xdat, 0xBAU);
		bm_tgt_rx(ctrl);
	}
	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	xec_i2c_bm_cap_update(xdat, 0xBFU);
}
#endif /* CONFIG_I2C_TARGET */

/* ---- interrupt service routine ---- */

static void xec_i2c_v3_bm_isr(const struct device *ctrl_dev)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl_dev->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl_dev->data;
	uintptr_t base = cfg->base;
	uint8_t sr;

	xec_i2c_bm_cap_update(xdat, 0x80U);

#ifdef CONFIG_I2C_TARGET
	/* While a target is attached the controller runs as a peripheral: master
	 * entry points are -EBUSY, so the state machine below is dormant and this
	 * branch owns every interrupt.
	 */
	if (xdat->target_attached) {
		xec_i2c_v3_bm_isr_target(ctrl_dev);
		xec_i2c_bm_cap_update(xdat, 0x8FU);
		return;
	}
#endif
	/* Awaiting the post-STOP IDLE interrupt (the controller does not
	 * interrupt on STOP itself). CMPL.IDLE latches when NBB goes 0->1;
	 * on that edge, disable IDLE_IEN, clear the latch, and signal the
	 * transfer complete. Any other interrupt in this state is spurious.
	 */
	if (xdat->state == BM_STATE_STOP) {
		xec_i2c_bm_cap_update(xdat, 0x81U);
		if ((sys_read32(base + XEC_I2C_CMPL_OFS) & BM_CMPL_IDLE) != 0U) {
			xec_i2c_bm_cap_update(xdat, 0x82U);
			sys_clear_bit(base + XEC_I2C_CFG_OFS, XEC_I2C_CFG_IDLE_IEN_POS);
			bm_cmpl_clear(base, BM_CMPL_IDLE);
			xec_i2c_v3_bm_finish(ctrl_dev);
		}
		xec_i2c_bm_cap_update(xdat, 0x83U);
		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
		return;
	}

	/* No transfer in flight: quiesce and drop any spurious interrupt so a
	 * stray BER/LAB after completion cannot dispatch completion twice.
	 */
	if (xdat->state != BM_STATE_START && xdat->state != BM_STATE_WRITE &&
	    xdat->state != BM_STATE_READ) {
		xec_i2c_bm_cap_update(xdat, 0x84U);
		sys_write8(BM_CR_DFLT, base + XEC_I2C_CR_OFS);
		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
		return;
	}

	xec_i2c_bm_cap_update(xdat, 0x85U);
	sr = sys_read8(base + XEC_I2C_SR_OFS);

	/* Bus error / lost arbitration abort the transfer regardless of the
	 * current phase. BER requires a controller reset (deferred to the
	 * next transfer's recovery); LAB means another host won the bus.
	 */
	if ((sr & BM_SR_BER) != 0U) {
		xec_i2c_bm_cap_update(xdat, 0x86U);
		xec_i2c_v3_bm_error(ctrl_dev, -EIO);
		goto out;
	}
	if ((sr & BM_SR_LAB) != 0U) {
		xec_i2c_bm_cap_update(xdat, 0x87U);
		xec_i2c_v3_bm_error(ctrl_dev, -EAGAIN);
		goto out;
	}

	switch (xdat->state) {
	case BM_STATE_START:
		xec_i2c_bm_cap_update(xdat, 0x90U);
		/* Address phase complete. LRB carries the target's ACK of the
		 * address byte (0 = ACK, 1 = NACK).
		 */
		if ((sr & BM_SR_LRB) != 0U) {
			xec_i2c_bm_cap_update(xdat, 0x91U);
			/* Target did not ACK its address: no such device. */
			xec_i2c_v3_bm_fail(ctrl_dev, -ENXIO);
			break;
		}

		if (xdat->dir_read) {
			xec_i2c_bm_cap_update(xdat, 0x92U);
			xdat->rx_total = bm_run_read_total(xdat);
			xdat->rx_done = 0;

			if (xdat->rx_total == 0U) {
				xec_i2c_bm_cap_update(xdat, 0x93U);
				/* Zero-length read: nothing to clock. */
				if (bm_stop_here(xdat)) {
					xec_i2c_bm_cap_update(xdat, 0x94U);
					sys_write8(BM_CR_STOP, base + XEC_I2C_CR_OFS);
					xec_i2c_v3_bm_wait_idle(ctrl_dev);
				} else {
					xec_i2c_bm_cap_update(xdat, 0x95U);
					xec_i2c_v3_bm_restart_next(ctrl_dev);
				}
				break;
			}

			/* Read-ahead model (confirmed on v3.8 silicon): byte 0 is
			 * NOT auto-received after the address phase -- it is
			 * clocked in by the *first* I2C.DATA read. That first read
			 * returns the (dummy) address and starts clocking byte 0;
			 * every subsequent read returns the byte just received and
			 * clocks the next. rx_done therefore indexes the byte about
			 * to be returned, and the terminating NACK/STOP are timed
			 * one read ahead of the final byte (see BM_STATE_READ).
			 *
			 * A one-byte read must NACK its single byte, so ACK is
			 * cleared before the dummy read that clocks it.
			 */
			if (xdat->rx_total == 1U) {
				xec_i2c_bm_cap_update(xdat, 0x96U);
				sys_write8(BM_CR_NACK, base + XEC_I2C_CR_OFS);
			}
			xec_i2c_bm_cap_update(xdat, 0x97U);
			(void)sys_read8(base + XEC_I2C_DATA_OFS);
			xdat->state = BM_STATE_READ;
		} else {
			xec_i2c_bm_cap_update(xdat, 0x98U);
			if (xdat->blen == 0U) {
				xec_i2c_bm_cap_update(xdat, 0x99U);
				/* Address-only write (e.g. bus scan): no data to
				 * send, so resolve the group boundary directly.
				 */
				bm_write_advance(ctrl_dev);
				break;
			}
			xec_i2c_bm_cap_update(xdat, 0x9AU);
			xdat->bpos = 0;
			sys_write8(xdat->buf[0], base + XEC_I2C_DATA_OFS);
			xdat->state = BM_STATE_WRITE;
		}
		break;

	case BM_STATE_WRITE:
		xec_i2c_bm_cap_update(xdat, 0xA0U);
		/* Byte xdat->bpos has been clocked out; LRB is the target's
		 * ACK for it.
		 */
		if ((sr & BM_SR_LRB) != 0U) {
			xec_i2c_bm_cap_update(xdat, 0xA1U);
			/* Target NACKed a data byte: abort the write. */
			xec_i2c_v3_bm_fail(ctrl_dev, -ENXIO);
			break;
		}

		if ((xdat->bpos + 1U) < xdat->blen) {
			xec_i2c_bm_cap_update(xdat, 0xA2U);
			xdat->bpos++;
			sys_write8(xdat->buf[xdat->bpos], base + XEC_I2C_DATA_OFS);
			break;
		}

		/* Current buffer drained: end the group, continue into the
		 * next same-direction buffer, or issue a repeated START.
		 */
		xec_i2c_bm_cap_update(xdat, 0xA3U);
		bm_write_advance(ctrl_dev);
		break;

	case BM_STATE_READ:
		xec_i2c_bm_cap_update(xdat, 0xA6U);
		/* Byte rx_done of the read run is now in the shadow register.
		 * Reading the Data Register retrieves it and clocks the next
		 * byte (unless we STOP first). The last byte was NACKed by an
		 * earlier BM_CR_NACK, so the target has released SDA.
		 */
		if (xdat->rx_done == (xdat->rx_total - 1U)) {
			xec_i2c_bm_cap_update(xdat, 0xA7U);
			if (bm_stop_here(xdat)) {
				xec_i2c_bm_cap_update(xdat, 0xA8U);
				/* Table 6-7 steps 9-10: STOP first (so the read
				 * does not clock another byte), then read the
				 * final byte out, then await the IDLE interrupt.
				 */
				sys_write8(BM_CR_STOP, base + XEC_I2C_CR_OFS);
				xdat->buf[xdat->bpos] = (uint8_t)sys_read8(base + XEC_I2C_DATA_OFS);
				xec_i2c_v3_bm_wait_idle(ctrl_dev);
			} else {
				xec_i2c_bm_cap_update(xdat, 0xA9U);
				xdat->buf[xdat->bpos] = (uint8_t)sys_read8(base + XEC_I2C_DATA_OFS);
				xec_i2c_v3_bm_restart_next(ctrl_dev);
			}
			break;
		}

		/* Arm the terminating NACK before the read that clocks the
		 * run's final byte.
		 */
		xec_i2c_bm_cap_update(xdat, 0xAAU);
		if ((xdat->rx_done + 2U) == xdat->rx_total) {
			xec_i2c_bm_cap_update(xdat, 0xABU);
			sys_write8(BM_CR_NACK, base + XEC_I2C_CR_OFS);
		}

		xdat->buf[xdat->bpos] = (uint8_t)sys_read8(base + XEC_I2C_DATA_OFS);
		xdat->bpos++;
		xdat->rx_done++;

		/* Cross into the next buffer of the read run, skipping any
		 * zero-length messages. rx_done < rx_total guarantees a
		 * non-empty buffer remains in the run.
		 */
		if (xdat->bpos == xdat->blen) {
			do {
				xec_i2c_bm_cap_update(xdat, 0xACU);
				xdat->msg_idx++;
				xdat->buf = xdat->msgs[xdat->msg_idx].buf;
				xdat->blen = xdat->msgs[xdat->msg_idx].len;
			} while (xdat->blen == 0U);
			xdat->bpos = 0;
		}
		xec_i2c_bm_cap_update(xdat, 0xADU);
		break;

	default:
		xec_i2c_bm_cap_update(xdat, 0xAFU);
		/* Spurious interrupt outside a transfer: quiesce. */
		sys_write8(BM_CR_DFLT, base + XEC_I2C_CR_OFS);
		break;
	}

out:
	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	xec_i2c_bm_cap_update(xdat, 0x8FU);
}

/* ---- transfer entry ----------------------------------------------------- */

/* Prepare and kick a transfer. Caller holds the controller lock. cb == NULL
 * selects the synchronous path (ISR gives data->sync); a non-NULL cb selects
 * the asynchronous path (ISR submits data->kw to the work queue).
 */
static int xec_i2c_v3_bm_start(const struct device *port_dev, struct i2c_msg *msgs,
			       uint8_t num_msgs, uint16_t addr, i2c_callback_t cb, void *userdata)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uintptr_t base = cfg->base;
	uint32_t freq;
	int rc;

	xec_i2c_bm_cap_update(xdat, 1U);

	rc = xec_i2c_v3_bm_apply_port(port_dev);
	if (rc != 0) {
		return rc;
	}

	/* If the bus/controller is not cleanly idle, recover before issuing
	 * a START.
	 */
	if (sys_read8(base + XEC_I2C_SR_OFS) != BM_SR_IDLE) {
		xec_i2c_bm_cap_update(xdat, 2U);
		freq = (pc->bitrate != 0U) ? pc->bitrate : cfg->dflt_freq;
		rc = xec_i2c_v3_bm_bus_recover(ctrl, freq, pc->port_id);
		if (rc != 0) {
			xec_i2c_bm_cap_update(xdat, 3U);
			return rc;
		}
	}

	xec_i2c_bm_cap_update(xdat, 4U);

	xdat->msgs = msgs;
	xdat->num_msgs = num_msgs;
	xdat->msg_idx = 0;
	xdat->addr = addr;
#ifdef CONFIG_I2C_CALLBACK
	xdat->cb = cb;
	xdat->cb_user_data = userdata;
	xdat->cb_dev = port_dev;
#else
	ARG_UNUSED(cb);
	ARG_UNUSED(userdata);
#endif

	if (cb == NULL) {
		xec_i2c_bm_cap_update(xdat, 5U);
		k_sem_reset(&xdat->sync);
	}

	/* Arm the first STOP-delimited group. Subsequent groups (if the
	 * message array contains intermediate STOPs) are armed on completion
	 * by the sync loop or the async work handler.
	 */
	bm_arm_group(ctrl);

	xec_i2c_bm_cap_update(xdat, 6U);

	return 0;
}

/* Validate transfer arguments: 7-bit addressing only, no NULL data. */
static int bm_validate(const struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
	if ((addr & ~XEC_I2C_TARGET_ADDR_MSK) != 0U) {
		return -EINVAL;
	}
	for (uint8_t i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_ADDR_10_BITS) != 0U) {
			return -ENOTSUP;
		}
		if (msgs[i].len != 0U && msgs[i].buf == NULL) {
			return -EINVAL;
		}
	}
	return 0;
}

static int xec_i2c_v3_bm_vport_transfer(const struct device *port_dev, struct i2c_msg *msgs,
					uint8_t num_msgs, uint16_t address)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	int rc;

	if (num_msgs == 0U) {
		return 0;
	}
	if (msgs == NULL) {
		return -EINVAL;
	}
	rc = bm_validate(msgs, num_msgs, address);
	if (rc != 0) {
		return rc;
	}

	k_sem_take(&xdat->lock, K_FOREVER);

#ifdef CONFIG_I2C_TARGET
	if (xdat->target_attached) {
		k_sem_give(&xdat->lock);
		return -EBUSY;
	}
#endif
	xec_i2c_bm_cap_init(xdat);

	rc = xec_i2c_v3_bm_start(port_dev, msgs, num_msgs, address, NULL, NULL);

	/* One iteration per STOP-delimited group: wait for the group to
	 * complete, then arm the next group (if any) on the now-idle bus.
	 */
	while (rc == 0) {
		xec_i2c_bm_cap_update(xdat, 8U);
		rc = k_sem_take(&xdat->sync, BM_TIMEOUT);
		if (rc != 0) {
			xec_i2c_bm_cap_update(xdat, 9U);
			LOG_ERR("i2c xfer timeout (%s)", port_dev->name);
			xec_i2c_v3_bm_abort(ctrl);
			rc = -ETIMEDOUT;
			break;
		}
		xec_i2c_bm_cap_update(xdat, 10U);
		rc = xdat->err;
		if (rc != 0 || (xdat->msg_idx + 1U) >= num_msgs) {
			xec_i2c_bm_cap_update(xdat, 11U);
			break;
		}
		/* The prior group completed via its IDLE interrupt, so the bus
		 * is idle (NBB=1) and the next group can START immediately.
		 */
		xdat->msg_idx++;
		bm_arm_group(ctrl);
	}

	xec_i2c_bm_cap_update(xdat, 13U);
	k_sem_give(&xdat->lock);
	return rc;
}

/* Asynchronous transfer. Returns immediately after arming the first group;
 * completion (and any group chaining) is driven from the ISR via the work
 * queue, which invokes cb and releases the controller lock.
 *
 * A per-group watchdog (timeout_dwork, BM_TIMEOUT) guards against a wedged
 * bus or lost interrupt: if a group does not complete in time the watchdog
 * resets the controller and delivers -ETIMEDOUT. The watchdog is armed here
 * (before the START, so an immediate completion still finds it scheduled and
 * cancels it), refreshed per group by the work handler, and canceled on
 * completion by async_deliver.
 */
#ifdef CONFIG_I2C_CALLBACK
static int xec_i2c_v3_bm_vport_transfer_cb(const struct device *port_dev, struct i2c_msg *msgs,
					   uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
					   void *userdata)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	int rc;

	if (cb == NULL) {
		return xec_i2c_v3_bm_vport_transfer(port_dev, msgs, num_msgs, addr);
	}
	if (num_msgs == 0U) {
		cb(port_dev, 0, userdata);
		return 0;
	}
	if (msgs == NULL) {
		return -EINVAL;
	}
	rc = bm_validate(msgs, num_msgs, addr);
	if (rc != 0) {
		return rc;
	}

	k_sem_take(&xdat->lock, K_FOREVER);

#ifdef CONFIG_I2C_TARGET
	if (xdat->target_attached) {
		k_sem_give(&xdat->lock);
		return -EBUSY;
	}
#endif

	/* Open the completion claim and arm the watchdog BEFORE issuing the
	 * START inside start(): if the transfer completes immediately, the
	 * work handler's async_deliver then finds a scheduled watchdog to
	 * cancel rather than racing an as-yet-unscheduled one.
	 */
	atomic_set(&xdat->async_done, 0);
	k_work_reschedule_for_queue(&xec_i2c_v3_bm_work_q, &xdat->timeout_dwork, BM_TIMEOUT);

	rc = xec_i2c_v3_bm_start(port_dev, msgs, num_msgs, addr, cb, userdata);
	if (rc != 0) {
		(void)k_work_cancel_delayable(&xdat->timeout_dwork);
		atomic_set(&xdat->async_done, 1);
		k_sem_give(&xdat->lock);
		return rc;
	}

	/* Completion is delivered from the work queue (or the watchdog), which
	 * releases the lock and invokes cb.
	 */
	return 0;
}
#endif /* CONFIG_I2C_CALLBACK */

static int xec_i2c_v3_bm_vport_configure(const struct device *port_dev, uint32_t dev_config)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uint32_t freq;
	int rc = 0;

	if ((dev_config & I2C_MODE_CONTROLLER) == 0U) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		freq = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		freq = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		freq = MHZ(1);
		break;
	case I2C_SPEED_DT:
		freq = cfg->dflt_freq;
		break;
	default:
		return -ENOTSUP;
	}

	k_sem_take(&xdat->lock, K_FOREVER);
#ifdef CONFIG_I2C_TARGET
	if (xdat->target_attached) {
		k_sem_give(&xdat->lock);
		return -EBUSY;
	}
#endif
	if (freq != xdat->active_freq || xdat->active_port != pc->port_id) {
		rc = pinctrl_apply_state(pc->pcfg, PINCTRL_STATE_DEFAULT);
		if (rc == 0) {
			rc = xec_i2c_v3_bm_program_ctrl(ctrl, freq, pc->port_id);
		}
	}
	k_sem_give(&xdat->lock);
	return rc;
}

static int xec_i2c_v3_bm_vport_get_config(const struct device *port_dev, uint32_t *dev_config)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	struct xec_i2c_v3_bm_xdat *xdat = pc->parent->data;
	uint32_t speed;

	if (dev_config == NULL) {
		return -EINVAL;
	}

	if (xdat->active_freq <= KHZ(100)) {
		speed = I2C_SPEED_STANDARD;
	} else if (xdat->active_freq <= KHZ(400)) {
		speed = I2C_SPEED_FAST;
	} else {
		speed = I2C_SPEED_FAST_PLUS;
	}

	*dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(speed);
	return 0;
}

static int xec_i2c_v3_bm_vport_recover_bus(const struct device *port_dev)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	uint32_t freq = (xdat->active_freq != 0U) ? xdat->active_freq : cfg->dflt_freq;
	int rc;

	k_sem_take(&xdat->lock, K_FOREVER);
#ifdef CONFIG_I2C_TARGET
	if (xdat->target_attached) {
		k_sem_give(&xdat->lock);
		return -EBUSY;
	}
#endif
	rc = xec_i2c_v3_bm_apply_port(port_dev);
	if (rc == 0) {
		rc = xec_i2c_v3_bm_bus_recover(ctrl, freq, pc->port_id);
	}
	k_sem_give(&xdat->lock);
	return rc;
}

#ifdef CONFIG_I2C_TARGET
/* Register a target (peripheral) on this port. The first target switches the
 * controller onto the port and locks it: while any target is attached the
 * master entry points return -EBUSY and runtime port switching is frozen.
 * A second target may share the port (two OA slots); a target on a different
 * port of the same controller is rejected until all targets are unregistered.
 */
static int xec_i2c_v3_bm_target_register(const struct device *port_dev,
					 struct i2c_target_config *tcfg)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	int slot;
	int rc = 0;

	if (tcfg == NULL || tcfg->callbacks == NULL) {
		return -EINVAL;
	}
	if ((tcfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) != 0U) {
		return -ENOTSUP;
	}
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	if (tcfg->callbacks->buf_write_received == NULL ||
	    tcfg->callbacks->buf_read_requested == NULL) {
		return -EINVAL;
	}
	if (cfg->tgt_rx_buf == NULL || cfg->tgt_rx_buf_size == 0U) {
		return -ENOMEM;
	}
#endif

	k_sem_take(&xdat->lock, K_FOREVER);

	if (xdat->target_attached && xdat->target_port != port_dev) {
		rc = -EBUSY;
		goto out;
	}
	if (bm_tgt_find_slot_by_cfg(xdat, tcfg) >= 0) {
		rc = -EALREADY;
		goto out;
	}
	if (bm_tgt_find_slot_by_addr(xdat, tcfg->address & XEC_I2C_TARGET_ADDR_MSK) >= 0) {
		rc = -EADDRINUSE;
		goto out;
	}
	slot = bm_tgt_find_free_slot(xdat);
	if (slot < 0) {
		rc = -EBUSY;
		goto out;
	}

	xdat->targets[slot] = tcfg;

	if (!xdat->target_attached) {
		/* First target: switch onto the port (PCR reset clears OA),
		 * program the own address, and arm target mode.
		 */
		rc = xec_i2c_v3_bm_apply_port(port_dev);
		if (rc != 0) {
			xdat->targets[slot] = NULL;
			goto out;
		}
		xdat->active_slot = (uint8_t)slot;
		xdat->target_read = false;
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
		xdat->tgt_rx_pos = 0U;
#endif
		bm_tgt_program_oa(ctrl);
		bm_tgt_restart(cfg->base);
		xdat->target_port = port_dev;
		xdat->target_attached = true;
	} else {
		/* Second slot on the same port: update OA atomically w.r.t. the
		 * target ISR.
		 */
		unsigned int key = irq_lock();

		bm_tgt_program_oa(ctrl);
		irq_unlock(key);
	}

out:
	k_sem_give(&xdat->lock);
	return rc;
}

/* Unregister a target. When the last target on the controller is removed the
 * driver disarms target interrupts (CR.ENI off, OA cleared) and reverts to
 * controller mode, re-allowing master transfers and runtime port switching.
 * The GIRQ stays enabled -- master transfers re-arm CR.ENI per transfer.
 */
static int xec_i2c_v3_bm_target_unregister(const struct device *port_dev,
					   struct i2c_target_config *tcfg)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;
	const struct device *ctrl = pc->parent;
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	unsigned int key;
	int remaining = 0;
	int slot;
	int rc = 0;

	k_sem_take(&xdat->lock, K_FOREVER);

	slot = bm_tgt_find_slot_by_cfg(xdat, tcfg);
	if (slot < 0) {
		rc = -EINVAL;
		goto out;
	}

	xdat->targets[slot] = NULL;
	for (int i = 0; i < XEC_I2C_MAX_TARGETS; i++) {
		if (xdat->targets[i] != NULL) {
			remaining++;
		}
	}

	key = irq_lock();
	if (remaining == 0) {
		sys_write8(BM_CR_DFLT, cfg->base + XEC_I2C_CR_OFS);
		sys_write32(0U, cfg->base + XEC_I2C_OA_OFS);
		xdat->target_attached = false;
		xdat->target_port = NULL;
	} else {
		bm_tgt_program_oa(ctrl);
	}
	irq_unlock(key);

out:
	k_sem_give(&xdat->lock);
	return rc;
}
#endif /* CONFIG_I2C_TARGET */

/* ---- work queue: async completion dispatch ------------------------------ */

#ifdef CONFIG_I2C_CALLBACK
/* Deliver the async result. The CALLER must already have won the async_done
 * claim, which guarantees this runs exactly once per transfer. Cancels the
 * watchdog, releases the controller lock, and invokes the user callback on the
 * work-queue thread (never interrupt context). cb/user-data/dev are captured
 * before the lock is dropped, since a new transfer may reuse those fields
 * immediately.
 */
static void xec_i2c_v3_bm_async_deliver(const struct device *ctrl, int result)
{
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	i2c_callback_t cb = xdat->cb;
	void *ud = xdat->cb_user_data;
	const struct device *dev = xdat->cb_dev;

	(void)k_work_cancel_delayable(&xdat->timeout_dwork);

	k_sem_give(&xdat->lock);

	if (cb != NULL) {
		cb(dev, result, ud);
	}
}

/* Async completion watchdog. Fires BM_TIMEOUT after a group was armed if the
 * transfer has not completed. Claims delivery FIRST (so the completion work
 * item can never also deliver), then decides the outcome: if a completion
 * actually landed (state == DONE and it was the last group) report its real
 * result; otherwise the bus/controller is wedged (or an interrupt was lost)
 * -- reset it and report -ETIMEDOUT.
 */
static void xec_i2c_v3_bm_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct xec_i2c_v3_bm_xdat *xdat =
		CONTAINER_OF(dwork, struct xec_i2c_v3_bm_xdat, timeout_dwork);
	const struct device *ctrl = xdat->ctrl_dev;
	int result = -ETIMEDOUT;
	unsigned int key;
	bool completed;

	if (!atomic_cas(&xdat->async_done, 0, 1)) {
		return; /* the completion path already delivered this transfer */
	}

	/* "Completed" only if the WHOLE transfer finished, not merely an
	 * intermediate group: a delayed work queue could let this fire after
	 * a mid-transfer group's IDLE set state=DONE but before the next group
	 * was armed. Treat that as a genuine timeout (later groups never ran).
	 * (A completion landing in the tiny window after this snapshot is
	 * reported as -ETIMEDOUT -- acceptable, as it is past the deadline.)
	 */
	key = irq_lock();
	completed = (xdat->state == BM_STATE_DONE) &&
		    (xdat->err != 0 || (xdat->msg_idx + 1U) >= xdat->num_msgs);
	irq_unlock(key);

	if (completed) {
		result = xdat->err;
	} else {
		LOG_ERR("i2c async xfer timeout (%s)", ctrl->name);
		xec_i2c_v3_bm_abort(ctrl);
	}

	xec_i2c_v3_bm_async_deliver(ctrl, result);
}

static void xec_i2c_v3_bm_work_handler(struct k_work *work)
{
	struct xec_i2c_v3_bm_xdat *const xdat = CONTAINER_OF(work, struct xec_i2c_v3_bm_xdat, kw);
	const struct device *ctrl = xdat->ctrl_dev;

	/* The watchdog already delivered this transfer (and released the lock):
	 * do not touch hardware, the lock, or the watchdog. This handler and the
	 * watchdog handler are serialized on the one work-queue thread, so the
	 * claim read here cannot change under us.
	 */
	if (atomic_get(&xdat->async_done) != 0) {
		return;
	}

	/* If the completed group succeeded and more STOP-delimited groups
	 * remain, arm the next one on the now-idle bus (the prior group
	 * completed via its IDLE interrupt, so NBB=1) and refresh the
	 * watchdog for the new group; its completion re-enters this handler.
	 * The controller lock stays held throughout.
	 */
	if (xdat->err == 0 && (xdat->msg_idx + 1U) < xdat->num_msgs) {
		xdat->msg_idx++;
		bm_arm_group(ctrl);
		k_work_reschedule_for_queue(&xec_i2c_v3_bm_work_q, &xdat->timeout_dwork,
					    BM_TIMEOUT);
		return;
	}

	/* Final group: claim delivery. If the watchdog beat us (it fired and
	 * claimed between our top-of-handler check and here -- impossible while
	 * serialized, but the CAS keeps it correct regardless), do nothing.
	 */
	if (atomic_cas(&xdat->async_done, 0, 1)) {
		xec_i2c_v3_bm_async_deliver(ctrl, xdat->err);
	}
}
#endif /* CONFIG_I2C_CALLBACK */

/* ---- device init ---- */

static int xec_i2c_v3_bm_ctrl_init(const struct device *ctrl)
{
	const struct xec_i2c_v3_bm_xcfg *cfg = ctrl->config;
	struct xec_i2c_v3_bm_xdat *xdat = ctrl->data;
	int rc;

	xdat->ctrl_dev = ctrl;
	xdat->state = BM_STATE_IDLE;
	xdat->active_port = BM_INVALID_PORT;
	xdat->active_freq = 0;

#ifdef CONFIG_I2C_TARGET
	for (int i = 0; i < XEC_I2C_MAX_TARGETS; i++) {
		xdat->targets[i] = NULL;
	}
	xdat->target_attached = false;
	xdat->target_port = NULL;
#endif

	k_sem_init(&xdat->lock, 1, 1);
	k_sem_init(&xdat->sync, 0, 1);
#ifdef CONFIG_I2C_CALLBACK
	k_work_init(&xdat->kw, xec_i2c_v3_bm_work_handler);
	k_work_init_delayable(&xdat->timeout_dwork, xec_i2c_v3_bm_timeout_handler);
	atomic_set(&xdat->async_done, 1); /* no async transfer in flight yet */

	/* The work queue exists only for the async completion/watchdog path. */
	if (!xec_i2c_v3_bm_wq_started) {
		k_work_queue_start(&xec_i2c_v3_bm_work_q, xec_i2c_v3_bm_q_stack,
				   K_THREAD_STACK_SIZEOF(xec_i2c_v3_bm_q_stack),
				   K_PRIO_PREEMPT(CONFIG_I2C_MCHP_XEC_V3_BM_KWQ_PRIORITY), NULL);
		xec_i2c_v3_bm_wq_started = true;
	}
#endif

	rc = xec_i2c_v3_bm_program_ctrl(ctrl, cfg->dflt_freq, 0);
	if (rc != 0) {
		return rc;
	}

	/* Force pinctrl to be applied on the first transfer's port. */
	xdat->active_port = BM_INVALID_PORT;

	if (cfg->irq_connect != NULL) {
		cfg->irq_connect();
	}

	return 0;
}

static int xec_i2c_v3_bm_port_init(const struct device *port_dev)
{
	const struct xec_i2c_v3_bm_port_xcfg *pc = port_dev->config;

	if (!device_is_ready(pc->parent)) {
		return -ENODEV;
	}

	if (pc->is_default) {
		return xec_i2c_v3_bm_apply_port(port_dev);
	}

	return 0;
}

static DEVICE_API(i2c, xec_i2c_v3_bm_port_api) = {
	.configure = xec_i2c_v3_bm_vport_configure,
	.get_config = xec_i2c_v3_bm_vport_get_config,
	.transfer = xec_i2c_v3_bm_vport_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = xec_i2c_v3_bm_vport_transfer_cb,
#endif
	.recover_bus = xec_i2c_v3_bm_vport_recover_bus,
#ifdef CONFIG_I2C_TARGET
	.target_register = xec_i2c_v3_bm_target_register,
	.target_unregister = xec_i2c_v3_bm_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* core (controller) device */
#define DT_DRV_COMPAT microchip_xec_i2c_v3

#define XEC_I2C_V3_DFLT_FREQ(inst) I2C_BITRATE_STANDARD

#define XEC_I2C_V3_GIRQ(inst, idx)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, idx))
#define XEC_I2C_V3_GIRQ_POS(inst, idx) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, idx))

/* Buffer-mode target RX staging: one static per-controller buffer sized from
 * CONFIG_I2C_MCHP_XEC_V3_BM_TARGET_BUFFER_SIZE. Compiled out entirely unless
 * CONFIG_I2C_TARGET_BUFFER_MODE.
 */
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
#define XEC_I2C_V3_BM_TGT_BUF_DEF(inst)                                                            \
	static uint8_t                                                                             \
		xec_i2c_v3_bm_tgt_rx_buf_##inst[CONFIG_I2C_MCHP_XEC_V3_BM_TARGET_BUFFER_SIZE];
#define XEC_I2C_V3_BM_TGT_BUF_INIT(inst)                                                           \
	.tgt_rx_buf = xec_i2c_v3_bm_tgt_rx_buf_##inst,                                             \
	.tgt_rx_buf_size = CONFIG_I2C_MCHP_XEC_V3_BM_TARGET_BUFFER_SIZE,
#else
#define XEC_I2C_V3_BM_TGT_BUF_DEF(inst)
#define XEC_I2C_V3_BM_TGT_BUF_INIT(inst)
#endif

/* Build-time guard: if the controller names a default-port, that port's
 * "controller" phandle must point back to this controller node. Expands to
 * nothing for controllers that omit default-port.
 */
#define XEC_I2C_V3_BM_DEFPORT_ASSERT(inst)                                                         \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, default_port),                                      \
		   (BUILD_ASSERT(DT_SAME_NODE(DT_DRV_INST(inst),                                   \
			DT_PHANDLE(DT_INST_PHANDLE(inst, default_port), controller)),              \
			"default-port must reference a port on this controller");))

#define XEC_I2C_V3_BM_CTRL_INIT(inst)                                                              \
	XEC_I2C_V3_BM_DEFPORT_ASSERT(inst)                                                         \
	static void xec_i2c_v3_bm_irq_connect_##inst(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), xec_i2c_v3_bm_isr,    \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	XEC_I2C_V3_BM_TGT_BUF_DEF(inst)                                                            \
	static const struct xec_i2c_v3_bm_xcfg xec_i2c_v3_bm_xcfg_##inst = {                       \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.irq_connect = xec_i2c_v3_bm_irq_connect_##inst,                                   \
		.dflt_freq = XEC_I2C_V3_DFLT_FREQ(inst),                                           \
		.girq = XEC_I2C_V3_GIRQ(inst, 0),                                                  \
		.girq_pos = XEC_I2C_V3_GIRQ_POS(inst, 0),                                          \
		.enc_pcr = DT_INST_PROP(inst, pcr_scr),                                            \
		XEC_I2C_V3_BM_TGT_BUF_INIT(inst)};                                                 \
	static struct xec_i2c_v3_bm_xdat xec_i2c_v3_bm_xdat_##inst;                                \
	DEVICE_DT_INST_DEFINE(inst, xec_i2c_v3_bm_ctrl_init, NULL, &xec_i2c_v3_bm_xdat_##inst,     \
			      &xec_i2c_v3_bm_xcfg_##inst, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(XEC_I2C_V3_BM_CTRL_INIT)

/* virtual port device */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_xec_i2c_v3_port

/* A port is the boot/default routing if and only if its controller's default-port
 * phandle names this port node. Controllers without a default-port yield 0
 * (no boot pre-routing; the first transfer applies its own port lazily).
 */
#define XEC_I2C_V3_BM_PORT_IS_DEFAULT(inst)                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_PHANDLE(inst, controller), default_port),             \
		    (DT_SAME_NODE(DT_DRV_INST(inst),                                               \
				  DT_PHANDLE(DT_INST_PHANDLE(inst, controller), default_port))),   \
		    (0))

#define XEC_I2C_V3_BM_PORT_INIT(inst)                                                              \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct xec_i2c_v3_bm_port_xcfg xec_i2c_v3_bm_port_xcfg_##inst = {             \
		.parent = DEVICE_DT_GET(DT_INST_PHANDLE(inst, controller)),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.bitrate = DT_INST_PROP_OR(inst, clock_frequency, I2C_BITRATE_STANDARD),           \
		.port_id = (uint8_t)(DT_INST_PROP(inst, port) & 0x0FU),                            \
		.is_default = XEC_I2C_V3_BM_PORT_IS_DEFAULT(inst),                                 \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(inst, xec_i2c_v3_bm_port_init, NULL, NULL,                       \
				  &xec_i2c_v3_bm_port_xcfg_##inst, POST_KERNEL,                    \
				  CONFIG_I2C_INIT_PRIORITY,                    \
				  &xec_i2c_v3_bm_port_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_I2C_V3_BM_PORT_INIT)
