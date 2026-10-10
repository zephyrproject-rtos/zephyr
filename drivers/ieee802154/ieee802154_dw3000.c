/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the Qorvo DW3000 family of IEEE 802.15.4 HRP UWB transceivers.
 *
 * Section, table and page numbers refer to the "DW3000 Family User Manual",
 * version 1.1. "Datasheet" is the "DW3000 Datasheet", version 1.3.
 */

#define DT_DRV_COMPAT qorvo_dw3000

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ieee802154_dw3000_regs.h"

LOG_MODULE_REGISTER(ieee802154_dw3000, CONFIG_IEEE802154_DRIVER_LOG_LEVEL);

/* SPI mode 0, most significant bit first (Datasheet 4.9.1, p24 and Table 20, p25) */
#define DW3000_SPI_OPERATION     (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
#define DW3000_SPI_HEADER_LEN    2U
/* Datasheet 4.9, p24 */
#define DW3000_SPI_MAX_FREQUENCY 36000000U

/* Time RSTn is held low. The datasheet states no minimum. */
#define DW3000_RESET_PULSE_MS 1
/* Number of times a register is polled, 1 ms apart, before giving up */
#define DW3000_POLL_COUNT     10U
/*
 * SPICSn held low for 500 us wakes the transceiver up (Datasheet 6, Figure 22,
 * p34). It is held for 1 ms, at a clock rate low enough for a small buffer.
 */
#define DW3000_WAKE_FREQUENCY 500000U
#define DW3000_WAKE_LEN       DIV_ROUND_UP(DW3000_WAKE_FREQUENCY, 8U * MSEC_PER_SEC)
/* Twice the start-up time after a wake-up, about 1 ms (Datasheet 3.5, Table 10, p14) */
#define DW3000_WAKE_UP_MS     2
/*
 * Time given to the transceiver to copy values from its OTP memory. The manual
 * only says that this memory starts up in about 85 us (2.5.1.2, p24).
 */
#define DW3000_OTP_LOAD_MS    1
/*
 * A frame of maximum length is on air for less than 1 ms at 6.81 Mb/s. The
 * rest of the time allows for the interrupt to be served.
 */
#define DW3000_TX_TIMEOUT     K_MSEC(50)
/* Number of times the events are served for one interrupt before giving up */
#define DW3000_EVENT_PASSES   8U

#define DW3000_RX_THREAD_PRIO K_PRIO_COOP(2)

/* Preamble code, valid on both channels at 64 MHz PRF (2.6, Table 8, p28) */
#define DW3000_PREAMBLE_CODE 9U

/*
 * Stage of the channel control register common to both channels: the IEEE
 * 802.15.4 short SFD and one preamble code for both directions.
 */
#define DW3000_CHAN_CTRL_COMMON                                                                    \
	(FIELD_PREP(DW3000_CHAN_CTRL_SFD_TYPE, DW3000_CHAN_CTRL_SFD_TYPE_IEEE) |                   \
	 FIELD_PREP(DW3000_CHAN_CTRL_TX_PCODE, DW3000_PREAMBLE_CODE) |                             \
	 FIELD_PREP(DW3000_CHAN_CTRL_RX_PCODE, DW3000_PREAMBLE_CODE))

/* Events that end a reception, with or without a frame */
#define DW3000_RX_END_EVENTS                                                                       \
	(DW3000_SYS_STATUS_RXFR | DW3000_SYS_STATUS_RXPHE | DW3000_SYS_STATUS_RXFSL |              \
	 DW3000_SYS_STATUS_RXSTO)
/* Events that raise the interrupt line */
#define DW3000_IRQ_EVENTS (DW3000_SYS_STATUS_TXFRS | DW3000_RX_END_EVENTS)
/* Events cleared once handled, so that none is carried over to the next frame */
#define DW3000_HANDLED_EVENTS                                                                      \
	(DW3000_IRQ_EVENTS | DW3000_SYS_STATUS_CIADONE | DW3000_SYS_STATUS_CIAERR |                \
	 DW3000_SYS_STATUS_RXFCG | DW3000_SYS_STATUS_RXFCE)

/*
 * Device time counts in units of 1/(128 * 499.2 MHz), about 15.65 ps
 * (8.2.2.16, p103). That is 39936 units in 625 ns.
 */
#define DW3000_TIME_UNITS 39936U
#define DW3000_TIME_NS    625U

/* One step of a register sequence */
struct dw3000_reg_op {
	uint16_t reg;
	uint8_t len;
	uint32_t mask; /* bits to update, or 0 to write the whole register */
	uint32_t val;
};

struct dw3000_channel {
	uint16_t number;
	const struct dw3000_reg_op *ops;
	size_t num_ops;
};

struct dw3000_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec irq_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec wakeup_gpio; /* optional */
};

struct dw3000_data {
	struct net_if *iface;
	struct gpio_callback irq_cb;
	struct k_sem irq_sem;
	struct k_mutex api_lock;  /* one radio API call at a time */
	struct k_mutex chip_lock; /* register sequences against the interrupt thread */
	struct k_sem tx_done;
	struct spi_config wake_cfg;           /* the bus at the clock rate of the wake-up */
	const struct dw3000_channel *channel; /* NULL while the PLL is not locked to one */
	bool started;                         /* the receiver is on unless a frame is being sent */
	bool tx_busy;
	uint8_t mac_addr[IEEE802154_EXT_ADDR_LENGTH];
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_DW3000_RX_STACK_SIZE);
};

