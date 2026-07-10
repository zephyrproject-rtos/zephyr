/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MSPI driver for the Microchip MEC175x GPSPI controller.
 *
 * Supports half-duplex, controller-mode SDR transfers in single, dual,
 * and quad I/O modes (including 1-1-N and 1-N-N variants). Data phase
 * runs through either PIO (descriptors + 8-byte FIFOs) or QMSPI local
 * DMA (three dedicated channels, length-override enabled).
 */

#define DT_DRV_COMPAT microchip_xec_gpspi_v2_controller

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
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
#include <zephyr/dt-bindings/dma/mchp-xec-dmac.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>

LOG_MODULE_REGISTER(mspi_mchp_xec_gpspi, CONFIG_MSPI_LOG_LEVEL);

/* #define XEC_GPSPI_DEBUG_ISR */
/* #define XEC_GPSPI_DEBUG_NO_TIMEOUT */

#define XEC_GPSPI_XFR_BUF_SIZE 8U

enum qmspi_data_phase_ifm {
	IFM_SINGLE = XEC_QSPI_CR_IFM_FD,
	IFM_DUAL = XEC_QSPI_CR_IFM_DUAL,
};

struct mspi_xec_gp_xcfg {
	mm_reg_t regs;
	void (*irq_cfg_func)(void);
	const struct pinctrl_dev_config *pincfg;
	struct mspi_cfg mspicfg;
	uint32_t flags;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t pcr_enc;
#ifdef CONFIG_MSPI_DMA
	const struct device *dma_dev;
	uint8_t dma_tx_chan;
	uint8_t dma_tx_slot;
	uint8_t dma_rx_chan;
	uint8_t dma_rx_slot;
#endif
};

struct xec_ca_buf {
	union {
		uint32_t w;
		uint16_t hw[2];
		uint8_t b[4];
	} buf;
	uint8_t nbytes;
};

#define MSPI_XEC_GPKT_FLAG_DIR_RD 0
#define MSPI_XEC_GPKT_FLAG_DIR_WR BIT(0)
#define MSPI_XEC_GPKT_FLAG_DMA    BIT(1)
#define MSPI_XEC_GPKT_FLAG_GO     BIT(7)

#define MSPI_XEC_GPKT_CR_CMD_IDX 0
#define MSPI_XEC_GPKT_CR_ADR_IDX 1U
#define MSPI_XEC_GPKT_CR_TSC_IDX 2U
#define MSPI_XEC_GPKT_CR_DAT_IDX 3U
#define MSPI_XEC_GPKT_CR_MAX_IDX 4U

struct mspi_xec_gpspi_pkt {
	struct xec_ca_buf cmd;
	struct xec_ca_buf adr;
	uint16_t ntsc;
	uint8_t flags;
	uint8_t dma_chan;
	uint8_t cr_bm;
	uint8_t cr_idx;
	bool dma_armed;
	uint32_t cr[MSPI_XEC_GPKT_CR_MAX_IDX];
	uint8_t *data_ptr;
	uint32_t data_nbytes;
	uint8_t *rem_data_ptr;
	uint32_t rem_nbytes;
};

struct mspi_xec_context {
	struct mspi_xfer xfer;
	uint32_t pkt_idx;
	int packets_left;
	uint32_t pio_pos; /* byte cursor within current PIO data chunk */
	int err;          /* sticky error latched by ISR */
	struct mspi_xec_gpspi_pkt gpkt;
#ifdef CONFIG_MULTITHREADING
	struct k_sem lock;
#else
	bool lock;
#endif
	bool req_async;
	bool req_dma;
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

struct mspi_xec_gp_xdat {
#ifdef XEC_GPSPI_DEBUG_ISR
	volatile uint32_t qsr;
	volatile uint32_t qbcntsr;
	volatile uint32_t qcr;
	volatile uint32_t qier;
#endif
	const struct device *ctrl;
	const struct mspi_dev_id *dev_id;
	struct mspi_xec_dev_cfg dev_cfg;
	struct mspi_xec_context ctx;
	uint32_t max_cfg_freq;
	uint8_t num_periph;
	bool ctrl_cfg_ok;
	bool ctrl_reinit;
	bool dev_cfg_ok;
#ifdef CONFIG_MULTITHREADING
	struct k_mutex lock;
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

/* GP-SPI is a reduced functionality QMSPI.
 * Frequency base = 48 MHz
 * No descriptor registers.
 * No Local-DMA.
 * We can use SoC QMSPI register offsets and defines with the following caviets.
 * Mode register
 *   frequency divider field is 6-bits at bits [21:16]
 *   One chip select, the chip select field is reserved.
 * Control register:
 *    IFM in bits [1:0] only implements bit[0]. 0=Full-duplex, 1=Dual
 *    TX and RX DMA Mode fields are only for central-DMA (no Local-DMA)
 *    Qunits field in bits [11:10] only implements bit[10]. 0=bits, 1=1-byte
 *    Start Descriptor field in bits [15:12] is read-only reserved 0
 *    Descriptor Mode Enable bit [16] is read-only reserved 0.
 *    NumQunits field is reduced from 15 bits to 6 bits (bits [22:17])
 * Status register:
 *    LDMA status bits removed (reserved)
 *    TX and RX FIFO trigger (request) status bits removed (reserved)
 *    Current descriptor read-only field removed (reserved)
 * Chip select timing register:
 *    Mmoved to offset 0x1C replacing the buffer count trigger register.
 *    GP-SPI does not implement FIFO trigger level interrupts.
 */
#define XEC_GPSPI_FREQ_MAX MHZ(48)
#define XEC_GPSPI_FREQ_MIN ((XEC_GPSPI_FREQ_MAX) / 64U)

#define XEC_GPSPI_CSTM_OFS 0x18U

/* Mode register */
#define XEC_GPSPI_MODE_CKDIV_POS    16
#define XEC_GPSPI_MODE_CKDIV_MSK    GENMASK(21, 16)
#define XEC_GPSPI_MODE_CKDIV_SET(v) FIELD_PREP(XEC_GPSPI_MODE_CKDIV_MSK, (v))
#define XEC_GPSPI_MODE_CKDIV_GET(r) FIELD_GET(XEC_GPSPI_MODE_CKDIV_MSK, (r))

#define XEC_GPSPI_MAX_NUM_QUNITS 0x3FU

#define XEC_GPSPI_CR_NQUNITS_MSK    GENMASK(22, 17)
#define XEC_GPSPI_CR_NQUNITS_SET(u) FIELD_PREP(XEC_GPSPI_CR_NQUNITS_MSK, (u))
#define XEC_GPSPI_CR_NQUNITS_GET(r) FIELD_GET(XEC_GPSPI_CR_NQUNITS_MSK, (r))

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

/* 6-bit NQUNITS limit per descriptor */
#define GPSPI_NQUNITS_MAX 0x3FU

/* aligned NQUNITS limit (divisible by 4) */
#define GPSPI_ALIGNED_NQUNITS_MAX 0x3CU

#define GPSPI_SR_ERR_MSK                                                                           \
	(BIT(XEC_QSPI_SR_TXB_ERR_POS) | BIT(XEC_QSPI_SR_RXB_ERR_POS) |                             \
	 BIT(XEC_QSPI_SR_PROG_ERR_POS))

#define GPSPI_IER_MSK                                                                              \
	(BIT(XEC_QSPI_IER_XFR_DONE_POS) | BIT(XEC_QSPI_IER_TXB_ERR_POS) |                          \
	 BIT(XEC_QSPI_IER_RXB_ERR_POS) | BIT(XEC_QSPI_IER_PROG_ERR_POS))

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

static int gpspi_hw_reset_and_init(const struct device *dev, uint32_t freq_hz)
{
	const struct mspi_xec_gp_xcfg *xcfg = dev->config;
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

	ckdiv = (uint32_t)calc_ckdiv(XEC_GPSPI_FREQ_MAX, freq_hz);
	mode_fdiv = XEC_QSPI_MODE_CK_DIV_SET(ckdiv);
	QRMW32(xcfg, XEC_QSPI_MODE_OFS, XEC_GPSPI_MODE_CKDIV_MSK, mode_fdiv);

	/* CS timing defaults */
	QW32(xcfg, XEC_GPSPI_CSTM_OFS,
	     XEC_QSPI_CSTM_CSA_FCK_SET(XEC_QSPI_CSTM_CSA_FCK_DFLT) |
		     XEC_QSPI_CSTM_LCK_CSD_SET(XEC_QSPI_CSTM_LCK_CSD_DFLT) |
		     XEC_QSPI_CSTM_LDHD_SET(XEC_QSPI_CSTM_LDHD_DFLT) |
		     XEC_QSPI_CSTM_CSD_CSA_SET(XEC_QSPI_CSTM_CSD_CSA_DFLT));

	/* Disable all interrupts, clear all status */
	QW32(xcfg, XEC_QSPI_IER_OFS, 0);
	QW32(xcfg, XEC_QSPI_SR_OFS, 0xFFFFFFFFu);

	/* GIRQ routing for this source */
	(void)soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);
	(void)soc_ecia_girq_ctrl(xcfg->girq, xcfg->girq_pos, MCHP_MEC_ECIA_GIRQ_EN);

