/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MSPI driver for the Microchip MEC175x QMSPI controller.
 *
 * Supports half-duplex, controller-mode SDR transfers in single, dual,
 * and quad I/O modes (including 1-1-N and 1-N-N variants). Data phase
 * runs through either PIO (descriptors + 8-byte FIFOs) or QMSPI local
 * DMA (three dedicated channels, length-override enabled).
 */

#define DT_DRV_COMPAT microchip_xec_qmspi_mspi_controller

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>

LOG_MODULE_REGISTER(mspi_mchp_xec_qmspi, CONFIG_MSPI_LOG_LEVEL);

enum qmspi_data_phase_ifm {
	IFM_SINGLE = XEC_QSPI_CR_IFM_FD,
	IFM_DUAL = XEC_QSPI_CR_IFM_DUAL,
	IFM_QUAD = XEC_QSPI_CR_IFM_QUAD,
};

struct mspi_xec_xcfg {
	mm_reg_t regs;
	void (*irq_cfg_func)(void);
	const struct pinctrl_dev_config *pincfg;
	struct mspi_cfg mspicfg;
	uint32_t flags;
	uint16_t girq_enc;
	uint16_t pcr_enc;
};

struct xec_ca_buf {
	union {
		uint32_t w;
		uint16_t hw[2];
		uint8_t b[4];
	};
};

struct mspi_xec_context {
	struct mspi_xfer xfer;
	uint32_t pkt_idx;
	int packets_left;
	int err;
	struct xec_ca_buf cmdbuf;
	struct xec_ca_buf adrbuf;
	uint32_t pio_pos;
#ifdef CONFIG_MULTITHREADING
	struct k_sem lock;
#else
	bool lock;
#endif
	uint32_t descrs[16];
};

struct qmspi_io_decode {
	uint8_t cmd_ifm;
	/* IFM value for the address phase */
	uint8_t addr_ifm;
	/* IFM value for the data phase */
	uint8_t data_ifm;
	/* reserved */
	uint8_t rsvd_ifm;
};

/* struct mspi_dev_cfg members we don't need
 * enum mspi_data_rate data_rate  HW is MSPI_DATE_RATE_SINGLE (can't do DDR)
 * enum mspi_endian endian        HW is MSPI_XFER_BIG_ENDIAN only
 * enum mspi_ce_polarity ce_polarity We can change polarity but it requires changing
 *                                   it in device tree for CE pin(s).
 *                                   Driver will not support this.
 *                                   Always is MSPI_CE_ACTIVE_LOW
 * bool dqs_enable HW does not support DQS operations
 */
struct mspi_xec_dev_cfg {
	uint32_t read_cmd;
	uint32_t write_cmd;
	uint32_t mem_boundary;
	uint32_t time_to_break;
	struct qmspi_io_decode iom;
	uint16_t freq_div;
	uint16_t rx_dummy;
	uint16_t tx_dummy;
	uint8_t cpp;
	uint8_t cmd_len;
	uint8_t adr_len;
	uint8_t ce_num;
	bool valid;
};

/* QMSPI hardware exposes two HW chip selects (CS0, CS1). The driver caches one
 * mspi_xec_dev_cfg per slot, indexed by dev_id->dev_idx, so partial dev_config
 * updates (and dev_id-only reselection via MSPI_DEVICE_CONFIG_NONE) restore
 * the right HW state when the application switches devices.
 */
#define XEC_QSPI_MAX_PERIPH 2

struct mspi_xec_xdat {
	const struct device *ctrl;
	const struct mspi_dev_id *dev_id;
	struct mspi_xec_dev_cfg dev_cfgs[XEC_QSPI_MAX_PERIPH];
	struct mspi_xec_context ctx;
	uint32_t max_cfg_freq;
	uint8_t num_periph;
	bool ctrl_cfg_ok;
#ifdef CONFIG_MULTITHREADING
	struct k_sem lock;
	struct k_sem xfer_done;
	struct k_timer async_timer;
	struct k_work async_pkt_work;
	struct k_work async_tmout_work;
	mspi_callback_handler_t cbs[MSPI_BUS_EVENT_MAX];
	struct mspi_callback_context *cbctxs[MSPI_BUS_EVENT_MAX];
#else
	volatile bool xfer_done;
	bool lock;
#endif
	struct mspi_cfg active_mspicfg;
};

/* Return the cached dev_cfg slot for the currently-selected device, or NULL
 * if no device has been selected or the slot index is out of range.
 */
static inline struct mspi_xec_dev_cfg *current_dev_cfg(struct mspi_xec_xdat *xdat)
{
	if (xdat->dev_id == NULL) {
		return NULL;
	}
	if (xdat->dev_id->dev_idx >= XEC_QSPI_MAX_PERIPH) {
		return NULL;
	}
	return &xdat->dev_cfgs[xdat->dev_id->dev_idx];
}

/* -------- Internal -------- */

/* Register helpers ---------------------------------------------------------- */

#define QR8(cfg, ofs)     sys_read8((cfg)->regs + (ofs))
#define QW8(cfg, ofs, v)  sys_write8((v), (cfg)->regs + (ofs))
#define QR16(cfg, ofs)    sys_read16((cfg)->regs + (ofs))
#define QW16(cfg, ofs, v) sys_write16((v), (cfg)->regs + (ofs))
#define QR32(cfg, ofs)    sys_read32((cfg)->regs + (ofs))
#define QW32(cfg, ofs, v) sys_write32((v), (cfg)->regs + (ofs))
#define QRMW32(cfg, ofs, msk, v)                                                                   \
	sys_write32((sys_read32((cfg)->regs + (ofs)) & ~(msk)) | ((v) & (msk)), (cfg)->regs + (ofs))
#define QTB32(cfg, ofs, bitpos) sys_test_bit((cfg)->regs + (ofs), (bitpos))
#define QCB32(cfg, ofs, bitpos) sys_clear_bit((cfg)->regs + (ofs), (bitpos))
#define QSB32(cfg, ofs, bitpos) sys_set_bit((cfg)->regs + (ofs), (bitpos))

/* 15-bit NQUNITS limit per descriptor */
#define QMSPI_NQUNITS_MAX 0x7FFFu

#define QMSPI_SR_ERR_MSK                                                                           \
	(BIT(XEC_QSPI_SR_TXB_ERR_POS) | BIT(XEC_QSPI_SR_RXB_ERR_POS) |                             \
	 BIT(XEC_QSPI_SR_PROG_ERR_POS) | BIT(XEC_QSPI_SR_LDMA_RX_ERR_POS) |                        \
	 BIT(XEC_QSPI_SR_LDMA_TX_ERR_POS))

/* Clock divider: 1..65535, with 0 meaning divide by 65536. */
static uint16_t calc_ckdiv(uint32_t src_hz, uint32_t dest_hz)
{
	uint32_t ckdiv;

	if (dest_hz == 0 || dest_hz >= src_hz) {
		return 1u;
	}

	ckdiv = DIV_ROUND_UP(src_hz, dest_hz);
	if (ckdiv >= 0x10000u) {
		return 0u;
	}

	return (uint16_t)ckdiv;
}

static int qmspi_hw_reset_and_init(const struct device *dev, uint32_t freq_hz)
{
	const struct mspi_xec_xcfg *xcfg = dev->config;
	uint32_t ckdiv = 0, mode_fdiv = 0;

	/* Ungate clocks via PCR (clock required register). Drop SLP bit so it
	 * runs while active.
	 */
	soc_xec_pcr_sleep_en_clear((uint8_t)xcfg->pcr_enc);

	/* Soft reset — self-clearing. Note: clk divider set to 0 (max) */
	QW32(xcfg, XEC_QSPI_MODE_OFS, BIT(XEC_QSPI_MODE_SRST_POS));
	while (QR32(xcfg, XEC_QSPI_MODE_OFS) & BIT(XEC_QSPI_MODE_SRST_POS)) {
		/* spin; reset is fast */
	}

	/* 12MHz is a safe default */
	ckdiv = (uint32_t)calc_ckdiv(MHZ(96), freq_hz);
	mode_fdiv = XEC_QSPI_MODE_CK_DIV_SET(ckdiv);
	QRMW32(xcfg, XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_CK_DIV_MSK, mode_fdiv);

	/* CS timing defaults */
	QW32(xcfg, XEC_QSPI_CSTM_OFS,
	     XEC_QSPI_CSTM_CSA_FCK_SET(XEC_QSPI_CSTM_CSA_FCK_DFLT) |
		     XEC_QSPI_CSTM_LCK_CSD_SET(XEC_QSPI_CSTM_LCK_CSD_DFLT) |
		     XEC_QSPI_CSTM_LDHD_SET(XEC_QSPI_CSTM_LDHD_DFLT) |
		     XEC_QSPI_CSTM_CSD_CSA_SET(XEC_QSPI_CSTM_CSD_CSA_DFLT));

	/* Disable all interrupts, clear all status */
	QW32(xcfg, XEC_QSPI_IER_OFS, 0);
	QW32(xcfg, XEC_QSPI_SR_OFS, 0xFFFFFFFFu);

	/* Clear LDMA enable bitmaps */
	QW32(xcfg, XEC_QSPI_LDMA_RX_EN_OFS, 0);
	QW32(xcfg, XEC_QSPI_LDMA_TX_EN_OFS, 0);

	/* GIRQ routing for this source */
	(void)soc_ecia_girq_status_clear(MCHP_XEC_ECIA_GIRQ(xcfg->girq_enc),
					 MCHP_XEC_ECIA_GIRQ_POS(xcfg->girq_enc));
	(void)soc_ecia_girq_ctrl(MCHP_XEC_ECIA_GIRQ(xcfg->girq_enc),
				 MCHP_XEC_ECIA_GIRQ_POS(xcfg->girq_enc), 1u);

	/* Activate the controller while preserving the clock divider just
	 * programmed above. dev_config() will later refine CPOL/CPHA/CS-select.
	 */
	QRMW32(xcfg, XEC_QSPI_MODE_OFS, BIT(XEC_QSPI_MODE_ACTV_POS), BIT(XEC_QSPI_MODE_ACTV_POS));

	return 0;
}

