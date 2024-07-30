/*
 * Copyright (c) 2023 PHOENIX CONTACT Electronics GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_adin2111, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

#if CONFIG_ETH_ADIN2111_SPI_CFG0
#include <zephyr/sys/crc.h>
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */
#include <string.h>
#include <errno.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/drivers/ethernet/eth_adin2111.h>

#include "phy/phy_adin2111_priv.h"
#include "eth_adin2111_priv.h"

#define DT_DRV_COMPAT adi_adin2111

/* SPI Communication check retry delay */
#define ADIN2111_DEV_AWAIT_DELAY_POLL_US	100U
/* Number of retries SPI Communication check */
#define ADIN2111_DEV_AWAIT_RETRY_COUNT		200U

/* ADIN RESETC check retry delay */
#define ADIN2111_RESETC_AWAIT_DELAY_POLL_US	100U
/* Number of retries for ADIN RESETC check */
#define ADIN2111_RESETC_AWAIT_RETRY_COUNT	200U

/* Boot delay for clocks stabilisation (maximum 90ms) */
#define ADIN2111_HW_BOOT_DELAY_MS		100

/* MAC Address Rule and DA Filter multicast slot/idx */
#define ADIN2111_MULTICAST_ADDR_SLOT		0U
/* MAC Address Rule and DA Filter broadcast slot/idx */
#define ADIN2111_BROADCAST_ADDR_SLOT		1U
/* MAC Address Rule and DA Filter Port 1 slot/idx */
#define ADIN2111_UNICAST_P1_ADDR_SLOT		2U
/* MAC Address Rule and DA Filter Port 2 slot/idx */
#define ADIN2111_UNICAST_P2_ADDR_SLOT		3U
/* Free slots for further filtering */
#define ADIN2111_FILTER_FIRST_SLOT		4U
#define ADIN2111_FILTER_SLOTS			16U

/* As per RM rev. A table 3, t3 >= 50ms, delay for SPI interface to be ready */
#define ADIN2111_SPI_ACTIVE_DELAY_MS		50U
/* As per RM rev. A page 20: approximately 10 ms (maximum) for internal logic to be ready. */
#define ADIN2111_SW_RESET_DELAY_MS		10U

int eth_adin2111_mac_reset(const struct device *dev)
{
	uint32_t val;
	int ret;

	ret = eth_adin2111_reg_write(dev, ADIN2111_SOFT_RST_REG, ADIN2111_SWRESET_KEY1);
	if (ret < 0) {
		return ret;
	}
	ret = eth_adin2111_reg_write(dev, ADIN2111_SOFT_RST_REG, ADIN2111_SWRESET_KEY2);
	if (ret < 0) {
		return ret;
	}
	ret = eth_adin2111_reg_write(dev, ADIN2111_SOFT_RST_REG, ADIN2111_SWRELEASE_KEY1);
	if (ret < 0) {
		return ret;
	}
	ret = eth_adin2111_reg_write(dev, ADIN2111_SOFT_RST_REG, ADIN2111_SWRELEASE_KEY2);
	if (ret < 0) {
		return ret;
	}
	ret = eth_adin2111_reg_read(dev, ADIN1110_MAC_RST_STATUS_REG, &val);
	if (ret < 0) {
		return ret;
	}
	if (val == 0) {
		return -EBUSY;
	}

	return 0;
}

int eth_adin2111_reg_update(const struct device *dev, const uint16_t reg,
			    uint32_t mask,  uint32_t data)
{
	uint32_t val;
	int ret;

	ret = eth_adin2111_reg_read(dev, reg, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~mask;
	val |= mask & data;

	return eth_adin2111_reg_write(dev, reg, val);
}

struct net_if *eth_adin2111_get_iface(const struct device *dev, const uint16_t port_idx)
{
	struct adin2111_data *ctx = dev->data;

	return ((struct adin2111_port_data *)ctx->port[port_idx]->data)->iface;
}

int eth_adin2111_lock(const struct device *dev, k_timeout_t timeout)
{
	struct adin2111_data *ctx = dev->data;

	return k_mutex_lock(&ctx->lock, timeout);
}

int eth_adin2111_unlock(const struct device *dev)
{
	struct adin2111_data *ctx = dev->data;

	return k_mutex_unlock(&ctx->lock);
}

static inline bool eth_adin2111_oa_get_parity(const uint32_t x)
{
	uint32_t y;

	y = x ^ (x >> 1);
	y = y ^ (y >> 2);
	y = y ^ (y >> 4);
	y = y ^ (y >> 8);
	y = y ^ (y >> 16);

	return !(y & 1);
}

int eth_adin2111_oa_spi_xfer(const struct device *dev, uint8_t *buf_rx, uint8_t *buf_tx, int len)
{
	const struct adin2111_config *cfg = dev->config;

	struct spi_buf tx_buf[1];
	struct spi_buf rx_buf[1];
	struct spi_buf_set tx;
	struct spi_buf_set rx;
	int ret;

	tx_buf[0].buf = buf_tx;
	tx_buf[0].len = len;
	rx_buf[0].buf = buf_rx;
	rx_buf[0].len = len;

	rx.buffers = rx_buf;
	rx.count = 1;
	tx.buffers = tx_buf;
	tx.count = 1;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret < 0) {
		LOG_ERR("ERRR dma!\n");
		return ret;
	}

	return 0;
}

static int eth_adin2111_reg_read_oa(const struct device *dev, const uint16_t reg,
				    uint32_t *val)
{
	struct adin2111_data *ctx = dev->data;
	uint32_t pval;
	uint32_t *hdr = (uint32_t *)ctx->oa_tx_buf;
	int len;
	int ret;

	*hdr = reg << 8;
	if (reg >= 0x30) {
		*hdr |= ADIN2111_OA_CTL_MMS;
	}

	*hdr |= eth_adin2111_oa_get_parity(*hdr);
	*hdr = sys_cpu_to_be32(*hdr);

	len = (ctx->oa_prot) ? ADIN2111_OA_CTL_LEN_PROT : ADIN2111_OA_CTL_LEN;

	ret = eth_adin2111_oa_spi_xfer(dev, ctx->oa_rx_buf, ctx->oa_tx_buf, len);
	if (ret < 0) {
		return ret;
	}

	*val = sys_be32_to_cpu(*(uint32_t *)&ctx->oa_rx_buf[8]);

	/* In protected mode read data is followed by its compliment value */
	if (ctx->oa_prot) {
		pval = sys_be32_to_cpu(*(uint32_t *)&ctx->oa_rx_buf[12]);
		if (*val != ~pval) {
			LOG_ERR("OA protected mode rx error !");
			return -1;
		}
	}

	return 0;
}

