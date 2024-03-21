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

struct espi_taf_npcx_config {
	uintptr_t base;
	uintptr_t mapped_addr;
	uintptr_t rx_plsz;
	enum NPCX_ESPI_TAF_ERASE_BLOCK_SIZE erase_sz;
	enum NPCX_ESPI_TAF_MAX_READ_REQ max_rd_sz;
};

struct espi_taf_npcx_data {
	sys_slist_t callbacks;
};

#define HAL_INSTANCE(dev)						\
	((struct espi_reg *)((const struct espi_taf_npcx_config *)	\
	(dev)->config)->base)

#define FLBASE_ADDR (							\
	GET_FIELD(inst->FLASHBASE, NPCX_FLASHBASE_FLBASE_ADDR)		\
	<< GET_FIELD_POS(NPCX_FLASHBASE_FLBASE_ADDR))

#define PRTR_BADDR(i) (							\
	GET_FIELD(inst->FLASH_PRTR_BADDR[i], NPCX_FLASH_PRTR_BADDR)	\
	<< GET_FIELD_POS(NPCX_FLASH_PRTR_BADDR))

#define PRTR_HADDR(i) (							\
	GET_FIELD(inst->FLASH_PRTR_HADDR[i], NPCX_FLASH_PRTR_HADDR)	\
	<< GET_FIELD_POS(NPCX_FLASH_PRTR_HADDR)) | 0xFFF;

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

	if (!IS_BIT_SET(inst->ESPICFG, NPCX_ESPICFG_FLCHANMODE)) {
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

static int taf_npcx_completion_handler(const struct device *dev, uint32_t *buffer)
{
	uint16_t size = DIV_ROUND_UP((uint8_t)(buffer[0]) + 1, sizeof(uint32_t));
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct npcx_taf_head *head = (struct npcx_taf_head *)buffer;
	uint8_t i;

	/* Check the Flash Access TX Queue is empty by polling
	 * FLASH_TX_AVAIL.
	 */
	if (WAIT_FOR(IS_BIT_SET(inst->FLASHCTL, NPCX_FLASHCTL_FLASH_TX_AVAIL),
		     NPCX_FLASH_CHK_TIMEOUT, NULL)) {
		LOG_ERR("Check TX Queue Is Empty Timeout");
		return -EBUSY;
	}

	/* Check ESPISTS.FLNACS is clear (no slave completion is detected) */
	if (WAIT_FOR(IS_BIT_SET(inst->ESPISTS, NPCX_ESPISTS_FLNACS),
		     NPCX_FLASH_CHK_TIMEOUT, NULL)) {
		LOG_ERR("Check Slave Completion Timeout");
		return -EBUSY;
	}

	/* Write packet to FLASHTXBUF */
	for (i = 0; i < size; i++) {
		inst->FLASHTXBUF[i] = buffer[i];
	}

	/* Set the FLASHCTL.FLASH_TX_AVAIL bit to 1 to enqueue the packet */
	taf_set_flash_c_avail(dev);

	/* Release FLASH_NP_FREE here to ready get next TAF request */
	if ((head->type != CYC_SCS_CMP_WITH_DATA_FIRST) &&
	    (head->type != CYC_SCS_CMP_WITH_DATA_MIDDLE)) {
		taf_release_flash_np_free(dev);
	}

	return 0;
}

static int espi_taf_npcx_flash_read(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_taf_npcx_config *config = ((struct espi_taf_npcx_config *)(dev)->config);
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t *data_ptr = (uint8_t *)taf_data_ptr->data;
	uint8_t cycle_type = CYC_SCS_CMP_WITH_DATA_ONLY;
	uint32_t total_len = pckt->len;
	uint32_t len = total_len;
	uint32_t addr = pckt->flash_addr;
	uint8_t flash_req_size = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLASHREQSIZE);
	uint8_t target_max_size = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLREQSUP);
	uint16_t max_read_req = 32 << flash_req_size;
	struct npcx_taf_head taf_head;
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

	if (espi_taf_check_read_protect(dev, addr, len, taf_data_ptr->tag)) {
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
		data_ptr = (uint8_t *)taf_data_ptr->data;

		taf_head.pkt_len = len + NPCX_TAF_CMP_HEADER_LEN;
		taf_head.type = cycle_type;
		taf_head.tag_hlen = (taf_data_ptr->tag << 4) | ((len & 0xF00) >> 8);
		taf_head.llen = len & 0xFF;
		memcpy(data_ptr, &taf_head, sizeof(taf_head));

		rc = flash_read(spi_dev, addr, data_ptr + 4, len);
		if (rc) {
			LOG_ERR("flash read fail 0x%x", rc);
			return -EIO;
		}

		rc = taf_npcx_completion_handler(dev, (uint32_t *)taf_data_ptr->data);
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

static int espi_taf_npcx_flash_write(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t *data_ptr = (uint8_t *)(taf_data_ptr->data);
	struct npcx_taf_head taf_head;
	int rc;

	if (espi_taf_check_write_protect(dev, pckt->flash_addr,
					 pckt->len, taf_data_ptr->tag)) {
		LOG_ERR("Access protection region");
		return -EINVAL;
	}

	rc = flash_write(spi_dev, pckt->flash_addr, data_ptr, pckt->len);
	if (rc) {
		LOG_ERR("flash write fail 0x%x", rc);
		return -EIO;
	}

	taf_head.pkt_len = NPCX_TAF_CMP_HEADER_LEN;
	taf_head.type = CYC_SCS_CMP_WITHOUT_DATA;
	taf_head.tag_hlen = (taf_data_ptr->tag << 4);
	taf_head.llen = 0x0;
	memcpy(data_ptr, &taf_head, sizeof(taf_head));

	rc = taf_npcx_completion_handler(dev, (uint32_t *)taf_data_ptr->data);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static int espi_taf_npcx_flash_erase(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t *data_ptr = (uint8_t *)taf_data_ptr->data;
	uint32_t addr = pckt->flash_addr;
	uint32_t len = pckt->len;
	struct npcx_taf_head taf_head;
	int rc;

	if (espi_taf_check_write_protect(dev, addr, len, taf_data_ptr->tag)) {
		LOG_ERR("Access protection region");
		return -EINVAL;
	}

	rc = flash_erase(spi_dev, addr, len);
	if (rc) {
		LOG_ERR("flash erase fail");
		return -EIO;
	}

	taf_head.pkt_len = NPCX_TAF_CMP_HEADER_LEN;
	taf_head.type = CYC_SCS_CMP_WITHOUT_DATA;
	taf_head.tag_hlen = (taf_data_ptr->tag << 4);
	taf_head.llen = 0x0;
	memcpy(data_ptr, &taf_head, sizeof(taf_head));

	rc = taf_npcx_completion_handler(dev, (uint32_t *)taf_data_ptr->data);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static int espi_taf_npcx_flash_unsuccess(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_taf_npcx_pckt *taf_data_ptr
			= (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t *data_ptr = (uint8_t *)taf_data_ptr->data;
	struct npcx_taf_head taf_head;
	int rc;

	taf_head.pkt_len = NPCX_TAF_CMP_HEADER_LEN;
	taf_head.type = CYC_UNSCS_CMP_WITHOUT_DATA_ONLY;
	taf_head.tag_hlen = (taf_data_ptr->tag << 4);
	taf_head.llen = 0x0;
	memcpy(data_ptr, &taf_head, sizeof(taf_head));

	rc = taf_npcx_completion_handler(dev, (uint32_t *)taf_data_ptr->data);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

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
	.flash_read = espi_taf_npcx_flash_read,
	.flash_write = espi_taf_npcx_flash_write,
	.flash_erase = espi_taf_npcx_flash_erase,
	.flash_unsuccess = espi_taf_npcx_flash_unsuccess,
};

static struct espi_taf_npcx_data npcx_espi_taf_data;

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