static int mspi_xec_validate_config(const struct mspi_cfg *cfg, uint8_t hw_ce_max)
{
	if ((cfg->channel_num != 0) || (cfg->dqs_support)) {
		return -ENOTSUP;
	}

	if ((cfg->max_freq < XEC_QSPI_FREQ_MIN) || (cfg->max_freq > XEC_QSPI_FREQ_MAX)) {
		return -ENOTSUP;
	}

	/* QMSPI controller requires software to set the HW CE field */
	if (cfg->sw_multi_periph == false) {
		return -ENOTSUP;
	}

	/* This driver supports only the QMSPI hardware chip enables, not
	 * generic GPIO pins used as chip enables. A configuration that
	 * specifies GPIO CEs is rejected below.
	 *
	 * Supporting GPIO CEs on QMSPI is non-trivial:
	 *
	 * 1. The QMSPI hardware always asserts a HW-controlled chip enable on a
	 *    transfer. Using a GPIO CE instead requires additional DT info and a
	 *    pinctrl state that switches the HW CE pin from the QMSPI CE function
	 *    to a tri-stated GPIO input, so the controller does not drive the
	 *    bus CE while the GPIO CE is driven by software.
	 *
	 * 2. The MSPI API specifies GPIO CEs in two places with unclear
	 *    precedence: struct mspi_cfg carries an array of GPIO CEs
	 *    (ce_gpios[]/num_ce_gpios) with no timing, while struct mspi_xfer
	 *    carries a single struct mspi_ce_control (one GPIO plus a 32-bit
	 *    microsecond delay). An implementation must define which source
	 *    wins, whether the two must match, and what a NULL xfer CE means.
	 *
	 * 3. The per-xfer CE delay is hard to honor for asynchronous transfers.
	 *    The CE deassert happens on transfer completion in ISR context, so a
	 *    microsecond delay cannot be taken there. Honoring it would need a
	 *    deferred mechanism (a driver-owned thread or timer); using the
	 *    system work queue would block other work queue users.
	 */
	if (cfg->num_ce_gpios != 0) {
		return -ENOTSUP;
	}

	/* Cap the requested peripheral count at the port's HW chip-enable
	 * count (num-hw-ce): two on the Shared SPI port, one on the Private
	 * and Internal ports.
	 */
	if ((cfg->num_periph == 0) || (cfg->num_periph > hw_ce_max)) {
		return -ENOTSUP;
	}

	/* QMSPI is a controller only. */
	if (cfg->op_mode != MSPI_OP_MODE_CONTROLLER) {
		return -ENOTSUP;
	}

	/* Driver only supports half-duplex. Half-duplex means transmit command and
	 * parameters ignoring RX lines. If command is read QMSPI will clock in
	 * data on the selected data lines (single, dual, quad). Full-duplex implies
	 * transmit and receive on each clock. QMSPI hardware supports full-duplex
	 * but we are not implementing it at this time. Most targets for MSPI will be
	 * SPI flash devices.
	 */
	if (cfg->duplex != MSPI_HALF_DUPLEX) {
		return -ENOTSUP;
	}

	return 0;
}

static bool mspi_is_inp(const struct device *ctrl)
{
	struct mspi_xec_xdat *const xdat = ctrl->data;

#ifdef CONFIG_MULTITHREADING
	return k_sem_count_get(&xdat->ctx.lock) == 0;
#else
	return xdat->ctx.lock;
#endif
}

/* -------- Driver API -------- */

/* Configure the controller and initialize driver state.
 * Do NOT call this API if there is an on-going transfer.
 * Use the get channel status API to determine if the controller
 * is available. If it is available this API can be called if
 * the controller must be reconfigured.
 *
 * Locking notes:
 * dev_cfg and transceive should be treated as atomic
 * We should acquire and hold a lock if dev_cfg returns success.
 * Transceive should release the lock when all packets are transferred or
 * on error.
 * Async mode has more locking requirements.
 * If transceive is called with async flag set and returns success
 * we should be able to call get channel status without disrupting
 * the in-progress transfer or requiring a lock.
 */
static int api_mspi_mchp_xec_qmspi_cfg(const struct mspi_dt_spec *spec)
{
	const struct mspi_xec_xcfg *xcfg = spec->bus->config;
	struct mspi_xec_xdat *xdat = spec->bus->data;
	const struct mspi_cfg *cfg = &spec->config;
	int rc = 0;

	rc = mspi_xec_validate_config(cfg, xcfg->mspicfg.num_periph);
	if (rc != 0) {
		xdat->ctrl_cfg_ok = false;
		return rc;
	}

	/* Controller (re)config invalidates every cached per-device cfg and
	 * ends any active session. Force-release the cfg lock (idempotent
	 * give — safe because it's a k_sem, not a k_mutex, so any thread can
	 * release regardless of which one held it).
	 */
	for (int i = 0; i < XEC_QSPI_MAX_PERIPH; i++) {
		xdat->dev_cfgs[i].valid = false;
	}
	xdat->dev_id = NULL;
	xdat->max_cfg_freq = cfg->max_freq;
	xdat->num_periph = cfg->num_periph;

#ifdef CONFIG_MULTITHREADING
	/* re-arm both cfg and ctx semaphores to "free". */
	if (k_sem_count_get(&xdat->lock) == 0) {
		k_sem_give(&xdat->lock);
	}
	if (k_sem_count_get(&xdat->ctx.lock) == 0) {
		k_sem_give(&xdat->ctx.lock);
	}
#else
	xdat->lock = false;
	xdat->ctx.lock = false;
#endif

	rc = qmspi_hw_reset_and_init(spec->bus, cfg->max_freq);
	if (rc != 0) {
		return rc;
	}

	xdat->ctrl_cfg_ok = true;
	xdat->active_mspicfg = xcfg->mspicfg;

	return 0;
}

static int io_mode_decode(enum mspi_io_mode mode, struct qmspi_io_decode *d)
{
	switch (mode) {
	case MSPI_IO_MODE_SINGLE:
		d->cmd_ifm = IFM_SINGLE;
		d->addr_ifm = IFM_SINGLE;
		d->data_ifm = IFM_SINGLE;
		return 0;
	case MSPI_IO_MODE_DUAL:
		d->cmd_ifm = IFM_DUAL;
		d->addr_ifm = IFM_DUAL;
		d->data_ifm = IFM_DUAL;
		return 0;
	case MSPI_IO_MODE_DUAL_1_1_2:
		d->cmd_ifm = IFM_SINGLE;
		d->addr_ifm = IFM_SINGLE;
		d->data_ifm = IFM_DUAL;
		return 0;
	case MSPI_IO_MODE_DUAL_1_2_2:
		d->cmd_ifm = IFM_SINGLE;
		d->addr_ifm = IFM_DUAL;
		d->data_ifm = IFM_DUAL;
		return 0;
	case MSPI_IO_MODE_QUAD:
		d->cmd_ifm = IFM_QUAD;
		d->addr_ifm = IFM_QUAD;
		d->data_ifm = IFM_QUAD;
		return 0;
	case MSPI_IO_MODE_QUAD_1_1_4:
		d->cmd_ifm = IFM_SINGLE;
		d->addr_ifm = IFM_SINGLE;
		d->data_ifm = IFM_QUAD;
		return 0;
	case MSPI_IO_MODE_QUAD_1_4_4:
		d->cmd_ifm = IFM_SINGLE;
		d->addr_ifm = IFM_QUAD;
		d->data_ifm = IFM_QUAD;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static uint8_t cpp_to_mode_bits(enum mspi_cpp_mode cpp, uint32_t freq_hz)
{
	uint32_t bits = 0;

	switch (cpp) {
	case MSPI_CPP_MODE_0:
		bits = 0;
		break;
	case MSPI_CPP_MODE_1:
		bits = 0x6U;
		break;
	case MSPI_CPP_MODE_2:
		bits = 0x1U;
		break;
	case MSPI_CPP_MODE_3:
		bits = 0x7U;
		break;
	default:
		bits = 0;
		break;
	}

	if (freq_hz >= MHZ(48)) {
		bits ^= (uint8_t)BIT(2);
	}

	return bits;
}

/* Range-check the masked fields before any are applied. Conditions are
 * flattened with `&&` (mask selected AND value out of range) so the caller's
 * save path is a straight sequence of assignments with no nested validation.
 */
static int mspi_mchp_xec_dev_cfg_validate(const struct mspi_xec_xdat *xdat,
					  const enum mspi_dev_cfg_mask param_mask,
					  const struct mspi_dev_cfg *dev_cfg)
{
	if (((param_mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0) &&
	    (dev_cfg->ce_num >= xdat->active_mspicfg.num_periph)) {
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0) &&
	    ((dev_cfg->freq > xdat->active_mspicfg.max_freq) ||
	     (dev_cfg->freq < XEC_QSPI_FREQ_MIN))) {
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0) && (dev_cfg->rx_dummy > 0x7fffU)) {
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) != 0) && (dev_cfg->tx_dummy > 0x7fffU)) {
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0) && (dev_cfg->cmd_length > 4U)) {
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0) && (dev_cfg->addr_length > 4U)) {
		return -ENOTSUP;
	}

	return 0;
}