static int eth_adin2111_reg_write_oa(const struct device *dev, const uint16_t reg,
				     uint32_t val)
{
	struct adin2111_data *ctx = dev->data;
	uint32_t pval;
	uint32_t *hdr = (uint32_t *)ctx->oa_tx_buf;
	int len;
	int ret;

	*hdr = reg << 8 | ADIN2111_OA_CTL_WNR;
	if (reg >= 0x30) {
		*hdr |= ADIN2111_OA_CTL_MMS;
	}

	*hdr |= eth_adin2111_oa_get_parity(*hdr);
	*hdr = sys_cpu_to_be32(*hdr);

	len = (ctx->oa_prot) ? ADIN2111_OA_CTL_LEN_PROT : ADIN2111_OA_CTL_LEN;

	*(uint32_t *)&ctx->oa_tx_buf[4] = sys_cpu_to_be32(val);
	if (ctx->oa_prot) {
		*(uint32_t *)&ctx->oa_tx_buf[8] = sys_cpu_to_be32(~val);
	}

	ret = eth_adin2111_oa_spi_xfer(dev, ctx->oa_rx_buf, ctx->oa_tx_buf, len);
	if (ret < 0) {
		return ret;
	}

	if (ctx->oa_prot) {
		pval = sys_be32_to_cpu(*(uint32_t *)&ctx->oa_rx_buf[12]);
		if (val != ~pval) {
			LOG_ERR("OA protected mode tx error !");
			return -1;
		}
	}

	return 0;
}

int eth_adin2111_oa_data_read(const struct device *dev, const uint16_t port_idx)
{
	struct adin2111_data *ctx = dev->data;
	struct net_if *iface = ((struct adin2111_port_data *)ctx->port[port_idx]->data)->iface;
	struct net_pkt *pkt;
	uint32_t hdr, ftr;
	int i, len, rx_pos, ret, rca, swo;

	ret = eth_adin2111_reg_read(dev, ADIN2111_BUFSTS, &rca);
	if (ret < 0) {
		LOG_ERR("can't read BUFSTS");
		return -EIO;
	}

	rca &= ADIN2111_BUFSTS_RCA_MASK;

	/* Preare all tx headers */
	for (i = 0, len = 0; i < rca; ++i) {
		hdr = ADIN2111_OA_DATA_HDR_DNC;
		hdr |= eth_adin2111_oa_get_parity(hdr);

		*(uint32_t *)&ctx->oa_tx_buf[len] = sys_cpu_to_be32(hdr);

		len += sizeof(uint32_t) + ctx->oa_cps;
	}

	ret = eth_adin2111_oa_spi_xfer(dev, ctx->oa_rx_buf, ctx->oa_tx_buf, len);
	if (ret < 0) {
		LOG_ERR("SPI xfer failed");
		return ret;
	}

	for (i = 0, rx_pos = 0; i < rca; ++i) {

		ftr = sys_be32_to_cpu(*(uint32_t *)&ctx->oa_rx_buf[rx_pos + ctx->oa_cps]);

		if (eth_adin2111_oa_get_parity(ftr)) {
			LOG_ERR("OA RX: Footer parity error !");
			return -EIO;
		}
		if (!(ftr & ADIN2111_OA_DATA_FTR_SYNC)) {
			LOG_ERR("OA RX: Configuration not in sync !");
			return -EIO;
		}
		if (!(ftr & ADIN2111_OA_DATA_FTR_DV)) {
			LOG_DBG("OA RX: Data chunk not valid, skip !");
			goto update_pos;
		}
		if (ftr & ADIN2111_OA_DATA_FTR_SV) {
			swo = (ftr & ADIN2111_OA_DATA_FTR_SWO_MSK) >> ADIN2111_OA_DATA_FTR_SWO;
			if (swo != 0) {
				LOG_ERR("OA RX: Misalignbed start of frame !");
				return -EIO;
			}
			/* Reset store cursor */
			ctx->scur = 0;
		}

		len = (ftr & ADIN2111_OA_DATA_FTR_EV) ?
		       ((ftr & ADIN2111_OA_DATA_FTR_EBO_MSK) >> ADIN2111_OA_DATA_FTR_EBO) + 1 :
		       ctx->oa_cps;
		memcpy(&ctx->buf[ctx->scur], &ctx->oa_rx_buf[rx_pos], len);
		ctx->scur += len;

		if (ftr & ADIN2111_OA_DATA_FTR_EV) {
			pkt = net_pkt_rx_alloc_with_buffer(iface, CONFIG_ETH_ADIN2111_BUFFER_SIZE,
							   AF_UNSPEC, 0,
							   K_MSEC(CONFIG_ETH_ADIN2111_TIMEOUT));
			if (!pkt) {
				LOG_ERR("OA RX: cannot allcate packet space, skipping.");
				return -EIO;
			}
			/* Skipping CRC32 */
			ret = net_pkt_write(pkt, ctx->buf, ctx->scur - sizeof(uint32_t));
			if (ret < 0) {
				net_pkt_unref(pkt);
				LOG_ERR("Failed to write pkt, scur %d, err %d", ctx->scur, ret);
				return ret;
			}
			ret = net_recv_data(iface, pkt);
			if (ret < 0) {
				net_pkt_unref(pkt);
				LOG_ERR("Port %u failed to enqueue frame to RX queue, %d",
					port_idx, ret);
				return ret;
			}
		}
update_pos:
		rx_pos += ctx->oa_cps + sizeof(uint32_t);
	}

	return ret;
}

/*
 * Setting up for a single dma transfer.
 */
static int eth_adin2111_send_oa_frame(const struct device *dev, struct net_pkt *pkt,
				      const uint16_t port_idx)
{
	struct adin2111_data *ctx = dev->data;
	uint16_t clen, len = net_pkt_get_len(pkt);
	uint32_t hdr;
	uint8_t chunks, i;
	int ret, txc, cur;

	chunks = len / ctx->oa_cps;

	if (len % ctx->oa_cps) {
		chunks++;
	}

	ret = eth_adin2111_reg_read(dev, ADIN2111_BUFSTS, &txc);
	if (ret < 0) {
		LOG_ERR("Cannot read txc");
		return -EIO;
	}

	txc = (txc & ADIN2111_BUFSTS_TXC_MASK) >> ADIN2111_BUFSTS_TXC;
	if (txc < chunks) {
		return -EIO;
	}

	/* Prepare for single dma transfer */
	for (i = 1, cur = 0; i <= chunks; i++) {
		hdr = ADIN2111_OA_DATA_HDR_DNC | ADIN2111_OA_DATA_HDR_DV |
			ADIN2111_OA_DATA_HDR_NORX;
		hdr |= (!!port_idx << ADIN2111_OA_DATA_HDR_VS);
		if (i == 1) {
			hdr |= ADIN2111_OA_DATA_HDR_SV;
		}
		if (i == chunks) {
			hdr |= ADIN2111_OA_DATA_HDR_EV;
			hdr |= (ctx->oa_cps - 1) << ADIN2111_OA_DATA_HDR_EBO;
		}

		hdr |= eth_adin2111_oa_get_parity(hdr);

		*(uint32_t *)&ctx->oa_tx_buf[cur] = sys_cpu_to_be32(hdr);
		cur += sizeof(uint32_t);

		clen = len > ctx->oa_cps ? ctx->oa_cps : len;
		ret = net_pkt_read(pkt, &ctx->oa_tx_buf[cur], clen);
		if (ret < 0) {
			LOG_ERR("Cannot read from tx packet");
			return ret;
		}
		cur += ctx->oa_cps;
		len -= clen;
	}

	ret = eth_adin2111_oa_spi_xfer(dev, ctx->oa_rx_buf, ctx->oa_tx_buf, cur);
	if (ret < 0) {
		LOG_ERR("Error on SPI xfer");
		return ret;
	}

	return 0;
}

