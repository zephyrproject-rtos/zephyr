/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
/* OOB operations will be attempted regardless of channel enabled or not */
#include "espi_oob_handler.h"

#ifdef CONFIG_ESPI_SAF
#include <zephyr/drivers/espi_saf.h>
#endif

LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);

/* eSPI flash parameters */
#define MAX_TEST_BUF_SIZE   64u
#define MAX_FLASH_REQUEST   64u
#define TARGET_FLASH_REGION 0x1000ul

#define ESPI_FREQ_20MHZ 20u
#define ESPI_FREQ_25MHZ 25u
#define ESPI_FREQ_66MHZ 66u

#define K_WAIT_DELAY 100u

/* eSPI event */
#define EVENT_MASK         0x0000FFFFu
#define EVENT_DETAILS_MASK 0xFFFF0000u
#define EVENT_DETAILS_POS  16u
#define EVENT_TYPE(x)      (x & EVENT_MASK)
#define EVENT_DETAILS(x)   ((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)

#define PWR_SEQ_TIMEOUT 3000u

/* The devicetree node identifier for the board power rails pins. */
#define BRD_PWR_NODE DT_NODELABEL(board_power)

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
static const struct gpio_dt_spec pwrgd_gpio = GPIO_DT_SPEC_GET(BRD_PWR_NODE, pwrg_gpios);
static const struct gpio_dt_spec rsm_gpio = GPIO_DT_SPEC_GET(BRD_PWR_NODE, rsm_gpios);
#endif

static const struct device *const espi_dev = DEVICE_DT_GET(DT_NODELABEL(espi0));
static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;
static uint8_t espi_rst_sts;
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
static struct espi_callback oob_cb;
#endif
#ifdef CONFIG_ESPI_SAF
#ifdef CONFIG_ESPI_TAF_NPCM
static struct espi_callback flash_taf_cb;
#endif
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#ifndef CONFIG_ESPI_SAF
static uint8_t flash_write_buf[MAX_TEST_BUF_SIZE];
static uint8_t flash_read_buf[MAX_TEST_BUF_SIZE];
#endif
#endif

#ifdef CONFIG_ESPI_SAF
#ifdef CONFIG_ESPI_TAF_NPCM
/* CONFIG_ESPI_TAF_NPCM (SOC_SERIES_NPCM4; Nuvoton NPCM ESPI TAF driver) */
#define SAF_TEST_BUF_SIZE 4096U
// ycchen-s: Test
#define SAF_TEST_DATA_LEN 64	// SAF_TEST_BUF_SIZE
// ycchen-e

/* SPI address of 4KB sector modified by test */
#define SAF_SPI_TEST_ADDRESS 0x6000U

enum saf_erase_size {
	SAF_ERASE_4K = 0,
	SAF_ERASE_32K = 1,
	SAF_ERASE_64K = 2,
	SAF_ERASE_MAX
};

static const struct device *const espi_saf_dev = DEVICE_DT_GET(DT_NODELABEL(espi_taf));
static uint8_t safbuf[SAF_TEST_BUF_SIZE] __aligned(4);
static uint8_t safbuf2[SAF_TEST_BUF_SIZE] __aligned(4);

static const struct espi_saf_cfg saf_cfg1 = {
	.nflash_devices = 1U,
	.hwcfg = {},
	.flash_cfgs = NULL};

/*
 * Example for SAF driver set protection regions API.
 */
static const struct espi_saf_pr w25q128_protect_regions[2] = {
	{
		/* Protection range: 0x70001000 ~ (0x70002000 - 1) */
		.start = 0x10001U,
		.end = 0x10002U,
		/* Override TAG: TAG 0/1 */
		.override_r = 0x3U,
		.override_w = 0x3U,
		.master_bm_we = 1U,
		.master_bm_rd = 1U,
		.pr_num = 0U,
		.flags = NPCM_TAF_PR_FLAG_UPDATE_ADDR_RANGE,
	},
	{
		/* Protection range: 0x70003000 ~ (0x70004000 - 1) */
		.start = 0x10003U,
		.end = 0x10004U,
		/* Override TAG: TAG 0/1 */
		.override_r = 0x3U,
		.override_w = 0x3U,
		.master_bm_we = 1U,
		.master_bm_rd = 1U,
		.pr_num = 1U,
		.flags = NPCM_TAF_PR_FLAG_UPDATE_ADDR_RANGE,
	},
};