static int mspi_mchp_xec_dev_cfg_save(const struct device *ctrl, const struct mspi_dev_id *dev_id,
				      const enum mspi_dev_cfg_mask param_mask,
				      const struct mspi_dev_cfg *dev_cfg)
{
	struct mspi_xec_xdat *const xdat = ctrl->data;
	struct mspi_xec_dev_cfg *xdcfg;
	int rc;

	if ((dev_cfg == NULL) || (dev_id == NULL)) {
		return -EINVAL;
	}

	if (dev_id->dev_idx >= XEC_QSPI_MAX_PERIPH) {
		return -ENOTSUP;
	}

	/* Validate every masked field up front so a failure never leaves the
	 * cached slot partially updated.
	 */
	rc = mspi_mchp_xec_dev_cfg_validate(xdat, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}

	xdcfg = &xdat->dev_cfgs[dev_id->dev_idx];

	/* Partial-mask updates must preserve fields not named in `param_mask`.
	 * Each slot is zero-initialized at driver startup (and on controller
	 * reconfig), so unset fields start at zero naturally — no need to
	 * memset on every call.
	 */

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0) {
		xdcfg->ce_num = dev_cfg->ce_num;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0) {
		xdcfg->freq_div = calc_ckdiv(MHZ(96), dev_cfg->freq);
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0) {
		rc = io_mode_decode(dev_cfg->io_mode, &xdcfg->iom);
		if (rc != 0) {
			return rc;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0) {
		xdcfg->cpp = cpp_to_mode_bits(dev_cfg->cpp, dev_cfg->freq);
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0) {
		xdcfg->rx_dummy = dev_cfg->rx_dummy;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) != 0) {
		xdcfg->tx_dummy = dev_cfg->tx_dummy;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0) {
		xdcfg->read_cmd = dev_cfg->read_cmd;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) != 0) {
		xdcfg->write_cmd = dev_cfg->write_cmd;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0) {
		xdcfg->cmd_len = dev_cfg->cmd_length;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0) {
		xdcfg->adr_len = dev_cfg->addr_length;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0) {
		xdcfg->mem_boundary = dev_cfg->mem_boundary;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) != 0) {
		xdcfg->time_to_break = dev_cfg->time_to_break;
	}

	xdcfg->valid = true;

	return 0;
}

/* Apply the currently-selected device's cached cfg (CKDIV / CPP / CS_SEL) to
 * the controller MODE register. xdat->dev_id must already point at the device
 * to apply.
 */
static void apply_dev_config(const struct device *ctrl)
{
	const struct mspi_xec_xcfg *xcfg = ctrl->config;
	struct mspi_xec_xdat *const xdat = ctrl->data;
	struct mspi_xec_dev_cfg *xdcfg = current_dev_cfg(xdat);
	uint32_t qmode;

	if (xdcfg == NULL) {
		return;
	}

	qmode = QR32(xcfg, XEC_QSPI_MODE_OFS);

	qmode &= (uint32_t)~(XEC_QSPI_MODE_CK_DIV_MSK | XEC_QSPI_MODE_CP_MSK |
			     XEC_QSPI_MODE_CS_SEL_MSK);

	qmode |= XEC_QSPI_MODE_CK_DIV_SET((uint32_t)xdcfg->freq_div);
	qmode |= ((((uint32_t)xdcfg->cpp) << XEC_QSPI_MODE_CP_POS) & XEC_QSPI_MODE_CP_MSK);
	qmode |= (((uint32_t)xdcfg->ce_num << XEC_QSPI_MODE_CS_SEL_POS) & XEC_QSPI_MODE_CS_SEL_MSK);
	qmode |= BIT(XEC_QSPI_MODE_ACTV_POS);

	QW32(xcfg, XEC_QSPI_MODE_OFS, qmode);
}

/* Validate target device configuration and apply it
 * return 0 success
 *        -EINVAL invalid capabilities
 *        -ENOTSUP capability not supported
 * Note: param_mask has two special values:
 * MSPI_DEVICE_CONFIG_NONE
 * MSPI_DEVICE_CONFIG_ALL
 *   We only validate those parameters supported by the HW/driver.
 * This API implements session-style locking: on a new device selection, take the cfg lock
 * and keep it held on success. The session ends when the application
 * calls get_channel_status (see api_mspi_mchp_xec_qmspi_get_chs), which
 * clears dev_id and releases the cfg lock. dev_config for the same
 * dev_id (partial update or NONE re-select) does not re-acquire — the
 * caller already owns the session.
 */
static int api_mspi_mchp_xec_qmspi_dev_cfg(const struct device *ctrl,
					   const struct mspi_dev_id *dev_id,
					   const enum mspi_dev_cfg_mask param_mask,
					   const struct mspi_dev_cfg *cfg)
{
	struct mspi_xec_xdat *const xdat = ctrl->data;
	int rc = 0;
	bool locked = false;

	if (xdat->dev_id != dev_id) {
#ifdef CONFIG_MULTITHREADING
		if (k_sem_take(&xdat->lock, K_FOREVER) != 0) {
			rc = -EBUSY;
		}
#else
		if (xdat->lock == false) {
			xdat->lock = true;
		} else {
			rc = -EBUSY;
		}
#endif
		if (rc != 0) {
			LOG_ERR("Can't acquire driver lock!");
			return -EBUSY;
		}

		locked = true;
	}

	if (mspi_is_inp(ctrl)) {
		rc = -EBUSY;
		goto exit_dev_cfg;
	}

	/* This driver requires software multiperipheral to always be true! */
	if (xdat->active_mspicfg.sw_multi_periph == false) {
		rc = -EINVAL;
		goto exit_dev_cfg;
	}

	if ((dev_id == NULL) || (dev_id->dev_idx >= XEC_QSPI_MAX_PERIPH) ||
	    (dev_id->dev_idx >= xdat->active_mspicfg.num_periph)) {
		rc = -EINVAL;
		goto exit_dev_cfg;
	}

	/* Re-select a previously-configured device. The cached cfg in this
	 * device's slot must be re-applied to MODE so CKDIV / CPP / CS_SEL
	 * reflect the new active device — without this, hardware keeps running
	 * the previously-selected device's settings.
	 */
	if (param_mask == MSPI_DEVICE_CONFIG_NONE) {
		if (xdat->dev_cfgs[dev_id->dev_idx].valid == false) {
			rc = -EPERM;
			goto exit_dev_cfg;
		}
		xdat->dev_id = dev_id;
		apply_dev_config(ctrl);
		goto exit_dev_cfg;
	}

	/* Set dev_id BEFORE save/apply so dev_cfg_save writes to the right slot
	 * and apply_dev_config reads it back.
	 */
	xdat->dev_id = dev_id;

	rc = mspi_mchp_xec_dev_cfg_save(ctrl, dev_id, param_mask, cfg);
	if (rc != 0) {
		LOG_ERR("Dev config error");
		goto exit_dev_cfg;
	}

	apply_dev_config(ctrl);

exit_dev_cfg:
	if (locked && (rc != 0)) {
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&xdat->lock);
#else
		xdat->lock = false;
#endif
	}

	return rc;
}

/* Release the current session. Waits for any in-flight transfer, clears
 * dev_id, and gives back the cfg lock so another device can be configured.
 * The per-slot dev_cfgs[i].valid flag is preserved so a follow-up
 * dev_config(dev_id, NONE) still cheaply re-selects that device.
 */
static int api_mspi_mchp_xec_qmspi_get_chs(const struct device *ctrl, uint8_t ch)
{
	struct mspi_xec_xdat *const xdat = ctrl->data;

	if (ch != 0) {
		return -EINVAL;
	}

#ifdef CONFIG_MULTITHREADING
	/* Block until any in-flight transfer (sync or async) has released ctx.
	 * count=1 means "free"; taking it acts as "wait for idle."
	 */
	(void)k_sem_take(&xdat->ctx.lock, K_FOREVER);

	if (xdat->dev_id != NULL) {
		xdat->dev_id = NULL;
		k_sem_give(&xdat->lock);
	}

	/* Re-arm ctx as "free" for the next transceive. */
	k_sem_give(&xdat->ctx.lock);
#else
	if (xdat->ctx.lock) {
		return -EBUSY;
	}
	if (xdat->dev_id != NULL) {
		xdat->dev_id = NULL;
		xdat->lock = false;
	}
#endif

	return 0;
}

#ifdef CONFIG_MULTITHREADING
static int api_mspi_mchp_xec_qmspi_rcb(const struct device *ctrl, const struct mspi_dev_id *dev_id,
				       const enum mspi_bus_event evt_type,
				       mspi_callback_handler_t cb,
				       struct mspi_callback_context *ctx)
{
	struct mspi_xec_xdat *const xdat = ctrl->data;

	if (mspi_is_inp(ctrl)) {
		return -EBUSY;
	}

	if (dev_id == NULL) {
		return -EINVAL;
	}

	if (evt_type == MSPI_BUS_RESET) {
		return -ENOTSUP;
	}

	xdat->cbs[evt_type] = cb;
	xdat->cbctxs[evt_type] = ctx;

	return 0;
}
#endif

/*
 * Alignment-aware pick of the QMSPI unit size and the PIO FIFO / LDMA
 * access width for a data-phase buffer.
 *
 *   unit  -> XEC_QSPI_CR_QUNIT_* enum (for PIO descriptor)
 *   acc   -> 1, 2, or 4 (bytes per FIFO or LDMA access)
 */
static void pick_unit_and_access(const uint8_t *buf, uint32_t len, uint8_t *unit, uint8_t *acc)
{
	uintptr_t a = (uintptr_t)buf;

	if (((a | len) & 0xFu) == 0 && len >= 16u) {
		*unit = XEC_QSPI_CR_QUNIT_16B;
		*acc = 4u;
	} else if (((a | len) & 0x3u) == 0 && len >= 4u) {
		*unit = XEC_QSPI_CR_QUNIT_4B;
		*acc = 4u;
	} else if (((a | len) & 0x1u) == 0 && len >= 2u) {
		*unit = XEC_QSPI_CR_QUNIT_1B;
		*acc = 2u;
	} else {
		*unit = XEC_QSPI_CR_QUNIT_1B;
		*acc = 1u;
	}
}

