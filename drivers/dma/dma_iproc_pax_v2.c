/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_iproc_pax_dma_v2

#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <soc.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep.h>
#include "dma_iproc_pax_v2.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_iproc_pax_v2);

/* Driver runtime data for PAX DMA and RM */
static struct dma_iproc_pax_data pax_dma_data;

/**
 * @brief Opaque/packet id allocator, range 0 to 31
 */
static inline uint32_t reset_pkt_id(struct dma_iproc_pax_ring_data *ring)
{
	return ring->pkt_id = 0x0;
}

static inline uint32_t alloc_pkt_id(struct dma_iproc_pax_ring_data *ring)
{
	ring->pkt_id = (ring->pkt_id + 1) % 32;
	return ring->pkt_id;
}

static inline uint32_t curr_pkt_id(struct dma_iproc_pax_ring_data *ring)
{
	return ring->pkt_id;
}

static inline uint32_t curr_toggle_val(struct dma_iproc_pax_ring_data *ring)
{
	return ring->curr.toggle;
}

/**
 * @brief Populate header descriptor
 */
static inline void rm_write_header_desc(void *desc, uint32_t toggle,
				  uint32_t opq, uint32_t bdcount,
				  uint64_t pci_addr)
{
	struct rm_header *r = (struct rm_header *)desc;

	r->opq = opq;
	r->bdf = 0x0;
	r->res1 = 0x0;
	/* DMA descriptor count init value */
	r->bdcount = bdcount;
	r->prot = 0x0;
	r->res2 = 0x0;
	/* No packet extension, start and end set to '1' */
	r->start = 1;
	r->end = 1;
	/* RM header type */
	r->type = PAX_DMA_TYPE_RM_HEADER;
	r->pcie_addr_msb = PAX_DMA_PCI_ADDR_HI_MSB8(pci_addr);
	r->res3 = 0x0;
	r->res4 = 0x0;
#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	r->toggle = toggle;
#elif CONFIG_DMA_IPROC_PAX_DOORBELL_MODE
	r->toggle = 0;
#endif
}

/**
 * @brief Populate pcie descriptor
 */
static inline void rm_write_pcie_desc(void *desc,
			     uint32_t toggle,
			     uint64_t pci_addr)
{
	struct pcie_desc *pcie = (struct pcie_desc *)desc;

	pcie->pcie_addr_lsb = pci_addr;
	pcie->res1 = 0x0;
	/* PCIE header type */
	pcie->type = PAX_DMA_TYPE_PCIE_DESC;
#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	pcie->toggle = toggle;
#elif CONFIG_DMA_IPROC_PAX_DOORBELL_MODE
	pcie->toggle = 0;
#endif
}

/**
 * @brief Populate src/destination descriptor
 */
static inline void rm_write_src_dst_desc(void *desc_ptr,
				bool is_mega,
				uint32_t toggle,
				uint64_t axi_addr,
				uint32_t size,
				enum pax_dma_dir direction)
{
	struct src_dst_desc *desc;

	desc = (struct src_dst_desc *)desc_ptr;
	desc->axi_addr = axi_addr;
	desc->length = size;
#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	desc->toggle = toggle;
#elif CONFIG_DMA_IPROC_PAX_DOORBELL_MODE
	desc->toggle = 0;
#endif

	if (direction == CARD_TO_HOST) {
		desc->type = is_mega ?
			     PAX_DMA_TYPE_MEGA_SRC_DESC : PAX_DMA_TYPE_SRC_DESC;
	} else {
		desc->type = is_mega ?
			     PAX_DMA_TYPE_MEGA_DST_DESC : PAX_DMA_TYPE_DST_DESC;
	}
}

#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
static void init_toggle(void *desc, uint32_t toggle)
{
	struct rm_header *r = (struct rm_header *)desc;

	r->toggle = toggle;
}
#endif

/**
 * @brief Return current descriptor memory address and
 *        increment to point to next descriptor memory address.
 */
static inline void *get_curr_desc_addr(struct dma_iproc_pax_ring_data *ring)
{
	struct next_ptr_desc *nxt;
	uintptr_t curr;

	curr = (uintptr_t)ring->curr.write_ptr;
	/* if hit next table ptr, skip to next location, flip toggle */
	nxt = (struct next_ptr_desc *)curr;
	if (nxt->type == PAX_DMA_TYPE_NEXT_PTR) {
		LOG_DBG("hit next_ptr@0x%lx %d, next_table@0x%lx\n",
			curr, nxt->toggle, (uintptr_t)nxt->addr);
		uintptr_t last = (uintptr_t)ring->bd +
			     PAX_DMA_RM_DESC_RING_SIZE * PAX_DMA_NUM_BD_BUFFS;
		nxt->toggle = ring->curr.toggle;
		ring->curr.toggle = (ring->curr.toggle == 0) ? 1 : 0;
		/* move to next addr, wrap around if hits end */
		curr += PAX_DMA_RM_DESC_BDWIDTH;
		if (curr == last) {
			curr = (uintptr_t)ring->bd;
			LOG_DBG("hit end of desc:0x%lx, wrap to 0x%lx\n",
				last, curr);
		}
		ring->descs_inflight++;
	}

	ring->curr.write_ptr = (void *)(curr + PAX_DMA_RM_DESC_BDWIDTH);
	ring->descs_inflight++;

	return (void *)curr;
}

