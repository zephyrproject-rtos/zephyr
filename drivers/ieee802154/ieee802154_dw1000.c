/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dw1000, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/random/random.h>
#include <zephyr/debug/stack.h>
#include <math.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/net/ieee802154_radio.h>
#include "ieee802154_dw1000_regs.h"

#define DT_DRV_COMPAT decawave_dw1000

#define DWT_FCS_LENGTH			2U
#define DWT_SPI_CSWAKEUP_FREQ		500000U
#define DWT_SPI_SLOW_FREQ		2000000U
#define DWT_SPI_TRANS_MAX_HDR_LEN	3
#define DWT_SPI_TRANS_REG_MAX_RANGE	0x3F
#define DWT_SPI_TRANS_SHORT_MAX_OFFSET	0x7F
#define DWT_SPI_TRANS_WRITE_OP		BIT(7)
#define DWT_SPI_TRANS_SUB_ADDR		BIT(6)
#define DWT_SPI_TRANS_EXTEND_ADDR	BIT(7)

#define DWT_TS_TIME_UNITS_FS		15650U /* DWT_TIME_UNITS in fs */

#define DW1000_TX_ANT_DLY		16450
#define DW1000_RX_ANT_DLY		16450

/* SHR Symbol Duration in ns */
#define UWB_PHY_TPSYM_PRF64		IEEE802154_PHY_HRP_UWB_PRF64_TPSYM_SYMBOL_PERIOD_NS
#define UWB_PHY_TPSYM_PRF16		IEEE802154_PHY_HRP_UWB_PRF16_TPSYM_SYMBOL_PERIOD_NS

#define UWB_PHY_NUMOF_SYM_SHR_SFD	8

/* PHR Symbol Duration Tdsym in ns */
#define UWB_PHY_TDSYM_PHR_110K		8205.13
#define UWB_PHY_TDSYM_PHR_850K		1025.64
#define UWB_PHY_TDSYM_PHR_6M8		1025.64

#define UWB_PHY_NUMOF_SYM_PHR		18

/* Data Symbol Duration Tdsym in ns */
#define UWB_PHY_TDSYM_DATA_110K		8205.13
#define UWB_PHY_TDSYM_DATA_850K		1025.64
#define UWB_PHY_TDSYM_DATA_6M8		128.21

#define DWT_WORK_QUEUE_STACK_SIZE	512

static struct k_work_q dwt_work_queue;
static K_KERNEL_STACK_DEFINE(dwt_work_queue_stack,
			     DWT_WORK_QUEUE_STACK_SIZE);

struct dwt_phy_config {
	uint8_t channel;	/* Channel 1, 2, 3, 4, 5, 7 */
	uint8_t dr;	/* Data rate DWT_BR_110K, DWT_BR_850K, DWT_BR_6M8 */
	uint8_t prf;	/* PRF DWT_PRF_16M or DWT_PRF_64M */

	uint8_t rx_pac_l;		/* DWT_PAC8..DWT_PAC64 */
	uint8_t rx_shr_code;	/* RX SHR preamble code */
	uint8_t rx_ns_sfd;		/* non-standard SFD */
	uint16_t rx_sfd_to;	/* SFD timeout value (in symbols)
				 * (tx_shr_nsync + 1 + SFD_length - rx_pac_l)
				 */

	uint8_t tx_shr_code;	/* TX SHR preamble code */
	uint32_t tx_shr_nsync;	/* PLEN index, e.g. DWT_PLEN_64 */

	float t_shr;
	float t_phr;
	float t_dsym;
};

struct dwt_hi_cfg {
	struct spi_dt_spec bus;
	struct gpio_dt_spec irq_gpio;
	struct gpio_dt_spec rst_gpio;
};

#define DWT_STATE_TX		0
#define DWT_STATE_CCA		1
#define DWT_STATE_RX_DEF_ON	2

struct dwt_context {
	const struct device *dev;
	struct net_if *iface;
	const struct spi_config *spi_cfg;
	struct spi_config spi_cfg_slow;
	struct gpio_callback gpio_cb;
	struct k_sem dev_lock;
	struct k_sem phy_sem;
	struct k_work irq_cb_work;
	struct k_thread thread;
	struct dwt_phy_config rf_cfg;
	atomic_t state;
	bool cca_busy;
	uint16_t sleep_mode;
	uint8_t mac_addr[8];
};

static const struct dwt_hi_cfg dw1000_0_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8), 0),
	.irq_gpio = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.rst_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

static struct dwt_context dwt_0_context = {
	.dev_lock = Z_SEM_INITIALIZER(dwt_0_context.dev_lock, 1,  1),
	.phy_sem = Z_SEM_INITIALIZER(dwt_0_context.phy_sem, 0,  1),
	.rf_cfg = {
		.channel = 5,
		.dr = DWT_BR_6M8,
		.prf = DWT_PRF_64M,

		.rx_pac_l = DWT_PAC8,
		.rx_shr_code = 10,
		.rx_ns_sfd = 0,
		.rx_sfd_to = (129 + 8 - 8),

		.tx_shr_code = 10,
		.tx_shr_nsync = DWT_PLEN_128,
	},
};

/* This struct is used to read all additional RX frame info at one push */
struct dwt_rx_info_regs {
	uint8_t rx_fqual[DWT_RX_FQUAL_LEN];
	uint8_t rx_ttcki[DWT_RX_TTCKI_LEN];
	uint8_t rx_ttcko[DWT_RX_TTCKO_LEN];
	/* RX_TIME without RX_RAWST */
	uint8_t rx_time[DWT_RX_TIME_FP_RAWST_OFFSET];
} _packed;

static int dwt_configure_rf_phy(const struct device *dev);