	/* Activate the controller. Leave all other MODE bits clear until
	 * dev_config() programs CPOL/CPHA/CS-select/clock-divider.
	 */
	QW32(xcfg, XEC_QSPI_MODE_OFS, BIT(XEC_QSPI_MODE_ACTV_POS));

	return 0;
}

/* Check control configuration is valid.
 * GP-SPIv2 hardware is a controller only.
 * GP-SPIv2 hardware controls a single chip select, driver implementation does not support using
 * generic GPIO pins as chip enables.
 * Driver only supports half-duplex, hardware is capable of both full and half-duplex.
 * Most targets for MSPI will be SPI flash devices.
 * Half-duplex means transmit tri-states RX input lines(s) and receive tri-states TX output line(s).
 * Software-multiperipheral is a DT property that only needs to be true if there are 2 or more
 * child devices. GPSPI is single-CE so num_periph is normally < 2 but we check it anyway for
 * completeness.
 */
static int mspi_xec_validate_config(const struct mspi_cfg *cfg)
{
	if ((cfg->channel_num != 0) || (cfg->dqs_support)) {
		return -ENOTSUP;
	}

	if ((cfg->max_freq < XEC_GPSPI_FREQ_MIN) || (cfg->max_freq > XEC_GPSPI_FREQ_MAX)) {
		return -ENOTSUP;
	}

	if ((cfg->num_periph >= 2) && (cfg->sw_multi_periph == false)) {
		return -ENOTSUP;
	}

	if (cfg->num_ce_gpios != 0) {
		return -ENOTSUP;
	}

	if (cfg->num_periph > 1U) {
		return -ENOTSUP;
	}

	if (cfg->op_mode != MSPI_OP_MODE_CONTROLLER) {
		return -ENOTSUP;
	}

	if (cfg->duplex != MSPI_HALF_DUPLEX) {
		return -ENOTSUP;
	}

	return 0;
}

/* HW-side "busy" check: CS_ASSERTED (STATUS bit 16, read-only) is set while
 * the controller has chip-select driven low, i.e. a transfer is in progress
 * on the wire. Reflects HW state directly, independent of ctx.lock.
 */
static bool mspi_hw_busy(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;

	return (QR32(xcfg, XEC_QSPI_SR_OFS) & BIT(XEC_QSPI_SR_CS_ASSERTED_POS)) != 0;
}

static bool mspi_is_inp(const struct device *ctrl)
{
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;

	return k_sem_count_get(&xdat->ctx.lock) == 0;
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
 *
 * NOTE: The caller must call get_channel_status to release the API lock.
 */
static int api_mspi_mchp_xec_gpspi_cfg(const struct mspi_dt_spec *spec)
{
	const struct mspi_xec_gp_xcfg *xcfg = spec->bus->config;
	struct mspi_xec_gp_xdat *xdat = spec->bus->data;
	const struct mspi_cfg *cfg = &spec->config;
	int rc = 0;

	rc = mspi_xec_validate_config(cfg);
	if (rc != 0) {
		xdat->ctrl_cfg_ok = false;
		return rc;
	}

	if (cfg->re_init && xdat->ctrl_cfg_ok &&
	    (mspi_is_inp(spec->bus) || mspi_hw_busy(spec->bus))) {
		return -EBUSY;
	}

	xdat->dev_cfg_ok = false;
	xdat->dev_id = NULL;
	xdat->ctrl_reinit = cfg->re_init;
	xdat->max_cfg_freq = cfg->max_freq;
	xdat->num_periph = cfg->num_periph;

#ifdef CONFIG_MULTITHREADING
	/* re-init the context semaphore to the "free" state. */
	if (k_sem_count_get(&xdat->ctx.lock) == 0) {
		k_sem_give(&xdat->ctx.lock);
	}
#else
	xdat->ctx.lock = false;
#endif

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc != 0) {
		return rc;
	}

	rc = gpspi_hw_reset_and_init(spec->bus, cfg->max_freq);
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
	}

	if (freq_hz >= MHZ(48)) {
		bits ^= (uint8_t)BIT(2);
	}

	return bits;
}

/* Per-param save helpers — see the QMSPI driver for the design rationale.
 * GPSPI is single-CE so save_ce_num rejects any ce_num > 0. Freq base for the
 * clock divider is XEC_GPSPI_FREQ_MAX (48 MHz) rather than QMSPI's 96 MHz.
 */

static int save_ce_num(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
		       const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_CE_NUM) == 0) {
		return 0;
	}
	if (dev_cfg->ce_num > 0) {
		return -ENOTSUP;
	}
	xdcfg->ce_num = dev_cfg->ce_num;
	return 0;
}

static int save_freq(struct mspi_xec_gp_xdat *xdat, struct mspi_xec_dev_cfg *xdcfg,
		     enum mspi_dev_cfg_mask mask, const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_FREQUENCY) == 0) {
		return 0;
	}
	if ((dev_cfg->freq > xdat->active_mspicfg.max_freq) ||
	    (dev_cfg->freq < XEC_GPSPI_FREQ_MIN)) {
		return -ENOTSUP;
	}
	xdcfg->freq_div = calc_ckdiv(XEC_GPSPI_FREQ_MAX, dev_cfg->freq);
	return 0;
}

static int save_io_mode(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
			const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_IO_MODE) == 0) {
		return 0;
	}
	return io_mode_decode(dev_cfg->io_mode, &xdcfg->iom);
}

static int save_cpp(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
		    const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_CPP) == 0) {
		return 0;
	}
	xdcfg->cpp = cpp_to_mode_bits(dev_cfg->cpp, dev_cfg->freq);
	return 0;
}

static int save_rx_dummy(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
			 const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_RX_DUMMY) == 0) {
		return 0;
	}
	if (dev_cfg->rx_dummy > 0x7fffU) {
		return -ENOTSUP;
	}
	xdcfg->rx_dummy = dev_cfg->rx_dummy;
	return 0;
}

static int save_tx_dummy(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
			 const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_TX_DUMMY) == 0) {
		return 0;
	}
	if (dev_cfg->tx_dummy > 0x7fffU) {
		return -ENOTSUP;
	}
	xdcfg->tx_dummy = dev_cfg->tx_dummy;
	return 0;
}

static int save_cmd_len(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
			const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_CMD_LEN) == 0) {
		return 0;
	}
	if (dev_cfg->cmd_length > 4U) {
		return -ENOTSUP;
	}
	xdcfg->cmd_len = dev_cfg->cmd_length;
	return 0;
}

static int save_adr_len(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
			const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_ADDR_LEN) == 0) {
		return 0;
	}
	if (dev_cfg->addr_length > 4U) {
		return -ENOTSUP;
	}
	xdcfg->adr_len = dev_cfg->addr_length;
	return 0;
}

