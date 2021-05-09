/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_iproc_pax_dma_v1

#include <arch/cpu.h>
#include <cache.h>
#include <errno.h>
#include <init.h>
#include <kernel.h>
#include <linker/sections.h>
#include <soc.h>
#include <string.h>
#include <toolchain.h>
#include <zephyr/types.h>
#include <drivers/dma.h>
#include <drivers/pcie/endpoint/pcie_ep.h>
#include "dma_iproc_pax_v1.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_iproc_pax);

#define PAX_DMA_DEV_NAME(dev) ((dev)->name)

#define PAX_DMA_DEV_CFG(dev) \
	((struct dma_iproc_pax_cfg *)(dev)->config)

#define PAX_DMA_DEV_DATA(dev) \
	((struct dma_iproc_pax_data *)(dev)->data)

/* Driver runtime data for PAX DMA and RM */
static struct dma_iproc_pax_data pax_dma_data;

static inline uint32_t reset_pkt_id(struct dma_iproc_pax_ring_data *ring)
{
	return ring->pkt_id = 0x0;
}

/**
 * @brief Opaque/packet id allocator, range 0 to 31
 */
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
			     uint32_t opq, uint32_t bdcount)
{
	struct rm_header *r = (struct rm_header *)desc;

	r->opq = opq;
	/* DMA descriptor count init vlaue */
	r->bdcount = bdcount;
	r->prot = 0x0;
	/* No packet extension, start and end set to '1' */
	r->start = 1;
	r->end = 1;
	r->toggle = toggle;
	/* RM header type */
	r->type = PAX_DMA_TYPE_RM_HEADER;
}

/**
 * @brief Fill RM header descriptor for next transfer
 *        with invalid toggle
 */
static inline void rm_write_header_next_desc(void *desc,
					     struct dma_iproc_pax_ring_data *r,
					     uint32_t opq, uint32_t bdcount)
{
	/* Toggle bit is invalid until next payload configured */
	rm_write_header_desc(desc, (r->curr.toggle == 0) ? 1 : 0, opq, bdcount);
}

static inline void rm_header_set_bd_count(void *desc, uint32_t bdcount)
{
	struct rm_header *r = (struct rm_header *)desc;

	/* DMA descriptor count */
	r->bdcount = bdcount;
}

static inline void rm_header_set_toggle(void *desc, uint32_t toggle)
{
	struct rm_header *r = (struct rm_header *)desc;

	r->toggle = toggle;
}

/**
 * @brief Populate dma header descriptor
 */
static inline void rm_write_dma_header_desc(void *desc,
					    struct dma_iproc_pax_payload *pl)
{
	struct dma_header_desc *hdr = (struct dma_header_desc *)desc;

	hdr->length = pl->xfer_sz;
	hdr->opcode = pl->direction;
	/* DMA header type */
	hdr->type = PAX_DMA_TYPE_DMA_DESC;
}

/**
 * @brief Populate axi address descriptor
 */
static inline void rm_write_axi_addr_desc(void *desc,
					  struct dma_iproc_pax_payload *pl)
{
	struct axi_addr_desc *axi = (struct axi_addr_desc *)desc;

	axi->axi_addr = pl->axi_addr;
	axi->type = PAX_DMA_TYPE_DMA_DESC;

}

/**
 * @brief Populate pci address descriptor
 */
static inline void rm_write_pci_addr_desc(void *desc,
					  struct dma_iproc_pax_payload *pl)
{
	struct pci_addr_desc *pci = (struct pci_addr_desc *)desc;

	pci->pcie_addr = pl->pci_addr >> PAX_DMA_PCI_ADDR_ALIGNMT_SHIFT;
	pci->type = PAX_DMA_TYPE_DMA_DESC;
}

/**
 * @brief Return's pointer to the descriptor memory to be written next,
 *	  skip next pointer descriptor address.
 */