static int dwt_spi_read(const struct device *dev,
			uint16_t hdr_len, const uint8_t *hdr_buf,
			uint32_t data_len, uint8_t *data)
{
	struct dwt_context *ctx = dev->data;
	const struct dwt_hi_cfg *hi_cfg = dev->config;
	const struct spi_buf tx_buf = {
		.buf = (uint8_t *)hdr_buf,
		.len = hdr_len
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = hdr_len,
		},
		{
			.buf = (uint8_t *)data,
			.len = data_len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	LOG_DBG("spi read, header length %u, data length %u",
		(uint16_t)hdr_len, (uint32_t)data_len);
	LOG_HEXDUMP_DBG(hdr_buf, (uint16_t)hdr_len, "rd: header");

	if (spi_transceive(hi_cfg->bus.bus, ctx->spi_cfg, &tx, &rx)) {
		LOG_ERR("SPI transfer failed");
		return -EIO;
	}

	LOG_HEXDUMP_DBG(data, (uint32_t)data_len, "rd: data");

	return 0;
}


static int dwt_spi_write(const struct device *dev,
			 uint16_t hdr_len, const uint8_t *hdr_buf,
			 uint32_t data_len, const uint8_t *data)
{
	struct dwt_context *ctx = dev->data;
	const struct dwt_hi_cfg *hi_cfg = dev->config;
	struct spi_buf buf[2] = {
		{.buf = (uint8_t *)hdr_buf, .len = hdr_len},
		{.buf = (uint8_t *)data, .len = data_len}
	};
	struct spi_buf_set buf_set = {.buffers = buf, .count = 2};

	LOG_DBG("spi write, header length %u, data length %u",
		(uint16_t)hdr_len, (uint32_t)data_len);
	LOG_HEXDUMP_DBG(hdr_buf, (uint16_t)hdr_len, "wr: header");
	LOG_HEXDUMP_DBG(data, (uint32_t)data_len, "wr: data");

	if (spi_write(hi_cfg->bus.bus, ctx->spi_cfg, &buf_set)) {
		LOG_ERR("SPI read failed");
		return -EIO;
	}

	return 0;
}

/* See 2.2.1.2 Transaction formats of the SPI interface */
static int dwt_spi_transfer(const struct device *dev,
			    uint8_t reg, uint16_t offset,
			    size_t buf_len, uint8_t *buf, bool write)
{
	uint8_t hdr[DWT_SPI_TRANS_MAX_HDR_LEN] = {0};
	size_t hdr_len = 0;

	hdr[0] = reg & DWT_SPI_TRANS_REG_MAX_RANGE;
	hdr_len += 1;

	if (offset != 0) {
		hdr[0] |= DWT_SPI_TRANS_SUB_ADDR;
		hdr[1] = (uint8_t)offset & DWT_SPI_TRANS_SHORT_MAX_OFFSET;
		hdr_len += 1;

		if (offset > DWT_SPI_TRANS_SHORT_MAX_OFFSET) {
			hdr[1] |= DWT_SPI_TRANS_EXTEND_ADDR;
			hdr[2] =  (uint8_t)(offset >> 7);
			hdr_len += 1;
		}

	}

	if (write) {
		hdr[0] |= DWT_SPI_TRANS_WRITE_OP;
		return dwt_spi_write(dev, hdr_len, hdr, buf_len, buf);
	} else {
		return dwt_spi_read(dev, hdr_len, hdr, buf_len, buf);
	}
}

static int dwt_register_read(const struct device *dev,
			     uint8_t reg, uint16_t offset, size_t buf_len, uint8_t *buf)
{
	return dwt_spi_transfer(dev, reg, offset, buf_len, buf, false);
}

static int dwt_register_write(const struct device *dev,
			      uint8_t reg, uint16_t offset, size_t buf_len, uint8_t *buf)
{
	return dwt_spi_transfer(dev, reg, offset, buf_len, buf, true);
}

static inline uint32_t dwt_reg_read_u32(const struct device *dev,
				     uint8_t reg, uint16_t offset)
{
	uint8_t buf[sizeof(uint32_t)];

	dwt_spi_transfer(dev, reg, offset, sizeof(buf), buf, false);

	return sys_get_le32(buf);
}

static inline uint16_t dwt_reg_read_u16(const struct device *dev,
				     uint8_t reg, uint16_t offset)
{
	uint8_t buf[sizeof(uint16_t)];

	dwt_spi_transfer(dev, reg, offset, sizeof(buf), buf, false);

	return sys_get_le16(buf);
}

static inline uint8_t dwt_reg_read_u8(const struct device *dev,
				   uint8_t reg, uint16_t offset)
{
	uint8_t buf;

	dwt_spi_transfer(dev, reg, offset, sizeof(buf), &buf, false);

	return buf;
}

static inline void dwt_reg_write_u32(const struct device *dev,
				     uint8_t reg, uint16_t offset, uint32_t val)
{
	uint8_t buf[sizeof(uint32_t)];

	sys_put_le32(val, buf);
	dwt_spi_transfer(dev, reg, offset, sizeof(buf), buf, true);
}

static inline void dwt_reg_write_u16(const struct device *dev,
				     uint8_t reg, uint16_t offset, uint16_t val)
{
	uint8_t buf[sizeof(uint16_t)];

	sys_put_le16(val, buf);
	dwt_spi_transfer(dev, reg, offset, sizeof(buf), buf, true);
}

static inline void dwt_reg_write_u8(const struct device *dev,
				    uint8_t reg, uint16_t offset, uint8_t val)
{
	dwt_spi_transfer(dev, reg, offset, sizeof(uint8_t), &val, true);
}

static ALWAYS_INLINE void dwt_setup_int(const struct device *dev,
					bool enable)
{
	const struct dwt_hi_cfg *hi_cfg = dev->config;

	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&hi_cfg->irq_gpio, flags);
}

static void dwt_reset_rfrx(const struct device *dev)
{
	/*
	 * Apply a receiver-only soft reset,
	 * see SOFTRESET field description in DW1000 User Manual.
	 */
	dwt_reg_write_u8(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_SOFTRESET_OFFSET,
			 DWT_PMSC_CTRL0_RESET_RX);
	dwt_reg_write_u8(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_SOFTRESET_OFFSET,
			 DWT_PMSC_CTRL0_RESET_CLEAR);
}

static void dwt_disable_txrx(const struct device *dev)
{
	dwt_setup_int(dev, false);

	dwt_reg_write_u8(dev, DWT_SYS_CTRL_ID, DWT_SYS_CTRL_OFFSET,
			 DWT_SYS_CTRL_TRXOFF);

	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, DWT_SYS_STATUS_OFFSET,
			  (DWT_SYS_STATUS_ALL_RX_GOOD |
			   DWT_SYS_STATUS_ALL_RX_TO |
			   DWT_SYS_STATUS_ALL_RX_ERR |
			   DWT_SYS_STATUS_ALL_TX));

	dwt_setup_int(dev, true);
}

/* timeout time in units of 1.026 microseconds */
static int dwt_enable_rx(const struct device *dev, uint16_t timeout)
{
	uint32_t sys_cfg;
	uint16_t sys_ctrl = DWT_SYS_CTRL_RXENAB;

	sys_cfg = dwt_reg_read_u32(dev, DWT_SYS_CFG_ID, 0);

	if (timeout != 0) {
		dwt_reg_write_u16(dev, DWT_RX_FWTO_ID, DWT_RX_FWTO_OFFSET,
				  timeout);
		sys_cfg |= DWT_SYS_CFG_RXWTOE;
	} else {
		sys_cfg &= ~DWT_SYS_CFG_RXWTOE;
	}

	dwt_reg_write_u32(dev, DWT_SYS_CFG_ID, 0, sys_cfg);
	dwt_reg_write_u16(dev, DWT_SYS_CTRL_ID, DWT_SYS_CTRL_OFFSET, sys_ctrl);

	return 0;
}

static inline void dwt_irq_handle_rx_cca(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;

	k_sem_give(&ctx->phy_sem);
	ctx->cca_busy = true;

	/* Clear all RX event bits */
	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, 0,
			  DWT_SYS_STATUS_ALL_RX_GOOD);
}

static inline void dwt_irq_handle_rx(const struct device *dev, uint32_t sys_stat)
{
	struct dwt_context *ctx = dev->data;
	struct net_pkt *pkt = NULL;
	struct dwt_rx_info_regs rx_inf_reg;
	float a_const;
	uint32_t rx_finfo;
	uint32_t ttcki;
	uint32_t rx_pacc;
	uint32_t cir_pwr;
	uint32_t flags_to_clear;
	int32_t ttcko;
	uint16_t pkt_len;
	uint8_t *fctrl;
	int8_t rx_level = INT8_MIN;

	LOG_DBG("RX OK event, SYS_STATUS 0x%08x", sys_stat);
	flags_to_clear = sys_stat & DWT_SYS_STATUS_ALL_RX_GOOD;

	rx_finfo = dwt_reg_read_u32(dev, DWT_RX_FINFO_ID, DWT_RX_FINFO_OFFSET);
	pkt_len = rx_finfo & DWT_RX_FINFO_RXFLEN_MASK;
	rx_pacc = (rx_finfo & DWT_RX_FINFO_RXPACC_MASK) >>
		   DWT_RX_FINFO_RXPACC_SHIFT;

	if (!(IS_ENABLED(CONFIG_IEEE802154_RAW_MODE))) {
		pkt_len -= DWT_FCS_LENGTH;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, pkt_len,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("No buf available");
		goto rx_out_enable_rx;
	}

	dwt_register_read(dev, DWT_RX_BUFFER_ID, 0, pkt_len, pkt->buffer->data);
	dwt_register_read(dev, DWT_RX_FQUAL_ID, 0, sizeof(rx_inf_reg),
			  (uint8_t *)&rx_inf_reg);
	net_buf_add(pkt->buffer, pkt_len);
	fctrl = pkt->buffer->data;

	/*
	 * Get Ranging tracking offset and tracking interval
	 * for Crystal characterization
	 */
	ttcki = sys_get_le32(rx_inf_reg.rx_ttcki);
	ttcko = sys_get_le32(rx_inf_reg.rx_ttcko) & DWT_RX_TTCKO_RXTOFS_MASK;
	/* Tracking offset value is a 19-bit signed integer */
	if (ttcko & BIT(18)) {
		ttcko |= ~DWT_RX_TTCKO_RXTOFS_MASK;
	}

	/* TODO add:
	 * net_pkt_set_ieee802154_tcki(pkt, ttcki);
	 * net_pkt_set_ieee802154_tcko(pkt, ttcko);
	 */
	LOG_DBG("ttcko %d ttcki: 0x%08x", ttcko, ttcki);

	if (IS_ENABLED(CONFIG_NET_PKT_TIMESTAMP)) {
		uint8_t ts_buf[sizeof(uint64_t)] = {0};
		uint64_t ts_nsec;

		memcpy(ts_buf, rx_inf_reg.rx_time, DWT_RX_TIME_RX_STAMP_LEN);
		ts_nsec = (sys_get_le64(ts_buf) * DWT_TS_TIME_UNITS_FS) / 1000000U;
		net_pkt_set_timestamp_ns(pkt, ts_nsec);
	}

	/* See 4.7.2 Estimating the receive signal power */
	cir_pwr = sys_get_le16(&rx_inf_reg.rx_fqual[6]);
	if (ctx->rf_cfg.prf == DWT_PRF_16M) {
		a_const = DWT_RX_SIG_PWR_A_CONST_PRF16;
	} else {
		a_const = DWT_RX_SIG_PWR_A_CONST_PRF64;
	}

	if (rx_pacc != 0) {
#if defined(CONFIG_NEWLIB_LIBC)
		/* From 4.7.2 Estimating the receive signal power */
		rx_level = 10.0 * log10f(cir_pwr * BIT(17) /
					 (rx_pacc * rx_pacc)) - a_const;
#endif
	}

	net_pkt_set_ieee802154_rssi_dbm(pkt, rx_level);

	/*
	 * Workaround for AAT status bit issue,
	 * From 5.3.5 Host Notification in DW1000 User Manual:
	 * "Note: there is a situation that can result in the AAT bit being set
	 * for the current frame as a result of a previous frame that was
	 * received and rejected due to frame filtering."
	 */
	if ((sys_stat & DWT_SYS_STATUS_AAT) && ((fctrl[0] & 0x20) == 0)) {
		flags_to_clear |= DWT_SYS_STATUS_AAT;
	}

	if (ieee802154_handle_ack(ctx->iface, pkt) == NET_OK) {
		LOG_INF("ACK packet handled");
		goto rx_out_unref_pkt;
	}

	/* LQI not implemented */
	LOG_DBG("Caught a packet (%u) (RSSI: %d)", pkt_len, rx_level);
	LOG_HEXDUMP_DBG(pkt->buffer->data, pkt_len, "RX buffer:");

	if (net_recv_data(ctx->iface, pkt) == NET_OK) {
		goto rx_out_enable_rx;
	} else {
		LOG_DBG("Packet dropped by NET stack");
	}

rx_out_unref_pkt:
	if (pkt) {
		net_pkt_unref(pkt);
	}

rx_out_enable_rx:
	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, 0, flags_to_clear);
	LOG_DBG("Cleared SYS_STATUS flags 0x%08x", flags_to_clear);
	if (atomic_test_bit(&ctx->state, DWT_STATE_RX_DEF_ON)) {
		/*
		 * Re-enable reception but in contrast to dwt_enable_rx()
		 * without to read SYS_STATUS and set delayed option.
		 */
		dwt_reg_write_u16(dev, DWT_SYS_CTRL_ID, DWT_SYS_CTRL_OFFSET,
				  DWT_SYS_CTRL_RXENAB);
	}
}