static const struct espi_saf_protection saf_pr_w25q128 = {.nregions = 2U,
							  .pregions = w25q128_protect_regions};
#else
/* CONFIG_ESPI_SAF_XEC_V2 (SOC_SERIES_MEC172X; XEC Microchip ESPI SAF V2 driver) */

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
static const struct espi_saf_cfg saf_cfg1 = {
	.nflash_devices = 1U,
	.hwcfg = {.version = 2U,           /* TODO */
		  .flags = 0U,                 /* TODO */
		  .qmspi_cpha = 0U,            /* TODO */
		  .qmspi_cs_timing = 0U,       /* TODO */
		  .flash_pd_timeout = 0U,      /* TODO */
		  .flash_pd_min_interval = 0U, /* TODO */
		  .generic_descr = {MCHP_SAF_EXIT_CM_DESCR12, MCHP_SAF_EXIT_CM_DESCR13,
				    MCHP_SAF_POLL_DESCR14, MCHP_SAF_POLL_DESCR15},
		  .tag_map = {0U, 0U, 0U}},
	.flash_cfgs = (struct espi_saf_flash_cfg *)&flash_w25q128};
#endif

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

/*
 * SAF hardware limited to 1 to 64 byte read requests.
 */
static int saf_read(uint32_t spi_addr, uint8_t *dest, int len)
{
	int rc, chunk_len, n;
	struct espi_saf_packet saf_pkt = {0};

	if ((dest == NULL) || (len < 0)) {
		return -EINVAL;
	}

	saf_pkt.flash_addr = spi_addr;
	saf_pkt.buf = dest;

	n = len;
	while (n) {
		chunk_len = 64;
		if (n < 64) {
			chunk_len = n;
		}

		saf_pkt.len = chunk_len;

		rc = espi_saf_flash_read(espi_saf_dev, &saf_pkt);
		if (rc != 0) {
			LOG_INF("%s: error = %d: chunk_len = %d "
				"spi_addr = %x",
				__func__, rc, chunk_len, spi_addr);
			return rc;
		}

		saf_pkt.flash_addr += chunk_len;
		saf_pkt.buf += chunk_len;
		n -= chunk_len;
	}

	return len;
}

/*
 * SAF hardware limited to 4KB(mandatory), 32KB, and 64KB erase sizes.
 * eSPI configuration has flags the Host can read specifying supported
 * erase sizes.
 */
static int saf_erase_block(uint32_t spi_addr, const uint8_t *pckt_meta, enum saf_erase_size ersz)
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
	saf_pkt.buf = (uint8_t *)pckt_meta;

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
	int rc, retries;
	bool erased;
	uint32_t n, saddr, progsz, chunksz;
	struct espi_taf_npcm_pckt taf_data = {0};

	rc = espi_saf_activate(espi_saf_dev);
	LOG_INF("%s: activate = %d", __func__, rc);

	if (spi_addr & 0xfffU) {
		LOG_INF("%s: SPI address 0x%08x not 4KB aligned", __func__, spi_addr);
		spi_addr &= ~(4096U - 1U);
		LOG_INF("%s: Aligned SPI address to 0x%08x", __func__, spi_addr);
	}

	memset(safbuf, 0x55, sizeof(safbuf));
	memset(safbuf2, 0, sizeof(safbuf2));

	erased = false;
	retries = 3;
	while (!erased && (retries-- > 0)) {
		/* read 4KB sector at 0 */
		rc = saf_read(spi_addr, safbuf, SAF_TEST_DATA_LEN);
		if (rc != SAF_TEST_DATA_LEN) {
			LOG_INF("%s: error=%d Read 4K sector at 0x%X failed", __func__, rc,
				spi_addr);
			return rc;
		}

		rc = 0;
		for (n = 0; n < SAF_TEST_DATA_LEN; n++) {
			if (safbuf[n] != 0xffUL) {
				rc = -1;
				break;
			}
		}

		if (rc == 0) {
			LOG_INF("4KB sector at 0x%x is in erased state. "
				"Continue tests",
				spi_addr);
			erased = true;
		} else {
			LOG_INF("4KB sector at 0x%x not in erased state. "
				"Send 4K erase.",
				spi_addr);
			taf_data.tag = 0;	/* assign the tag to 0 just for test */
			taf_data.data = NULL;	/* unused */
			rc = saf_erase_block(spi_addr, (const uint8_t *)&taf_data, SAF_ERASE_4K);
			if (rc != 0) {
				LOG_INF("SAF erase block at 0x%x returned "
					"error %d",
					spi_addr, rc);
				return rc;
			}
		}
	}

	if (!erased) {
		LOG_INF("%s: Could not erase 4KB sector at 0x%08x", __func__, spi_addr);
		return -1;
	}

	/*
	 * Page program test pattern every 256 bytes = 0,1,...,255
	 */
	for (n = 0; n < 4096U; n++) {
		safbuf[n] = n % 256U;
	}

	/* SPI flash sector erase size is 4KB, page program is 256 bytes */
	progsz = 4096U;
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

	rc = saf_read(spi_addr, safbuf2, progsz);
	if (rc == progsz) {
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

	return rc;
}
#endif /* CONFIG_ESPI_SAF */