static void *next_desc_addr(struct dma_iproc_pax_ring_data *ring)
{
	struct next_ptr_desc *nxt;
	uintptr_t curr;

	curr = (uintptr_t)ring->curr.write_ptr + PAX_DMA_RM_DESC_BDWIDTH;
	/* if hit next table ptr, skip to next location, flip toggle */
	nxt = (struct next_ptr_desc *)curr;
	if (nxt->type == PAX_DMA_TYPE_NEXT_PTR) {
		LOG_DBG("hit next_ptr@0x%lx:T%d, next_table@0x%lx\n",
			curr, nxt->toggle, (uintptr_t)nxt->addr);
		uintptr_t last = (uintptr_t)ring->bd +
			     PAX_DMA_RM_DESC_RING_SIZE * PAX_DMA_NUM_BD_BUFFS;
		ring->curr.toggle = (ring->curr.toggle == 0) ? 1 : 0;
		/* move to next addr, wrap around if hits end */
		curr += PAX_DMA_RM_DESC_BDWIDTH;
		if (curr == last) {
			curr = (uintptr_t)ring->bd;
			LOG_DBG("hit end of desc:0x%lx, wrap to 0x%lx\n",
				last, curr);
		}
	}
	ring->curr.write_ptr = (void *)curr;
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
	uint32_t  toggle;
	int buff_count = PAX_DMA_NUM_BD_BUFFS;

	/* zero out descriptor area */
	memset(ring->bd, 0x0, PAX_DMA_RM_DESC_RING_SIZE * PAX_DMA_NUM_BD_BUFFS);
	memset(ring->cmpl, 0x0, PAX_DMA_RM_CMPL_RING_SIZE);

	/* opaque/packet id value */
	rm_write_header_desc(ring->bd, 0x0, reset_pkt_id(ring),
			     PAX_DMA_RM_DESC_BDCOUNT);
	/* start with first buffer, valid toggle is 0x1 */
	toggle = 0x1;
	curr = (uintptr_t)ring->bd;
	next = curr + PAX_DMA_RM_DESC_RING_SIZE;
	last = curr + PAX_DMA_RM_DESC_RING_SIZE * PAX_DMA_NUM_BD_BUFFS;
	do {
		/* Place next_table desc as last BD entry on each buffer */
		rm_write_next_table_desc(PAX_DMA_NEXT_TBL_ADDR((void *)curr),
					 (void *)next, toggle);

		/* valid toggle flips for each buffer */
		toggle = toggle ? 0x0 : 0x1;
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
	val |= (RM_COMM_CONTROL_MODE_TOGGLE << RM_COMM_CONTROL_MODE_SHIFT);
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CONTROL));

	/* Disable MSI */
	sys_write32(RM_COMM_MSI_DISABLE_VAL,
		    RM_COMM_REG(pd, RM_COMM_MSI_DISABLE));
	/* Enable Line interrupt */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CONTROL));
	val |= RM_COMM_CONTROL_LINE_INTR_EN;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CONTROL));

	/* Enable AE_TIMEOUT */
	sys_write32(RM_COMM_AE_TIMEOUT_VAL, RM_COMM_REG(pd,
							RM_COMM_AE_TIMEOUT));
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
	sys_write32(RM_COMM_RM_BURST_LENGTH,
		    RM_COMM_REG(pd, RM_COMM_RM_BURST_LENGTH));

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
	sys_write32(val,  RM_COMM_REG(pd, RM_COMM_CONTROL));

	k_mutex_unlock(&pd->dma_lock);
}

