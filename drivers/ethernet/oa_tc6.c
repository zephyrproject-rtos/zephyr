/*
 * Copyright (c) 2023 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/mdio.h>
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

int oa_tc6_send_chunks(struct oa_tc6 *tc6, struct net_pkt *pkt)
{
	uint16_t len = net_pkt_get_len(pkt);
	uint8_t oa_tx[tc6->cps];
	uint32_t hdr, ftr;
	uint8_t chunks, i;
	int ret;

	if (len == 0) {
		return -ENODATA;
	}

	chunks = len / tc6->cps;
	if (len % tc6->cps) {
		chunks++;
	}

	/* Check if LAN865x has any free internal buffer space */
	if (chunks > tc6->txc) {
		return -EIO;
	}

	/* Transform struct net_pkt content into chunks */
	for (i = 1; i <= chunks; i++) {
		hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1) | FIELD_PREP(OA_DATA_HDR_DV, 1) |
		      FIELD_PREP(OA_DATA_HDR_NORX, 1) | FIELD_PREP(OA_DATA_HDR_SWO, 0);

		if (i == 1) {
			hdr |= FIELD_PREP(OA_DATA_HDR_SV, 1);
		}

		if (i == chunks) {
			hdr |= FIELD_PREP(OA_DATA_HDR_EBO, len - 1) | FIELD_PREP(OA_DATA_HDR_EV, 1);
		}

		hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(hdr));

		ret = net_pkt_read(pkt, oa_tx, len > tc6->cps ? tc6->cps : len);
		if (ret < 0) {
			return ret;
		}

		ret = oa_tc6_chunk_spi_transfer(tc6, NULL, oa_tx, hdr, &ftr);
		if (ret < 0) {
			return ret;
		}

		len -= tc6->cps;
	}

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
		oa_tc6_reg_read(tc6, OA_STATUS0, &sts);
		if (sts != 0) {
			oa_tc6_reg_write(tc6, OA_STATUS0, sts);
			LOG_WRN("EXST: OA_STATUS0: 0x%x", sts);
		}

		oa_tc6_reg_read(tc6, OA_STATUS1, &sts);
		if (sts != 0) {
			oa_tc6_reg_write(tc6, OA_STATUS1, sts);
			LOG_WRN("EXST: OA_STATUS1: 0x%x", sts);
		}
	}

	return 0;
}

static int oa_tc6_update_status(struct oa_tc6 *tc6, uint32_t ftr)
{
	if (oa_tc6_get_parity(ftr)) {
		LOG_DBG("OA Status Update: Footer parity error!");
		return -EIO;
	}

	tc6->exst = FIELD_GET(OA_DATA_FTR_EXST, ftr);
	tc6->sync = FIELD_GET(OA_DATA_FTR_SYNC, ftr);
	tc6->rca = FIELD_GET(OA_DATA_FTR_RCA, ftr);
	tc6->txc = FIELD_GET(OA_DATA_FTR_TXC, ftr);

	return 0;
}

int oa_tc6_chunk_spi_transfer(struct oa_tc6 *tc6, uint8_t *buf_rx, uint8_t *buf_tx, uint32_t hdr,
			      uint32_t *ftr)
{
	struct spi_buf tx_buf[2];
	struct spi_buf rx_buf[2];
	struct spi_buf_set tx;
	struct spi_buf_set rx;
	int ret;

	hdr = sys_cpu_to_be32(hdr);
	tx_buf[0].buf = &hdr;
	tx_buf[0].len = sizeof(hdr);

	tx_buf[1].buf = buf_tx;
	tx_buf[1].len = tc6->cps;

	tx.buffers = tx_buf;
	tx.count = ARRAY_SIZE(tx_buf);

	rx_buf[0].buf = buf_rx;
	rx_buf[0].len = tc6->cps;

	rx_buf[1].buf = ftr;
	rx_buf[1].len = sizeof(*ftr);

	rx.buffers = rx_buf;
	rx.count = ARRAY_SIZE(rx_buf);

	ret = spi_transceive_dt(tc6->spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}
	*ftr = sys_be32_to_cpu(*ftr);

	return oa_tc6_update_status(tc6, *ftr);
}

int oa_tc6_read_status(struct oa_tc6 *tc6, uint32_t *ftr)
{
	uint32_t hdr;

	hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1) | FIELD_PREP(OA_DATA_HDR_DV, 0) |
	      FIELD_PREP(OA_DATA_HDR_NORX, 1);
	hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(hdr));

	return oa_tc6_chunk_spi_transfer(tc6, NULL, NULL, hdr, ftr);
}