/* Fields with no validation — just conditional copy. */
static void save_scalar_fields(struct mspi_xec_dev_cfg *xdcfg, enum mspi_dev_cfg_mask mask,
			       const struct mspi_dev_cfg *dev_cfg)
{
	if ((mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0) {
		xdcfg->read_cmd = dev_cfg->read_cmd;
	}
	if ((mask & MSPI_DEVICE_CONFIG_WRITE_CMD) != 0) {
		xdcfg->write_cmd = dev_cfg->write_cmd;
	}
	if ((mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0) {
		xdcfg->mem_boundary = dev_cfg->mem_boundary;
	}
	if ((mask & MSPI_DEVICE_CONFIG_BREAK_TIME) != 0) {
		xdcfg->time_to_break = dev_cfg->time_to_break;
	}
}

/* Partial-mask updates must preserve fields not named in `param_mask`.
 * xdat (and therefore xdcfg) is zero-initialized at driver startup, so
 * unset fields start at zero naturally — no need to memset on every call.
 */
static int mspi_mchp_xec_dev_cfg_save(const struct device *ctrl,
				      const enum mspi_dev_cfg_mask param_mask,
				      const struct mspi_dev_cfg *dev_cfg)
{
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;
	struct mspi_xec_dev_cfg *xdcfg = &xdat->dev_cfg;
	int rc;

	if (dev_cfg == NULL) {
		return -EINVAL;
	}

	rc = save_ce_num(xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}
	rc = save_freq(xdat, xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}
	rc = save_io_mode(xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}
	rc = save_cpp(xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}
	rc = save_rx_dummy(xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}
	rc = save_tx_dummy(xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}
	rc = save_cmd_len(xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}
	rc = save_adr_len(xdcfg, param_mask, dev_cfg);
	if (rc != 0) {
		return rc;
	}

	save_scalar_fields(xdcfg, param_mask, dev_cfg);

	xdcfg->valid = true;

	return 0;
}

/* Apply device configuration frequency, io_mode, and CPP */
static void apply_dev_config(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;
	struct mspi_xec_dev_cfg *xdcfg = &xdat->dev_cfg;
	uint32_t qmode = QR32(xcfg, XEC_QSPI_MODE_OFS);

	qmode &= (uint32_t)~BIT(XEC_QSPI_MODE_ACTV_POS);
	QW32(xcfg, XEC_QSPI_MODE_OFS, qmode);

	qmode &= (uint32_t)~(XEC_GPSPI_MODE_CKDIV_MSK | XEC_QSPI_MODE_CP_MSK);

	qmode |= XEC_GPSPI_MODE_CKDIV_SET((uint32_t)xdcfg->freq_div);
	qmode |= ((((uint32_t)xdcfg->cpp) << XEC_QSPI_MODE_CP_POS) & XEC_QSPI_MODE_CP_MSK);

	QW32(xcfg, XEC_QSPI_MODE_OFS, qmode);
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
 *
 * software-multiperipheral is only required when 2+ child devices are
 * present. GPSPI is single-CE so this check is normally inert; kept
 * parallel with QMSPI.
 *
 * Use session-style locking: on a new device selection, take the cfg lock
 * and keep it held on success. The session ends when the application
 * calls get_channel_status (see api_mspi_mchp_xec_gpspi_get_chs), which
 * clears dev_id and releases the cfg lock.
 */
static int api_mspi_mchp_xec_gpspi_dev_cfg(const struct device *ctrl,
					   const struct mspi_dev_id *dev_id,
					   const enum mspi_dev_cfg_mask param_mask,
					   const struct mspi_dev_cfg *cfg)
{
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;
	int rc = 0;
	bool locked = false;

	if (xdat->dev_id != dev_id) {
#ifdef CONFIG_MULTITHREADING
		rc = k_mutex_lock(&xdat->lock, K_FOREVER);
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

	if (mspi_is_inp(ctrl) || mspi_hw_busy(ctrl)) {
		rc = -EBUSY;
		goto exit_dev_cfg;
	}

	if ((xdat->active_mspicfg.num_periph >= 2) &&
	    (xdat->active_mspicfg.sw_multi_periph == false)) {
		rc = -EINVAL;
		goto exit_dev_cfg;
	}

	/* Use current device configuration */
	if (param_mask == MSPI_DEVICE_CONFIG_NONE) {
		if (xdat->dev_cfg_ok == false) {
			rc = -EPERM;
			goto exit_dev_cfg;
		}
		xdat->dev_id = dev_id;
		apply_dev_config(ctrl);
		return 0;
	}

	rc = mspi_mchp_xec_dev_cfg_save(ctrl, param_mask, cfg);
	if (rc != 0) {
		LOG_ERR("Dev config error");
		goto exit_dev_cfg;
	}

	apply_dev_config(ctrl);

	xdat->dev_id = dev_id;
	xdat->dev_cfg_ok = true;

	return 0;

exit_dev_cfg:
	if (locked && (rc != 0)) {
#ifdef CONFIG_MULTITHREADING
		k_mutex_unlock(&xdat->lock);
#else
		xdat->lock = false;
#endif
	}

	return rc;
}

/* Release the current session. Waits for any in-flight transfer, releases
 * the cfg mutex, and clears dev_id. The cached dev_cfg (dev_cfg_ok flag) is
 * preserved so a follow-up dev_config(dev_id, NONE) still cheaply re-selects
 * the same device.
 *
 * Must be called from the same thread that owns the cfg mutex (the thread
 * that did the dev_config on this dev_id). If called from a different thread,
 * k_mutex_unlock returns -EPERM which is propagated back to the caller so
 * the mis-use is visible instead of silently leaking the lock.
 */
static int api_mspi_mchp_xec_gpspi_get_chs(const struct device *ctrl, uint8_t ch)
{
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;

	if (ch != 0) {
		return -EINVAL;
	}

#ifdef CONFIG_MULTITHREADING
	/* Block until any in-flight transfer (sync or async) has released ctx.
	 * count=1 means "free"; taking it acts as "wait for idle."
	 */
	(void)k_sem_take(&xdat->ctx.lock, K_FOREVER);

	if (xdat->dev_id != NULL) {
		int unlock_rc = k_mutex_unlock(&xdat->lock);

		if (unlock_rc != 0) {
			k_sem_give(&xdat->ctx.lock);
			return unlock_rc;
		}
		xdat->dev_id = NULL;
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
/* Register a callback if the controller is not busy
 * The driver does not support bus reset callback type.
 */
static int api_mspi_mchp_xec_gpspi_rcb(const struct device *ctrl, const struct mspi_dev_id *dev_id,
				       const enum mspi_bus_event evt_type,
				       mspi_callback_handler_t cb,
				       struct mspi_callback_context *ctx)
{
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;

	if (dev_id == NULL) {
		return -EINVAL;
	}

	if (evt_type == MSPI_BUS_RESET) {
		return -ENOTSUP;
	}

	if (mspi_is_inp(ctrl) || mspi_hw_busy(ctrl)) {
		return -EBUSY;
	}

	xdat->cbs[evt_type] = cb;
	xdat->cbctxs[evt_type] = ctx;

	return 0;
}
#endif

static void release_ctx(const struct device *ctrl)
{
	struct mspi_xec_gp_xdat *xdat = ctrl->data;

	/* ctx.lock convention: count=1 free, count=0 in-progress. */
#ifdef CONFIG_MULTITHREADING
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
	struct mspi_xec_gp_xdat *xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;

	if ((req->cmd_length > 4U) || (req->addr_length > 4U)) {
		LOG_ERR("Cmd and Addr len must be <= 4");
		return -ENOTSUP;
	}

	if ((req->tx_dummy > XEC_GPSPI_MAX_NUM_QUNITS) ||
	    (req->rx_dummy > XEC_GPSPI_MAX_NUM_QUNITS)) {
		LOG_ERR("Dummy cycles must be < 0x8000");
		return -ENOTSUP;
	}

#ifdef CONFIG_MULTITHREADING
	if (k_sem_take(&ctx->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE)) != 0) {
		return -EBUSY;
	}

	/* Drop any stale "give" left behind by a previous timed-out / aborted
	 * transfer so wait_for_xfr_done() actually waits for THIS xfer's ISR.
	 */
	k_sem_reset(&xdat->xfer_done);
#else
	if (ctx->lock == true) {
		return -EBUSY;
	}

	ctx->lock = true;
	xdat->xfer_done = false;
#endif

	pm_device_busy_set(ctrl);

	ctx->xfer = *req;
	ctx->pkt_idx = 0;
	ctx->packets_left = (int)req->num_packet;
	ctx->pio_pos = 0;
	ctx->err = 0;
	ctx->req_async = false;
	ctx->req_dma = false;
#ifdef CONFIG_MSPI_ASYNC
	if (req->async) {
		ctx->req_async = true;
	}
#endif
#ifdef CONFIG_MSPI_DMA
	if (req->xfer_mode == MSPI_DMA) {
		ctx->req_dma = true;
	}
#endif
	memset((void *)&ctx->gpkt, 0, sizeof(struct mspi_xec_gpspi_pkt));

	return 0;
}

static void mspi_xec_gpspi_tx_fifo_wr(const struct device *ctrl, struct xec_ca_buf *cab)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;

	if (cab->nbytes == 4U) {
		QW32(xcfg, XEC_QSPI_TXB_OFS, cab->buf.w);
	} else if (cab->nbytes == 3U) {
		QW16(xcfg, XEC_QSPI_TXB_OFS, cab->buf.hw[0]);
		QW8(xcfg, XEC_QSPI_TXB_OFS, cab->buf.b[2]);
	} else if (cab->nbytes == 2U) {
		QW16(xcfg, XEC_QSPI_TXB_OFS, cab->buf.hw[0]);
	} else {
		QW8(xcfg, XEC_QSPI_TXB_OFS, cab->buf.b[0]);
	}
}

#ifdef CONFIG_MSPI_DMA
static uint32_t gpspi_dma_unit_size(uint8_t *buf, uint32_t buflen)
{
	uint32_t bmsk = ((uint32_t)buf | buflen) & 0x3U;

	if (bmsk == 0) {
		return 4U;
	} else if (bmsk == 0x2U) {
		return 2U;
	} else {
		return 1U;
	}
}

static uint32_t dma_unit_to_gpspi_cr(uint32_t dma_unit, enum mspi_xfer_direction dir)
{
	uint32_t dma_unit_enc = 0;

	if (dma_unit == 4U) {
		dma_unit_enc = XEC_QSPI_CR_DMAC_U4B;
	} else if (dma_unit == 2U) {
		dma_unit_enc = XEC_QSPI_CR_DMAC_U2B;
	} else {
		dma_unit_enc = XEC_QSPI_CR_DMAC_U1B;
	}

	if (dir == MSPI_TX) {
		dma_unit_enc = XEC_QSPI_CR_TXDMA_SET(dma_unit_enc);
	} else {
		dma_unit_enc = XEC_QSPI_CR_RXDMA_SET(dma_unit_enc);
	}

	return dma_unit_enc;
}

/* DMA driver callback: events = done and error.
 * Normal completion is signalled by GP-SPI XFR_DONE; we only act on errors.
 * user_data points to this driver's data structure.
 */
static void xec_gpspi_dma_cb(const struct device *dev, void *user_data, uint32_t channel,
			     int status)
{
	struct mspi_xec_gp_xdat *xdat = (struct mspi_xec_gp_xdat *)user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if ((xdat != NULL) && (status < 0)) {
		xdat->ctx.err = -EIO;
	}
}
#endif

/* Find the next set bit in cr_bm at index `from` or higher.
 * Returns MSPI_XEC_GPKT_CR_MAX_IDX if none.
 */
static int next_cr_slot(uint8_t cr_bm, int from)
{
	for (int i = from; i < MSPI_XEC_GPKT_CR_MAX_IDX; i++) {
		if (cr_bm & BIT(i)) {
			return i;
		}
	}
	return MSPI_XEC_GPKT_CR_MAX_IDX;
}

/* Push from gpkt->data_ptr into the TX FIFO until the FIFO is full or the
 * current chunk is exhausted. ctx->pio_pos is the byte cursor within the
 * current chunk.
 */
static void pio_tx_fill(const struct mspi_xec_gp_xcfg *xcfg, struct mspi_xec_context *ctx)
{
	struct mspi_xec_gpspi_pkt *gpkt = &ctx->gpkt;

	while (ctx->pio_pos < gpkt->data_nbytes &&
	       !(QR32(xcfg, XEC_QSPI_SR_OFS) & BIT(XEC_QSPI_SR_TXB_FULL_POS))) {
		QW8(xcfg, XEC_QSPI_TXB_OFS, gpkt->data_ptr[ctx->pio_pos++]);
	}
}

/* Drain RX FIFO into gpkt->data_ptr until the FIFO is empty or the current
 * chunk is full. Always uses byte access; QUNIT in CR controls bus framing,
 * not how software accesses the FIFO.
 */
static void pio_rx_drain(const struct mspi_xec_gp_xcfg *xcfg, struct mspi_xec_context *ctx)
{
	struct mspi_xec_gpspi_pkt *gpkt = &ctx->gpkt;

	while (ctx->pio_pos < gpkt->data_nbytes &&
	       !(QR32(xcfg, XEC_QSPI_SR_OFS) & BIT(XEC_QSPI_SR_RXB_EMPTY_POS))) {
		gpkt->data_ptr[ctx->pio_pos++] = QR8(xcfg, XEC_QSPI_RXB_OFS);
	}
}

/* Build a GP-SPI packet from the current MSPI xfer packet.
 * GP-SPI only has one transfer descriptor, its control register.
 * We parse the MSPI xfer structure and the current MSPI xfer packet to build
 * the GP-SPI control register values for each phase of the transfer:
 * If command length is not 0 (its limited to 4 bytes), format the command,
 * write command byte(s) to the TX FIFO, and build the GP-SPI control value.
 * If address length is not 0 (its limited to 4 bytes), format the address,
 * write address bytes(s) to the TX FIFO and build the GP-SPI control value.
 * NOTE: we dont' combine command and address because they may use a different
 *       number of pins.
 * Based on the transfer direction if the corresponding RX or TX dummy cycles
 * value is not 0, build a GP-SPI control value. We put GP-SPI into bit mode
 * with one output pin, where number of units equals cycles. TX and RX are kept
 * disabled and the TX FIFO empty. GP-SPI will generate clocks with all I/O pins tri-stated.
 * If the xfer packet number of bytes is non-zero then we create a GP-SPI control value
 * for the TX or RX data phase. If DMA mode is requested we configure and start the DMA channel.
 *
 * Synchronous operation:
 * If cmd CR value != 0
 *   Write cmd CR value to GP-SPI CR register
 *   Arm GP-SPI interrupt
 *   Start GP-SPI
 *   Wait for ISR to give semphore
 *   clean up
 * endif
 * If adr CR value != 0
 *   Write adr CR value to GP-SPI CR register
 *   Arm GP-SPI interrupt
 *   Start GP-SPI
 *   Wait for ISR to give semaphore
 *   cleanup
 * endif
 * If tsc CR value != 0
 *   Write tsc CR value to GP-SPI CR register
 *   Arm GP-SPI interrupt
 *   Start GP-SPI
 *   Wait for ISR to give semaphore
 *   cleanup
 * endif
 * if data CR value 1= 0
 *   Write data CR value to GP-SPI CR register
 *   Arm GP-SPI interrupt
 *   Start GP-SPI
 *   Wait for ISR to give semaphore
 *   cleanup
 * endif
 * Packet transfer done
 *
 * Async operation
 * For the first non-zero CR value
 * Write the GP-SPI CR register
 * Arm GP-SPI interrupt
 * Start GP-SPI
 * return success
 * ISR handles remaining steps in struct mspi_xec_gpspi_pkt
 *
 * We could implement the logic for Async mode and let sync mode use it.
 * Sync mode would wait on the semaphore which the ISR will only set
 * when it finishes all parts of struct mspi_xec_gpspi_pkt.
 *
 * DMA is configured for the FIRST chunk only. The ISR's
 * chunk-continuation path uses dma_reload() to re-arm the
 * channel for each subsequent chunk so the GP-SPI controller
 * sees a fresh per-burst DMA handshake every chunk and the
 * channel stops cleanly at the chunk boundary instead of
 * straddling one.
 */
static int build_gpspi_pkt(const struct device *ctrl, uint32_t pkt_idx)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	struct mspi_xec_gp_xdat *xdat = ctrl->data;
	struct mspi_xec_dev_cfg *dev_cfg = &xdat->dev_cfg;
	struct mspi_xec_context *ctx = &xdat->ctx;
	struct mspi_xfer *xfer = &ctx->xfer;
	struct mspi_xec_gpspi_pkt *gpkt = &ctx->gpkt;
	const struct mspi_xfer_packet *pkt = NULL;
	uint32_t ndata = 0;
	uint32_t ntsc = 0;
	int cr_idx = -1;
#ifdef CONFIG_MSPI_DMA
	int rc = 0;
	uint32_t dma_unit = 0;
	uint32_t gp_dma_unit = 0;
	struct dma_config dcfg = {0};
	struct dma_block_config dblk = {0};
#endif
	if (pkt_idx >= xfer->num_packet) {
		return -EINVAL;
	}

	memset(gpkt, 0, sizeof(struct mspi_xec_gpspi_pkt));

	pkt = &xfer->packets[pkt_idx];

	ntsc = xfer->rx_dummy;
	if (pkt->dir == MSPI_TX) {
		gpkt->flags |= MSPI_XEC_GPKT_FLAG_DIR_WR;
		ntsc = xfer->tx_dummy;
	}

#ifdef CONFIG_MSPI_DMA
	if (xfer->xfer_mode == MSPI_DMA) {
		dma_unit = gpspi_dma_unit_size(pkt->data_buf, pkt->num_bytes);
		gp_dma_unit = dma_unit_to_gpspi_cr(dma_unit, pkt->dir);
		gpkt->flags |= MSPI_XEC_GPKT_FLAG_DMA;
	}
#endif

	if (xfer->cmd_length != 0) { /* packet has 1 to 4 byte command to transmit? */
		cr_idx = MSPI_XEC_GPKT_CR_CMD_IDX;
		gpkt->cr_bm |= BIT(MSPI_XEC_GPKT_CR_CMD_IDX);
		gpkt->cmd.nbytes = (uint8_t)(xfer->cmd_length & 0x7U);
		sys_put_be32(pkt->cmd, gpkt->cmd.buf.b);
		gpkt->cmd.buf.w >>= ((4U - gpkt->cmd.nbytes) * 8U);

		mspi_xec_gpspi_tx_fifo_wr(ctrl, &gpkt->cmd);

		gpkt->cr[cr_idx] = dev_cfg->iom.cmd_ifm;
		gpkt->cr[cr_idx] |= XEC_QSPI_CR_TXM_SET(XEC_QSPI_CR_TXM_DATA);
		gpkt->cr[cr_idx] |= XEC_QSPI_CR_QUNIT_SET(XEC_QSPI_CR_QUNIT_1B);
		gpkt->cr[cr_idx] |= XEC_GPSPI_CR_NQUNITS_SET((uint32_t)gpkt->cmd.nbytes);
	}

	if (xfer->addr_length != 0) { /* packet has 1 to 4 byte address to transmit? */
		gpkt->adr.nbytes = (uint8_t)(xfer->addr_length & 0x7U);
		sys_put_be32(pkt->address, gpkt->adr.buf.b);
		gpkt->adr.buf.w >>= ((4U - gpkt->adr.nbytes) * 8U);

		mspi_xec_gpspi_tx_fifo_wr(ctrl, &gpkt->adr);

		if ((xfer->cmd_length != 0) && (dev_cfg->iom.cmd_ifm == dev_cfg->iom.addr_ifm)) {
			/* combine cmd and adr into one CR */
			gpkt->cr[cr_idx] += XEC_GPSPI_CR_NQUNITS_SET((uint32_t)gpkt->adr.nbytes);
		} else {
			cr_idx = MSPI_XEC_GPKT_CR_ADR_IDX;
			gpkt->cr_bm |= BIT(MSPI_XEC_GPKT_CR_ADR_IDX);
			gpkt->cr[cr_idx] = dev_cfg->iom.addr_ifm;
			gpkt->cr[cr_idx] |= XEC_QSPI_CR_QUNIT_SET(XEC_QSPI_CR_QUNIT_1B);
			gpkt->cr[cr_idx] |= XEC_QSPI_CR_TXM_SET(XEC_QSPI_CR_TXM_DATA);
			gpkt->cr[cr_idx] |= XEC_GPSPI_CR_NQUNITS_SET((uint32_t)gpkt->adr.nbytes);
		}
	}

	if (ntsc != 0) { /* packet needs clocks with I/O's tri-stated after the address phase? */
		cr_idx = MSPI_XEC_GPKT_CR_TSC_IDX;
		gpkt->cr_bm |= BIT(MSPI_XEC_GPKT_CR_TSC_IDX);
		gpkt->cr[cr_idx] = IFM_SINGLE;
		gpkt->cr[cr_idx] |= XEC_QSPI_CR_QUNIT_SET(XEC_QSPI_CR_QUNIT_BITS);
		gpkt->cr[cr_idx] |= XEC_GPSPI_CR_NQUNITS_SET((uint32_t)ntsc);
	}

	if (pkt->num_bytes != 0) { /* packet has data to transmit or receive? */
		cr_idx = MSPI_XEC_GPKT_CR_DAT_IDX;
		gpkt->cr_bm |= BIT(MSPI_XEC_GPKT_CR_DAT_IDX);
		ndata = pkt->num_bytes;
		if (ndata > GPSPI_ALIGNED_NQUNITS_MAX) {
			ndata = GPSPI_ALIGNED_NQUNITS_MAX;
			gpkt->rem_data_ptr = pkt->data_buf + GPSPI_ALIGNED_NQUNITS_MAX;
			gpkt->rem_nbytes = pkt->num_bytes - GPSPI_ALIGNED_NQUNITS_MAX;
		}
		/* data_ptr/data_nbytes describe the CURRENT chunk; updated in the
		 * ISR for chunk continuation (see mspi_mchp_xec_gpspi_isr).
		 */
		gpkt->data_ptr = pkt->data_buf;
		gpkt->data_nbytes = ndata;

		gpkt->cr[cr_idx] = dev_cfg->iom.data_ifm;
		gpkt->cr[cr_idx] |= XEC_QSPI_CR_QUNIT_SET(XEC_QSPI_CR_QUNIT_1B);
		if (pkt->dir == MSPI_TX) {
			gpkt->cr[cr_idx] |= XEC_QSPI_CR_TXM_SET(XEC_QSPI_CR_TXM_DATA);
		} else {
			gpkt->cr[cr_idx] |= BIT(XEC_QSPI_CR_RX_EN_POS);
		}
		gpkt->cr[cr_idx] |= XEC_GPSPI_CR_NQUNITS_SET(ndata);
#ifdef CONFIG_MSPI_DMA
		if (xfer->xfer_mode == MSPI_DMA) {
			gpkt->cr[cr_idx] |= gp_dma_unit;

			dblk.block_size = ndata;

			if (pkt->dir == MSPI_TX) {
				gpkt->dma_chan = xcfg->dma_tx_chan;
				dblk.source_address = (uint32_t)pkt->data_buf;
				dblk.dest_address = (uint32_t)xcfg->regs + XEC_QSPI_TXB_OFS;
				dblk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
				dblk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
				dcfg.dma_slot = xcfg->dma_tx_slot;
				dcfg.channel_direction = MEMORY_TO_PERIPHERAL;
			} else {
				gpkt->dma_chan = xcfg->dma_rx_chan;
				dblk.source_address = (uint32_t)xcfg->regs + XEC_QSPI_RXB_OFS;
				dblk.dest_address = (uint32_t)pkt->data_buf;
				dblk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
				dblk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
				dcfg.dma_slot = xcfg->dma_rx_slot;
				dcfg.channel_direction = PERIPHERAL_TO_MEMORY;
			}

			/* dma_unit is 1, 2, or 4 (bytes per DMA word) — direct from
			 * gpspi_dma_unit_size().
			 */
			dcfg.source_data_size = dma_unit;
			dcfg.dest_data_size = dma_unit;

			dcfg.block_count = 1U;
			dcfg.head_block = &dblk;
			dcfg.user_data = (void *)xdat;
			dcfg.dma_callback = xec_gpspi_dma_cb;

			rc = dma_config(xcfg->dma_dev, gpkt->dma_chan, &dcfg);
			if (rc != 0) {
				LOG_ERR("DMA chan cfg error (%d)", rc);
				return rc;
			}
		}
#endif
	}

	if (cr_idx < 0) { /* Nothing to do! */
		return 0;
	}

	/* On last descriptor de-assert CE if hold_ce is false and no remaining bytes */
	gpkt->cr[cr_idx] |= BIT(XEC_QSPI_DR_LD_POS);
	if ((xfer->hold_ce == false) && (gpkt->rem_nbytes == 0)) { /* De-assert CE on last CR */
		gpkt->cr[cr_idx] |= BIT(XEC_QSPI_CR_CLOSE_EN_POS);
	}

	/* Start at the first populated slot (CMD/ADR/TSC/DAT in that order). */
	gpkt->cr_idx = (uint8_t)next_cr_slot(gpkt->cr_bm, 0);

	gpkt->flags |= MSPI_XEC_GPKT_FLAG_GO;

	return 0;
}

/* Kick the slot at gpkt->cr_idx. Called for fresh slots from transceive entry,
 * for the next slot from the ISR after XFR_DONE, and for chunk continuations
 * within the data phase from the ISR.
 *
 * IER mask is built per-slot: cmd/adr/tsc only need XFR_DONE + error bits
 * (FIFO is preloaded). The data slot in PIO mode also needs TXB_EMPTY (TX) or
 * RXB_FULL (RX). DMA mode never enables FIFO-state interrupts.
 *
 * Every entry into the data slot in DMA mode calls dma_start. The first chunk
 * runs against the dma_config done in build_gpspi_pkt; subsequent chunks run
 * against the dma_reload done by the ISR's chunk-continuation path. Each
 * chunk's DMA stops cleanly at the chunk boundary, giving the GP-SPI
 * controller a fresh per-burst DMA handshake on the next chunk.
 */
static int start_gpkt_phase(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;
	struct mspi_xec_gpspi_pkt *gpkt = &ctx->gpkt;
	uint32_t ier_val = (BIT(XEC_QSPI_IER_XFR_DONE_POS) | BIT(XEC_QSPI_IER_TXB_ERR_POS) |
			    BIT(XEC_QSPI_IER_RXB_ERR_POS) | BIT(XEC_QSPI_IER_PROG_ERR_POS));
	bool is_dat = (gpkt->cr_idx == MSPI_XEC_GPKT_CR_DAT_IDX);
	bool is_tx = (gpkt->flags & MSPI_XEC_GPKT_FLAG_DIR_WR);

	if (gpkt->cr_bm == 0) {
		return 0;
	}

	QW32(xcfg, XEC_QSPI_IER_OFS, 0);
	QW32(xcfg, XEC_QSPI_SR_OFS, UINT32_MAX);
	soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);

	if (is_dat && !ctx->req_dma) {
		/* PIO data phase: cursor 0 at the start of every chunk */
		ctx->pio_pos = 0;

		if (is_tx) {
			/* Pre-fill the TX FIFO with as much of the chunk as fits.
			 * cmd+adr were already shifted out, so the FIFO is empty here.
			 */
			pio_tx_fill(xcfg, ctx);
			if (ctx->pio_pos < gpkt->data_nbytes) {
				ier_val |= BIT(XEC_QSPI_IER_TXB_EMPTY_POS);
			}
		} else {
			ier_val |= BIT(XEC_QSPI_IER_RXB_FULL_POS);
		}
	}

	QW32(xcfg, XEC_QSPI_CR_OFS, gpkt->cr[gpkt->cr_idx]);

#ifdef CONFIG_MSPI_DMA
	if (is_dat && ctx->req_dma) {
		int rc = dma_start(xcfg->dma_dev, gpkt->dma_chan);

		if (rc != 0) {
			LOG_ERR("DMA start error (%d)", rc);
			return rc;
		}
		gpkt->dma_armed = true;
	}
#endif

	QW32(xcfg, XEC_QSPI_IER_OFS, ier_val);
	QW32(xcfg, XEC_QSPI_EXE_OFS, BIT(XEC_QSPI_EXE_START_POS));

	return 0;
}

/* Stop the DMA channel if a chunk's DMA might still be in flight.
 * Idempotent — clears dma_armed so a second call is a no-op. Used on the
 * error / abort / timeout paths. Each chunk's DMA stops cleanly at its
 * block_size boundary; in normal completion no dma_stop is needed and
 * dma_armed gets cleared by the next build_gpspi_pkt's memset.
 */
static inline void try_stop_dma(struct mspi_xec_gp_xdat *xdat)
{
#ifdef CONFIG_MSPI_DMA
	const struct mspi_xec_gp_xcfg *xcfg = xdat->ctrl->config;

	if (xdat->ctx.req_dma && xdat->ctx.gpkt.dma_armed) {
		(void)dma_stop(xcfg->dma_dev, xdat->ctx.gpkt.dma_chan);
		xdat->ctx.gpkt.dma_armed = false;
	}
#else
	ARG_UNUSED(xdat);
#endif
}

/* Block the sync caller until the ISR signals end-of-packet (or error/timeout).
 * Returns 0 on success, -ETIMEDOUT, or whatever the ISR latched in ctx->err.
 */
static int wait_for_xfr_done(const struct device *ctrl, uint32_t timeout_ms)
{
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;
	uint32_t budget = timeout_ms + CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

#ifdef CONFIG_MULTITHREADING
#ifdef XEC_GPSPI_DEBUG_NO_TIMEOUT
	k_sem_take(&xdat->xfer_done, K_FOREVER);
#else
	if (k_sem_take(&xdat->xfer_done, K_MSEC(budget)) != 0) {
		return -ETIMEDOUT;
	}
#endif
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

#ifdef CONFIG_MULTITHREADING
/* Fan out a controller-level callback. Safe when no handler is registered. */
static void fire_controller_cb(struct mspi_xec_gp_xdat *xdat, enum mspi_bus_event evt, int status)
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
 * release ctx lock. Called from work-queue context, never from ISR (the
 * callback may do anything).
 */
static void finalize_xfer_async(struct mspi_xec_gp_xdat *xdat, int rc)
{
	if (xdat->ctx.err == 0) {
		xdat->ctx.err = rc;
	}
	k_timer_stop(&xdat->async_timer);
	if (xdat->ctx.err != 0) {
		try_stop_dma(xdat);
	}
	if (xdat->ctx.err == 0) {
		fire_controller_cb(xdat, MSPI_BUS_XFER_COMPLETE, 0);
	} else {
		fire_controller_cb(xdat, MSPI_BUS_ERROR, xdat->ctx.err);
	}
	release_ctx(xdat->ctrl);
}
#endif /* CONFIG_MULTITHREADING */

/* Called from ISR at end of a packet (success or hard error). Routes to the
 * sync waiter (sem) or to the async work handler. Must be ISR-safe.
 */
static void signal_packet_done(struct mspi_xec_gp_xdat *xdat)
{
#ifdef CONFIG_MULTITHREADING
	if (xdat->ctx.req_async) {
		k_work_submit(&xdat->async_pkt_work);
		return;
	}
	k_sem_give(&xdat->xfer_done);
#else
	xdat->xfer_done = true;
#endif
}

/* Transcieve API
 * Sync path: we loop blocking on the xfer_done sem between packets.
 * The ISR walks slots within each packet.
 *
 * Async path: we build packet 0, kick the first slot, start the total-xfer timeout, and
 * return. The ISR + async_pkt_work_handler drive the rest of the transfer.
 */
static int api_mspi_mchp_xec_gpspi_tc(const struct device *ctrl, const struct mspi_dev_id *dev_id,
				      const struct mspi_xfer *req)
{
	struct mspi_xec_gp_xdat *xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;
	int rc = 0;

	if ((dev_id == NULL) || (req == NULL)) {
		return -EINVAL;
	}

	if (mspi_is_inp(ctrl)) {
		return -EBUSY;
	}

	if ((!xdat->dev_cfg_ok) || (xdat->dev_id != dev_id)) {
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

	if (req->num_packet == 0) {
		return 0; /* nothing to do */
	}

	if (req->packets == NULL) {
		LOG_ERR("App passed NULL packet pointer with non-zero num_packet");
		return -EINVAL;
	}

	rc = pre_xfr_xdat_init(ctrl, req);
	if (rc != 0) {
		return rc;
	}

	if (req->xfer_mode == MSPI_DMA && !IS_ENABLED(CONFIG_MSPI_DMA)) {
		LOG_ERR("MSPI_DMA requested but CONFIG_MSPI_DMA is disabled");
		release_ctx(ctrl);
		return -ENOTSUP;
	}

#ifdef CONFIG_MULTITHREADING
	if (req->async) {
		uint32_t total_ms =
			(req->timeout != 0)
				? (req->timeout * req->num_packet)
				: (CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE * req->num_packet);

		ctx->pkt_idx = 0;
		rc = build_gpspi_pkt(ctrl, 0);
		if (rc != 0) {
			LOG_ERR("Build GP-SPI pkt 0 error (%d)", rc);
			release_ctx(ctrl);
			return rc;
		}

		k_timer_start(&xdat->async_timer, K_MSEC(total_ms), K_NO_WAIT);

		rc = start_gpkt_phase(ctrl);
		if (rc != 0) {
			k_timer_stop(&xdat->async_timer);
			release_ctx(ctrl);
			return rc;
		}

		return 0;
	}
#endif /* CONFIG_MULTITHREADING */

	for (uint32_t pkt_idx = 0; pkt_idx < req->num_packet; pkt_idx++) {
		ctx->pkt_idx = pkt_idx;

		rc = build_gpspi_pkt(ctrl, pkt_idx);
		if (rc != 0) {
			LOG_ERR("Build GP-SPI pkt %u error (%d)", pkt_idx, rc);
			break;
		}

		/* Empty packet (no cmd, no addr, no dummy, no data): skip. */
		if (ctx->gpkt.cr_bm == 0) {
			continue;
		}

		rc = start_gpkt_phase(ctrl);
		if (rc != 0) {
			break;
		}

		rc = wait_for_xfr_done(ctrl, req->timeout);
		if (rc != 0) {
			break;
		}
	}

	if (rc != 0) {
		LOG_ERR("Transfer error (%d)", rc);
		try_stop_dma(xdat);
	}

	release_ctx(ctrl);
	return rc;
}

/* -------- Driver interrupt handling  -------- */
#ifdef CONFIG_MULTITHREADING
static void async_tmout_timer_handler(struct k_timer *timer)
{
	struct mspi_xec_gp_xdat *const xdat =
		CONTAINER_OF(timer, struct mspi_xec_gp_xdat, async_timer);

	/* Submit work to handle timeout in proper context */
	k_work_submit(&xdat->async_tmout_work);
}

/* Timeout work: abort the controller, fire MSPI_BUS_TIMEOUT, release ctx.
 * Runs in syswq context (timer ISR submits this work).
 */
static void async_tmout_work_handler(struct k_work *work)
{
	struct mspi_xec_gp_xdat *const xdat =
		CONTAINER_OF(work, struct mspi_xec_gp_xdat, async_tmout_work);
	const struct mspi_xec_gp_xcfg *xcfg = xdat->ctrl->config;

	LOG_ERR("Async transfer timed out: %s", xdat->ctrl->name);

	/* Stop the controller and abort the DMA channel if armed. SRST is
	 * self-clearing; mode register settings (CKDIV/CPP) are re-applied on
	 * the next dev_cfg or by apply_dev_config().
	 */
	QW32(xcfg, XEC_QSPI_IER_OFS, 0);
	QW32(xcfg, XEC_QSPI_EXE_OFS, BIT(XEC_QSPI_EXE_STOP_POS));
	QW32(xcfg, XEC_QSPI_MODE_OFS, BIT(XEC_QSPI_MODE_SRST_POS));
	while (QR32(xcfg, XEC_QSPI_MODE_OFS) & BIT(XEC_QSPI_MODE_SRST_POS)) {
		/* spin; reset is fast */
	}
	QW32(xcfg, XEC_QSPI_MODE_OFS, BIT(XEC_QSPI_MODE_ACTV_POS));
	apply_dev_config(xdat->ctrl);
	soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);

	try_stop_dma(xdat);

	xdat->ctx.err = -ETIMEDOUT;
	fire_controller_cb(xdat, MSPI_BUS_TIMEOUT, -ETIMEDOUT);
	release_ctx(xdat->ctrl);
}

/* Per-packet work: invoked from ISR's signal_packet_done() in the async path.
 * Decides whether to advance to the next packet, finalize, or report error.
 * Runs in syswq context — safe to call dma_config / fire callbacks.
 */
static void async_pkt_work_handler(struct k_work *work)
{
	struct mspi_xec_gp_xdat *const xdat =
		CONTAINER_OF(work, struct mspi_xec_gp_xdat, async_pkt_work);
	struct mspi_xec_context *ctx = &xdat->ctx;
	int rc;

	/* Error latched by ISR during the prior packet — terminate the xfer. */
	if (ctx->err != 0) {
		finalize_xfer_async(xdat, ctx->err);
		return;
	}

	/* The packet that just completed (or was skipped as empty) was at
	 * ctx->pkt_idx. If it was the last, finalize WITHOUT advancing — the
	 * MSPI_BUS_XFER_COMPLETE callback's evt_data.packet_idx must reference
	 * the last packet that actually completed (num_packet - 1), not one
	 * past it.
	 */
	if (ctx->pkt_idx + 1 >= ctx->xfer.num_packet) {
		finalize_xfer_async(xdat, 0);
		return;
	}

	ctx->pkt_idx++;

	rc = build_gpspi_pkt(xdat->ctrl, ctx->pkt_idx);
	if (rc != 0) {
		LOG_ERR("Build GP-SPI pkt %u error (%d)", ctx->pkt_idx, rc);
		finalize_xfer_async(xdat, rc);
		return;
	}

	/* Empty packet: skip and re-submit ourselves to advance again. */
	if (ctx->gpkt.cr_bm == 0) {
		k_work_submit(&xdat->async_pkt_work);
		return;
	}

	rc = start_gpkt_phase(xdat->ctrl);
	if (rc != 0) {
		finalize_xfer_async(xdat, rc);
	}
}
#endif

/* GP-SPI interrupt service routine.
 *
 * Drives the per-packet state machine:
 *   - Hard error (TXB/RXB/Prog): latch ctx->err = -EIO, signal end-of-packet.
 *   - PIO data slot: refill TX FIFO on TXB_EMPTY, drain RX FIFO on RXB_FULL.
 *   - XFR_DONE: drain RX tail bytes (PIO RX); if data slot has rem_nbytes,
 *     rebuild the data CR for the next chunk (DMA stays armed and resumes
 *     when GP-SPI restarts); else advance to the next populated CR slot.
 *     When the last slot of a packet completes, route via signal_packet_done()
 *     to the sync caller (xfer_done sem) or async work handler.
 */
static void mspi_mchp_xec_gpspi_isr(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;
	struct mspi_xec_context *ctx = &xdat->ctx;
	struct mspi_xec_gpspi_pkt *gpkt = &ctx->gpkt;
	uint32_t qsr = QR32(xcfg, XEC_QSPI_SR_OFS);
	bool is_dat = (gpkt->cr_idx == MSPI_XEC_GPKT_CR_DAT_IDX);
	bool is_tx = (gpkt->flags & MSPI_XEC_GPKT_FLAG_DIR_WR);
	bool is_pio = !ctx->req_dma;
	int next_slot;

#ifdef XEC_GPSPI_DEBUG_ISR
	xdat->qsr = qsr;
	xdat->qbcntsr = QR32(xcfg, XEC_QSPI_BCNT_SR_OFS);
	xdat->qcr = QR32(xcfg, XEC_QSPI_CR_OFS);
	xdat->qier = QR32(xcfg, XEC_QSPI_IER_OFS);
#endif
	QW32(xcfg, XEC_QSPI_IER_OFS, 0);

	/* Defensive: nothing was scheduled (cr_bm == 0) or cr_idx is out of range. */
	if ((gpkt->cr_bm == 0) || (gpkt->cr_idx >= MSPI_XEC_GPKT_CR_MAX_IDX)) {
		QW32(xcfg, XEC_QSPI_SR_OFS, qsr);
		soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);
		return;
	}

	/* Hard error: latch and end the packet. */
	if (qsr & GPSPI_SR_ERR_MSK) {
		ctx->err = -EIO;
		QW32(xcfg, XEC_QSPI_SR_OFS, qsr);
		soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);
		signal_packet_done(xdat);
		return;
	}

	/* PIO data movement (only meaningful in the data slot). */
	if (is_dat && is_pio) {
		if (is_tx && (qsr & BIT(XEC_QSPI_SR_TXB_EMPTY_POS))) {
			pio_tx_fill(xcfg, ctx);
		} else if (!is_tx && (qsr & BIT(XEC_QSPI_SR_RXB_FULL_POS))) {
			pio_rx_drain(xcfg, ctx);
		}
	}

	if (qsr & BIT(XEC_QSPI_SR_XFR_DONE_POS)) {
		/* Drain RX tail bytes that arrived between the last RXB_FULL edge
		 * and this XFR_DONE.
		 */
		if (is_dat && is_pio && !is_tx) {
			pio_rx_drain(xcfg, ctx);
		}

		QW32(xcfg, XEC_QSPI_SR_OFS, qsr);
		soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);

		/* Chunk continuation within the data phase: rebuild CR with the
		 * next NQUNITS slice. PIO needs pio_pos reset and FIFO preload
		 * (handled inside start_gpkt_phase). DMA stops cleanly at the
		 * end of each chunk's block_size; we re-arm the channel here via
		 * dma_reload (direction is unchanged so no full reconfigure
		 * needed), and start_gpkt_phase will dma_start it.
		 */
		if (is_dat && (gpkt->rem_nbytes > 0)) {
			uint32_t next = MIN(gpkt->rem_nbytes, GPSPI_ALIGNED_NQUNITS_MAX);

			gpkt->cr[MSPI_XEC_GPKT_CR_DAT_IDX] =
				(gpkt->cr[MSPI_XEC_GPKT_CR_DAT_IDX] & ~XEC_QSPI_CR_NQUNITS_MSK) |
				XEC_GPSPI_CR_NQUNITS_SET(next);

			/* Final chunk: deassert CE on completion unless held. */
			if ((gpkt->rem_nbytes <= GPSPI_ALIGNED_NQUNITS_MAX) &&
			    (ctx->xfer.hold_ce == false)) {
				gpkt->cr[MSPI_XEC_GPKT_CR_DAT_IDX] |= BIT(XEC_QSPI_CR_CLOSE_EN_POS);
			}

			gpkt->data_ptr = gpkt->rem_data_ptr;
			gpkt->data_nbytes = next;
			gpkt->rem_data_ptr += next;
			gpkt->rem_nbytes -= next;

#ifdef CONFIG_MSPI_DMA
			if (ctx->req_dma) {
				int rc;

				if (is_tx) {
					rc = dma_reload(xcfg->dma_dev, gpkt->dma_chan,
							(uint32_t)gpkt->data_ptr,
							(uint32_t)xcfg->regs + XEC_QSPI_TXB_OFS,
							gpkt->data_nbytes);
				} else {
					rc = dma_reload(xcfg->dma_dev, gpkt->dma_chan,
							(uint32_t)xcfg->regs + XEC_QSPI_RXB_OFS,
							(uint32_t)gpkt->data_ptr,
							gpkt->data_nbytes);
				}
				if (rc != 0) {
					LOG_ERR("DMA reload error (%d)", rc);
					ctx->err = rc;
					signal_packet_done(xdat);
					return;
				}
			}
#endif

			(void)start_gpkt_phase(ctrl);
			return;
		}

		/* Slot done; advance to the next populated slot, or finalize. */
		next_slot = next_cr_slot(gpkt->cr_bm, gpkt->cr_idx + 1);
		if (next_slot < MSPI_XEC_GPKT_CR_MAX_IDX) {
			gpkt->cr_idx = (uint8_t)next_slot;
			(void)start_gpkt_phase(ctrl);
			return;
		}

		signal_packet_done(xdat);
		return;
	}

	/* No XFR_DONE — only PIO FIFO maintenance happened. Re-arm IER. */
	if (is_dat && is_pio) {
		uint32_t ier = (BIT(XEC_QSPI_IER_XFR_DONE_POS) | BIT(XEC_QSPI_IER_TXB_ERR_POS) |
				BIT(XEC_QSPI_IER_RXB_ERR_POS) | BIT(XEC_QSPI_IER_PROG_ERR_POS));

		if (is_tx) {
			if (ctx->pio_pos < gpkt->data_nbytes) {
				ier |= BIT(XEC_QSPI_IER_TXB_EMPTY_POS);
			}
		} else {
			ier |= BIT(XEC_QSPI_IER_RXB_FULL_POS);
		}

		QW32(xcfg, XEC_QSPI_SR_OFS, qsr);
		soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);
		QW32(xcfg, XEC_QSPI_IER_OFS, ier);
		return;
	}

	/* Spurious / no actionable bits — clear and exit. */
	QW32(xcfg, XEC_QSPI_SR_OFS, qsr);
	soc_ecia_girq_status_clear(xcfg->girq, xcfg->girq_pos);
}

