/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_espi_taf

#include <soc.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi_saf.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espi_taf, CONFIG_ESPI_LOG_LEVEL);

static const struct device *const spi_dev = DEVICE_DT_GET(DT_ALIAS(taf_flash));

enum ESPI_TAF_ERASE_LEN {
	NPCX_ESPI_TAF_ERASE_LEN_4KB,
	NPCX_ESPI_TAF_ERASE_LEN_32KB,
	NPCX_ESPI_TAF_ERASE_LEN_64KB,
	NPCX_ESPI_TAF_ERASE_LEN_128KB,
	NPCX_ESPI_TAF_ERASE_LEN_MAX,
};

struct espi_taf_npcx_config {
	uintptr_t base;
	uintptr_t mapped_addr;
	uintptr_t rx_plsz;
	enum NPCX_ESPI_TAF_ERASE_BLOCK_SIZE erase_sz;
	enum NPCX_ESPI_TAF_MAX_READ_REQ max_rd_sz;
};

struct espi_taf_npcx_data {
	sys_slist_t *callbacks;
	const struct device *host_dev;
	struct espi_taf_pckt pckt;
	struct k_work work;
};

static struct espi_taf_npcx_data npcx_espi_taf_data;
static struct espi_callback espi_taf_cb;

#define HAL_INSTANCE(dev)						\
	((struct espi_reg *)((const struct espi_taf_npcx_config *)	\
	(dev)->config)->base)

#define FLBASE_ADDR (							\
	GET_FIELD(inst->FLASHBASE, NPCX_FLASHBASE_FLBASE_ADDR)		\
	<< GET_FIELD_POS(NPCX_FLASHBASE_FLBASE_ADDR))

/* Cycle types with data: 0x09, 0x0B, 0x0D, 0x0F (all odd and >= 0x09) */
#define CYC_TYPE_HAS_DATA(type) \
	(((type) >= CYC_SCS_CMP_WITH_DATA_MIDDLE) && ((type) & 0x01))

/* Cycle types that are part of a multi-packet sequence (not last or only) */
#define CYC_TYPE_IS_CONTINUATION(type) \
	(((type) == CYC_SCS_CMP_WITH_DATA_FIRST) || \
	 ((type) == CYC_SCS_CMP_WITH_DATA_MIDDLE))

#define PRTR_BADDR(i) (							\
	GET_FIELD(inst->FLASH_PRTR_BADDR[i], NPCX_FLASH_PRTR_BADDR)	\
	<< GET_FIELD_POS(NPCX_FLASH_PRTR_BADDR))

#define PRTR_HADDR(i) (							\
	GET_FIELD(inst->FLASH_PRTR_HADDR[i], NPCX_FLASH_PRTR_HADDR)	\
	<< GET_FIELD_POS(NPCX_FLASH_PRTR_HADDR)) | 0xFFF;

static void espi_taf_get_pckt(const struct device *dev, struct espi_taf_npcx_data *data,
			      struct espi_event event)
{
	data->pckt = *(struct espi_taf_pckt *)event.evt_data;
}

#if defined(CONFIG_ESPI_TAF_MANUAL_MODE)
/* Check access region of read request is protected or not */
static bool espi_taf_check_read_protect(const struct device *dev, uint32_t addr, uint32_t len,
					uint8_t tag)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t flash_addr = addr;
	uint8_t i;
	uint16_t override_rd;
	uint32_t base, high;
	bool rdpr;

	flash_addr += FLBASE_ADDR;

	for (i = 0; i < CONFIG_ESPI_TAF_PR_NUM; i++) {
		base = PRTR_BADDR(i);
		high = PRTR_HADDR(i);

		rdpr = IS_BIT_SET(inst->FLASH_PRTR_BADDR[i], NPCX_FRGN_RPR);
		override_rd = GET_FIELD(inst->FLASH_RGN_TAG_OVR[i], NPCX_FLASH_TAG_OVR_RPR);

		if (rdpr && !IS_BIT_SET(override_rd, tag) &&
		    (base <= flash_addr + len - 1 && flash_addr <= high)) {
			return true;
		}
	}

	return false;
}
#endif

/* Check access region of write request is protected or not */
static bool espi_taf_check_write_protect(const struct device *dev, uint32_t addr,
					 uint32_t len, uint8_t tag)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t flash_addr = addr;
	uint8_t i;
	uint16_t override_wr;
	uint32_t base, high;
	bool wrpr;

	flash_addr += FLBASE_ADDR;

	for (i = 0; i < CONFIG_ESPI_TAF_PR_NUM; i++) {
		base = PRTR_BADDR(i);
		high = PRTR_HADDR(i);

		wrpr = IS_BIT_SET(inst->FLASH_PRTR_BADDR[i], NPCX_FRGN_WPR);
		override_wr = GET_FIELD(inst->FLASH_RGN_TAG_OVR[i], NPCX_FLASH_TAG_OVR_WPR);

		if (wrpr && !IS_BIT_SET(override_wr, tag) &&
		    (base <= flash_addr + len - 1 && flash_addr <= high)) {
			return true;
		}
	}

	return false;
}