/* Values the manual says to change from their reset defaults */
static const struct dw3000_reg_op dw3000_tuning_ops[] = {
	/* THR_64 "should be changed from the default value of 0x38 to 0x32" (8.2.4.1, p126) */
	{DW3000_DGC_CFG, 2, DW3000_DGC_CFG_THR_64 | DW3000_DGC_CFG_RX_TUNE_EN,
	 FIELD_PREP(DW3000_DGC_CFG_THR_64, DW3000_DGC_CFG_THR_64_TUNED) |
		 DW3000_DGC_CFG_RX_TUNE_EN},
	/* Receiver tuning, the same on both channels (8.2.4, Table 24, p126) */
	{DW3000_DGC_CFG0, 4, 0, DW3000_DGC_CFG0_TUNED},
	{DW3000_DGC_CFG1, 4, 0, DW3000_DGC_CFG1_TUNED},
	/* DT0B4 "should be cleared to zero for best performance" (8.2.7.1, p146) */
	{DW3000_DTUNE0, 2, DW3000_DTUNE0_DT0B4, 0},
	/* "for optimal receiver performance" (8.2.7.4, p147) */
	{DW3000_DTUNE3, 4, 0, DW3000_DTUNE3_TUNED},
	/* "needs to be set to 0x0E for the optimal performance" (8.2.8.4, p151) */
	{DW3000_RF_TX_CTRL_1, 1, 0, DW3000_RF_TX_CTRL_1_TUNED},
	/* "should be set to 0x14 for optimal operation" (8.2.8.10, p155) */
	{DW3000_LDO_RLOAD, 1, 0, DW3000_LDO_RLOAD_TUNED},
};

/*
 * Per channel: transmitter (8.2.8.5, Table 30, p152), PLL (8.2.10.1, Table 35,
 * p163) and the receiver tuning look-up table (8.2.4, Table 24, p126).
 */
static const struct dw3000_reg_op dw3000_channel_5_ops[] = {
	{DW3000_CHAN_CTRL, 2, 0, DW3000_CHAN_CTRL_COMMON},
	{DW3000_RF_TX_CTRL_2, 4, 0, DW3000_RF_TX_CTRL_2_CH5},
	{DW3000_PLL_CFG, 2, 0, DW3000_PLL_CFG_CH5},
	{DW3000_DGC_LUT(0), 4, 0, 0x0001c0fd},
	{DW3000_DGC_LUT(1), 4, 0, 0x0001c43e},
	{DW3000_DGC_LUT(2), 4, 0, 0x0001c6be},
	{DW3000_DGC_LUT(3), 4, 0, 0x0001c77e},
	{DW3000_DGC_LUT(4), 4, 0, 0x0001cf36},
	{DW3000_DGC_LUT(5), 4, 0, 0x0001cfb5},
	{DW3000_DGC_LUT(6), 4, 0, 0x0001cff5},
};

static const struct dw3000_reg_op dw3000_channel_9_ops[] = {
	{DW3000_CHAN_CTRL, 2, 0, DW3000_CHAN_CTRL_COMMON | DW3000_CHAN_CTRL_RF_CHAN},
	{DW3000_RF_TX_CTRL_2, 4, 0, DW3000_RF_TX_CTRL_2_CH9},
	{DW3000_PLL_CFG, 2, 0, DW3000_PLL_CFG_CH9},
	{DW3000_DGC_LUT(0), 4, 0, 0x0002a8fe},
	{DW3000_DGC_LUT(1), 4, 0, 0x0002ac36},
	{DW3000_DGC_LUT(2), 4, 0, 0x0002a5fe},
	{DW3000_DGC_LUT(3), 4, 0, 0x0002af3e},
	{DW3000_DGC_LUT(4), 4, 0, 0x0002af7d},
	{DW3000_DGC_LUT(5), 4, 0, 0x0002afb5},
	{DW3000_DGC_LUT(6), 4, 0, 0x0002afb5},
};

static const struct dw3000_channel dw3000_channels[] = {
	{5, dw3000_channel_5_ops, ARRAY_SIZE(dw3000_channel_5_ops)},
	{9, dw3000_channel_9_ops, ARRAY_SIZE(dw3000_channel_9_ops)},
};

/*
 * Leave IDLE_PLL for IDLE_RC. The system clock is moved to FAST_RC "prior to"
 * forcing the state (8.2.15.3, p223): the PLL stops there, and a transceiver
 * still clocked from it no longer answers. Section 10.4 (p245) lists the first
 * two steps the other way round.
 */
static const struct dw3000_reg_op dw3000_idle_rc_ops[] = {
	{DW3000_CLK_CTRL, 4, DW3000_CLK_CTRL_SYS_CLK, DW3000_CLK_CTRL_SYS_CLK_FAST_RC},
	{DW3000_SEQ_CTRL, 4, DW3000_SEQ_CTRL_AINIT2IDLE | DW3000_SEQ_CTRL_FORCE2INIT,
	 DW3000_SEQ_CTRL_FORCE2INIT},
	{DW3000_SEQ_CTRL, 4, DW3000_SEQ_CTRL_FORCE2INIT, 0},
	{DW3000_CLK_CTRL, 4, DW3000_CLK_CTRL_SYS_CLK, DW3000_CLK_CTRL_SYS_CLK_AUTO},
};

/*
 * Calibrate the PLL and let the transceiver move from IDLE_RC to IDLE_PLL once
 * it has locked (10.4, p245). A lock event left from before is cleared first.
 */
static const struct dw3000_reg_op dw3000_pll_lock_ops[] = {
	{DW3000_SYS_STATUS, 4, 0, DW3000_SYS_STATUS_CPLOCK},
	{DW3000_PLL_CAL, 2, 0, DW3000_PLL_CAL_CAL_EN | DW3000_PLL_CAL_CONFIG},
	{DW3000_SEQ_CTRL, 4, DW3000_SEQ_CTRL_AINIT2IDLE, DW3000_SEQ_CTRL_AINIT2IDLE},
};

/*
 * Go to DEEPSLEEP, to wake up on SPICSn, calibrate the receiver and go on to
 * IDLE_PLL. SAVE stores these wake-up options and enters the sleep state that
 * SLEEP_EN asks for (8.2.11.1, p166; 8.2.11.2, p167 and 8.2.11.6, p172).
 */
