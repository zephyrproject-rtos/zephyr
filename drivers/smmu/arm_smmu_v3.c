
/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "arm_smmu_v3.h"
#include "smmu_page_map.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/arm64/mm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arm_smmu_v3, LOG_LEVEL_DBG);

#define _INTERNAL_DEBUG

#define DT_DRV_COMPAT arm_smmu_v3

#define QUEUE_SIZE 8

#define STRTAB_SPLIT       8
#define STRTAB_L1_SZ_SHIFT 20

#define STRTAB_L1_DESC_DWORDS 1
#define STRTAB_STE_DWORDS     8
#define CD_DWORDS             8
#define CMDQ_ENTRY_DWORDS     2
#define EVTQ_ENTRY_DWORDS     4

#define Q_IDX(q, p) ((p) & ((1 << (q)->size_log2) - 1))
#define Q_WRP(q, p) ((p) & (1 << (q)->size_log2))
#define Q_OVF(p)    ((p) & (1 << 31))

struct smmu_device_config {
	DEVICE_MMIO_ROM;
};

struct smmu_device_data {
	DEVICE_MMIO_RAM;
	struct smmu_strtab strtab;
	struct smmu_queue cmdq;
	struct smmu_queue evtq;
	struct smmu_domain default_domain; /* TODO: Domain should be create by domain alloc */
	uint16_t sid_bits;
	uint16_t oas; /* Physical address size */
	uint16_t vas; /* Virtual address size */
	uint32_t features;
#define SMMU_FEAT_2_LVL_STREAM_TABLE BIT(0)
#define SMMU_FEAT_STALL_FORCE        BIT(9)
#define SMMU_FEAT_STALL              BIT(10)
};

SYS_MEM_BLOCKS_DEFINE(ctx_allocator, sizeof(struct smmu_ctx), CONFIG_SMMU_CTX_ALLOCATOR_SIZE, 4);

static void smmu_show_err_if_occur(struct smmu_device_data *data, int line)
{
	uint32_t gerror;

	gerror = sys_read32(data->_mmio + SMMU_GERROR);
	if (gerror) {
		LOG_WRN("-----%d----", line);
		LOG_WRN("SMMU_GERROR: 0x%x", sys_read32(data->_mmio + SMMU_GERROR));
		LOG_WRN("SMMU_CMDQ_CONS 0x%x", sys_read32(data->cmdq.cons_reg));
	}
}

static int smmu_write_ack(struct smmu_device_data *data, uint32_t reg, uint32_t reg_ack,
			  uint32_t val)
{
	uint32_t v;
	int timeout;

	timeout = 100000;

	sys_write32(val, data->_mmio + reg);

	do {
		v = sys_read32(data->_mmio + reg_ack);
		if (v == val) {
			break;
		}
	} while (timeout--);

	if (timeout <= 0) {
		LOG_ERR("Failed to write reg.");
		return -EACCES;
	}

	return 0;
}

static int smmu_q_has_space(struct smmu_queue *q)
{
	if (Q_IDX(q, q->lc.cons) != Q_IDX(q, q->lc.prod) ||
	    Q_WRP(q, q->lc.cons) == Q_WRP(q, q->lc.prod)) {
		return 1;
	}
	return 0;
}

static uint32_t smmu_q_inc_prod(struct smmu_queue *q)
{
	uint32_t prod;
	uint32_t val;

	prod = (Q_WRP(q, q->lc.prod) | Q_IDX(q, q->lc.prod)) + 1;
	val = (Q_OVF(q->lc.prod) | Q_WRP(q, prod) | Q_IDX(q, prod));

	return val;
}