static int eth_adin2111_reg_read_generic(const struct device *dev,
					 const uint16_t reg,
					 uint32_t *val)
{
	const struct adin2111_config *cfg = dev->config;
	size_t header_len = ADIN2111_READ_HEADER_SIZE;
	size_t read_len = sizeof(uint32_t);
	int ret;
#if CONFIG_ETH_ADIN2111_SPI_CFG0
	uint8_t rcv_crc;
	uint8_t comp_crc;
	uint8_t buf[ADIN2111_REG_READ_BUF_SIZE_CRC] = { 0 };
#else
	uint8_t buf[ADIN2111_REG_READ_BUF_SIZE] = { 0 };
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	/* spi header */
	*(uint16_t *)buf = htons((ADIN2111_READ_TXN_CTRL | reg));
#if CONFIG_ETH_ADIN2111_SPI_CFG0
	buf[2] = crc8_ccitt(0, buf, ADIN2111_SPI_HEADER_SIZE);
	/* TA */
	buf[3] = 0U;
	++header_len;
	++read_len;
#else
	/* TA */
	buf[2] = 0U;
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	const struct spi_buf tx_buf = { .buf = buf, .len = header_len + read_len };
	const struct spi_buf rx_buf = { .buf = buf, .len = header_len + read_len };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1U };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1U };

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_ETH_ADIN2111_SPI_CFG0
	comp_crc = crc8_ccitt(0, &buf[header_len], sizeof(uint32_t));
	rcv_crc = buf[header_len + sizeof(uint32_t)];

	if (rcv_crc != comp_crc) {
		/* invalid crc */
		return -EIO;
	}
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	*val = ntohl((*(uint32_t *)(&buf[header_len])));

	return ret;
}

static int eth_adin2111_reg_write_generic(const struct device *dev,
					  const uint16_t reg,
					  const uint32_t val)
{
	const struct adin2111_config *cfg = dev->config;
	size_t header_size = ADIN2111_WRITE_HEADER_SIZE;
	size_t data_size = sizeof(uint32_t);
#if CONFIG_ETH_ADIN2111_SPI_CFG0
	uint8_t buf[ADIN2111_REG_WRITE_BUF_SIZE_CRC] = { 0 };
#else
	uint8_t buf[ADIN2111_REG_WRITE_BUF_SIZE] = { 0 };
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	/* spi header */
	*(uint16_t *)buf = htons((ADIN2111_WRITE_TXN_CTRL | reg));
	#if CONFIG_ETH_ADIN2111_SPI_CFG0
	buf[2] = crc8_ccitt(0, buf, header_size);
	++header_size;
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	/* reg */
	*(uint32_t *)(buf + header_size) = htonl(val);
#if CONFIG_ETH_ADIN2111_SPI_CFG0
	buf[header_size + data_size] = crc8_ccitt(0, &buf[header_size], data_size);
	++data_size;
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	const struct spi_buf spi_tx_buf = {
		.buf = buf,
		.len = header_size + data_size
	};
	const struct spi_buf_set tx = { .buffers = &spi_tx_buf, .count = 1U };

	return spi_write_dt(&cfg->spi, &tx);
}

int eth_adin2111_reg_read(const struct device *dev, const uint16_t reg,
			  uint32_t *val)
{
	struct adin2111_data *ctx = dev->data;
	int rval;

	if (ctx->oa) {
		rval = eth_adin2111_reg_read_oa(dev, reg, val);
	} else {
		rval = eth_adin2111_reg_read_generic(dev, reg, val);
	}

	return rval;
}

int eth_adin2111_reg_write(const struct device *dev, const uint16_t reg,
			   const uint32_t val)
{
	struct adin2111_data *ctx = dev->data;
	int rval;

	if (ctx->oa) {
		rval = eth_adin2111_reg_write_oa(dev, reg, val);
	} else {
		rval = eth_adin2111_reg_write_generic(dev, reg, val);
	}

	return rval;
}

static int adin2111_read_fifo(const struct device *dev, const uint16_t port_idx)
{
	const struct adin2111_config *cfg = dev->config;
	struct adin2111_data *ctx = dev->data;
	struct net_if *iface;
	struct net_pkt *pkt;
	uint16_t fsize_reg = ((port_idx == 0U) ? ADIN2111_P1_RX_FSIZE : ADIN2111_P2_RX_FSIZE);
	uint16_t rx_reg = ((port_idx == 0U) ? ADIN2111_P1_RX : ADIN2111_P2_RX);
	uint32_t fsize;
	uint32_t fsize_real;
	uint32_t padding_len;
#if CONFIG_ETH_ADIN2111_SPI_CFG0
	uint8_t cmd_buf[ADIN2111_FIFO_READ_CMD_BUF_SIZE_CRC] = { 0 };
#else
	uint8_t cmd_buf[ADIN2111_FIFO_READ_CMD_BUF_SIZE] = { 0 };
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */
	int ret;

	iface = ((struct adin2111_port_data *)ctx->port[port_idx]->data)->iface;

	/* get received frame size in bytes */
	ret = eth_adin2111_reg_read(dev, fsize_reg, &fsize);
	if (ret < 0) {
		eth_stats_update_errors_rx(iface);
		LOG_ERR("Port %u failed to read RX FSIZE, %d", port_idx, ret);
		return ret;
	}

	/* burst read must be in multiples of 4 */
	padding_len = ((fsize % 4) == 0) ? 0U : (ROUND_UP(fsize, 4U) - fsize);
	/* actual frame length is FSIZE - FRAME HEADER - CRC32 */
	fsize_real = fsize - (ADIN2111_FRAME_HEADER_SIZE + sizeof(uint32_t));

	/* spi header */
	*(uint16_t *)cmd_buf = htons((ADIN2111_READ_TXN_CTRL | rx_reg));
#if CONFIG_ETH_ADIN2111_SPI_CFG0
	cmd_buf[2] = crc8_ccitt(0, cmd_buf, ADIN2111_SPI_HEADER_SIZE);
	/* TA */
	cmd_buf[3] = 0U;
#else
	/* TA */
	cmd_buf[2] = 0U;
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	const struct spi_buf tx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf rx_buf[3] = {
		{.buf = NULL, .len = sizeof(cmd_buf) + ADIN2111_FRAME_HEADER_SIZE},
		{.buf = ctx->buf, .len = fsize_real},
		{.buf = NULL, .len = padding_len }
	};
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1U };
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ((padding_len == 0U) ? 2U : 3U)
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret < 0) {
		eth_stats_update_errors_rx(iface);
		LOG_ERR("Port %u failed to read RX FIFO, %d", port_idx, ret);
		return ret;
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface, fsize_real, AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_ADIN2111_TIMEOUT));
	if (!pkt) {
		eth_stats_update_errors_rx(iface);
		LOG_ERR("Port %u failed to alloc frame RX buffer, %u bytes",
			port_idx, fsize_real);
		return -ENOMEM;
	}

	ret = net_pkt_write(pkt, ctx->buf, fsize_real);
	if (ret < 0) {
		eth_stats_update_errors_rx(iface);
		net_pkt_unref(pkt);
		LOG_ERR("Port %u failed to fill RX frame, %d", port_idx, ret);
		return ret;
	}

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		eth_stats_update_errors_rx(iface);
		net_pkt_unref(pkt);
		LOG_ERR("Port %u failed to enqueue frame to RX queue, %d",
			port_idx, ret);
		return ret;
	}

	eth_stats_update_bytes_rx(iface, fsize_real);
	eth_stats_update_pkts_rx(iface);

	return ret;
}

