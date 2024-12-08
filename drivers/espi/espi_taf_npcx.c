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
#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
#include <zephyr/drivers/flash/npcx_flash_api_ex.h>
#endif
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
#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
	uint8_t rpmc_cnt_num;
	uint8_t rpmc_op1_code;
#endif
};

#define MAX_TX_PAYLOAD_SIZE DT_PROP(DT_INST_PARENT(0), tx_plsize)

struct espi_taf_npcx_data {
	sys_slist_t *callbacks;
	const struct device *host_dev;
	uint8_t taf_type;
	uint8_t taf_tag;
	uint32_t address;
	uint16_t length;
	uint32_t src[16];
	uint8_t read_buf[MAX_TX_PAYLOAD_SIZE];
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

#define PRTR_BADDR(i) (							\
	GET_FIELD(inst->FLASH_PRTR_BADDR[i], NPCX_FLASH_PRTR_BADDR)	\
	<< GET_FIELD_POS(NPCX_FLASH_PRTR_BADDR))

#define PRTR_HADDR(i) (							\
	GET_FIELD(inst->FLASH_PRTR_HADDR[i], NPCX_FLASH_PRTR_HADDR)	\
	<< GET_FIELD_POS(NPCX_FLASH_PRTR_HADDR)) | 0xFFF;

static void espi_taf_get_pckt(const struct device *dev, struct espi_taf_npcx_data *pckt,
			      struct espi_event event)
{
	struct espi_taf_pckt *data_ptr;

	data_ptr = (struct espi_taf_pckt *)event.evt_data;

	pckt->taf_type = data_ptr->type;
	pckt->length = data_ptr->len;
	pckt->taf_tag = data_ptr->tag;
	pckt->address = data_ptr->addr;
	if ((data_ptr->type == NPCX_ESPI_TAF_REQ_WRITE) ||
	    (IS_ENABLED(CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT) &&
	     (data_ptr->type == NPCX_ESPI_TAF_REQ_RPMC_OP1))) {
		memcpy(pckt->src, data_ptr->src, sizeof(pckt->src));
	}
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

static int taf_npcx_completion_handler(const struct device *dev, uint8_t type, uint8_t tag,
				       uint16_t len, uint32_t *buffer)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct npcx_taf_head taf_head;
	uint16_t i, size;
	uint32_t tx_buf[16];

	taf_head.pkt_len = NPCX_TAF_CMP_HEADER_LEN + len;
	taf_head.type = type;
	taf_head.tag_hlen = (tag << 4) | ((len & 0xF00) >> 8);
	taf_head.llen = len & 0xFF;

	memcpy(&tx_buf[0], &taf_head, sizeof(struct npcx_taf_head));

	if (type == CYC_SCS_CMP_WITH_DATA_ONLY || type == CYC_SCS_CMP_WITH_DATA_FIRST ||
	    type == CYC_SCS_CMP_WITH_DATA_MIDDLE || type == CYC_SCS_CMP_WITH_DATA_LAST) {
		memcpy(&tx_buf[1], buffer, (uint8_t)(len));
	}

	/* Check the Flash Access TX Queue is empty by polling
	 * FLASH_TX_AVAIL.
	 */
	if (WAIT_FOR(!IS_BIT_SET(inst->FLASHCTL, NPCX_FLASHCTL_FLASH_TX_AVAIL),
		     NPCX_FLASH_CHK_TIMEOUT, NULL) == false) {
		LOG_ERR("Check TX Queue Is Empty Timeout");
		return -EBUSY;
	}

	/* Write packet to FLASHTXBUF */
	size = DIV_ROUND_UP((uint8_t)(tx_buf[0]) + 1, sizeof(uint32_t));
	for (i = 0; i < size; i++) {
		inst->FLASHTXBUF[i] = tx_buf[i];
	}

	/* Set the FLASHCTL.FLASH_TX_AVAIL bit to 1 to enqueue the packet */
	taf_set_flash_c_avail(dev);

