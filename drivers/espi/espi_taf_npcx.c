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
#include <zephyr/dt-bindings/flash_controller/npcx_fiu_qspi.h>
#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
#include <zephyr/drivers/flash/npcx_flash_api_ex.h>
#endif
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espi_taf, CONFIG_ESPI_LOG_LEVEL);

#define NPCX_TAF_PRIME_FLASH_NODE DT_ALIAS(taf_flash)
#define NPCX_TAF_SEC_FLASH_NODE   DT_ALIAS(taf_flash1)

#define NPCX_TAF_ALLOC_SIZE(node) (MB(1) << DT_ENUM_IDX(node, spi_dev_size))

static const struct device *const spi_dev = DEVICE_DT_GET(NPCX_TAF_PRIME_FLASH_NODE);
#if DT_NODE_HAS_STATUS_OKAY(NPCX_TAF_SEC_FLASH_NODE)
static const struct device *const spi_dev1 = DEVICE_DT_GET(NPCX_TAF_SEC_FLASH_NODE);
#endif

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
	struct espi_taf_pckt pckt;
	uint8_t read_buf[MAX_TX_PAYLOAD_SIZE];
	struct k_work work;
#if DT_NODE_HAS_STATUS_OKAY(NPCX_TAF_SEC_FLASH_NODE)
	const struct device *low_dev_ptr;
	const struct device *high_dev_ptr;
	uint32_t low_dev_size;
#endif
};

static struct espi_taf_npcx_data npcx_espi_taf_data;
static struct espi_callback espi_taf_cb;

#define HAL_INSTANCE(dev)						\
	((struct espi_reg *)((const struct espi_taf_npcx_config *)	\
	(dev)->config)->base)

#define PROT_FLBASE_ADDR (							\
	GET_FIELD(inst->FLASHBASE, NPCX_FLASH_PRTR_BADDR)		\
	<< GET_FIELD_POS(NPCX_FLASH_PRTR_BADDR))

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

#define DT_NODE_QUAD_PROP_OR(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, quad_enable_requirements),	\
		    (DT_PROP(node, quad_enable_requirements)),		\
		    (("NONE")))

#define RPMC_OP2_MAX_RETRY		3U
#define RPMC_OP2_BUSY_MASK		0x1U

static void espi_taf_get_pckt(const struct device *dev, struct espi_taf_npcx_data *data,
			      struct espi_event event)
{
	data->pckt = *(struct espi_taf_pckt *)event.evt_data;
}

#if defined(CONFIG_ESPI_NPCX_NPCKN_V1)
static void espi_taf_fiu_mode_set(void)
{
	struct fiu_reg *const inst = (struct fiu_reg *)DT_INST_REG_ADDR_BY_NAME(0, fiu1);

	if (strcmp(DT_NODE_QUAD_PROP_OR(NPCX_TAF_PRIME_FLASH_NODE), "NONE") != 0) {
		/* Set quad read for FIU1 */
		SET_FIELD(inst->SPI_FL_CFG, NPCX_SPI_FL_CFG_RD_MODE, NPCX_RD_MODE_FAST_DUAL);
		inst->RESP_CFG |= BIT(NPCX_RESP_CFG_QUAD_EN);
	}

	if (DT_PROP(NPCX_TAF_PRIME_FLASH_NODE, enter_4byte_addr) != 0) {
		int flags = DT_PROP(NPCX_TAF_PRIME_FLASH_NODE, qspi_flags);

		/* Enable 4 byte address mode for FIU1 */
		if ((flags & NPCX_QSPI_SHD_FLASH_SL) != 0) {
			inst->FIU_4B_EN |= BIT(NPCX_MSR_FIU_4B_EN_SHD_4B);
		} else if ((flags & NPCX_QSPI_PVT_FLASH_SL) != 0) {
			inst->FIU_4B_EN |= BIT(NPCX_MSR_FIU_4B_EN_PVT_4B);
		} else if ((flags & NPCX_QSPI_BKP_FLASH_SL) != 0) {
			inst->FIU_4B_EN |= BIT(NPCX_MSR_FIU_4B_EN_BKP_4B);
		} else {
			LOG_ERR("No valid flash selected");
		}
	}
}
#endif

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

	flash_addr += PROT_FLBASE_ADDR;

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

	flash_addr += PROT_FLBASE_ADDR;

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
	const struct espi_saf_pr *preg;
	size_t n;
	uint8_t regnum;
	uint16_t offset;
	uint32_t bitmask, rw_pr, override_rw;

	if (pr == NULL) {
		return -EINVAL;
	}

	if (pr->nregions >= CONFIG_ESPI_TAF_PR_NUM) {
		return -EINVAL;
	}

	preg = pr->pregions;
	n = pr->nregions;

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
							 | rw_pr | PROT_FLBASE_ADDR;
			bitmask = BIT_MASK(GET_FIELD_SZ(NPCX_FLASH_PRTR_HADDR));
			offset = GET_FIELD_POS(NPCX_FLASH_PRTR_HADDR);
			inst->FLASH_PRTR_HADDR[regnum] = (preg->end & bitmask) << offset
							 | PROT_FLBASE_ADDR;
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