static inline void adin2111_port_on_phyint(const struct device *dev)
{
	const struct adin2111_port_config *cfg = dev->config;
	struct adin2111_port_data *data = dev->data;
	struct phy_link_state state;

	if (phy_adin2111_handle_phy_irq(cfg->phy, &state) < 0) {
		/* no change or error */
		return;
	}

	if (state.is_up) {
		net_eth_carrier_on(data->iface);
	} else {
		net_eth_carrier_off(data->iface);
	}
}

static void adin2111_offload_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct adin2111_data *ctx = dev->data;
	const struct adin2111_config *adin_cfg = dev->config;
	bool is_adin2111 = (adin_cfg->id == ADIN2111_MAC);
	uint32_t status0;
	uint32_t status1;
	int ret;

	for (;;) {
		/* await INT */
		k_sem_take(&ctx->offload_sem, K_FOREVER);

		/* lock device */
		eth_adin2111_lock(dev, K_FOREVER);

		/* disable interrupts */
		ret = eth_adin2111_reg_write(dev, ADIN2111_IMASK0, UINT32_MAX);
		if (ret < 0) {
			goto continue_unlock;
		}
		ret = eth_adin2111_reg_write(dev, ADIN2111_IMASK1, UINT32_MAX);
		if (ret < 0) {
			goto continue_unlock;
		}

		/* read interrupts */
		ret = eth_adin2111_reg_read(dev, ADIN2111_STATUS0, &status0);
		if (ret < 0) {
			goto continue_unlock;
		}
		ret = eth_adin2111_reg_read(dev, ADIN2111_STATUS1, &status1);
		if (ret < 0) {
			goto continue_unlock;
		}

		if (!ctx->oa) {
#if CONFIG_ETH_ADIN2111_SPI_CFG0
			if (status0 & ADIN2111_STATUS1_SPI_ERR) {
				LOG_WRN("Detected TX SPI CRC error");
			}
#endif
		}

		/* handle port 1 phy interrupts */
		if (status0 & ADIN2111_STATUS0_PHYINT) {
			adin2111_port_on_phyint(ctx->port[0]);
		}

		/* handle port 2 phy interrupts */
		if ((status1 & ADIN2111_STATUS1_PHYINT) && is_adin2111) {
			adin2111_port_on_phyint(ctx->port[1]);
		}

		if (ctx->oa) {
			if (status1 & ADIN2111_STATUS1_P1_RX_RDY) {
				ret = eth_adin2111_oa_data_read(dev, 0);
				if (ret < 0) {
					break;
				}
			}
			if (status1 & ADIN2111_STATUS1_P2_RX_RDY) {
				ret = eth_adin2111_oa_data_read(dev, 1);
				if (ret < 0) {
					break;
				}
			}
			goto continue_unlock;
		}

		/* handle port 1 rx */
		if (status1 & ADIN2111_STATUS1_P1_RX_RDY) {
			do {
				ret = adin2111_read_fifo(dev, 0U);
				if (ret < 0) {
					break;
				}

				ret = eth_adin2111_reg_read(dev, ADIN2111_STATUS1, &status1);
				if (ret < 0) {
					goto continue_unlock;
				}
			} while (!!(status1 & ADIN2111_STATUS1_P1_RX_RDY));
		}

		/* handle port 2 rx */
		if ((status1 & ADIN2111_STATUS1_P2_RX_RDY) && is_adin2111) {
			do {
				ret = adin2111_read_fifo(dev, 1U);
				if (ret < 0) {
					break;
				}

				ret = eth_adin2111_reg_read(dev, ADIN2111_STATUS1, &status1);
				if (ret < 0) {
					goto continue_unlock;
				}
			} while (!!(status1 & ADIN2111_STATUS1_P2_RX_RDY));
		}

continue_unlock:
		/* clear interrupts */
		ret = eth_adin2111_reg_write(dev, ADIN2111_STATUS0, ADIN2111_STATUS0_CLEAR);
		if (ret < 0) {
			LOG_ERR("Failed to clear STATUS0, %d", ret);
		}
		ret = eth_adin2111_reg_write(dev, ADIN2111_STATUS1, ADIN2111_STATUS1_CLEAR);
		if (ret < 0) {
			LOG_ERR("Failed to clear STATUS1, %d", ret);
		}
		/* enable interrupts */
		ret = eth_adin2111_reg_write(dev, ADIN2111_IMASK0, ctx->imask0);
		if (ret < 0) {
			LOG_ERR("Failed to write IMASK0, %d", ret);
		}
		ret = eth_adin2111_reg_write(dev, ADIN2111_IMASK1, ctx->imask1);
		if (ret < 0) {
			LOG_ERR("Failed to write IMASK1, %d", ret);
		}
		eth_adin2111_unlock(dev);
	}
}

static void adin2111_int_callback(const struct device *dev,
				  struct gpio_callback *cb,
				  uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct adin2111_data *ctx = CONTAINER_OF(cb, struct adin2111_data, gpio_int_callback);

	k_sem_give(&ctx->offload_sem);
}

static int adin2111_read_tx_space(const struct device *dev, uint32_t *space)
{
	uint32_t val;
	int ret;

	ret = eth_adin2111_reg_read(dev, ADIN2111_TX_SPACE, &val);
	if (ret < 0) {
		return ret;
	}

	/* tx space is a number of halfwords (16-bits), multiply by 2 for bytes */
	*space = val * 2;

	return ret;
}