/**
 * @brief Populate next ptr descriptor
 */
static void rm_write_next_table_desc(void *desc, void *next_ptr,
				     uint32_t toggle)
{
	struct next_ptr_desc *nxt = (struct next_ptr_desc *)desc;

	nxt->addr = (uintptr_t)next_ptr;
	nxt->type = PAX_DMA_TYPE_NEXT_PTR;
	nxt->toggle = toggle;
}

static void prepare_ring(struct dma_iproc_pax_ring_data *ring)
{
	uintptr_t curr, next, last;
	int buff_count = PAX_DMA_NUM_BD_BUFFS;
#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	uint32_t toggle;
#endif

	/* zero out descriptor area */
	memset(ring->bd, 0x0, PAX_DMA_RM_DESC_RING_SIZE * PAX_DMA_NUM_BD_BUFFS);
	memset(ring->cmpl, 0x0, PAX_DMA_RM_CMPL_RING_SIZE);

	/* start with first buffer, valid toggle is 0x1 */
#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	toggle = 0x1;
#endif
	curr = (uintptr_t)ring->bd;
	next = curr + PAX_DMA_RM_DESC_RING_SIZE;
	last = curr + PAX_DMA_RM_DESC_RING_SIZE * PAX_DMA_NUM_BD_BUFFS;
	do {
#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
		init_toggle((void *)curr, toggle);
		/* Place next_table desc as last BD entry on each buffer */
		rm_write_next_table_desc(PAX_DMA_NEXT_TBL_ADDR((void *)curr),
					 (void *)next, toggle);
#elif CONFIG_DMA_IPROC_PAX_DOORBELL_MODE
		/* Place next_table desc as last BD entry on each buffer */
		rm_write_next_table_desc(PAX_DMA_NEXT_TBL_ADDR((void *)curr),
					 (void *)next, 0);
#endif

#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
		/* valid toggle flips for each buffer */
		toggle = toggle ? 0x0 : 0x1;
#endif
		curr += PAX_DMA_RM_DESC_RING_SIZE;
		next += PAX_DMA_RM_DESC_RING_SIZE;
		/* last entry, chain back to first buffer */
		if (next == last) {
			next = (uintptr_t)ring->bd;
		}

	} while (--buff_count);

	dma_mb();

	/* start programming from first RM header */
	ring->curr.write_ptr = ring->bd;
	/* valid toggle starts with 1 after reset */
	ring->curr.toggle = 1;
	/* completion read offset */
	ring->curr.cmpl_rd_offs = 0;
	/* inflight descs */
	ring->descs_inflight = 0;

	/* init sync data for the ring */
	ring->curr.sync_data.signature = PAX_DMA_WRITE_SYNC_SIGNATURE;
	ring->curr.sync_data.ring = ring->idx;
	/* pkt id for active dma xfer */
	ring->curr.sync_data.opaque = 0x0;
	/* pkt count for active dma xfer */
	ring->curr.sync_data.total_pkts = 0x0;
}

static int init_rm(struct dma_iproc_pax_data *pd)
{
	int ret = -ETIMEDOUT, timeout = 1000;

	k_mutex_lock(&pd->dma_lock, K_FOREVER);
	/* Wait for Ring Manager ready */
	do {
		LOG_DBG("Waiting for RM HW init\n");
		if ((sys_read32(RM_COMM_REG(pd, RM_COMM_MAIN_HW_INIT_DONE)) &
		    RM_COMM_MAIN_HW_INIT_DONE_MASK)) {
			ret = 0;
			break;
		}
		k_sleep(K_MSEC(1));
	} while (--timeout);
	k_mutex_unlock(&pd->dma_lock);

	if (!timeout) {
		LOG_WRN("RM HW Init timedout!\n");
	} else {
		LOG_INF("PAX DMA RM HW Init Done\n");
	}

	return ret;
}

static void rm_cfg_start(struct dma_iproc_pax_data *pd)
{
	uint32_t val;

	k_mutex_lock(&pd->dma_lock, K_FOREVER);

	/* set config done 0, enable toggle mode */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CONTROL));
	val &= ~RM_COMM_CONTROL_CONFIG_DONE;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CONTROL));

	val &= ~(RM_COMM_CONTROL_MODE_MASK << RM_COMM_CONTROL_MODE_SHIFT);

#ifdef CONFIG_DMA_IPROC_PAX_DOORBELL_MODE
	val |= (RM_COMM_CONTROL_MODE_DOORBELL <<
		RM_COMM_CONTROL_MODE_SHIFT);
#elif CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	val |= (RM_COMM_CONTROL_MODE_ALL_BD_TOGGLE <<
		RM_COMM_CONTROL_MODE_SHIFT);