static void dwt_irq_handle_tx(const struct device *dev, uint32_t sys_stat)
{
	struct dwt_context *ctx = dev->data;

	/* Clear TX event bits */
	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, 0,
			  DWT_SYS_STATUS_ALL_TX);

	LOG_DBG("TX confirmed event");
	k_sem_give(&ctx->phy_sem);
}

static void dwt_irq_handle_rxto(const struct device *dev, uint32_t sys_stat)
{
	struct dwt_context *ctx = dev->data;

	/* Clear RX timeout event bits */
	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, 0,
			  DWT_SYS_STATUS_RXRFTO);

	dwt_disable_txrx(dev);
	/* Receiver reset necessary, see 4.1.6 RX Message timestamp */
	dwt_reset_rfrx(dev);

	LOG_DBG("RX timeout event");

	if (atomic_test_bit(&ctx->state, DWT_STATE_CCA)) {
		k_sem_give(&ctx->phy_sem);
		ctx->cca_busy = false;
	}
}

static void dwt_irq_handle_error(const struct device *dev, uint32_t sys_stat)
{
	struct dwt_context *ctx = dev->data;

	/* Clear RX error event bits */
	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, 0, DWT_SYS_STATUS_ALL_RX_ERR);

	dwt_disable_txrx(dev);
	/* Receiver reset necessary, see 4.1.6 RX Message timestamp */
	dwt_reset_rfrx(dev);

	LOG_INF("RX error event");
	if (atomic_test_bit(&ctx->state, DWT_STATE_CCA)) {
		k_sem_give(&ctx->phy_sem);
		ctx->cca_busy = true;
		return;
	}

	if (atomic_test_bit(&ctx->state, DWT_STATE_RX_DEF_ON)) {
		dwt_enable_rx(dev, 0);
	}
}

static void dwt_irq_work_handler(struct k_work *item)
{
	struct dwt_context *ctx = CONTAINER_OF(item, struct dwt_context,
					       irq_cb_work);
	const struct device *dev = ctx->dev;
	uint32_t sys_stat;

	k_sem_take(&ctx->dev_lock, K_FOREVER);

	sys_stat = dwt_reg_read_u32(dev, DWT_SYS_STATUS_ID, 0);

	if (sys_stat & DWT_SYS_STATUS_RXFCG) {
		if (atomic_test_bit(&ctx->state, DWT_STATE_CCA)) {
			dwt_irq_handle_rx_cca(dev);
		} else {
			dwt_irq_handle_rx(dev, sys_stat);
		}
	}

	if (sys_stat & DWT_SYS_STATUS_TXFRS) {
		dwt_irq_handle_tx(dev, sys_stat);
	}

	if (sys_stat & DWT_SYS_STATUS_ALL_RX_TO) {
		dwt_irq_handle_rxto(dev, sys_stat);
	}

	if (sys_stat & DWT_SYS_STATUS_ALL_RX_ERR) {
		dwt_irq_handle_error(dev, sys_stat);
	}

	k_sem_give(&ctx->dev_lock);
}

static void dwt_gpio_callback(const struct device *dev,
			      struct gpio_callback *cb, uint32_t pins)
{
	struct dwt_context *ctx = CONTAINER_OF(cb, struct dwt_context, gpio_cb);

	LOG_DBG("IRQ callback triggered %p", ctx);
	k_work_submit_to_queue(&dwt_work_queue, &ctx->irq_cb_work);
}

static enum ieee802154_hw_caps dwt_get_capabilities(const struct device *dev)
{
	/* TODO: Implement HW-supported AUTOACK + frame pending bit handling. */
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER |
	       IEEE802154_HW_TXTIME;
}

static uint32_t dwt_get_pkt_duration_ns(struct dwt_context *ctx, uint8_t psdu_len)
{
	struct dwt_phy_config *rf_cfg = &ctx->rf_cfg;
	float t_psdu = rf_cfg->t_dsym * psdu_len * 8;

	return (rf_cfg->t_shr + rf_cfg->t_phr + t_psdu);
}

static int dwt_cca(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	uint32_t cca_dur = (dwt_get_pkt_duration_ns(ctx, 127) +
			 dwt_get_pkt_duration_ns(ctx, 5)) /
			 UWB_PHY_TDSYM_PHR_6M8;

	if (atomic_test_and_set_bit(&ctx->state, DWT_STATE_CCA)) {
		LOG_ERR("Transceiver busy");
		return -EBUSY;
	}

	/* Perform CCA Mode 5 */
	k_sem_take(&ctx->dev_lock, K_FOREVER);
	dwt_disable_txrx(dev);
	LOG_DBG("CCA duration %u us", cca_dur);

	dwt_enable_rx(dev, cca_dur);
	k_sem_give(&ctx->dev_lock);

	k_sem_take(&ctx->phy_sem, K_FOREVER);
	LOG_DBG("CCA finished %p", ctx);

	atomic_clear_bit(&ctx->state, DWT_STATE_CCA);
	if (atomic_test_bit(&ctx->state, DWT_STATE_RX_DEF_ON)) {
		k_sem_take(&ctx->dev_lock, K_FOREVER);
		dwt_enable_rx(dev, 0);
		k_sem_give(&ctx->dev_lock);
	}

	return ctx->cca_busy ? -EBUSY : 0;
}

static int dwt_ed(const struct device *dev, uint16_t duration,
		  energy_scan_done_cb_t done_cb)
{
	/* TODO: see description Sub-Register 0x23:02 – AGC_CTRL1 */

	return -ENOTSUP;
}

static int dwt_set_channel(const struct device *dev, uint16_t channel)
{
	struct dwt_context *ctx = dev->data;
	struct dwt_phy_config *rf_cfg = &ctx->rf_cfg;

	if (channel > 15) {
		return -EINVAL;
	}

	if (channel == 0 || channel == 6 || channel > 7) {
		return -ENOTSUP;
	}

	rf_cfg->channel = channel;
	LOG_INF("Set channel %u", channel);

	k_sem_take(&ctx->dev_lock, K_FOREVER);

	dwt_disable_txrx(dev);
	dwt_configure_rf_phy(dev);

	if (atomic_test_bit(&ctx->state, DWT_STATE_RX_DEF_ON)) {
		dwt_enable_rx(dev, 0);
	}

	k_sem_give(&ctx->dev_lock);

	return 0;
}