static void host_warn_handler(uint32_t signal, uint32_t status)
{
	switch (signal) {
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		LOG_INF("Host reset warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("HOST RST ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, status);
		}
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		LOG_INF("Host suspend warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("SUS ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_ACK, status);
		}
		break;
	default:
		break;
	}
}

/* eSPI bus event handler */
static void espi_reset_handler(const struct device *dev, struct espi_callback *cb,
			       struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_RESET) {
		espi_rst_sts = event.evt_data;
		LOG_INF("eSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(const struct device *dev, struct espi_callback *cb,
			    struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_CHANNEL_READY) {
		switch (event.evt_details) {
		case ESPI_CHANNEL_VWIRE:
			LOG_INF("VW channel event %x", event.evt_data);
			break;
		case ESPI_CHANNEL_FLASH:
			LOG_INF("Flash channel event %d", event.evt_data);
			break;
		case ESPI_CHANNEL_OOB:
			LOG_INF("OOB channel event %d", event.evt_data);
			break;
		default:
			LOG_ERR("Unknown channel event");
		}
	}
}

/* eSPI vwire received event handler */
static void vwire_handler(const struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
		switch (event.evt_details) {
		case ESPI_VWIRE_SIGNAL_PLTRST:
			LOG_INF("PLT_RST changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_S3:
		case ESPI_VWIRE_SIGNAL_SLP_S4:
		case ESPI_VWIRE_SIGNAL_SLP_S5:
			LOG_INF("SLP signal changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SUS_WARN:
		case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
			host_warn_handler(event.evt_details, event.evt_data);
			break;
		}
	}
}

/* eSPI peripheral channel notifications handler */
static void periph_handler(const struct device *dev, struct espi_callback *cb,
			   struct espi_event event)
{
	uint8_t periph_type;
	uint8_t periph_index;

	periph_type = EVENT_TYPE(event.evt_details);
	periph_index = EVENT_DETAILS(event.evt_details);

	switch (periph_type) {
	case ESPI_PERIPHERAL_DEBUG_PORT80:
		LOG_INF("Postcode %x", event.evt_data);
		break;
	case ESPI_PERIPHERAL_HOST_IO:
		LOG_INF("ACPI %x", event.evt_data);
		espi_remove_callback(espi_dev, &p80_cb);
		break;
	default:
		LOG_INF("%s periph 0x%x [%x]", __func__, periph_type, event.evt_data);
	}
}

#ifdef CONFIG_ESPI_SAF
#ifdef CONFIG_ESPI_TAF_NPCM
/* eSPI flash TAF received event handler */
static void flash_taf_rx_handler(const struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	struct espi_taf_pckt *taf_pckt = (struct espi_taf_pckt *)event.evt_data;
	uint8_t i, roundsize;
	uint32_t spi_addr;
	uint32_t readsz;
	uint32_t n, saddr, progsz, chunksz;
	int rc;
	struct espi_taf_npcm_pckt taf_data = {0};

	if (event.evt_type == ESPI_BUS_SAF_NOTIFICATION) {
		switch (event.evt_details) {
		case ESPI_CHANNEL_FLASH:
			LOG_INF("Flash TAF request received %d", event.evt_data);
			LOG_INF("  type %d", taf_pckt->type);
			LOG_INF("  len %d", taf_pckt->len);
			LOG_INF("  tag %d", taf_pckt->tag);
			LOG_INF("  addr 0x%x", taf_pckt->addr);
			if (taf_pckt->type == NPCM_ESPI_TAF_REQ_WRITE) {
				roundsize = DIV_ROUND_UP(taf_pckt->len, sizeof(uint32_t));
				for (i = 0; i < roundsize; i++) {
					LOG_INF("  data[%d] = 0x%x", i, taf_pckt->src[i]);
				}
			}
			switch (taf_pckt->type & 0xF) {
			case NPCM_ESPI_TAF_REQ_READ:
				LOG_INF("  Read request");

				spi_addr = taf_pckt->addr;
				readsz = taf_pckt->len;
				taf_data.tag = taf_pckt->tag;
				taf_data.data = safbuf;
				rc = saf_read(spi_addr, (uint8_t *)&taf_data, readsz);
				if (rc != readsz) {
					LOG_INF("%s: error=%d Read 4K sector at 0x%X failed", __func__, rc,
						spi_addr);
				}
				break;
			case NPCM_ESPI_TAF_REQ_WRITE:
				LOG_INF("  Write request");

				/* SPI flash sector erase size is 4KB, page program is 256 bytes */
				progsz = taf_pckt->len;
				chunksz = taf_pckt->len;
				saddr = taf_pckt->addr;
				n = 0;

				LOG_INF("%s: Program 4KB sector at 0x%X", __func__, saddr);

				taf_data.tag = taf_pckt->tag;
				taf_data.data = (uint8_t *)taf_pckt->src;
				while (n < progsz) {
					rc = saf_page_prog(saddr, (const uint8_t *)&taf_data, (int)chunksz);
					if (rc != chunksz) {
						LOG_INF("saf_page_prog error=%d at 0x%X", rc, saddr);
						break;
					}
					saddr += chunksz;
					n += chunksz;
					taf_data.data += chunksz;
				}
				break;
			case NPCM_ESPI_TAF_REQ_ERASE:
				LOG_INF("  Erase request");

				spi_addr = taf_pckt->addr;
				if (spi_addr & 0xfffU) {
					LOG_INF("%s: SPI address 0x%08x not 4KB aligned", __func__, spi_addr);
					spi_addr &= ~(4096U - 1U);
					LOG_INF("%s: Aligned SPI address to 0x%08x", __func__, spi_addr);
				}

				taf_data.tag = taf_pckt->tag;
				taf_data.data = NULL;	/* unused */
				rc = saf_erase_block(spi_addr, (const uint8_t *)&taf_data, SAF_ERASE_4K);
				if (rc != 0) {
					LOG_INF("SAF erase block at 0x%x returned "
						"error %d",
						spi_addr, rc);
				}
				break;
			break;
			}
		}
	}
}
#endif
#endif

int espi_init(void)
{
	int ret;
	/* Indicate to eSPI master simplest configuration: Single line,
	 * 20MHz frequency and only logical channel 0 and 1 are supported
	 */
	struct espi_cfg cfg = {
		.io_caps = ESPI_IO_MODE_SINGLE_LINE,
		.channel_caps = ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
		.max_freq = ESPI_FREQ_20MHZ,
	};

	/* If eSPI driver supports additional capabilities use them */
#ifdef CONFIG_ESPI_OOB_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_OOB;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_FLASH;
	cfg.io_caps |= ESPI_IO_MODE_QUAD_LINES;
	cfg.max_freq = ESPI_FREQ_25MHZ;
#endif

	ret = espi_config(espi_dev, &cfg);
	if (ret) {
		LOG_ERR("Failed to configure eSPI target channels:%x err: %d", cfg.channel_caps,
			ret);
		return ret;
	} else {
		LOG_INF("eSPI target configured successfully!");
	}

	LOG_INF("eSPI test - callbacks initialization... ");
	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler, ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&vw_cb, vwire_handler, ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&p80_cb, periph_handler, ESPI_BUS_PERIPHERAL_NOTIFICATION);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	espi_init_callback(&oob_cb, oob_rx_handler, ESPI_BUS_EVENT_OOB_RECEIVED);
#endif
#ifdef CONFIG_ESPI_SAF
#ifdef CONFIG_ESPI_TAF_NPCM
	espi_init_callback(&flash_taf_cb, flash_taf_rx_handler, ESPI_BUS_SAF_NOTIFICATION);
#endif
#endif
	LOG_INF("complete");

	LOG_INF("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	espi_add_callback(espi_dev, &oob_cb);
#endif
#ifdef CONFIG_ESPI_SAF
#ifdef CONFIG_ESPI_TAF_NPCM
	espi_add_callback(espi_dev, &flash_taf_cb);
#endif
#endif
	LOG_INF("complete");

	return ret;
}

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
static int wait_for_pin(const struct gpio_dt_spec *gpio, uint16_t timeout, int exp_level)
{
	uint16_t loop_cnt = timeout;
	int level;

	do {
		level = gpio_pin_get_dt(gpio);
		if (level < 0) {
			LOG_ERR("Failed to read %x %d", gpio->pin, level);
			return -EIO;
		}

		if (exp_level == level) {
			LOG_DBG("PIN %x = %x", gpio->pin, exp_level);
			break;
		}

		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_ERR("Timeout for %x %x", gpio->pin, level);
		return -ETIMEDOUT;
	}

	return 0;
}
#endif

static int wait_for_vwire(const struct device *espi_dev, enum espi_vwire_signal signal,
			  uint16_t timeout, uint8_t exp_level)
{
	int ret;
	uint8_t level;
	uint16_t loop_cnt = timeout;

	do {
		ret = espi_receive_vwire(espi_dev, signal, &level);
		if (ret) {
			LOG_ERR("Failed to read %x %d", signal, ret);
			return -EIO;
		}

		if (exp_level == level) {
			break;
		}

		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_ERR("VWIRE %d is %x", signal, level);
		return -ETIMEDOUT;
	}

	return 0;
}

static int wait_for_espi_reset(uint8_t exp_sts)
{
	uint16_t loop_cnt = CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT;

	do {
		if (exp_sts == espi_rst_sts) {
			break;
		}
		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		return -ETIMEDOUT;
	}

	return 0;
}

int espi_handshake(void)
{
	int ret;

	LOG_INF("eSPI test - Handshake with eSPI master...");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_WARN, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SUS_WARN Timeout");
		return ret;
	}

	LOG_INF("1st phase completed");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S5, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SLP_S5 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S4, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SLP_S4 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S3, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SLP_S3 Timeout");
		return ret;
	}

	LOG_INF("2nd phase completed");

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_PLTRST, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("PLT_RST Timeout");
		return ret;
	}

	LOG_INF("3rd phase completed");

	return 0;
}

