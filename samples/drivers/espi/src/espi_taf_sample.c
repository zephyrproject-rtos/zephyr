/*
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi_saf.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "espi_taf_sample.h"

LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);

#ifdef CONFIG_SOC_SERIES_MEC15XX
/* Not updating old HAL just for these */
#define MCHP_SAF_PR_REGS_SIZE 0x10u

#define MCHP_SAF_PR_START_OFS(n)                                                                   \
	(MCHP_SAF_PROT_RG0_START_OFS + ((uint32_t)(n) * MCHP_SAF_PR_REGS_SIZE))
#define MCHP_SAF_PR_LIMIT_OFS(n)                                                                   \
	(MCHP_SAF_PROT_RG0_LIMIT_OFS + ((uint32_t)(n) * MCHP_SAF_PR_REGS_SIZE))

#define MCHP_SAF_PR_WBM_OFS(n) (MCHP_SAF_PROT_RG0_WBM_OFS + ((uint32_t)(n) * MCHP_SAF_PR_REGS_SIZE))
#define MCHP_SAF_PR_RBM_OFS(n) (MCHP_SAF_PROT_RG0_RBM_OFS + ((uint32_t)(n) * MCHP_SAF_PR_REGS_SIZE))
#endif

/* Device bindings */
#ifdef CONFIG_ESPI_TAF_XEC_V2
#define ESPI_TAF_FLASH_CTRL_NODE DT_NODELABEL(qspi0)
#define ESPI_TAF_NODE            DT_NODELABEL(espi_taf0)
#else
#define ESPI_TAF_FLASH_CTRL_NODE DT_NODELABEL(spi0)
#define ESPI_TAF_NODE            DT_NODELABEL(espi_saf0)
#endif

static const struct device *const qspi_dev = DEVICE_DT_GET(ESPI_TAF_FLASH_CTRL_NODE);
static const struct device *const espi_saf_dev = DEVICE_DT_GET(ESPI_TAF_NODE);

/* Test buffers */
static uint8_t safbuf[SAF_TEST_BUF_SIZE] __aligned(4);
static uint8_t safbuf2[SAF_TEST_BUF_SIZE] __aligned(4);

/*
 * W25Q128 SPI flash SAF configuration.
 * Size is 16Mbytes, it requires no continuous mode prefix, or
 * other special SAF configuration.
 */
static const struct espi_saf_flash_cfg flash_w25q128 = {
	.flashsz = 0x1000000U,
	.opa = MCHP_SAF_OPCODE_REG_VAL(0x06U, 0x75U, 0x7aU, 0x05U),
	.opb = MCHP_SAF_OPCODE_REG_VAL(0x20U, 0x52U, 0xd8U, 0x02U),
	.opc = MCHP_SAF_OPCODE_REG_VAL(0xebU, 0xffU, 0xa5U, 0x35U),
	.poll2_mask = MCHP_W25Q128_POLL2_MASK,
	.cont_prefix = 0U,
	.cs_cfg_descr_ids = MCHP_CS0_CFG_DESCR_IDX_REG_VAL,
	.flags = 0,
	.descr = {MCHP_W25Q128_CM_RD_D0, MCHP_W25Q128_CM_RD_D1, MCHP_W25Q128_CM_RD_D2,
		  MCHP_W25Q128_ENTER_CM_D0, MCHP_W25Q128_ENTER_CM_D1, MCHP_W25Q128_ENTER_CM_D2}};

/*
 * SAF driver configuration.
 * One SPI flash device.
 * Use QMSPI frequency, chip select timing, and signal sampling configured
 * by QMSPI driver.
 * Use SAF hardware default TAG map.
 */
#ifdef CONFIG_ESPI_TAF_XEC_V2
static const struct espi_saf_cfg saf_cfg1 = {
	.nflash_devices = 1U,
	.hwcfg = {.version = 2U,
		  .flags = 0U,
		  .qmspi_cpha = 0U,
		  .qmspi_cs_timing = 0U,
		  .flash_pd_timeout = 0U,
		  .flash_pd_min_interval = 0U,
		  .generic_descr = {MCHP_SAF_EXIT_CM_DESCR12, MCHP_SAF_EXIT_CM_DESCR13,
				    MCHP_SAF_POLL_DESCR14, MCHP_SAF_POLL_DESCR15},
		  .tag_map = {0U, 0U, 0U}},
	.flash_cfgs = (struct espi_saf_flash_cfg *)&flash_w25q128};