#ifdef CONFIG_ESPI_NPCX_NPCKN_V1
	inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_SAF_PROT_LOCK);
#else
	inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_AUTO_RD_DIS_CTL);
	inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_BLK_FLASH_NP_FREE);
#endif

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
#if DT_NODE_HAS_STATUS_OKAY(NPCX_TAF_SEC_FLASH_NODE)
	if (!device_is_ready(spi_dev1)) {
		return false;
	}
#endif

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

	/* Set FLASHCTL_FLASH_ACC_TX_AVAIL */
	tmp |= BIT(NPCX_FLASHCTL_FLASH_ACC_TX_AVAIL);
	inst->FLASHCTL = tmp;
}

/* This routine release FLASH_NP_FREE for standard request */
static void taf_release_flash_np_free(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t tmp;

	if (WAIT_FOR(!IS_BIT_SET(inst->FLASHCTL, NPCX_FLASHCTL_FLASH_ACC_TX_AVAIL),
		CONFIG_ESPI_TAF_TX_AVAIL_CHECK_TIME, NULL) == false) {
		LOG_ERR("Flash_ACC_TX_AVAIL is not cleared");
	}

	tmp = inst->FLASHCTL;
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

	if (WAIT_FOR(!IS_BIT_SET(inst->FLASHCTL, NPCX_FLASHCTL_FLASH_ACC_TX_AVAIL),
		     CONFIG_ESPI_TAF_TX_AVAIL_CHECK_TIME, NULL) == false) {
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
	uint8_t cycle_type = CYC_SCS_CMP_WITH_DATA_ONLY;
	uint32_t total_len = taf->pckt.len;
	uint32_t len = total_len;
	uint32_t addr = taf->pckt.addr;
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

	if (espi_taf_check_read_protect(dev, addr, len, taf->pckt.tag)) {
		LOG_ERR("Access protect region");
		return -EINVAL;
	}

	if (total_len <= MAX_TX_PAYLOAD_SIZE) {
		cycle_type = CYC_SCS_CMP_WITH_DATA_ONLY;
		len = total_len;
	} else {
		cycle_type = CYC_SCS_CMP_WITH_DATA_FIRST;
		len = MAX_TX_PAYLOAD_SIZE;
	}

	do {
#if DT_NODE_HAS_STATUS_OKAY(NPCX_TAF_SEC_FLASH_NODE)
		if ((addr + len) <= taf->low_dev_size) {
			rc = flash_read(taf->low_dev_ptr, addr, taf->read_buf, len);
		} else if (addr >= taf->low_dev_size) {
			rc = flash_read(taf->high_dev_ptr,
					(addr - taf->low_dev_size), taf->read_buf, len);
		} else {
			rc = flash_read(taf->low_dev_ptr, addr, taf->read_buf,
					(taf->low_dev_size - addr));

			if (rc) {
				LOG_ERR("flash read fail 0x%x", rc);
				return -EIO;
			}

			uint32_t index = taf->low_dev_size - addr;

			rc = flash_read(taf->high_dev_ptr, 0x0, &taf->read_buf[index],
					(addr + len - taf->low_dev_size));
		}
#else
		rc = flash_read(spi_dev, addr, taf->read_buf, len);
#endif
		if (rc) {
			LOG_ERR("flash read fail 0x%x", rc);
			return -EIO;
		}

		rc = taf_npcx_completion_handler(dev, cycle_type, taf->pckt.tag, len,
						 (uint32_t *)taf->read_buf);
		if (rc) {
			LOG_ERR("espi taf completion handler fail");
			return rc;
		}

		total_len -= len;
		addr += len;

		if (total_len <= MAX_TX_PAYLOAD_SIZE) {
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

#if DT_NODE_HAS_STATUS_OKAY(NPCX_TAF_SEC_FLASH_NODE)
	if ((addr + len) <= taf->low_dev_size) {
		rc = flash_write(taf->low_dev_ptr, addr, data_ptr, len);
	} else if (addr >= taf->low_dev_size) {
		rc = flash_write(taf->high_dev_ptr, (addr - taf->low_dev_size), data_ptr, len);
	} else {
		LOG_ERR("Write across two flashes");
		return -EINVAL;
	}
#else
	rc = flash_write(spi_dev, addr, data_ptr, len);
#endif
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

#if DT_NODE_HAS_STATUS_OKAY(NPCX_TAF_SEC_FLASH_NODE)
	if ((addr + len) <= taf->low_dev_size) {
		rc = flash_erase(taf->low_dev_ptr, addr, len);
	} else if (addr >= taf->low_dev_size) {
		rc = flash_erase(taf->high_dev_ptr, (addr - taf->low_dev_size), len);
	} else {
		LOG_ERR("Erase across two flashes");
		return -EINVAL;
	}
#else
	rc = flash_erase(spi_dev, addr, len);
#endif
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

#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
static int espi_taf_npcx_rpmc_op1(const struct device *dev, struct espi_taf_npcx_data *taf)
{
	uint8_t *data_ptr = (uint8_t *)taf->pckt.src;
	struct npcx_ex_ops_uma_in op_in = {
		.opcode = ESPI_TAF_RPMC_OP1_CMD,
		.tx_buf = data_ptr + 1,
		.tx_count = taf->pckt.len - 1,
		.rx_count = 0,
	};
	int rc;

	rc = flash_ex_op(spi_dev, FLASH_NPCX_EX_OP_EXEC_UMA, (uintptr_t)&op_in, NULL);
	if (rc) {
		LOG_ERR("flash RPMC OP1 fail");
		return -EIO;
	}

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITHOUT_DATA, taf->pckt.tag, 0x0, NULL);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}

static int espi_taf_npcx_rpmc_op2(const struct device *dev, struct espi_taf_npcx_data *taf)
{
	uint8_t dummy_byte = 0;
	uint8_t retry = RPMC_OP2_MAX_RETRY;
	uint8_t status;
	struct npcx_ex_ops_uma_in op_in = {
		.opcode = ESPI_TAF_RPMC_OP2_CMD,
		.tx_buf = &dummy_byte,
		.tx_count = 1,
		.rx_count = 1,
	};
	struct npcx_ex_ops_uma_out op_out = {
		.rx_buf = &status,
	};
	int rc;

	if (taf->pckt.len > MAX_TX_PAYLOAD_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	do {
		rc = flash_ex_op(spi_dev, FLASH_NPCX_EX_OP_EXEC_UMA, (uintptr_t)&op_in, &op_out);
		if (rc) {
			LOG_ERR("flash RPMC OP2 status read fail");
			return -EIO;
		}

		status = taf->read_buf[0];

		if (status & RPMC_OP2_BUSY_MASK) {
			LOG_DBG("RPMC OP2 status code: %x, in attempt %d", status,
				(RPMC_OP2_MAX_RETRY - retry + 1));
			retry--;
			continue;
		}

		/* Status is not busy */
		break;
	} while (retry > 0);

	if (status & RPMC_OP2_BUSY_MASK) {
		LOG_ERR("RPMC OP2 still busy after retries, status: %x", status);
		return -EBUSY;
	}

	op_in.rx_count = taf->pckt.len;
	op_out.rx_buf = taf->read_buf;
	rc = flash_ex_op(spi_dev, FLASH_NPCX_EX_OP_EXEC_UMA, (uintptr_t)&op_in, &op_out);
	if (rc) {
		LOG_ERR("flash RPMC OP2 data read fail");
		return -EIO;
	}

	rc = taf_npcx_completion_handler(dev, CYC_SCS_CMP_WITH_DATA_ONLY, taf->pckt.tag,
					 taf->pckt.len, (uint32_t *)taf->read_buf);
	if (rc) {
		LOG_ERR("espi taf completion handler fail");
		return rc;
	}

	return 0;
}
#endif

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
#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
	case NPCX_ESPI_TAF_REQ_RPMC_OP1:
		ret = espi_taf_npcx_rpmc_op1(info->host_dev, info);
		break;
	case NPCX_ESPI_TAF_REQ_RPMC_OP2:
		ret = espi_taf_npcx_rpmc_op2(info->host_dev, info);
		break;
#endif
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

int espi_taf_npcx_block(const struct device *dev, bool en_block)
{
#ifdef CONFIG_ESPI_NPCX_NPCXN_V3
	struct espi_reg *const inst = HAL_INSTANCE(dev);

	if (!IS_BIT_SET(inst->FLASHCTL, NPCX_FLASHCTL_SAF_AUTO_READ)) {
		return 0;
	}

	if (en_block) {
		if (WAIT_FOR(!IS_BIT_SET(inst->ESPISTS, NPCX_ESPISTS_FLAUTORDREQ),
			     CONFIG_ESPI_TAF_NPCX_STS_AWAIT_TIMEOUT, NULL) == false) {
			LOG_ERR("Check Automatic Read Queue Empty Timeout");
			return -ETIMEDOUT;
		}

		inst->FLASHCTL |= BIT(NPCX_FLASHCTL_AUTO_RD_DIS_CTL);

		if (WAIT_FOR(IS_BIT_SET(inst->ESPISTS, NPCX_ESPISTS_AUTO_RD_DIS_STS),
			     CONFIG_ESPI_TAF_NPCX_STS_AWAIT_TIMEOUT, NULL) == false) {
			inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_AUTO_RD_DIS_CTL);
			inst->ESPISTS |= BIT(NPCX_ESPISTS_AUTO_RD_DIS_STS);
			LOG_ERR("Check Automatic Read Disable Timeout");
			return -ETIMEDOUT;
		}
	} else {
		inst->FLASHCTL &= ~BIT(NPCX_FLASHCTL_AUTO_RD_DIS_CTL);
		inst->ESPISTS |= BIT(NPCX_ESPISTS_AUTO_RD_DIS_STS);
	}
#endif
	return 0;
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

#if DT_NODE_HAS_STATUS_OKAY(NPCX_TAF_SEC_FLASH_NODE)
	if (IS_ENABLED(CONFIG_FLASH_NPCX_FIU_SUPP_LOW_DEV_SWAP)) {
		npcx_espi_taf_data.low_dev_ptr = spi_dev1;
		npcx_espi_taf_data.high_dev_ptr = spi_dev;
		npcx_espi_taf_data.low_dev_size = NPCX_TAF_ALLOC_SIZE(NPCX_TAF_SEC_FLASH_NODE);
	} else {
		npcx_espi_taf_data.low_dev_ptr = spi_dev;
		npcx_espi_taf_data.high_dev_ptr = spi_dev1;
		npcx_espi_taf_data.low_dev_size = NPCX_TAF_ALLOC_SIZE(NPCX_TAF_PRIME_FLASH_NODE);
	}
#endif

#ifdef CONFIG_ESPI_TAF_NPCX_RPMC_SUPPORT
	uint8_t count_num = 0;

	/* RPMC_CFG1_CNTR is 0-based number, e.g. 0 indicates that 1 counter is supported, 1
	 * indicates 2 counters, etc.
	 */
	if (config->rpmc_cnt_num > 0) {
		count_num = config->rpmc_cnt_num - 1;
	}

#if defined(CONFIG_ESPI_NPCX_NPCXN_V3)
	SET_FIELD(inst->FLASH_RPMC_CFG_1, NPCX_FLASH_RPMC_CFG1_CNTR, count_num);
	SET_FIELD(inst->FLASH_RPMC_CFG_1, NPCX_FLASH_RPMC_CFG1_OP1, config->rpmc_op1_code);
	SET_FIELD(inst->FLASH_RPMC_CFG_1, NPCX_FLASH_RPMC_CFG1_TRGRPMCSUP, config->rpmc_cnt_num);
#elif defined(CONFIG_ESPI_NPCX_NPCKN_V1)
	SET_FIELD(inst->FLASHCFG2, NPCX_FLASHCFG2_RPMC1COUNT, count_num);
	SET_FIELD(inst->FLASHCFG2, NPCX_FLASHCFG2_RPMC1OP1CODE, config->rpmc_op1_code);
	SET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_TRGRPMCSUPP, config->rpmc_cnt_num);
#endif
#endif

#if defined(CONFIG_ESPI_NPCX_NPCKN_V1)
	espi_taf_fiu_mode_set();
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
