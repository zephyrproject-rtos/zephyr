/*
 * Copyright (c) 2023 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/mdio.h>
#include <zephyr/net/net_if.h>
#include <ethernet/eth_stats.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include "oa_tc6.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(oa_tc6, CONFIG_ETHERNET_LOG_LEVEL);

/*
 * When IPv6 support enabled - the minimal size of network buffer
 * shall be at least 128 bytes (i.e. default value).
 */
#if defined(CONFIG_NET_IPV6) && (CONFIG_NET_BUF_DATA_SIZE < 128)
#error IPv6 requires at least 128 bytes of continuous data to handle headers!
#endif

int oa_tc6_reg_read(struct oa_tc6 *tc6, const uint32_t reg, uint32_t *val)
{
	uint8_t buf[OA_TC6_HDR_SIZE + 12] = {0};
	struct spi_buf tx_buf = {.buf = buf, .len = sizeof(buf)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf = {.buf = buf, .len = sizeof(buf)};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};
	uint32_t rv, rvn, hdr_bkp, *hdr = (uint32_t *)&buf[0];
	int ret = 0;

	/*
	 * Buffers are allocated for protected (larger) case (by 4 bytes).
	 * When non-protected case - we need to decrase them
	 */
	if (!tc6->protected) {
		tx_buf.len -= sizeof(rvn);
		rx_buf.len -= sizeof(rvn);
	}

	*hdr = FIELD_PREP(OA_CTRL_HDR_DNC, 0) | FIELD_PREP(OA_CTRL_HDR_WNR, 0) |
	       FIELD_PREP(OA_CTRL_HDR_AID, 0) | FIELD_PREP(OA_CTRL_HDR_MMS, reg >> 16) |
	       FIELD_PREP(OA_CTRL_HDR_ADDR, reg) |
	       FIELD_PREP(OA_CTRL_HDR_LEN, 0); /* To read single register len = 0 */
	*hdr |= FIELD_PREP(OA_CTRL_HDR_P, oa_tc6_get_parity(*hdr));
	hdr_bkp = *hdr;
	*hdr = sys_cpu_to_be32(*hdr);

	ret = spi_transceive_dt(tc6->spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	/* Check if echoed control command header is correct */
	rv = sys_be32_to_cpu(*(uint32_t *)&buf[4]);
	if (hdr_bkp != rv) {
		LOG_ERR("Header transmission error!");
		return -1;
	}

	rv = sys_be32_to_cpu(*(uint32_t *)&buf[8]);

	/* In protected mode read data is followed by its compliment value */
	if (tc6->protected) {
		rvn = sys_be32_to_cpu(*(uint32_t *)&buf[12]);
		if (rv != ~rvn) {
			LOG_ERR("Protected mode transmission error!");
			return -1;
		}
	}

	*val = rv;

	return ret;
}

int oa_tc6_reg_write(struct oa_tc6 *tc6, const uint32_t reg, uint32_t val)
{
	uint8_t buf_tx[OA_TC6_HDR_SIZE + 12] = {0};
	uint8_t buf_rx[OA_TC6_HDR_SIZE + 12] = {0};
	struct spi_buf tx_buf = {.buf = buf_tx, .len = sizeof(buf_tx)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf = {.buf = buf_rx, .len = sizeof(buf_rx)};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};
	uint32_t rv, rvn, hdr_bkp, *hdr = (uint32_t *)&buf_tx[0];
	int ret;

	/*
	 * Buffers are allocated for protected (larger) case (by 4 bytes).
	 * When non-protected case - we need to decrase them
	 */
	if (!tc6->protected) {
		tx_buf.len -= sizeof(rvn);
		rx_buf.len -= sizeof(rvn);
	}

	*hdr = FIELD_PREP(OA_CTRL_HDR_DNC, 0) | FIELD_PREP(OA_CTRL_HDR_WNR, 1) |
	       FIELD_PREP(OA_CTRL_HDR_AID, 0) | FIELD_PREP(OA_CTRL_HDR_MMS, reg >> 16) |
	       FIELD_PREP(OA_CTRL_HDR_ADDR, reg) |
	       FIELD_PREP(OA_CTRL_HDR_LEN, 0); /* To read single register len = 0 */
	*hdr |= FIELD_PREP(OA_CTRL_HDR_P, oa_tc6_get_parity(*hdr));
	hdr_bkp = *hdr;
	*hdr = sys_cpu_to_be32(*hdr);

	*(uint32_t *)&buf_tx[4] = sys_cpu_to_be32(val);
	if (tc6->protected) {
		*(uint32_t *)&buf_tx[8] = sys_be32_to_cpu(~val);
	}

	ret = spi_transceive_dt(tc6->spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	/* Check if echoed control command header is correct */
	rv = sys_be32_to_cpu(*(uint32_t *)&buf_rx[4]);
	if (hdr_bkp != rv) {
		LOG_ERR("Header transmission error!");
		return -1;
	}

	/* Check if echoed value is correct */
	rv = sys_be32_to_cpu(*(uint32_t *)&buf_rx[8]);
	if (val != rv) {
		LOG_ERR("Header transmission error!");
		return -1;
	}

	/*
	 * In protected mode check if read value is followed by its
	 * compliment value
	 */
	if (tc6->protected) {
		rvn = sys_be32_to_cpu(*(uint32_t *)&buf_rx[12]);
		if (val != ~rvn) {
			LOG_ERR("Protected mode transmission error!");
			return -1;
		}
	}

	return ret;
}

int oa_tc6_enable_sync(struct oa_tc6 *tc6)
{
	uint32_t val;
	int ret;

	ret = oa_tc6_reg_read(tc6, OA_CONFIG0, &val);
	if (ret) {
		return ret;
	}
	val |= OA_CONFIG0_SYNC;
	return oa_tc6_reg_write(tc6, OA_CONFIG0, val);
}

int oa_tc6_zero_align_receive_frame_enable(struct oa_tc6 *tc6)
{
	uint32_t val;
	int ret;

	ret = oa_tc6_reg_read(tc6, OA_CONFIG0, &val);
	if (ret) {
		return ret;
	}
	val |= OA_CONFIG0_RFA_ZARFE;
	return oa_tc6_reg_write(tc6, OA_CONFIG0, val);
}

int oa_tc6_reg_rmw(struct oa_tc6 *tc6, const uint32_t reg, uint32_t mask, uint32_t val)
{
	uint32_t tmp;
	int ret;

	ret = oa_tc6_reg_read(tc6, reg, &tmp);
	if (ret < 0) {
		return ret;
	}

	tmp &= ~mask;

	if (val) {
		tmp |= val;
	}

	return oa_tc6_reg_write(tc6, reg, tmp);
}

int oa_tc6_mdio_read(struct oa_tc6 *tc6, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	return oa_tc6_reg_read(
		tc6, OA_TC6_PHY_STD_REG_ADDR_BASE | (regad & OA_TC6_PHY_STD_REG_ADDR_MASK),
		(uint32_t *)data);
}

int oa_tc6_mdio_write(struct oa_tc6 *tc6, uint8_t prtad, uint8_t regad, uint16_t data)
{
	return oa_tc6_reg_write(
		tc6, OA_TC6_PHY_STD_REG_ADDR_BASE | (regad & OA_TC6_PHY_STD_REG_ADDR_MASK), data);
}

static int oa_tc6_get_phy_c45_mms(int devad)
{
	switch (devad) {
	case MDIO_MMD_PCS:
		return OA_TC6_PHY_C45_PCS_MMS2;
	case MDIO_MMD_PMAPMD:
		return OA_TC6_PHY_C45_PMA_PMD_MMS3;
	case MDIO_MMD_VENDOR_SPECIFIC2:
		return OA_TC6_PHY_C45_VS_PLCA_MMS4;
	case MDIO_MMD_AN:
		return OA_TC6_PHY_C45_AUTO_NEG_MMS5;
	default:
		return -EOPNOTSUPP;
	}
}

int oa_tc6_mdio_read_c45(struct oa_tc6 *tc6, uint8_t prtad, uint8_t devad, uint16_t regad,
			 uint16_t *data)
{
	uint32_t tmp;
	int ret;

	ret = oa_tc6_get_phy_c45_mms(devad);
	if (ret < 0) {
		return ret;
	}

	ret = oa_tc6_reg_read(tc6, (ret << 16) | regad, &tmp);
	if (ret < 0) {
		return ret;
	}

	*data = (uint16_t)tmp;

	return 0;
}

int oa_tc6_mdio_write_c45(struct oa_tc6 *tc6, uint8_t prtad, uint8_t devad, uint16_t regad,
			  uint16_t data)
{
	int ret;

	ret = oa_tc6_get_phy_c45_mms(devad);
	if (ret < 0) {
		return ret;
	}

	return oa_tc6_reg_write(tc6, (ret << 16) | regad, (uint32_t)data);
}

int oa_tc6_set_protected_ctrl(struct oa_tc6 *tc6, bool prote)
{
	int ret;

	ret = oa_tc6_reg_rmw(tc6, OA_CONFIG0, OA_CONFIG0_PROTE, prote ? OA_CONFIG0_PROTE : 0);
	if (ret < 0) {
		return ret;
	}

	tc6->protected = prote;
	return 0;
}

static int oa_tc6_submit_rx_pkt(struct oa_tc6 *tc6)
{
	int ret;

	if (tc6->buf_rx) {
		net_pkt_append_buffer(tc6->rx_pkt, tc6->buf_rx);
	}

	ret = net_recv_data(tc6->iface, tc6->rx_pkt);
	if (ret < 0) {
		eth_stats_update_errors_rx(tc6->iface);
		net_pkt_unref(tc6->rx_pkt);
		tc6->rx_pkt = NULL;
		tc6->buf_rx = NULL;
		tc6->buf_rx_used = 0;
		return ret;
	}
	eth_stats_update_pkts_rx(tc6->iface);
	tc6->rx_pkt = NULL;
	tc6->buf_rx = NULL;
	tc6->buf_rx_used = 0;

	return 0;
}

static int oa_tc6_update_rx_pkt(struct oa_tc6 *tc6, uint8_t *payload, uint8_t length)
{
	const uint16_t buf_rx_size = CONFIG_NET_BUF_DATA_SIZE;
	uint8_t *data;

	/* Append the buffer into tc6->rx_pkt */
	if ((buf_rx_size - tc6->buf_rx_used) < length) {
		net_pkt_append_buffer(tc6->rx_pkt, tc6->buf_rx);
		tc6->buf_rx = NULL;
		tc6->buf_rx_used = 0;
	}

	if (!tc6->buf_rx) {
		tc6->buf_rx = net_pkt_get_frag(tc6->rx_pkt, buf_rx_size, K_MSEC(10));
		if (!tc6->buf_rx) {
			LOG_ERR("RX buffer allocation failed!");
			net_pkt_unref(tc6->rx_pkt);
			tc6->rx_pkt = NULL;
			return -ENOMEM;
		}
	}

	data = net_buf_add(tc6->buf_rx, length);
	if (!data) {
		LOG_ERR("Failed to allocate RX buffer for data");
		net_pkt_unref(tc6->rx_pkt);
		tc6->rx_pkt = NULL;
		tc6->buf_rx = NULL;
		tc6->buf_rx_used = 0;
		return -ENOMEM;
	}
	memcpy(data, payload, length);
	tc6->buf_rx_used += length;
	tc6->buf_rx->len = tc6->buf_rx_used;

	eth_stats_update_bytes_rx(tc6->iface, length);

	return 0;
}

static int oa_tc6_allocate_rx_pkt(struct oa_tc6 *tc6)
{
	tc6->rx_pkt = net_pkt_rx_alloc_on_iface(tc6->iface, K_MSEC(100));

	/* Free the previously allocated network buffer */
	if (tc6->buf_rx) {
		net_buf_unref(tc6->buf_rx);
		tc6->buf_rx = NULL;
		tc6->buf_rx_used = 0;
	}

	if (!tc6->rx_pkt) {
		eth_stats_update_errors_rx(tc6->iface);
		return -ENOMEM;
	}

	return 0;
}

static int oa_tc6_prcs_complete_rx_frame(struct oa_tc6 *tc6, uint8_t *payload, uint16_t size)
{
	int ret;

	ret = oa_tc6_allocate_rx_pkt(tc6);
	if (ret) {
		return ret;
	}

	ret = oa_tc6_update_rx_pkt(tc6, payload, size);
	if (ret) {
		return ret;
	}

	return oa_tc6_submit_rx_pkt(tc6);
}

static int oa_tc6_prcs_rx_frame_start(struct oa_tc6 *tc6, uint8_t *payload, uint16_t size)
{
	int ret;

	ret = oa_tc6_allocate_rx_pkt(tc6);
	if (ret) {
		return ret;
	}

	return oa_tc6_update_rx_pkt(tc6, payload, size);
}

static int oa_tc6_prcs_rx_frame_end(struct oa_tc6 *tc6, uint8_t *payload, uint16_t size)
{
	int ret;

	ret = oa_tc6_update_rx_pkt(tc6, payload, size);
	if (ret) {
		return ret;
	}

	return oa_tc6_submit_rx_pkt(tc6);
}

static int oa_tc6_prcs_ongoing_rx_frame(struct oa_tc6 *tc6, uint8_t *payload)
{
	return oa_tc6_update_rx_pkt(tc6, payload, tc6->cps);
}

static int oa_tc6_prcs_rx_chunk_payload(struct oa_tc6 *tc6, uint8_t *payload, uint32_t ftr)
{
	uint8_t start_byte_offset = FIELD_GET(OA_DATA_FTR_SWO, ftr) * sizeof(uint32_t);
	uint8_t end_byte_offset = FIELD_GET(OA_DATA_FTR_EBO, ftr);
	bool start_valid = FIELD_GET(OA_DATA_FTR_SV, ftr);
	bool end_valid = FIELD_GET(OA_DATA_FTR_EV, ftr);
	uint16_t size;
	int ret;

	/* Restart the new rx frame after receiving rx buffer overflow error */
	if (start_valid && tc6->rx_buf_overflow) {
		tc6->rx_buf_overflow = false;
	}

	if (tc6->rx_buf_overflow) {
		return 0;
	}

	/* Process the chunk with complete rx frame */
	if (start_valid && end_valid && start_byte_offset < end_byte_offset) {
		size = (end_byte_offset + 1) - start_byte_offset;
		return oa_tc6_prcs_complete_rx_frame(tc6, &payload[start_byte_offset], size);
	}

	/* Process the chunk with only rx frame start */
	if (start_valid && !end_valid) {
		size = tc6->cps - start_byte_offset;
		return oa_tc6_prcs_rx_frame_start(tc6, &payload[start_byte_offset], size);
	}

	/* Process the chunk with only rx frame end */
	if (end_valid && !start_valid) {
		size = end_byte_offset + 1;
		return oa_tc6_prcs_rx_frame_end(tc6, payload, size);
	}

	/* Process the chunk with previous rx frame end and next rx frame start */
	if (start_valid && end_valid && start_byte_offset > end_byte_offset) {
		/* After rx buffer overflow error received, there might be a
		 * possibility of getting an end valid of a previously
		 * incomplete rx frame along with the new rx frame start valid.
		 */
		if (tc6->rx_pkt) {
			size = end_byte_offset + 1;
			ret = oa_tc6_prcs_rx_frame_end(tc6, payload, size);
			if (ret) {
				return ret;
			}
		}
		size = tc6->cps - start_byte_offset;
		return oa_tc6_prcs_rx_frame_start(tc6, &payload[start_byte_offset], size);
	}

	/* Process the chunk with ongoing rx frame data */
	return oa_tc6_prcs_ongoing_rx_frame(tc6, payload);
}

static void oa_tc6_update_last_ftr_info(struct oa_tc6 *tc6)
{
	uint16_t rx_chunks = tc6->spi_length / tc6->chunk_size;
	uint16_t rx_offset = 0;
	uint32_t *ftr_pos;
	uint32_t ftr;

	rx_offset = ((rx_chunks - 1) * tc6->chunk_size) + tc6->cps;
	ftr_pos = (uint32_t *)(tc6->spi_rx_buf + rx_offset);
	ftr = *ftr_pos;
	ftr = sys_be32_to_cpu(ftr);

	tc6->rca = FIELD_GET(OA_DATA_FTR_RCA, ftr);
	tc6->txc = FIELD_GET(OA_DATA_FTR_TXC, ftr);
	tc6->exst = FIELD_GET(OA_DATA_FTR_EXST, ftr);
	tc6->sync = FIELD_GET(OA_DATA_FTR_SYNC, ftr);
}

static int oa_tc6_process_spi_rx_buf(struct oa_tc6 *tc6)
{
	uint16_t rx_chunks = tc6->spi_length / tc6->chunk_size;
	uint16_t rx_offset = 0;
	uint32_t *ftr_pos;
	uint32_t ftr;
	uint8_t i;
	int ret;

	for (i = 0; i < rx_chunks; i++) {
		rx_offset = (i * tc6->chunk_size) + tc6->cps;
		ftr_pos = (uint32_t *)(tc6->spi_rx_buf + rx_offset);
		ftr = *ftr_pos;
		ftr = sys_be32_to_cpu(ftr);

		tc6->rca = FIELD_GET(OA_DATA_FTR_RCA, ftr);
		tc6->txc = FIELD_GET(OA_DATA_FTR_TXC, ftr);
		tc6->exst = FIELD_GET(OA_DATA_FTR_EXST, ftr);
		tc6->sync = FIELD_GET(OA_DATA_FTR_SYNC, ftr);

		ret = oa_tc6_check_status(tc6);
		if (ret) {
			oa_tc6_update_last_ftr_info(tc6);
			return ret;
		}

		if (FIELD_GET(OA_DATA_FTR_DV, ftr)) {
			uint8_t *payload = tc6->spi_rx_buf + (rx_offset - tc6->cps);

			ret = oa_tc6_prcs_rx_chunk_payload(tc6, payload, ftr);
			if (ret) {
				return ret;
			}
		}
	}

	return 0;
}

static int oa_tc6_perform_spi(struct oa_tc6 *tc6)
{
	struct spi_buf spi_tx_buf;
	struct spi_buf spi_rx_buf;
	struct spi_buf_set tx;
	struct spi_buf_set rx;

	spi_tx_buf.buf = tc6->spi_tx_buf;
	spi_tx_buf.len = tc6->spi_length;
	tx.buffers = &spi_tx_buf;
	tx.count = 1;

	spi_rx_buf.buf = tc6->spi_rx_buf;
	spi_rx_buf.len = tc6->spi_length;
	rx.buffers = &spi_rx_buf;
	rx.count = 1;

	return spi_transceive_dt(tc6->spi, &tx, &rx);
}

static uint16_t oa_tc6_prepare_spi_tx_buf_from_net_pkt(struct oa_tc6 *tc6)
{
	uint16_t tx_offset = 0;
	uint8_t no_of_chunks;
	uint32_t *hdr_pos;
	uint8_t used_txc;
	uint16_t tx_len;
	uint32_t hdr;
	int ret;

	for (used_txc = 0; used_txc < tc6->txc; used_txc++) {
		hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1) | FIELD_PREP(OA_DATA_HDR_DV, 1) |
		      FIELD_PREP(OA_DATA_HDR_SWO, 0);

		if (!tc6->ongoing_net_pkt) {
			tc6->ongoing_net_pkt = tc6->waiting_net_pkt;
			tc6->waiting_net_pkt = NULL;
			k_sem_give(&tc6->tx_enq_sem);
			net_pkt_cursor_init(tc6->ongoing_net_pkt);
			tc6->tx_eth_len = net_pkt_get_len(tc6->ongoing_net_pkt);
			hdr |= FIELD_PREP(OA_DATA_HDR_SV, 1);
		}

		no_of_chunks = tc6->tx_eth_len / tc6->cps;
		if (tc6->tx_eth_len % tc6->cps) {
			no_of_chunks++;
		}

		if (no_of_chunks == 1) {
			hdr |= FIELD_PREP(OA_DATA_HDR_EBO, tc6->tx_eth_len - 1) |
			       FIELD_PREP(OA_DATA_HDR_EV, 1);
			tc6->tx_eth_frame_end = true;
		}

		hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(hdr));
		hdr = sys_cpu_to_be32(hdr);
		hdr_pos = (uint32_t *)(tc6->spi_tx_buf + tx_offset);
		*hdr_pos = hdr;
		tx_offset += OA_TC6_HDR_SIZE;

		tx_len = MIN(tc6->tx_eth_len, tc6->cps);
		ret = net_pkt_read(tc6->ongoing_net_pkt, &tc6->spi_tx_buf[tx_offset], tx_len);
		if (ret < 0) {
			eth_stats_update_errors_tx(tc6->iface);
			net_pkt_unref(tc6->ongoing_net_pkt);
			tc6->ongoing_net_pkt = NULL;
			return 0;
		}
		tx_offset += tc6->cps;
		tc6->tx_eth_len -= tx_len;
		eth_stats_update_bytes_tx(tc6->iface, tx_len);
		if (tc6->tx_eth_frame_end) {
			net_pkt_unref(tc6->ongoing_net_pkt);
			tc6->ongoing_net_pkt = NULL;
			break;
		}
		if (tx_offset >= OA_SPI_TX_RX_BUFFER_SIZE) {
			return tx_offset;
		}
	}
	return tx_offset;
}