#else
static const struct espi_saf_cfg saf_cfg1 = {
	.nflash_devices = 1U,
	.hwcfg = {.qmspi_freq_hz = 0U,
		  .qmspi_cs_timing = 0U,
		  .qmspi_cpha = 0U,
		  .flags = 0U,
		  .generic_descr = {MCHP_SAF_EXIT_CM_DESCR12, MCHP_SAF_EXIT_CM_DESCR13,
				    MCHP_SAF_POLL_DESCR14, MCHP_SAF_POLL_DESCR15},
		  .tag_map = {0U, 0U, 0U}},
	.flash_cfgs = (struct espi_saf_flash_cfg *)&flash_w25q128};
#endif

/*
 * Example for SAF driver set protection regions API.
 */
static const struct espi_saf_pr w25q128_protect_regions[2] = {
	{
		.start = 0xe00000U,
		.size = 0x100000U,
		.master_bm_we = (1U << MCHP_SAF_MSTR_HOST_PCH_ME),
		.master_bm_rd = (1U << MCHP_SAF_MSTR_HOST_PCH_ME),
		.pr_num = 1U,
		.flags = MCHP_SAF_PR_FLAG_ENABLE | MCHP_SAF_PR_FLAG_LOCK,
	},
	{
		.start = 0xf00000U,
		.size = 0x100000U,
		.master_bm_we = (1U << MCHP_SAF_MSTR_HOST_PCH_LAN),
		.master_bm_rd = (1U << MCHP_SAF_MSTR_HOST_PCH_LAN),
		.pr_num = 2U,
		.flags = MCHP_SAF_PR_FLAG_ENABLE | MCHP_SAF_PR_FLAG_LOCK,
	},
};

static const struct espi_saf_protection saf_pr_w25q128 = {.nregions = 2U,
							  .pregions = w25q128_protect_regions};

/* Transmit command opcode plus optional params and optionally read a response
 * Helper for sending short commands to the SPI flash device.
 * Uses Zephyr SPI driver which should ONLY implement Full-Duplex per Zephyr documentation.
 * Zephyr's definition of full-duplex is to transmit and receive on every clock.
 * If no TX buffer is given the driver transmits DT based dummy data.
 * If no RX buffer is given the driver discards the data clocked in.
 */
int spi_cmd_response(const struct device *spi_dev, const struct spi_config *spi_cfg, uint8_t opcode,
		     uint8_t *params, uint8_t paramlen, uint8_t *response, uint8_t resplen)
{
	struct spi_buf_set tx_bufs = {0};
	struct spi_buf_set rx_bufs = {0};
	struct spi_buf txb[3] = {0};
	struct spi_buf rxb[3] = {0};
	uint8_t buf[4] = {0};
	uint8_t buf_idx = 0;
	int rc = 0;

	if ((spi_dev == NULL) || (spi_cfg == NULL)) {
		return -EINVAL;
	}

	buf[buf_idx] = opcode;
	txb[buf_idx].buf = buf;
	txb[buf_idx].len = 1U;
	rxb[buf_idx].buf = NULL;
	rxb[buf_idx].len = 1U;
	buf_idx++;

	if ((params != NULL) && (paramlen != 0)) {
		txb[buf_idx].buf = (uint8_t *)params;
		txb[buf_idx].len = paramlen;
		rxb[buf_idx].buf = NULL;
		rxb[buf_idx].len = paramlen;
		buf_idx++;
	}

	if ((response != NULL) && (resplen != 0)) {
		txb[buf_idx].buf = NULL;
		txb[buf_idx].len = resplen;
		rxb[buf_idx].buf = response;
		rxb[buf_idx].len = resplen;
		buf_idx++;
	}

	tx_bufs.buffers = &txb[0];
	tx_bufs.count = buf_idx;

	rx_bufs.buffers = &rxb[0];
	rx_bufs.count = buf_idx;

	rc = spi_transceive(spi_dev, spi_cfg, (const struct spi_buf_set *)&tx_bufs,
			    (const struct spi_buf_set *)&rx_bufs);
	if (rc != 0) {
		LOG_ERR("SPI cmd response error %d", rc);
		return rc;
	}

	return 0;
}