#endif
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CONTROL));
	sys_write32(RM_COMM_MSI_DISABLE_MASK,
		    RM_COMM_REG(pd, RM_COMM_MSI_DISABLE));

	val = sys_read32(RM_COMM_REG(pd, RM_COMM_AXI_READ_BURST_THRESHOLD));
	val &= ~(RM_COMM_THRESHOLD_CFG_RD_FIFO_MAX_THRESHOLD_MASK <<
		 RM_COMM_THRESHOLD_CFG_RD_FIFO_MAX_THRESHOLD_SHIFT);
	val |= RM_COMM_THRESHOLD_CFG_RD_FIFO_MAX_THRESHOLD_SHIFT_VAL <<
	       RM_COMM_THRESHOLD_CFG_RD_FIFO_MAX_THRESHOLD_SHIFT;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_AXI_READ_BURST_THRESHOLD));

	val = sys_read32(RM_COMM_REG(pd, RM_COMM_FIFO_FULL_THRESHOLD));
	val &= ~(RM_COMM_PKT_ALIGNMENT_BD_FIFO_FULL_THRESHOLD_MASK <<
		 RM_COMM_PKT_ALIGNMENT_BD_FIFO_FULL_THRESHOLD_SHIFT);
	val |= RM_COMM_PKT_ALIGNMENT_BD_FIFO_FULL_THRESHOLD_VAL <<
	       RM_COMM_PKT_ALIGNMENT_BD_FIFO_FULL_THRESHOLD_SHIFT;

	val &= ~(RM_COMM_BD_FIFO_FULL_THRESHOLD_MASK <<
		 RM_COMM_BD_FIFO_FULL_THRESHOLD_SHIFT);
	val |= RM_COMM_BD_FIFO_FULL_THRESHOLD_VAL <<
	       RM_COMM_BD_FIFO_FULL_THRESHOLD_SHIFT;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_FIFO_FULL_THRESHOLD));

	/* Enable Line interrupt */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CONTROL));
	val |= RM_COMM_CONTROL_LINE_INTR_EN;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CONTROL));

	/* Enable AE_TIMEOUT */
	sys_write32(RM_COMM_AE_TIMEOUT_VAL,
		    RM_COMM_REG(pd, RM_COMM_AE_TIMEOUT));
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CONTROL));
	val |= RM_COMM_CONTROL_AE_TIMEOUT_EN;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CONTROL));

	/* AE (Acceleration Engine) grouping to group '0' */
	val = sys_read32(RM_COMM_REG(pd, RM_AE0_AE_CONTROL));
	val &= ~RM_AE_CTRL_AE_GROUP_MASK;
	sys_write32(val, RM_COMM_REG(pd, RM_AE0_AE_CONTROL));
	val |= RM_AE_CONTROL_ACTIVE;
	sys_write32(val, RM_COMM_REG(pd, RM_AE0_AE_CONTROL));

	/* AXI read/write channel enable */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_AXI_CONTROL));
	val |= (RM_COMM_AXI_CONTROL_RD_CH_EN | RM_COMM_AXI_CONTROL_WR_CH_EN);
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_AXI_CONTROL));

	/* Tune RM control programming for 4 rings */
	sys_write32(RM_COMM_TIMER_CONTROL0_VAL,
		    RM_COMM_REG(pd, RM_COMM_TIMER_CONTROL_0));
	sys_write32(RM_COMM_TIMER_CONTROL1_VAL,
		    RM_COMM_REG(pd, RM_COMM_TIMER_CONTROL_1));
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_BURST_LENGTH));
	val |= RM_COMM_BD_FETCH_CACHE_ALIGNED_DISABLED;
	val |= RM_COMM_VALUE_FOR_DDR_ADDR_GEN_VAL <<
	       RM_COMM_VALUE_FOR_DDR_ADDR_GEN_SHIFT;
	val |= RM_COMM_VALUE_FOR_TOGGLE_VAL << RM_COMM_VALUE_FOR_TOGGLE_SHIFT;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_BURST_LENGTH));

	val = sys_read32(RM_COMM_REG(pd, RM_COMM_BD_FETCH_MODE_CONTROL));
	val |= RM_COMM_DISABLE_GRP_BD_FIFO_FLOW_CONTROL_FOR_PKT_ALIGNMENT;
	val |= RM_COMM_DISABLE_PKT_ALIGNMENT_BD_FIFO_FLOW_CONTROL;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_BD_FETCH_MODE_CONTROL));

	/* Set Sequence max count to the max supported value */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_MASK_SEQUENCE_MAX_COUNT));
	val = (val | RING_MASK_SEQ_MAX_COUNT_MASK);
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_MASK_SEQUENCE_MAX_COUNT));

	k_mutex_unlock(&pd->dma_lock);
}

static void rm_ring_clear_stats(struct dma_iproc_pax_data *pd,
				enum ring_idx idx)
{
	/* Read ring Tx, Rx, and Outstanding counts to clear */
	sys_read32(RM_RING_REG(pd, idx, RING_NUM_REQ_RECV_LS));
	sys_read32(RM_RING_REG(pd, idx, RING_NUM_REQ_RECV_MS));
	sys_read32(RM_RING_REG(pd, idx, RING_NUM_REQ_TRANS_LS));
	sys_read32(RM_RING_REG(pd, idx, RING_NUM_REQ_TRANS_MS));
	sys_read32(RM_RING_REG(pd, idx, RING_NUM_REQ_OUTSTAND));
}

static void rm_cfg_finish(struct dma_iproc_pax_data *pd)
{
	uint32_t val;

	k_mutex_lock(&pd->dma_lock, K_FOREVER);

	/* set Ring config done */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CONTROL));
	val |= RM_COMM_CONTROL_CONFIG_DONE;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CONTROL));

	k_mutex_unlock(&pd->dma_lock);
}

static inline void write_doorbell(struct dma_iproc_pax_data *pd,
				  enum ring_idx idx)
{
	struct dma_iproc_pax_ring_data *ring = &(pd->ring[idx]);