static int adin2111_port_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct adin2111_port_config *cfg = dev->config;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct adin2111_port_data *data = dev->data;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	const struct device *adin = cfg->adin;
	struct adin2111_data *ctx = cfg->adin->data;
	size_t pkt_len = net_pkt_get_len(pkt);
	size_t header_size = ADIN2111_WRITE_HEADER_SIZE;
	size_t padded_size;
	size_t burst_size;
	uint32_t tx_space;
	int ret;

	eth_adin2111_lock(adin, K_FOREVER);

	if (ctx->oa) {
		uint32_t val, rca = 0;
		/*
		 * By high-traffic zperf test, noted that ADIN2111 does not like we send
		 * if there is something to be received. It stops to issue rx interrupts
		 * and zperf transfer hangs. Forcing a receive for this case.
		 */
		ret = eth_adin2111_reg_read(adin, ADIN2111_BUFSTS, &val);
		if (ret < 0) {
			return ret;
		}
		rca = val & ADIN2111_BUFSTS_RCA_MASK;

		if (rca > 0) {
			eth_adin2111_unlock(adin);
			k_sem_give(&ctx->offload_sem);
			k_yield();
			eth_adin2111_lock(adin, K_FOREVER);
		}

		ret = eth_adin2111_send_oa_frame(cfg->adin, pkt, htons(cfg->port_idx));

		goto end_check;
	}

	/* query remaining tx fifo space */
	ret = adin2111_read_tx_space(adin, &tx_space);
	if (ret < 0) {
		eth_stats_update_errors_tx(data->iface);
		LOG_ERR("Failed to read TX FIFO space, %d", ret);
		goto end_unlock;
	}

	/**
	 * verify that there is space for the frame
	 * (frame + 2b header + 2b size field)
	 */
	if (tx_space <
	   (pkt_len + ADIN2111_FRAME_HEADER_SIZE + ADIN2111_INTERNAL_HEADER_SIZE)) {
		/* tx buffer is full */
		eth_stats_update_errors_tx(data->iface);
		ret = -EBUSY;
		goto end_unlock;
	}

	/**
	 * pad to 64 bytes, otherwise MAC/PHY has to do it
	 * internally MAC adds 4 bytes for forward error correction
	 */
	if ((pkt_len + ADIN2111_TX_FIFO_BUFFER_MARGIN) < 64) {
		padded_size = pkt_len
			+ (64 - (pkt_len + ADIN2111_TX_FIFO_BUFFER_MARGIN))
			+ ADIN2111_FRAME_HEADER_SIZE;
	} else {
		padded_size = pkt_len + ADIN2111_FRAME_HEADER_SIZE;
	}

	/* prepare burst write (write data must be in multiples of 4) */
	burst_size = ROUND_UP(padded_size, 4);
	if ((burst_size + ADIN2111_WRITE_HEADER_SIZE) > CONFIG_ETH_ADIN2111_BUFFER_SIZE) {
		ret = -ENOMEM;
		eth_stats_update_errors_tx(data->iface);
		goto end_unlock;
	}

	/* prepare tx buffer */
	memset(ctx->buf, 0, burst_size + ADIN2111_WRITE_HEADER_SIZE);

	/* spi header */
	*(uint16_t *)ctx->buf = htons(ADIN2111_TXN_CTRL_TX_REG);
#if CONFIG_ETH_ADIN2111_SPI_CFG0
	ctx->buf[2] = crc8_ccitt(0, ctx->buf, header_size);
	++header_size;
#endif /* CONFIG_ETH_ADIN2111_SPI_CFG0 */

	/* frame header */
	*(uint16_t *)(ctx->buf + header_size) = htons(cfg->port_idx);

	/* read pkt into tx buffer */
	ret = net_pkt_read(pkt,
			   (ctx->buf + header_size + ADIN2111_FRAME_HEADER_SIZE),
			   pkt_len);
	if (ret < 0) {
		eth_stats_update_errors_tx(data->iface);
		LOG_ERR("Port %u failed to read PKT into TX buffer, %d",
			cfg->port_idx, ret);
		goto end_unlock;
	}

	/* write transmit size */
	ret = eth_adin2111_reg_write(adin, ADIN2111_TX_FSIZE, padded_size);
	if (ret < 0) {
		eth_stats_update_errors_tx(data->iface);
		LOG_ERR("Port %u write FSIZE failed, %d", cfg->port_idx, ret);
		goto end_unlock;
	}

	/* write transaction */
	const struct spi_buf buf = {
		.buf = ctx->buf,
		.len = header_size + burst_size
	};
	const struct spi_buf_set tx = { .buffers = &buf, .count = 1U };

	ret = spi_write_dt(&((const struct adin2111_config *) adin->config)->spi,
			   &tx);
end_check:
	if (ret < 0) {
		eth_stats_update_errors_tx(data->iface);
		LOG_ERR("Port %u frame SPI write failed, %d", cfg->port_idx, ret);
		goto end_unlock;
	}

	eth_stats_update_bytes_tx(data->iface, pkt_len);
	eth_stats_update_pkts_tx(data->iface);

end_unlock:
	eth_adin2111_unlock(adin);
	return ret;
}