int spi_saf_init(void)
{
	struct spi_config spi_cfg = {0};
	uint32_t jedec_id = 0, temp = 0;
	uint8_t spi_status1 = 0, spi_status2 = 0;
	int ret = 0;

	/*
	 * Use SPI master mode and inform driver the SPI controller hardware
	 * controls chip select.
	 */
	spi_cfg.frequency = SAF_TEST_FREQ_HZ;
	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8);
	spi_cfg.slave = 0;
	spi_cfg.cs.delay = 0;
	spi_cfg.cs.gpio.pin = 0;
	spi_cfg.cs.gpio.dt_flags = 0;
	spi_cfg.cs.gpio.port = NULL;

	/* Read JEDEC ID command and fill read buffer */
	ret = spi_cmd_response(qspi_dev, (const struct spi_config *)&spi_cfg, SPI_READ_JEDEC_ID,
			       NULL, 0, (uint8_t *)&jedec_id, 3U);
	if (ret) {
		LOG_ERR("Read JEDEC ID spi_transceive failure: error %d", ret);
		return ret;
	}

	if ((jedec_id != W25Q128_JEDEC_ID) && (jedec_id != W25Q128_FW_JEDEC_ID)) {
		LOG_ERR("JEDIC ID 0x%08x does not match W25Q128(0x%08x) or W25Q128_FW(0x%08x)",
			jedec_id, W25Q128_JEDEC_ID, W25Q128_FW_JEDEC_ID);
		return -1;
	}

	/* Read STATUS2 to get quad enable bit */
	ret = spi_cmd_response(qspi_dev, (const struct spi_config *)&spi_cfg, SPI_READ_STATUS2,
			       NULL, 0, (uint8_t *)&spi_status2, 1U);
	if (ret != 0) {
		LOG_ERR("Read SPI STATUS2 error (%d)", ret);
		return ret;
	}

	LOG_INF("SPI flash STATUS2 = 0x%02x", spi_status2);

	/*
	 * If QE not set then write the volatile QE bit.
	 * SAF test requires SPI flash quad enabled so the WP#/HOLD# signals
	 * will act as IO2/IO3. We will write the volatile QE bit for less
	 * wear of the STATUS2 register
	 */
	if ((spi_status2 & SPI_STATUS2_QE) == 0U) {
		ret = spi_cmd_response(qspi_dev, (const struct spi_config *)&spi_cfg,
				       SPI_WRITE_ENABLE_VS, NULL, 0, NULL, 0);
		if (ret) {
			LOG_ERR("Send write enable volatile spi_transceive failure: error %d", ret);
			return ret;
		}

		temp = spi_status2 | SPI_STATUS2_QE;
		ret = spi_cmd_response(qspi_dev, (const struct spi_config *)&spi_cfg,
				       SPI_WRITE_STATUS2, (uint8_t *)&temp, 1U, NULL, 0);
		if (ret) {
			LOG_ERR("Write SPI STATUS2 QE=1 spi_transceive"
				" failure: error %d",
				ret);
			return ret;
		}

		/* Write to volatile status is fast, expect BUSY to be clear */
		ret = spi_cmd_response(qspi_dev, (const struct spi_config *)&spi_cfg,
				       SPI_READ_STATUS1, NULL, 0, (uint8_t *)&spi_status1, 1U);
		if (ret) {
			LOG_ERR("Read SPI STATUS1 spi_transceive failure: error %d", ret);
			return ret;
		}

		if ((spi_status1 & SPI_STATUS1_BUSY) != 0) {
			LOG_ERR("SPI BUSY set after write to volatile STATUS2: STATUS1=0x%02X",
				spi_status1);
			return ret;
		}

		/* Read STATUS2 and check if QE bit was set */
		ret = spi_cmd_response(qspi_dev, (const struct spi_config *)&spi_cfg,
				       SPI_READ_STATUS2, NULL, 0, (uint8_t *)&spi_status2, 1U);
		if (ret) {
			LOG_ERR("Read 2 of SPI STATUS2 spi_transceive failure: error %d", ret);
			return ret;
		}

		if ((spi_status2 & SPI_STATUS2_QE) == 0) {
			LOG_ERR("Read back of SPI STATUS2 after setting "
				"volatile QE bit shows QE not set: 0x%02X",
				spi_status2);
			return -1;
		}
	}

	return 0;
}