static const struct dw3000_reg_op dw3000_sleep_ops[] = {
	{DW3000_AON_DIG_CFG, 3, DW3000_AON_DIG_CFG_ONW_GO2IDLE | DW3000_AON_DIG_CFG_ONW_PGFCAL,
	 DW3000_AON_DIG_CFG_ONW_GO2IDLE | DW3000_AON_DIG_CFG_ONW_PGFCAL},
	{DW3000_AON_CFG, 1, DW3000_AON_CFG_SLEEP_EN | DW3000_AON_CFG_WAKE_CSN,
	 DW3000_AON_CFG_SLEEP_EN | DW3000_AON_CFG_WAKE_CSN},
	{DW3000_AON_CTRL, 1, 0, DW3000_AON_CTRL_SAVE},
};

/* Zeros sent to wake the transceiver up: SPIMOSI has to stay low (2.5.1.1, p24) */
static const uint8_t dw3000_wake[DW3000_WAKE_LEN];

/* Results of the receiver calibration (8.2.5.3 and 8.2.5.4, p129) */
static const uint16_t dw3000_rx_cal_results[] = {DW3000_RX_CAL_RESI, DW3000_RX_CAL_RESQ};

/* Have the LDO and bias trim values copied from the OTP memory (8.2.12.3, p176) */
static const struct dw3000_reg_op dw3000_trim_ops[] = {
	{DW3000_OTP_CFG, 2, DW3000_OTP_CFG_LDO_KICK | DW3000_OTP_CFG_BIAS_KICK,
	 DW3000_OTP_CFG_LDO_KICK | DW3000_OTP_CFG_BIAS_KICK},
};

/* Channel page four, HRP UWB: of its channels the transceiver has 5 and 9 */
static const struct ieee802154_phy_channel_range dw3000_channel_ranges[] = {
	{.from_channel = 5, .to_channel = 5},
	{.from_channel = 9, .to_channel = 9},
};

static const struct ieee802154_phy_supported_channels dw3000_supported_channels = {
	.ranges = dw3000_channel_ranges,
	.num_ranges = ARRAY_SIZE(dw3000_channel_ranges),
};

/*
 * Build the header of a full addressed transaction (2.3.1.2, Figure 2, p13).
 * Bit 6 of the octet offset ends the first octet and bits 5 to 0 start the
 * second.
 */
static void dw3000_spi_header(uint16_t reg, bool write, uint8_t header[DW3000_SPI_HEADER_LEN])
{
	uint8_t file = FIELD_GET(DW3000_REG_FILE, reg);
	uint8_t offset = FIELD_GET(DW3000_REG_OFFSET, reg);

	header[0] = DW3000_SPI_FULL_ADDR | FIELD_PREP(DW3000_SPI_FILE, file) | (offset >> 6);
	header[1] = (uint8_t)(offset << 2);

	if (write) {
		header[0] |= DW3000_SPI_WRITE;
	}
}

static int dw3000_read(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len)
{
	const struct dw3000_config *cfg = dev->config;
	uint8_t header[DW3000_SPI_HEADER_LEN];
	const struct spi_buf tx_buf = {.buf = header, .len = sizeof(header)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(header)},
		{.buf = buf, .len = len},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	dw3000_spi_header(reg, false, header);

	return spi_transceive_dt(&cfg->bus, &tx, &rx);
}

static int dw3000_write(const struct device *dev, uint16_t reg, const uint8_t *buf, size_t len)
{
	const struct dw3000_config *cfg = dev->config;
	uint8_t header[DW3000_SPI_HEADER_LEN];
	const struct spi_buf tx_bufs[] = {
		{.buf = header, .len = sizeof(header)},
		{.buf = (uint8_t *)buf, .len = len},
	};
	const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};

	dw3000_spi_header(reg, true, header);

	return spi_write_dt(&cfg->bus, &tx);
}

static int dw3000_fast_cmd(const struct device *dev, uint8_t cmd)
{
	const struct dw3000_config *cfg = dev->config;
	uint8_t header = DW3000_SPI_WRITE | FIELD_PREP(DW3000_SPI_CMD, cmd) | DW3000_SPI_FAST_CMD;
	const struct spi_buf tx_buf = {.buf = &header, .len = sizeof(header)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&cfg->bus, &tx);
}

/*
 * Read a register of up to four octets. Register values are transferred least
 * significant octet first (8.2.1, p72).
 */
static int dw3000_reg_read(const struct device *dev, uint16_t reg, size_t len, uint32_t *val)
{
	uint8_t buf[sizeof(uint32_t)] = {0};
	int ret;

	__ASSERT_NO_MSG(len <= sizeof(buf));

	ret = dw3000_read(dev, reg, buf, len);
	if (ret != 0) {
		return ret;
	}

	*val = sys_get_le32(buf);

	return 0;
}

static int dw3000_reg_write(const struct device *dev, uint16_t reg, size_t len, uint32_t val)
{
	uint8_t buf[sizeof(uint32_t)];

	__ASSERT_NO_MSG(len <= sizeof(buf));

	sys_put_le32(val, buf);

	return dw3000_write(dev, reg, buf, len);
}

