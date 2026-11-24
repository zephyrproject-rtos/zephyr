/*
 * Copyright 2021 BayLibre, SAS
 * Copyright 2025 NXP
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_gicv3_its, LOG_LEVEL_ERR);

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/bitarray.h>

#if defined(CONFIG_ARM64) && !defined(CONFIG_DCACHE)
#include <zephyr/arch/cache.h>
#endif

#include "intc_gic_common_priv.h"
#include "intc_gicv3_priv.h"

#define DT_DRV_COMPAT   arm_gic_v3_its

/*
 * Current ITS implementation only handle GICv3 ITS physical interruption generation
 * Implementation is designed for the PCIe MSI/MSI-X use-case in mind.
 */

#define GITS_BASER_NR_REGS              8

/* convenient access to all redistributors base address */
extern mem_addr_t gic_rdists[CONFIG_MP_MAX_NUM_CPUS];

#define SIZE_256                        256
#define SIZE_4K                         KB(4)
#define SIZE_16K                        KB(16)
#define SIZE_64K                        KB(64)

struct its_cmd_block {
	uint64_t raw_cmd[4];
};

#define ITS_CMD_QUEUE_SIZE              SIZE_64K
#define ITS_CMD_QUEUE_NR_ENTRIES        (ITS_CMD_QUEUE_SIZE / sizeof(struct its_cmd_block))
#define ITS_MAX_DEVICE_ID_BITS          16U
#define ITS_MAX_BASER_ENTRY_SIZE        32U
#define ITS_MAX_ITT_ENTRY_SIZE          (GITS_TYPER_ITT_ENTRY_SIZE_MASK + 1U)
#define ITS_EVENT_BITMAP_BITS_PER_WORD  32U
/* The level 1 entry size is a 64bit pointer */
#define GITS_LVL1_ENTRY_SIZE (8UL)

#define ITS_LPI_ID_BITS       DT_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(arm_gic_v3), zephyr_lpi_id_bits)
#define ITS_DEVICE_ID_BITS(n) DT_INST_PROP(n, zephyr_device_id_bits)
#define ITS_ITT_BYTES         (CONFIG_GIC_V3_ITS_MAX_VECTORS * ITS_MAX_ITT_ENTRY_SIZE)
#define ITS_ITT_SLOT_SIZE     ROUND_UP(ITS_ITT_BYTES, SIZE_256)
#define ITS_COLLECTION_TABLE_BYTES (ITS_MAX_BASER_ENTRY_SIZE * CONFIG_MP_MAX_NUM_CPUS)
#define ITS_COLLECTION_TABLE_SIZE  ROUND_UP(ITS_COLLECTION_TABLE_BYTES, SIZE_64K)

/* Size the direct Device table for the largest BASER entry and page sizes. */
#define ITS_FLAT_TABLE_SIZE(n)                                                                     \
	ROUND_UP(BIT(ITS_DEVICE_ID_BITS(n)) * ITS_MAX_BASER_ENTRY_SIZE, SIZE_64K)
#define ITS_DEVICE_TABLE_SIZE(n)                                                                   \
	MIN(ITS_FLAT_TABLE_SIZE(n), DT_INST_PROP_OR(n, zephyr_device_table_size, SIZE_MAX))
#define ITS_BASER_LAYOUT                                                                           \
	(MASK(GITS_BASER_PAGE_SIZE) | MASK(GITS_BASER_INDIRECT) | MASK(GITS_BASER_SIZE) |          \
	 MASK(GITS_BASER_ADDR))
#define ITS_BASER_ATTRIBUTES                                                                       \
	(MASK(GITS_BASER_SHAREABILITY) | MASK(GITS_BASER_INNER_CACHE) |                            \
	 MASK(GITS_BASER_OUTER_CACHE))

BUILD_ASSERT((CONFIG_GIC_V3_ITS_MAX_VECTORS >= 2U) &&
		     IS_POWER_OF_TWO(CONFIG_GIC_V3_ITS_MAX_VECTORS),
	     "CONFIG_GIC_V3_ITS_MAX_VECTORS must be a power of two and at least 2");
BUILD_ASSERT(CONFIG_NUM_IRQS > GIC_LPI_INT_BASE,
	     "CONFIG_NUM_IRQS must exceed 8192 when GIC_V3_ITS is enabled");

#define ITS_LPI_BITMAP_BITS                                                                        \
	((MIN(CONFIG_NUM_IRQS, BIT(ITS_LPI_ID_BITS)) > GIC_LPI_INT_BASE)                           \
		 ? (MIN(CONFIG_NUM_IRQS, BIT(ITS_LPI_ID_BITS)) - GIC_LPI_INT_BASE)                 \
		 : 1U)
#define ITS_EVENT_BITMAP_WORDS                                                                     \
	DIV_ROUND_UP(CONFIG_GIC_V3_ITS_MAX_VECTORS, ITS_EVENT_BITMAP_BITS_PER_WORD)

SYS_BITARRAY_DEFINE_STATIC(its_lpi_intids, ITS_LPI_BITMAP_BITS);

struct its_device_slot {
	uint32_t device_id;
	unsigned int nites;
	/* Includes mappings in progress to prevent duplicate EventIDs. */
	uint32_t event_id_bitmap[ITS_EVENT_BITMAP_WORDS];
	/* Owned until mapping or rollback completes. */
	uint32_t event_id_busy_bitmap[ITS_EVENT_BITMAP_WORDS];
	bool used;
	int status;
};

struct gicv3_its_data {
	mm_reg_t base;
	struct its_cmd_block *cmd_base;
	struct its_cmd_block *cmd_write;
	uint32_t cmd_read;
	uint64_t cmd_posted;
	uint64_t cmd_completed;
	bool tables_need_flush;
	bool cmd_queue_needs_flush;
	uint64_t *indirect_dev_l1_table;
	size_t indirect_dev_entries_per_page;
	unsigned int device_id_bits;
	unsigned int lpi_intid_limit;
	ATOMIC_DEFINE(mapped_collections, CONFIG_MP_MAX_NUM_CPUS);
	ATOMIC_DEFINE(mapping_collections, CONFIG_MP_MAX_NUM_CPUS);
	struct its_device_slot device_slots[CONFIG_GIC_V3_ITS_MAX_DEVICES];
	uint8_t *next_indirect_page;
	size_t indirect_page_size;
	size_t indirect_pages_left;
	struct k_spinlock lock;
};

struct gicv3_its_config {
	uintptr_t base_addr;
	size_t base_size;
	struct its_cmd_block *cmd_queue;
	size_t cmd_queue_size;
	uint8_t *device_table;
	size_t device_table_size;
	uint8_t *collection_table;
	size_t collection_table_size;
	uint8_t (*itts)[ITS_ITT_SLOT_SIZE];
	unsigned int device_id_bits;
};