static void make_cmd(uint64_t *cmd, struct smmu_cmdq_entry *entry)
{
	memset(cmd, 0, CMDQ_ENTRY_DWORDS * 8);
	cmd[0] = entry->opcode;

	switch (entry->opcode) {
	case CMD_TLBI_NH_VA:
		cmd[0] |= FIELD_PREP(TLBI_0_ASID_M, entry->tlbi.asid);
		cmd[1] = entry->tlbi.addr & TLBI_1_ADDR_M;
		if (entry->tlbi.leaf) {
			/*
			 * Leaf flag means that only cached entries
			 * for the last level of translation table walk
			 * are required to be invalidated.
			 */
			cmd[1] |= TLBI_1_LEAF;
		}
		break;
	case CMD_TLBI_NH_ASID:
		cmd[0] |= FIELD_PREP(TLBI_0_ASID_M, (uint64_t)entry->tlbi.asid);
		break;
	case CMD_TLBI_NSNH_ALL:
	case CMD_TLBI_EL2_ALL:
		break;
	case CMD_CFGI_CD:
		cmd[0] |= FIELD_PREP(CFGI_0_SSID_M, (uint64_t)entry->cfgi.ssid);
		/* FALLTROUGH */
	case CMD_CFGI_STE:
		cmd[0] |= FIELD_PREP(CFGI_0_STE_SID_M, (uint64_t)entry->cfgi.sid);
		cmd[1] |= (uint64_t)(entry->cfgi.leaf) & CFGI_1_LEAF;
		break;
	case CMD_CFGI_STE_RANGE:
		cmd[1] = FIELD_PREP(CFGI_1_STE_RANGE_M, 31);
		break;
	case CMD_SYNC:
		cmd[0] |= FIELD_PREP(SYNC_0_MSH_M, SYNC_0_MSH_IS);
		cmd[0] |= FIELD_PREP(SYNC_0_MSIATTR_M, SYNC_0_MSIATTR_OIWB);
		if (entry->sync.msiaddr) {
			cmd[0] |= FIELD_PREP(SYNC_0_CS_M, SYNC_0_CS_SIG_IRQ);
			cmd[1] |= FIELD_PREP(SYNC_1_MSIADDRESS_M, entry->sync.msiaddr);
		} else {
			cmd[0] |= FIELD_PREP(SYNC_0_CS_M, SYNC_0_CS_SIG_SEV);
		}
		break;
	case CMD_PREFETCH_CONFIG:
		cmd[0] |= FIELD_PREP(PREFETCH_0_SID_M, (uint64_t)entry->prefetch.sid);
		break;
	default:
		break;
	};
}

static void smmu_cmdq_enqueue_cmd(struct smmu_device_data *data, struct smmu_cmdq_entry *entry)
{
	uint64_t cmd[CMDQ_ENTRY_DWORDS];
	struct smmu_queue *cmdq;
	void *entry_addr;

	cmdq = &data->cmdq;
	make_cmd(cmd, entry);

	do {
		cmdq->lc.cons = sys_read32(cmdq->cons_reg);
	} while (smmu_q_has_space(cmdq) == 0);

	entry_addr =
		(void *)((uint64_t)cmdq->base + Q_IDX(cmdq, cmdq->lc.prod) * CMDQ_ENTRY_DWORDS * 8);
	memcpy(entry_addr, cmd, CMDQ_ENTRY_DWORDS * 8);

	cmdq->lc.prod = smmu_q_inc_prod(cmdq);
	sys_write32(cmdq->lc.prod, cmdq->prod_reg);
}

static int smmu_sync(struct smmu_device_data *data)
{
	struct smmu_cmdq_entry cmd;
	struct smmu_queue *q;
	int timeout;
	int prod;

	q = &data->cmdq;
	prod = q->lc.prod;

	cmd.opcode = CMD_SYNC;
	cmd.sync.msiaddr = 0;
	smmu_cmdq_enqueue_cmd(data, &cmd);

	timeout = 10000;

	do {
		q->lc.cons = sys_read32(q->cons_reg);
		if (FIELD_GET(CMDQ_CONS_RD_M, q->lc.cons) == q->lc.prod) {
			break;
		}
		wfe();
	} while (timeout--);

	smmu_show_err_if_occur(data, __LINE__);

	if (timeout < 0) {
		LOG_WRN("Failed to sync");
	}

	return 0;
}

/*
static int smmu_sync_cd(struct smmu_device_data *data, int sid, int ssid, bool leaf)
{
	struct smmu_cmdq_entry cmd;

	cmd.opcode = CMD_CFGI_CD;
	cmd.cfgi.sid = sid;
	cmd.cfgi.ssid = ssid;
	cmd.cfgi.leaf = leaf;
	smmu_cmdq_enqueue_cmd(data, &cmd);

	return 0;
}
*/

static void smmu_invalidate_sid(struct smmu_device_data *data, uint32_t sid)
{
	struct smmu_cmdq_entry cmd;

	cmd.opcode = CMD_CFGI_STE;
	cmd.cfgi.sid = sid;
	cmd.cfgi.leaf = true;
	smmu_cmdq_enqueue_cmd(data, &cmd);
	smmu_sync(data);
}

static void smmu_prefetch_sid(struct smmu_device_data *data, uint32_t sid)
{
	struct smmu_cmdq_entry cmd;

	cmd.opcode = CMD_PREFETCH_CONFIG;
	cmd.prefetch.sid = sid;
	smmu_cmdq_enqueue_cmd(data, &cmd);
	smmu_sync(data);
}