/* Reset per-transfer state that submit_packet does not unconditionally
 * overwrite: the MODE LDMA-enable bits and the LDMA descriptor-bitmap
 * registers. Descriptor slots and the CR register are rewritten in full
 * by submit_packet().
 */
static void prep_qspi(const struct mspi_xec_xcfg *hw)
{
	uint32_t msk = BIT(XEC_QSPI_MODE_LD_RX_EN_POS) | BIT(XEC_QSPI_MODE_LD_TX_EN_POS);

	QRMW32(hw, XEC_QSPI_MODE_OFS, msk, 0);
	QW32(hw, XEC_QSPI_LDMA_RX_EN_OFS, 0);
	QW32(hw, XEC_QSPI_LDMA_TX_EN_OFS, 0);
}

/* Parameters describing a single QMSPI descriptor to be built. */
struct qmspi_desc_cfg {
	uint8_t ifm;
	uint8_t txm;
	uint8_t tx_dma;
	uint8_t rx_dma;
	uint8_t qunit;
	uint16_t nqunits;
	uint8_t next_idx;
	bool rx_en;
	bool close_en;
	bool last;
};

static uint32_t desc_build(const struct qmspi_desc_cfg *c)
{
	uint32_t d = XEC_QSPI_CR_IFM_SET(c->ifm) | XEC_QSPI_CR_TXM_SET(c->txm) |
		     XEC_QSPI_CR_TXDMA_SET(c->tx_dma) | XEC_QSPI_CR_RXDMA_SET(c->rx_dma) |
		     XEC_QSPI_CR_QUNIT_SET(c->qunit) | XEC_QSPI_CR_NQUNITS_SET(c->nqunits) |
		     XEC_QSPI_DR_ND_SET(c->next_idx);

	if (c->rx_en) {
		d |= BIT(XEC_QSPI_CR_RX_EN_POS);
	}
	if (c->close_en) {
		d |= BIT(XEC_QSPI_CR_CLOSE_EN_POS);
	}
	if (c->last) {
		d |= BIT(XEC_QSPI_DR_LD_POS);
	}

	return d;
}

/* Build the data-phase descriptor chain for the PIO path.
 *
 * Walks the buffer in five staged phases so that each emitted descriptor
 * uses the largest QUNIT compatible with the address+length still pending:
 *
 *   1. 1B QUNIT bytes until 4-byte aligned (head)
 *   2. 4B QUNIT chunks until 16-byte aligned
 *   3. 16B QUNIT bulk (split at QMSPI_NQUNITS_MAX = 0x7FFF units per descr)
 *   4. trailing 4B QUNIT chunks
 *   5. trailing 1B QUNIT bytes
 *
 * Each descriptor is emitted with next_idx = idx+1, close_en=false, last=false.
 * The caller (submit_packet) sets CLOSE | LD on the returned last index.
 *
 * Returns the index of the last descriptor written.
 */
static uint8_t emit_pio_data_descrs(struct mspi_xec_context *ctx, uint8_t start_idx,
				    uint8_t data_ifm, bool dir_tx, const uint8_t *buf, uint32_t len)
{
	uint8_t idx = start_idx;
	uint8_t last = start_idx;
	uintptr_t a = (uintptr_t)buf;
	uint32_t off = 0;
	uint16_t chunk;
	uint32_t units;
	struct qmspi_desc_cfg cfg = {
		.ifm = data_ifm,
		.txm = dir_tx ? XEC_QSPI_CR_TXM_DATA : XEC_QSPI_CR_TXM_DIS,
		.tx_dma = XEC_QSPI_CR_DMA_DIS,
		.rx_dma = XEC_QSPI_CR_DMA_DIS,
		.rx_en = !dir_tx,
	};

	/* Phase 1: 1B units until 4-byte aligned */
	while ((off < len) && (((a + off) & 0x3u) != 0u)) {
		__ASSERT_NO_MSG(idx < ARRAY_SIZE(ctx->descrs));
		cfg.qunit = XEC_QSPI_CR_QUNIT_1B;
		cfg.nqunits = 1u;
		cfg.next_idx = (uint8_t)((idx + 1u) & 0xFu);
		ctx->descrs[idx] = desc_build(&cfg);
		last = idx;
		idx++;
		off += 1u;
	}

	/* Phase 2: 4B units until 16-byte aligned */
	while (((len - off) >= 4u) && (((a + off) & 0xFu) != 0u)) {
		__ASSERT_NO_MSG(idx < ARRAY_SIZE(ctx->descrs));
		cfg.qunit = XEC_QSPI_CR_QUNIT_4B;
		cfg.nqunits = 1u;
		cfg.next_idx = (uint8_t)((idx + 1u) & 0xFu);
		ctx->descrs[idx] = desc_build(&cfg);
		last = idx;
		idx++;
		off += 4u;
	}

	/* Phase 3: bulk 16B chunks (split at NQUNITS_MAX) */
	while ((len - off) >= 16u) {
		__ASSERT_NO_MSG(idx < ARRAY_SIZE(ctx->descrs));
		units = (len - off) / 16u;
		chunk = (units > QMSPI_NQUNITS_MAX) ? QMSPI_NQUNITS_MAX : (uint16_t)units;
		cfg.qunit = XEC_QSPI_CR_QUNIT_16B;
		cfg.nqunits = chunk;
		cfg.next_idx = (uint8_t)((idx + 1u) & 0xFu);
		ctx->descrs[idx] = desc_build(&cfg);
		last = idx;
		idx++;
		off += (uint32_t)chunk * 16u;
	}

	/* Phase 4: trailing 4B chunks */
	while ((len - off) >= 4u) {
		__ASSERT_NO_MSG(idx < ARRAY_SIZE(ctx->descrs));
		units = (len - off) / 4u;
		chunk = (units > QMSPI_NQUNITS_MAX) ? QMSPI_NQUNITS_MAX : (uint16_t)units;
		cfg.qunit = XEC_QSPI_CR_QUNIT_4B;
		cfg.nqunits = chunk;
		cfg.next_idx = (uint8_t)((idx + 1u) & 0xFu);
		ctx->descrs[idx] = desc_build(&cfg);
		last = idx;
		idx++;
		off += (uint32_t)chunk * 4u;
	}

	/* Phase 5: trailing 1B bytes */
	if ((len - off) > 0u) {
		__ASSERT_NO_MSG(idx < ARRAY_SIZE(ctx->descrs));
		chunk = (uint16_t)(len - off);
		cfg.qunit = XEC_QSPI_CR_QUNIT_1B;
		cfg.nqunits = chunk;
		cfg.next_idx = (uint8_t)((idx + 1u) & 0xFu);
		ctx->descrs[idx] = desc_build(&cfg);
		last = idx;
	}

	return last;
}

#ifdef CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA
/* Build the data-phase descriptor for the LDMA path: a single descriptor
 * with the LDMA TXDMA/RXDMA channel selector. Length-override on the
 * channel drives the byte count, so QUNIT/NQUNITS are placeholders.
 */
static uint8_t emit_ldma_data_descr(struct mspi_xec_context *ctx, uint8_t start_idx,
				    uint8_t data_ifm, bool dir_tx, uint8_t qunit)
{
	struct qmspi_desc_cfg cfg = {
		.ifm = data_ifm,
		.txm = dir_tx ? XEC_QSPI_CR_TXM_DATA : XEC_QSPI_CR_TXM_DIS,
		.tx_dma = dir_tx ? XEC_QSPI_CR_LDMA_CH2 : XEC_QSPI_CR_DMA_DIS,
		.rx_dma = dir_tx ? XEC_QSPI_CR_DMA_DIS : XEC_QSPI_CR_LDMA_CH0,
		/* On the LDMA path, length-override drives the byte count and INCRA
		 * walks the memory address. QUNIT only controls the SCK-side unit
		 * granularity, so force 1-byte units to keep tail bytes safe regardless
		 * of buffer alignment.
		 */
		.qunit = XEC_QSPI_CR_QUNIT_1B,
		.nqunits = 1u,
		.next_idx = (uint8_t)((start_idx + 1u) & 0xFu),
		.rx_en = !dir_tx,
	};

	ARG_UNUSED(qunit);
	__ASSERT_NO_MSG(start_idx < ARRAY_SIZE(ctx->descrs));
	ctx->descrs[start_idx] = desc_build(&cfg);
	return start_idx;
}
#endif /* CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA */

/* Set IER for the next transfer, after clearing SR.
 * NOTE: DMA Done fires when ANY DMA channel or LDMA channel finished.
 * We use multiple LDMA channels and do not need the ISR to fire on each LDMA channel done.
 * The purpose of DMA Done status is if software requires more channels for a single transfer
 * than we have. For example, using central DMA might require DMA if we want to transmit
 * command, address, and data using the same central DMA channel.
 * Hardware implements three LDMA channel per direction. We do not need DMA Done interrupt.
 */