static inline int fls_z(unsigned int x)
{
	unsigned int bits = sizeof(x) * 8;
	unsigned int cmp = 1 << (bits - 1);

	while (bits) {
		if (x & cmp) {
			return bits;
		}
		cmp >>= 1;
		bits--;
	}

	return 0;
}

/* Poll for up to 500ms. */
#define WAIT_QUIESCENT 500

static int its_force_quiescent(struct gicv3_its_data *data)
{
	unsigned int count = WAIT_QUIESCENT;
	uint32_t reg = sys_read32(data->base + GITS_CTLR);

	if (GITS_CTLR_ENABLED_GET(reg)) {
		/* Disable ITS */
		reg &= ~MASK(GITS_CTLR_ENABLED);
		sys_write32(reg, data->base + GITS_CTLR);
		reg = sys_read32(data->base + GITS_CTLR);
	}

	while (1) {
		if (GITS_CTLR_QUIESCENT_GET(reg)) {
			return 0;
		}

		count--;
		if (!count) {
			return -EBUSY;
		}

		k_busy_wait(USEC_PER_MSEC);
		reg = sys_read32(data->base + GITS_CTLR);
	}

	return 0;
}

static const char *const its_base_type_string[] = {
	[GITS_BASER_TYPE_DEVICE] = "Devices",
	[GITS_BASER_TYPE_COLLECTION] = "Interrupt Collections",
};

static int its_cache_flush(void *addr, size_t size)
{
#if defined(CONFIG_ARM64) && !defined(CONFIG_DCACHE)
	/* ARM64 startup enables the data cache even without the public API. */
	return arch_dcache_flush_and_invd_range(addr, size);
#else
	return sys_cache_data_flush_and_invd_range(addr, size);
#endif
}

/* BASER and CBASER have the same cacheability and shareability fields. */
static int its_write_baser(mem_addr_t addr, uint64_t reg, uint64_t *actual, bool *needs_flush)
{
	uint64_t value;
	unsigned int shareability;
	bool noncoherent;

	sys_write64(reg, addr);
	value = sys_read64(addr);

	if (((value ^ reg) & MASK(GITS_BASER_SHAREABILITY)) != 0U) {
		/* Honor fixed shareability and retry with non-cacheable accesses. */
		reg &= ~ITS_BASER_ATTRIBUTES;
		reg |= value & MASK(GITS_BASER_SHAREABILITY);
		reg |= MASK_SET(GIC_BASER_CACHE_NCACHEABLE, GITS_BASER_INNER_CACHE);
		sys_write64(reg, addr);
		value = sys_read64(addr);
	}

	shareability = MASK_GET(value, GITS_BASER_SHAREABILITY);
	/* Reserved shareability is treated as non-shareable. */
	noncoherent = IS_ENABLED(CONFIG_GIC_V3_ITS_DMA_NONCOHERENT) ||
		      ((shareability != GIC_BASER_SHARE_INNER) &&
		       (shareability != GIC_BASER_SHARE_OUTER));

	if (noncoherent &&
	    ((MASK_GET(value, GITS_BASER_INNER_CACHE) > GIC_BASER_CACHE_NCACHEABLE) ||
	     (MASK_GET(value, GITS_BASER_OUTER_CACHE) > GIC_BASER_CACHE_NCACHEABLE))) {
		sys_write64(value & ~MASK(GITS_BASER_VALID), addr);
		return -ENOTSUP;
	}

	*actual = value;
	*needs_flush = (MASK_GET(value, GITS_BASER_INNER_CACHE) <= GIC_BASER_CACHE_NCACHEABLE) ||
		       (MASK_GET(value, GITS_BASER_OUTER_CACHE) == GIC_BASER_CACHE_NCACHEABLE);

	return 0;
}

static int its_prepare_baser(struct gicv3_its_data *data, mem_addr_t addr, uint8_t *table,
			     size_t table_size, size_t entries, uint64_t *baser)
{
	static const size_t page_sizes[] = {
		[GITS_BASER_PAGE_SIZE_4K] = SIZE_4K,
		[GITS_BASER_PAGE_SIZE_16K] = SIZE_16K,
		[GITS_BASER_PAGE_SIZE_64K] = SIZE_64K,
	};
	uint64_t reg = sys_read64(addr);
	unsigned int type = GITS_BASER_TYPE_GET(reg);
	size_t entry_size = GITS_BASER_ENTRY_SIZE_GET(reg) + 1U;
	unsigned int max_indirect = (type == GITS_BASER_TYPE_DEVICE) ? 1U : 0U;
	bool exceeds_capacity = false;
	bool fits = false;

	reg &= ~(ITS_BASER_LAYOUT | ITS_BASER_ATTRIBUTES | MASK(GITS_BASER_VALID));
	reg |= MASK_SET(GIC_BASER_CACHE_INNERLIKE, GITS_BASER_OUTER_CACHE);

	if (IS_ENABLED(CONFIG_GIC_V3_ITS_DMA_NONCOHERENT)) {
		reg |= MASK_SET(GIC_BASER_SHARE_NO, GITS_BASER_SHAREABILITY);
		reg |= MASK_SET(GIC_BASER_CACHE_NCACHEABLE, GITS_BASER_INNER_CACHE);
	} else {
		reg |= MASK_SET(GIC_BASER_SHARE_INNER, GITS_BASER_SHAREABILITY);
		reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_BASER_INNER_CACHE);
	}

	/* Prefer a direct table and try the largest usable page size first. */
	for (unsigned int indirect = 0U; indirect <= max_indirect; indirect++) {
		for (int page = GITS_BASER_PAGE_SIZE_64K; page >= GITS_BASER_PAGE_SIZE_4K; page--) {
			uint64_t candidate = reg | MASK_SET(page, GITS_BASER_PAGE_SIZE) |
					     MASK_SET(indirect, GITS_BASER_INDIRECT);
			size_t page_size = page_sizes[page];
			size_t entries_per_page = page_size / entry_size;
			size_t root_bytes = entries * entry_size;
			size_t leaf_pages = 0U;
			size_t page_count, required_size;
			uint64_t actual;
			bool needs_flush;
			int ret;

			/* Probe the layout while the table is invalid. */
			sys_write64(candidate, addr);
			actual = sys_read64(addr);

			if (((actual ^ candidate) &
			     (MASK(GITS_BASER_PAGE_SIZE) | MASK(GITS_BASER_INDIRECT) |
			      MASK(GITS_BASER_VALID))) != 0U) {
				continue;
			}

			if (indirect != 0U) {
				size_t root_entries = DIV_ROUND_UP(entries, entries_per_page);

				root_bytes = root_entries * GITS_LVL1_ENTRY_SIZE;
				leaf_pages = MIN(root_entries, CONFIG_GIC_V3_ITS_MAX_DEVICES);
			}

			page_count = DIV_ROUND_UP(root_bytes, page_size);
			required_size = (page_count + leaf_pages) * page_size;

			if ((required_size > table_size) ||
			    (page_count > GITS_BASER_SIZE_MASK + 1U)) {
				exceeds_capacity = true;
				continue;
			}

			fits = true;
			candidate |= MASK_SET(page_count - 1U, GITS_BASER_SIZE);
			candidate |= MASK_SET((uintptr_t)table >> GITS_BASER_ADDR_SHIFT,
					      GITS_BASER_ADDR);

			ret = its_write_baser(addr, candidate, &actual, &needs_flush);

			if ((ret != 0) || (((actual ^ candidate) &
					    (ITS_BASER_LAYOUT | MASK(GITS_BASER_VALID))) != 0U)) {
				continue;
			}

			memset(table, 0, required_size);

			if (needs_flush) {
				ret = its_cache_flush(table, required_size);

				if (ret != 0) {
					return ret;
				}
			}

			data->tables_need_flush |= needs_flush;
			*baser = (candidate & ~ITS_BASER_ATTRIBUTES) |
				 (actual & ITS_BASER_ATTRIBUTES) | MASK(GITS_BASER_VALID);

			if (indirect != 0U) {
				data->next_indirect_page = table + page_count * page_size;
				data->indirect_page_size = page_size;
				data->indirect_pages_left = leaf_pages;
				data->indirect_dev_l1_table = (uint64_t *)table;
				data->indirect_dev_entries_per_page = entries_per_page;
			}

			LOG_INF("Using %s table of %zu x %zuK pages (%zu bytes per entry)",
				its_base_type_string[type], page_count, page_size / 1024U,
				(indirect != 0U) ? GITS_LVL1_ENTRY_SIZE : entry_size);

			return 0;
		}
	}

	if (exceeds_capacity && !fits) {
		LOG_ERR("%s table exceeds static capacity or BASER page limit",
			its_base_type_string[type]);
		return -ENOSPC;
	}

	LOG_ERR("No supported %s table layout", its_base_type_string[type]);
	return -ENOTSUP;
}