static void smmu_invalidate_all_sid(struct smmu_device_data *data)
{
	struct smmu_cmdq_entry cmd;

	cmd.opcode = CMD_CFGI_STE_RANGE;
	smmu_cmdq_enqueue_cmd(data, &cmd);
	smmu_sync(data);
}

static void smmu_tlbi_va(struct smmu_device_data *data, mem_addr_t va, uint16_t asid)
{
	struct smmu_cmdq_entry cmd;

	/* Invalidate specific range */
	cmd.opcode = CMD_TLBI_NH_VA;
	cmd.tlbi.asid = asid;
	cmd.tlbi.vmid = 0;
	cmd.tlbi.leaf = true; /* We change only L3. */
	cmd.tlbi.addr = va;
	smmu_cmdq_enqueue_cmd(data, &cmd);
}

static void smmu_tlbi_all(struct smmu_device_data *data)
{
	struct smmu_cmdq_entry cmd;

	cmd.opcode = CMD_TLBI_NSNH_ALL;
	smmu_cmdq_enqueue_cmd(data, &cmd);
	smmu_sync(data);
}

/*! TODO: Currently we don't need this function.
 *  \todo Currently we don't need this function.
 */
static __attribute__((unused)) void smmu_tlbi_asid(struct smmu_device_data *data, uint16_t asid)
{
	struct smmu_cmdq_entry cmd;

	cmd.opcode = CMD_TLBI_NH_ASID;
	cmd.tlbi.asid = asid;
	smmu_cmdq_enqueue_cmd(data, &cmd);
	smmu_sync(data);
}

static int smmu_init_queue(struct smmu_device_data *data, struct smmu_queue *q, uint32_t prod_off,
			   uint32_t cons_off, uint32_t dwords)
{
	/*! TODO: queue size_log2 should be ensure that won't greater than IDRQ_CMDQ_MAX_SZ_SHIFT
	 *  \todo queue size_log2 should be ensure that won't greater than IDRQ_CMDQ_MAX_SZ_SHIFT
	 */
	int size_log2 = ilog2(QUEUE_SIZE);
	int sz = (1 << size_log2) * dwords * 8;
	q->size_log2 = size_log2;

	/*! TODO: alignment is dynamic, related to QUEUE SIZE.
	 *  \todo alignment is dynamic, related to QUEUE SIZE.
	 */
	q->base = k_aligned_alloc(SMMU_Q_ALIGN, sz);
	if (q->base == NULL) {
		return -ENOMEM;
	}

	q->base_dma = (mem_addr_t)q->base;

	q->prod_reg = data->_mmio + prod_off;
	q->cons_reg = data->_mmio + cons_off;

	q->q_base = CMDQ_BASE_RA | EVENTQ_BASE_WA;
	q->q_base |= q->base_dma & Q_BASE_ADDR_M;
	q->q_base |= q->size_log2 & Q_LOG2SIZE_M;

	return 0;
}

static int smmu_init_queues(struct smmu_device_data *data)
{
	int err;
	struct smmu_queue *cmdq = &data->cmdq;
	struct smmu_queue *evtq = &data->evtq;

	err = smmu_init_queue(data, cmdq, SMMU_CMDQ_PROD, SMMU_CMDQ_CONS, CMDQ_ENTRY_DWORDS);
	if (err) {
		return err;
	}

	err = smmu_init_queue(data, evtq, SMMU_EVENTQ_PROD, SMMU_EVENTQ_CONS, EVTQ_ENTRY_DWORDS);
	if (err) {
		return err;
	}

	return 0;
}

static int smmu_init_strtab_2lv1(struct smmu_device_data *data)
{
	struct smmu_strtab *strtab = &data->strtab;
	uint32_t size;
	uint32_t l1size;

	/*! TODO: don't understand
	 *  \todo don't understand, I still need look at it.
	 *  these code is port from FreeBSD
	 */
	size = STRTAB_L1_SZ_SHIFT - (ilog2(STRTAB_L1_DESC_DWORDS) + 3);
	size = MIN(size, data->sid_bits - STRTAB_SPLIT);
	strtab->num_l1_entries = (1 << size);

	l1size = strtab->num_l1_entries * (STRTAB_L1_DESC_DWORDS << 3);

	void *ptr = NULL;
	ptr = k_aligned_alloc(STRTAB_BASE_ALIGN, l1size);
	if (!ptr) {
		return -ENOMEM;
	}

	strtab->vaddr = (mem_addr_t)ptr;
	strtab->paddr = strtab->vaddr; /* here pa = va */
	strtab->base = strtab->paddr & STRTAB_BASE_ADDR_M;
	strtab->base |= STRTAB_BASE_RA;

	uint32_t val = 0;
	val = FIELD_PREP(STRTAB_BASE_CFG_LOG2SIZE_MASK, ilog2(strtab->num_l1_entries));
	val |= FIELD_PREP(STRTAB_BASE_CFG_SPLIT_MASK, STRTAB_SPLIT);
	val |= FIELD_PREP(STRTAB_BASE_CFG_FMT_MASK, STRTAB_BASE_CFG_FMT_2LVL);
	strtab->base_cfg = val;

	/* allocate l1_desc and reset it to 0 */
	strtab->l1 = k_calloc(strtab->num_l1_entries, sizeof(*strtab->l1));
	if (!strtab->l1) {
		// todo release strtab->strtab
		return -ENOMEM;
	}

	return 0;
}