int oa_tc6_read_chunks(struct oa_tc6 *tc6, struct net_pkt *pkt)
{
	const uint16_t buf_rx_size = CONFIG_NET_BUF_DATA_SIZE;
	struct net_buf *buf_rx = NULL;
	uint32_t buf_rx_used = 0;
	uint32_t hdr, ftr;
	uint8_t sbo, ebo;
	int ret;

	/*
	 * Special case - append already received data (extracted from previous
	 * chunk) to new packet.
	 *
	 * This code is NOT used when OA_CONFIG0 RFA [13:12] is set to 01
	 * (ZAREFE) - so received ethernet frames will always start on the
	 * beginning of new chunks.
	 */
	if (tc6->concat_buf) {
		net_pkt_append_buffer(pkt, tc6->concat_buf);
		tc6->concat_buf = NULL;
	}

	do {
		if (!buf_rx) {
			buf_rx = net_pkt_get_frag(pkt, buf_rx_size, OA_TC6_BUF_ALLOC_TIMEOUT);
			if (!buf_rx) {
				LOG_ERR("OA RX: Can't allocate RX buffer fordata!");
				return -ENOMEM;
			}
		}

		hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1);
		hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(hdr));

		ret = oa_tc6_chunk_spi_transfer(tc6, buf_rx->data + buf_rx_used, NULL, hdr, &ftr);
		if (ret < 0) {
			LOG_ERR("OA RX: transmission error: %d!", ret);
			goto unref_buf;
		}

		ret = -EIO;
		if (oa_tc6_get_parity(ftr)) {
			LOG_ERR("OA RX: Footer parity error!");
			goto unref_buf;
		}

		if (!FIELD_GET(OA_DATA_FTR_SYNC, ftr)) {
			LOG_ERR("OA RX: Configuration not SYNC'ed!");
			goto unref_buf;
		}

		if (!FIELD_GET(OA_DATA_FTR_DV, ftr)) {
			LOG_DBG("OA RX: Data chunk not valid, skip!");
			goto unref_buf;
		}

		sbo = FIELD_GET(OA_DATA_FTR_SWO, ftr) * sizeof(uint32_t);
		ebo = FIELD_GET(OA_DATA_FTR_EBO, ftr) + 1;

		if (FIELD_GET(OA_DATA_FTR_SV, ftr)) {
			/*
			 * Adjust beginning of the buffer with SWO only when
			 * we DO NOT have two frames concatenated together
			 * in one chunk.
			 */
			if (!(FIELD_GET(OA_DATA_FTR_EV, ftr) && (ebo <= sbo))) {
				if (sbo) {
					net_buf_pull(buf_rx, sbo);
				}
			}
		}

		if (FIELD_GET(OA_DATA_FTR_EV, ftr)) {
			/*
			 * Check if received frame shall be dropped - i.e. MAC has
			 * detected error condition, which shall result in frame drop
			 * by the SPI host.
			 */
			if (FIELD_GET(OA_DATA_FTR_FD, ftr)) {
				ret = -EIO;
				goto unref_buf;
			}

			/*
			 * Concatenation of frames in a single chunk - one frame ends
			 * and second one starts just afterwards (ebo == sbo).
			 */
			if (FIELD_GET(OA_DATA_FTR_SV, ftr) && (ebo <= sbo)) {
				tc6->concat_buf = net_buf_clone(buf_rx, OA_TC6_BUF_ALLOC_TIMEOUT);
				if (!tc6->concat_buf) {
					LOG_ERR("OA RX: Can't allocate RX buffer for data!");
					ret = -ENOMEM;
					goto unref_buf;
				}
				net_buf_pull(tc6->concat_buf, sbo);
			}

			/* Set final size of the buffer */
			buf_rx_used += ebo;
			buf_rx->len = buf_rx_used;
			net_pkt_append_buffer(pkt, buf_rx);
			/*
			 * Exit when complete packet is read and added to
			 * struct net_pkt
			 */
			break;
		} else {
			buf_rx_used += tc6->cps;
			if ((buf_rx_size - buf_rx_used) < tc6->cps) {
				net_pkt_append_buffer(pkt, buf_rx);
				buf_rx->len = buf_rx_used;
				buf_rx_used = 0;
				buf_rx = NULL;
			}
		}
	} while (tc6->rca > 0);

	return 0;

unref_buf:
	net_buf_unref(buf_rx);
	return ret;
}