static void prime_interrupts(const struct mspi_xec_xcfg *hw, bool pio, bool dir_tx)
{
	uint32_t ier = 0, triglvl = 0;

	QW32(hw, XEC_QSPI_SR_OFS, 0xFFFFFFFFu);

	ier = BIT(XEC_QSPI_IER_TXB_ERR_POS) | BIT(XEC_QSPI_IER_RXB_ERR_POS) |
	      BIT(XEC_QSPI_IER_PROG_ERR_POS);

	if (pio) {
		ier |= BIT(XEC_QSPI_IER_XFR_DONE_POS);
		if (dir_tx) {
			ier |= BIT(XEC_QSPI_IER_TXB_REQ_POS);
			triglvl = XEC_QSPI_BCNT_TR_TXB_SET(4U); /* interrupt on <= this value */
		} else {
			ier |= BIT(XEC_QSPI_IER_RXB_REQ_POS);
			triglvl = XEC_QSPI_BCNT_TR_RXB_SET(4U); /* interrupt on >= this value */
		}
	} else {
		ier |= (BIT(XEC_QSPI_IER_LDMA_RX_ERR_POS) | BIT(XEC_QSPI_IER_LDMA_TX_ERR_POS) |
			BIT(XEC_QSPI_IER_XFR_DONE_POS));
	}

	QW32(hw, XEC_QSPI_BCNT_TR_OFS, triglvl);
	QW32(hw, XEC_QSPI_IER_OFS, ier);
}

#ifdef CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA
/* Program one QMSPI Local-DMA channel for a single transfer.
 *
 * ch is the channel index 0..5 (RX_CH0..RX_CH2 = 0..2, TX_CH0..TX_CH2 = 3..5)
 * — the per-channel CR/SA/LR registers are linearly addressed by this index.
 * The TX vs RX direction and the descriptor's TXDMA/RXDMA selector are set by
 * the caller; this helper only writes the channel registers.
 *
 * `access` is the memory-side access width in bytes (1, 2, or 4) and maps to
 * the channel SZ field. `ovrl` enables length-override so the channel's
 * length register drives the byte count instead of the descriptor's NQUNITS.
 */
static void ldma_program(const struct mspi_xec_xcfg *hw, uint8_t ch, const void *buf, uint32_t len,
			 uint8_t access, bool ovrl)
{
	uint32_t cr = BIT(XEC_QSPI_LDMA_CHX_CR_EN_POS) | BIT(XEC_QSPI_LDMA_CHX_CR_INCRA_POS);
	uint32_t sz;

	switch (access) {
	case 4u:
		sz = XEC_QSPI_LDMA_CHX_CR_SZ_4B;
		break;
	case 2u:
		sz = XEC_QSPI_LDMA_CHX_CR_SZ_2B;
		break;
	default:
		sz = XEC_QSPI_LDMA_CHX_CR_SZ_1B;
		break;
	}
	cr |= XEC_QSPI_LDMA_CHX_CR_SZ_SET(sz);

	if (ovrl) {
		cr |= BIT(XEC_QSPI_LDMA_CHX_CR_OVRL_POS);
	}

	QW32(hw, XEC_QSPI_LDMA_CHX_SA_OFS(ch), (uint32_t)(uintptr_t)buf);
	QW32(hw, XEC_QSPI_LDMA_CHX_LR_OFS(ch), len);
	QW32(hw, XEC_QSPI_LDMA_CHX_CR_OFS(ch), cr);
}
#endif /* CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA */

/* Push a single byte into the TX FIFO, blocking only on TXB_FULL. The 8-byte
 * FIFO is shared across cmd/addr/data preloads, so writes that would overflow
 * are silently dropped by hardware unless we honor TXB_FULL.
 */
static void tx_fifo_push_byte(const struct mspi_xec_xcfg *hw, uint8_t b)
{
	while (QR32(hw, XEC_QSPI_SR_OFS) & BIT(XEC_QSPI_SR_TXB_FULL_POS)) {
		/* spin until at least one slot drains; relevant only if cmd or
		 * addr already filled the FIFO. Pre-START this loop is bounded
		 * by FIFO depth (8) at most.
		 */
	}
	QW8(hw, XEC_QSPI_TXB_OFS, b);
}

/* Parameters describing a TX prefix (command or address) descriptor. */
struct qmspi_cmd_addr_desc {
	const uint8_t *buf;   /* MSB-first serialized bytes to send */
	uint8_t len;          /* number of bytes */
	uint8_t ifm;          /* interface mode for this phase */
	uint8_t cr_ldma_ch;   /* CR TXDMA selector when use_ldma */
	uint8_t prog_ldma_ch; /* ldma_program() TX channel when use_ldma */
};

/* Emit a command/address phase descriptor at desc_idx. Either drives an LDMA
 * TX channel or preloads the serialized bytes into the shared TX FIFO.
 */
static void emit_cmd_addr_descr(const struct mspi_xec_xcfg *hw, struct mspi_xec_context *ctx,
				uint8_t desc_idx, bool use_ldma,
				const struct qmspi_cmd_addr_desc *p)
{
	uint8_t txdma = XEC_QSPI_CR_DMA_DIS;

	if (use_ldma) {
		txdma = p->cr_ldma_ch;
	} else {
		for (uint8_t i = 0; i < p->len; i++) {
			tx_fifo_push_byte(hw, p->buf[i]);
		}
	}

	ctx->descrs[desc_idx] = desc_build(&(struct qmspi_desc_cfg){
		.ifm = p->ifm,
		.txm = XEC_QSPI_CR_TXM_DATA,
		.tx_dma = txdma,
		.rx_dma = XEC_QSPI_CR_DMA_DIS,
		.qunit = XEC_QSPI_CR_QUNIT_1B,
		.nqunits = p->len,
		.next_idx = (desc_idx + 1u),
	});

#ifdef CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA
	if (use_ldma) {
		ldma_program(hw, p->prog_ldma_ch, p->buf, p->len, 1u, true);
		QW32(hw, XEC_QSPI_LDMA_TX_EN_OFS,
		     QR32(hw, XEC_QSPI_LDMA_TX_EN_OFS) | BIT(desc_idx));
	}
#endif
}

/* Emit the data-phase descriptor(s) starting at desc_idx, program the transfer
 * (LDMA channel or PIO TX FIFO top-up), mark the final descriptor CLOSE | LD,
 * and return the index of that final descriptor.
 */
static uint8_t emit_data_descr(const struct device *ctrl, const struct mspi_xfer_packet *pkt,
			       uint8_t desc_idx, uint8_t ifm, bool use_ldma, uint8_t qunit,
			       uint8_t access)
{
	const struct mspi_xec_xcfg *hw = ctrl->config;
	struct mspi_xec_xdat *const xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;
	bool dir_tx = (pkt->dir == MSPI_TX);
	uint8_t last = desc_idx;

	ARG_UNUSED(qunit);
	ARG_UNUSED(access);

	if (use_ldma) {
#ifdef CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA
		uint8_t ch = dir_tx ? XEC_QSPI_LDMA_TX_CH2 : XEC_QSPI_LDMA_RX_CH0;
		uint32_t ofs_en = dir_tx ? XEC_QSPI_LDMA_TX_EN_OFS : XEC_QSPI_LDMA_RX_EN_OFS;

		last = emit_ldma_data_descr(ctx, desc_idx, ifm, dir_tx, qunit);
		ldma_program(hw, ch, pkt->data_buf, pkt->num_bytes, access, true);
		QW32(hw, ofs_en, QR32(hw, ofs_en) | BIT(last));
#endif
	} else {
		last = emit_pio_data_descrs(ctx, desc_idx, ifm, dir_tx, pkt->data_buf,
					    pkt->num_bytes);

		/* For PIO TX, top up the TX FIFO from the data buffer so the data
		 * phase has bytes ready when the descriptor starts clocking — don't
		 * rely on BCNT_TR firing at empty. tx_fifo_push_byte respects
		 * TXB_FULL, so this coexists with cmd/addr bytes already queued. The
		 * ISR's pio_tx_fill resumes from ctx->pio_pos.
		 */
		if (dir_tx && pkt->data_buf != NULL) {
			const uint8_t *src = pkt->data_buf;
			uint32_t pos = 0;

			while (pos < pkt->num_bytes &&
			       !(QR32(hw, XEC_QSPI_SR_OFS) & BIT(XEC_QSPI_SR_TXB_FULL_POS))) {
				QW8(hw, XEC_QSPI_TXB_OFS, src[pos]);
				pos++;
			}
			ctx->pio_pos = pos;
		}
	}

	/* Mark the final data descriptor as CLOSE | LD. */
	ctx->descrs[last] |= BIT(XEC_QSPI_CR_CLOSE_EN_POS) | BIT(XEC_QSPI_DR_LD_POS);
	return last;
}