static void its_disable_tables(struct gicv3_its_data *data)
{
	for (int i = 0; i < GITS_BASER_NR_REGS; i++) {
		mem_addr_t addr = data->base + GITS_BASER(i);
		uint64_t reg = sys_read64(addr);
		unsigned int type = GITS_BASER_TYPE_GET(reg);

		if ((type == GITS_BASER_TYPE_DEVICE) || (type == GITS_BASER_TYPE_COLLECTION)) {
			sys_write64(reg & ~(MASK(GITS_BASER_VALID) | MASK(GITS_BASER_ADDR)), addr);
		}
	}

	data->tables_need_flush = false;
	data->indirect_dev_l1_table = NULL;
	data->indirect_dev_entries_per_page = 0U;
	data->next_indirect_page = NULL;
	data->indirect_page_size = 0U;
	data->indirect_pages_left = 0U;
	data->device_id_bits = 0U;
	memset(data->mapped_collections, 0, sizeof(data->mapped_collections));
	memset(data->mapping_collections, 0, sizeof(data->mapping_collections));
}

static int its_setup_tables(struct gicv3_its_data *data, const struct gicv3_its_config *cfg)
{
	uint64_t typer = sys_read64(data->base + GITS_TYPER);
	unsigned int hw_device_id_bits = GITS_TYPER_DEVBITS_GET(typer) + 1U;
	uint64_t baser_regs[GITS_BASER_NR_REGS] = {0};
	bool device_table_seen = false;
	bool collection_table_seen = false;
	int i, ret;

	if ((cfg->device_id_bits == 0U) || (cfg->device_id_bits > ITS_MAX_DEVICE_ID_BITS)) {
		return -EINVAL;
	}

	if ((GITS_TYPER_CIL_GET(typer) != 0U) &&
	    (CONFIG_MP_MAX_NUM_CPUS > BIT(GITS_TYPER_CIDBITS_GET(typer) + 1U))) {
		return -ENOTSUP;
	}

	data->device_id_bits = MIN(hw_device_id_bits, cfg->device_id_bits);
	data->indirect_dev_l1_table = NULL;
	data->tables_need_flush = false;

	for (i = 0; i < GITS_BASER_NR_REGS; ++i) {
		mem_addr_t addr = data->base + GITS_BASER(i);
		unsigned int type = GITS_BASER_TYPE_GET(sys_read64(addr));

		switch (type) {
		case GITS_BASER_TYPE_DEVICE:
			if (device_table_seen) {
				goto unsupported;
			}

			device_table_seen = true;
			ret = its_prepare_baser(data, addr, cfg->device_table,
						cfg->device_table_size, BIT(data->device_id_bits),
						&baser_regs[i]);
			break;
		case GITS_BASER_TYPE_COLLECTION:
			if (collection_table_seen) {
				goto unsupported;
			}

			collection_table_seen = true;
			ret = its_prepare_baser(data, addr, cfg->collection_table,
						cfg->collection_table_size, CONFIG_MP_MAX_NUM_CPUS,
						&baser_regs[i]);
			break;
		default:
			continue;
		}

		if (ret != 0) {
			goto disable_tables;
		}
	}

	if (!device_table_seen ||
	    (!collection_table_seen && GITS_TYPER_HCC_GET(typer) < CONFIG_MP_MAX_NUM_CPUS)) {
		goto unsupported;
	}

	barrier_dsync_fence_full();

	for (i = 0; i < GITS_BASER_NR_REGS; ++i) {
		uint64_t actual;

		if (baser_regs[i] == 0U) {
			continue;
		}

		sys_write64(baser_regs[i], data->base + GITS_BASER(i));
		actual = sys_read64(data->base + GITS_BASER(i));

		if ((((actual ^ baser_regs[i]) & ITS_BASER_LAYOUT) != 0U) ||
		    (MASK_GET(actual, GITS_BASER_VALID) == 0U)) {
			goto unsupported;
		}
	}

	return 0;

unsupported:
	ret = -ENOTSUP;

disable_tables:
	its_disable_tables(data);
	return ret;
}

static bool its_queue_full(struct gicv3_its_data *data)
{
	int widx;
	int ridx;

	widx = data->cmd_write - data->cmd_base;
	ridx = sys_read32(data->base + GITS_CREADR) / sizeof(struct its_cmd_block);

	/* This is incredibly unlikely to happen, unless the ITS locks up. */
	return (((widx + 1) % ITS_CMD_QUEUE_NR_ENTRIES) == ridx);
}

/*
 * Caller holds data->lock. Each post samples CREADR, so it cannot advance a full
 * queue turn between samples: at most one queue's worth of commands is pending.
 */
static uint32_t its_update_cmd_completed(struct gicv3_its_data *data)
{
	uint32_t read = sys_read32(data->base + GITS_CREADR) &
			(ITS_CMD_QUEUE_SIZE - sizeof(struct its_cmd_block));
	uint32_t delta = (read + ITS_CMD_QUEUE_SIZE - data->cmd_read) % ITS_CMD_QUEUE_SIZE;

	data->cmd_completed += delta / sizeof(struct its_cmd_block);
	data->cmd_read = read;

	return read;
}