static int smmu_init_strtab(struct smmu_device_data *data)
{
	int ret;

	if (data->features & SMMU_FEAT_2_LVL_STREAM_TABLE) {
		ret = smmu_init_strtab_2lv1(data);
	} else {
		LOG_ERR("Not support yet");
		ret = -ENOTSUP;
	}

	return ret;
}

static int smmu_init_l1_entry(struct smmu_device_data *data, uint32_t sid)
{
	mem_addr_t ptr;
	uint64_t *addr;
	struct smmu_strtab *strtab = &data->strtab;
	struct l1_desc *l1_desc;
	uint64_t val;
	size_t size;
	int i;

	l1_desc = &strtab->l1[sid >> STRTAB_SPLIT];
	size = 1 << (STRTAB_SPLIT + ilog2(STRTAB_STE_DWORDS) + 3);

	ptr = (mem_addr_t)k_aligned_alloc(STE_ALIGN, size);
	if (!ptr) {
		return -ENOMEM;
	}
	memset((void *)ptr, 0, size);

	l1_desc->l2va = (uint64_t *)ptr;
	l1_desc->l2pa = ptr;
	l1_desc->span = STRTAB_SPLIT + 1;

	i = sid >> STRTAB_SPLIT;
	addr = (void *)(strtab->vaddr + STRTAB_L1_DESC_DWORDS * 8 * i);

	/* write the l1 entry. */
	val = STRTAB_L1_DESC_L2PTR_M & l1_desc->l2pa;
	val |= FIELD_PREP(STRTAB_L1_DESC_SPAM, l1_desc->span);
	*addr = val;

	return 0;
}

static int smmu_check_features(struct smmu_device_data *data)
{
	uint32_t reg;

	data->features = 0;
	reg = sys_read32(data->_mmio + SMMU_IDR0);

	if (reg & IDR0_ST_LVL_2) {
		data->features |= SMMU_FEAT_2_LVL_STREAM_TABLE;
		LOG_INF("2-level stream table supported.");
	}

	switch (FIELD_GET(IDR0_STALL_MODEL_M, reg)) {
	case IDR0_STALL_MODEL_FORCE:
		data->features |= SMMU_FEAT_STALL_FORCE;
	case IDR0_STALL_MODEL_STALL:
		data->features |= SMMU_FEAT_STALL;
		break;
	}

	switch (FIELD_GET(IDR0_TTF_M, reg)) {
	case IDR0_TTF_ALL:
	case IDR0_TTF_AA64:
		data->vas = CONFIG_ARM64_VA_BITS;
		__ASSERT(data->vas > SMMU_L2_S , "Virtual address (%d) is unsupported", data->vas);
	}

	reg = sys_read32(data->_mmio + SMMU_IDR1);
	data->sid_bits = FIELD_GET(IDR1_SIDSIZE_M, reg);
	if (data->sid_bits <= STRTAB_SPLIT) {
		data->features &= ~SMMU_FEAT_2_LVL_STREAM_TABLE;
		LOG_INF("disable 2-level stream table feature.");
	}

	reg = sys_read32(data->_mmio + SMMU_IDR5);
	switch (FIELD_GET(IDR5_OAS_M, reg)) {
	case IDR5_OAS_32:
		data->oas = 32;
		break;
	case IDR5_OAS_36:
		data->oas = 36;
		break;
	case IDR5_OAS_40:
		data->oas = 40;
		break;
	case IDR5_OAS_42:
		data->oas = 42;
		break;
	case IDR5_OAS_44:
		data->oas = 44;
		break;
	case IDR5_OAS_48:
		data->oas = 47;
		break;
	case IDR5_OAS_52:
		data->oas = 52;
		break;
	}
	LOG_INF("Output address size: %d bits", data->oas);

	switch (FIELD_GET(IDR5_VAX_M, reg)) {
		case IDR5_VAX_48:
			__ASSERT(data->vas <= 48, "VA Size (%d) is out of range(48 bits)", data->vas);
			break;
		case IDR5_VAX_52:
			__ASSERT(data->vas <= 52, "VA Size (%d) is out of range(52 bits)", data->vas);
			break;
		default:
			LOG_ERR("Unknown VA range");
			break;
	}

	return 0;
}

