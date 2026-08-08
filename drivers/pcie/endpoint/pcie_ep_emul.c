/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_pcie_ep_emul

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep_emul.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pcie_ep_emul, LOG_LEVEL_ERR);

#define PCIE_EP_EMUL_CFG_SIZE     4096U
#define PCIE_EP_EMUL_BAR_COUNT    6U
#define PCIE_EP_EMUL_BAR_MIN_SIZE 4096U

/*
 * Configuration image layout and write policy.
 *
 * Each instance owns a 4 KiB configuration image modelling a conventional
 * PCIe (Type 0) configuration header. All layout and writability rules
 * live in this section so that an alternate transport backend can reuse
 * them unchanged:
 *
 *  0x00        vendor/device ID     read-only (from devicetree)
 *  0x04        command/status       command bits 0..2 writable (I/O space,
 *                                   memory space, bus master enables),
 *                                   status (upper 16 bits) read-only;
 *                                   status bit 4 (capabilities list) is
 *                                   set when MSI and/or MSI-X is configured
 *  0x08        class code/revision  read-only (from devicetree)
 *  0x10-0x27   BAR0..BAR5           writable as base assignment only:
 *                                   bits below the BAR size mask are
 *                                   ignored on writes, like hardware;
 *                                   disabled BARs read as zero
 *  0x34        capability pointer   read-only: offset of the first
 *                                   capability, or zero when neither MSI
 *                                   nor MSI-X is configured
 *  0x40        MSI capability       present when msi-vectors > 0 (ID 0x05,
 *                                   32-bit message address, no per-vector
 *                                   masking): the multiple-message-capable
 *                                   field encodes the devicetree vector
 *                                   count; the MSI enable bit and the
 *                                   multiple-message-enable field (bits
 *                                   22:20, the host-programmed vector
 *                                   allocation) are writable, with written
 *                                   values above MMC clamped to MMC (the
 *                                   spec leaves that case undefined;
 *                                   clamping keeps the model
 *                                   deterministic); message address/data
 *                                   read as zero and ignore writes
 *  next        MSI-X capability     present when msix-vectors > 0 (ID 0x11),
 *                                   placed directly after the MSI
 *                                   capability when both exist: the table
 *                                   size field encodes vectors - 1; the
 *                                   MSI-X enable bit and the function mask
 *                                   bit are writable; table and
 *                                   PBA BIR/offset are read-only and point
 *                                   into the first enabled BAR
 *  everything else                  read-only, reads as zero
 *
 * Writes to registers without writable bits leave the image unchanged and
 * are reported as -EPERM by the returning interfaces; the void
 * pcie_ep_conf_write() device API drops the result, so endpoint-side
 * writes stay silently ignored as on hardware.
 *
 * raise_irq() consults the same state like hardware would: MSI raises
 * fail with -ENOTSUP while the MSI enable bit is clear and with -EINVAL
 * when the vector lies outside the host-programmed allocation of
 * 2^MME vectors (MME defaults to zero, one vector); MSI-X raises fail
 * with -ENOTSUP while the MSI-X enable bit is clear or the function
 * mask bit is set; legacy interrupts have no enable concept.
 */
#define PCIE_EP_EMUL_CFG_VENDOR_DEVICE_ID 0x00U
#define PCIE_EP_EMUL_CFG_COMMAND_STATUS   0x04U
#define PCIE_EP_EMUL_CFG_CLASS_REVISION   0x08U
#define PCIE_EP_EMUL_CFG_BAR_BASE         0x10U
#define PCIE_EP_EMUL_CFG_CAPABILITY_PTR   0x34U
#define PCIE_EP_EMUL_CFG_FIRST_CAP        0x40U

/* Command register bits an endpoint application may toggle. */
#define PCIE_EP_EMUL_CMD_WRITABLE_MASK 0x0007U

/* Status register (upper command/status half) capabilities-list bit. */
#define PCIE_EP_EMUL_STATUS_CAP_LIST BIT(20)

/* Capability IDs and layout constants. */
#define PCIE_EP_EMUL_MSI_CAP_ID            0x05U
#define PCIE_EP_EMUL_MSIX_CAP_ID           0x11U
/* Config space stride reserved for the MSI capability. */
#define PCIE_EP_EMUL_MSI_CAP_SIZE          0x10U
/* In the first capability dword, message control is the upper half. */
#define PCIE_EP_EMUL_MSI_MMC_SHIFT         17U
#define PCIE_EP_EMUL_MSI_CTRL_ENABLE       BIT(16)
#define PCIE_EP_EMUL_MSI_MME_SHIFT         20U
#define PCIE_EP_EMUL_MSI_MME_MASK          (0x7U << PCIE_EP_EMUL_MSI_MME_SHIFT)
#define PCIE_EP_EMUL_MSIX_TABLE_SIZE_SHIFT 16U
#define PCIE_EP_EMUL_MSIX_CTRL_ENABLE      BIT(31)
#define PCIE_EP_EMUL_MSIX_CTRL_FUNC_MASK   BIT(30)

struct pcie_ep_emul_config {
	uint32_t vendor_id;
	uint32_t device_id;
	uint32_t class_code;
	uint32_t revision_id;
	uint32_t bar_sizes[PCIE_EP_EMUL_BAR_COUNT];
	uint32_t msi_vectors;
	uint32_t msix_vectors;
	bool legacy_irq;
	/*
	 * Optional dedicated DMA channels, indexed by enum xfer_direction
	 * (HOST_TO_DEVICE first, DEVICE_TO_HOST second, as ordered by
	 * dma-names in devicetree). dma_devs[] is NULL for both directions
	 * when the instance has no dmas property.
	 */
	const struct device *dma_devs[2];
	uint32_t dma_channels[2];
};

struct pcie_ep_emul_aperture {
	uint64_t pcie_base;
	uint8_t *local;
	uint32_t len;
	bool active;
};