static void oa_tc6_add_tx_empty_chunks(struct oa_tc6 *tc6, uint8_t empty_chunks)
{
	uint32_t *spi_tx_buf;
	uint32_t hdr;

	hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1) | FIELD_PREP(OA_DATA_HDR_DV, 0);
	hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(hdr));
	hdr = sys_cpu_to_be32(hdr);

	while (empty_chunks--) {
		if (tc6->spi_length > OA_SPI_TX_RX_BUFFER_SIZE) {
			tc6->spi_length = OA_SPI_TX_RX_BUFFER_SIZE;
			break;
		} else if (tc6->spi_length == OA_SPI_TX_RX_BUFFER_SIZE) {
			break;
		}

		spi_tx_buf = (uint32_t *)(tc6->spi_tx_buf + tc6->spi_length);
		*spi_tx_buf = hdr;
		tc6->spi_length += tc6->chunk_size;
	}
}

int oa_tc6_spi_thread(struct oa_tc6 *tc6)
{
	uint8_t needed_tx_empty_chunks;
	uint8_t tx_chunks;
	int ret;

	k_sem_take(&tc6->spi_sem, K_FOREVER);

	if (tc6->waiting_net_pkt || tc6->ongoing_net_pkt) {
		tc6->spi_length = oa_tc6_prepare_spi_tx_buf_from_net_pkt(tc6);
	}

	tx_chunks = tc6->spi_length / tc6->chunk_size;
	if (tx_chunks < tc6->rca) {
		needed_tx_empty_chunks = tc6->rca - tx_chunks;
		oa_tc6_add_tx_empty_chunks(tc6, needed_tx_empty_chunks);
	}

	if (tc6->int_flag) {
		tc6->int_flag = false;
		if (tc6->spi_length == 0) {
			oa_tc6_add_tx_empty_chunks(tc6, 1);
		}
	}

	if (tc6->spi_length == 0) {
		return 0;
	}

	ret = oa_tc6_perform_spi(tc6);
	if (ret) {
		LOG_ERR("SPI transfer failed: %d\n", ret);
		return ret;
	}

	ret = oa_tc6_process_spi_rx_buf(tc6);
	if (ret) {
		if (ret == -EAGAIN) {
			if (tc6->rca) {
				k_sem_give(&tc6->spi_sem);
			}
			return 0;
		}
		LOG_ERR("Device error: %d\n", ret);
		return ret;
	}

	if (tc6->tx_eth_frame_end) {
		eth_stats_update_pkts_tx(tc6->iface);
		tc6->tx_eth_frame_end = false;
	}
	tc6->spi_length = 0;

	if ((tc6->ongoing_net_pkt && tc6->txc) || (tc6->waiting_net_pkt && tc6->txc) ||
	    (tc6->rca > 0)) {
		k_sem_give(&tc6->spi_sem);
	}

	return 0;
}