/* TODO: Currently we only create a global domain */
static int smmu_init_default_domain(const struct device *dev)
{
	struct smmu_device_data *data = dev->data;
	struct smmu_domain *domain = &data->default_domain;
	struct iommu_domain *iodom = &domain->iodom;

	sys_slist_init(&domain->ctx_list);
	k_mutex_init(&domain->lock);

	iodom->dev = dev;
	domain->asid = 0; // Unused yet.
	page_map_init(&domain->pmap, data->vas);

	return 0;
}

/*! TODO: we use one context descriptor, I will add domain later.
 */
static int smmu_init_cd(struct smmu_device_data *data)
{
	size_t size;
	struct smmu_cd *cd;
	struct smmu_domain *domain = &data->default_domain;
	uint64_t *ptr;
	uint64_t val;
	mem_addr_t paddr;

	size = CD_DWORDS * 8;

	cd = domain->cd = k_calloc(1, sizeof(struct smmu_cd));
	if (!cd) {
		return -ENOMEM;
	}

	cd->vaddr = (mem_addr_t)k_aligned_alloc(CD_ALIGN, size);
	if (!cd->vaddr) {
		LOG_ERR("Failed to allocate CD.");
		k_free(cd);
		return -ENOMEM;
	}
	memset((void *)cd->vaddr, 0, size);

	cd->size = size;
	cd->paddr = cd->vaddr; // Assuming vaddr is equal to paddr in Zephyr

	ptr = (uint64_t *)cd->vaddr;

	val = CD0_VALID;
	val |= CD0_AA64;
	val |= CD0_R;
	val |= CD0_A;
	val |= CD0_ASET;
	val |= FIELD_PREP(CD0_ASID_M, domain->asid);
	val |= FIELD_PREP(CD0_TG0_M, CD0_TG0_4KB);
	val |= CD0_EPD1;
	val |= FIELD_PREP(CD0_T0SZ_M, 64 - data->vas);
	val |= CD0_IPS_32BITS;

	/*! TODO: get ttb from kernel, should get from domain later.
	 */
	mem_addr_t base_xlat;
#if 0
	extern struct arm_mmu_ptables *smmu_page_table;
	base_xlat = (mem_addr_t)smmu_page_table->base_xlat_table;
#else
	base_xlat = data->default_domain.pmap.paddr;
#endif
	LOG_DBG("CD->PTABLE: 0x%lx", base_xlat);
	paddr = FIELD_PREP(CD1_TTB0_M, base_xlat >> 4);

	ptr[1] = paddr;
	ptr[2] = 0;
	ptr[3] = MEMORY_ATTRIBUTES;

	ptr[0] = val;
	return 0;
}

static int smmu_init_ste_bypass(struct smmu_device_data *data, uint32_t sid, uint64_t *ste)
{
	uint64_t val;

	val = STE0_VALID | FIELD_PREP(STE0_CONFIG_M, STE0_CONFIG_BYPASS);

	ste[1] = FIELD_PREP(STE1_SHCFG_M, STE1_SHCFG_INCOMING) |
		 FIELD_PREP(STE1_EATS_M, STE1_EATS_FULLATS);
	ste[2] = 0;
	ste[3] = 0;
	ste[4] = 0;
	ste[5] = 0;
	ste[6] = 0;
	ste[7] = 0;

	smmu_invalidate_sid(data, sid);
	ste[0] = val;
	barrier_dsync_fence_full();
	smmu_invalidate_sid(data, sid);

	smmu_prefetch_sid(data, sid);

	return 0;
}