struct pcie_ep_emul_map {
	uint64_t mapped_addr;
	uint32_t size;
	int aperture_idx;
	bool active;
	/*
	 * Number of DMA transfers currently pinned to this record. A record
	 * unmapped while pinned is draining: it stays reserved (unusable by
	 * map_addr and blocking aperture unregistration) until the last
	 * in-flight transfer referencing it completes.
	 */
	unsigned int dma_inflight;
};

/*
 * Per-direction DMA channel state. The mutex serializes the whole
 * configure/start/completion cycle so concurrent users of the same
 * channel cannot interleave; the semaphore is given by the channel
 * completion callback, which runs on the controller workqueue thread.
 */
struct pcie_ep_emul_dma_chan {
	struct k_mutex lock;
	struct k_sem done;
	int status;
};

struct pcie_ep_emul_data {
	struct k_spinlock lock;
	uint8_t cfg[PCIE_EP_EMUL_CFG_SIZE];
	uint8_t *bar_backing[PCIE_EP_EMUL_BAR_COUNT];
	struct pcie_ep_emul_aperture apertures[CONFIG_PCIE_EP_EMUL_MAX_APERTURES];
	struct pcie_ep_emul_map maps[CONFIG_PCIE_EP_EMUL_MAX_MAPS];
	struct pcie_ep_emul_dma_chan dma[2];
	/*
	 * Bounded interrupt event queue: one entry per successful
	 * raise_irq, consumed FIFO by the test host. The semaphore is
	 * given once per recorded event, after the lock is dropped, so a
	 * successful take always pairs with a queue entry.
	 */
	struct k_sem irq_sem;
	struct pcie_ep_emul_irq_event irq_events[CONFIG_PCIE_EP_EMUL_MAX_IRQ_EVENTS];
	uint32_t irq_events_head;
	uint32_t irq_events_tail;
	pcie_ep_reset_callback_t reset_cbs[PCIE_RESET_MAX];
	void *reset_cb_args[PCIE_RESET_MAX];
};

static bool pcie_ep_emul_cfg_access_valid(uint32_t offset)
{
	return (offset % sizeof(uint32_t) == 0) &&
	       (offset <= PCIE_EP_EMUL_CFG_SIZE - sizeof(uint32_t));
}

static int pcie_ep_emul_cfg_bar_index(uint32_t offset)
{
	if (offset < PCIE_EP_EMUL_CFG_BAR_BASE ||
	    offset >= PCIE_EP_EMUL_CFG_BAR_BASE + PCIE_EP_EMUL_BAR_COUNT * sizeof(uint32_t)) {
		return -ENOENT;
	}

	return (offset - PCIE_EP_EMUL_CFG_BAR_BASE) / sizeof(uint32_t);
}

/* Offset of the MSI-X capability, right after the MSI one when present. */
static uint32_t pcie_ep_emul_cfg_msix_offset(const struct pcie_ep_emul_config *cfg)
{
	return PCIE_EP_EMUL_CFG_FIRST_CAP + (cfg->msi_vectors > 0 ? PCIE_EP_EMUL_MSI_CAP_SIZE : 0);
}

static void pcie_ep_emul_cfg_init(const struct device *dev)
{
	const struct pcie_ep_emul_config *cfg = dev->config;
	struct pcie_ep_emul_data *data = dev->data;
	uint32_t cap = PCIE_EP_EMUL_CFG_FIRST_CAP;

	sys_put_le32(cfg->vendor_id | (cfg->device_id << 16),
		     &data->cfg[PCIE_EP_EMUL_CFG_VENDOR_DEVICE_ID]);
	sys_put_le32(cfg->revision_id | (cfg->class_code << 8),
		     &data->cfg[PCIE_EP_EMUL_CFG_CLASS_REVISION]);
	/* BARs start unassigned (zero); the host assigns bases via writes. */

	if (cfg->msi_vectors == 0 && cfg->msix_vectors == 0) {
		return;
	}

	sys_put_le32(PCIE_EP_EMUL_STATUS_CAP_LIST, &data->cfg[PCIE_EP_EMUL_CFG_COMMAND_STATUS]);
	sys_put_le32(cap, &data->cfg[PCIE_EP_EMUL_CFG_CAPABILITY_PTR]);

	if (cfg->msi_vectors > 0) {
		uint32_t next = cfg->msix_vectors > 0 ? cap + PCIE_EP_EMUL_MSI_CAP_SIZE : 0;

		/* Multiple Message Capable encodes log2 of the vector count. */
		sys_put_le32(PCIE_EP_EMUL_MSI_CAP_ID | (next << 8) |
				     (LOG2(cfg->msi_vectors) << PCIE_EP_EMUL_MSI_MMC_SHIFT),
			     &data->cfg[cap]);
		cap += PCIE_EP_EMUL_MSI_CAP_SIZE;
	}

	if (cfg->msix_vectors > 0) {
		uint32_t bar = 0;

		/* A build-time assert guarantees one BAR is enabled here. */
		while (cfg->bar_sizes[bar] == 0) {
			bar++;
		}

		sys_put_le32(PCIE_EP_EMUL_MSIX_CAP_ID | ((cfg->msix_vectors - 1)
							 << PCIE_EP_EMUL_MSIX_TABLE_SIZE_SHIFT),
			     &data->cfg[cap]);
		/* Table at offset 0 of the first enabled BAR, PBA after it. */
		sys_put_le32(bar, &data->cfg[cap + 4]);
		sys_put_le32(bar | (cfg->msix_vectors * 16U), &data->cfg[cap + 8]);
	}
}