#ifdef CONFIG_ESPI_FLASH_CHANNEL
int read_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t block_len)
{
	uint8_t i = 0;
	uint32_t flash_addr = start_flash_adr;
	uint16_t transactions = block_len / MAX_FLASH_REQUEST;
	int ret = 0;
	struct espi_flash_packet pckt;

	for (i = 0; i < transactions; i++) {
		pckt.buf = buf;
		pckt.flash_addr = flash_addr;
		pckt.len = MAX_FLASH_REQUEST;

		ret = espi_read_flash(espi_dev, &pckt);
		if (ret) {
			LOG_ERR("espi_read_flash failed: %d", ret);
			return ret;
		}

		buf += MAX_FLASH_REQUEST;
		flash_addr += MAX_FLASH_REQUEST;
	}

	LOG_INF("%d read flash transactions completed", transactions);
	return 0;
}

int write_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t block_len)
{
	uint8_t i = 0;
	uint32_t flash_addr = start_flash_adr;
	uint16_t transactions = block_len / MAX_FLASH_REQUEST;
	int ret = 0;
	struct espi_flash_packet pckt;

	/* Split operation in multiple MAX_FLASH_REQ transactions */
	for (i = 0; i < transactions; i++) {
		pckt.buf = buf;
		pckt.flash_addr = flash_addr;
		pckt.len = MAX_FLASH_REQUEST;

		ret = espi_write_flash(espi_dev, &pckt);
		if (ret) {
			LOG_ERR("espi_write_flash failed: %d", ret);
			return ret;
		}

		buf += MAX_FLASH_REQUEST;
		flash_addr += MAX_FLASH_REQUEST;
	}

	LOG_INF("%d write flash transactions completed", transactions);
	return 0;
}