static int espi_taf_npcx_configure(const struct device *dev, const struct espi_saf_cfg *cfg)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);

	if (cfg->nflash_devices == 0U) {
		return -EINVAL;
	}

#if defined(CONFIG_ESPI_TAF_AUTO_MODE)
	inst->FLASHCTL |= BIT(NPCX_FLASHCTL_SAF_AUTO_READ);
#else
	inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_SAF_AUTO_READ);
#endif

	return 0;
}

static int espi_taf_npcx_set_pr(const struct device *dev, const struct espi_saf_protection *pr)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	const struct espi_saf_pr *preg = pr->pregions;
	size_t n = pr->nregions;
	uint8_t regnum;
	uint16_t bitmask, offset;
	uint32_t rw_pr, override_rw;

	if ((dev == NULL) || (pr == NULL)) {
		return -EINVAL;
	}

	if (pr->nregions >= CONFIG_ESPI_TAF_PR_NUM) {
		return -EINVAL;
	}

	while (n--) {
		regnum = preg->pr_num;

		if (regnum >= CONFIG_ESPI_TAF_PR_NUM) {
			return -EINVAL;
		}

		rw_pr = preg->master_bm_we << NPCX_FRGN_WPR;
		rw_pr = rw_pr | (preg->master_bm_rd << NPCX_FRGN_RPR);

		if (preg->flags) {
			bitmask = BIT_MASK(GET_FIELD_SZ(NPCX_FLASH_PRTR_BADDR));
			offset = GET_FIELD_POS(NPCX_FLASH_PRTR_BADDR);
			inst->FLASH_PRTR_BADDR[regnum] = ((preg->start & bitmask) << offset)
							 | rw_pr;
			bitmask = BIT_MASK(GET_FIELD_SZ(NPCX_FLASH_PRTR_HADDR));
			offset = GET_FIELD_POS(NPCX_FLASH_PRTR_HADDR);
			inst->FLASH_PRTR_HADDR[regnum] = (preg->end & bitmask) << offset;
		}

		override_rw = (preg->override_r << 16) | preg->override_w;
		inst->FLASH_RGN_TAG_OVR[regnum] = override_rw;
		preg++;
	}

	return 0;
}

static int espi_taf_npcx_activate(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);

	inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_AUTO_RD_DIS_CTL);
	inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_BLK_FLASH_NP_FREE);

	return 0;
}

static bool espi_taf_npcx_channel_ready(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t ret =
		GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLCAPA) & NPCX_FLASH_SHARING_CAP_SUPP_TAF;

	if (ret != NPCX_FLASH_SHARING_CAP_SUPP_TAF) {
		return false;
	}

	if (!device_is_ready(spi_dev)) {
		return false;
	}

	return true;
}

/* This routine set FLASH_C_AVAIL for standard request */
static void taf_set_flash_c_avail(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t tmp = inst->FLASHCTL;

	/*
	 * Clear FLASHCTL_FLASH_NP_FREE to avoid host puts a flash
	 * standard request command at here.
	 */
	tmp &= NPCX_FLASHCTL_ACCESS_MASK;

	/* Set FLASHCTL_FLASH_TX_AVAIL */
	tmp |= BIT(NPCX_FLASHCTL_FLASH_TX_AVAIL);
	inst->FLASHCTL = tmp;
}

/* This routine release FLASH_NP_FREE for standard request */
static void taf_release_flash_np_free(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t tmp = inst->FLASHCTL;

	/*
	 * Clear FLASHCTL_FLASH_TX_AVAIL to avoid host puts a
	 * GET_FLASH_C command at here.
	 */
	tmp &= NPCX_FLASHCTL_ACCESS_MASK;

	/* Release FLASH_NP_FREE */
	tmp |= BIT(NPCX_FLASHCTL_FLASH_NP_FREE);
	inst->FLASHCTL = tmp;
}

BUILD_ASSERT(sizeof(struct npcx_taf_head) == sizeof(uint32_t),
	     "struct npcx_taf_head size must be exactly 4 bytes");