#define ITS_CMD_WAIT_ATTEMPTS 1000000U

static int its_post_command(struct gicv3_its_data *data, struct its_cmd_block *cmd)
{
	struct its_cmd_block *cmd_entry;
	uint64_t wr_idx, rd_idx, idx, ticket;
	k_spinlock_key_t key;
	unsigned int count = ITS_CMD_WAIT_ATTEMPTS;
	bool done;

	if ((data->cmd_base == NULL) || (data->cmd_write == NULL)) {
		return -ENODEV;
	}

	while (true) {
		key = k_spin_lock(&data->lock);

		if (!its_queue_full(data)) {
			break;
		}

		k_spin_unlock(&data->lock, key);
		count--;

		if (count == 0U) {
			LOG_ERR("ITS queue not draining");
			return -EBUSY;
		}

		k_busy_wait(1U);
	}

	cmd_entry = data->cmd_write++;

	if (data->cmd_write == (data->cmd_base + ITS_CMD_QUEUE_NR_ENTRIES)) {
		data->cmd_write = data->cmd_base;
	}

	memcpy(cmd_entry, cmd, sizeof(*cmd_entry));

	if (data->cmd_queue_needs_flush) {
		int ret = its_cache_flush(cmd_entry, sizeof(*cmd_entry));

		if (ret != 0) {
			data->cmd_write = cmd_entry;
			k_spin_unlock(&data->lock, key);
			return ret;
		}
	}

	wr_idx = (data->cmd_write - data->cmd_base) * sizeof(*cmd_entry);
	rd_idx = its_update_cmd_completed(data);
	data->cmd_posted++;
	ticket = data->cmd_posted;

	barrier_dsync_fence_full();

	sys_write32(wr_idx, data->base + GITS_CWRITER);

	k_spin_unlock(&data->lock, key);

	count = ITS_CMD_WAIT_ATTEMPTS;

	while (1) {
		key = k_spin_lock(&data->lock);
		idx = its_update_cmd_completed(data);
		done = data->cmd_completed >= ticket;
		k_spin_unlock(&data->lock, key);

		if (done) {
			break;
		}

		count--;
		if (!count) {
			LOG_ERR("ITS queue timeout (rd %lld => %lld => wr %lld)",
				rd_idx, idx, wr_idx);
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	return 0;
}

static int its_send_sync_cmd(struct gicv3_its_data *data, uintptr_t rd_addr)
{
	struct its_cmd_block cmd = {0};

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_SYNC, GITS_CMD_ID);
	cmd.raw_cmd[2] = MASK_SET(rd_addr, GITS_CMD_RDBASE);

	return its_post_command(data, &cmd);
}

static int its_send_mapc_cmd(struct gicv3_its_data *data, uint32_t icid,
			     uintptr_t rd_addr, bool valid)
{
	struct its_cmd_block cmd = {0};

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_MAPC, GITS_CMD_ID);
	cmd.raw_cmd[2] = MASK_SET(icid, GITS_CMD_ICID) | MASK_SET(rd_addr, GITS_CMD_RDBASE) |
			  MASK_SET(valid ? 1 : 0, GITS_CMD_VALID);

	return its_post_command(data, &cmd);
}

static int its_send_mapd_cmd(struct gicv3_its_data *data, uint32_t device_id,
			     uint32_t size, uintptr_t itt_addr, bool valid)
{
	struct its_cmd_block cmd = {0};

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_MAPD, GITS_CMD_ID) |
			  MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd.raw_cmd[1] = MASK_SET(size, GITS_CMD_SIZE);
	cmd.raw_cmd[2] = MASK_SET(itt_addr >> GITS_CMD_ITTADDR_ALIGN, GITS_CMD_ITTADDR) |
			  MASK_SET(valid ? 1 : 0, GITS_CMD_VALID);

	return its_post_command(data, &cmd);
}

static int its_send_mapti_cmd(struct gicv3_its_data *data, uint32_t device_id,
			      uint32_t event_id, uint32_t intid, uint32_t icid)
{
	struct its_cmd_block cmd = {0};

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_MAPTI, GITS_CMD_ID) |
			  MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd.raw_cmd[1] = MASK_SET(event_id, GITS_CMD_EVENTID) |
			  MASK_SET(intid, GITS_CMD_PINTID);
	cmd.raw_cmd[2] = MASK_SET(icid, GITS_CMD_ICID);

	return its_post_command(data, &cmd);
}

static int its_send_discard_cmd(struct gicv3_its_data *data, uint32_t device_id, uint32_t event_id)
{
	struct its_cmd_block cmd = {0};

	cmd.raw_cmd[0] =
		MASK_SET(GITS_CMD_ID_DISCARD, GITS_CMD_ID) | MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd.raw_cmd[1] = MASK_SET(event_id, GITS_CMD_EVENTID);

	return its_post_command(data, &cmd);
}

static int its_send_int_cmd(struct gicv3_its_data *data, uint32_t device_id,
			    uint32_t event_id)
{
	struct its_cmd_block cmd = {0};

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_INT, GITS_CMD_ID) |
			  MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd.raw_cmd[1] = MASK_SET(event_id, GITS_CMD_EVENTID);

	return its_post_command(data, &cmd);
}

static int its_send_invall_cmd(struct gicv3_its_data *data, uint32_t icid)
{
	struct its_cmd_block cmd = {0};

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_INVALL, GITS_CMD_ID);
	cmd.raw_cmd[2] = MASK_SET(icid, GITS_CMD_ICID);

	return its_post_command(data, &cmd);
}

static struct its_device_slot *its_find_device_slot(struct gicv3_its_data *data, uint32_t device_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(data->device_slots); i++) {
		if (data->device_slots[i].used && data->device_slots[i].device_id == device_id) {
			return &data->device_slots[i];
		}
	}

	return NULL;
}