	sys_write32(ring->descs_inflight,
		    RM_RING_REG(pd, idx, RING_DOORBELL_BD_WRITE_COUNT));
	ring->descs_inflight = 0;
}

static inline void set_ring_active(struct dma_iproc_pax_data *pd,
				   enum ring_idx idx,
				   bool active)
{
	uint32_t val;

	val = sys_read32(RM_RING_REG(pd, idx, RING_CONTROL));
	if (active)
		val |= RING_CONTROL_ACTIVE;
	else
		val &= ~RING_CONTROL_ACTIVE;
	sys_write32(val, RM_RING_REG(pd, idx, RING_CONTROL));
}

static int init_ring(struct dma_iproc_pax_data *pd, enum ring_idx idx)
{
	uint32_t val;
	uintptr_t desc = (uintptr_t)pd->ring[idx].bd;
	uintptr_t cmpl = (uintptr_t)pd->ring[idx].cmpl;
	int timeout = 5000, ret = 0;

	k_mutex_lock(&pd->dma_lock, K_FOREVER);

	/*  Read cmpl write ptr incase previous dma stopped */
	sys_read32(RM_RING_REG(pd, idx, RING_CMPL_WRITE_PTR));

	/* Inactivate ring */
	sys_write32(0x0, RM_RING_REG(pd, idx, RING_CONTROL));

	/* set Ring config done */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CONTROL));
	val |= RM_COMM_CONTROL_CONFIG_DONE;
	sys_write32(val,  RM_COMM_REG(pd, RM_COMM_CONTROL));
	/* Flush ring before loading new descriptor */
	sys_write32(RING_CONTROL_FLUSH, RM_RING_REG(pd, idx,
						    RING_CONTROL));
	do {
		if (sys_read32(RM_RING_REG(pd, idx, RING_FLUSH_DONE)) &
		    RING_FLUSH_DONE_MASK)
			break;
		k_busy_wait(1);
	} while (--timeout);

	if (!timeout) {
		LOG_WRN("Ring %d flush timedout!\n", idx);
		ret = -ETIMEDOUT;
		goto err;
	}

	/* clear ring after flush */
	sys_write32(0x0, RM_RING_REG(pd, idx, RING_CONTROL));

	/* Clear Ring config done */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CONTROL));
	val &= ~(RM_COMM_CONTROL_CONFIG_DONE);
	sys_write32(val,  RM_COMM_REG(pd, RM_COMM_CONTROL));
	/* ring group id set to '0' */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CTRL_REG(idx)));
	val &= ~RING_COMM_CTRL_AE_GROUP_MASK;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CTRL_REG(idx)));

	/* DDR update control, set timeout value */
	val = RING_DDR_CONTROL_COUNT(RING_DDR_CONTROL_COUNT_VAL) |
	      RING_DDR_CONTROL_TIMER(RING_DDR_CONTROL_TIMER_VAL) |
	      RING_DDR_CONTROL_ENABLE;

	sys_write32(val, RM_RING_REG(pd, idx, RING_CMPL_WR_PTR_DDR_CONTROL));
	/* Disable Ring MSI Timeout */
	sys_write32(RING_DISABLE_MSI_TIMEOUT_VALUE,
		    RM_RING_REG(pd, idx, RING_DISABLE_MSI_TIMEOUT));

	/* BD and CMPL desc queue start address */
	sys_write32((uint32_t)desc, RM_RING_REG(pd, idx, RING_BD_START_ADDR));
	sys_write32((uint32_t)cmpl, RM_RING_REG(pd, idx, RING_CMPL_START_ADDR));
	val = sys_read32(RM_RING_REG(pd, idx, RING_BD_READ_PTR));

	/* keep ring inactive after init to avoid BD poll */
#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	set_ring_active(pd, idx, false);
#elif CONFIG_DMA_IPROC_PAX_DOORBELL_MODE
	set_ring_active(pd, idx, true);
#endif

#if !defined(CONFIG_DMA_IPROC_PAX_POLL_MODE)
	/* Enable ring completion interrupt */
	sys_write32(0x0, RM_RING_REG(pd, idx,
				     RING_COMPLETION_INTERRUPT_STAT_MASK));
#endif
	rm_ring_clear_stats(pd, idx);
err:
	k_mutex_unlock(&pd->dma_lock);

	return ret;
}

static int poll_on_write_sync(const struct device *dev,
			      struct dma_iproc_pax_ring_data *ring)
{
	const struct dma_iproc_pax_cfg *cfg = dev->config;
	struct dma_iproc_pax_write_sync_data sync_rd, *recv, *sent;
	uint64_t pci_addr;
	uint32_t *pci32, *axi32;
	uint32_t zero_init = 0, timeout = PAX_DMA_MAX_SYNC_WAIT;
	int ret;

	recv = &sync_rd;
	sent = &(ring->curr.sync_data);
	/* form host pci sync address */
	pci32 = (uint32_t *)&pci_addr;
	pci32[0] = ring->sync_pci.addr_lo;
	pci32[1] = ring->sync_pci.addr_hi;
	axi32 = (uint32_t *)&sync_rd;