static int smmu_init_ste_s1(struct smmu_device_data *data, struct smmu_cd *cd, uint32_t sid,
			    uint64_t *ste)
{
	uint64_t val;

	val = STE0_VALID;

	ste[1] = FIELD_PREP(STE1_EATS_M, STE1_EATS_FULLATS) |
		 FIELD_PREP(STE1_S1CSH_M, STE1_S1CSH_IS) |
		 FIELD_PREP(STE1_S1CIR_M, STE1_S1CIR_WBRA) |
		 FIELD_PREP(STE1_S1COR_M, STE1_S1COR_WBRA) |
		 FIELD_PREP(STE1_STRW_M, STE1_STRW_NS_EL1);
	ste[2] = 0;
	ste[3] = 0;
	ste[4] = 0;
	ste[5] = 0;
	ste[6] = 0;
	ste[7] = 0;

	if (data->features & SMMU_FEAT_STALL && ((data->features & SMMU_FEAT_STALL_FORCE) == 0)) {

		ste[1] |= STE1_S1STALLD;
	}

	val |= FIELD_PREP(STE0_S1CONTEXTPTR_M, cd->paddr >> STE0_S1CONTEXTPTR_S);
	val |= FIELD_PREP(STE0_CONFIG_M, STE0_CONFIG_S1_TRANS);

	smmu_invalidate_sid(data, sid);

	ste[0] = val;
	barrier_dsync_fence_full();
	smmu_invalidate_sid(data, sid);
	smmu_prefetch_sid(data, sid);

	return 0;
}

static int smmu_init_ste(struct smmu_device_data *data, struct smmu_cd *cd, uint32_t sid,
			 bool bypass)
{
	struct smmu_strtab *strtab;
	struct l1_desc *l1_desc;
	uint64_t *addr;

	strtab = &data->strtab;

	if (data->features & SMMU_FEAT_2_LVL_STREAM_TABLE) {
		l1_desc = &strtab->l1[sid >> STRTAB_SPLIT];
		addr = l1_desc->l2va;
		addr += (sid & ((1 << STRTAB_SPLIT) - 1)) * STRTAB_STE_DWORDS;
	} else {
		LOG_ERR("Liner STREAM TABLE isn't supported yet.");
		return -ENOTSUP;
	}

	if (bypass) {
		smmu_init_ste_bypass(data, sid, addr);
	} else {
		smmu_init_ste_s1(data, cd, sid, addr);
	}

	smmu_sync(data);

	return 0;
}

static struct iommu_domain *smmu_domain_alloc(const struct device *dev)
{
	/* TODO: We always return default domain currently */
	struct smmu_device_data *data = dev->data;
	struct smmu_domain *domain;

	domain = &data->default_domain;

	return &domain->iodom;
}

static struct iommu_ctx *smmu_ctx_alloc(const struct device *dev, struct iommu_domain *iodom,
		const struct device *child, uint32_t sid, bool bypass)
{
	struct smmu_domain *domain;
	struct smmu_ctx *ctx;
	int ret;

	domain = (struct smmu_domain *)iodom;
	ret = sys_mem_blocks_alloc(&ctx_allocator, 1, (void **)&ctx);
	if (ret != 0) {
		LOG_ERR("Ran out of ctx_allocator");
		return NULL;
	}

	ctx->dev = child;
	ctx->domain = domain;
	ctx->sid = sid;
	ctx->bypass = (bypass == true) ? true : false;

	k_mutex_lock(&domain->lock, K_FOREVER);
	sys_slist_append(&domain->ctx_list, &ctx->next);
	k_mutex_unlock(&domain->lock);

	return (&ctx->ioctx);
}

static int smmu_ctx_init(const struct device *dev, struct iommu_ctx *ioctx)
{
	struct smmu_device_data *data = dev->data;
	struct smmu_ctx *ctx;
	struct smmu_domain *domain;
	struct iommu_domain *iodom;
	int ret;

	ctx = (struct smmu_ctx *)ioctx;
	domain = ctx->domain;
	iodom = &domain->iodom;

	if (data->features & SMMU_FEAT_2_LVL_STREAM_TABLE) {
		/*! TODO: should this be check whether l1 stream table entry is initialized.
		 */
		ret = smmu_init_l1_entry(data, ctx->sid);
		if (ret) {
			LOG_ERR("Failed to init l1 stream table entry");
			return ret;
		}
	} else {
		LOG_ERR("Liner stream table not supported yet.");
		return -ENOTSUP;
	}

	return smmu_init_ste(data, domain->cd, ctx->sid, ctx->bypass);
}

/*
static int smmu_ctx_free(const struct device *dev, struct iommu_ctx *ctx)
{
	LOG_ERR("smmu_ctx_free hasn't implemented yet.");
	return -ENOSYS;
}
*/