static int pcie_ep_emul_conf_read(const struct device *dev, uint32_t offset, uint32_t *out)
{
	struct pcie_ep_emul_data *data = dev->data;
	k_spinlock_key_t key;

	if (!pcie_ep_emul_cfg_access_valid(offset)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	*out = sys_get_le32(&data->cfg[offset]);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int pcie_ep_emul_conf_write_op(const struct device *dev, uint32_t offset, uint32_t value)
{
	const struct pcie_ep_emul_config *cfg = dev->config;
	struct pcie_ep_emul_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t cur;
	int ret = 0;
	int bar;

	if (!pcie_ep_emul_cfg_access_valid(offset)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	switch (offset) {
	case PCIE_EP_EMUL_CFG_COMMAND_STATUS:
		cur = sys_get_le32(&data->cfg[offset]);
		cur &= ~PCIE_EP_EMUL_CMD_WRITABLE_MASK;
		cur |= value & PCIE_EP_EMUL_CMD_WRITABLE_MASK;
		sys_put_le32(cur, &data->cfg[offset]);
		break;
	default:
		bar = pcie_ep_emul_cfg_bar_index(offset);
		if (bar >= 0 && cfg->bar_sizes[bar] != 0) {
			sys_put_le32(value & ~(cfg->bar_sizes[bar] - 1U), &data->cfg[offset]);
		} else if (cfg->msi_vectors > 0 && offset == PCIE_EP_EMUL_CFG_FIRST_CAP) {
			uint32_t mme =
				(value & PCIE_EP_EMUL_MSI_MME_MASK) >> PCIE_EP_EMUL_MSI_MME_SHIFT;

			/*
			 * The enable bit and the multiple-message-enable
			 * field are writable. A written MME above MMC is
			 * clamped to MMC: the spec leaves that case
			 * undefined, and clamping keeps the model
			 * deterministic.
			 */
			mme = MIN(mme, LOG2(cfg->msi_vectors));
			cur = sys_get_le32(&data->cfg[offset]);
			cur &= ~(PCIE_EP_EMUL_MSI_CTRL_ENABLE | PCIE_EP_EMUL_MSI_MME_MASK);
			cur |= (value & PCIE_EP_EMUL_MSI_CTRL_ENABLE) |
			       (mme << PCIE_EP_EMUL_MSI_MME_SHIFT);
			sys_put_le32(cur, &data->cfg[offset]);
		} else if (cfg->msix_vectors > 0 && offset == pcie_ep_emul_cfg_msix_offset(cfg)) {
			/* Only the MSI-X enable and function mask bits are writable. */
			cur = sys_get_le32(&data->cfg[offset]);
			cur &= ~(PCIE_EP_EMUL_MSIX_CTRL_ENABLE | PCIE_EP_EMUL_MSIX_CTRL_FUNC_MASK);
			cur |= value &
			       (PCIE_EP_EMUL_MSIX_CTRL_ENABLE | PCIE_EP_EMUL_MSIX_CTRL_FUNC_MASK);
			sys_put_le32(cur, &data->cfg[offset]);
		} else {
			ret = -EPERM;
		}
		break;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void pcie_ep_emul_conf_write(const struct device *dev, uint32_t offset, uint32_t value)
{
	/* The void device API silently ignores invalid writes. */
	(void)pcie_ep_emul_conf_write_op(dev, offset, value);
}

int pcie_ep_emul_host_conf_read(const struct device *dev, uint32_t offset, uint32_t *value)
{
	return pcie_ep_emul_conf_read(dev, offset, value);
}

int pcie_ep_emul_host_conf_write(const struct device *dev, uint32_t offset, uint32_t value)
{
	return pcie_ep_emul_conf_write_op(dev, offset, value);
}

static int pcie_ep_emul_host_bar_access(const struct device *dev, uint32_t bar, uint32_t offset,
					void *dst, const void *src, uint32_t len, bool host_write)
{
	const struct pcie_ep_emul_config *cfg = dev->config;
	struct pcie_ep_emul_data *data = dev->data;
	k_spinlock_key_t key;

	/* A host write copies from src, a host read into dst; the other is NULL. */
	if (bar >= PCIE_EP_EMUL_BAR_COUNT || len == 0 || (host_write ? src : dst) == NULL) {
		return -EINVAL;
	}

	if (len > cfg->bar_sizes[bar] || offset > cfg->bar_sizes[bar] - len) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (host_write) {
		memcpy(&data->bar_backing[bar][offset], src, len);
	} else {
		memcpy(dst, &data->bar_backing[bar][offset], len);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

int pcie_ep_emul_host_bar_read(const struct device *dev, uint32_t bar, uint32_t offset, void *buf,
			       uint32_t len)
{
	return pcie_ep_emul_host_bar_access(dev, bar, offset, buf, NULL, len, false);
}

int pcie_ep_emul_host_bar_write(const struct device *dev, uint32_t bar, uint32_t offset,
				const void *buf, uint32_t len)
{
	return pcie_ep_emul_host_bar_access(dev, bar, offset, NULL, buf, len, true);
}

static bool pcie_ep_emul_range_overflows(uint64_t base, uint64_t len)
{
	return len > UINT64_MAX - base;
}

int pcie_ep_emul_register_aperture(const struct device *dev, uint64_t pcie_base, void *local_buf,
				   uint32_t len)
{
	struct pcie_ep_emul_data *data = dev->data;
	struct pcie_ep_emul_aperture *free_slot = NULL;
	k_spinlock_key_t key;
	int ret = -ENOMEM;

	if (local_buf == NULL || len == 0 || pcie_ep_emul_range_overflows(pcie_base, len)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	for (int i = 0; i < ARRAY_SIZE(data->apertures); i++) {
		struct pcie_ep_emul_aperture *ap = &data->apertures[i];

		if (!ap->active) {
			if (free_slot == NULL) {
				free_slot = ap;
			}
			continue;
		}

		if (pcie_base < ap->pcie_base + ap->len && ap->pcie_base < pcie_base + len) {
			ret = -EALREADY;
			goto out;
		}
	}

	if (free_slot != NULL) {
		free_slot->pcie_base = pcie_base;
		free_slot->local = local_buf;
		free_slot->len = len;
		free_slot->active = true;
		ret = 0;
	}
out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

int pcie_ep_emul_unregister_aperture(const struct device *dev, uint64_t pcie_base)
{
	struct pcie_ep_emul_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = -ENOENT;

	key = k_spin_lock(&data->lock);

	for (int i = 0; i < ARRAY_SIZE(data->apertures); i++) {
		struct pcie_ep_emul_aperture *ap = &data->apertures[i];

		if (!ap->active || ap->pcie_base != pcie_base) {
			continue;
		}

		for (int j = 0; j < ARRAY_SIZE(data->maps); j++) {
			/*
			 * A record pins the aperture while it is active or
			 * while an in-flight DMA transfer references it.
			 */
			if (data->maps[j].aperture_idx == i &&
			    (data->maps[j].active || data->maps[j].dma_inflight != 0U)) {
				ret = -EBUSY;
				goto out;
			}
		}

		ap->active = false;
		ret = 0;
		goto out;
	}
out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static struct pcie_ep_emul_aperture *pcie_ep_emul_find_aperture(struct pcie_ep_emul_data *data,
								uint64_t pcie_addr, uint32_t size,
								int *idx)
{
	for (int i = 0; i < ARRAY_SIZE(data->apertures); i++) {
		struct pcie_ep_emul_aperture *ap = &data->apertures[i];
		uint64_t offset;

		if (!ap->active || pcie_addr < ap->pcie_base) {
			continue;
		}

		offset = pcie_addr - ap->pcie_base;
		if (offset <= ap->len && size <= ap->len - offset) {
			*idx = i;
			return ap;
		}
	}

	return NULL;
}

static int pcie_ep_emul_map_addr(const struct device *dev, uint64_t pcie_addr,
				 uint64_t *mapped_addr, uint32_t size,
				 enum pcie_ob_mem_type ob_mem_type)
{
	struct pcie_ep_emul_data *data = dev->data;
	struct pcie_ep_emul_aperture *ap;
	k_spinlock_key_t key;
	uint64_t candidate;
	int aperture_idx;
	int ret = -ENOMEM;

	ARG_UNUSED(ob_mem_type);

	/*
	 * The mapping size is returned as int on success, so sizes that do
	 * not fit an int are rejected before any record is allocated rather
	 * than truncating the return value.
	 */
	if (size == 0 || size > (uint32_t)INT_MAX ||
	    pcie_ep_emul_range_overflows(pcie_addr, size)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	ap = pcie_ep_emul_find_aperture(data, pcie_addr, size, &aperture_idx);
	if (ap == NULL) {
		ret = -ENOTSUP;
		goto out;
	}

	candidate = (uint64_t)(uintptr_t)ap->local + (pcie_addr - ap->pcie_base);

	/*
	 * Each live host address has exactly one owning record, so two
	 * callers cannot hold mappings to the same address at once; records
	 * still pinned by an in-flight DMA transfer count as live. Note
	 * the inherent limit of the void unmap_addr(dev, addr) API: a
	 * re-issued mapping of the same range returns the same bare
	 * address, so unmapping a stale (already released) handle of
	 * identical value is a caller error the API cannot detect, the
	 * same class as a double free.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(data->maps); i++) {
		struct pcie_ep_emul_map *map = &data->maps[i];

		if ((map->active || map->dma_inflight != 0U) && map->mapped_addr == candidate) {
			ret = -EALREADY;
			goto out;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(data->maps); i++) {
		struct pcie_ep_emul_map *map = &data->maps[i];

		/*
		 * A draining record (unmapped but still pinned by an
		 * in-flight DMA transfer) is not reusable until the
		 * transfer completes.
		 */
		if (map->active || map->dma_inflight != 0U) {
			continue;
		}

		map->mapped_addr = candidate;
		map->size = size;
		map->aperture_idx = aperture_idx;
		map->active = true;
		*mapped_addr = map->mapped_addr;
		ret = (int)size;
		goto out;
	}
out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static void pcie_ep_emul_unmap_addr(const struct device *dev, uint64_t mapped_addr)
{
	struct pcie_ep_emul_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	for (int i = 0; i < ARRAY_SIZE(data->maps); i++) {
		if (data->maps[i].active && data->maps[i].mapped_addr == mapped_addr) {
			/*
			 * Deactivate only: an in-flight DMA transfer may still
			 * pin the record (dma_inflight), in which case the slot
			 * stays reserved and keeps blocking aperture
			 * unregistration until the transfer completes.
			 */
			data->maps[i].active = false;
			break;
		}
	}

	k_spin_unlock(&data->lock, key);
}

static int pcie_ep_emul_raise_irq(const struct device *dev, enum pci_ep_irq_type irq_type,
				  uint32_t irq_num)
{
	const struct pcie_ep_emul_config *cfg = dev->config;
	struct pcie_ep_emul_data *data = dev->data;
	struct pcie_ep_emul_irq_event *event;
	k_spinlock_key_t key;
	uint32_t ctrl;
	int ret = 0;

	switch (irq_type) {
	case PCIE_EP_IRQ_LEGACY:
		if (!cfg->legacy_irq) {
			return -ENOTSUP;
		}
		break;
	case PCIE_EP_IRQ_MSI:
		if (cfg->msi_vectors == 0) {
			return -ENOTSUP;
		}
		if (irq_num >= cfg->msi_vectors) {
			return -EINVAL;
		}
		break;
	case PCIE_EP_IRQ_MSIX:
		if (cfg->msix_vectors == 0) {
			return -ENOTSUP;
		}
		if (irq_num >= cfg->msix_vectors) {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/*
	 * Like hardware, raises are honored only while the host has the
	 * capability enabled: MSI additionally bounds vectors by the
	 * host-programmed allocation (2^MME, one vector by default), and
	 * MSI-X raises are suppressed while the function mask is set.
	 * All checks run before any event is recorded.
	 */
	switch (irq_type) {
	case PCIE_EP_IRQ_MSI:
		ctrl = sys_get_le32(&data->cfg[PCIE_EP_EMUL_CFG_FIRST_CAP]);
		if ((ctrl & PCIE_EP_EMUL_MSI_CTRL_ENABLE) == 0) {
			ret = -ENOTSUP;
		} else if (irq_num >=
			   BIT((ctrl & PCIE_EP_EMUL_MSI_MME_MASK) >> PCIE_EP_EMUL_MSI_MME_SHIFT)) {
			ret = -EINVAL;
		}
		break;
	case PCIE_EP_IRQ_MSIX:
		ctrl = sys_get_le32(&data->cfg[pcie_ep_emul_cfg_msix_offset(cfg)]);
		if ((ctrl & PCIE_EP_EMUL_MSIX_CTRL_ENABLE) == 0 ||
		    (ctrl & PCIE_EP_EMUL_MSIX_CTRL_FUNC_MASK) != 0) {
			ret = -ENOTSUP;
		}
		break;
	default:
		break;
	}
	if (ret != 0) {
		goto out;
	}

	if (data->irq_events_head - data->irq_events_tail >= ARRAY_SIZE(data->irq_events)) {
		ret = -ENOSPC;
		goto out;
	}

	event = &data->irq_events[data->irq_events_head % ARRAY_SIZE(data->irq_events)];
	event->type = irq_type;
	event->vector = irq_type == PCIE_EP_IRQ_LEGACY ? 0 : irq_num;
	data->irq_events_head++;

out:
	k_spin_unlock(&data->lock, key);

	if (ret == 0) {
		/* Notify waiting test hosts only after the event is visible. */
		k_sem_give(&data->irq_sem);
	}

	return ret;
}

int pcie_ep_emul_wait_irq_event(const struct device *dev, struct pcie_ep_emul_irq_event *event,
				k_timeout_t timeout)
{
	struct pcie_ep_emul_data *data = dev->data;
	k_spinlock_key_t key;

	if (event == NULL) {
		return -EINVAL;
	}

	if (k_sem_take(&data->irq_sem, timeout) != 0) {
		return -EAGAIN;
	}

	key = k_spin_lock(&data->lock);
	*event = data->irq_events[data->irq_events_tail % ARRAY_SIZE(data->irq_events)];
	data->irq_events_tail++;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int pcie_ep_emul_register_reset_cb(const struct device *dev, enum pcie_reset reset,
					  pcie_ep_reset_callback_t cb, void *arg)
{
	struct pcie_ep_emul_data *data = dev->data;
	k_spinlock_key_t key;

	if ((uint32_t)reset >= PCIE_RESET_MAX || cb == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	data->reset_cbs[reset] = cb;
	data->reset_cb_args[reset] = arg;
	k_spin_unlock(&data->lock, key);

	return 0;
}

struct pcie_ep_emul_reset_inject {
	const struct device *dev;
	enum pcie_reset reset;
};

static void pcie_ep_emul_reset_isr(const void *param)
{
	const struct pcie_ep_emul_reset_inject *inject = param;
	struct pcie_ep_emul_data *data = inject->dev->data;
	pcie_ep_reset_callback_t cb;
	k_spinlock_key_t key;
	void *arg;

	key = k_spin_lock(&data->lock);
	cb = data->reset_cbs[inject->reset];
	arg = data->reset_cb_args[inject->reset];
	k_spin_unlock(&data->lock, key);

	if (cb != NULL) {
		cb(arg);
	}
}

int pcie_ep_emul_inject_reset(const struct device *dev, enum pcie_reset reset)
{
	struct pcie_ep_emul_reset_inject inject = {
		.dev = dev,
		.reset = reset,
	};

	if ((uint32_t)reset >= PCIE_RESET_MAX) {
		return -EINVAL;
	}

	/* Deliver synchronously in interrupt context, like real hardware. */
	irq_offload(pcie_ep_emul_reset_isr, &inject);

	return 0;
}

static void pcie_ep_emul_dma_callback(const struct device *dma_dev, void *user_data,
				      uint32_t channel, int status)
{
	struct pcie_ep_emul_dma_chan *chan = user_data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	/* Record the status before releasing the waiter. */
	chan->status = status;
	k_sem_give(&chan->done);
}

/*
 * The host side of a DMA transfer must be wholly covered by an active
 * map record produced by map_addr; containment in a registered aperture
 * alone is not sufficient, so raw or already unmapped addresses are
 * rejected. The caller pins the returned record (dma_inflight) for the
 * whole transfer, so unmapping the record or unregistering its aperture
 * mid-transfer cannot release the host buffer underneath the copy: the
 * record stays reserved until the transfer completes, and the aperture
 * cannot be unregistered while any record referencing it is active or
 * pinned. Caller must hold data->lock.
 */
static struct pcie_ep_emul_map *pcie_ep_emul_mapped_covered(struct pcie_ep_emul_data *data,
							    uint64_t mapped_addr, uint32_t size)
{
	for (int i = 0; i < ARRAY_SIZE(data->maps); i++) {
		struct pcie_ep_emul_map *map = &data->maps[i];
		uint64_t offset;

		if (!map->active || mapped_addr < map->mapped_addr) {
			continue;
		}

		offset = mapped_addr - map->mapped_addr;
		if (offset <= map->size && size <= map->size - offset) {
			return map;
		}
	}

	return NULL;
}

static int pcie_ep_emul_dma_xfer(const struct device *dev, uint64_t mapped_addr,
				 uintptr_t local_addr, uint32_t size, enum xfer_direction dir)
{
	const struct pcie_ep_emul_config *cfg = dev->config;
	struct pcie_ep_emul_data *data = dev->data;
	struct pcie_ep_emul_dma_chan *chan;
	struct dma_block_config block_cfg;
	const struct device *dma_dev;
	struct pcie_ep_emul_map *pin;
	struct dma_config dma_cfg;
	k_spinlock_key_t key;
	int ret;

	if (dir != HOST_TO_DEVICE && dir != DEVICE_TO_HOST) {
		return -EINVAL;
	}

	if (local_addr == 0 || size == 0 || pcie_ep_emul_range_overflows(mapped_addr, size)) {
		return -EINVAL;
	}

	dma_dev = cfg->dma_devs[dir];
	if (dma_dev == NULL) {
		/* The instance has no dmas property: DMA is not available. */
		return -ENODEV;
	}
	if (!device_is_ready(dma_dev)) {
		return -ENODEV;
	}

	/*
	 * Pin the covering map record for the whole transfer so that a
	 * concurrent unmap_addr() cannot hand its slot out again and a
	 * concurrent aperture unregistration is blocked (-EBUSY) until the
	 * copy below has completed.
	 */
	key = k_spin_lock(&data->lock);
	pin = pcie_ep_emul_mapped_covered(data, mapped_addr, size);
	if (pin != NULL) {
		pin->dma_inflight++;
	}
	k_spin_unlock(&data->lock, key);
	if (pin == NULL) {
		return -ENOTSUP;
	}

	chan = &data->dma[dir];

	/*
	 * Hold the channel for the whole configure..completion cycle so
	 * concurrent users of the same direction cannot interleave. A
	 * K_FOREVER wait cannot fail; assert the return value rather than
	 * discarding it.
	 */
	/* clang-format off */
	__ASSERT_EVAL((void)k_mutex_lock(&chan->lock, K_FOREVER),
		int lock_ret = k_mutex_lock(&chan->lock, K_FOREVER),
		lock_ret == 0, "k_mutex_lock failed");
	/* clang-format on */

	chan->status = -EIO;

	block_cfg = (struct dma_block_config){
		.source_address = dir == DEVICE_TO_HOST ? (uint64_t)local_addr : mapped_addr,
		.dest_address = dir == DEVICE_TO_HOST ? mapped_addr : (uint64_t)local_addr,
		.block_size = size,
	};
	dma_cfg = (struct dma_config){
		.dma_slot = 0,
		.channel_direction = MEMORY_TO_MEMORY,
		.complete_callback_en = 1,
		.source_data_size = 1,
		.dest_data_size = 1,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.block_count = 1,
		.head_block = &block_cfg,
		.user_data = chan,
		.dma_callback = pcie_ep_emul_dma_callback,
	};

	/*
	 * Configure-time -EBUSY flags a channel grabbed by another user
	 * directly on the controller; cross-instance sharing is already
	 * excluded by the exclusive channel claims made at init.
	 */
	ret = dma_config(dma_dev, cfg->dma_channels[dir], &dma_cfg);
	if (ret == 0) {
		ret = dma_start(dma_dev, cfg->dma_channels[dir]);
		if (ret != 0) {
			/* Undo our LOADED state so the channel stays usable. */
			(void)dma_stop(dma_dev, cfg->dma_channels[dir]);
		}
	}
	if (ret == 0) {
		/*
		 * Once dma_start() accepted the transfer, the callback
		 * always fires from the controller workqueue, with
		 * DMA_STATUS_COMPLETE on success or a negative status on
		 * cancellation; the waiter therefore cannot sleep forever
		 * and the pinned mapping stays reserved through the copy.
		 */
		/* clang-format off */
		__ASSERT_EVAL((void)k_sem_take(&chan->done, K_FOREVER),
			int take_ret = k_sem_take(&chan->done, K_FOREVER),
			take_ret == 0, "k_sem_take failed");
		/* clang-format on */
		if (chan->status == DMA_STATUS_COMPLETE) {
			ret = 0;
		} else {
			ret = chan->status < 0 ? chan->status : -EIO;
		}
	}

	/* Unlocking a mutex this thread holds cannot fail either. */
	/* clang-format off */
	__ASSERT_EVAL((void)k_mutex_unlock(&chan->lock),
		int unlock_ret = k_mutex_unlock(&chan->lock),
		unlock_ret == 0, "k_mutex_unlock failed");
	/* clang-format on */

	/* The copy is over: release the pin taken before the transfer. */
	key = k_spin_lock(&data->lock);
	pin->dma_inflight--;
	k_spin_unlock(&data->lock, key);

	return ret;
}

static DEVICE_API(pcie_ep, pcie_ep_emul_api) = {
	.conf_read = pcie_ep_emul_conf_read,
	.conf_write = pcie_ep_emul_conf_write,
	.map_addr = pcie_ep_emul_map_addr,
	.unmap_addr = pcie_ep_emul_unmap_addr,
	.raise_irq = pcie_ep_emul_raise_irq,
	.register_reset_cb = pcie_ep_emul_register_reset_cb,
	.dma_xfer = pcie_ep_emul_dma_xfer,
};

/*
 * Claim each named channel from its controller so that two endpoint
 * instances cannot drive the same channel. dma_request_channel()
 * allocates through the controller's dma_context atomic bitmask, so
 * claims are race-free across instances; the claim is held for the
 * whole device lifetime and never released. The channel number named in
 * devicetree is passed to the controller's channel filter by address, so
 * claims do not depend on device init order: receiving any other channel,
 * or no channel at all, means the named channel was taken by another
 * user. On any failure all channels already claimed by this init are
 * released again, so a device that never becomes ready leaks no claims.
 * filter_param is not standardized by the DMA API, so the by-address
 * uint32_t channel-number encoding is only valid for controllers
 * implementing the dma_emul filter convention; the dmas controllers are
 * restricted to zephyr,dma-emul by build-time assertion.
 */
static int pcie_ep_emul_claim_dma_channels(const struct device *dev)
{
	const struct pcie_ep_emul_config *cfg = dev->config;
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(cfg->dma_devs); i++) {
		const struct device *dma_dev = cfg->dma_devs[i];
		uint32_t channel = cfg->dma_channels[i];
		int claimed;

		if (dma_dev == NULL) {
			continue;
		}

		if (!device_is_ready(dma_dev)) {
			LOG_ERR("%s: DMA controller %s is not ready", dev->name, dma_dev->name);
			ret = -ENODEV;
			break;
		}

		claimed = dma_request_channel(dma_dev, &channel);
		if (claimed >= 0 && (uint32_t)claimed != cfg->dma_channels[i]) {
			dma_release_channel(dma_dev, (uint32_t)claimed);
			claimed = -EBUSY;
		}
		if (claimed < 0) {
			LOG_ERR("%s: cannot claim channel %u of %s", dev->name,
				cfg->dma_channels[i], dma_dev->name);
			ret = -EBUSY;
			break;
		}
	}

	if (ret != 0) {
		/* Roll back every channel this init already claimed. */
		for (int j = 0; j < i; j++) {
			if (cfg->dma_devs[j] != NULL) {
				dma_release_channel(cfg->dma_devs[j], cfg->dma_channels[j]);
			}
		}
	}

	return ret;
}

static int pcie_ep_emul_init(const struct device *dev)
{
	const struct pcie_ep_emul_config *cfg = dev->config;
	struct pcie_ep_emul_data *data = dev->data;

	/* The two directions of one instance must use distinct channels. */
	if (cfg->dma_devs[0] != NULL && cfg->dma_devs[0] == cfg->dma_devs[1] &&
	    cfg->dma_channels[0] == cfg->dma_channels[1]) {
		return -ENODEV;
	}

	if (pcie_ep_emul_claim_dma_channels(dev) != 0) {
		return -ENODEV;
	}

	k_sem_init(&data->irq_sem, 0, CONFIG_PCIE_EP_EMUL_MAX_IRQ_EVENTS);
	for (int i = 0; i < ARRAY_SIZE(data->dma); i++) {
		k_mutex_init(&data->dma[i].lock);
		k_sem_init(&data->dma[i].done, 0, 1);
	}
	pcie_ep_emul_cfg_init(dev);

	return 0;
}

#define PCIE_EP_EMUL_BAR_SIZE_ASSERT(inst, idx)                                                    \
	BUILD_ASSERT(                                                                              \
		DT_INST_PROP_BY_IDX(inst, bar_sizes, idx) == 0 ||                                  \
			(IS_POWER_OF_TWO(DT_INST_PROP_BY_IDX(inst, bar_sizes, idx)) &&             \
			 DT_INST_PROP_BY_IDX(inst, bar_sizes, idx) >= PCIE_EP_EMUL_BAR_MIN_SIZE),  \
		"bar-sizes entries must be zero or a power of two >= 4096")

#define PCIE_EP_EMUL_BAR_BACKING_DEFINE(inst, idx)                                                 \
	static uint8_t pcie_ep_emul_bar_backing_##inst##_##idx[MAX(                                \
		DT_INST_PROP_BY_IDX(inst, bar_sizes, idx), 1)]

#define PCIE_EP_EMUL_BAR_BACKING_PTR(inst, idx)                                                    \
	DT_INST_PROP_BY_IDX(inst, bar_sizes, idx) != 0 ? pcie_ep_emul_bar_backing_##inst##_##idx   \
						       : NULL

#define PCIE_EP_EMUL_DMA_DEV(inst, name)                                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dmas),                                          \
		    (DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, name))), (NULL))

#define PCIE_EP_EMUL_DMA_CHANNEL(inst, name)                                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dmas),                                          \
		    (DT_INST_DMAS_CELL_BY_NAME(inst, name, channel)), (0))

/*
 * Exact-channel claiming passes the devicetree channel number to the
 * controller's channel filter as a pointer to uint32_t. The DMA API does
 * not standardize filter_param; that encoding is the dma_emul
 * convention, so restrict dmas controllers to zephyr,dma-emul at build
 * time rather than assuming any controller honors it.
 */
#define PCIE_EP_EMUL_DMA_CTRL_ASSERT(node_id, prop, idx)                                           \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_PHANDLE_BY_IDX(node_id, prop, idx), zephyr_dma_emul),   \
		     "zephyr,pcie-ep-emul dmas controllers must be zephyr,dma-emul: "              \
		     "exact-channel claiming relies on the dma_emul channel-filter encoding");

#define PCIE_EP_EMUL_DMA_CTRLS_ASSERT(inst)                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dmas),                                          \
		    (DT_INST_FOREACH_PROP_ELEM(inst, dmas, PCIE_EP_EMUL_DMA_CTRL_ASSERT)), ())

/* Size of the first enabled BAR, or zero when all BARs are disabled. */
#define PCIE_EP_EMUL_FIRST_BAR_SIZE(inst)                                                          \
	(DT_INST_PROP_BY_IDX(inst, bar_sizes, 0) != 0   ? DT_INST_PROP_BY_IDX(inst, bar_sizes, 0)  \
	 : DT_INST_PROP_BY_IDX(inst, bar_sizes, 1) != 0 ? DT_INST_PROP_BY_IDX(inst, bar_sizes, 1)  \
	 : DT_INST_PROP_BY_IDX(inst, bar_sizes, 2) != 0 ? DT_INST_PROP_BY_IDX(inst, bar_sizes, 2)  \
	 : DT_INST_PROP_BY_IDX(inst, bar_sizes, 3) != 0 ? DT_INST_PROP_BY_IDX(inst, bar_sizes, 3)  \
	 : DT_INST_PROP_BY_IDX(inst, bar_sizes, 4) != 0 ? DT_INST_PROP_BY_IDX(inst, bar_sizes, 4)  \
							: DT_INST_PROP_BY_IDX(inst, bar_sizes, 5))

/* PBA storage for a vector count: one bit per vector, in 8-byte multiples. */
#define PCIE_EP_EMUL_MSIX_PBA_SIZE(vectors) ROUND_UP(DIV_ROUND_UP(vectors, 8), 8)

#define PCIE_EP_EMUL_MSIX_FITS_ASSERT(inst)                                                        \
	BUILD_ASSERT(DT_INST_PROP(inst, msix_vectors) == 0 ||                                      \
			     DT_INST_PROP(inst, msix_vectors) * 16U +                              \
					     PCIE_EP_EMUL_MSIX_PBA_SIZE(                           \
						     DT_INST_PROP(inst, msix_vectors)) <=          \
				     PCIE_EP_EMUL_FIRST_BAR_SIZE(inst),                            \
		     "msix-vectors table and PBA must fit in the first enabled BAR")

#define PCIE_EP_EMUL_DEFINE(inst)                                                                  \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, bar_sizes) == PCIE_EP_EMUL_BAR_COUNT,                  \
		     "bar-sizes must have exactly 6 entries");                                     \
	BUILD_ASSERT(DT_INST_PROP(inst, vendor_id) <= 0xFFFFU,                                     \
		     "vendor-id must fit the 16-bit configuration space field");                   \
	BUILD_ASSERT(DT_INST_PROP(inst, device_id) <= 0xFFFFU,                                     \
		     "device-id must fit the 16-bit configuration space field");                   \
	BUILD_ASSERT(DT_INST_PROP(inst, class_code) <= 0xFFFFFFU,                                  \
		     "class-code must fit the 24-bit configuration space field");                  \
	BUILD_ASSERT(DT_INST_PROP(inst, revision_id) <= 0xFFU,                                     \
		     "revision-id must fit the 8-bit configuration space field");                  \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(inst, dmas) ||                                         \
			     (DT_INST_PROP_LEN_OR(inst, dmas, 0) == 2 &&                           \
			      DT_INST_PROP_LEN_OR(inst, dma_names, 0) == 2),                       \
		     "dmas and dma-names must have exactly two entries when present");             \
	PCIE_EP_EMUL_DMA_CTRLS_ASSERT(inst);                                                       \
	BUILD_ASSERT(DT_INST_PROP(inst, msi_vectors) == 0 ||                                       \
			     (IS_POWER_OF_TWO(DT_INST_PROP(inst, msi_vectors)) &&                  \
			      DT_INST_PROP(inst, msi_vectors) <= 32U),                             \
		     "msi-vectors must be zero or a power of two up to 32");                       \
	BUILD_ASSERT(DT_INST_PROP(inst, msix_vectors) <= 2048U,                                    \
		     "msix-vectors must fit the 11-bit table size field");                         \
	BUILD_ASSERT(DT_INST_PROP(inst, msix_vectors) == 0 ||                                      \
			     DT_INST_PROP_BY_IDX(inst, bar_sizes, 0) != 0 ||                       \
			     DT_INST_PROP_BY_IDX(inst, bar_sizes, 1) != 0 ||                       \
			     DT_INST_PROP_BY_IDX(inst, bar_sizes, 2) != 0 ||                       \
			     DT_INST_PROP_BY_IDX(inst, bar_sizes, 3) != 0 ||                       \
			     DT_INST_PROP_BY_IDX(inst, bar_sizes, 4) != 0 ||                       \
			     DT_INST_PROP_BY_IDX(inst, bar_sizes, 5) != 0,                         \
		     "msix-vectors requires at least one enabled BAR");                            \
	PCIE_EP_EMUL_MSIX_FITS_ASSERT(inst);                                                       \
	PCIE_EP_EMUL_BAR_SIZE_ASSERT(inst, 0);                                                     \
	PCIE_EP_EMUL_BAR_SIZE_ASSERT(inst, 1);                                                     \
	PCIE_EP_EMUL_BAR_SIZE_ASSERT(inst, 2);                                                     \
	PCIE_EP_EMUL_BAR_SIZE_ASSERT(inst, 3);                                                     \
	PCIE_EP_EMUL_BAR_SIZE_ASSERT(inst, 4);                                                     \
	PCIE_EP_EMUL_BAR_SIZE_ASSERT(inst, 5);                                                     \
	PCIE_EP_EMUL_BAR_BACKING_DEFINE(inst, 0);                                                  \
	PCIE_EP_EMUL_BAR_BACKING_DEFINE(inst, 1);                                                  \
	PCIE_EP_EMUL_BAR_BACKING_DEFINE(inst, 2);                                                  \
	PCIE_EP_EMUL_BAR_BACKING_DEFINE(inst, 3);                                                  \
	PCIE_EP_EMUL_BAR_BACKING_DEFINE(inst, 4);                                                  \
	PCIE_EP_EMUL_BAR_BACKING_DEFINE(inst, 5);                                                  \
	static const struct pcie_ep_emul_config pcie_ep_emul_config_##inst = {                     \
		.vendor_id = DT_INST_PROP(inst, vendor_id),                                        \
		.device_id = DT_INST_PROP(inst, device_id),                                        \
		.class_code = DT_INST_PROP(inst, class_code),                                      \
		.revision_id = DT_INST_PROP(inst, revision_id),                                    \
		.bar_sizes = DT_INST_PROP(inst, bar_sizes),                                        \
		.msi_vectors = DT_INST_PROP(inst, msi_vectors),                                    \
		.msix_vectors = DT_INST_PROP(inst, msix_vectors),                                  \
		.legacy_irq = DT_INST_PROP(inst, legacy_irq),                                      \
		.dma_devs = {PCIE_EP_EMUL_DMA_DEV(inst, host_to_device),                           \
			     PCIE_EP_EMUL_DMA_DEV(inst, device_to_host)},                          \
		.dma_channels = {PCIE_EP_EMUL_DMA_CHANNEL(inst, host_to_device),                   \
				 PCIE_EP_EMUL_DMA_CHANNEL(inst, device_to_host)},                  \
	};                                                                                         \
	static struct pcie_ep_emul_data pcie_ep_emul_data_##inst = {                               \
		.bar_backing = {PCIE_EP_EMUL_BAR_BACKING_PTR(inst, 0),                             \
				PCIE_EP_EMUL_BAR_BACKING_PTR(inst, 1),                             \
				PCIE_EP_EMUL_BAR_BACKING_PTR(inst, 2),                             \
				PCIE_EP_EMUL_BAR_BACKING_PTR(inst, 3),                             \
				PCIE_EP_EMUL_BAR_BACKING_PTR(inst, 4),                             \
				PCIE_EP_EMUL_BAR_BACKING_PTR(inst, 5)},                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &pcie_ep_emul_init, NULL, &pcie_ep_emul_data_##inst,           \
			      &pcie_ep_emul_config_##inst, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &pcie_ep_emul_api);

DT_INST_FOREACH_STATUS_OKAY(PCIE_EP_EMUL_DEFINE)