static int taf_npcx_completion_handler(const struct device *dev, uint8_t type, uint8_t tag,
				       uint16_t len, uint32_t *buffer)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct npcx_taf_head taf_head;
	uint32_t header;

	taf_head.pkt_len = NPCX_TAF_CMP_HEADER_LEN + len;
	taf_head.type = type;
	taf_head.tag_hlen = (tag << 4) | ((len >> 8) & 0x0F);
	taf_head.llen = len & 0xFF;

	if (WAIT_FOR(!IS_BIT_SET(inst->FLASHCTL, NPCX_FLASHCTL_FLASH_TX_AVAIL),
		     NPCX_FLASH_CHK_TIMEOUT, NULL) == false) {
		LOG_ERR("Check TX Queue Is Empty Timeout");
		return -EBUSY;
	}

	memcpy(&header, &taf_head, sizeof(taf_head));
	inst->FLASHTXBUF[0] = header;

	if (CYC_TYPE_HAS_DATA(type) && len > 0) {
		uint16_t words = DIV_ROUND_UP(len, sizeof(uint32_t));
		uint16_t max_words =
			ARRAY_SIZE(((struct espi_reg *)0)->FLASHTXBUF) - NPCX_ESPI_FLASH_HEADER_LEN;

		if (words > max_words) {
			LOG_ERR("FLASHTXBUF overflow len=%u words=%u avail=%u", len, words,
				max_words);
			return -EOVERFLOW;
		}

		for (uint16_t i = 0; i < words; i++) {
			inst->FLASHTXBUF[i + NPCX_ESPI_FLASH_HEADER_LEN] = buffer[i];
		}
	}

	taf_set_flash_c_avail(dev);

	if (!CYC_TYPE_IS_CONTINUATION(type)) {
		taf_release_flash_np_free(dev);
	}

	return 0;
}

#if defined(CONFIG_ESPI_TAF_MANUAL_MODE)
static int espi_taf_npcx_flash_read(const struct device *dev, struct espi_taf_npcx_data *taf)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_taf_npcx_config *config = ((struct espi_taf_npcx_config *)(dev)->config);
	uint8_t cycle_type = CYC_SCS_CMP_WITH_DATA_ONLY;
	uint32_t total_len = taf->pckt.len;
	uint32_t len = total_len;
	uint32_t addr = taf->pckt.addr;
	uint8_t flash_req_size = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLASHREQSIZE);
	uint8_t target_max_size = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLREQSUP);
	uint16_t max_read_req = 32 << flash_req_size;
	uint8_t read_buf[64];
	int rc;

	if (flash_req_size > target_max_size) {
		LOG_DBG("Exceeded the maximum supported length");
		if (target_max_size == 0) {
			target_max_size = 1;
		}
		max_read_req = 32 << target_max_size;
	}

	if (total_len > max_read_req) {
		LOG_ERR("Exceeded the limitation of read length");
		return -EINVAL;
	}

	if (espi_taf_check_read_protect(dev, addr, len, taf->pckt.tag)) {
		LOG_ERR("Access protect region");
		return -EINVAL;
	}

	if (total_len <= config->rx_plsz) {
		cycle_type = CYC_SCS_CMP_WITH_DATA_ONLY;
		len = total_len;
	} else {
		cycle_type = CYC_SCS_CMP_WITH_DATA_FIRST;
		len = config->rx_plsz;
	}

	do {
		rc = flash_read(spi_dev, addr, &read_buf[0], len);
		if (rc) {
			LOG_ERR("flash read fail 0x%x", rc);
			return -EIO;
		}

		rc = taf_npcx_completion_handler(dev, cycle_type, taf->pckt.tag, len,
						 (uint32_t *)&read_buf[0]);
		if (rc) {
			LOG_ERR("espi taf completion handler fail");
			return rc;
		}

		total_len -= len;
		addr += len;

		if (total_len <= config->rx_plsz) {
			cycle_type = CYC_SCS_CMP_WITH_DATA_LAST;
			len = total_len;
		} else {
			cycle_type = CYC_SCS_CMP_WITH_DATA_MIDDLE;
		}
	} while (total_len);

	return 0;
}
#endif