static int adin2111_config_sync(const struct device *dev)
{
	int ret;
	uint32_t val;

	ret = eth_adin2111_reg_read(dev, ADIN2111_CONFIG0, &val);
	if (ret < 0) {
		return ret;
	}

	val |= ADIN2111_CONFIG0_SYNC;

	ret = eth_adin2111_reg_write(dev, ADIN2111_CONFIG0, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int adin2111_write_filter_address(const struct device *dev,
					 uint8_t *addr, uint8_t *mask,
					 uint32_t rules, uint16_t slot)
{
	uint16_t offset = slot * 2U;
	int ret;

	ret = eth_adin2111_reg_write(dev, ADIN2111_ADDR_FILT_UPR + offset,
				     rules | sys_get_be16(&addr[0]));
	if (ret < 0) {
		return ret;
	}

	ret = eth_adin2111_reg_write(dev, ADIN2111_ADDR_FILT_LWR  + offset,
				     sys_get_be32(&addr[2]));
	if (ret < 0) {
		return ret;
	}

	if (offset > 2U) {
		/* mask filter addresses are limited to 2 */
		return 0;
	}

	ret = eth_adin2111_reg_write(dev, ADIN2111_ADDR_MSK_UPR + offset,
				     sys_get_be16(&mask[0]));
	if (ret < 0) {
		return ret;
	}

	ret = eth_adin2111_reg_write(dev, ADIN2111_ADDR_MSK_LWR + offset,
				     sys_get_be32(&mask[2]));
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int adin2111_filter_multicast(const struct device *dev)
{
	const struct adin2111_config *cfg = dev->config;
	bool is_adin2111 = (cfg->id == ADIN2111_MAC);
	uint8_t mm[NET_ETH_ADDR_LEN] = {BIT(0), 0U, 0U, 0U, 0U, 0U};
	uint8_t mmask[NET_ETH_ADDR_LEN] = {0xFFU, 0U, 0U, 0U, 0U, 0U};
	uint32_t rules = ADIN2111_ADDR_APPLY2PORT1 |
			 (is_adin2111 ? ADIN2111_ADDR_APPLY2PORT2 : 0) |
			 ADIN2111_ADDR_TO_HOST |
			 ADIN2111_ADDR_TO_OTHER_PORT;

	return adin2111_write_filter_address(dev, mm, mmask, rules,
					     ADIN2111_MULTICAST_ADDR_SLOT);
}

static int adin2111_filter_broadcast(const struct device *dev)
{
	const struct adin2111_config *cfg = dev->config;
	bool is_adin2111 = (cfg->id == ADIN2111_MAC);
	uint8_t mac[NET_ETH_ADDR_LEN] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
	uint32_t rules = ADIN2111_ADDR_APPLY2PORT1 |
			 (is_adin2111 ? ADIN2111_ADDR_APPLY2PORT2 : 0) |
			 ADIN2111_ADDR_TO_HOST |
			 ADIN2111_ADDR_TO_OTHER_PORT;

	return adin2111_write_filter_address(dev, mac, mac, rules,
					     ADIN2111_BROADCAST_ADDR_SLOT);
}

static int adin2111_filter_unicast(const struct device *dev, uint8_t *addr,
				   const uint16_t port_idx)
{
	uint32_t rules = (port_idx == 0 ? ADIN2111_ADDR_APPLY2PORT1
					: ADIN2111_ADDR_APPLY2PORT2)
			 | ADIN2111_ADDR_TO_HOST;
	uint16_t slot = (port_idx == 0 ? ADIN2111_UNICAST_P1_ADDR_SLOT
				       : ADIN2111_UNICAST_P2_ADDR_SLOT);

	return adin2111_write_filter_address(dev, addr, NULL, rules, slot);
}

int eth_adin2111_broadcast_filter(const struct device *dev, bool enable)
{
	if (!enable) {
		/* Clean up */
		uint8_t mac[NET_ETH_ADDR_LEN] = {0};

		return adin2111_write_filter_address(dev, mac, mac, 0,
						     ADIN2111_BROADCAST_ADDR_SLOT);
	}

	return adin2111_filter_broadcast(dev);
}

/*
 * Check if a filter exists already.
 */
static int eth_adin2111_find_filter(const struct device *dev, uint8_t *mac, const uint16_t port_idx)
{
	int i, offset, reg, ret;

	for (i = ADIN2111_FILTER_FIRST_SLOT; i < ADIN2111_FILTER_SLOTS; i++) {
		offset = i << 1;
		ret = eth_adin2111_reg_read(dev, ADIN2111_ADDR_FILT_UPR + offset, &reg);
		if (ret < 0) {
			return ret;
		}
		if ((reg & UINT16_MAX) == sys_get_be16(&mac[0])) {
			if ((port_idx == 0 && !(reg & ADIN2111_ADDR_APPLY2PORT1)) ||
			    (port_idx == 1 && !(reg & ADIN2111_ADDR_APPLY2PORT2)))
				continue;

			ret = eth_adin2111_reg_read(dev, ADIN2111_ADDR_FILT_LWR + offset, &reg);
			if (ret < 0) {
				return ret;
			}
			if (reg == sys_get_be32(&mac[2])) {
				return i;
			}
		}
	}

	return -ENOENT;
}

static int eth_adin2111_set_mac_filter(const struct device *dev, uint8_t *mac,
				       const uint16_t port_idx)
{
	int i, ret, offset;
	uint32_t reg;

	ret = eth_adin2111_find_filter(dev, mac, port_idx);
	if (ret >= 0) {
		LOG_WRN("MAC filter already set at pos %d, not setting it.", ret);
		return ret;
	}
	if (ret != -ENOENT) {
		return ret;
	}

	for (i = ADIN2111_FILTER_FIRST_SLOT; i < ADIN2111_FILTER_SLOTS; i++) {
		offset = i << 1;
		ret = eth_adin2111_reg_read(dev, ADIN2111_ADDR_FILT_UPR + offset, &reg);
		if (ret < 0) {
			return ret;
		}
		if (reg == 0) {
			uint32_t rules = (port_idx == 0 ? ADIN2111_ADDR_APPLY2PORT1
					: ADIN2111_ADDR_APPLY2PORT2)
					| ADIN2111_ADDR_TO_HOST;

			return adin2111_write_filter_address(dev, mac, NULL, rules, i);
		}
	}

	return -ENOSPC;
}

static int eth_adin2111_clear_mac_filter(const struct device *dev, uint8_t *mac,
					 const uint16_t port_idx)
{
	int i;
	uint8_t cmac[NET_ETH_ADDR_LEN] = {0};

	i = eth_adin2111_find_filter(dev, mac, port_idx);
	if (i < 0) {
		return i;
	}

	return adin2111_write_filter_address(dev, cmac, cmac, 0, i);
}

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
static int eth_adin2111_set_promiscuous(const struct device *dev, const uint16_t port_idx,
					bool enable)
{
	const struct adin2111_config *cfg = dev->config;
	bool is_adin2111 = (cfg->id == ADIN2111_MAC);
	uint32_t fwd_mask;

	if ((!is_adin2111 && port_idx > 0) || (is_adin2111 && port_idx > 1)) {
		return -EINVAL;
	}

	fwd_mask = port_idx ? ADIN2111_CONFIG2_P2_FWD_UNK2HOST : ADIN2111_CONFIG2_P1_FWD_UNK2HOST;

	return eth_adin2111_reg_update(dev, ADIN2111_CONFIG2, fwd_mask, enable ? fwd_mask : 0);
}
#endif

static void adin2111_port_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct adin2111_port_config *cfg = dev->config;
	struct adin2111_port_data *data = dev->data;
	const struct device *adin = cfg->adin;
	struct adin2111_data *ctx = adin->data;
	int ret;

	if (!device_is_ready(adin)) {
		LOG_ERR("ADIN %s is not ready, can't init port %u iface",
			cfg->adin->name, cfg->port_idx);
		return;
	}

	if (!device_is_ready(cfg->phy)) {
		LOG_ERR("PHY %u is not ready, can't init port %u iface",
			cfg->phy_addr, cfg->port_idx);
		return;
	}

	ctx->port[cfg->port_idx] = dev;
	data->iface = iface;

	ret = adin2111_filter_unicast(adin, data->mac_addr, cfg->port_idx);
	if (ret < 0) {
		LOG_ERR("Port %u, failed to set unicast filter, %d",
			cfg->port_idx, ret);
		return;
	}
	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);
	ethernet_init(iface);
	net_if_carrier_off(iface);

	--ctx->ifaces_left_to_init;

	/* if all ports are initialized */
	if (ctx->ifaces_left_to_init == 0U) {
		/* setup rx filters */
		ret = adin2111_filter_multicast(adin);
		if (ret < 0) {
			LOG_ERR("Couldn't set multicast filter, %d", ret);
			return;
		}
		ret = adin2111_filter_broadcast(adin);
		if (ret < 0) {
			LOG_ERR("Couldn't set broadcast filter, %d", ret);
			return;
		}

		/* sync */
		ret = adin2111_config_sync(adin);
		if (ret < 0) {
			LOG_ERR("Failed to write CONFIG0 SYNC, %d", ret);
			return;
		}

		/* all ifaces are done, start INT processing */
		k_thread_create(&ctx->rx_thread, ctx->rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(ctx->rx_thread_stack),
				adin2111_offload_thread,
				(void *)adin, NULL, NULL,
				CONFIG_ETH_ADIN2111_IRQ_THREAD_PRIO,
				K_ESSENTIAL, K_NO_WAIT);
		k_thread_name_set(&ctx->rx_thread, "eth_adin2111_offload");
	}
}

static enum ethernet_hw_caps adin2111_port_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE_T |
		ETHERNET_HW_FILTERING
#if defined(CONFIG_NET_LLDP)
		| ETHERNET_LLDP
#endif
		| ETHERNET_PROMISC_MODE;
}