/* Activate/Deactivate rings */
static inline void set_ring_active(struct dma_iproc_pax_data *pd,
				   enum ring_idx idx,
				   bool active)
{
	uint32_t val;

	val = sys_read32(RM_RING_REG(pd, idx, RING_CONTROL));
	if (active) {
		val |= RING_CONTROL_ACTIVE;
	} else {
		val &= ~RING_CONTROL_ACTIVE;
	}
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

	/* Flush ring before loading new descriptor */
	sys_write32(RING_CONTROL_FLUSH, RM_RING_REG(pd, idx, RING_CONTROL));
	do {
		if (sys_read32(RM_RING_REG(pd, idx, RING_FLUSH_DONE)) &
		    RING_FLUSH_DONE_MASK) {
			break;
		}
		k_busy_wait(1);
	} while (--timeout);

	if (!timeout) {
		LOG_WRN("Ring %d flush timedout!\n", idx);
		ret = -ETIMEDOUT;
		goto err;
	}

	/* clear ring after flush */
	sys_write32(0x0, RM_RING_REG(pd, idx, RING_CONTROL));

	/* ring group id set to '0' */
	val = sys_read32(RM_COMM_REG(pd, RM_COMM_CTRL_REG(idx)));
	val &= ~RING_COMM_CTRL_AE_GROUP_MASK;
	sys_write32(val, RM_COMM_REG(pd, RM_COMM_CTRL_REG(idx)));

	/* DDR update control, set timeout value */
	val = RING_DDR_CONTROL_COUNT(RING_DDR_CONTROL_COUNT_VAL) |
	      RING_DDR_CONTROL_TIMER(RING_DDR_CONTROL_TIMER_VAL) |
	      RING_DDR_CONTROL_ENABLE;

	sys_write32(val, RM_RING_REG(pd, idx, RING_CMPL_WR_PTR_DDR_CONTROL));

	val = (uint32_t)((uintptr_t)desc >> PAX_DMA_RING_BD_ALIGN_ORDER);
	sys_write32(val, RM_RING_REG(pd, idx, RING_BD_START_ADDR));
	val = (uint32_t)((uintptr_t)cmpl >> PAX_DMA_RING_CMPL_ALIGN_ORDER);
	sys_write32(val, RM_RING_REG(pd, idx, RING_CMPL_START_ADDR));
	val = sys_read32(RM_RING_REG(pd, idx, RING_BD_READ_PTR));

	/* keep ring inactive after init to avoid BD poll */
	set_ring_active(pd, idx, false);
	rm_ring_clear_stats(pd, idx);
err:
	k_mutex_unlock(&pd->dma_lock);

	return ret;
}

static int poll_on_write_sync(const struct device *dev,
			      struct dma_iproc_pax_ring_data *ring)
{
	struct dma_iproc_pax_cfg *cfg = PAX_DMA_DEV_CFG(dev);
	const struct device *pcidev;
	struct dma_iproc_pax_write_sync_data sync_rd, *recv, *sent;
	uint64_t pci_addr;
	uint32_t *pci32, *axi32;
	uint32_t zero_init = 0, timeout = PAX_DMA_MAX_SYNC_WAIT;
	int ret;

	pcidev = device_get_binding(cfg->pcie_dev_name);
	if (!pcidev) {
		LOG_ERR("Cannot get pcie device\n");
		return -EINVAL;
	}
	recv = &sync_rd;
	sent = &(ring->curr.sync_data);
	/* form host pci sync address */
	pci32 = (uint32_t *)&pci_addr;
	pci32[0] = ring->sync_pci.addr_lo;
	pci32[1] = ring->sync_pci.addr_hi;
	axi32 = (uint32_t *)&sync_rd;