static int espi_taf_npcx_flash_write(const struct device *dev, struct espi_taf_npcx_data *taf)
{
	uint8_t *data_ptr = (uint8_t *)taf->pckt.src;
	uint32_t addr = taf->pckt.addr;
	uint32_t len = taf->pckt.len;
	int rc;

	if (espi_taf_check_write_protect(dev, addr, len, taf->pckt.tag)) {
		LOG_ERR("Access protection region");
		return -EINVAL;
	}

	rc = flash_write(spi_dev, addr, data_ptr, len);
	if (rc) {
		LOG_ERR("flash write fail 0x%x", rc);
		return -EIO;
	}

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITHOUT_DATA, taf->pckt.tag, 0x0, NULL);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static int espi_taf_npcx_flash_erase(const struct device *dev, struct espi_taf_npcx_data *taf)
{
	int erase_blk[] = {KB(4), KB(32), KB(64), KB(128)};
	uint32_t addr = taf->pckt.addr;
	uint32_t len;
	int rc;

	if ((taf->pckt.len < 0) || (taf->pckt.len >= NPCX_ESPI_TAF_ERASE_LEN_MAX)) {
		LOG_ERR("Invalid erase block size");
		return -EINVAL;
	}

	len = erase_blk[taf->pckt.len];

	if (espi_taf_check_write_protect(dev, addr, len, taf->pckt.tag)) {
		LOG_ERR("Access protection region");
		return -EINVAL;
	}

	rc = flash_erase(spi_dev, addr, len);
	if (rc) {
		LOG_ERR("flash erase fail");
		return -EIO;
	}

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITHOUT_DATA, taf->pckt.tag, 0x0, NULL);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static int espi_taf_npcx_flash_unsuccess(const struct device *dev, struct espi_taf_npcx_data *taf)
{
	int rc;

	rc = taf_npcx_completion_handler(dev, CYC_UNSCS_CMP_WITHOUT_DATA_ONLY, taf->pckt.tag,
					 0x0, NULL);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static void espi_taf_work(struct k_work *item)
{
	struct espi_taf_npcx_data *info = CONTAINER_OF(item, struct espi_taf_npcx_data, work);
	int ret = 0;

	if (info->pckt.invalid_flag == 1) {
		LOG_ERR("Invalid TAF packet");

		ret = espi_taf_npcx_flash_unsuccess(info->host_dev, info);

		return;
	}

	switch (info->pckt.type) {
#if defined(CONFIG_ESPI_TAF_MANUAL_MODE)
	case NPCX_ESPI_TAF_REQ_READ:
		ret = espi_taf_npcx_flash_read(info->host_dev, info);
		break;
#endif
	case NPCX_ESPI_TAF_REQ_ERASE:
		ret = espi_taf_npcx_flash_erase(info->host_dev, info);
		break;
	case NPCX_ESPI_TAF_REQ_WRITE:
		ret = espi_taf_npcx_flash_write(info->host_dev, info);
		break;
	default:
		LOG_ERR("Unsupported TAF cycle type %u", info->pckt.type);
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		ret = espi_taf_npcx_flash_unsuccess(info->host_dev, info);
	}
}

static void espi_taf_event_handler(const struct device *dev, struct espi_callback *cb,
				   struct espi_event event)
{
	if ((event.evt_type != ESPI_BUS_TAF_NOTIFICATION) ||
	    (event.evt_details != ESPI_CHANNEL_FLASH)) {
		return;
	}

	espi_taf_get_pckt(dev, &npcx_espi_taf_data, event);
	k_work_submit(&npcx_espi_taf_data.work);
}

int npcx_init_taf(const struct device *dev, sys_slist_t *callbacks)
{
	espi_init_callback(&espi_taf_cb, espi_taf_event_handler, ESPI_BUS_TAF_NOTIFICATION);
	espi_add_callback(dev, &espi_taf_cb);

	npcx_espi_taf_data.host_dev = dev;
	npcx_espi_taf_data.callbacks = callbacks;
	k_work_init(&npcx_espi_taf_data.work, espi_taf_work);

	return 0;
}

static int espi_taf_npcx_init(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_taf_npcx_config *config = ((struct espi_taf_npcx_config *)(dev)->config);

	SET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLCAPA,
		  NPCX_FLASH_SHARING_CAP_SUPP_TAF_AND_CAF);
	SET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_TRGFLEBLKSIZE,
		  BIT(config->erase_sz));
	SET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLREQSUP,
		  config->max_rd_sz);
	inst->FLASHBASE = config->mapped_addr;

	return 0;
}

static const struct espi_saf_driver_api espi_taf_npcx_driver_api = {
	.config = espi_taf_npcx_configure,
	.set_protection_regions = espi_taf_npcx_set_pr,
	.activate = espi_taf_npcx_activate,
	.get_channel_status = espi_taf_npcx_channel_ready,
};

static const struct espi_taf_npcx_config espi_taf_npcx_config = {
	.base = DT_INST_REG_ADDR(0),
	.mapped_addr = DT_INST_PROP(0, mapped_addr),
	.rx_plsz = DT_PROP(DT_INST_PARENT(0), rx_plsize),
	.erase_sz = DT_INST_STRING_TOKEN(0, erase_sz),
	.max_rd_sz = DT_INST_STRING_TOKEN(0, max_read_sz),
};

DEVICE_DT_INST_DEFINE(0, &espi_taf_npcx_init, NULL,
			&npcx_espi_taf_data, &espi_taf_npcx_config,
			PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
			&espi_taf_npcx_driver_api);