static int its_reserve_event_range(struct gicv3_its_data *data, uint32_t device_id,
				   uint32_t first_event, unsigned int count)
{
	struct its_device_slot *slot;
	k_spinlock_key_t key;
	int ret;

	if ((device_id >= BIT(data->device_id_bits)) || (count == 0U)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	slot = its_find_device_slot(data, device_id);

	if (slot == NULL) {
		ret = -EINVAL;
	} else if (slot->status == -EINPROGRESS) {
		ret = -EBUSY;
	} else if (slot->status != 0) {
		ret = slot->status;
	} else if ((first_event >= slot->nites) || (count > slot->nites - first_event)) {
		ret = -EINVAL;
	} else {
		ret = 0;

		for (unsigned int i = 0U; i < count; i++) {
			uint32_t event_id = first_event + i;
			size_t word = event_id / ITS_EVENT_BITMAP_BITS_PER_WORD;
			uint32_t mask = BIT(event_id % ITS_EVENT_BITMAP_BITS_PER_WORD);

			if ((slot->event_id_bitmap[word] & mask) != 0U) {
				ret = -EBUSY;
				break;
			}
		}

		if (ret == 0) {
			for (unsigned int i = 0U; i < count; i++) {
				uint32_t event_id = first_event + i;
				size_t word = event_id / ITS_EVENT_BITMAP_BITS_PER_WORD;
				uint32_t mask = BIT(event_id % ITS_EVENT_BITMAP_BITS_PER_WORD);

				slot->event_id_bitmap[word] |= mask;
				slot->event_id_busy_bitmap[word] |= mask;
			}
		}
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void its_finish_event_range(struct gicv3_its_data *data, uint32_t device_id,
				   uint32_t first_event, unsigned int count, bool release)
{
	struct its_device_slot *slot;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	slot = its_find_device_slot(data, device_id);

	if (slot != NULL) {
		for (unsigned int i = 0U; i < count; i++) {
			uint32_t event_id = first_event + i;
			size_t word = event_id / ITS_EVENT_BITMAP_BITS_PER_WORD;
			uint32_t mask = BIT(event_id % ITS_EVENT_BITMAP_BITS_PER_WORD);

			slot->event_id_busy_bitmap[word] &= ~mask;
			if (release) {
				slot->event_id_bitmap[word] &= ~mask;
			}
		}
	}

	k_spin_unlock(&data->lock, key);
}

static int its_check_mapped_event(struct gicv3_its_data *data, uint32_t device_id,
				  uint32_t event_id)
{
	struct its_device_slot *slot;
	size_t word = event_id / ITS_EVENT_BITMAP_BITS_PER_WORD;
	uint32_t mask = BIT(event_id % ITS_EVENT_BITMAP_BITS_PER_WORD);
	k_spinlock_key_t key;
	int ret;

	if (device_id >= BIT(data->device_id_bits)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	slot = its_find_device_slot(data, device_id);

	if (slot == NULL) {
		ret = -EINVAL;
	} else if (slot->status == -EINPROGRESS) {
		ret = -EBUSY;
	} else if (slot->status != 0) {
		ret = slot->status;
	} else if ((event_id >= slot->nites) || ((slot->event_id_bitmap[word] & mask) == 0U)) {
		ret = -EINVAL;
	} else if ((slot->event_id_busy_bitmap[word] & mask) != 0U) {
		ret = -EBUSY;
	} else {
		ret = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int gicv3_its_send_int(const struct device *dev, uint32_t device_id, uint32_t event_id)
{
	struct gicv3_its_data *data = dev->data;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	ret = its_check_mapped_event(data, device_id, event_id);

	if (ret != 0) {
		return ret;
	}

	return its_send_int_cmd(data, device_id, event_id);
}

static void its_disable_cmd_queue(struct gicv3_its_data *data)
{
	mem_addr_t addr = data->base + GITS_CBASER;
	uint64_t reg = sys_read64(addr);

	sys_write64(reg & ~(MASK(GITS_CBASER_VALID) | MASK(GITS_CBASER_ADDR)), addr);

	data->cmd_base = NULL;
	data->cmd_write = NULL;
	data->cmd_read = 0U;
	data->cmd_posted = 0U;
	data->cmd_completed = 0U;
	data->cmd_queue_needs_flush = false;
}

static int its_setup_cmd_queue(const struct device *dev)
{
	const struct gicv3_its_config *cfg = dev->config;
	struct gicv3_its_data *data = dev->data;
	uint64_t reg = 0;
	uint64_t actual;
	int ret;

	/* Zero out cmd table */
	memset(cfg->cmd_queue, 0, cfg->cmd_queue_size);

	reg |= MASK_SET((cfg->cmd_queue_size / SIZE_4K) - 1, GITS_CBASER_SIZE);
	reg |= MASK_SET((uintptr_t)cfg->cmd_queue >> GITS_CBASER_ADDR_SHIFT, GITS_CBASER_ADDR);

	if (IS_ENABLED(CONFIG_GIC_V3_ITS_DMA_NONCOHERENT)) {
		reg |= MASK_SET(GIC_BASER_SHARE_NO, GITS_CBASER_SHAREABILITY);
		reg |= MASK_SET(GIC_BASER_CACHE_INNERLIKE, GITS_CBASER_OUTER_CACHE);
		reg |= MASK_SET(GIC_BASER_CACHE_NCACHEABLE, GITS_CBASER_INNER_CACHE);
	} else {
		reg |= MASK_SET(GIC_BASER_SHARE_INNER, GITS_CBASER_SHAREABILITY);
		reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_CBASER_OUTER_CACHE);
		reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_CBASER_INNER_CACHE);
	}

	reg |= MASK_SET(1, GITS_CBASER_VALID);

	/* ITS is disabled and quiescent, so retries can keep Valid set. */
	ret = its_write_baser(data->base + GITS_CBASER, reg, &actual, &data->cmd_queue_needs_flush);

	if (ret != 0) {
		goto disable_queue;
	}

	if ((((actual ^ reg) & (MASK(GITS_CBASER_SIZE) | MASK(GITS_CBASER_ADDR))) != 0U) ||
	    (MASK_GET(actual, GITS_CBASER_VALID) == 0U)) {
		ret = -ENOTSUP;
		goto disable_queue;
	}

	if (data->cmd_queue_needs_flush) {
		ret = its_cache_flush(cfg->cmd_queue, cfg->cmd_queue_size);

		if (ret != 0) {
			goto disable_queue;
		}
	}

	data->cmd_base = (struct its_cmd_block *)cfg->cmd_queue;
	data->cmd_write = data->cmd_base;
	data->cmd_read = 0U;
	data->cmd_posted = 0U;
	data->cmd_completed = 0U;

	LOG_INF("Allocated %ld entries for command table", ITS_CMD_QUEUE_NR_ENTRIES);

	sys_write64(0, data->base + GITS_CWRITER);

	return 0;

disable_queue:
	its_disable_cmd_queue(data);
	return ret;
}

static uintptr_t gicv3_rdist_get_rdbase(const struct device *dev, unsigned int cpuid)
{
	struct gicv3_its_data *data = dev->data;
	uint64_t typer = sys_read64(data->base + GITS_TYPER);
	uintptr_t rdbase;

	if (GITS_TYPER_PTA_GET(typer)) {
		rdbase = gic_rdists[cpuid];
		/* RDbase must be 64KB aligned, only return bits[51:16] of the address */
		rdbase = rdbase >> GITS_CMD_RDBASE_ALIGN;
	} else {
		rdbase =
			GICR_TYPER_PROCESSOR_NUMBER_GET(sys_read64(gic_rdists[cpuid] + GICR_TYPER));
	}

	return rdbase;
}

static int its_sync_ready_rdist(const struct device *dev)
{
	struct gicv3_its_data *data = dev->data;
	unsigned int cpu_id = arch_curr_cpu()->id;

	if (gicv3_rdist_lpi_is_ready(cpu_id)) {
		return its_send_sync_cmd(data, gicv3_rdist_get_rdbase(dev, cpu_id));
	}

	for (cpu_id = 0U; cpu_id < CONFIG_MP_MAX_NUM_CPUS; cpu_id++) {
		if (gicv3_rdist_lpi_is_ready(cpu_id)) {
			return its_send_sync_cmd(data, gicv3_rdist_get_rdbase(dev, cpu_id));
		}
	}

	return -ENODEV;
}

static int its_ensure_collection(const struct device *dev, unsigned int cpu_id)
{
	struct gicv3_its_data *data = dev->data;
	int ret = 0;

	if (!gicv3_rdist_lpi_is_ready(cpu_id)) {
		return -ENODEV;
	}

	if (atomic_test_bit(data->mapped_collections, cpu_id)) {
		return 0;
	}

	if (atomic_test_and_set_bit(data->mapping_collections, cpu_id)) {
		return -EBUSY;
	}

	/* Another caller may have completed MAPC before we claimed the collection. */
	if (!atomic_test_bit(data->mapped_collections, cpu_id)) {
		ret = its_send_mapc_cmd(data, cpu_id, gicv3_rdist_get_rdbase(dev, cpu_id), true);
		if (ret == 0) {
			atomic_set_bit(data->mapped_collections, cpu_id);
		}
	}

	atomic_clear_bit(data->mapping_collections, cpu_id);

	return ret;
}

/* Callers validate the INTID and reserve the EventID before submitting MAPTI. */
static int gicv3_its_map_intid_internal(const struct device *dev, uint32_t device_id,
					uint32_t event_id, unsigned int intid, unsigned int cpu_id,
					bool *mapping_may_exist);

static int gicv3_its_map_intid(const struct device *dev, uint32_t device_id, uint32_t event_id,
			       unsigned int intid)
{
	struct gicv3_its_data *data = dev->data;
	bool mapping_may_exist;
	unsigned int cpu_id;
	int ret, cleanup_ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if ((intid < GIC_LPI_INT_BASE) || (intid >= data->lpi_intid_limit)) {
		return -EINVAL;
	}

	cpu_id = arch_curr_cpu()->id;
	ret = its_ensure_collection(dev, cpu_id);

	if (ret != 0) {
		return ret;
	}

	ret = its_reserve_event_range(data, device_id, event_id, 1U);

	if (ret != 0) {
		return ret;
	}

	ret = gicv3_its_map_intid_internal(dev, device_id, event_id, intid, cpu_id,
					   &mapping_may_exist);
	if (ret == 0) {
		its_finish_event_range(data, device_id, event_id, 1U, false);
		return ret;
	}

	if (mapping_may_exist) {
		uintptr_t rd_addr;

		cleanup_ret = its_send_discard_cmd(data, device_id, event_id);

		if (cleanup_ret == 0) {
			rd_addr = gicv3_rdist_get_rdbase(dev, cpu_id);
			cleanup_ret = its_send_sync_cmd(data, rd_addr);
		}

		if (cleanup_ret != 0) {
			/* Keep the EventID busy while a DISCARD may still be pending. */
			LOG_ERR("Failed to discard mapped EventID %u for deviceid %x: %d", event_id,
				device_id, cleanup_ret);
		} else {
			its_finish_event_range(data, device_id, event_id, 1U, true);
		}
	} else {
		its_finish_event_range(data, device_id, event_id, 1U, true);
	}

	return ret;
}

static int gicv3_its_map_intid_internal(const struct device *dev, uint32_t device_id,
					uint32_t event_id, unsigned int intid, unsigned int cpu_id,
					bool *mapping_may_exist)
{
	struct gicv3_its_data *data = dev->data;
	int ret;

	ret = its_send_mapti_cmd(data, device_id, event_id, intid, cpu_id);

	if (ret != 0) {
		/* A timed-out MAPTI may still have reached the ITS. */
		*mapping_may_exist = (ret == -ETIMEDOUT);
		LOG_ERR("Failed to map eventid %d to intid %d for deviceid %x",
			event_id, intid, device_id);
		return ret;
	}

	*mapping_may_exist = true;

	return its_send_sync_cmd(data, gicv3_rdist_get_rdbase(dev, cpu_id));
}

static int gicv3_its_init_device_id(const struct device *dev, uint32_t device_id,
				    unsigned int nites)
{
	const struct gicv3_its_config *cfg = dev->config;
	struct gicv3_its_data *data = dev->data;
	struct its_device_slot *slot;
	size_t entry_size, offset = 0U;
	size_t slot_index = ARRAY_SIZE(data->device_slots);
	uint64_t typer;
	unsigned int mapd_size;
	k_spinlock_key_t key;
	int ret, cleanup_ret = 0;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if ((device_id >= BIT(data->device_id_bits)) || (nites == 0U)) {
		return -EINVAL;
	}

	if (nites > CONFIG_GIC_V3_ITS_MAX_VECTORS) {
		return -ENOMEM;
	}

	typer = sys_read64(data->base + GITS_TYPER);
	mapd_size =
		MIN(fls_z(CONFIG_GIC_V3_ITS_MAX_VECTORS) - 2U, MASK_GET(typer, GITS_TYPER_IDBITS));

	if (nites > BIT(mapd_size + 1U)) {
		return -ENOTSUP;
	}

	entry_size = GITS_TYPER_ITT_ENTRY_SIZE_GET(typer) + 1U;

	if (data->indirect_dev_l1_table != NULL) {
		offset = device_id / data->indirect_dev_entries_per_page;
	}

	key = k_spin_lock(&data->lock);
	slot = its_find_device_slot(data, device_id);

	if (slot != NULL) {
		if (slot->status == -EINPROGRESS) {
			ret = -EBUSY;
			goto unlock;
		}

		if (slot->status == 0) {
			slot->nites = MAX(slot->nites, nites);
			ret = 0;
			goto unlock;
		}

		ret = slot->status;
		goto unlock;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(data->device_slots); i++) {
		if (!data->device_slots[i].used) {
			slot_index = i;
			break;
		}
	}

	if (slot_index == ARRAY_SIZE(data->device_slots)) {
		ret = -ENOMEM;
		goto unlock;
	}

	if (data->indirect_dev_l1_table != NULL) {
		uint64_t *l1_entry = &data->indirect_dev_l1_table[offset];

		if (*l1_entry == 0U) {
			if (data->indirect_pages_left == 0U) {
				ret = -ENOMEM;
				goto unlock;
			}

			/* A published level 1 entry keeps its page until the ITS is disabled. */
			*l1_entry =
				(uintptr_t)data->next_indirect_page | MASK_SET(1, GITS_BASER_VALID);
			data->next_indirect_page += data->indirect_page_size;
			data->indirect_pages_left--;
		}

		if (data->tables_need_flush) {
			/* Retry a flush if setup stopped before issuing MAPD. */
			ret = its_cache_flush(l1_entry, GITS_LVL1_ENTRY_SIZE);

			if (ret != 0) {
				goto unlock;
			}
		}

		barrier_dsync_fence_full();
	}

	slot = &data->device_slots[slot_index];
	slot->device_id = device_id;
	slot->status = -EINPROGRESS;
	slot->used = true;
	k_spin_unlock(&data->lock, key);

	LOG_INF("Using ITT for DeviceID %x and %u vectors (%zu bytes per entry)", device_id,
		CONFIG_GIC_V3_ITS_MAX_VECTORS, entry_size);

	ret = its_send_mapd_cmd(data, device_id, mapd_size, (uintptr_t)cfg->itts[slot_index], true);

	if (ret != 0) {
		LOG_ERR("Failed to map device id %x ITT table", device_id);

		if (ret == -ETIMEDOUT) {
			cleanup_ret = its_send_mapd_cmd(data, device_id, 0U, 0U, false);

			if (cleanup_ret == 0) {
				cleanup_ret = its_sync_ready_rdist(dev);
			}

			if (cleanup_ret == 0) {
				/* MAPD requires a zeroed ITT when the slot is used again. */
				memset(cfg->itts[slot_index], 0, ITS_ITT_SLOT_SIZE);

				if (data->tables_need_flush) {
					cleanup_ret = its_cache_flush(cfg->itts[slot_index],
								      ITS_ITT_SLOT_SIZE);
				}
			}
		}
	}

	key = k_spin_lock(&data->lock);

	if (ret == 0) {
		slot->nites = nites;
	} else if (cleanup_ret == 0) {
		/* Published indirect pages cannot be reclaimed while the ITS is enabled. */
		slot->used = false;
	} else {
		LOG_ERR("Failed to recycle ITT for device id %x after MAPD timeout: %d", device_id,
			cleanup_ret);
	}

	slot->status = ret;
	k_spin_unlock(&data->lock, key);

	return ret;

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int gicv3_its_alloc_intids(const struct device *dev, unsigned int count, unsigned int *intid)
{
	struct gicv3_its_data *data = dev->data;
	unsigned int limit;
	size_t offset;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if ((intid == NULL) || (count == 0U)) {
		return -EINVAL;
	}

	limit = data->lpi_intid_limit;

	if ((limit <= GIC_LPI_INT_BASE) || (count > limit - GIC_LPI_INT_BASE)) {
		return -ENOSPC;
	}

	ret = sys_bitarray_alloc(&its_lpi_intids, count, &offset);

	if (ret != 0) {
		return -ENOSPC;
	}

	*intid = GIC_LPI_INT_BASE + offset;

	return 0;
}

static int gicv3_its_alloc_map_intids(const struct device *dev, uint32_t device_id,
				      unsigned int count, unsigned int *intid)
{
	struct gicv3_its_data *data = dev->data;
	size_t offset;
	unsigned int first_intid, mapped = 0U;
	unsigned int cleanup_count;
	unsigned int retained = 0U;
	unsigned int cpu_id;
	bool mapping_may_exist = false;
	int ret, cleanup_ret = 0;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if ((intid == NULL) || (count == 0U) || (count > CONFIG_GIC_V3_ITS_MAX_VECTORS)) {
		return -EINVAL;
	}

	cpu_id = arch_curr_cpu()->id;
	ret = its_ensure_collection(dev, cpu_id);

	if (ret != 0) {
		return ret;
	}

	ret = its_reserve_event_range(data, device_id, 0U, count);

	if (ret != 0) {
		return ret;
	}

	ret = gicv3_its_alloc_intids(dev, count, &first_intid);

	if (ret != 0) {
		its_finish_event_range(data, device_id, 0U, count, true);
		return ret;
	}

	offset = first_intid - GIC_LPI_INT_BASE;

	for (unsigned int i = 0U; i < count; i++) {
		ret = gicv3_its_map_intid_internal(dev, device_id, i, first_intid + i, cpu_id,
						   &mapping_may_exist);

		if (ret != 0) {
			break;
		}

		mapped++;
	}

	if (mapped == count) {
		its_finish_event_range(data, device_id, 0U, count, false);
		*intid = first_intid;
		return 0;
	}

	cleanup_count = mapped + (mapping_may_exist ? 1U : 0U);

	for (unsigned int i = 0U; i < cleanup_count; i++) {
		cleanup_ret = its_send_discard_cmd(data, device_id, i);

		if (cleanup_ret != 0) {
			break;
		}
	}

	if ((cleanup_ret == 0) && (cleanup_count > 0U)) {
		cleanup_ret = its_send_sync_cmd(data, gicv3_rdist_get_rdbase(dev, cpu_id));
	}

	if (cleanup_ret != 0) {
		LOG_ERR("Failed to discard mapped MSI EventIDs for deviceid %x: %d", device_id,
			cleanup_ret);
		retained = cleanup_count;
	}

	if (retained < count) {
		unsigned int released = count - retained;

		if (sys_bitarray_free(&its_lpi_intids, released, offset + retained) != 0) {
			LOG_ERR("Failed to release MSI INTIDs for deviceid %x", device_id);
			return -EIO;
		}

		its_finish_event_range(data, device_id, retained, released, true);
	}

	return ret;
}

static unsigned int gicv3_its_alloc_intid(const struct device *dev)
{
	unsigned int intid;
	int ret = gicv3_its_alloc_intids(dev, 1U, &intid);

	return (ret == 0) ? intid : 0U;
}

static uint64_t gicv3_its_get_msi_addr(const struct device *dev)
{
	const struct gicv3_its_config *cfg = (const struct gicv3_its_config *)dev->config;

	return (uint64_t)cfg->base_addr + GITS_TRANSLATER;
}

#define ITS_RDIST_MAP(n)                                                                           \
	{                                                                                          \
		const struct device *const dev = DEVICE_DT_INST_GET(n);                            \
                                                                                                   \
		if (device_is_ready(dev)) {                                                        \
			int ret = its_ensure_collection(dev, arch_curr_cpu()->id);                 \
                                                                                                   \
			if (ret != 0) {                                                            \
				LOG_ERR("Failed to map CPU%d redistributor", arch_curr_cpu()->id); \
			}                                                                          \
		}                                                                                  \
	}

void its_rdist_map(void)
{
	DT_INST_FOREACH_STATUS_OKAY(ITS_RDIST_MAP)
}

#define ITS_RDIST_INVALL(n)                                                                        \
	{                                                                                          \
		const struct device *const dev = DEVICE_DT_INST_GET(n);                            \
		struct gicv3_its_data *data = dev->data;                                           \
		int ret;                                                                           \
                                                                                                   \
		if (device_is_ready(dev) && atomic_test_bit(data->mapped_collections, cpu_id)) {   \
			ret = its_send_invall_cmd(data, cpu_id);                                   \
			if (ret == 0) {                                                            \
				ret = its_send_sync_cmd(data,                                      \
							gicv3_rdist_get_rdbase(dev, cpu_id));      \
			}                                                                          \
			if (ret != 0) {                                                            \
				LOG_ERR("Failed to sync RDIST LPI cache for CPU%u: %d", cpu_id,    \
					ret);                                                      \
			}                                                                          \
		}                                                                                  \
	}

void its_rdist_invall(void)
{
	unsigned int cpu_id = arch_curr_cpu()->id;

	if (!gicv3_rdist_lpi_is_ready(cpu_id)) {
		return;
	}

	DT_INST_FOREACH_STATUS_OKAY(ITS_RDIST_INVALL)
}

static int gicv3_its_init(const struct device *dev)
{
	const struct gicv3_its_config *cfg = dev->config;
	struct gicv3_its_data *data = dev->data;
	unsigned int lpi_id_bits, lpi_intid_limit;
	size_t usable_lpis;
	uint32_t gicd_typer;
	uint32_t reg;
	int ret;

	gicd_typer = sys_read32(GICD_TYPER);
	lpi_id_bits = MIN(GICD_TYPER_IDBITS(gicd_typer), ITS_LPI_ID_BITS);
	lpi_intid_limit = gicv3_lpi_intid_limit(gicd_typer, lpi_id_bits, CONFIG_NUM_IRQS);

	if (lpi_intid_limit <= GIC_LPI_INT_BASE) {
		return -ENOTSUP;
	}

	if (!gicv3_rdist_lpi_is_ready(arch_curr_cpu()->id)) {
		return -ENODEV;
	}

	/* All ITS instances use the same Distributor and LPI limit. */
	usable_lpis = lpi_intid_limit - GIC_LPI_INT_BASE;

	if (usable_lpis < ITS_LPI_BITMAP_BITS) {
		ret = sys_bitarray_set_region(&its_lpi_intids, ITS_LPI_BITMAP_BITS - usable_lpis,
					      usable_lpis);

		if (ret != 0) {
			return ret;
		}
	}

	if (data->base == 0U) {
		device_map(&data->base, cfg->base_addr, cfg->base_size, K_MEM_CACHE_NONE);
	}

	ret = its_force_quiescent(data);

	if (ret != 0) {
		LOG_ERR("Failed to quiesce, giving up");
		return ret;
	}

	/* A previous initialization may have timed out waiting for quiescence. */
	if (data->cmd_base != NULL) {
		its_disable_cmd_queue(data);
		its_disable_tables(data);
	}

	ret = its_setup_tables(data, cfg);

	if (ret != 0) {
		LOG_ERR("Failed to set up tables: %d", ret);
		return ret;
	}

	if (data->tables_need_flush) {
		size_t itts_size = CONFIG_GIC_V3_ITS_MAX_DEVICES * ITS_ITT_SLOT_SIZE;

		ret = its_cache_flush(cfg->itts, itts_size);

		if (ret != 0) {
			goto disable_tables;
		}
	}

	ret = its_setup_cmd_queue(dev);

	if (ret != 0) {
		LOG_ERR("Failed to set up command queue: %d", ret);
		goto disable_tables;
	}

	barrier_dsync_fence_full();
	reg = sys_read32(data->base + GITS_CTLR);
	reg |= MASK_SET(1, GITS_CTLR_ENABLED);
	sys_write32(reg, data->base + GITS_CTLR);

	/* A late initialization can find secondary redistributors already online. */
	for (unsigned int cpu_id = 0U; cpu_id < CONFIG_MP_MAX_NUM_CPUS; cpu_id++) {
		if (!gicv3_rdist_lpi_is_ready(cpu_id)) {
			continue;
		}

		ret = its_ensure_collection(dev, cpu_id);

		if (ret != 0) {
			LOG_ERR("Failed to map CPU%u redistributor: %d", cpu_id, ret);
			goto disable_its;
		}
	}

	data->lpi_intid_limit = lpi_intid_limit;

	return 0;

disable_its:
	if (its_force_quiescent(data) != 0) {
		/* The ITS may still be accessing the tables and queue. */
		LOG_ERR("Failed to quiesce after MAPC failure");
		return ret;
	}

	its_disable_cmd_queue(data);

disable_tables:
	its_disable_tables(data);
	return ret;
}

DEVICE_API(its, gicv3_its_api) = {
	.alloc_intid = gicv3_its_alloc_intid,
	.setup_deviceid = gicv3_its_init_device_id,
	.map_intid = gicv3_its_map_intid,
	.send_int = gicv3_its_send_int,
	.get_msi_addr = gicv3_its_get_msi_addr,
	.alloc_intids = gicv3_its_alloc_intids,
	.alloc_map_intids = gicv3_its_alloc_map_intids,
};

#define GICV3_ITS_INIT(n)                                                                          \
	BUILD_ASSERT(DT_INST_PROP(n, zephyr_device_id_bits) > 0U &&                                \
		     DT_INST_PROP(n, zephyr_device_id_bits) <= ITS_MAX_DEVICE_ID_BITS);            \
	static uint8_t gicv3_its_device_table##n[ITS_DEVICE_TABLE_SIZE(n)] __aligned(SIZE_64K);    \
	static uint8_t gicv3_its_collection_table##n[ITS_COLLECTION_TABLE_SIZE]                    \
		__aligned(SIZE_64K);                                                               \
	static uint8_t gicv3_its_itts##n[CONFIG_GIC_V3_ITS_MAX_DEVICES][ITS_ITT_SLOT_SIZE]         \
		__aligned(SIZE_256);                                                               \
	static struct its_cmd_block gicv3_its_cmd##n[ITS_CMD_QUEUE_NR_ENTRIES]                     \
		__aligned(ITS_CMD_QUEUE_SIZE);                                                     \
	static struct gicv3_its_data gicv3_its_data##n;                                            \
	static const struct gicv3_its_config gicv3_its_config##n = {                               \
		.base_addr = DT_INST_REG_ADDR(n),                                                  \
		.base_size = DT_INST_REG_SIZE(n),                                                  \
		.cmd_queue = gicv3_its_cmd##n,                                                     \
		.cmd_queue_size = sizeof(gicv3_its_cmd##n),                                        \
		.device_table = gicv3_its_device_table##n,                                         \
		.device_table_size = sizeof(gicv3_its_device_table##n),                            \
		.collection_table = gicv3_its_collection_table##n,                                 \
		.collection_table_size = sizeof(gicv3_its_collection_table##n),                    \
		.itts = gicv3_its_itts##n,                                                         \
		.device_id_bits = ITS_DEVICE_ID_BITS(n),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &gicv3_its_init, NULL, &gicv3_its_data##n, &gicv3_its_config##n,  \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, &gicv3_its_api);

DT_INST_FOREACH_STATUS_OKAY(GICV3_ITS_INIT)