static int smmu_map(const struct device *dev, mem_addr_t va, mem_addr_t pa, int size,
		    uint32_t attrs)
{
	int i;
	int ret;
	struct smmu_device_data *data = dev->data;
	struct smmu_domain *domain = &data->default_domain;

	LOG_DBG("%lx -> %lx, %d", va, pa, size);
	for (i = 0; size > 0; size -= SMMU_PAGE_SIZE) {
		ret = page_map_smmu_add(&domain->pmap, va, pa, 0);
		if (ret) {
			return ret;
		}
		smmu_tlbi_va(data, va, domain->asid);
		va += SMMU_PAGE_SIZE;
		pa += SMMU_PAGE_SIZE;
	}

	smmu_sync(data);

	return 0;
}

static int smmu_unmap(const struct device *dev, mem_addr_t va, int size)
{
	int i;
	int ret;
	struct smmu_device_data *data = dev->data;
	struct smmu_domain *domain = &data->default_domain;

	LOG_DBG("UNMAP: %lx, %d", va, size);
	for (i = 0; i < size; i+= SMMU_PAGE_SIZE) {
		ret = page_map_smmu_remove(&domain->pmap, va);
		if (ret) {
			break;
		}
		smmu_tlbi_va(data, va, domain->asid);
		va += SMMU_PAGE_SIZE;
	}

	smmu_sync(data);
	return -ENOSYS;
}

static const struct iommu_driver_api smmu_driver_api = {
	.dev_map = smmu_map,
	.dev_unmap = smmu_unmap,
	.domain_alloc = smmu_domain_alloc,
	.ctx_alloc = smmu_ctx_alloc,
	.ctx_init = smmu_ctx_init,
};

static int smmu_init(const struct device *dev)
{
	const struct smmu_device_config *dev_cfg = dev->config;
	struct smmu_device_data *data = dev->data;
	uint64_t val;
	int ret;

	device_map(DEVICE_MMIO_RAM_PTR(dev), dev_cfg->_mmio.phys_addr, dev_cfg->_mmio.size,
		   K_MEM_CACHE_NONE);

	if (IS_ENABLED(CONFIG_SMMU_TYPE_GLOBAL_BYPASS)) {
		/* Clear SMMU_GBPA[ABORT] bit to enable global bypass*/
		sys_write32(0, data->_mmio + SMMU_GBPA);
		return 0;
	}

	ret = smmu_check_features(data);
	if (ret) {
		return ret;
	}

	ret = smmu_init_queues(data);
	if (ret) {
		return ret;
	}

	ret = smmu_init_strtab(data);
	if (ret) {
		return ret;
	}

	/*! TODO: we don't have domain yet, so we use a global smmu_cd
	 */
	ret = smmu_init_default_domain(dev);
	if (ret) {
		return ret;
	}
	ret = smmu_init_cd(data);
	if (ret) {
		return ret;
	}

	val = FIELD_PREP(CR1_TABLE_SH, CR1_TABLE_SH_IS);
	val |= FIELD_PREP(CR1_TABLE_OC, CR1_TABLE_OC_WBC);
	val |= FIELD_PREP(CR1_TABLE_IC, CR1_TABLE_IC_WBC);
	val |= FIELD_PREP(CR1_QUEUE_SH, CR1_QUEUE_SH_IS);
	val |= FIELD_PREP(CR1_QUEUE_OC, CR1_QUEUE_OC_WBC);
	val |= FIELD_PREP(CR1_QUEUE_IC, CR1_QUEUE_OC_WBC);
	sys_write32(val, data->_mmio + SMMU_CR1);

	val = CR2_PTM | CR2_RECINVSID | CR2_E2H;
	sys_write32(val, data->_mmio);

	sys_write64(data->cmdq.q_base, data->_mmio + SMMU_CMDQ_BASE);
	sys_write32(data->cmdq.lc.cons, data->_mmio + SMMU_CMDQ_CONS);
	sys_write32(data->cmdq.lc.prod, data->_mmio + SMMU_CMDQ_PROD);

	sys_write64(data->strtab.base, data->_mmio + SMMU_STRTAB_BASE);
	sys_write32(data->strtab.base_cfg, data->_mmio + SMMU_STRTAB_BASE_CFG);

	val = CR0_CMDQEN;
	ret = smmu_write_ack(data, SMMU_CR0, SMMU_CR0ACK, val);
	if (ret) {
		LOG_ERR("Could not enable command queue");
		return ret;
	}

	smmu_invalidate_all_sid(data);

	smmu_tlbi_all(data);

	smmu_show_err_if_occur(data, __LINE__);

	sys_write64(data->evtq.q_base, data->_mmio + SMMU_EVENTQ_BASE);
	sys_write32(data->evtq.lc.cons, data->_mmio + SMMU_EVENTQ_CONS);
	sys_write32(data->evtq.lc.prod, data->_mmio + SMMU_EVENTQ_PROD);

	val |= CR0_EVENTQEN;
	ret = smmu_write_ack(data, SMMU_CR0, SMMU_CR0ACK, val);
	if (ret) {
		LOG_ERR("Could not enable event queue");
		return ret;
	}

	val |= CR0_ATSCHK;
	ret = smmu_write_ack(data, SMMU_CR0, SMMU_CR0ACK, val);
	if (ret) {
		LOG_ERR("Could not enable ATS check");
		return ret;
	}

	val |= CR0_SMMUEN;
	ret = smmu_write_ack(data, SMMU_CR0, SMMU_CR0ACK, val);
	if (ret) {
		LOG_ERR("Cound not enable SMMU");
		return ret;
	}

	return 0;
}