int espi_saf_init(void)
{
	int ret;

	ret = espi_saf_config(espi_saf_dev, (struct espi_saf_cfg *)&saf_cfg1);
	if (ret) {
		LOG_ERR("Failed to configure eSPI SAF error %d", ret);
	} else {
		LOG_INF("eSPI SAF configured successfully!");
	}

	ret = espi_saf_set_protection_regions(espi_saf_dev, &saf_pr_w25q128);
	if (ret) {
		LOG_ERR("Failed to set SAF protection region(s) %d", ret);
	} else {
		LOG_INF("eSPI SAF protection regions(s) configured!");
	}

	return ret;
}

static int pr_check_range(mm_reg_t taf_base, const struct espi_saf_pr *pr)
{
	uint32_t expected_limit = pr->start + pr->size - 1U;
	uint32_t start = (sys_read32(taf_base + MCHP_SAF_PR_START_OFS(pr->pr_num)) &
			  MCHP_SAF_PROT_RG_START_MASK);
	uint32_t limit = (sys_read32(taf_base + MCHP_SAF_PR_LIMIT_OFS(pr->pr_num)) &
			  MCHP_SAF_PROT_RG_LIMIT_MASK);

	/* registers b[19:0] = bits[31:12] (align on 4KB) */
	if (start != (pr->start >> 12)) {
		return -1;
	}

	if (limit != (expected_limit >> 12)) {
		return -1;
	}

	return 0;
}

static int pr_check_enable(mm_reg_t taf_base, const struct espi_saf_pr *pr)
{
	uint32_t start = (sys_read32(taf_base + MCHP_SAF_PR_START_OFS(pr->pr_num)) &
			  MCHP_SAF_PROT_RG_START_MASK);
	uint32_t limit = (sys_read32(taf_base + MCHP_SAF_PR_LIMIT_OFS(pr->pr_num)) &
			  MCHP_SAF_PROT_RG_LIMIT_MASK);

	if (pr->flags & MCHP_SAF_PR_FLAG_ENABLE) {
		if (limit > start) {
			return 0;
		}
	} else {
		if (start > limit) {
			return 0;
		}
	}

	return -2;
}

static int pr_check_lock(mm_reg_t taf_base, const struct espi_saf_pr *pr)
{
	uint32_t lockbm = sys_read32(taf_base + MCHP_SAF_PROT_LOCK_OFS);

	if (pr->flags & MCHP_SAF_PR_FLAG_LOCK) {
		if ((lockbm & BIT(pr->pr_num)) != 0) {
			return 0;
		}
	} else {
		if ((lockbm & BIT(pr->pr_num)) == 0) {
			return 0;
		}
	}

	return -3;
}

/* NOTE: bit[0] of bit map registers is read-only = 1 */
static int pr_check_master_bm(mm_reg_t taf_base, const struct espi_saf_pr *pr)
{
	uint32_t webm = (sys_read32(taf_base + MCHP_SAF_PR_WBM_OFS(pr->pr_num)) &
			 MCHP_SAF_PROT_RG_WBM_MASK);
	uint32_t rdbm = (sys_read32(taf_base + MCHP_SAF_PR_RBM_OFS(pr->pr_num)) &
			 MCHP_SAF_PROT_RG_RBM_MASK);

	if ((webm != (pr->master_bm_we | BIT(0))) || (rdbm != (pr->master_bm_rd | BIT(0)))) {
		return -4;
	}

	return 0;
}