static int submit_packet(const struct device *ctrl, uint32_t pkt_idx)
{
	const struct mspi_xec_xcfg *hw = ctrl->config;
	struct mspi_xec_xdat *const xdat = ctrl->data;
	struct mspi_xec_dev_cfg *dev_cfg = current_dev_cfg(xdat);
	struct mspi_xec_context *ctx = &xdat->ctx;
	struct mspi_xfer *xfer = &ctx->xfer;
	const struct mspi_xfer_packet *pkt = &xfer->packets[pkt_idx];
	uint16_t dummy_cycles = 0;
	uint8_t cmd_len = xfer->cmd_length;
	uint8_t addr_len = xfer->addr_length;
	uint8_t desc_idx = 0, qunit = 0, access = 0, last = 0;
	bool use_ldma =
		IS_ENABLED(CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA) && (xfer->xfer_mode == MSPI_DMA);
	bool dir_tx = (pkt->dir == MSPI_TX);

	if (dev_cfg == NULL) {
		return -EPERM;
	}
	if (cmd_len > 4U || addr_len > 4U) {
		return -EINVAL;
	}

	dummy_cycles = dir_tx ? xfer->tx_dummy : xfer->rx_dummy;

	pick_unit_and_access(pkt->data_buf, pkt->num_bytes, &qunit, &access);

	ctx->cmdbuf.w = 0;
	ctx->adrbuf.w = 0;
	ctx->pio_pos = 0;

	/* Clear LDMA enable bitmaps from any prior transfer. */
	prep_qspi(hw);

	/* Flush any leftover bytes from a previous transfer that errored
	 * before its FIFO drained. CLR_FIFOS is self-clearing.
	 */
	QW32(hw, XEC_QSPI_EXE_OFS, BIT(XEC_QSPI_EXE_CLR_FIFOS_POS));

	/* ----- Descriptor 0: command phase (TX, single lane) ----- */
	if (cmd_len > 0) {
		/* Format cmd as MSB-first and adjust for length */
		sys_put_be32(pkt->cmd, ctx->cmdbuf.b);
		ctx->cmdbuf.w >>= ((4U - cmd_len) * 8U);

		emit_cmd_addr_descr(hw, ctx, desc_idx, use_ldma, &(struct qmspi_cmd_addr_desc){
			.buf = ctx->cmdbuf.b,
			.len = cmd_len,
			.ifm = IFM_SINGLE,
			.cr_ldma_ch = XEC_QSPI_CR_LDMA_CH0,
			.prog_ldma_ch = XEC_QSPI_LDMA_TX_CH0,
		});
		desc_idx++;
	}

	/* ----- Descriptor 1: address phase (TX, 1-1-N or 1-N-N) ----- */
	if (addr_len > 0) {
		/* Serialize address MSB-first into addr_buf. */
		sys_put_be32(pkt->address, ctx->adrbuf.b);
		ctx->adrbuf.w >>= ((4U - addr_len) * 8U);

		emit_cmd_addr_descr(hw, ctx, desc_idx, use_ldma, &(struct qmspi_cmd_addr_desc){
			.buf = ctx->adrbuf.b,
			.len = addr_len,
			.ifm = dev_cfg->iom.addr_ifm,
			.cr_ldma_ch = XEC_QSPI_CR_LDMA_CH1,
			.prog_ldma_ch = XEC_QSPI_LDMA_TX_CH1,
		});
		desc_idx++;
	}

	/* ----- Optional dummy-clocks descriptor ----- */
	if (dummy_cycles > 0) {
		ctx->descrs[desc_idx] = desc_build(&(struct qmspi_desc_cfg){
			.ifm = IFM_SINGLE,
			.txm = XEC_QSPI_CR_TXM_DIS,
			.tx_dma = XEC_QSPI_CR_DMA_DIS,
			.rx_dma = XEC_QSPI_CR_DMA_DIS,
			.qunit = XEC_QSPI_CR_QUNIT_BITS,
			.nqunits = dummy_cycles,
			.next_idx = (desc_idx + 1u),
		});
		desc_idx++;
	}

	/* ----- Data phase ----- */
	if (pkt->num_bytes > 0) {
		last = emit_data_descr(ctrl, pkt, desc_idx, dev_cfg->iom.data_ifm, use_ldma,
				       qunit, access);
	} else {
		/* No data phase — mark the last non-data descriptor (in the
		 * descrs[] mirror) as LD/CLOSE. The mirror is what gets
		 * written to the QMSPI descriptor registers below.
		 */
		if (desc_idx == 0) {
			/* Caller asked for a transfer with no cmd, addr, dummy,
			 * or data. Nothing to send.
			 */
			return -EINVAL;
		}

		last = desc_idx - 1u;
		ctx->descrs[last] |= BIT(XEC_QSPI_CR_CLOSE_EN_POS) | BIT(XEC_QSPI_DR_LD_POS);
	}

	/* Flush the descriptor mirror to the QMSPI descriptor register file
	 * (slots 0..last) and clear any slots beyond `last` so a stale entry
	 * from a previous transfer cannot be reached if the chain advances.
	 */
	for (uint8_t i = 0; i <= last; i++) {
		QW32(hw, XEC_QSPI_DESCR_OFS(i), ctx->descrs[i]);
	}
	for (uint8_t i = last + 1u; i < XEC_QSPI_MAX_DESCR_IDX; i++) {
		QW32(hw, XEC_QSPI_DESCR_OFS(i), 0);
	}

	/* ----- Start: enable descriptor mode and fire ----- */
	if (use_ldma) {
		uint32_t mask = BIT(XEC_QSPI_MODE_LD_RX_EN_POS) | BIT(XEC_QSPI_MODE_LD_TX_EN_POS);

		QRMW32(hw, XEC_QSPI_MODE_OFS, mask, mask);
	}

	/* In descriptor mode the controller reads descriptor fields from the
	 * descriptor registers; CR's per-phase fields (IFM, TXM, NQUNITS, ...)
	 * are unread. Write only DESCR_EN.
	 */
	QW32(hw, XEC_QSPI_CR_OFS, BIT(XEC_QSPI_CR_DESCR_EN_POS));

	prime_interrupts(hw, !use_ldma, dir_tx);

	/* Arm completion: clear stale signal/error before HW START. */
	ctx->err = 0;
#ifdef CONFIG_MULTITHREADING
	k_sem_reset(&xdat->xfer_done);
#else
	xdat->xfer_done = false;
#endif

	/* QMSPI starts at descriptor 0 when DESCR_EN is set; just kick START. */
	QW32(hw, XEC_QSPI_EXE_OFS, BIT(XEC_QSPI_EXE_START_POS));

	return 0;
}

static void release_ctx(const struct device *ctrl)
{
	struct mspi_xec_xdat *xdat = ctrl->data;

#ifdef CONFIG_MULTITHREADING
	/* ctx.lock convention: count=1 free, count=0 in-progress.
	 * pre_xfr_xdat_init() does k_sem_take() to acquire; release gives it
	 * back. Guard against double-give under the limit=1 cap.
	 */
	if (k_sem_count_get(&xdat->ctx.lock) == 0) {
		k_sem_give(&xdat->ctx.lock);
	}
#else
	xdat->ctx.lock = false;
#endif
	pm_device_busy_clear(ctrl);
}

static int pre_xfr_xdat_init(const struct device *ctrl, const struct mspi_xfer *req)
{
	struct mspi_xec_xdat *xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;

#ifdef CONFIG_MULTITHREADING
	/* Use a short, bounded wait to acquire the per-controller xfer slot.
	 * The user's req->timeout is reserved for actual transfer completion
	 * in wait_for_xfr_done(); don't double-count it here.
	 */
	if (k_sem_take(&ctx->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE)) != 0) {
		return -EBUSY;
	}
#else
	if (ctx->lock == true) {
		return -EBUSY;
	}

	ctx->lock = true;
#endif

	pm_device_busy_set(ctrl);

	ctx->xfer = *req;
	ctx->pkt_idx = 0;
	ctx->packets_left = (int)req->num_packet;
	ctx->cmdbuf.w = 0;
	ctx->adrbuf.w = 0;

	memset(ctx->descrs, 0, sizeof(ctx->descrs));

	return 0;
}

/* Block the caller until the ISR signals completion (or error) for the
 * packet most recently kicked by submit_packet().
 */
static int wait_for_xfr_done(const struct device *ctrl, uint32_t timeout_ms)
{
	struct mspi_xec_xdat *const xdat = ctrl->data;
	uint32_t budget = timeout_ms + CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

#ifdef CONFIG_MULTITHREADING
	if (k_sem_take(&xdat->xfer_done, K_MSEC(budget)) != 0) {
		return -ETIMEDOUT;
	}
#else
	uint32_t elapsed = 0;

	while (!xdat->xfer_done) {
		if (elapsed >= budget) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1000u);
		elapsed++;
	}
	xdat->xfer_done = false;
#endif

	return xdat->ctx.err;
}

/* Transcieve API */
static int api_mspi_mchp_xec_qmspi_tc(const struct device *ctrl, const struct mspi_dev_id *dev_id,
				      const struct mspi_xfer *req)
{
	struct mspi_xec_xdat *xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;
	int rc = 0;

	if ((dev_id == NULL) || (req == NULL)) {
		return -EINVAL;
	}

	if (mspi_is_inp(ctrl)) {
		return -EBUSY;
	}

	if ((dev_id == NULL) || (dev_id->dev_idx >= XEC_QSPI_MAX_PERIPH) ||
	    (xdat->dev_id != dev_id) || (xdat->dev_cfgs[dev_id->dev_idx].valid == false)) {
		LOG_ERR("Transceive without prior dev_config for this dev_id");
		return -EPERM;
	}

	if (req->async) {
		if (!IS_ENABLED(CONFIG_MSPI_ASYNC)) {
			LOG_ERR("Async requested. Driver not built with async support");
			return -ENOTSUP;
		}
		if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
			LOG_ERR("Async requested. Driver not built with multithreading");
			return -ENOTSUP;
		}
	}

	if (req->xfer_mode == MSPI_DMA && !IS_ENABLED(CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA)) {
		LOG_ERR("MSPI_DMA requested but CONFIG_MSPI_MCHP_XEC_QMSPI_LDMA is disabled");
		return -ENOTSUP;
	}

	if (req->num_packet == 0) {
		return 0; /* nothing to do */
	}

	if (req->packets == NULL) {
		return -EINVAL;
	}

	rc = pre_xfr_xdat_init(ctrl, req);
	if (rc != 0) {
		return rc;
	}

#ifdef CONFIG_MULTITHREADING
	if (req->async) {
		/* Async path: kick packet 0, start the total-xfer timeout, return.
		 * The ISR routes end-of-packet via signal_packet_done() to
		 * async_pkt_work_handler, which advances or finalizes.
		 */
		uint32_t total_ms =
			(req->timeout != 0)
				? (req->timeout * req->num_packet)
				: (CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE * req->num_packet);

		ctx->pkt_idx = 0;
		k_timer_start(&xdat->async_timer, K_MSEC(total_ms), K_NO_WAIT);

		rc = submit_packet(ctrl, 0);
		if (rc != 0) {
			LOG_ERR("Submit QMSPI pkt 0 error (%d)", rc);
			k_timer_stop(&xdat->async_timer);
			release_ctx(ctrl);
			return rc;
		}

		return 0;
	}