int erase_test_block(uint32_t start_flash_adr)
{
	uint32_t flash_addr = start_flash_adr;
	int ret = 0;
	struct espi_flash_packet pckt;

	pckt.buf = NULL;
	pckt.flash_addr = flash_addr;
	/*
	 * For Flash Erase, the least significant 3 bits of the
	 * length field specifies the size of the block to be
	 * erased with the encoding matches the value of the
	 * Flash Block Erase Size field of the Channel Capabilities
	 * and Configuration register.
	 *   Flash Block Erase Size:
	 *     001b: 4 Kbytes
	 *     010b: 64 Kbytes
	 *     011b: Both 4 Kbytes and 64 Kbytes are supported
	 *     100b: 128 Kbytes
	 *     101b: 256 Kbytes
	 */
	pckt.len = 1;

	ret = espi_flash_erase(espi_dev, &pckt);
	if (ret) {
		LOG_ERR("espi_flash_erase failed: %d", ret);
		return ret;
	}

	LOG_INF("erase flash transactions completed");
	return 0;
}

#ifndef CONFIG_ESPI_SAF
static int espi_flash_test(uint32_t start_flash_addr, uint8_t blocks)
{
	uint32_t i, j;
	// uint8_t pattern;
	uint32_t flash_addr;
	int ret = 0;

	LOG_INF("Test eSPI erase flash");
	flash_addr = start_flash_addr & ~0xFFF;	/* align to 4K boundary */
	for (i = 0; i < blocks; i++) {
		ret = erase_test_block(flash_addr);
		if (ret) {
			LOG_ERR("Failed to erase to eSPI");
			return ret;
		}

		flash_addr += 0x1000;
	}

	LOG_INF("Test eSPI write flash");
	flash_addr = start_flash_addr;
	// pattern = 0x99;
	for (i = 0; i < blocks; i++) {
		// memset(flash_write_buf, pattern++, sizeof(flash_write_buf));
		for (j = 0; j < sizeof(flash_write_buf); j++) {
			flash_write_buf[j] = j;
		}
		ret = write_test_block(flash_write_buf, flash_addr, sizeof(flash_write_buf));
		if (ret) {
			LOG_ERR("Failed to write to eSPI");
			return ret;
		}

		flash_addr += sizeof(flash_write_buf);
	}

	LOG_INF("Test eSPI read flash");
	flash_addr = start_flash_addr;
	// pattern = 0x99;
	for (i = 0; i < blocks; i++) {
		/* Set expected content */
		// memset(flash_write_buf, pattern, sizeof(flash_write_buf));
		for (j = 0; j < sizeof(flash_write_buf); j++) {
			flash_write_buf[j] = j;
		}
		/* Clear last read content */
		memset(flash_read_buf, 0, sizeof(flash_read_buf));
		ret = read_test_block(flash_read_buf, flash_addr, sizeof(flash_read_buf));
		if (ret) {
			LOG_ERR("Failed to read from eSPI");
			return ret;
		}

		/* Compare buffers  */
		int cmp = memcmp(flash_write_buf, flash_read_buf, sizeof(flash_write_buf));

		if (cmp != 0) {
			// LOG_ERR("eSPI read mismmatch at %d expected %x", cmp, pattern);
			LOG_ERR("eSPI read mismmatch at %d", cmp);
		}

		flash_addr += sizeof(flash_read_buf);
		// pattern++;
	}

	return 0;
}
#endif
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