	do {
		ret = pcie_ep_xfer_data_memcpy(cfg->pcie_dev, pci_addr,
					       (uintptr_t *)axi32, 4,
					       PCIE_OB_LOWMEM, HOST_TO_DEVICE);

		if (memcmp((void *)recv, (void *)sent, 4) == 0) {
			/* clear the sync word */
			ret = pcie_ep_xfer_data_memcpy(cfg->pcie_dev, pci_addr,
						       (uintptr_t *)&zero_init,
						       4, PCIE_OB_LOWMEM,
						       DEVICE_TO_HOST);
			dma_mb();
			ret = 0;
			break;
		}
		k_busy_wait(1);
	} while (--timeout);

	if (!timeout) {
		LOG_ERR("[ring %d]: not recvd write sync!\n", ring->idx);
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int process_cmpl_event(const struct device *dev,
			      enum ring_idx idx, uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = dev->data;
	uint32_t wr_offs, rd_offs, ret = 0;
	struct dma_iproc_pax_ring_data *ring = &(pd->ring[idx]);
	struct cmpl_pkt *c;
	uint32_t is_outstanding;

	/* cmpl read offset, unprocessed cmpl location */
	rd_offs = ring->curr.cmpl_rd_offs;

	wr_offs = sys_read32(RM_RING_REG(pd, idx,
					 RING_CMPL_WRITE_PTR));

	/* Update read ptr to "processed" */
	ring->curr.cmpl_rd_offs = wr_offs;

	/*
	 * Ensure consistency of completion descriptor
	 * The completion desc is updated by RM via AXI stream
	 * CPU need to ensure the memory operations are completed
	 * before reading cmpl area, by a "dsb"
	 * If Dcache enabled, need to invalidate the cachelines to
	 * read updated cmpl desc. The cache API also issues dsb.
	 */
	dma_mb();

	/* Decode cmpl pkt id to verify */
	c = (struct cmpl_pkt *)((uintptr_t)ring->cmpl +
	    PAX_DMA_CMPL_DESC_SIZE * PAX_DMA_CURR_CMPL_IDX(wr_offs));

	LOG_DBG("RING%d WR_PTR:%d opq:%d, rm_status:%x dma_status:%x\n",
		idx, wr_offs, c->opq, c->rm_status, c->dma_status);

	is_outstanding = sys_read32(RM_RING_REG(pd, idx,
						RING_NUM_REQ_OUTSTAND));
	if ((ring->curr.opq != c->opq) && (is_outstanding != 0)) {
		LOG_ERR("RING%d: pkt id should be %d, rcvd %d outst=%d\n",
			idx, ring->curr.opq, c->opq, is_outstanding);
		ret = -EIO;
	}
	/* check for completion AE timeout */
	if (c->rm_status == RM_COMPLETION_AE_TIMEOUT) {
		LOG_ERR("RING%d WR_PTR:%d rm_status:%x AE Timeout!\n",
			idx, wr_offs, c->rm_status);
		/* TBD: Issue full card reset to restore operations */
		LOG_ERR("Needs Card Reset to recover!\n");
		ret = -ETIMEDOUT;
	}

	if (ring->dma_callback) {
		ring->dma_callback(dev, ring->callback_arg, idx, ret);
	}

	/* clear total packet count and non header bd count */
	ring->total_pkt_count = 0;

	return ret;
}

#ifdef CONFIG_DMA_IPROC_PAX_POLL_MODE
static int peek_ring_cmpl(const struct device *dev,
			  enum ring_idx idx, uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = dev->data;
	uint32_t wr_offs, rd_offs, timeout = PAX_DMA_MAX_POLL_WAIT;
	struct dma_iproc_pax_ring_data *ring = &(pd->ring[idx]);

	/* cmpl read offset, unprocessed cmpl location */
	rd_offs = ring->curr.cmpl_rd_offs;

	/* poll write_ptr until cmpl received for all buffers */
	do {
		wr_offs = sys_read32(RM_RING_REG(pd, idx,
						 RING_CMPL_WRITE_PTR));
		if (PAX_DMA_GET_CMPL_COUNT(wr_offs, rd_offs) >= pl_len)
			break;
		k_busy_wait(1);
	} while (--timeout);

	if (timeout == 0) {
		LOG_ERR("RING%d timeout, rcvd %d, expected %d!\n",
			idx, PAX_DMA_GET_CMPL_COUNT(wr_offs, rd_offs), pl_len);
		/* More debug info on current dma instance */
		LOG_ERR("WR_PTR:%x RD_PTR%x\n", wr_offs, rd_offs);
		return  -ETIMEDOUT;
	}

	return process_cmpl_event(dev, idx, pl_len);
}
#else
static void rm_isr(const struct device *dev)
{
	uint32_t status, err_stat, idx;
	struct dma_iproc_pax_data *pd = dev->data;

	err_stat =
	sys_read32(RM_COMM_REG(pd,
			       RM_COMM_AE_INTERFACE_GROUP_0_INTERRUPT_MASK));
	sys_write32(err_stat,
		    RM_COMM_REG(pd,
				RM_COMM_AE_INTERFACE_GROUP_0_INTERRUPT_CLEAR));

	/* alert waiting thread to process, for each completed ring */
	for (idx = PAX_DMA_RING0; idx < PAX_DMA_RINGS_MAX; idx++) {
		status =
		sys_read32(RM_RING_REG(pd, idx,
				       RING_COMPLETION_INTERRUPT_STAT));
		sys_write32(status,
			    RM_RING_REG(pd, idx,
					RING_COMPLETION_INTERRUPT_STAT_CLEAR));
		if (status & 0x1) {
			k_sem_give(&pd->ring[idx].alert);
		}
	}
}
#endif

static int dma_iproc_pax_init(const struct device *dev)
{
	const struct dma_iproc_pax_cfg *cfg = dev->config;
	struct dma_iproc_pax_data *pd = dev->data;
	int r;
	uintptr_t mem_aligned;

	if (!device_is_ready(cfg->pcie_dev)) {
		LOG_ERR("PCIe device not ready");
		return -ENODEV;
	}

	pd->dma_base = cfg->dma_base;
	pd->rm_comm_base = cfg->rm_comm_base;
	pd->used_rings = (cfg->use_rings < PAX_DMA_RINGS_MAX) ?
			 cfg->use_rings : PAX_DMA_RINGS_MAX;

	/* dma/rm access lock */
	k_mutex_init(&pd->dma_lock);

	/* Ring Manager H/W init */
	if (init_rm(pd)) {
		return -ETIMEDOUT;
	}

	/* common rm config */
	rm_cfg_start(pd);

	/* individual ring config */
	for (r = 0; r < pd->used_rings; r++) {
		/* per-ring mutex lock */
		k_mutex_init(&pd->ring[r].lock);
		/* Init alerts */
		k_sem_init(&pd->ring[r].alert, 0, 1);

		pd->ring[r].idx = r;
		pd->ring[r].ring_base = cfg->rm_base +
					PAX_DMA_RING_ADDR_OFFSET(r);
		LOG_DBG("RING%d,VERSION:0x%x\n", pd->ring[r].idx,
			sys_read32(RM_RING_REG(pd, r, RING_VER)));

		/* Allocate for 2 BD buffers + cmpl buffer + sync location */
		pd->ring[r].ring_mem = (void *)((uintptr_t)cfg->bd_memory_base +
					r * PAX_DMA_PER_RING_ALLOC_SIZE);
		if (!pd->ring[r].ring_mem) {
			LOG_ERR("RING%d failed to alloc desc memory!\n", r);
			return -ENOMEM;
		}
		/* Find 8K aligned address within allocated region */
		mem_aligned = ((uintptr_t)pd->ring[r].ring_mem +
			       PAX_DMA_RING_ALIGN - 1) &
			       ~(PAX_DMA_RING_ALIGN - 1);

		pd->ring[r].cmpl = (void *)mem_aligned;
		pd->ring[r].bd = (void *)(mem_aligned +
					  PAX_DMA_RM_CMPL_RING_SIZE);
		pd->ring[r].sync_loc = (void *)((uintptr_t)pd->ring[r].bd +
				      PAX_DMA_RM_DESC_RING_SIZE *
				      PAX_DMA_NUM_BD_BUFFS);

		LOG_DBG("Ring%d,allocated Mem:0x%p Size %d\n",
			pd->ring[r].idx,
			pd->ring[r].ring_mem,
			PAX_DMA_PER_RING_ALLOC_SIZE);
		LOG_DBG("Ring%d,BD:0x%p, CMPL:0x%p, SYNC_LOC:0x%p\n",
			pd->ring[r].idx,
			pd->ring[r].bd,
			pd->ring[r].cmpl,
			pd->ring[r].sync_loc);

		/* Prepare ring desc table */
		prepare_ring(&(pd->ring[r]));

		/* initialize ring */
		init_ring(pd, r);
	}

	/* set ring config done */
	rm_cfg_finish(pd);

#ifndef CONFIG_DMA_IPROC_PAX_POLL_MODE
	/* Register and enable RM interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    rm_isr,
		    DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));
#else
	LOG_INF("%s PAX DMA rings in poll mode!\n", dev->name);
#endif
	LOG_INF("%s RM setup %d rings\n", dev->name, pd->used_rings);

	return 0;
}

static int dma_iproc_pax_gen_desc(struct dma_iproc_pax_ring_data *ring,
				  bool is_mega,
				  uint64_t pci_addr,
				  uint64_t axi_addr,
				  uint32_t length,
				  enum pax_dma_dir dir,
				  uint32_t *non_hdr_bd_count)
{
	struct rm_header *hdr;

	if (*non_hdr_bd_count == 0) {
		/* Generate Header BD */
		ring->current_hdr = (uintptr_t)get_curr_desc_addr(ring);
		rm_write_header_desc((void *)ring->current_hdr,
				     curr_toggle_val(ring),
				     curr_pkt_id(ring),
				     PAX_DMA_RM_DESC_BDCOUNT,
				     pci_addr);
		ring->total_pkt_count++;
	}

	rm_write_pcie_desc(get_curr_desc_addr(ring),
			   curr_toggle_val(ring), pci_addr);
	*non_hdr_bd_count = *non_hdr_bd_count + 1;
	rm_write_src_dst_desc(get_curr_desc_addr(ring),
			      is_mega, curr_toggle_val(ring),
			      axi_addr, length, dir);
	*non_hdr_bd_count = *non_hdr_bd_count + 1;

	/* Update Header BD with bd count */
	hdr = (struct rm_header *)ring->current_hdr;
	hdr->bdcount = *non_hdr_bd_count;
	if (*non_hdr_bd_count == MAX_BD_COUNT_PER_HEADER) {
		*non_hdr_bd_count = 0;
	}

	return 0;
}

static int dma_iproc_pax_gen_packets(const struct device *dev,
				     struct dma_iproc_pax_ring_data *ring,
				     uint32_t direction,
				     struct dma_block_config *config,
				     uint32_t *non_hdr_bd_count)
{
	uint32_t outstanding, remaining_len;
	uint32_t offset, curr, mega_len;
	uint64_t axi_addr;
	uint64_t pci_addr;
	enum pax_dma_dir dir;

	switch (direction) {
	case MEMORY_TO_PERIPHERAL:
		pci_addr = config->dest_address;
		axi_addr = config->source_address;
		dir = CARD_TO_HOST;
		break;
	case PERIPHERAL_TO_MEMORY:
		axi_addr = config->dest_address;
		pci_addr = config->source_address;
		dir = HOST_TO_CARD;
		break;
	default:
		LOG_ERR("not supported transfer direction");
		return -EINVAL;
	}

	outstanding = config->block_size;
	offset = 0;
	while (outstanding) {
		curr = MIN(outstanding, PAX_DMA_MAX_SZ_PER_BD);
		mega_len = curr / PAX_DMA_MEGA_LENGTH_MULTIPLE;
		remaining_len = curr % PAX_DMA_MEGA_LENGTH_MULTIPLE;
		pci_addr = pci_addr + offset;
		axi_addr = axi_addr + offset;

		if (mega_len) {
			dma_iproc_pax_gen_desc(ring, true, pci_addr,
					       axi_addr, mega_len, dir,
					       non_hdr_bd_count);
			offset = offset + mega_len *
				PAX_DMA_MEGA_LENGTH_MULTIPLE;
		}

		if (remaining_len) {
			pci_addr = pci_addr + offset;
			axi_addr = axi_addr + offset;
			dma_iproc_pax_gen_desc(ring, false, pci_addr, axi_addr,
					       remaining_len, dir,
					       non_hdr_bd_count);
			offset = offset + remaining_len;
		}

		outstanding =  outstanding - curr;
	}

	return 0;
}

#ifdef CONFIG_DMA_IPROC_PAX_POLL_MODE
static void set_pkt_count(const struct device *dev,
			  enum ring_idx idx,
			  uint32_t pl_len)
{
	/* Nothing needs to be programmed here in poll mode */
}

static int wait_for_pkt_completion(const struct device *dev,
				   enum ring_idx idx,
				   uint32_t pl_len)
{
	/* poll for completion */
	return peek_ring_cmpl(dev, idx, pl_len);
}
#else
static void set_pkt_count(const struct device *dev,
			  enum ring_idx idx,
			  uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = dev->data;
	uint32_t val;

	/* program packet count for interrupt assertion */
	val = sys_read32(RM_RING_REG(pd, idx,
				     RING_CMPL_WR_PTR_DDR_CONTROL));
	val &= ~RING_DDR_CONTROL_COUNT_MASK;
	val |= RING_DDR_CONTROL_COUNT(pl_len);
	sys_write32(val, RM_RING_REG(pd, idx,
				     RING_CMPL_WR_PTR_DDR_CONTROL));
}

static int wait_for_pkt_completion(const struct device *dev,
				   enum ring_idx idx,
				   uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = dev->data;
	struct dma_iproc_pax_ring_data *ring;

	ring = &(pd->ring[idx]);
	/* wait for sg dma completion alert */
	if (k_sem_take(&ring->alert, K_MSEC(PAX_DMA_TIMEOUT)) != 0) {
		LOG_ERR("PAX DMA [ring %d] Timeout!\n", idx);
		return -ETIMEDOUT;
	}

	return process_cmpl_event(dev, idx, pl_len);
}
#endif

static int dma_iproc_pax_process_dma_blocks(const struct device *dev,
					    enum ring_idx idx,
					    struct dma_config *config)
{
	struct dma_iproc_pax_data *pd = dev->data;
	const struct dma_iproc_pax_cfg *cfg = dev->config;
	int ret = 0;
	struct dma_iproc_pax_ring_data *ring;
	uint32_t toggle_bit, non_hdr_bd_count = 0;
	struct dma_block_config sync_pl;
	struct dma_iproc_pax_addr64 sync;
	struct dma_block_config *block_config = config->head_block;