/* -------- Driver power management and initialization -------- */
static int mspi_mchp_xec_gp_pm_off(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	int rc = 0;

	QW32(xcfg, XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_SRST_POS);
	while (QR32(xcfg, XEC_QSPI_MODE_OFS) & BIT(XEC_QSPI_MODE_SRST_POS)) {
		/* spin; reset is fast */
	}

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_DEFAULT);

	return rc;
}

static int mspi_mchp_xec_gp_pm_on(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	const struct mspi_dt_spec spec = {
		.bus = ctrl,
		.config = xcfg->mspicfg,
	};
	int rc = 0;

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc != 0) {
		return rc;
	}

	rc = api_mspi_mchp_xec_gpspi_cfg(&spec);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

/* QSPI is not a wake source. We apply default pin state and then set the
 * block's activate bit. This presumes the SoC did not lose context in suspend.
 */
static int mspi_mchp_xec_gp_pm_resume(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
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
static int mspi_mchp_xec_gp_pm_suspend(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	int rc = 0;

	QCB32(xcfg, XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_ACTV_POS);

	rc = pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_SLEEP);
	if (rc == -ENOENT) { /* pinctrl-1 is not present? */
		rc = 0;
	}

	return rc;
}

static int mspi_mchp_xec_gp_pm_action_cb(const struct device *ctrl, enum pm_device_action action)
{
	int rc = 0;

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		rc = mspi_mchp_xec_gp_pm_suspend(ctrl);
	} else if (action == PM_DEVICE_ACTION_RESUME) {
		rc = mspi_mchp_xec_gp_pm_resume(ctrl);
	} else if (action == PM_DEVICE_ACTION_TURN_OFF) {
		rc = mspi_mchp_xec_gp_pm_off(ctrl);
	} else if (action == PM_DEVICE_ACTION_TURN_ON) {
		rc = mspi_mchp_xec_gp_pm_on(ctrl);
	} else {
		rc = -ENOTSUP;
	}

	return rc;
}