static int espi_saf_test_pr1(const struct espi_saf_protection *spr)
{
	mm_reg_t taf_base = (mm_reg_t)(SAF_BASE_ADDR);
	const struct espi_saf_pr *pr;
	int rc;

	LOG_INF("espi_saf_test_pr1");

	if (spr == NULL) {
		return 0;
	}

	pr = spr->pregions;

	for (size_t n = 0U; n < spr->nregions; n++) {
		rc = pr_check_range(taf_base, pr);
		if (rc) {
			LOG_INF("TAF Protection region %u range fail", pr->pr_num);
			return rc;
		}

		rc = pr_check_enable(taf_base, pr);
		if (rc) {
			LOG_INF("TAF Protection region %u enable fail", pr->pr_num);
			return rc;
		}

		rc = pr_check_lock(taf_base, pr);
		if (rc) {
			LOG_INF("TAF Protection region %u lock check fail", pr->pr_num);
			return rc;
		}

		rc = pr_check_master_bm(taf_base, pr);
		if (rc) {
			LOG_INF("TAF Protection region %u Master select fail", pr->pr_num);
			return rc;
		}

		pr++;
	}

	return 0;
}

/*
 * SAF hardware limited to 1 to 64 byte read requests.
 */
static int saf_read(uint32_t spi_addr, uint8_t *dest, uint32_t reqlen, uint32_t *actual_read_len)
{
	int rc = 0;
	uint32_t chunk_len = 0, n = 0, nr = 0;
	struct espi_saf_packet saf_pkt = {0};

	if (dest == NULL) {
		return -EINVAL;
	}

	if (reqlen == 0) {
		return 0; /* nothing to do */
	}

	saf_pkt.flash_addr = spi_addr;
	saf_pkt.buf = dest;

	n = reqlen;
	while (n != 0) {
		chunk_len = 64U;
		if (n < 64U) {
			chunk_len = n;
		}

		saf_pkt.len = chunk_len;

		rc = espi_saf_flash_read(espi_saf_dev, &saf_pkt);
		if (rc != 0) {
			LOG_INF("%s: error = %d: chunk_len = %u "
				"spi_addr = %x",
				__func__, rc, chunk_len, spi_addr);
			break;
		}

		saf_pkt.flash_addr += chunk_len;
		saf_pkt.buf += chunk_len;
		nr += chunk_len;
		n -= chunk_len;
	}

	if (actual_read_len != NULL) {
		*actual_read_len = nr;
	}

	return rc;
}

/*
 * SAF hardware limited to 4KB(mandatory), 32KB, and 64KB erase sizes.
 * eSPI configuration has flags the Host can read specifying supported
 * erase sizes.
 */
static int saf_erase_block(uint32_t spi_addr, enum saf_erase_size ersz)
{
	int rc;
	struct espi_saf_packet saf_pkt = {0};

	switch (ersz) {
	case SAF_ERASE_4K:
		saf_pkt.len = 4096U;
		spi_addr &= ~(4096U - 1U);
		break;
	case SAF_ERASE_32K:
		saf_pkt.len = (32U * 1024U);
		spi_addr &= ~((32U * 1024U) - 1U);
		break;
	case SAF_ERASE_64K:
		saf_pkt.len = (64U * 1024U);
		spi_addr &= ~((64U * 1024U) - 1U);
		break;
	default:
		return -EINVAL;
	}

	saf_pkt.flash_addr = spi_addr;

	rc = espi_saf_flash_erase(espi_saf_dev, &saf_pkt);
	if (rc != 0) {
		LOG_INF("espi_saf_test1: erase fail = %d", rc);
		return rc;
	}

	return 0;
}

/*
 * SAF hardware limited to 1 to 64 byte programming within a 256 byte page.
 */
static int saf_page_prog(uint32_t spi_addr, const uint8_t *src, int progsz)
{
	int rc, chunk_len, n;
	struct espi_saf_packet saf_pkt = {0};

	if ((src == NULL) || (progsz < 0) || (progsz > 256)) {
		return -EINVAL;
	}

	if (progsz == 0) {
		return 0;
	}

	saf_pkt.flash_addr = spi_addr;
	saf_pkt.buf = (uint8_t *)src;

	n = progsz;
	while (n) {
		chunk_len = 64;
		if (n < 64) {
			chunk_len = n;
		}

		saf_pkt.len = (uint32_t)chunk_len;

		rc = espi_saf_flash_write(espi_saf_dev, &saf_pkt);
		if (rc != 0) {
			LOG_INF("%s: error = %d: erase fail spi_addr = 0x%X", __func__, rc,
				spi_addr);
			return rc;
		}

		saf_pkt.flash_addr += chunk_len;
		saf_pkt.buf += chunk_len;
		n -= chunk_len;
	}

	return progsz;
}