int oa_tc6_send_chunks(struct oa_tc6 *tc6, struct net_pkt *pkt)
{
	net_pkt_ref(pkt);

	tc6->waiting_net_pkt = pkt;

	k_sem_give(&tc6->spi_sem);

	k_sem_take(&tc6->tx_enq_sem, K_FOREVER);

	return 0;
}

int oa_tc6_check_status(struct oa_tc6 *tc6)
{
	uint32_t sts;

	if (!tc6->sync) {
		LOG_ERR("SYNC: Configuration lost, reset IC!");
		return -EIO;
	}

	if (tc6->exst) {
		/*
		 * Just clear any pending interrupts.
		 * The RESETC is handled separately as it requires per
		 * device configuration.
		 */
		if (oa_tc6_reg_read(tc6, OA_STATUS0, &sts) < 0) {
			return -EIO;
		}

		if (sts != 0) {
			oa_tc6_reg_write(tc6, OA_STATUS0, sts);
			if (FIELD_GET(OA_STATUS0_RX_BUFFER_OVERFLOW, sts)) {
				tc6->rx_buf_overflow = true;
				if (tc6->rx_pkt) {
					net_pkt_unref(tc6->rx_pkt);
					tc6->rx_pkt = NULL;
				}

				/* Free the buffer that is not added into the tc6->rx_pkt */
				if (tc6->buf_rx) {
					net_buf_unref(tc6->buf_rx);
					tc6->buf_rx = NULL;
					tc6->buf_rx_used = 0;
				}
				eth_stats_update_errors_rx(tc6->iface);
				return -EAGAIN;
			}
		}
		if (oa_tc6_reg_read(tc6, OA_STATUS1, &sts) < 0) {
			return -EIO;
		}
		if (sts != 0) {
			oa_tc6_reg_write(tc6, OA_STATUS1, sts);
			eth_stats_update_errors_rx(tc6->iface);
		}
	}

	return 0;
}