static int dw3000_reg_apply(const struct device *dev, const struct dw3000_reg_op *ops,
			    size_t num_ops)
{
	uint32_t val;
	int ret;

	for (size_t i = 0; i < num_ops; i++) {
		val = 0U;

		if (ops[i].mask != 0U) {
			ret = dw3000_reg_read(dev, ops[i].reg, ops[i].len, &val);
			if (ret != 0) {
				return ret;
			}
		}

		ret = dw3000_reg_write(dev, ops[i].reg, ops[i].len,
				       (val & ~ops[i].mask) | ops[i].val);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/* Poll a register until all bits of mask are set */
static int dw3000_reg_wait(const struct device *dev, uint16_t reg, size_t len, uint32_t mask)
{
	uint32_t val;
	int ret;

	for (uint32_t i = 0; i < DW3000_POLL_COUNT; i++) {
		ret = dw3000_reg_read(dev, reg, len, &val);
		if (ret != 0) {
			return ret;
		}

		if ((val & mask) == mask) {
			return 0;
		}

		k_msleep(1);
	}

	return -ETIMEDOUT;
}

/* Read a 40-bit time stamp, in device time units */
static int dw3000_read_stamp(const struct device *dev, uint16_t reg, uint64_t *stamp)
{
	uint8_t buf[DW3000_TIME_STAMP_LEN];
	int ret;

	ret = dw3000_read(dev, reg, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	*stamp = sys_get_le40(buf);

	return 0;
}

/*
 * A time stamp is the transceiver's own 40-bit time, which wraps about every
 * 17.2 s (8.2.2.7, p84). It is not related to the clock of the network
 * subsystem.
 */
static net_time_t dw3000_stamp_to_ns(uint64_t stamp)
{
	return (net_time_t)(stamp * DW3000_TIME_NS / DW3000_TIME_UNITS);
}

/*
 * Turn the receiver on, unless the interface is down, a frame is being sent or
 * the PLL is not locked to a channel.
 */
static void dw3000_rx_resume(const struct device *dev)
{
	struct dw3000_data *data = dev->data;

	if (!data->started || data->tx_busy || data->channel == NULL) {
		return;
	}

	if (dw3000_fast_cmd(dev, DW3000_CMD_RX) != 0) {
		LOG_ERR("Failed to enable the receiver");
	}
}

/*
 * Read a received frame into a new packet. Its time stamp is that of the
 * RMARKER. It is valid once the CIADONE event is set (8.2.2.16, p103), unless
 * the analysis that adjusts it ended in CIAERR (8.2.2.14, p96).
 */
static struct net_pkt *dw3000_rx_frame(const struct device *dev, uint32_t status)
{
	struct dw3000_data *data = dev->data;
	uint8_t psdu[IEEE802154_MAX_PHY_PACKET_SIZE];
	struct net_pkt *pkt;
	uint64_t stamp;
	uint32_t finfo;
	size_t len;

	if (dw3000_reg_read(dev, DW3000_RX_FINFO, sizeof(finfo), &finfo) != 0) {
		LOG_ERR("Failed to read the length of a received frame");
		return NULL;
	}

	/* A frame holds more than its FCS */
	len = FIELD_GET(DW3000_RX_FINFO_RXFLEN, finfo);
	if (len <= IEEE802154_FCS_LENGTH || len > IEEE802154_MAX_PHY_PACKET_SIZE) {
		LOG_DBG("Dropping frame of invalid length %zu", len);
		return NULL;
	}

	if (!IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
		len -= IEEE802154_FCS_LENGTH;
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_WRN("No buffer for a received frame");
		return NULL;
	}

	if (dw3000_read(dev, DW3000_RX_BUFFER_0, psdu, len) != 0) {
		LOG_ERR("Failed to read a received frame");
		goto out_unref;
	}

	if (net_pkt_write(pkt, psdu, len) != 0) {
		LOG_ERR("Failed to copy a received frame");
		goto out_unref;
	}

	net_pkt_set_ieee802154_rssi_dbm(pkt, IEEE802154_MAC_RSSI_DBM_UNDEFINED);

	if (!IS_ENABLED(CONFIG_NET_PKT_TIMESTAMP) ||
	    (status & (DW3000_SYS_STATUS_CIADONE | DW3000_SYS_STATUS_CIAERR)) !=
		    DW3000_SYS_STATUS_CIADONE) {
		return pkt;
	}

	if (dw3000_read_stamp(dev, DW3000_RX_TIME, &stamp) == 0) {
		net_pkt_set_timestamp_ns(pkt, dw3000_stamp_to_ns(stamp));
	}

	return pkt;

out_unref:
	net_pkt_unref(pkt);

	return NULL;
}

/*
 * Hand a received frame to the network stack. The caller does not hold
 * chip_lock, which leaves the stack free to call the driver from here.
 */
static void dw3000_rx_deliver(const struct device *dev, struct net_pkt *pkt)
{
	struct dw3000_data *data = dev->data;

	if (ieee802154_handle_ack(data->iface, pkt) == NET_OK) {
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(data->iface, pkt) != 0) {
		net_pkt_unref(pkt);
	}
}

/*
 * Handle the events of one reading of SYS_STATUS and return the frame that
 * was received, if any. Events are cleared by writing 1 to them (8.2.2.14,
 * p93). Only those seen in status are cleared, so that one raised since is not
 * lost. The caller holds chip_lock.
 */
static struct net_pkt *dw3000_handle_events(const struct device *dev, uint32_t status)
{
	struct dw3000_data *data = dev->data;
	uint32_t handled = status & DW3000_HANDLED_EVENTS;
	struct net_pkt *pkt = NULL;

	if ((status & DW3000_SYS_STATUS_RXFCG) != 0U) {
		pkt = dw3000_rx_frame(dev, status);
	}

	/* The SFD event tells dw3000_cca() of a reception under way and goes with its end */
	if ((status & DW3000_RX_END_EVENTS) != 0U) {
		handled |= status & DW3000_SYS_STATUS_RXSFDD;
	}

	if (dw3000_reg_write(dev, DW3000_SYS_STATUS, sizeof(status), handled) != 0) {
		LOG_ERR("Failed to clear events");
	}

	/* The transceiver has turned its receiver back on by now (9.13, p241) */
	if ((status & DW3000_SYS_STATUS_TXFRS) != 0U) {
		data->tx_busy = false;
		k_sem_give(&data->tx_done);
	}

	if ((status & DW3000_RX_END_EVENTS) != 0U) {
		dw3000_rx_resume(dev);
	}

	return pkt;
}

/* Serve the pending events once. Returns -EAGAIN if there may be more. */
static int dw3000_serve_events(const struct device *dev)
{
	struct dw3000_data *data = dev->data;
	struct net_pkt *pkt = NULL;
	uint32_t status;
	int ret;

	k_mutex_lock(&data->chip_lock, K_FOREVER);

	ret = dw3000_reg_read(dev, DW3000_SYS_STATUS, sizeof(status), &status);
	if (ret == 0 && (status & DW3000_IRQ_EVENTS) != 0U) {
		pkt = dw3000_handle_events(dev, status);
		ret = -EAGAIN;
	}

	k_mutex_unlock(&data->chip_lock);

	if (pkt != NULL) {
		dw3000_rx_deliver(dev, pkt);
	}

	return ret;
}

/*
 * The interrupt line stays high while an enabled event is pending and the GPIO
 * interrupt is edge triggered, so events are served until none is left. That
 * is given up after a number of rounds, since a transceiver that no longer
 * answers can read as events that never clear, and after a failed transfer. An
 * event left pending keeps the line high, so no edge will come for it: the
 * thread lets the others run and then looks at the line itself.
 */
static void dw3000_rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	const struct dw3000_config *cfg = dev->config;
	struct dw3000_data *data = dev->data;
	uint32_t passes;
	int ret;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->irq_sem, K_FOREVER);

		passes = 0U;

		do {
			ret = dw3000_serve_events(dev);
			passes++;
		} while (ret == -EAGAIN && passes < DW3000_EVENT_PASSES);

		if (ret == -EAGAIN) {
			LOG_DBG("Events left after %u passes", passes);
			k_msleep(1);
		} else if (ret != 0) {
			LOG_WRN("Failed to serve events: %d", ret);
			k_msleep(1);
		}

		if (gpio_pin_get_dt(&cfg->irq_gpio) > 0) {
			k_sem_give(&data->irq_sem);
		}
	}
}