#endif /* CONFIG_MULTITHREADING */

	/* Sync path: per-packet chain restart. Each packet rebuilds descriptors
	 * from index 0 and is started independently. The ISR signals xfer_done;
	 * we wait, check err, and advance the cursor here.
	 */
	for (ctx->pkt_idx = 0; ctx->pkt_idx < req->num_packet; ctx->pkt_idx++) {
		rc = submit_packet(ctrl, ctx->pkt_idx);
		if (rc != 0) {
			break;
		}

		rc = wait_for_xfr_done(ctrl, req->timeout);
		if (rc != 0) {
			break;
		}

		ctx->packets_left--;
	}

	if (rc != 0) {
		LOG_ERR("Transfer error (%d) at packet %u", rc, ctx->pkt_idx);
	}

	release_ctx(ctrl);

	return rc;
}

/* -------- Driver interrupt handling  -------- */
#ifdef CONFIG_MULTITHREADING
/* Fan out a controller-level callback. Safe when no handler is registered. */
static void fire_controller_cb(struct mspi_xec_xdat *xdat, enum mspi_bus_event evt, int status)
{
	struct mspi_callback_context *cbctx;
	mspi_callback_handler_t cb;

	if (evt >= MSPI_BUS_EVENT_MAX) {
		return;
	}

	cb = xdat->cbs[evt];
	cbctx = xdat->cbctxs[evt];
	if ((cb == NULL) || (cbctx == NULL)) {
		return;
	}

	cbctx->mspi_evt.evt_type = evt;
	cbctx->mspi_evt.evt_data.controller = xdat->ctrl;
	cbctx->mspi_evt.evt_data.dev_id = xdat->dev_id;
	cbctx->mspi_evt.evt_data.packet = ((xdat->ctx.xfer.packets != NULL) &&
					   (xdat->ctx.pkt_idx < xdat->ctx.xfer.num_packet))
						  ? &xdat->ctx.xfer.packets[xdat->ctx.pkt_idx]
						  : NULL;
	cbctx->mspi_evt.evt_data.status = (uint32_t)status;
	cbctx->mspi_evt.evt_data.packet_idx = xdat->ctx.pkt_idx;

	cb(cbctx);
}

/* Finalize an async transfer: stop timer, fire controller-level callback,
 * release ctx lock. Called from work-queue context, never from ISR.
 */
static void finalize_xfer_async(struct mspi_xec_xdat *xdat, int rc)
{
	if (xdat->ctx.err == 0) {
		xdat->ctx.err = rc;
	}
	k_timer_stop(&xdat->async_timer);
	if (xdat->ctx.err == 0) {
		fire_controller_cb(xdat, MSPI_BUS_XFER_COMPLETE, 0);
	} else {
		fire_controller_cb(xdat, MSPI_BUS_ERROR, xdat->ctx.err);
	}
	release_ctx(xdat->ctrl);
}

static void async_tmout_timer_handler(struct k_timer *timer)
{
	struct mspi_xec_xdat *const xdat = CONTAINER_OF(timer, struct mspi_xec_xdat, async_timer);

	/* Submit work to handle timeout in proper context */
	k_work_submit(&xdat->async_tmout_work);
}

/* Timeout work: abort the controller, fire MSPI_BUS_TIMEOUT, release ctx.
 * Runs in syswq context (the timer ISR submits this work).
 */
static void async_tmout_work_handler(struct k_work *work)
{
	struct mspi_xec_xdat *const xdat =
		CONTAINER_OF(work, struct mspi_xec_xdat, async_tmout_work);
	const struct mspi_xec_xcfg *hw = xdat->ctrl->config;

	LOG_ERR("Async transfer timed out: %s", xdat->ctrl->name);

	/* Halt the controller, kill any in-flight LDMA, soft-reset, then
	 * re-apply CKDIV/CPP via apply_dev_config. SRST is self-clearing.
	 */
	QW32(hw, XEC_QSPI_IER_OFS, 0);
	QW32(hw, XEC_QSPI_EXE_OFS, BIT(XEC_QSPI_EXE_STOP_POS));
	prep_qspi(hw);
	QW32(hw, XEC_QSPI_MODE_OFS, BIT(XEC_QSPI_MODE_SRST_POS));
	while (QR32(hw, XEC_QSPI_MODE_OFS) & BIT(XEC_QSPI_MODE_SRST_POS)) {
		/* spin; reset is fast */
	}
	apply_dev_config(xdat->ctrl);
	(void)soc_ecia_girq_status_clear(MCHP_XEC_ECIA_GIRQ(hw->girq_enc),
					 MCHP_XEC_ECIA_GIRQ_POS(hw->girq_enc));

	xdat->ctx.err = -ETIMEDOUT;
	fire_controller_cb(xdat, MSPI_BUS_TIMEOUT, -ETIMEDOUT);
	release_ctx(xdat->ctrl);
}

/* Per-packet work: invoked from ISR's signal_packet_done() in the async path.
 * Decides whether to advance to the next packet, finalize, or report error.
 * Runs in syswq context — safe to call submit_packet (which programs LDMA
 * and writes register banks) and to fire user callbacks.
 */
static void async_pkt_work_handler(struct k_work *work)
{
	struct mspi_xec_xdat *const xdat = CONTAINER_OF(work, struct mspi_xec_xdat, async_pkt_work);
	struct mspi_xec_context *ctx = &xdat->ctx;
	int rc;

	/* Error latched by ISR during the prior packet — terminate the xfer. */
	if (ctx->err != 0) {
		finalize_xfer_async(xdat, ctx->err);
		return;
	}

	/* The packet that just completed was at ctx->pkt_idx. If it was the
	 * last one, finalize WITHOUT advancing — the MSPI_BUS_XFER_COMPLETE
	 * callback's evt_data.packet_idx must reference the last packet that
	 * actually completed (num_packet - 1), not one past it.
	 */
	if (ctx->pkt_idx + 1 >= ctx->xfer.num_packet) {
		ctx->packets_left = 0;
		finalize_xfer_async(xdat, 0);
		return;
	}

	ctx->pkt_idx++;
	ctx->packets_left--;

	rc = submit_packet(xdat->ctrl, ctx->pkt_idx);
	if (rc != 0) {
		LOG_ERR("Submit QMSPI pkt %u error (%d)", ctx->pkt_idx, rc);
		finalize_xfer_async(xdat, rc);
	}
}
#endif

/* Called from ISR at end of a packet (success or hard error). Routes to the
 * sync waiter (sem) or to the async work handler. Must be ISR-safe.
 */
static void signal_packet_done(struct mspi_xec_xdat *xdat)
{
#ifdef CONFIG_MULTITHREADING
	if (xdat->ctx.xfer.async) {
		k_work_submit(&xdat->async_pkt_work);
		return;
	}
	k_sem_give(&xdat->xfer_done);
#else
	xdat->xfer_done = true;
#endif
}

/* Drain RX FIFO into the active packet's buffer until it's empty or the
 * buffer is full. Always uses byte access; QUNIT in the descriptor controls
 * how the controller clocks bits out of the SPI bus, not how software reads
 * the FIFO.
 */
static void pio_rx_drain(const struct mspi_xec_xcfg *hw, struct mspi_xec_context *ctx,
			 const struct mspi_xfer_packet *pkt)
{
	while (ctx->pio_pos < pkt->num_bytes &&
	       !(QR32(hw, XEC_QSPI_SR_OFS) & BIT(XEC_QSPI_SR_RXB_EMPTY_POS))) {
		pkt->data_buf[ctx->pio_pos++] = QR8(hw, XEC_QSPI_RXB_OFS);
	}
}

/* Push from the packet buffer into the TX FIFO until the FIFO is full or the
 * buffer is exhausted.
 */
static void pio_tx_fill(const struct mspi_xec_xcfg *hw, struct mspi_xec_context *ctx,
			const struct mspi_xfer_packet *pkt)
{
	while (ctx->pio_pos < pkt->num_bytes &&
	       !(QR32(hw, XEC_QSPI_SR_OFS) & BIT(XEC_QSPI_SR_TXB_FULL_POS))) {
		QW8(hw, XEC_QSPI_TXB_OFS, pkt->data_buf[ctx->pio_pos++]);
	}
}

static void mspi_mchp_xec_qmspi_isr(const struct device *ctrl)
{
	const struct mspi_xec_xcfg *xcfg = ctrl->config;
	struct mspi_xec_xdat *const xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;
	uint32_t sr = QR32(xcfg, XEC_QSPI_SR_OFS);
	const struct mspi_xfer_packet *pkt = NULL;
	bool is_pio = (ctx->xfer.xfer_mode != MSPI_DMA);
	bool done = false;

	if (ctx->packets_left > 0 && ctx->xfer.packets != NULL) {
		pkt = &ctx->xfer.packets[ctx->pkt_idx];
	}

	/* Hard error: stop, signal -EIO. */
	if (sr & QMSPI_SR_ERR_MSK) {
		ctx->err = -EIO;
		QW32(xcfg, XEC_QSPI_IER_OFS, 0);
		done = true;
		goto ack;
	}

	/* PIO data movement. LDMA path doesn't enable RXB_REQ/TXB_REQ in IER
	 * (see prime_interrupts), so these branches are dead under DMA.
	 */
	if (is_pio && pkt != NULL && pkt->data_buf != NULL && pkt->num_bytes > 0) {
		if ((sr & BIT(XEC_QSPI_SR_RXB_REQ_POS)) && pkt->dir == MSPI_RX) {
			pio_rx_drain(xcfg, ctx, pkt);
		}
		if ((sr & BIT(XEC_QSPI_SR_TXB_REQ_POS)) && pkt->dir == MSPI_TX) {
			pio_tx_fill(xcfg, ctx, pkt);
		}
	}

	if (sr & BIT(XEC_QSPI_SR_XFR_DONE_POS)) {
		/* Drain any tail bytes the controller delivered between the
		 * last RXB_REQ and the XFR_DONE edge.
		 */
		if (is_pio && pkt != NULL && pkt->dir == MSPI_RX && pkt->data_buf != NULL) {
			pio_rx_drain(xcfg, ctx, pkt);
		}
		QW32(xcfg, XEC_QSPI_IER_OFS, 0);
		done = true;
	}

ack:
	/* ACK the bits we observed and clear the GIRQ source latch so the
	 * level-triggered NVIC line deasserts.
	 */
	QW32(xcfg, XEC_QSPI_SR_OFS, sr);
	(void)soc_ecia_girq_status_clear(MCHP_XEC_ECIA_GIRQ(xcfg->girq_enc),
					 MCHP_XEC_ECIA_GIRQ_POS(xcfg->girq_enc));

	if (done) {
		signal_packet_done(xdat);
	}
}