/* -------- Driver init and de-init -------- */
static int mspi_xec_gpspi_init(const struct device *ctrl)
{
	const struct mspi_xec_gp_xcfg *xcfg = ctrl->config;
	struct mspi_xec_gp_xdat *const xdat = ctrl->data;
	int rc = 0;

	xdat->ctrl = ctrl;
	xdat->active_mspicfg = xcfg->mspicfg;

#ifdef CONFIG_MULTITHREADING
	k_mutex_init(&xdat->lock);
	k_sem_init(&xdat->xfer_done, 0, 1);
	k_sem_init(&xdat->ctx.lock, 1, 1);
	k_timer_init(&xdat->async_timer, async_tmout_timer_handler, NULL);
	k_work_init(&xdat->async_pkt_work, async_pkt_work_handler);
	k_work_init(&xdat->async_tmout_work, async_tmout_work_handler);
#endif

	rc = pm_device_driver_init(ctrl, mspi_mchp_xec_gp_pm_action_cb);
	if (rc != 0) {
		return rc;
	}

	if (xcfg->irq_cfg_func != NULL) {
		xcfg->irq_cfg_func();
		soc_ecia_girq_ctrl(xcfg->girq, xcfg->girq_pos, 1U);
	}

	return 0;
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int mspi_xec_gpspi_deinit(const struct device *ctrl)
{
	return pm_device_driver_deinit(ctrl, mspi_mchp_xec_gp_pm_action_cb);
}
#endif

static DEVICE_API(mspi, mspi_mchp_xec_gpspi_api) = {
	.config = api_mspi_mchp_xec_gpspi_cfg,
	.dev_config = api_mspi_mchp_xec_gpspi_dev_cfg,
	.get_channel_status = api_mspi_mchp_xec_gpspi_get_chs,
	.transceive = api_mspi_mchp_xec_gpspi_tc,
#ifdef CONFIG_MULTITHREADING
	.register_callback = api_mspi_mchp_xec_gpspi_rcb,
#endif
};

#define MSPI_MCHP_XEC_GPSPI_MSPI_CFG(inst)                                                         \
	{                                                                                          \
		.channel_num = 0,                                                                  \
		.op_mode = DT_INST_ENUM_IDX(inst, op_mode),                                        \
		.duplex = DT_INST_ENUM_IDX(inst, duplex),                                          \
		.dqs_support = DT_INST_PROP(inst, dqs_support),                                    \
		.sw_multi_periph = DT_INST_PROP(inst, software_multiperipheral),                   \
		.ce_group = NULL,                                                                  \
		.num_ce_gpios = 0,                                                                 \
		.num_periph = 1U,                                                                  \
		.max_freq = DT_INST_PROP_OR(inst, clock_frequency, MHZ(12)),                       \
		.re_init = true,                                                                   \
	}

#ifdef CONFIG_MSPI_DMA

#define XEC_GPSPI_DMA_NODE(i, name)    DT_INST_DMAS_CTLR_BY_NAME(i, name)
#define XEC_GPSPI_DMA_DEVICE(i, name)  DEVICE_DT_GET(XEC_GPSPI_DMA_NODE(i, name))
#define XEC_GPSPI_DMA_CHAN(i, name)    DT_INST_DMAS_CELL_BY_NAME(i, name, channel)
#define XEC_GPSPI_DMA_TRIGSRC(i, name) DT_INST_DMAS_CELL_BY_NAME(i, name, trigsrc)

/* DMA device is the same central DMA controller for tx and rx */
#define XEC_GPSPI_DMA_CONFIG(inst)                                                                 \
	.dma_dev = XEC_GPSPI_DMA_DEVICE(inst, tx), .dma_tx_chan = XEC_GPSPI_DMA_CHAN(inst, tx),    \
	.dma_tx_slot = XEC_GPSPI_DMA_TRIGSRC(inst, tx),                                            \
	.dma_rx_chan = XEC_GPSPI_DMA_CHAN(inst, rx),                                               \
	.dma_rx_slot = XEC_GPSPI_DMA_TRIGSRC(inst, rx),
#else
#define XEC_GPSPI_DMA_CONFIG(inst)
#endif

#define XEC_GPSPI_GIRQ_DT(inst)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0))
#define XEC_GPSPI_GIRQ_POS_DT(inst) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, 0))

#define MSPI_MCHP_XEC_GPSPI_DEVICE_DEF(inst)                                                       \
	PM_DEVICE_DT_INST_DEFINE(inst, mspi_mchp_xec_gp_pm_action_cb);                             \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static void mspi_mchp_xec_gpspi_irq_cfg_##inst(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    mspi_mchp_xec_gpspi_isr, DEVICE_DT_INST_GET(inst), 0);                 \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	static struct mspi_xec_gp_xdat mspi_xec_gp_xdat_##inst;                                    \
	static const struct mspi_xec_gp_xcfg mspi_xec_gp_xcfg_##inst = {                           \
		.regs = (mm_reg_t)DT_INST_REG_ADDR(inst),                                          \
		.irq_cfg_func = mspi_mchp_xec_gpspi_irq_cfg_##inst,                                \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.mspicfg = MSPI_MCHP_XEC_GPSPI_MSPI_CFG(inst),                                     \
		.girq = (uint8_t)XEC_GPSPI_GIRQ_DT(inst),                                          \
		.girq_pos = (uint8_t)XEC_GPSPI_GIRQ_POS_DT(inst),                                  \
		.pcr_enc = (uint8_t)DT_INST_PROP(inst, pcr_scr),                                   \
		XEC_GPSPI_DMA_CONFIG(inst)};                                                       \
	DEVICE_DT_INST_DEINIT_DEFINE(inst, mspi_xec_gpspi_init, mspi_xec_gpspi_deinit,             \
				     PM_DEVICE_DT_INST_GET(inst), &mspi_xec_gp_xdat_##inst,        \
				     &mspi_xec_gp_xcfg_##inst, POST_KERNEL,                        \
				     CONFIG_MSPI_INIT_PRIORITY, &mspi_mchp_xec_gpspi_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_MCHP_XEC_GPSPI_DEVICE_DEF)