	do {
		ret = pcie_ep_xfer_data_memcpy(pcidev, pci_addr,
					       (uintptr_t *)axi32, 4,
					       PCIE_OB_LOWMEM, HOST_TO_DEVICE);

		if (memcmp((void *)recv, (void *)sent, 4) == 0) {
			/* clear the sync word */
			ret = pcie_ep_xfer_data_memcpy(pcidev, pci_addr,
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
		LOG_DBG("[ring %d]: not recvd write sync!\n", ring->idx);
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int process_cmpl_event(const struct device *dev,
			      enum ring_idx idx, uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
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

	return ret;
}

#ifdef CONFIG_DMA_IPROC_PAX_POLL_MODE
static int peek_ring_cmpl(const struct device *dev,
			  enum ring_idx idx, uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
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
static void rm_isr(void *arg)
{
	uint32_t status, err_stat, idx;
	const struct device *dev = arg;
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);

	/* read and clear interrupt status */
	status = sys_read32(RM_COMM_REG(pd, RM_COMM_MSI_INTR_INTERRUPT_STATUS));
	sys_write32(status, RM_COMM_REG(pd,
					RM_COMM_MSI_INTERRUPT_STATUS_CLEAR));

	/* read and clear DME/AE error interrupts */
	err_stat = sys_read32(RM_COMM_REG(pd,
					  RM_COMM_DME_INTERRUPT_STATUS_MASK));
	sys_write32(err_stat,
		    RM_COMM_REG(pd, RM_COMM_DME_INTERRUPT_STATUS_CLEAR));
	err_stat =
	sys_read32(RM_COMM_REG(pd,
			       RM_COMM_AE_INTERFACE_GROUP_0_INTERRUPT_MASK));
	sys_write32(err_stat,
		    RM_COMM_REG(pd,
				RM_COMM_AE_INTERFACE_GROUP_0_INTERRUPT_CLEAR));

	/* alert waiting thread to process, for each completed ring */
	for (idx = PAX_DMA_RING0; idx < PAX_DMA_RINGS_MAX; idx++) {
		if (status & (0x1 << idx)) {
			k_sem_give(&pd->ring[idx].alert);
		}
	}
}
#endif

static int dma_iproc_pax_init(const struct device *dev)
{
	struct dma_iproc_pax_cfg *cfg = PAX_DMA_DEV_CFG(dev);
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
	int r;
	uintptr_t mem_aligned;

	pd->dma_base = cfg->dma_base;
	pd->rm_comm_base = cfg->rm_comm_base;
	pd->used_rings = (cfg->use_rings < PAX_DMA_RINGS_MAX) ?
			 cfg->use_rings : PAX_DMA_RINGS_MAX;

	LOG_DBG("dma base:0x%x, rm comm base:0x%x, needed rings %d\n",
		pd->dma_base, pd->rm_comm_base, pd->used_rings);

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

		/* Allocate for 2 BD buffers + cmpl buffer + payload struct */
		pd->ring[r].ring_mem = (void *)((uintptr_t)cfg->bd_memory_base +
						r *
						PAX_DMA_PER_RING_ALLOC_SIZE);
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
		pd->ring[r].payload = (void *)((uintptr_t)pd->ring[r].bd +
				      PAX_DMA_RM_DESC_RING_SIZE *
				      PAX_DMA_NUM_BD_BUFFS);

		LOG_DBG("Ring%d,allocated Mem:0x%p Size %d\n",
			pd->ring[r].idx,
			pd->ring[r].ring_mem,
			PAX_DMA_PER_RING_ALLOC_SIZE);
		LOG_DBG("Ring%d,BD:0x%p, CMPL:0x%p, PL:0x%p\n",
			pd->ring[r].idx,
			pd->ring[r].bd,
			pd->ring[r].cmpl,
			pd->ring[r].payload);

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
	LOG_INF("%s PAX DMA rings in poll mode!\n", PAX_DMA_DEV_NAME(dev));
#endif
	LOG_INF("%s RM setup %d rings\n", PAX_DMA_DEV_NAME(dev),
		pd->used_rings);

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
	return peek_ring_cmpl(dev, idx, pl_len + 1);
}
#else
static void set_pkt_count(const struct device *dev,
			  enum ring_idx idx,
			  uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
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
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
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

static int dma_iproc_pax_do_xfer(const struct device *dev,
				 enum ring_idx idx,
				 struct dma_iproc_pax_payload *pl,
				 uint32_t pl_len)
{
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
	struct dma_iproc_pax_cfg *cfg = PAX_DMA_DEV_CFG(dev);
	int ret = 0, cnt;
	struct dma_iproc_pax_ring_data *ring;
	void *hdr;
	uint32_t toggle_bit;
	struct dma_iproc_pax_payload sync_pl;
	struct dma_iproc_pax_addr64 sync;

	ring = &(pd->ring[idx]);
	pl = ring->payload;

	/*
	 * Host sync buffer isn't ready at zephyr/driver init-time
	 * Read the host address location once at first DMA write
	 * on that ring.
	 */
	if ((ring->sync_pci.addr_lo == 0x0) &&
	    (ring->sync_pci.addr_hi == 0x0)) {
		/* populate sync data location */
		LOG_DBG("sync addr loc 0x%x\n", cfg->scr_addr_loc);

		sync.addr_lo = sys_read32(cfg->scr_addr_loc
					  + 4);
		sync.addr_hi = sys_read32(cfg->scr_addr_loc);
		ring->sync_pci.addr_lo = sync.addr_lo + idx * 4;
		ring->sync_pci.addr_hi = sync.addr_hi;
		LOG_DBG("ring:%d,sync addr:0x%x.0x%x\n", idx,
			ring->sync_pci.addr_hi,
			ring->sync_pci.addr_lo);
	}

	/* account extra sync packet */
	ring->curr.sync_data.opaque = ring->curr.opq;
	ring->curr.sync_data.total_pkts = pl_len;
	memcpy((void *)&ring->sync_loc,
	       (void *)&(ring->curr.sync_data), 4);
	sync_pl.pci_addr = ring->sync_pci.addr_lo |
			   (uint64_t)ring->sync_pci.addr_hi << 32;
	sync_pl.axi_addr = (uintptr_t)&ring->sync_loc;

	sync_pl.xfer_sz = 4; /* 4-bytes */
	sync_pl.direction = CARD_TO_HOST;

	/* Get descriptor write pointer for first header */
	hdr = (void *)ring->curr.write_ptr;
	/* current toggle bit */
	toggle_bit = ring->curr.toggle;
	/* current opq value for cmpl check */
	ring->curr.opq = curr_pkt_id(ring);

	/* DMA desc count for first payload */
	rm_header_set_bd_count(hdr, PAX_DMA_RM_DESC_BDCOUNT);

	/* Form dma descriptors for total sg payload */
	for (cnt = 0; cnt < pl_len; cnt++) {
		rm_write_dma_header_desc(next_desc_addr(ring), pl + cnt);
		rm_write_axi_addr_desc(next_desc_addr(ring), pl + cnt);
		rm_write_pci_addr_desc(next_desc_addr(ring), pl + cnt);
		/* Toggle may flip, program updated toggle value */
		rm_write_header_desc(next_desc_addr(ring),
				     curr_toggle_val(ring),
				     curr_pkt_id(ring),
				     PAX_DMA_RM_DESC_BDCOUNT);
	}

	/* Append write sync payload descriptors */
	rm_write_dma_header_desc(next_desc_addr(ring), &sync_pl);
	rm_write_axi_addr_desc(next_desc_addr(ring), &sync_pl);
	rm_write_pci_addr_desc(next_desc_addr(ring), &sync_pl);

	/* RM header for next transfer, RM wait on (invalid) toggle bit */
	rm_write_header_next_desc(next_desc_addr(ring), ring, alloc_pkt_id(ring),
		       PAX_DMA_RM_DESC_BDCOUNT);

	set_pkt_count(dev, idx, pl_len + 1);

	/* Ensure memory write before toggle flip */
	dma_mb();

	/* set toggle to valid in first header */
	rm_header_set_toggle(hdr, toggle_bit);
	/* activate the ring */
	set_ring_active(pd, idx, true);

	ret = wait_for_pkt_completion(dev, idx, pl_len + 1);
	if (ret)
		goto err_ret;

	ret = poll_on_write_sync(dev, ring);
	k_mutex_lock(&ring->lock, K_FOREVER);
	ring->ring_active = 0;
	k_mutex_unlock(&ring->lock);

err_ret:
	ring->ring_active = 0;
	/* deactivate the ring until next active transfer */
	set_ring_active(pd, idx, false);

	return ret;
}

static int dma_iproc_pax_configure(const struct device *dev, uint32_t channel,
				   struct dma_config *cfg)
{
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
	struct dma_iproc_pax_ring_data *ring;
	uint32_t xfer_sz;
	int ret = 0;
#ifdef CONFIG_DMA_IPROC_PAX_DEBUG
	uint32_t *pci_addr32;
	uint32_t *axi_addr32;
#endif

	if (channel >= PAX_DMA_RINGS_MAX) {
		LOG_ERR("Invalid ring/channel %d\n", channel);
		return -EINVAL;
	}

	ring = &(pd->ring[channel]);
	k_mutex_lock(&ring->lock, K_FOREVER);

	if (cfg->block_count > 1) {
		/* Scatter/gather list handling is not supported */
		ret = -ENOTSUP;
		goto err;
	}

	if (ring->ring_active) {
		ret = -EBUSY;
		goto err;
	}

	ring->ring_active = 1;

	if (cfg->channel_direction == MEMORY_TO_PERIPHERAL) {
#ifdef CONFIG_DMA_IPROC_PAX_DEBUG
		axi_addr32 = (uint32_t *)&cfg->head_block->source_address;
		pci_addr32 = (uint32_t *)&cfg->head_block->dest_address;
#endif
		ring->payload->direction = CARD_TO_HOST;
		ring->payload->pci_addr = cfg->head_block->dest_address;
		ring->payload->axi_addr = cfg->head_block->source_address;
	} else if (cfg->channel_direction == PERIPHERAL_TO_MEMORY) {
#ifdef CONFIG_DMA_IPROC_PAX_DEBUG
		axi_addr32 = (uint32_t *)&cfg->head_block->dest_address;
		pci_addr32 = (uint32_t *)&cfg->head_block->source_address;
#endif
		ring->payload->direction = HOST_TO_CARD;
		ring->payload->pci_addr = cfg->head_block->source_address;
		ring->payload->axi_addr = cfg->head_block->dest_address;
	} else {
		ring->ring_active = 0;
		ret = -ENOTSUP;
		goto err;
	}

	xfer_sz = cfg->head_block->block_size;

#ifdef CONFIG_DMA_IPROC_PAX_DEBUG
	if (xfer_sz > PAX_DMA_MAX_SIZE) {
		LOG_ERR("Unsupported size: %d\n", xfer_size);
		ring->ring_active = 0;
		ret = -EINVAL;
		goto err;
	}

	if (xfer_sz % PAX_DMA_MIN_SIZE) {
		LOG_ERR("Unaligned size 0x%x\n", xfer_size);
		ring->ring_active = 0;
		ret = -EINVAL;
		goto err;
	}

	if (pci_addr32[0] % PAX_DMA_ADDR_ALIGN) {
		LOG_ERR("Unaligned Host addr: 0x%x.0x%x\n",
			pci_addr32[1], pci_addr32[0]);
		ring->ring_active = 0;
		ret = -EINVAL;
		goto err;
	}

	if (axi_addr32[0] % PAX_DMA_ADDR_ALIGN) {
		LOG_ERR("Unaligned Card addr: 0x%x.0x%x\n",
			axi_addr32[1], axi_addr32[0]);
		ring->ring_active = 0;
		ret = -EINVAL;
		goto err;
	}
#endif
	ring->payload->xfer_sz = xfer_sz;

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
	struct dma_iproc_pax_data *pd = PAX_DMA_DEV_DATA(dev);
	struct dma_iproc_pax_ring_data *ring;

	if (channel >= PAX_DMA_RINGS_MAX) {
		LOG_ERR("Invalid ring %d\n", channel);
		return -EINVAL;
	}
	ring = &(pd->ring[channel]);
	/* do dma transfer of single buffer */
	ret = dma_iproc_pax_do_xfer(dev, channel, ring->payload, 1);

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
	.pcie_dev_name = DT_INST_PROP_BY_PHANDLE(0, pcie_ep, label),
};

DEVICE_DT_INST_DEFINE(0,
		    &dma_iproc_pax_init,
		    device_pm_control_nop,
		    &pax_dma_data,
		    &pax_dma_cfg,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pax_dma_driver_api);