static int dwt_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	struct dwt_context *ctx = dev->data;

	k_sem_take(&ctx->dev_lock, K_FOREVER);
	dwt_reg_write_u16(dev, DWT_PANADR_ID, DWT_PANADR_PAN_ID_OFFSET, pan_id);
	k_sem_give(&ctx->dev_lock);

	LOG_INF("Set PAN ID 0x%04x %p", pan_id, ctx);

	return 0;
}

static int dwt_set_short_addr(const struct device *dev, uint16_t short_addr)
{
	struct dwt_context *ctx = dev->data;

	k_sem_take(&ctx->dev_lock, K_FOREVER);
	dwt_reg_write_u16(dev, DWT_PANADR_ID, DWT_PANADR_SHORT_ADDR_OFFSET,
			  short_addr);
	k_sem_give(&ctx->dev_lock);

	LOG_INF("Set short 0x%x %p", short_addr, ctx);

	return 0;
}

static int dwt_set_ieee_addr(const struct device *dev,
			     const uint8_t *ieee_addr)
{
	struct dwt_context *ctx = dev->data;

	LOG_INF("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	k_sem_take(&ctx->dev_lock, K_FOREVER);
	dwt_register_write(dev, DWT_EUI_64_ID, DWT_EUI_64_OFFSET,
			   DWT_EUI_64_LEN, (uint8_t *)ieee_addr);
	k_sem_give(&ctx->dev_lock);

	return 0;
}

static int dwt_filter(const struct device *dev,
		      bool set,
		      enum ieee802154_filter_type type,
		      const struct ieee802154_filter *filter)
{
	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return dwt_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return dwt_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return dwt_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

static int dwt_set_power(const struct device *dev, int16_t dbm)
{
	struct dwt_context *ctx = dev->data;

	LOG_INF("set_txpower not supported %p", ctx);

	return 0;
}

static int dwt_tx(const struct device *dev, enum ieee802154_tx_mode tx_mode,
		  struct net_pkt *pkt, struct net_buf *frag)
{
	struct dwt_context *ctx = dev->data;
	size_t len = frag->len;
	uint32_t tx_time = 0;
	uint64_t tmp_fs;
	uint32_t tx_fctrl;
	uint8_t sys_ctrl = DWT_SYS_CTRL_TXSTRT;

	if (atomic_test_and_set_bit(&ctx->state, DWT_STATE_TX)) {
		LOG_ERR("Transceiver busy");
		return -EBUSY;
	}

	k_sem_reset(&ctx->phy_sem);
	k_sem_take(&ctx->dev_lock, K_FOREVER);

	switch (tx_mode) {
	case IEEE802154_TX_MODE_DIRECT:
		break;
	case IEEE802154_TX_MODE_TXTIME:
		/*
		 * tx_time is the high 32-bit of the 40-bit system
		 * time value at which to send the message.
		 */
		tmp_fs = net_pkt_timestamp_ns(pkt);
		tmp_fs *= 1000U * 1000U;

		tx_time = (tmp_fs / DWT_TS_TIME_UNITS_FS) >> 8;
		sys_ctrl |= DWT_SYS_CTRL_TXDLYS;
		/* DX_TIME is 40-bit register */
		dwt_reg_write_u32(dev, DWT_DX_TIME_ID, 1, tx_time);

		LOG_DBG("ntx hi32 %x", tx_time);
		LOG_DBG("sys hi32 %x",
			dwt_reg_read_u32(dev, DWT_SYS_TIME_ID, 1));
		break;
	default:
		LOG_ERR("TX mode %d not supported", tx_mode);
		goto error;
	}

	LOG_HEXDUMP_DBG(frag->data, len, "TX buffer:");

	/*
	 * See "3 Message Transmission" in DW1000 User Manual for
	 * more details about transmission configuration.
	 */
	if (dwt_register_write(dev, DWT_TX_BUFFER_ID, 0, len, frag->data)) {
		LOG_ERR("Failed to write TX data");
		goto error;
	}

	tx_fctrl = dwt_reg_read_u32(dev, DWT_TX_FCTRL_ID, 0);
	/* Clear TX buffer index offset, frame length, and length extension */
	tx_fctrl &= ~(DWT_TX_FCTRL_TFLEN_MASK | DWT_TX_FCTRL_TFLE_MASK |
		      DWT_TX_FCTRL_TXBOFFS_MASK);
	/* Set frame length and ranging flag */
	tx_fctrl |= (len + DWT_FCS_LENGTH) & DWT_TX_FCTRL_TFLEN_MASK;
	tx_fctrl |= DWT_TX_FCTRL_TR;
	/* Update Transmit Frame Control register */
	dwt_reg_write_u32(dev, DWT_TX_FCTRL_ID, 0, tx_fctrl);

	dwt_disable_txrx(dev);

	/* Begin transmission */
	dwt_reg_write_u8(dev, DWT_SYS_CTRL_ID, DWT_SYS_CTRL_OFFSET, sys_ctrl);

	if (sys_ctrl & DWT_SYS_CTRL_TXDLYS) {
		uint32_t sys_stat = dwt_reg_read_u32(dev, DWT_SYS_STATUS_ID, 0);

		if (sys_stat & DWT_SYS_STATUS_HPDWARN) {
			LOG_WRN("Half Period Delay Warning");
		}
	}

	k_sem_give(&ctx->dev_lock);
	/* Wait for the TX confirmed event */
	k_sem_take(&ctx->phy_sem, K_FOREVER);

	if (IS_ENABLED(CONFIG_NET_PKT_TIMESTAMP)) {
		uint8_t ts_buf[sizeof(uint64_t)] = {0};

		k_sem_take(&ctx->dev_lock, K_FOREVER);
		dwt_register_read(dev, DWT_TX_TIME_ID,
				  DWT_TX_TIME_TX_STAMP_OFFSET,
				  DWT_TX_TIME_TX_STAMP_LEN,
				  ts_buf);
		LOG_DBG("ts  hi32 %x", (uint32_t)(sys_get_le64(ts_buf) >> 8));
		LOG_DBG("sys hi32 %x",
			dwt_reg_read_u32(dev, DWT_SYS_TIME_ID, 1));
		k_sem_give(&ctx->dev_lock);

		tmp_fs = sys_get_le64(ts_buf) * DWT_TS_TIME_UNITS_FS;
		net_pkt_set_timestamp_ns(pkt, tmp_fs / 1000000U);
	}

	atomic_clear_bit(&ctx->state, DWT_STATE_TX);

	if (atomic_test_bit(&ctx->state, DWT_STATE_RX_DEF_ON)) {
		k_sem_take(&ctx->dev_lock, K_FOREVER);
		dwt_enable_rx(dev, 0);
		k_sem_give(&ctx->dev_lock);
	}

	return 0;

error:
	atomic_clear_bit(&ctx->state, DWT_STATE_TX);
	k_sem_give(&ctx->dev_lock);

	return -EIO;
}

static void dwt_set_frame_filter(const struct device *dev,
				 bool ff_enable, uint8_t ff_type)
{
	uint32_t sys_cfg_ff = ff_enable ? DWT_SYS_CFG_FFE : 0;

	sys_cfg_ff |= ff_type & DWT_SYS_CFG_FF_ALL_EN;

	dwt_reg_write_u8(dev, DWT_SYS_CFG_ID, 0, (uint8_t)sys_cfg_ff);
}

static int dwt_configure(const struct device *dev,
			 enum ieee802154_config_type type,
			 const struct ieee802154_config *config)
{
	struct dwt_context *ctx = dev->data;

	LOG_DBG("API configure %p", ctx);

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		LOG_DBG("IEEE802154_CONFIG_AUTO_ACK_FPB");
		break;

	case IEEE802154_CONFIG_ACK_FPB:
		LOG_DBG("IEEE802154_CONFIG_ACK_FPB");
		break;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		LOG_DBG("IEEE802154_CONFIG_PAN_COORDINATOR");
		break;

	case IEEE802154_CONFIG_PROMISCUOUS:
		LOG_DBG("IEEE802154_CONFIG_PROMISCUOUS");
		break;

	case IEEE802154_CONFIG_EVENT_HANDLER:
		LOG_DBG("IEEE802154_CONFIG_EVENT_HANDLER");
		break;

	default:
		return -EINVAL;
	}

	return -ENOTSUP;
}

/* driver-allocated attribute memory - constant across all driver instances */
static const struct {
	const struct ieee802154_phy_channel_range phy_channel_range[2];
	const struct ieee802154_phy_supported_channels phy_supported_channels;
} drv_attr = {
	.phy_channel_range = {
		{ .from_channel = 1, .to_channel = 5 },
		{ .from_channel = 7, .to_channel = 7 },
	},
	.phy_supported_channels = {
		.ranges = drv_attr.phy_channel_range,
		.num_ranges = 2U,
	},
};

static int dwt_attr_get(const struct device *dev, enum ieee802154_attr attr,
			struct ieee802154_attr_value *value)
{
	if (ieee802154_attr_get_channel_page_and_range(
		    attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_FOUR_HRP_UWB,
		    &drv_attr.phy_supported_channels, value) == 0) {
		return 0;
	}

	switch (attr) {
	case IEEE802154_ATTR_PHY_HRP_UWB_SUPPORTED_PRFS: {
		struct dwt_context *ctx = dev->data;
		struct dwt_phy_config *rf_cfg = &ctx->rf_cfg;

		value->phy_hrp_uwb_supported_nominal_prfs =
			rf_cfg->prf == DWT_PRF_64M ? IEEE802154_PHY_HRP_UWB_NOMINAL_64_M
						   : IEEE802154_PHY_HRP_UWB_NOMINAL_16_M;
		return 0;
	}

	default:
		return -ENOENT;
	}
}

/*
 * Note, the DW_RESET pin should not be driven high externally.
 */
static int dwt_hw_reset(const struct device *dev)
{
	const struct dwt_hi_cfg *hi_cfg = dev->config;

	if (gpio_pin_configure_dt(&hi_cfg->rst_gpio, GPIO_OUTPUT_ACTIVE)) {
		LOG_ERR("Failed to configure GPIO pin %u", hi_cfg->rst_gpio.pin);
		return -EINVAL;
	}

	k_sleep(K_MSEC(1));
	gpio_pin_set_dt(&hi_cfg->rst_gpio, 0);
	k_sleep(K_MSEC(5));

	if (gpio_pin_configure_dt(&hi_cfg->rst_gpio, GPIO_INPUT)) {
		LOG_ERR("Failed to configure GPIO pin %u", hi_cfg->rst_gpio.pin);
		return -EINVAL;
	}

	return 0;
}

/*
 * SPI speed in INIT state or for wake-up sequence,
 * see 2.3.2 Overview of main operational states
 */
static void dwt_set_spi_slow(const struct device *dev, const uint32_t freq)
{
	struct dwt_context *ctx = dev->data;

	ctx->spi_cfg_slow.frequency = freq;
	ctx->spi_cfg = &ctx->spi_cfg_slow;
}

/* SPI speed in IDLE, RX, and TX state */
static void dwt_set_spi_fast(const struct device *dev)
{
	const struct dwt_hi_cfg *hi_cfg = dev->config;
	struct dwt_context *ctx = dev->data;

	ctx->spi_cfg = &hi_cfg->bus.config;
}

static void dwt_set_rx_mode(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	struct dwt_phy_config *rf_cfg = &ctx->rf_cfg;
	uint32_t pmsc_ctrl0;
	uint32_t t_on_us;
	uint8_t rx_sniff[2];

	/* SNIFF Mode ON time in units of PAC */
	rx_sniff[0] = CONFIG_IEEE802154_DW1000_SNIFF_ONT &
		      DWT_RX_SNIFF_SNIFF_ONT_MASK;
	/* SNIFF Mode OFF time in microseconds */
	rx_sniff[1] = CONFIG_IEEE802154_DW1000_SNIFF_OFFT;

	t_on_us = (rx_sniff[0] + 1) * (BIT(3) << rf_cfg->rx_pac_l);
	LOG_INF("RX duty cycle %u%%", t_on_us * 100 / (t_on_us + rx_sniff[1]));

	dwt_register_write(dev, DWT_RX_SNIFF_ID, DWT_RX_SNIFF_OFFSET,
			   sizeof(rx_sniff), rx_sniff);

	pmsc_ctrl0 = dwt_reg_read_u32(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_OFFSET);
	/* Enable PLL2 on/off sequencing for SNIFF mode */
	pmsc_ctrl0 |= DWT_PMSC_CTRL0_PLL2_SEQ_EN;
	dwt_reg_write_u32(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_OFFSET, pmsc_ctrl0);
}

static int dwt_start(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	uint8_t cswakeup_buf[32] = {0};

	k_sem_take(&ctx->dev_lock, K_FOREVER);

	/* Set SPI clock to lowest frequency */
	dwt_set_spi_slow(dev, DWT_SPI_CSWAKEUP_FREQ);

	if (dwt_reg_read_u32(dev, DWT_DEV_ID_ID, 0) != DWT_DEVICE_ID) {
		/* Keep SPI CS line low for 500 microseconds */
		dwt_register_read(dev, 0, 0, sizeof(cswakeup_buf),
				  cswakeup_buf);
		/* Give device time to initialize */
		k_sleep(K_MSEC(5));

		if (dwt_reg_read_u32(dev, DWT_DEV_ID_ID, 0) != DWT_DEVICE_ID) {
			LOG_ERR("Failed to wake-up %p", dev);
			k_sem_give(&ctx->dev_lock);
			return -1;
		}
	} else {
		LOG_WRN("Device not in a sleep mode");
	}

	/* Restore SPI clock settings */
	dwt_set_spi_slow(dev, DWT_SPI_SLOW_FREQ);
	dwt_set_spi_fast(dev);

	dwt_setup_int(dev, true);
	dwt_disable_txrx(dev);
	dwt_reset_rfrx(dev);

	if (CONFIG_IEEE802154_DW1000_SNIFF_ONT != 0) {
		dwt_set_rx_mode(dev);
	}

	/* Re-enable RX after packet reception */
	atomic_set_bit(&ctx->state, DWT_STATE_RX_DEF_ON);
	dwt_enable_rx(dev, 0);
	k_sem_give(&ctx->dev_lock);

	LOG_INF("Started %p", dev);

	return 0;
}

static int dwt_stop(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;

	k_sem_take(&ctx->dev_lock, K_FOREVER);
	dwt_disable_txrx(dev);
	dwt_reset_rfrx(dev);
	dwt_setup_int(dev, false);

	/* Copy the user configuration and enter sleep mode */
	dwt_reg_write_u8(dev, DWT_AON_ID, DWT_AON_CTRL_OFFSET,
			 DWT_AON_CTRL_SAVE);
	k_sem_give(&ctx->dev_lock);

	LOG_INF("Stopped %p", dev);

	return 0;
}

static inline void dwt_set_sysclks_xti(const struct device *dev, bool ldeload)
{
	uint16_t clks = BIT(9) | DWT_PMSC_CTRL0_SYSCLKS_19M;

	/*
	 * See Table 4: Register accesses required to load LDE microcode,
	 * set PMSC_CTRL0 0x0301, load LDE, set PMSC_CTRL0 0x0200.
	 */
	if (ldeload) {
		clks |= BIT(8);
	}

	/* Force system clock to be the 19.2 MHz XTI clock */
	dwt_reg_write_u16(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_OFFSET, clks);
}

static inline void dwt_set_sysclks_auto(const struct device *dev)
{
	uint8_t sclks = DWT_PMSC_CTRL0_SYSCLKS_AUTO |
		     DWT_PMSC_CTRL0_RXCLKS_AUTO |
		     DWT_PMSC_CTRL0_TXCLKS_AUTO;

	dwt_reg_write_u8(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_OFFSET, sclks);
}

static uint32_t dwt_otpmem_read(const struct device *dev, uint16_t otp_addr)
{
	dwt_reg_write_u16(dev, DWT_OTP_IF_ID, DWT_OTP_ADDR, otp_addr);

	dwt_reg_write_u8(dev, DWT_OTP_IF_ID, DWT_OTP_CTRL,
				DWT_OTP_CTRL_OTPREAD | DWT_OTP_CTRL_OTPRDEN);
	/* OTPREAD is self clearing but OTPRDEN is not */
	dwt_reg_write_u8(dev, DWT_OTP_IF_ID, DWT_OTP_CTRL, 0x00);

	/* Read read data, available 40ns after rising edge of OTP_READ */
	return dwt_reg_read_u32(dev, DWT_OTP_IF_ID, DWT_OTP_RDAT);
}

static int dwt_initialise_dev(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	uint32_t otp_val = 0;
	uint8_t xtal_trim;

	dwt_set_sysclks_xti(dev, false);
	ctx->sleep_mode = 0;

	/* Disable PMSC control of analog RF subsystem */
	dwt_reg_write_u16(dev, DWT_PMSC_ID, DWT_PMSC_CTRL1_OFFSET,
			  DWT_PMSC_CTRL1_PKTSEQ_DISABLE);

	/* Clear all status flags */
	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, 0, DWT_SYS_STATUS_MASK_32);

	/*
	 * Apply soft reset,
	 * see SOFTRESET field description in DW1000 User Manual.
	 */
	dwt_reg_write_u8(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_SOFTRESET_OFFSET,
			 DWT_PMSC_CTRL0_RESET_ALL);
	k_sleep(K_MSEC(1));
	dwt_reg_write_u8(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_SOFTRESET_OFFSET,
			 DWT_PMSC_CTRL0_RESET_CLEAR);

	dwt_set_sysclks_xti(dev, false);

	/*
	 * This bit (a.k.a PLLLDT) should be set to ensure reliable
	 * operation of the CPLOCK bit.
	 */
	dwt_reg_write_u8(dev, DWT_EXT_SYNC_ID, DWT_EC_CTRL_OFFSET,
			 DWT_EC_CTRL_PLLLCK);

	/* Kick LDO if there is a value programmed. */
	otp_val = dwt_otpmem_read(dev, DWT_OTP_LDOTUNE_ADDR);
	if ((otp_val & 0xFF) != 0) {
		dwt_reg_write_u8(dev, DWT_OTP_IF_ID, DWT_OTP_SF,
				 DWT_OTP_SF_LDO_KICK);
		ctx->sleep_mode |= DWT_AON_WCFG_ONW_LLDO;
		LOG_INF("Load LDOTUNE_CAL parameter");
	}

	otp_val = dwt_otpmem_read(dev, DWT_OTP_XTRIM_ADDR);
	xtal_trim = otp_val & DWT_FS_XTALT_MASK;
	LOG_INF("OTP Revision 0x%02x, XTAL Trim 0x%02x",
		(uint8_t)(otp_val >> 8), xtal_trim);

	LOG_DBG("CHIP ID 0x%08x", dwt_otpmem_read(dev, DWT_OTP_PARTID_ADDR));
	LOG_DBG("LOT ID 0x%08x", dwt_otpmem_read(dev, DWT_OTP_LOTID_ADDR));
	LOG_DBG("Vbat 0x%02x", dwt_otpmem_read(dev, DWT_OTP_VBAT_ADDR));
	LOG_DBG("Vtemp 0x%02x", dwt_otpmem_read(dev, DWT_OTP_VTEMP_ADDR));

	if (xtal_trim == 0) {
		/* Set to default */
		xtal_trim = DWT_FS_XTALT_MIDRANGE;
	}

	/* For FS_XTALT bits 7:5 must always be set to binary “011” */
	xtal_trim |= BIT(6) | BIT(5);
	dwt_reg_write_u8(dev, DWT_FS_CTRL_ID, DWT_FS_XTALT_OFFSET, xtal_trim);

	/* Load LDE microcode into RAM, see 2.5.5.10 LDELOAD */
	dwt_set_sysclks_xti(dev, true);
	dwt_reg_write_u16(dev, DWT_OTP_IF_ID, DWT_OTP_CTRL,
			  DWT_OTP_CTRL_LDELOAD);
	k_sleep(K_MSEC(1));
	dwt_set_sysclks_xti(dev, false);
	ctx->sleep_mode |= DWT_AON_WCFG_ONW_LLDE;

	dwt_set_sysclks_auto(dev);

	if (!(dwt_reg_read_u8(dev, DWT_SYS_STATUS_ID, 0) &
	     DWT_SYS_STATUS_CPLOCK)) {
		LOG_WRN("PLL has not locked");
		return -EIO;
	}

	dwt_set_spi_fast(dev);

	/* Setup antenna delay values */
	dwt_reg_write_u16(dev, DWT_LDE_IF_ID, DWT_LDE_RXANTD_OFFSET,
			  DW1000_RX_ANT_DLY);
	dwt_reg_write_u16(dev, DWT_TX_ANTD_ID, DWT_TX_ANTD_OFFSET,
			  DW1000_TX_ANT_DLY);

	/* Clear AON_CFG1 register */
	dwt_reg_write_u8(dev, DWT_AON_ID, DWT_AON_CFG1_OFFSET, 0);
	/*
	 * Configure sleep mode:
	 *  - On wake-up load configurations from the AON memory
	 *  - preserve sleep mode configuration
	 *  - On Wake-up load the LDE microcode
	 *  - When available, on wake-up load the LDO tune value
	 */
	ctx->sleep_mode |= DWT_AON_WCFG_ONW_LDC |
			   DWT_AON_WCFG_PRES_SLEEP;
	dwt_reg_write_u16(dev, DWT_AON_ID, DWT_AON_WCFG_OFFSET,
			  ctx->sleep_mode);
	LOG_DBG("sleep mode 0x%04x", ctx->sleep_mode);
	/* Enable sleep and wake using SPI CSn */
	dwt_reg_write_u8(dev, DWT_AON_ID, DWT_AON_CFG0_OFFSET,
			 DWT_AON_CFG0_WAKE_SPI | DWT_AON_CFG0_SLEEP_EN);

	return 0;
}