static void dw3000_irq_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct dw3000_data *data = CONTAINER_OF(cb, struct dw3000_data, irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->irq_sem);
}

static enum ieee802154_hw_caps dw3000_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return IEEE802154_HW_FCS;
}

/*
 * CCA mode 4 of IEEE 802.15.4, ALOHA: the medium is reported idle without an
 * assessment. The transceiver can only look for a preamble as part of a
 * transmit command (5.7, p55). In every mode the medium is busy while a frame
 * is being received, from the detection of its SFD on (IEEE 802.15.4-2024,
 * 11.2.8). RXSFDD reports that detection (8.2.2.14, p94) and is cleared with
 * the events that end the reception.
 */
static int dw3000_cca(const struct device *dev)
{
	struct dw3000_data *data = dev->data;
	uint32_t status;
	int ret;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->started) {
		ret = -ENETDOWN;
		goto out_unlock;
	}

	if (dw3000_reg_read(dev, DW3000_SYS_STATUS, sizeof(status), &status) != 0) {
		ret = -EIO;
		goto out_unlock;
	}

	if ((status & DW3000_SYS_STATUS_RXSFDD) != 0U && (status & DW3000_RX_END_EVENTS) == 0U) {
		ret = -EBUSY;
	} else {
		ret = 0;
	}

out_unlock:
	k_mutex_unlock(&data->api_lock);

	return ret;
}

/*
 * Select a channel and lock the PLL to it. The transceiver must be in IDLE_RC
 * and is in IDLE_PLL on return.
 */
static int dw3000_tune(const struct device *dev, const struct dw3000_channel *channel)
{
	struct dw3000_data *data = dev->data;
	int ret;

	ret = dw3000_reg_apply(dev, channel->ops, channel->num_ops);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_apply(dev, dw3000_pll_lock_ops, ARRAY_SIZE(dw3000_pll_lock_ops));
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_wait(dev, DW3000_SYS_STATUS, sizeof(uint32_t), DW3000_SYS_STATUS_CPLOCK);
	if (ret != 0) {
		LOG_ERR("PLL did not lock on channel %u", channel->number);
		return ret;
	}

	data->channel = channel;

	return 0;
}

/*
 * Change the channel. The receiver is turned off first, unless the PLL is not
 * locked to a channel: a failed attempt leaves the transceiver in IDLE_RC,
 * where CMD_TXRXOFF "should not be issued" (9.1, p239). The caller holds
 * chip_lock.
 */
static int dw3000_retune(const struct device *dev, const struct dw3000_channel *channel)
{
	struct dw3000_data *data = dev->data;
	int ret;

	if (data->channel != NULL) {
		ret = dw3000_fast_cmd(dev, DW3000_CMD_TXRXOFF);
		if (ret != 0) {
			return ret;
		}
	}

	data->channel = NULL;

	ret = dw3000_reg_apply(dev, dw3000_idle_rc_ops, ARRAY_SIZE(dw3000_idle_rc_ops));
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_tune(dev, channel);
	if (ret != 0) {
		return ret;
	}

	dw3000_rx_resume(dev);

	return 0;
}

static const struct dw3000_channel *dw3000_find_channel(uint16_t number)
{
	ARRAY_FOR_EACH_PTR(dw3000_channels, channel) {
		if (channel->number == number) {
			return channel;
		}
	}

	return NULL;
}

static int dw3000_set_channel(const struct device *dev, uint16_t channel)
{
	const struct dw3000_channel *target = dw3000_find_channel(channel);
	struct dw3000_data *data = dev->data;
	int ret = 0;

	/* Channel page four has channels 0 to 15 */
	if (channel > 15U) {
		return -EINVAL;
	}

	if (target == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);
	k_mutex_lock(&data->chip_lock, K_FOREVER);

	if (target != data->channel) {
		if (dw3000_retune(dev, target) != 0) {
			ret = -EIO;
		}
	}

	k_mutex_unlock(&data->chip_lock);
	k_mutex_unlock(&data->api_lock);

	return ret;
}

/*
 * The power register holds gain codes (8.2.2.21, p106). What they give in dBm
 * is a calibration of the board, which the driver does not have, so the
 * register keeps its reset value. The request is accepted, as the DW1000
 * driver does: the L2 makes one at every start, and the network configuration
 * library fails its set-up when one is refused.
 */
static int dw3000_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);

	LOG_DBG("Transmit power is not set; %d dBm requested", dbm);

	return 0;
}