#ifndef CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE
static void send_target_bootdone(void)
{
	int ret;
	uint8_t boot_done;

	ret = espi_receive_vwire(espi_dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, &boot_done);
	LOG_INF("%s boot_done: %d", __func__, boot_done);
	if (ret) {
		LOG_WRN("Fail to retrieve target boot done");
	} else if (!boot_done) {
		/* TARGET_BOOT_DONE & TARGET_LOAD_STS have to be sent together */
		espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);
	}
}
#endif

int espi_test(void)
{
	int ret;

	/* Account for the time serial port is detected so log messages can
	 * be seen
	 */
	k_sleep(K_SECONDS(1));

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
	if (!gpio_is_ready_dt(&pwrgd_gpio)) {
		LOG_ERR("%s: device not ready.", pwrgd_gpio.port->name);
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&rsm_gpio)) {
		LOG_ERR("%s: device not ready.", rsm_gpio.port->name);
		return -ENODEV;
	}
#endif
	if (!device_is_ready(espi_dev)) {
		LOG_ERR("%s: device not ready.", espi_dev->name);
		return -ENODEV;
	}

#ifdef CONFIG_ESPI_SAF
	if (!device_is_ready(espi_saf_dev)) {
		LOG_ERR("%s: device not ready.", espi_saf_dev->name);
		return -ENODEV;
	}