	if (block_config == NULL) {
		LOG_ERR("head_block is NULL\n");
		return -EINVAL;
	}

	ring = &(pd->ring[idx]);

	/*
	 * Host sync buffer isn't ready at zephyr/driver init-time
	 * Read the host address location once at first DMA write
	 * on that ring.
	 */
	if ((ring->sync_pci.addr_lo == 0x0) &&
	    (ring->sync_pci.addr_hi == 0x0)) {
		/* populate sync data location */
		LOG_DBG("sync addr loc 0x%x\n", cfg->scr_addr_loc);
		sync.addr_lo = sys_read32(cfg->scr_addr_loc + 4);
		sync.addr_hi = sys_read32(cfg->scr_addr_loc);
		ring->sync_pci.addr_lo = sync.addr_lo + idx * 4;
		ring->sync_pci.addr_hi = sync.addr_hi;
		LOG_DBG("ring:%d,sync addr:0x%x.0x%x\n", idx,
			ring->sync_pci.addr_hi,
			ring->sync_pci.addr_lo);
	}

	/* account extra sync packet */
	ring->curr.sync_data.opaque = ring->curr.opq;
	ring->curr.sync_data.total_pkts = config->block_count;
	memcpy((void *)ring->sync_loc,
	       (void *)&(ring->curr.sync_data), 4);
	sync_pl.dest_address = ring->sync_pci.addr_lo |
			   (uint64_t)ring->sync_pci.addr_hi << 32;
	sync_pl.source_address = (uintptr_t)ring->sync_loc;
	sync_pl.block_size = 4; /* 4-bytes */