static int adin2111_port_set_config(const struct device *dev,
				    enum ethernet_config_type type,
				    const struct ethernet_config *config)
{
	const struct adin2111_port_config *cfg = dev->config;
	struct adin2111_port_data *data = dev->data;
	const struct device *adin = cfg->adin;
	int ret = -ENOTSUP;

	(void)eth_adin2111_lock(adin, K_FOREVER);

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		ret = adin2111_filter_unicast(adin, (uint8_t *)&config->mac_address.addr[0],
					      cfg->port_idx);
		if (ret < 0) {
			goto end_unlock;
		}

		(void)memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));

		(void)net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
					   NET_LINK_ETHERNET);
	}

	if (type == ETHERNET_CONFIG_TYPE_FILTER) {
		/* Filtering for DA only */
		if (config->filter.type & ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS) {
			uint8_t *mac = (uint8_t *)config->filter.mac_address.addr;

			if (config->filter.set) {
				ret = eth_adin2111_set_mac_filter(adin, mac, cfg->port_idx);
			} else {
				ret = eth_adin2111_clear_mac_filter(adin, mac, cfg->port_idx);
			}
		}
	}

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	if (type == ETHERNET_CONFIG_TYPE_PROMISC_MODE) {
		ret = eth_adin2111_set_promiscuous(adin, cfg->port_idx, config->promisc_mode);
	}
#endif