#endif

	LOG_INF("Hello eSPI test %s", CONFIG_BOARD);

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
	ret = gpio_pin_configure_dt(&pwrgd_gpio, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Unable to configure %d:%d", pwrgd_gpio.pin, ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&rsm_gpio, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Unable to config %d: %d", rsm_gpio.pin, ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&rsm_gpio, 0);
	if (ret) {
		LOG_ERR("Unable to initialize %d", rsm_gpio.pin);
		return -1;
	}
#endif

	espi_init();

#ifdef CONFIG_ESPI_SAF
	/*
	 * eSPI SAF configuration must be after eSPI configuration.
	 * eSPI SAF EC portal flash tests before EC releases RSMRST# and
	 * Host de-asserts ESPI_RESET#.
	 */
	ret = espi_saf_init();
	if (ret) {
		LOG_ERR("Unable to configure %d:%s", ret, espi_saf_dev->name);
		return ret;
	}

// ycchen-s: Test
	// ret = espi_saf_test1(SAF_SPI_TEST_ADDRESS);
	ret = 0;
// ycchen-e
	if (ret) {
		LOG_INF("eSPI SAF test1 returned error %d", ret);
	}
#endif

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
	ret = wait_for_pin(&pwrgd_gpio, PWR_SEQ_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("RSMRST_PWRGD timeout");
		return ret;
	}

	ret = gpio_pin_set_dt(&rsm_gpio, 1);
	if (ret) {
		LOG_ERR("Failed to set rsm err: %d", ret);
		return ret;
	}
#endif
	ret = wait_for_espi_reset(1);
	if (ret) {
		LOG_INF("ESPI_RESET timeout");
		return ret;
	}

#ifndef CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE
	/* When automatic acknowledge is disabled to perform lengthy operations
	 * in the eSPI target, need to explicitly send target boot virtual wires
	 */
	bool vw_ch_sts;

	/* Simulate lengthy operation during boot */
	k_sleep(K_SECONDS(2));

	do {
		vw_ch_sts = espi_get_channel_status(espi_dev, ESPI_CHANNEL_VWIRE);
		k_busy_wait(100);
	} while (!vw_ch_sts);

	send_target_bootdone();
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* Flash operation need to be perform before VW handshake or
	 * after eSPI host completes full initialization.
	 * This sample code can't assume a full initialized eSPI host
	 * so flash operations are perform here.
	 */
	bool flash_sts;

	do {
		flash_sts = espi_get_channel_status(espi_dev, ESPI_CHANNEL_FLASH);
		k_busy_wait(100);
	} while (!flash_sts);

#ifndef CONFIG_ESPI_SAF
	/* eSPI flash test can fail and rest of operation can continue */
	ret = espi_flash_test(TARGET_FLASH_REGION, 1);
	if (ret) {
		LOG_INF("eSPI flash test failed %d", ret);
	}
#endif
#endif

	/* Showcase VW channel by exchanging virtual wires with eSPI host */
	ret = espi_handshake();
	if (ret) {
		LOG_ERR("eSPI VW handshake failed %d", ret);
		return ret;
	}

	/*  Attempt to use OOB channel to read temperature, regardless of
	 * if is enabled or not.
	 */
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	/* System without host-initiated OOB Rx traffic */
	get_pch_temp_sync(espi_dev);
#else
	/* System with host-initiated OOB Rx traffic */
	get_pch_temp_async(espi_dev);
#endif

	/* Cleanup */
	k_sleep(K_SECONDS(1));
	espi_remove_callback(espi_dev, &espi_bus_cb);
	espi_remove_callback(espi_dev, &vw_rdy_cb);
	espi_remove_callback(espi_dev, &vw_cb);
	espi_remove_callback(espi_dev, &p80_cb);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	espi_remove_callback(espi_dev, &oob_cb);
#endif
#ifdef CONFIG_ESPI_SAF
#ifdef CONFIG_ESPI_TAF_NPCM
	espi_remove_callback(espi_dev, &flash_taf_cb);
#endif
#endif

	LOG_INF("eSPI sample completed err: %d", ret);

	return ret;
}

int main(void)
{
	espi_test();
	return 0;
}