	/* Release FLASH_NP_FREE here to ready get next TAF request */
	if ((type != CYC_SCS_CMP_WITH_DATA_FIRST) && (type != CYC_SCS_CMP_WITH_DATA_MIDDLE)) {
		taf_release_flash_np_free(dev);
	}

	return 0;
}

#if defined(CONFIG_ESPI_TAF_MANUAL_MODE)
static int espi_taf_npcx_flash_read(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_taf_npcx_config *config = ((struct espi_taf_npcx_config *)(dev)->config);
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t cycle_type = CYC_SCS_CMP_WITH_DATA_ONLY;
	uint32_t total_len = pckt->len;
	uint32_t len = total_len;
	uint32_t addr = pckt->flash_addr;
	uint8_t flash_req_size = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLASHREQSIZE);
	uint8_t target_max_size = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLREQSUP);
	uint16_t max_read_req = 32 << flash_req_size;
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
		rc = flash_read(spi_dev, addr, npcx_espi_taf_data.read_buf, len);
		if (rc) {
			LOG_ERR("flash read fail 0x%x", rc);
			return -EIO;
		}

		rc = taf_npcx_completion_handler(dev, cycle_type, taf_data_ptr->tag, len,
						 (uint32_t *)npcx_espi_taf_data.read_buf);
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

static int espi_taf_npcx_flash_write(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t *data_ptr = (uint8_t *)(taf_data_ptr->data);
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

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITHOUT_DATA, taf_data_ptr->tag, 0x0,
					 NULL);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static int espi_taf_npcx_flash_erase(const struct device *dev, struct espi_saf_packet *pckt)
{
	int erase_blk[] = {KB(4), KB(32), KB(64), KB(128)};
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint32_t addr = pckt->flash_addr;
	uint32_t len;
	int rc;

	if ((pckt->len < 0) || (pckt->len >= NPCX_ESPI_TAF_ERASE_LEN_MAX)) {
		LOG_ERR("Invalid erase block size");
		return -EINVAL;
	}

	len = erase_blk[pckt->len];

	if (espi_taf_check_write_protect(dev, addr, len, taf_data_ptr->tag)) {
		LOG_ERR("Access protection region");
		return -EINVAL;
	}

	rc = flash_erase(spi_dev, addr, len);
	if (rc) {
		LOG_ERR("flash erase fail");
		return -EIO;
	}

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITHOUT_DATA, taf_data_ptr->tag, 0x0,
					 NULL);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
static int espi_taf_npcx_rpmc_op1(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t *data_ptr = taf_data_ptr->data;
	struct npcx_ex_ops_uma_in op_in = {
		.opcode = ESPI_TAF_RPMC_OP1_CMD,
		.tx_buf = data_ptr + 1,
		.tx_count = (pckt->len) - 1,
		.rx_count = 0,
	};
	int rc;

	rc = flash_ex_op(spi_dev, FLASH_NPCX_EX_OP_EXEC_UMA, (uintptr_t)&op_in, NULL);
	if (rc) {
		LOG_ERR("flash RPMC OP1 fail");
		return -EIO;
	}

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITHOUT_DATA, taf_data_ptr->tag, 0x0,
					 NULL);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static int espi_taf_npcx_rpmc_op2(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	uint8_t dummy_byte = 0;
	struct npcx_ex_ops_uma_in op_in = {
		.opcode = ESPI_TAF_RPMC_OP2_CMD,
		.tx_buf = &dummy_byte,
		.tx_count = 1,
		.rx_count = pckt->len,
	};
	struct npcx_ex_ops_uma_out op_out = {
		.rx_buf = npcx_espi_taf_data.read_buf,
	};

	int rc;

	if (pckt->len > MAX_TX_PAYLOAD_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	rc = flash_ex_op(spi_dev, FLASH_NPCX_EX_OP_EXEC_UMA, (uintptr_t)&op_in, &op_out);
	if (rc) {
		LOG_ERR("flash RPMC OP2 fail");
		return -EIO;
	}

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITH_DATA_ONLY, taf_data_ptr->tag,
					 pckt->len, (uint32_t *)npcx_espi_taf_data.read_buf);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}