/* Without a channel the PLL is not locked and the transceiver takes no command. */
static int dw3000_start(const struct device *dev)
{
	struct dw3000_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	k_mutex_lock(&data->chip_lock, K_FOREVER);

	if (data->started) {
		goto out_unlock;
	}

	if (data->channel == NULL) {
		ret = -EIO;
		goto out_unlock;
	}

	if (dw3000_fast_cmd(dev, DW3000_CMD_RX) != 0) {
		ret = -EIO;
		goto out_unlock;
	}

	data->started = true;

out_unlock:
	k_mutex_unlock(&data->chip_lock);
	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int dw3000_stop(const struct device *dev)
{
	struct dw3000_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	k_mutex_lock(&data->chip_lock, K_FOREVER);

	if (!data->started) {
		goto out_unlock;
	}

	if (data->channel != NULL) {
		if (dw3000_fast_cmd(dev, DW3000_CMD_TXRXOFF) != 0) {
			ret = -EIO;
			goto out_unlock;
		}
	}

	data->started = false;

out_unlock:
	k_mutex_unlock(&data->chip_lock);
	k_mutex_unlock(&data->api_lock);

	return ret;
}

/*
 * Serve the events pending before a transmission. CMD_TXRXOFF clears every
 * event (9, Table 46, p238), so a reception that ended since the interrupt
 * thread last looked would be lost with it; the frame it holds, if any, is
 * returned for the caller to deliver once chip_lock is released. The caller
 * holds chip_lock.
 */
static int dw3000_tx_prepare(const struct device *dev, struct net_pkt **rx_pkt)
{
	uint32_t status;
	int ret;

	ret = dw3000_reg_read(dev, DW3000_SYS_STATUS, sizeof(status), &status);
	if (ret != 0) {
		return ret;
	}

	if ((status & DW3000_IRQ_EVENTS) != 0U) {
		*rx_pkt = dw3000_handle_events(dev, status);
	}

	return 0;
}

/*
 * Load a frame and start sending it (3.1, p33). A transmission starts from
 * IDLE_PLL, so the receiver is turned off first. The transceiver turns it back
 * on itself once the frame is sent (5.6, p55), and appends the FCS, which the
 * frame length written to it includes (8.2.2.8, p85). The caller holds
 * chip_lock.
 */
static int dw3000_tx_start(const struct device *dev, const struct net_buf *frag,
			   struct net_pkt **rx_pkt)
{
	struct dw3000_data *data = dev->data;
	uint32_t fctrl = FIELD_PREP(DW3000_TX_FCTRL_TXFLEN, frag->len + IEEE802154_FCS_LENGTH) |
			 FIELD_PREP(DW3000_TX_FCTRL_TXPSR, DW3000_TX_FCTRL_TXPSR_64) |
			 DW3000_TX_FCTRL_TXBR;
	int ret;

	ret = dw3000_tx_prepare(dev, rx_pkt);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_fast_cmd(dev, DW3000_CMD_TXRXOFF);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_write(dev, DW3000_TX_BUFFER, frag->data, frag->len);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_write(dev, DW3000_TX_FCTRL, sizeof(fctrl), fctrl);
	if (ret != 0) {
		return ret;
	}

	k_sem_reset(&data->tx_done);
	data->tx_busy = true;

	return dw3000_fast_cmd(dev, DW3000_CMD_TX_W4R);
}

/*
 * Give up on a transmission and return to reception. CMD_TXRXOFF also "clears
 * any events" (9, Table 46, p238), so none is left to be taken for the end of
 * the next transmission. The caller holds chip_lock.
 */
static void dw3000_tx_abort(const struct device *dev)
{
	struct dw3000_data *data = dev->data;

	if (dw3000_fast_cmd(dev, DW3000_CMD_TXRXOFF) != 0) {
		LOG_ERR("Failed to abort a transmission");
	}

	data->tx_busy = false;
	dw3000_rx_resume(dev);
}

/*
 * Report the time the frame was sent: that of its RMARKER leaving the antenna
 * (8.2.2.17, p103). A frame that was sent is not failed for its time stamp.
 */
static void dw3000_tx_stamp(const struct device *dev, struct net_pkt *pkt)
{
	uint64_t stamp;

	if (!IS_ENABLED(CONFIG_NET_PKT_TIMESTAMP)) {
		return;
	}

	if (dw3000_read_stamp(dev, DW3000_TX_TIME, &stamp) != 0) {
		LOG_WRN("Failed to read a transmit time stamp");
		return;
	}

	net_pkt_set_timestamp_ns(pkt, dw3000_stamp_to_ns(stamp));
}

/*
 * chip_lock is released while the frame is on air, for the interrupt thread to
 * run and report the end of the transmission.
 */
static int dw3000_tx(const struct device *dev, enum ieee802154_tx_mode mode, struct net_pkt *pkt,
		     struct net_buf *frag)
{
	struct dw3000_data *data = dev->data;
	struct net_pkt *rx_pkt = NULL;
	int ret;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->started) {
		ret = -ENETDOWN;
		goto out_unlock;
	}

	if (mode != IEEE802154_TX_MODE_DIRECT) {
		ret = -ENOTSUP;
		goto out_unlock;
	}

	if (frag->len > IEEE802154_MTU) {
		ret = -EINVAL;
		goto out_unlock;
	}

	/* The channel only changes under api_lock */
	if (data->channel == NULL) {
		ret = -EIO;
		goto out_unlock;
	}

	k_mutex_lock(&data->chip_lock, K_FOREVER);
	ret = dw3000_tx_start(dev, frag, &rx_pkt);
	k_mutex_unlock(&data->chip_lock);

	if (rx_pkt != NULL) {
		dw3000_rx_deliver(dev, rx_pkt);
	}

	if (ret == 0) {
		ret = k_sem_take(&data->tx_done, DW3000_TX_TIMEOUT);
	}

	k_mutex_lock(&data->chip_lock, K_FOREVER);

	if (ret == 0) {
		dw3000_tx_stamp(dev, pkt);
	} else {
		dw3000_tx_abort(dev);
		ret = -EIO;
	}

	k_mutex_unlock(&data->chip_lock);

out_unlock:
	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int dw3000_configure(const struct device *dev, enum ieee802154_config_type type,
			    const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(config);

	return -ENOTSUP;
}

static int dw3000_attr_get(const struct device *dev, enum ieee802154_attr attr,
			   struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	if (ieee802154_attr_get_channel_page_and_range(
		    attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_FOUR_HRP_UWB, &dw3000_supported_channels,
		    value) == 0) {
		return 0;
	}

	if (attr == IEEE802154_ATTR_PHY_HRP_UWB_SUPPORTED_PRFS) {
		value->phy_hrp_uwb_supported_nominal_prfs = IEEE802154_PHY_HRP_UWB_NOMINAL_64_M;
		return 0;
	}

	return -ENOENT;
}

/* The interface gets a random, locally administered EUI-64 */
static void dw3000_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dw3000_data *data = dev->data;

	sys_rand_get(data->mac_addr, sizeof(data->mac_addr));
	data->mac_addr[0] = (data->mac_addr[0] & ~0x01U) | 0x02U;

	(void)net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr),
				   NET_LINK_IEEE802154);
	data->iface = iface;

	ieee802154_init(iface);
}