static int oa_tc6_unmask_macphy_error_interrupts(struct oa_tc6 *tc6)
{
	uint32_t regval;
	int ret;

	ret = oa_tc6_reg_read(tc6, OA_IMASK0, &regval);
	if (ret) {
		return ret;
	}

	WRITE_BIT(regval, OA_IMASK0_RXBOEM, 0);

	return oa_tc6_reg_write(tc6, OA_IMASK0, regval);
}

static int oa_tc6_wait_for_reset(struct oa_tc6 *tc6)
{
	uint8_t i;
	int ret;

	/* Wait for end of MAC-PHY reset */
	for (i = 0; !tc6->rst_flag && i < OA_TC6_RESET_TIMEOUT; i++) {
		k_msleep(1);
	}

	if (i == OA_TC6_RESET_TIMEOUT) {
		LOG_ERR("MAC-PHY reset timeout reached!");
		return -ENODEV;
	}

	ret = oa_tc6_unmask_macphy_error_interrupts(tc6);
	if (ret) {
		LOG_ERR("MAC-PHY error interrupts unmask failed, %d", ret);
		return ret;
	}

	return 0;
}

static int oa_tc6_gpio_reset(struct oa_tc6 *tc6)
{
	tc6->rst_flag = false;
	tc6->protected = false;

	/* Perform (GPIO based) HW reset */
	/* assert RESET_N low for 10 µs (5 µs min) */
	gpio_pin_set_dt(tc6->reset, 1);
	k_busy_wait(10U);
	/* deassert - end of reset indicated by IRQ_N low  */
	gpio_pin_set_dt(tc6->reset, 0);

	return oa_tc6_wait_for_reset(tc6);
}