#endif

static int espi_taf_npcx_flash_unsuccess(const struct device *dev, struct espi_saf_packet *pckt)
{
	struct espi_taf_npcx_pckt *taf_data_ptr = (struct espi_taf_npcx_pckt *)pckt->buf;
	int rc;

	rc = taf_npcx_completion_handler(dev, CYC_UNSCS_CMP_WITHOUT_DATA_ONLY, taf_data_ptr->tag,
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

	struct espi_taf_npcx_pckt taf_data;
	struct espi_saf_packet pckt_taf;

	pckt_taf.flash_addr = info->address;
	pckt_taf.len = info->length;
	taf_data.tag = info->taf_tag;
	if ((info->taf_type == NPCX_ESPI_TAF_REQ_WRITE) ||
	    (IS_ENABLED(CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT) &&
	     (info->taf_type == NPCX_ESPI_TAF_REQ_RPMC_OP1))) {
		taf_data.data = (uint8_t *)info->src;
	} else {
		taf_data.data = NULL;
	}
	pckt_taf.buf = (uint8_t *)&taf_data;

	switch (info->taf_type) {
#if defined(CONFIG_ESPI_TAF_MANUAL_MODE)
	case NPCX_ESPI_TAF_REQ_READ:
		ret = espi_taf_npcx_flash_read(info->host_dev, &pckt_taf);
		break;
#endif
	case NPCX_ESPI_TAF_REQ_ERASE:
		ret = espi_taf_npcx_flash_erase(info->host_dev, &pckt_taf);
		break;
	case NPCX_ESPI_TAF_REQ_WRITE:
		ret = espi_taf_npcx_flash_write(info->host_dev, &pckt_taf);
		break;
#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
	case NPCX_ESPI_TAF_REQ_RPMC_OP1:
		ret = espi_taf_npcx_rpmc_op1(info->host_dev, &pckt_taf);
		break;
	case NPCX_ESPI_TAF_REQ_RPMC_OP2:
		ret = espi_taf_npcx_rpmc_op2(info->host_dev, &pckt_taf);
		break;
#endif
	}

	if (ret != 0) {
		ret = espi_taf_npcx_flash_unsuccess(info->host_dev, &pckt_taf);
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

#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
	uint8_t count_num = 0;

	/* RPMC_CFG1_CNTR is 0-based number, e.g. 0 indicates that 1 counter is supported, 1
	 * indicates 2 counters, etc.
	 */
	if (config->rpmc_cnt_num > 0) {
		count_num = config->rpmc_cnt_num - 1;
	}

	SET_FIELD(inst->FLASH_RPMC_CFG_1, NPCX_FLASH_RPMC_CFG1_CNTR, count_num);
	SET_FIELD(inst->FLASH_RPMC_CFG_1, NPCX_FLASH_RPMC_CFG1_OP1, config->rpmc_op1_code);
	SET_FIELD(inst->FLASH_RPMC_CFG_1, NPCX_FLASH_RPMC_CFG1_TRGRPMCSUP, config->rpmc_cnt_num);
#endif

	return 0;
}

static DEVICE_API(espi_saf, espi_taf_npcx_driver_api) = {
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
#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
	.rpmc_cnt_num = DT_INST_PROP(0, rpmc_cntr),
	.rpmc_op1_code = DT_INST_PROP(0, rpmc_op1_code),
#endif
};

DEVICE_DT_INST_DEFINE(0, &espi_taf_npcx_init, NULL,
			&npcx_espi_taf_data, &espi_taf_npcx_config,
			PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
			&espi_taf_npcx_driver_api);