static const struct ieee802154_radio_api dw3000_radio_api = {
	.iface_api.init = dw3000_iface_init,

	.get_capabilities = dw3000_get_capabilities,
	.cca = dw3000_cca,
	.set_channel = dw3000_set_channel,
	.set_txpower = dw3000_set_txpower,
	.start = dw3000_start,
	.stop = dw3000_stop,
	.tx = dw3000_tx,
	.configure = dw3000_configure,
	.attr_get = dw3000_attr_get,
};

/*
 * Before the transceiver reaches IDLE_RC its SPI is limited to 7 MHz (2.4,
 * Table 4, p19), so it is left alone until the SPIRDY event, which is enabled
 * by default, raises the interrupt line (2.3.2, p16).
 */
static int dw3000_wait_idle_rc(const struct device *dev)
{
	const struct dw3000_config *cfg = dev->config;

	for (uint32_t i = 0; i < DW3000_POLL_COUNT; i++) {
		k_msleep(1);

		if (gpio_pin_get_dt(&cfg->irq_gpio) > 0) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

/*
 * RSTn is driven by the transceiver and must never be driven high, so it is
 * pulled low and then released (Datasheet, pin description, p9). The
 * transceiver restarts as after power-up, in about 1 ms (Datasheet 6, Figure
 * 21, p33).
 */
static int dw3000_reset(const struct device *dev)
{
	const struct dw3000_config *cfg = dev->config;
	int ret;

	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	k_msleep(DW3000_RESET_PULSE_MS);

	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	return dw3000_wait_idle_rc(dev);
}

/*
 * The manual tells the host to check that the device ID is one its software
 * supports before going on (8.2.2.1, p74).
 */
static int dw3000_check_id(const struct device *dev)
{
	uint32_t id;
	int ret;

	ret = dw3000_reg_read(dev, DW3000_DEV_ID, sizeof(id), &id);
	if (ret != 0) {
		return ret;
	}

	if ((id & ~DW3000_DEV_ID_REV) != (DW3000_DEV_ID_DW3X10 & ~DW3000_DEV_ID_REV)) {
		LOG_ERR("Unsupported device ID 0x%08x", id);
		return -ENODEV;
	}

	return 0;
}

/*
 * The receiver must be calibrated before it is first enabled, and the
 * transceiver does that itself when it wakes up with ONW_PGFCAL set (4.1, p38
 * and 8.2.11.1, p166). So it is sent to DEEPSLEEP once, from IDLE_RC, and woken
 * up by holding SPICSn low with SPIMOSI low: "a dummy SPI read of sufficient
 * length" (2.5.1.1, p24).
 *
 * Two things here are not in the manual and were seen on the DW3110. The
 * calibration has no effect unless the transceiver goes on to IDLE_PLL as it
 * wakes up, which ONW_GO2IDLE asks for. And no SPIRDY event follows such a
 * wake-up, so the start-up time is waited for instead, and the device ID read
 * again to see that the transceiver answers.
 *
 * Its registers are at their reset values then, and it is returned to IDLE_RC
 * here to be configured from there.
 */
static int dw3000_rx_calibrate(const struct device *dev)
{
	const struct dw3000_config *cfg = dev->config;
	struct dw3000_data *data = dev->data;
	const struct spi_buf tx_buf = {.buf = (uint8_t *)dw3000_wake, .len = sizeof(dw3000_wake)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	uint32_t result;
	int ret;

	ret = dw3000_reg_apply(dev, dw3000_sleep_ops, ARRAY_SIZE(dw3000_sleep_ops));
	if (ret != 0) {
		return ret;
	}

	/* Saving the configuration takes about 85 us (2.5.1.2, p24). Let it sleep first. */
	k_msleep(1);

	ret = spi_write(cfg->bus.bus, &data->wake_cfg, &tx);
	if (ret != 0) {
		return ret;
	}

	k_msleep(DW3000_WAKE_UP_MS);

	ret = dw3000_check_id(dev);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_wait(dev, DW3000_SYS_STATUS, sizeof(uint32_t), DW3000_SYS_STATUS_CPLOCK);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_wait(dev, DW3000_RX_CAL_STS, sizeof(uint8_t), DW3000_RX_CAL_STS_DONE);
	if (ret != 0) {
		return ret;
	}

	ARRAY_FOR_EACH(dw3000_rx_cal_results, i) {
		ret = dw3000_reg_read(dev, dw3000_rx_cal_results[i], sizeof(result), &result);
		if (ret != 0) {
			return ret;
		}

		if (FIELD_GET(DW3000_RX_CAL_RES, result) == DW3000_RX_CAL_RES_FAILED) {
			return -EIO;
		}
	}

	return dw3000_reg_apply(dev, dw3000_idle_rc_ops, ARRAY_SIZE(dw3000_idle_rc_ops));
}

/*
 * Have the transceiver copy its LDO and bias trim values from its OTP memory
 * (7.3, p69). The LDO value has to be loaded after a wake-up (2.5.1.3, p26), in
 * IDLE_RC (2.4, Table 4, p19), and the bias value "on power up, and also after
 * SLEEP/DEEPSLEEP" (8.2.15.7, p227).
 *
 * A memory that was never programmed gives an LDO value of zero, for which the
 * "default should be used" (8.2.8.8, p155): the value from before is put back.
 */
static int dw3000_load_trim(const struct device *dev)
{
	uint8_t before[DW3000_LDO_TUNE_LEN];
	uint8_t loaded[DW3000_LDO_TUNE_LEN];
	int ret;

	ret = dw3000_read(dev, DW3000_LDO_TUNE, before, sizeof(before));
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_apply(dev, dw3000_trim_ops, ARRAY_SIZE(dw3000_trim_ops));
	if (ret != 0) {
		return ret;
	}

	k_msleep(DW3000_OTP_LOAD_MS);

	ret = dw3000_read(dev, DW3000_LDO_TUNE, loaded, sizeof(loaded));
	if (ret != 0) {
		return ret;
	}

	if (sys_get_le64(loaded) != 0U) {
		return 0;
	}

	LOG_WRN("No LDO trim value in the OTP memory");

	return dw3000_write(dev, DW3000_LDO_TUNE, before, sizeof(before));
}

/* Take the transceiver from IDLE_RC to IDLE_PLL on channel 5 */
static int dw3000_setup(const struct device *dev)
{
	int ret;

	ret = dw3000_load_trim(dev);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_apply(dev, dw3000_tuning_ops, ARRAY_SIZE(dw3000_tuning_ops));
	if (ret != 0) {
		return ret;
	}

	/*
	 * Only the events that are handled may raise the interrupt line, or it
	 * would stay high with an event that nothing clears. Command and SPI
	 * error events are enabled after reset (8.2.2.13, p89).
	 */
	ret = dw3000_reg_write(dev, DW3000_SYS_ENABLE, sizeof(uint32_t), DW3000_IRQ_EVENTS);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_reg_write(dev, DW3000_SYS_ENABLE_HI, sizeof(uint16_t), 0U);
	if (ret != 0) {
		return ret;
	}

	return dw3000_tune(dev, &dw3000_channels[0]);
}

static int dw3000_gpio_init(const struct device *dev)
{
	const struct dw3000_config *cfg = dev->config;
	struct dw3000_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&cfg->irq_gpio) || !gpio_is_ready_dt(&cfg->reset_gpio)) {
		return -ENODEV;
	}

	/* The transceiver is woken up through SPICSn, so the WAKEUP pin stays low */
	if (cfg->wakeup_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->wakeup_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->wakeup_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			return ret;
		}
	}

	ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->irq_cb, dw3000_irq_handler, BIT(cfg->irq_gpio.pin));

	return gpio_add_callback_dt(&cfg->irq_gpio, &data->irq_cb);
}