static const struct smmu_device_config smmu_cfg_0 = {DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0))};

static struct smmu_device_data smmu_data_0 = {};

// clang-format off
DEVICE_DT_INST_DEFINE(0,
		      &smmu_init,
		      NULL,
		      &smmu_data_0,
		      &smmu_cfg_0,
		      PRE_KERNEL_2,
		      CONFIG_INTC_INIT_PRIORITY, /* TODO: PRIORITY */
		      &smmu_driver_api);
//clang-format on

#ifdef _INTERNAL_DEBUG

#include <stdio.h>
#include <zephyr/shell/shell.h>

void cmd_smmu_dump_cmdq(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(smmu));
	struct smmu_device_data *data = dev->data;
	struct smmu_queue *cmdq = &data->cmdq;

	int sz = ((1 << ilog2(QUEUE_SIZE)) * CMDQ_ENTRY_DWORDS * 8);
	shell_print(sh, "PROD points to 0x%x", cmdq->lc.prod * CMDQ_ENTRY_DWORDS * 8);
	shell_print(sh, "CONS points to 0x%x",
			(uint32_t)FIELD_GET(CMDQ_CONS_RD_M, cmdq->lc.cons) * CMDQ_ENTRY_DWORDS * 8);
	shell_hexdump(sh, cmdq->base, sz);
}

void cmd_smmu_dump_evtq(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(smmu));
	struct smmu_device_data *data = dev->data;
	struct smmu_queue *evtq = &data->evtq;

	int sz = ((1 << ilog2(QUEUE_SIZE)) * EVTQ_ENTRY_DWORDS * 8);
	shell_print(sh, "PROD points to 0x%x", evtq->lc.prod * EVTQ_ENTRY_DWORDS * 8);
	shell_print(sh, "CONS points to 0x%x",
			(uint32_t)FIELD_GET(EVENTQ_CONS_RD_M, evtq->lc.cons) * EVTQ_ENTRY_DWORDS * 8);
	shell_hexdump(sh, evtq->base, sz);
}

void cmd_smmu_dump_cd(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(smmu));
	struct smmu_device_data *data = dev->data;

	shell_hexdump(sh, (const uint8_t *)data->default_domain.cd->vaddr, CD_DWORDS * 8);
}

void cmd_smmu_map(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(smmu));
	mem_addr_t va, pa;
	int size;
	int ret;

	ret = sscanf(argv[1], "0x%lx", &va);
	if (ret != 1) {
		shell_print(sh, "Parse VA(%s) failed", argv[1]);
	}
	sscanf(argv[2], "0x%lx", &pa);
	if (ret != 1) {
		shell_print(sh, "Parse PA(%s) failed", argv[2]);
	}
	sscanf(argv[3], "%d", &size);
	if (ret != 1) {
		shell_print(sh, "Parse size(%s) failed", argv[3]);
	}

	smmu_map(dev, va, pa, size, 0);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dump,
		SHELL_CMD_ARG(cmdq, NULL, "Dump command queue", cmd_smmu_dump_cmdq, 1, 0),
		SHELL_CMD_ARG(evtq, NULL, "Dump event queue", cmd_smmu_dump_evtq, 1, 0),
		SHELL_CMD_ARG(cd, NULL, "Dump context descriptor", cmd_smmu_dump_cd, 1, 0),
		SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_smmu,
		SHELL_CMD_ARG(dump, &sub_dump, "Dump smmu internal data structure", NULL, 2, 0),
		SHELL_CMD_ARG(map, NULL,
			"SMMU mapping va to pa\n"
			"map <va> <pa> <size>", cmd_smmu_map, 4, 0),
		SHELL_SUBCMD_SET_END
		);


SHELL_CMD_REGISTER(smmu, &sub_smmu, "Utils command for debuging smmu driver", NULL);

#endif