int espi_saf_test1(uint32_t spi_addr)
{
	int rc = 0, retries = 0;
	bool erased = false;
	uint32_t n = 0, num_bytes_to_read = 0, num_bytes_read = 0;
	uint32_t saddr = 0, progsz = 0, chunksz = 0;

	rc = espi_saf_activate(espi_saf_dev);
	LOG_INF("%s: activate = %d", __func__, rc);

	if (spi_addr & 0xfffU) {
		LOG_INF("%s: SPI address 0x%08x not 4KB aligned", __func__, spi_addr);
		spi_addr &= ~(4096U - 1U);
		LOG_INF("%s: Aligned SPI address to 0x%08x", __func__, spi_addr);
	}

	memset(safbuf, 0x55, sizeof(safbuf));
	memset(safbuf2, 0, sizeof(safbuf2));

	num_bytes_to_read = KB(4);
	retries = 3;
	while (!erased && (retries-- > 0)) {
		/* read 4KB sector at 0 */
		num_bytes_read = 0;
		rc = saf_read(spi_addr, safbuf, num_bytes_to_read, &num_bytes_read);
		if (rc != 0) {
			LOG_INF("%s: error=%d Read 4K sector at 0x%X failed", __func__, rc,
				spi_addr);
			return rc;
		}

		if (num_bytes_read != num_bytes_to_read) {
			LOG_ERR("%s: Requested read of %u bytes, actually read %u bytes", __func__,
				num_bytes_to_read, num_bytes_read);
			return -EIO;
		}

		rc = 0;
		for (n = 0; n < num_bytes_to_read; n++) {
			if (safbuf[n] != 0xffUL) {
				rc = -1;
				break;
			}
		}

		if (rc == 0) {
			LOG_INF("4KB sector at 0x%x is in erased state. Continue tests", spi_addr);
			erased = true;
		} else {
			LOG_INF("4KB sector at 0x%x not in erased state. Send 4K erase.", spi_addr);
			rc = saf_erase_block(spi_addr, SAF_ERASE_4K);
			if (rc != 0) {
				LOG_INF("SAF erase block at 0x%x returned error %d", spi_addr, rc);
				return rc;
			}
		}
	}

	if (!erased) {
		LOG_INF("%s: Could not erase 4KB sector at 0x%08x", __func__, spi_addr);
		return -1;
	}

	/* Use a random test pattern in case driver reports erase or write successful
	 * but it actually is not.
	 * !!! WARNING we are not using a cryptographically secure RNG !!!
	 */
	sys_rand_get(safbuf, KB(4));

	/* SPI flash sector erase size is 4KB, page program is 256 bytes */
	progsz = KB(4);
	chunksz = 256U;
	saddr = spi_addr;
	n = 0;
	const uint8_t *src = (const uint8_t *)safbuf;

	LOG_INF("%s: Program 4KB sector at 0x%X", __func__, saddr);

	while (n < progsz) {
		rc = saf_page_prog(saddr, (const uint8_t *)src, (int)chunksz);
		if (rc != chunksz) {
			LOG_INF("saf_page_prog error=%d at 0x%X", rc, saddr);
			break;
		}
		saddr += chunksz;
		n += chunksz;
		src += chunksz;
	}

	/* read back and check */
	LOG_INF("%s: Read back 4K sector at 0x%X", __func__, spi_addr);

	num_bytes_read = 0;
	rc = saf_read(spi_addr, safbuf2, progsz, &num_bytes_read);
	if (rc != 0) {
		rc = memcmp(safbuf, safbuf2, progsz);
		if (rc == 0) {
			LOG_INF("%s: Read back match: PASS", __func__);
		} else {
			LOG_INF("%s: Read back mismatch: FAIL", __func__);
		}
	} else {
		LOG_INF("%s: Read back 4K error=%d", __func__, rc);
		return rc;
	}

	/* Run protection region test */
	rc = espi_saf_test_pr1(&saf_pr_w25q128);
	if (rc) {
		LOG_INF("eSPI SAF test pr1 returned error %d", rc);
	}

	return rc;
}