/*
 * RF PHY configuration. Must be carried out as part of initialization and
 * for every channel change. See also 2.5 Default Configuration on Power Up.
 */
static int dwt_configure_rf_phy(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	struct dwt_phy_config *rf_cfg = &ctx->rf_cfg;
	uint8_t chan = rf_cfg->channel;
	uint8_t prf_idx = rf_cfg->prf;
	uint32_t chan_ctrl = 0;
	uint8_t rxctrlh;
	uint8_t pll_tune;
	uint8_t tune4h;
	uint8_t pgdelay;
	uint16_t lde_repc;
	uint16_t agc_tune1;
	uint16_t sfdto;
	uint16_t tune1a;
	uint16_t tune0b;
	uint16_t tune1b;
	uint32_t txctrl;
	uint32_t pll_cfg;
	uint32_t tune2;
	uint32_t sys_cfg;
	uint32_t tx_fctrl;
	uint32_t power;

	if ((chan < 1) || (chan > 7) || (chan == 6)) {
		LOG_ERR("Channel not supported %u", chan);
		return -ENOTSUP;
	}

	if (rf_cfg->rx_shr_code >= ARRAY_SIZE(dwt_lde_repc_defs)) {
		LOG_ERR("Preamble code not supported %u",
			rf_cfg->rx_shr_code);
		return -ENOTSUP;
	}

	if (prf_idx >= DWT_NUMOF_PRFS) {
		LOG_ERR("PRF not supported %u", prf_idx);
		return -ENOTSUP;
	}

	if (rf_cfg->rx_pac_l >= DWT_NUMOF_PACS) {
		LOG_ERR("RX PAC not supported %u", rf_cfg->rx_pac_l);
		return -ENOTSUP;
	}

	if (rf_cfg->rx_ns_sfd > 1) {
		LOG_ERR("Wrong NS SFD configuration");
		return -ENOTSUP;
	}

	if (rf_cfg->tx_shr_nsync >= DWT_NUM_OF_PLEN) {
		LOG_ERR("Wrong SHR configuration");
		return -ENOTSUP;
	}

	lde_repc = dwt_lde_repc_defs[rf_cfg->rx_shr_code];
	agc_tune1 = dwt_agc_tune1_defs[prf_idx];
	sfdto = rf_cfg->rx_sfd_to;
	rxctrlh = dwt_rxctrlh_defs[dwt_ch_to_cfg[chan]];
	txctrl = dwt_txctrl_defs[dwt_ch_to_cfg[chan]];
	pll_tune = dwt_plltune_defs[dwt_ch_to_cfg[chan]];
	pll_cfg = dwt_pllcfg_defs[dwt_ch_to_cfg[chan]];
	tune2 = dwt_tune2_defs[prf_idx][rf_cfg->rx_pac_l];
	tune1a = dwt_tune1a_defs[prf_idx];
	tune0b = dwt_tune0b_defs[rf_cfg->dr][rf_cfg->rx_ns_sfd];
	pgdelay = dwt_pgdelay_defs[dwt_ch_to_cfg[chan]];

	sys_cfg = dwt_reg_read_u32(dev, DWT_SYS_CFG_ID, 0);
	tx_fctrl = dwt_reg_read_u32(dev, DWT_TX_FCTRL_ID, 0);

	/* Don't allow 0 - SFD timeout will always be enabled */
	if (sfdto == 0) {
		sfdto = DWT_SFDTOC_DEF;
	}

	/* Set IEEE 802.15.4 compliant mode */
	sys_cfg &= ~DWT_SYS_CFG_PHR_MODE_11;

	if (rf_cfg->dr == DWT_BR_110K) {
		/* Set Receiver Mode 110 kbps data rate */
		sys_cfg |= DWT_SYS_CFG_RXM110K;
		lde_repc = lde_repc >> 3;
		tune1b = DWT_DRX_TUNE1b_110K;
		tune4h = DWT_DRX_TUNE4H_PRE64;
	} else {
		sys_cfg &= ~DWT_SYS_CFG_RXM110K;
		if (rf_cfg->tx_shr_nsync == DWT_PLEN_64) {
			tune1b = DWT_DRX_TUNE1b_6M8_PRE64;
			tune4h = DWT_DRX_TUNE4H_PRE64;
		} else {
			tune1b = DWT_DRX_TUNE1b_850K_6M8;
			tune4h = DWT_DRX_TUNE4H_PRE128PLUS;
		}
	}

	if (sys_cfg & DWT_SYS_CFG_DIS_STXP) {
		if (rf_cfg->prf == DWT_PRF_64M) {
			power = dwt_txpwr_stxp1_64[dwt_ch_to_cfg[chan]];
		} else {
			power = dwt_txpwr_stxp1_16[dwt_ch_to_cfg[chan]];
		}
	} else {
		if (rf_cfg->prf == DWT_PRF_64M) {
			power = dwt_txpwr_stxp0_64[dwt_ch_to_cfg[chan]];
		} else {
			power = dwt_txpwr_stxp0_16[dwt_ch_to_cfg[chan]];
		}
	}

	dwt_reg_write_u32(dev, DWT_SYS_CFG_ID, 0, sys_cfg);
	LOG_DBG("SYS_CFG: 0x%08x", sys_cfg);

	dwt_reg_write_u16(dev, DWT_LDE_IF_ID, DWT_LDE_REPC_OFFSET, lde_repc);
	LOG_DBG("LDE_REPC: 0x%04x", lde_repc);

	dwt_reg_write_u8(dev, DWT_LDE_IF_ID, DWT_LDE_CFG1_OFFSET,
			 DWT_DEFAULT_LDE_CFG1);

	if (rf_cfg->prf == DWT_PRF_64M) {
		dwt_reg_write_u16(dev, DWT_LDE_IF_ID, DWT_LDE_CFG2_OFFSET,
				  DWT_DEFAULT_LDE_CFG2_PRF64);
		LOG_DBG("LDE_CFG2: 0x%04x", DWT_DEFAULT_LDE_CFG2_PRF64);
	} else {
		dwt_reg_write_u16(dev, DWT_LDE_IF_ID, DWT_LDE_CFG2_OFFSET,
				  DWT_DEFAULT_LDE_CFG2_PRF16);
		LOG_DBG("LDE_CFG2: 0x%04x", DWT_DEFAULT_LDE_CFG2_PRF16);
	}

	/* Configure PLL2/RF PLL block CFG/TUNE (for a given channel) */
	dwt_reg_write_u32(dev, DWT_FS_CTRL_ID, DWT_FS_PLLCFG_OFFSET, pll_cfg);
	LOG_DBG("PLLCFG: 0x%08x", pll_cfg);
	dwt_reg_write_u8(dev, DWT_FS_CTRL_ID, DWT_FS_PLLTUNE_OFFSET, pll_tune);
	LOG_DBG("PLLTUNE: 0x%02x", pll_tune);
	/* Configure RF RX blocks (for specified channel/bandwidth) */
	dwt_reg_write_u8(dev, DWT_RF_CONF_ID, DWT_RF_RXCTRLH_OFFSET, rxctrlh);
	LOG_DBG("RXCTRLH: 0x%02x", rxctrlh);
	/* Configure RF/TX blocks for specified channel and PRF */
	dwt_reg_write_u32(dev, DWT_RF_CONF_ID, DWT_RF_TXCTRL_OFFSET, txctrl);
	LOG_DBG("TXCTRL: 0x%08x", txctrl);

	/* Digital receiver configuration, DRX_CONF */
	dwt_reg_write_u16(dev, DWT_DRX_CONF_ID, DWT_DRX_TUNE0b_OFFSET, tune0b);
	LOG_DBG("DRX_TUNE0b: 0x%04x", tune0b);
	dwt_reg_write_u16(dev, DWT_DRX_CONF_ID, DWT_DRX_TUNE1a_OFFSET, tune1a);
	LOG_DBG("DRX_TUNE1a: 0x%04x", tune1a);
	dwt_reg_write_u16(dev, DWT_DRX_CONF_ID, DWT_DRX_TUNE1b_OFFSET, tune1b);
	LOG_DBG("DRX_TUNE1b: 0x%04x", tune1b);
	dwt_reg_write_u32(dev, DWT_DRX_CONF_ID, DWT_DRX_TUNE2_OFFSET, tune2);
	LOG_DBG("DRX_TUNE2: 0x%08x", tune2);
	dwt_reg_write_u8(dev, DWT_DRX_CONF_ID, DWT_DRX_TUNE4H_OFFSET, tune4h);
	LOG_DBG("DRX_TUNE4H: 0x%02x", tune4h);
	dwt_reg_write_u16(dev, DWT_DRX_CONF_ID, DWT_DRX_SFDTOC_OFFSET, sfdto);
	LOG_DBG("DRX_SFDTOC: 0x%04x", sfdto);

	/* Automatic Gain Control configuration and control, AGC_CTRL */
	dwt_reg_write_u16(dev, DWT_AGC_CTRL_ID, DWT_AGC_TUNE1_OFFSET,
			  agc_tune1);
	LOG_DBG("AGC_TUNE1: 0x%04x", agc_tune1);
	dwt_reg_write_u32(dev, DWT_AGC_CTRL_ID, DWT_AGC_TUNE2_OFFSET,
			  DWT_AGC_TUNE2_VAL);

	if (rf_cfg->rx_ns_sfd) {
		/*
		 * SFD_LENGTH, length of the SFD sequence used when
		 * the data rate is 850 kbps or 6.8 Mbps,
		 * must be set to either 8 or 16.
		 */
		dwt_reg_write_u8(dev, DWT_USR_SFD_ID, 0x00,
				 dwt_ns_sfdlen[rf_cfg->dr]);
		LOG_DBG("USR_SFDLEN: 0x%02x", dwt_ns_sfdlen[rf_cfg->dr]);
		chan_ctrl |= DWT_CHAN_CTRL_DWSFD;
	}

	/* Set RX_CHAN and TX CHAN */
	chan_ctrl |= (chan & DWT_CHAN_CTRL_TX_CHAN_MASK) |
		     ((chan << DWT_CHAN_CTRL_RX_CHAN_SHIFT) &
		      DWT_CHAN_CTRL_RX_CHAN_MASK);

	/* Set RXPRF */
	chan_ctrl |= (BIT(rf_cfg->prf) << DWT_CHAN_CTRL_RXFPRF_SHIFT) &
		     DWT_CHAN_CTRL_RXFPRF_MASK;

	/* Set TX_PCOD */
	chan_ctrl |= (rf_cfg->tx_shr_code << DWT_CHAN_CTRL_TX_PCOD_SHIFT) &
		     DWT_CHAN_CTRL_TX_PCOD_MASK;

	/* Set RX_PCOD */
	chan_ctrl |= (rf_cfg->rx_shr_code << DWT_CHAN_CTRL_RX_PCOD_SHIFT) &
		     DWT_CHAN_CTRL_RX_PCOD_MASK;

	/* Set Channel Control */
	dwt_reg_write_u32(dev, DWT_CHAN_CTRL_ID, 0, chan_ctrl);
	LOG_DBG("CHAN_CTRL 0x%08x", chan_ctrl);

	/* Set up TX Preamble Size, PRF and Data Rate */
	tx_fctrl = dwt_plen_cfg[rf_cfg->tx_shr_nsync] |
		   (BIT(rf_cfg->prf) << DWT_TX_FCTRL_TXPRF_SHFT) |
		   (rf_cfg->dr << DWT_TX_FCTRL_TXBR_SHFT);

	dwt_reg_write_u32(dev, DWT_TX_FCTRL_ID, 0, tx_fctrl);
	LOG_DBG("TX_FCTRL 0x%08x", tx_fctrl);

	/* Set the Pulse Generator Delay */
	dwt_reg_write_u8(dev, DWT_TX_CAL_ID, DWT_TC_PGDELAY_OFFSET, pgdelay);
	LOG_DBG("PGDELAY 0x%02x", pgdelay);
	/* Set Transmit Power Control */
	dwt_reg_write_u32(dev, DWT_TX_POWER_ID, 0, power);
	LOG_DBG("TX_POWER 0x%08x", power);

	/*
	 * From 5.3.1.2 SFD Initialisation,
	 * SFD sequence initialisation for Auto ACK frame.
	 */
	dwt_reg_write_u8(dev, DWT_SYS_CTRL_ID, DWT_SYS_CTRL_OFFSET,
			 DWT_SYS_CTRL_TXSTRT | DWT_SYS_CTRL_TRXOFF);

	/*
	 * Calculate PHY timing parameters
	 *
	 * From (9.4) Std 802.15.4-2011
	 * Tshr = Tpsym * (NSYNC + NSFD )
	 * Tphr = NPHR * Tdsym1m
	 * Tpsdu = Tdsym * NPSDU * NSYMPEROCTET / Rfec
	 *
	 * PRF: pulse repetition frequency
	 * PSR: preamble symbol repetitions
	 * SFD: start of frame delimiter
	 * SHR: synchronisation header (SYNC + SFD)
	 * PHR: PHY header
	 */
	uint16_t nsync = BIT(rf_cfg->tx_shr_nsync + 6);

	if (rf_cfg->prf == DWT_PRF_64M) {
		rf_cfg->t_shr = UWB_PHY_TPSYM_PRF64 *
				(nsync + UWB_PHY_NUMOF_SYM_SHR_SFD);
	} else {
		rf_cfg->t_shr = UWB_PHY_TPSYM_PRF16 *
				(nsync + UWB_PHY_NUMOF_SYM_SHR_SFD);
	}

	if (rf_cfg->dr == DWT_BR_6M8) {
		rf_cfg->t_phr = UWB_PHY_NUMOF_SYM_PHR * UWB_PHY_TDSYM_PHR_6M8;
		rf_cfg->t_dsym = UWB_PHY_TDSYM_DATA_6M8 / 0.44;
	} else if (rf_cfg->dr == DWT_BR_850K) {
		rf_cfg->t_phr = UWB_PHY_NUMOF_SYM_PHR * UWB_PHY_TDSYM_PHR_850K;
		rf_cfg->t_dsym = UWB_PHY_TDSYM_DATA_850K / 0.44;
	} else {
		rf_cfg->t_phr = UWB_PHY_NUMOF_SYM_PHR * UWB_PHY_TDSYM_PHR_110K;
		rf_cfg->t_dsym = UWB_PHY_TDSYM_DATA_110K / 0.44;
	}

	return 0;
}