	/* current toggle bit */
	toggle_bit = ring->curr.toggle;
	/* current opq value for cmpl check */
	ring->curr.opq = curr_pkt_id(ring);

	/* Form descriptors for total block counts */
	while (block_config != NULL) {
		ret = dma_iproc_pax_gen_packets(dev, ring,
						config->channel_direction,
						block_config,
						&non_hdr_bd_count);
		if (ret)
			goto err;
		block_config = block_config->next_block;
	}

	/*
	 * Write sync payload descriptors should go with separate RM header
	 * as RM implementation allows all the BD's in a header packet should
	 * have same data transfer direction. Setting non_hdr_bd_count to 0,
	 * helps generate separate packet.
	 */
	ring->non_hdr_bd_count = 0;
	dma_iproc_pax_gen_packets(dev, ring, MEMORY_TO_PERIPHERAL, &sync_pl,
				  &non_hdr_bd_count);

	alloc_pkt_id(ring);
err:
	return ret;
}

static int dma_iproc_pax_configure(const struct device *dev, uint32_t channel,
				   struct dma_config *cfg)
{
	struct dma_iproc_pax_data *pd = dev->data;
	struct dma_iproc_pax_ring_data *ring;
	int ret = 0;

	if (channel >= PAX_DMA_RINGS_MAX) {
		LOG_ERR("Invalid ring/channel %d\n", channel);
		return -EINVAL;
	}