end_unlock:
	(void)eth_adin2111_unlock(adin);
	return ret;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *adin2111_port_get_stats(const struct device *dev)
{
	struct adin2111_port_data *data = dev->data;

	return &data->stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static int adin2111_check_spi(const struct device *dev)
{
	uint32_t count;
	uint32_t val;
	int ret;

	/* check SPI communication by reading PHYID */
	for (count = 0U; count < ADIN2111_DEV_AWAIT_RETRY_COUNT; ++count) {
		ret = eth_adin2111_reg_read(dev, ADIN2111_PHYID, &val);
		if (ret >= 0) {
			if (val == ADIN2111_PHYID_RST_VAL || val == ADIN1110_PHYID_RST_VAL) {
				break;
			}
			ret = -ETIMEDOUT;
		}
		k_sleep(K_USEC(ADIN2111_DEV_AWAIT_DELAY_POLL_US));
	}

	return ret;
}

static int adin2111_await_device(const struct device *dev)
{
	uint32_t count;
	uint32_t val;
	int ret;

	/* await reset complete (RESETC) and clear it */
	for (count = 0U; count < ADIN2111_RESETC_AWAIT_RETRY_COUNT; ++count) {
		ret = eth_adin2111_reg_read(dev, ADIN2111_PHYID, &val);
		if (ret >= 0) {
			/*
			 * Even after getting RESETC, for some milliseconds registers are
			 * still not properly readable (they reads 0),
			 * so checking OUI read-only value instead.
			 */
			if ((val >> 10) == ADIN2111_PHYID_OUI) {
				/* clear RESETC */
				ret = eth_adin2111_reg_write(dev, ADIN2111_STATUS0,
							 ADIN2111_STATUS0_RESETC);
				if (ret >= 0) {
					break;
				}
			}
			ret = -ETIMEDOUT;
		}
		k_sleep(K_USEC(ADIN2111_RESETC_AWAIT_DELAY_POLL_US));
	}

	return ret;
}

int eth_adin2111_sw_reset(const struct device *dev, uint16_t delay)
{
	int ret;

	ret = eth_adin2111_reg_write(dev, ADIN2111_RESET, ADIN2111_RESET_SWRESET);
	if (ret < 0) {
		return ret;
	}

	k_msleep(delay);

	ret = adin2111_await_device(dev);
	if (ret < 0) {
		LOG_ERR("ADIN did't come out of the reset, %d", ret);
		return ret;
	}

	return ret;
}

static int adin2111_init(const struct device *dev)
{
	const struct adin2111_config *cfg = dev->config;
	bool is_adin2111 = (cfg->id == ADIN2111_MAC);
	struct adin2111_data *ctx = dev->data;
	int ret;
	uint32_t val;

	__ASSERT(cfg->spi.config.frequency <= ADIN2111_SPI_MAX_FREQUENCY,
		 "SPI frequency exceeds supported maximum\n");

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus %s not ready", cfg->spi.bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->interrupt)) {
		LOG_ERR("Interrupt GPIO device %s is not ready",
			cfg->interrupt.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO, %d", ret);
		return ret;
	}

	if (cfg->reset.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("Reset GPIO device %s is not ready",
			cfg->reset.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO, %d", ret);
			return ret;
		}

		/* perform hard reset */
		/* assert pin low for 16 µs (10 µs min) */
		gpio_pin_set_dt(&cfg->reset, 1);
		k_busy_wait(16U);
		/* deassert and wait for 90 ms (max) for clocks stabilisation */
		gpio_pin_set_dt(&cfg->reset, 0);
		k_msleep(ADIN2111_HW_BOOT_DELAY_MS);
	}

	gpio_init_callback(&(ctx->gpio_int_callback),
			   adin2111_int_callback,
			   BIT(cfg->interrupt.pin));

	ret = gpio_add_callback(cfg->interrupt.port, &ctx->gpio_int_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add INT callback, %d", ret);
		return ret;
	}

	k_msleep(ADIN2111_SPI_ACTIVE_DELAY_MS);

	ret = adin2111_check_spi(dev);
	if (ret < 0) {
		LOG_ERR("Failed to communicate over SPI, %d", ret);
		return ret;
	}

	/* perform MACPHY soft reset */
	ret = eth_adin2111_sw_reset(dev, ADIN2111_SW_RESET_DELAY_MS);
	if (ret < 0) {
		LOG_ERR("MACPHY software reset failed, %d", ret);
		return ret;
	}

	/* CONFIG 0 */
	/* disable Frame Check Sequence validation on the host */
	/* if that is enabled, then CONFIG_ETH_ADIN2111_SPI_CFG0 must be off */
	ret = eth_adin2111_reg_read(dev, ADIN2111_CONFIG0, &val);
	if (ret < 0) {
		LOG_ERR("Failed to read CONFIG0, %d", ret);
		return ret;
	}

	/* RXCTE must be disabled for Generic SPI */
	val &= ~ADIN2111_CONFIG0_RXCTE;
	val &= ~(ADIN2111_CONFIG0_TXCTE | ADIN2111_CONFIG0_TXFCSVE);

	if (ctx->oa) {
		val |= ADIN2111_CONFIG0_ZARFE;
	}

	ret = eth_adin2111_reg_write(dev, ADIN2111_CONFIG0, val);
	if (ret < 0) {
		LOG_ERR("Failed to write CONFIG0, %d", ret);
		return ret;
	}

	/* CONFIG 2 */
	ret = eth_adin2111_reg_read(dev, ADIN2111_CONFIG2, &val);
	if (ret < 0) {
		LOG_ERR("Failed to read CONFIG2, %d", ret);
		return ret;
	}

	val |= ADIN2111_CONFIG2_CRC_APPEND;

	/* configure forwarding of frames with unknown destination address */
	/* to the other port. This forwarding is done in hardware.         */
	/* The setting will take effect after the ports                    */
	/* are out of software powerdown.                                  */
	val |= (ADIN2111_CONFIG2_PORT_CUT_THRU_EN |
		(is_adin2111 ? ADIN2111_CONFIG2_P1_FWD_UNK2P2 : 0) |
		(is_adin2111 ? ADIN2111_CONFIG2_P2_FWD_UNK2P1 : 0));

	ret = eth_adin2111_reg_write(dev, ADIN2111_CONFIG2, val);
	if (ret < 0) {
		LOG_ERR("Failed to write CONFIG2, %d", ret);
		return ret;
	}

	/* configure interrupt masks */
	ctx->imask0 = ~((uint32_t)ADIN2111_IMASK0_PHYINTM);
	ctx->imask1 = ~(ADIN2111_IMASK1_TX_RDY_MASK |
			ADIN2111_IMASK1_P1_RX_RDY_MASK |
			ADIN2111_IMASK1_SPI_ERR_MASK |
			(is_adin2111 ? ADIN2111_IMASK1_P2_RX_RDY_MASK : 0) |
			(is_adin2111 ? ADIN2111_IMASK1_P2_PHYINT_MASK : 0));

	/* enable interrupts */
	ret = eth_adin2111_reg_write(dev, ADIN2111_IMASK0, ctx->imask0);
	if (ret < 0) {
		LOG_ERR("Failed to write IMASK0, %d", ret);
		return ret;
	}
	ret = eth_adin2111_reg_write(dev, ADIN2111_IMASK1, ctx->imask1);
	if (ret < 0) {
		LOG_ERR("Failed to write IMASK1, %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
						GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to enable INT, %d", ret);
		return ret;
	}

	return ret;
}

static const struct ethernet_api adin2111_port_api = {
	.iface_api.init = adin2111_port_iface_init,
	.get_capabilities = adin2111_port_get_capabilities,
	.set_config = adin2111_port_set_config,
	.send = adin2111_port_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = adin2111_port_get_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
};

#define ADIN2111_STR(x)		#x
#define ADIN2111_XSTR(x)	ADIN2111_STR(x)

#define ADIN2111_DEF_BUF(name, size) static uint8_t __aligned(4) name[size]

#define ADIN2111_MDIO_PHY_BY_ADDR(adin_n, phy_addr)						\
	DEVICE_DT_GET(DT_CHILD(DT_INST_CHILD(adin_n, mdio), ethernet_phy_##phy_addr))

#define ADIN2111_PORT_MAC(adin_n, port_n)							\
	DT_PROP(DT_CHILD(DT_DRV_INST(adin_n), port##port_n), local_mac_address)

#define ADIN2111_PORT_DEVICE_INIT_INSTANCE(parent_n, port_n, phy_n, name)			\
	static struct adin2111_port_data name##_port_data_##port_n = {				\
		.mac_addr = ADIN2111_PORT_MAC(parent_n, phy_n),					\
	};											\
	static const struct adin2111_port_config name##_port_config_##port_n = {		\
		.adin = DEVICE_DT_INST_GET(parent_n),						\
		.phy = ADIN2111_MDIO_PHY_BY_ADDR(parent_n, phy_n),				\
		.port_idx = port_n,								\
		.phy_addr = phy_n,								\
	};											\
	ETH_NET_DEVICE_INIT_INSTANCE(name##_port_##port_n, "port_" ADIN2111_XSTR(port_n),	\
				     port_n, NULL, NULL, &name##_port_data_##port_n,		\
				     &name##_port_config_##port_n, CONFIG_ETH_INIT_PRIORITY,	\
				     &adin2111_port_api, NET_ETH_MTU);

#define ADIN2111_SPI_OPERATION ((uint16_t)(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)))
#define ADIN2111_MAC_INITIALIZE(inst, dev_id, ifaces, name)					\
	ADIN2111_DEF_BUF(name##_buffer_##inst, CONFIG_ETH_ADIN2111_BUFFER_SIZE);		\
	COND_CODE_1(DT_INST_PROP(inst, spi_oa),							\
	(											\
		ADIN2111_DEF_BUF(name##_oa_tx_buf_##inst, ADIN2111_OA_BUF_SZ);			\
		ADIN2111_DEF_BUF(name##_oa_rx_buf_##inst, ADIN2111_OA_BUF_SZ);			\
	), ())											\
	static const struct adin2111_config name##_config_##inst = {				\
		.id = dev_id,									\
		.spi = SPI_DT_SPEC_INST_GET(inst, ADIN2111_SPI_OPERATION, 0),			\
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),				\
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, { 0 }),			\
	};											\
	static struct adin2111_data name##_data_##inst = {					\
		.ifaces_left_to_init = ifaces,							\
		.port = {},									\
		.offload_sem = Z_SEM_INITIALIZER(name##_data_##inst.offload_sem, 0, 1),		\
		.lock = Z_MUTEX_INITIALIZER(name##_data_##inst.lock),				\
		.buf = name##_buffer_##inst,							\
		.oa = DT_INST_PROP(inst, spi_oa),						\
		.oa_prot = DT_INST_PROP(inst, spi_oa_protection),				\
		.oa_cps = 64,									\
		.oa_tx_buf = COND_CODE_1(DT_INST_PROP(inst, spi_oa),				\
					 (name##_oa_tx_buf_##inst), (NULL)),			\
		.oa_rx_buf = COND_CODE_1(DT_INST_PROP(inst, spi_oa),				\
					 (name##_oa_rx_buf_##inst), (NULL)),			\
	};											\
	/* adin */										\
	DEVICE_DT_DEFINE(DT_DRV_INST(inst), adin2111_init, NULL,				\
			 &name##_data_##inst, &name##_config_##inst,				\
			 POST_KERNEL, CONFIG_ETH_INIT_PRIORITY,					\
			 NULL);

#define ADIN2111_MAC_INIT(inst)	ADIN2111_MAC_INITIALIZE(inst, ADIN2111_MAC, 2, adin2111)	\
	/* ports */										\
	ADIN2111_PORT_DEVICE_INIT_INSTANCE(inst, 0, 1, adin2111)				\
	ADIN2111_PORT_DEVICE_INIT_INSTANCE(inst, 1, 2, adin2111)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adin2111
DT_INST_FOREACH_STATUS_OKAY(ADIN2111_MAC_INIT)

#define ADIN1110_MAC_INIT(inst)	ADIN2111_MAC_INITIALIZE(inst, ADIN1110_MAC, 1, adin1110)	\
	/* ports */										\
	ADIN2111_PORT_DEVICE_INIT_INSTANCE(inst, 0, 1, adin1110)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adin1110
DT_INST_FOREACH_STATUS_OKAY(ADIN1110_MAC_INIT)