static int dw1000_init(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	const struct dwt_hi_cfg *hi_cfg = dev->config;

	LOG_INF("Initialize DW1000 Transceiver");
	k_sem_init(&ctx->phy_sem, 0, 1);

	/* slow SPI config */
	memcpy(&ctx->spi_cfg_slow, &hi_cfg->bus.config, sizeof(ctx->spi_cfg_slow));
	ctx->spi_cfg_slow.frequency = DWT_SPI_SLOW_FREQ;

	if (!spi_is_ready_dt(&hi_cfg->bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	dwt_set_spi_slow(dev, DWT_SPI_SLOW_FREQ);

	/* Initialize IRQ GPIO */
	if (!gpio_is_ready_dt(&hi_cfg->irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&hi_cfg->irq_gpio, GPIO_INPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", hi_cfg->irq_gpio.pin);
		return -EINVAL;
	}

	gpio_init_callback(&(ctx->gpio_cb), dwt_gpio_callback,
			   BIT(hi_cfg->irq_gpio.pin));

	if (gpio_add_callback(hi_cfg->irq_gpio.port, &(ctx->gpio_cb))) {
		LOG_ERR("Failed to add IRQ callback");
		return -EINVAL;
	}

	/* Initialize RESET GPIO */
	if (!gpio_is_ready_dt(&hi_cfg->rst_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&hi_cfg->rst_gpio, GPIO_INPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", hi_cfg->rst_gpio.pin);
		return -EINVAL;
	}

	LOG_INF("GPIO and SPI configured");

	dwt_hw_reset(dev);

	if (dwt_reg_read_u32(dev, DWT_DEV_ID_ID, 0) != DWT_DEVICE_ID) {
		LOG_ERR("Failed to read device ID %p", dev);
		return -ENODEV;
	}

	if (dwt_initialise_dev(dev)) {
		LOG_ERR("Failed to initialize DW1000");
		return -EIO;
	}

	if (dwt_configure_rf_phy(dev)) {
		LOG_ERR("Failed to configure RF PHY");
		return -EIO;
	}

	/* Allow Beacon, Data, Acknowledgement, MAC command */
	dwt_set_frame_filter(dev, true, DWT_SYS_CFG_FFAB | DWT_SYS_CFG_FFAD |
			     DWT_SYS_CFG_FFAA | DWT_SYS_CFG_FFAM);

	/*
	 * Enable system events:
	 *  - transmit frame sent,
	 *  - receiver FCS good,
	 *  - receiver PHY header error,
	 *  - receiver FCS error,
	 *  - receiver Reed Solomon Frame Sync Loss,
	 *  - receive Frame Wait Timeout,
	 *  - preamble detection timeout,
	 *  - receive SFD timeout
	 */
	dwt_reg_write_u32(dev, DWT_SYS_MASK_ID, 0,
			  DWT_SYS_MASK_MTXFRS |
			  DWT_SYS_MASK_MRXFCG |
			  DWT_SYS_MASK_MRXPHE |
			  DWT_SYS_MASK_MRXFCE |
			  DWT_SYS_MASK_MRXRFSL |
			  DWT_SYS_MASK_MRXRFTO |
			  DWT_SYS_MASK_MRXPTO |
			  DWT_SYS_MASK_MRXSFDTO);

	/* Initialize IRQ event work queue */
	ctx->dev = dev;

	k_work_queue_start(&dwt_work_queue, dwt_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(dwt_work_queue_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);

	k_work_init(&ctx->irq_cb_work, dwt_irq_work_handler);

	dwt_setup_int(dev, true);

	LOG_INF("DW1000 device initialized and configured");

	return 0;
}

static inline uint8_t *get_mac(const struct device *dev)
{
	struct dwt_context *dw1000 = dev->data;
	uint32_t *ptr = (uint32_t *)(dw1000->mac_addr);

	UNALIGNED_PUT(sys_rand32_get(), ptr);
	ptr = (uint32_t *)(dw1000->mac_addr + 4);
	UNALIGNED_PUT(sys_rand32_get(), ptr);

	dw1000->mac_addr[0] = (dw1000->mac_addr[0] & ~0x01) | 0x02;

	return dw1000->mac_addr;
}

static void dwt_iface_api_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dwt_context *dw1000 = dev->data;
	uint8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	dw1000->iface = iface;

	ieee802154_init(iface);

	LOG_INF("Iface initialized");
}

static struct ieee802154_radio_api dwt_radio_api = {
	.iface_api.init		= dwt_iface_api_init,

	.get_capabilities	= dwt_get_capabilities,
	.cca			= dwt_cca,
	.set_channel		= dwt_set_channel,
	.filter			= dwt_filter,
	.set_txpower		= dwt_set_power,
	.start			= dwt_start,
	.stop			= dwt_stop,
	.configure		= dwt_configure,
	.ed_scan		= dwt_ed,
	.tx			= dwt_tx,
	.attr_get		= dwt_attr_get,
};

#define DWT_PSDU_LENGTH		(127 - DWT_FCS_LENGTH)

#if defined(CONFIG_IEEE802154_RAW_MODE)
DEVICE_DT_INST_DEFINE(0, dw1000_init, NULL,
		    &dwt_0_context, &dw1000_0_config,
		    POST_KERNEL, CONFIG_IEEE802154_DW1000_INIT_PRIO,
		    &dwt_radio_api);
#else
NET_DEVICE_DT_INST_DEFINE(0,
		dw1000_init,
		NULL,
		&dwt_0_context,
		&dw1000_0_config,
		CONFIG_IEEE802154_DW1000_INIT_PRIO,
		&dwt_radio_api,
		IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2),
		DWT_PSDU_LENGTH);
#endif