	ring = &(pd->ring[channel]);
	k_mutex_lock(&ring->lock, K_FOREVER);

	if (ring->ring_active) {
		ret = -EBUSY;
		goto err;
	}

	if (cfg->block_count >= RM_V2_MAX_BLOCK_COUNT) {
		LOG_ERR("Dma block count[%d] supported exceeds limit[%d]\n",
			cfg->block_count, RM_V2_MAX_BLOCK_COUNT);
		ret = -ENOTSUP;
		goto err;
	}

	ring->ring_active = 1;
	ret = dma_iproc_pax_process_dma_blocks(dev, channel, cfg);

	if (ret) {
		ring->ring_active = 0;
		goto err;
	}

	ring->dma_callback = cfg->dma_callback;
	ring->callback_arg = cfg->user_data;
err:
	k_mutex_unlock(&ring->lock);
	return ret;
}

static int dma_iproc_pax_transfer_start(const struct device *dev,
					uint32_t channel)
{
	int ret = 0;
	struct dma_iproc_pax_data *pd = dev->data;
	struct dma_iproc_pax_ring_data *ring;

	if (channel >= PAX_DMA_RINGS_MAX) {
		LOG_ERR("Invalid ring %d\n", channel);
		return -EINVAL;
	}

	ring = &(pd->ring[channel]);
	set_pkt_count(dev, channel, ring->total_pkt_count);

#ifdef CONFIG_DMA_IPROC_PAX_DOORBELL_MODE
	write_doorbell(pd, channel);
#elif CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	/* activate the ring */
	set_ring_active(pd, channel, true);
#endif

	ret = wait_for_pkt_completion(dev, channel, ring->total_pkt_count);
	if (ret) {
		goto err_ret;
	}

	ret = poll_on_write_sync(dev, ring);

err_ret:
	k_mutex_lock(&ring->lock, K_FOREVER);
	ring->ring_active = 0;
	k_mutex_unlock(&ring->lock);

#ifdef CONFIG_DMA_IPROC_PAX_TOGGLE_MODE
	/* deactivate the ring until next active transfer */
	set_ring_active(pd, channel, false);
#endif
	return ret;
}

static int dma_iproc_pax_transfer_stop(const struct device *dev,
				       uint32_t channel)
{
	return 0;
}

static const struct dma_driver_api pax_dma_driver_api = {
	.config = dma_iproc_pax_configure,
	.start = dma_iproc_pax_transfer_start,
	.stop = dma_iproc_pax_transfer_stop,
};

static const struct dma_iproc_pax_cfg pax_dma_cfg = {
	.dma_base = DT_INST_REG_ADDR_BY_NAME(0, dme_regs),
	.rm_base = DT_INST_REG_ADDR_BY_NAME(0, rm_ring_regs),
	.rm_comm_base = DT_INST_REG_ADDR_BY_NAME(0, rm_comm_regs),
	.use_rings = DT_INST_PROP(0, dma_channels),
	.bd_memory_base = (void *)DT_INST_PROP_BY_IDX(0, bd_memory, 0),
	.scr_addr_loc = DT_INST_PROP(0, scr_addr_loc),
	.pcie_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, pcie_ep)),
};

DEVICE_DT_INST_DEFINE(0,
		    &dma_iproc_pax_init,
		    NULL,
		    &pax_dma_data,
		    &pax_dma_cfg,
		    POST_KERNEL,
		    CONFIG_DMA_INIT_PRIORITY,
		    &pax_dma_driver_api);