static void oa_tc6_int_thread(struct oa_tc6 *tc6)
{
	uint32_t sts;

	while (true) {
		if (!tc6->rst_flag) {
			k_sem_take(&tc6->int_sem, K_FOREVER);
			oa_tc6_reg_read(tc6, OA_STATUS0, &sts);
			if (sts & OA_STATUS0_RESETC) {
				oa_tc6_reg_write(tc6, OA_STATUS0, sts);

				tc6->rst_flag = true;
			}
		}

		else {
			if (oa_tc6_spi_thread(tc6)) {
				LOG_ERR("Non recoverable error\n");
				break;
			}
		}
	}
}

static void oa_tc6_int_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct oa_tc6 *tc6 = CONTAINER_OF(cb, struct oa_tc6, gpio_int_callback);

	if (!tc6->rst_flag) {
		k_sem_give(&tc6->int_sem);
	} else {
		tc6->int_flag = true;
		k_sem_give(&tc6->spi_sem);
	}
}

int oa_tc6_init(struct oa_tc6 *tc6)
{
	int ret;

	tc6->chunk_size = tc6->cps + OA_TC6_HDR_SIZE;

	k_sem_init(&tc6->spi_sem, 0, 1);
	k_sem_init(&tc6->tx_enq_sem, 0, 1);

	tc6->waiting_net_pkt = NULL;
	tc6->ongoing_net_pkt = NULL;

	if (!spi_is_ready_dt(tc6->spi)) {
		LOG_ERR("SPI bus %s not ready", tc6->spi->bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(tc6->interrupt)) {
		LOG_ERR("Interrupt GPIO device %s is not ready", tc6->interrupt->port->name);
		return -ENODEV;
	}

	/*
	 * Configure interrupt service routine for MAC-PHY IRQ
	 */
	ret = gpio_pin_configure_dt(tc6->interrupt, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO, %d", ret);
		return ret;
	}

	gpio_init_callback(&(tc6->gpio_int_callback), oa_tc6_int_callback,
			   BIT(tc6->interrupt->pin));

	ret = gpio_add_callback(tc6->interrupt->port, &tc6->gpio_int_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add INT callback, %d", ret);
		return ret;
	}

	gpio_pin_interrupt_configure_dt(tc6->interrupt, GPIO_INT_EDGE_TO_ACTIVE);

	/* Start interruption-poll thread */
	tc6->tid_int = k_thread_create(&tc6->thread, tc6->thread_stack,
				       CONFIG_OA_TC6_IRQ_THREAD_STACK_SIZE,
				       (k_thread_entry_t)oa_tc6_int_thread, (void *)tc6, NULL, NULL,
				       K_PRIO_COOP(CONFIG_OA_TC6_IRQ_THREAD_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(tc6->tid_int, "oa_tc6_interrupt");

	/* Perform HW reset - 'rst-gpios' required property set in DT */
	if (!gpio_is_ready_dt(tc6->reset)) {
		LOG_ERR("Reset GPIO device %s is not ready", tc6->reset->port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(tc6->reset, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure reset GPIO, %d", ret);
		return ret;
	}

	return oa_tc6_gpio_reset(tc6);
}