/* -------- Driver power management and initialization -------- */
static int mspi_mchp_xec_pm_off(const struct device *ctrl)
{
	const struct mspi_xec_xcfg *xcfg = ctrl->config;
	int rc = 0;

	QW32(xcfg, XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_SRST_POS);
	while (QR32(xcfg, XEC_QSPI_MODE_OFS) & BIT(XEC_QSPI_MODE_SRST_POS)) {
		/* spin; reset is fast */
	}

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_DEFAULT);

	return rc;
}

static int mspi_mchp_xec_pm_on(const struct device *ctrl)
{
	const struct mspi_xec_xcfg *xcfg = ctrl->config;
	const struct mspi_dt_spec spec = {
		.bus = ctrl,
		.config = xcfg->mspicfg,
	};
	int rc = 0;

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc != 0) {
		return rc;
	}

	rc = api_mspi_mchp_xec_qmspi_cfg(&spec);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

/* QSPI is not a wake source. We apply default pin state and then set the
 * block's activate bit. This presumes the SoC did not lose context in suspend.
 */
static int mspi_mchp_xec_pm_resume(const struct device *ctrl)
{
	const struct mspi_xec_xcfg *xcfg = ctrl->config;
	int rc = 0;

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_DEFAULT);
	QSB32(xcfg, XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_ACTV_POS);

	return rc;
}

/* QSPI is not a wake source. We clear QSPI activate bit which gates
 * off clocks in the block. All QSPI registers hold their state in chip
 * suspend. Only if the chip loses VTR1 power rail in suspend will the
 * QSPI hardware lose its context. Next, we set the QSPI pins to sleep state.
 * Note: QSPI obeys the chip PCR SLP_EN protocol and will gate off its
 * clocks when PCR asserts QSPI's SLP_EN signal. We manually clear QSPI
 * activate bit because the app may want to put QSPI in suspend when
 * not using it.
 */
static int mspi_mchp_xec_pm_suspend(const struct device *ctrl)
{
	const struct mspi_xec_xcfg *xcfg = ctrl->config;
	int rc = 0;

	QCB32(xcfg, XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_ACTV_POS);

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_SLEEP);
	if (rc == -ENOENT) {
		rc = 0;
	}

	return rc;
}

static int mspi_mchp_xec_pm_action_cb(const struct device *ctrl, enum pm_device_action action)
{
	int rc = 0;

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		rc = mspi_mchp_xec_pm_suspend(ctrl);
	} else if (action == PM_DEVICE_ACTION_RESUME) {
		rc = mspi_mchp_xec_pm_resume(ctrl);
	} else if (action == PM_DEVICE_ACTION_TURN_OFF) {
		rc = mspi_mchp_xec_pm_off(ctrl);
	} else if (action == PM_DEVICE_ACTION_TURN_ON) {
		rc = mspi_mchp_xec_pm_on(ctrl);
	} else {
		rc = -ENOTSUP;
	}

	return rc;
}

/* -------- Driver init and de-init -------- */
static int mspi_xec_qmspi_init(const struct device *ctrl)
{
	const struct mspi_xec_xcfg *xcfg = ctrl->config;
	struct mspi_xec_xdat *const xdat = ctrl->data;
	int rc = 0;

#ifdef CONFIG_MULTITHREADING
	k_sem_init(&xdat->lock, 1, 1);
	k_sem_init(&xdat->xfer_done, 0, 1);
	k_sem_init(&xdat->ctx.lock, 1, 1);
	k_timer_init(&xdat->async_timer, async_tmout_timer_handler, NULL);
	k_work_init(&xdat->async_pkt_work, async_pkt_work_handler);
	k_work_init(&xdat->async_tmout_work, async_tmout_work_handler);
#else
	xdat->ctx.lock = false;
#endif
	xdat->ctrl = ctrl;

	/* Sets device's pm->state to PM_DEVICE_STATE_OFF
	 * if pm_device_is_powered(ctrl) returns false return 0 here
	 *   Note: pm_device_is_powered always returns TRUE if CONFIG_PM_DEVICE_POWER_DOMAIN
	 *         is not set. Otherwise it returns
	 *              TRUE if pm ptr == NULL or
	 *              TRUE if pm->domain ptr == NULL or
	 *              TRUE/FALSE based on pm->domain->pm_base->state == PM_DEVICE_STATE_ACTIVE
	 * Invokes callback with PM_DEVICE_ACTION_TURN_ON, exit if error other than -ENOTSUP
	 * If device has no PM structure returns callback with PM_DEVICE_ACTION_RESUME
	 * Set device's pm->state to PM_DEVICE_STATE_SUSPENDED
	 * If PM_CONFIG_RUNTIME is enabled and atomic PM_DEVICE_FLAG_RUNTIME_AUTO set then return 0
	 * Invoke callback with PM_DEVICE_ACTION_RESUME, return on error
	 * set pm->state = PM_DEVICE_STATE_ACTIVE
	 *
	 */
	rc = pm_device_driver_init(ctrl, mspi_mchp_xec_pm_action_cb);
	if (rc != 0) {
		return rc;
	}

	if (xcfg->irq_cfg_func != NULL) {
		xcfg->irq_cfg_func();
	}

	return 0;
}

/* Always defined so DEVICE_DT_INST_DEINIT_DEFINE has a valid symbol to
 * reference regardless of CONFIG_DEVICE_DEINIT_SUPPORT.
 */
#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int mspi_xec_qmspi_deinit(const struct device *ctrl)
{
	return pm_device_driver_deinit(ctrl, mspi_mchp_xec_pm_action_cb);
}
#endif /* CONFIG_DEVICE_DEINIT_SUPPORT */

static DEVICE_API(mspi, mspi_mchp_xec_qmspi_api) = {
	.config = api_mspi_mchp_xec_qmspi_cfg,
	.dev_config = api_mspi_mchp_xec_qmspi_dev_cfg,
	.get_channel_status = api_mspi_mchp_xec_qmspi_get_chs,
	.transceive = api_mspi_mchp_xec_qmspi_tc,
#ifdef CONFIG_MULTITHREADING
	.register_callback = api_mspi_mchp_xec_qmspi_rcb,
#endif
};

#define MSPI_MCHP_XEC_QMSPI_MSPI_CFG(inst)                                                         \
	{                                                                                          \
		.channel_num = 0,                                                                  \
		.op_mode = DT_INST_ENUM_IDX(inst, op_mode),                                        \
		.duplex = DT_INST_ENUM_IDX(inst, duplex),                                          \
		.dqs_support = DT_INST_PROP(inst, dqs_support),                                    \
		.sw_multi_periph = DT_INST_PROP_OR(inst, software_multiperipheral, true),          \
		.ce_group = NULL,                                                                  \
		.num_ce_gpios = 0,                                                                 \
		.num_periph = DT_INST_PROP(inst, num_hw_ce),                                       \
		.max_freq = DT_INST_PROP_OR(inst, clock_frequency, MHZ(12)),                       \
		.re_init = true,                                                                   \
	}

#define MSPI_MCHP_XEC_QMSPI_DEVICE_DEF(inst)                                                       \
	PM_DEVICE_DT_INST_DEFINE(inst, mspi_mchp_xec_pm_action_cb);                                \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static void mspi_mchp_xec_qmspi_irq_cfg_##inst(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    mspi_mchp_xec_qmspi_isr, DEVICE_DT_INST_GET(inst), 0);                 \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	static struct mspi_xec_xdat mspi_xec_xdat_##inst;                                          \
	static const struct mspi_xec_xcfg mspi_xec_xcfg_##inst = {                                 \
		.regs = (mm_reg_t)DT_INST_REG_ADDR(inst),                                          \
		.irq_cfg_func = mspi_mchp_xec_qmspi_irq_cfg_##inst,                                \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.mspicfg = MSPI_MCHP_XEC_QMSPI_MSPI_CFG(inst),                                     \
		.girq_enc = (uint16_t)DT_INST_PROP_BY_IDX(inst, girqs, 0),                         \
		.pcr_enc = (uint16_t)DT_INST_PROP(inst, pcr),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEINIT_DEFINE(inst, mspi_xec_qmspi_init, mspi_xec_qmspi_deinit,             \
				     PM_DEVICE_DT_INST_GET(inst), &mspi_xec_xdat_##inst,           \
				     &mspi_xec_xcfg_##inst, POST_KERNEL,                           \
				     CONFIG_MSPI_INIT_PRIORITY, &mspi_mchp_xec_qmspi_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_MCHP_XEC_QMSPI_DEVICE_DEF)