static int dw3000_init(const struct device *dev)
{
	const struct dw3000_config *cfg = dev->config;
	struct dw3000_data *data = dev->data;
	int ret;

	k_mutex_init(&data->api_lock);
	k_mutex_init(&data->chip_lock);
	k_sem_init(&data->tx_done, 0, 1);
	k_sem_init(&data->irq_sem, 0, 1);

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	data->wake_cfg = cfg->bus.config;
	data->wake_cfg.frequency = MIN(cfg->bus.config.frequency, DW3000_WAKE_FREQUENCY);

	ret = dw3000_gpio_init(dev);
	if (ret != 0) {
		LOG_ERR("Failed to set up GPIOs: %d", ret);
		return ret;
	}

	ret = dw3000_reset(dev);
	if (ret != 0) {
		LOG_ERR("No response after reset: %d", ret);
		return ret;
	}

	ret = dw3000_check_id(dev);
	if (ret != 0) {
		return ret;
	}

	ret = dw3000_rx_calibrate(dev);
	if (ret != 0) {
		LOG_ERR("Failed to calibrate the receiver: %d", ret);
		return ret;
	}

	ret = dw3000_setup(dev);
	if (ret != 0) {
		LOG_ERR("Failed to set up the transceiver: %d", ret);
		return ret;
	}

	k_thread_create(&data->rx_thread, data->rx_stack, K_KERNEL_STACK_SIZEOF(data->rx_stack),
			dw3000_rx_thread, (void *)dev, NULL, NULL, DW3000_RX_THREAD_PRIO, 0,
			K_NO_WAIT);
	(void)k_thread_name_set(&data->rx_thread, "dw3000_rx");

	return gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

#if defined(CONFIG_IEEE802154_RAW_MODE)
#define DW3000_DEVICE_DEFINE(inst)                                                                 \
	DEVICE_DT_INST_DEFINE(inst, dw3000_init, NULL, &dw3000_data_##inst, &dw3000_config_##inst, \
			      POST_KERNEL, CONFIG_IEEE802154_DW3000_INIT_PRIO, &dw3000_radio_api)
#else
/*
 * Without a receive queue the L2 runs on the thread that serves the interrupt
 * line and sends acknowledgments from it, where tx() would wait for an event
 * that only this thread reports.
 */
BUILD_ASSERT(CONFIG_NET_TC_RX_COUNT > 0, "The DW3000 driver needs a network RX queue");

#define DW3000_DEVICE_DEFINE(inst)                                                                 \
	NET_DEVICE_DT_INST_DEFINE(inst, dw3000_init, NULL, &dw3000_data_##inst,                    \
				  &dw3000_config_##inst, CONFIG_IEEE802154_DW3000_INIT_PRIO,       \
				  &dw3000_radio_api, IEEE802154_L2,                                \
				  NET_L2_GET_CTX_TYPE(IEEE802154_L2), IEEE802154_MTU)
#endif /* CONFIG_IEEE802154_RAW_MODE */

#define DW3000_DEFINE(inst)                                                                        \
	BUILD_ASSERT(DT_INST_PROP(inst, spi_max_frequency) <= DW3000_SPI_MAX_FREQUENCY,            \
		     "SPI clock frequency exceeds supported maximum");                             \
	static const struct dw3000_config dw3000_config_##inst = {                                 \
		.bus = SPI_DT_SPEC_INST_GET(inst, DW3000_SPI_OPERATION),                           \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.wakeup_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, wakeup_gpios, {0}),                  \
	};                                                                                         \
	static struct dw3000_data dw3000_data_##inst;                                              \
	DW3000_DEVICE_DEFINE(inst);

DT_INST_FOREACH_STATUS_OKAY(DW3000_DEFINE)
