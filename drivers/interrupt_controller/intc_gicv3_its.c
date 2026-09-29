/*
 * Copyright 2021 BayLibre, SAS
 * Copyright 2025 NXP
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
#define ITS_MAX_LPI_ID_BITS             16U
#define ITS_MAX_BASER_ENTRY_SIZE        32U
#define ITS_MAX_ITT_ENTRY_SIZE          16U
#define ITS_LPI_ID_BITS                 \
	DT_PROP_OR(DT_COMPAT_GET_ANY_STATUS_OKAY(arm_gic_v3), zephyr_lpi_id_bits, 16)
#define ITS_DEVICE_ID_BITS(n)           \
	DT_INST_PROP_OR(n, zephyr_device_id_bits, ITS_MAX_DEVICE_ID_BITS)
#define ITS_INDIRECT_PAGE_COUNT(n)      \
	MIN(CONFIG_GIC_V3_ITS_MAX_DEVICES, \
	    BIT((ITS_DEVICE_ID_BITS(n) > 7U) ? ITS_DEVICE_ID_BITS(n) - 7U : 0U))
#define ITS_ITT_SLOT_SIZE               \
	ROUND_UP(CONFIG_GIC_V3_ITS_MAX_VECTORS * ITS_MAX_ITT_ENTRY_SIZE, SIZE_256)
#define ITS_DEVICE_TABLE_SIZE(n)        \
	ROUND_UP(BIT(ITS_DEVICE_ID_BITS(n)) * \
		 ITS_MAX_BASER_ENTRY_SIZE, SIZE_64K)
#define ITS_COLLECTION_TABLE_SIZE      \
	ROUND_UP(ITS_MAX_BASER_ENTRY_SIZE * CONFIG_MP_MAX_NUM_CPUS, SIZE_64K)

BUILD_ASSERT(IS_POWER_OF_TWO(CONFIG_GIC_V3_ITS_MAX_VECTORS));

#define ITS_LPI_BITMAP_BITS \
	((MIN(CONFIG_NUM_IRQS, BIT(ITS_MAX_LPI_ID_BITS)) > 8192U) ? \
	 (MIN(CONFIG_NUM_IRQS, BIT(ITS_MAX_LPI_ID_BITS)) - 8192U) : 1U)

SYS_BITARRAY_DEFINE_STATIC(its_lpi_intids, ITS_LPI_BITMAP_BITS);

/* The level 1 entry size is a 64bit pointer */
#define GITS_LVL1_ENTRY_SIZE (8UL)

struct its_device_slot {
	uint32_t device_id;
	unsigned int nites;
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
	bool dev_table_is_indirect;
	uint64_t *indirect_dev_lvl1_table;
	size_t indirect_dev_lvl1_width;
	size_t indirect_dev_lvl2_width;
	unsigned int device_id_bits;
	unsigned int lpi_id_bits;
	struct its_device_slot device_slots[CONFIG_GIC_V3_ITS_MAX_DEVICES];
	bool indirect_pages_used[CONFIG_GIC_V3_ITS_MAX_DEVICES];
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
	uint8_t (*indirect_dev_pages)[SIZE_64K];
	size_t indirect_dev_page_count;
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

/* Probe the BASER(i) to get the largest supported page size */
static size_t its_probe_baser_page_size(struct gicv3_its_data *data, int i)
{
	uint64_t page_size = GITS_BASER_PAGE_SIZE_64K;

	while (page_size > GITS_BASER_PAGE_SIZE_4K) {
		uint64_t reg = sys_read64(data->base + GITS_BASER(i));

		reg &= ~(MASK(GITS_BASER_PAGE_SIZE) | MASK(GITS_BASER_ADDR) |
			 MASK(GITS_BASER_VALID));
		reg |= MASK_SET(page_size, GITS_BASER_PAGE_SIZE);

		sys_write64(reg, data->base + GITS_BASER(i));

		reg = sys_read64(data->base + GITS_BASER(i));

		if (MASK_GET(reg, GITS_BASER_PAGE_SIZE) == page_size) {
			break;
		}

		switch (page_size) {
		case GITS_BASER_PAGE_SIZE_64K:
			page_size = GITS_BASER_PAGE_SIZE_16K;
			break;
		default:
			page_size = GITS_BASER_PAGE_SIZE_4K;
		}
	}

	switch (page_size) {
	case GITS_BASER_PAGE_SIZE_64K:
		return SIZE_64K;
	case GITS_BASER_PAGE_SIZE_16K:
		return SIZE_16K;
	default:
		return SIZE_4K;
	}
}

static int its_setup_tables(struct gicv3_its_data *data, const struct gicv3_its_config *cfg)
{
	unsigned int hw_device_id_bits =
		GITS_TYPER_DEVBITS_GET(sys_read64(data->base + GITS_TYPER)) + 1U;
	uint64_t baser_regs[GITS_BASER_NR_REGS] = {0};
	bool device_table_seen = false;
	bool collection_table_seen = false;
	int i;

	if ((cfg->device_id_bits == 0U) || (cfg->device_id_bits > ITS_MAX_DEVICE_ID_BITS)) {
		return -EINVAL;
	}
	data->device_id_bits = MIN(hw_device_id_bits, cfg->device_id_bits);

	for (i = 0; i < GITS_BASER_NR_REGS; ++i) {
		uint64_t reg = sys_read64(data->base + GITS_BASER(i));
		uint64_t actual;
		uint64_t probe;
		unsigned int type = GITS_BASER_TYPE_GET(reg);
		size_t page_size, entry_size, page_cnt, lvl2_width = 0U;
		size_t table_size, required_size, root_bits;
		bool indirect = false;
		void *table;

		entry_size = GITS_BASER_ENTRY_SIZE_GET(reg) + 1;

		switch (GITS_BASER_PAGE_SIZE_GET(reg)) {
		case GITS_BASER_PAGE_SIZE_4K:
			page_size = SIZE_4K;
			break;
		case GITS_BASER_PAGE_SIZE_16K:
			page_size = SIZE_16K;
			break;
		case GITS_BASER_PAGE_SIZE_64K:
			page_size = SIZE_64K;
			break;
		default:
			page_size = SIZE_4K;
		}

		switch (type) {
		case GITS_BASER_TYPE_DEVICE:
			if (device_table_seen) {
				return -ENOTSUP;
			}
			device_table_seen = true;
			table = cfg->device_table;
			table_size = cfg->device_table_size;
			root_bits = data->device_id_bits;
			if (hw_device_id_bits > ITS_MAX_DEVICE_ID_BITS) {
				/* Use the largest possible page size for indirect */
				page_size = its_probe_baser_page_size(data, i);
				lvl2_width = fls_z(page_size / entry_size) - 1;
				root_bits = (root_bits > lvl2_width) ? root_bits - lvl2_width : 0U;
				entry_size = GITS_LVL1_ENTRY_SIZE;
				indirect = true;
			}
			required_size = ROUND_UP(entry_size * BIT(root_bits), page_size);
			break;
		case GITS_BASER_TYPE_COLLECTION:
			if (collection_table_seen) {
				return -ENOTSUP;
			}
			collection_table_seen = true;
			table = cfg->collection_table;
			table_size = cfg->collection_table_size;
			required_size = ROUND_UP(entry_size * CONFIG_MP_MAX_NUM_CPUS, page_size);
			break;
		default:
			continue;
		}

		page_cnt = required_size / page_size;
		if ((required_size > table_size) || (page_cnt > GITS_BASER_SIZE_MASK + 1U)) {
			LOG_ERR("%s table exceeds static capacity", its_base_type_string[type]);
			return -ENOSPC;
		}

		LOG_INF("Using %s table of %zu x %zuK pages (%zu bytes per entry)",
			its_base_type_string[type], page_cnt, page_size / 1024U, entry_size);

		reg &= ~(MASK(GITS_BASER_PAGE_SIZE) | MASK(GITS_BASER_SIZE) |
			 MASK(GITS_BASER_ADDR) | MASK(GITS_BASER_INDIRECT) |
			 MASK(GITS_BASER_VALID) | MASK(GITS_BASER_OUTER_CACHE) |
			 MASK(GITS_BASER_INNER_CACHE) | MASK(GITS_BASER_SHAREABILITY));
		switch (page_size) {
		case SIZE_4K:
			reg |= MASK_SET(GITS_BASER_PAGE_SIZE_4K, GITS_BASER_PAGE_SIZE);
			break;
		case SIZE_16K:
			reg |= MASK_SET(GITS_BASER_PAGE_SIZE_16K, GITS_BASER_PAGE_SIZE);
			break;
		case SIZE_64K:
			reg |= MASK_SET(GITS_BASER_PAGE_SIZE_64K, GITS_BASER_PAGE_SIZE);
			break;
		}

		reg |= MASK_SET(page_cnt - 1U, GITS_BASER_SIZE);
		reg |= MASK_SET((uintptr_t)table >> GITS_BASER_ADDR_SHIFT, GITS_BASER_ADDR);
		reg |= MASK_SET(GIC_BASER_CACHE_INNERLIKE, GITS_BASER_OUTER_CACHE);
#ifdef CONFIG_GIC_V3_ITS_DMA_NONCOHERENT
		reg |= MASK_SET(GIC_BASER_SHARE_NO, GITS_BASER_SHAREABILITY);
		reg |= MASK_SET(GIC_BASER_CACHE_NCACHEABLE, GITS_BASER_INNER_CACHE);
#else
		reg |= MASK_SET(GIC_BASER_SHARE_INNER, GITS_BASER_SHAREABILITY);
		reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_BASER_INNER_CACHE);
#endif
		reg |= MASK_SET(indirect ? 1 : 0, GITS_BASER_INDIRECT);
		reg |= MASK_SET(1, GITS_BASER_VALID);
		probe = reg & ~MASK(GITS_BASER_VALID);

		sys_write64(probe, data->base + GITS_BASER(i));
		actual = sys_read64(data->base + GITS_BASER(i));
		if ((MASK_GET(actual, GITS_BASER_PAGE_SIZE) !=
		     MASK_GET(reg, GITS_BASER_PAGE_SIZE)) ||
		    (MASK_GET(actual, GITS_BASER_INDIRECT) != (unsigned int)indirect) ||
		    (MASK_GET(actual, GITS_BASER_SIZE) != page_cnt - 1U) ||
		    (MASK_GET(actual, GITS_BASER_ADDR) != MASK_GET(reg, GITS_BASER_ADDR)) ||
		    (MASK_GET(actual, GITS_BASER_SHAREABILITY) !=
		     MASK_GET(reg, GITS_BASER_SHAREABILITY)) ||
		    (MASK_GET(actual, GITS_BASER_INNER_CACHE) !=
		     MASK_GET(reg, GITS_BASER_INNER_CACHE)) ||
		    (MASK_GET(actual, GITS_BASER_OUTER_CACHE) !=
		     MASK_GET(reg, GITS_BASER_OUTER_CACHE))) {
			return -ENOTSUP;
		}

#ifdef CONFIG_GIC_V3_ITS_DMA_NONCOHERENT
		arch_dcache_flush_and_invd_range(table, required_size);
#endif

		baser_regs[i] = reg;

		if (type == GITS_BASER_TYPE_DEVICE && indirect) {
			data->dev_table_is_indirect = true;
			data->indirect_dev_lvl1_table = table;
			data->indirect_dev_lvl1_width = root_bits;
			data->indirect_dev_lvl2_width = lvl2_width;
		}
	}

	if (!device_table_seen ||
	    (!collection_table_seen &&
	     GITS_TYPER_HCC_GET(sys_read64(data->base + GITS_TYPER)) < CONFIG_MP_MAX_NUM_CPUS)) {
		return -ENOTSUP;
	}

	for (i = 0; i < GITS_BASER_NR_REGS; ++i) {
		uint64_t actual;

		if (baser_regs[i] == 0U) {
			continue;
		}
		sys_write64(baser_regs[i], data->base + GITS_BASER(i));
		actual = sys_read64(data->base + GITS_BASER(i));
		if ((MASK_GET(actual, GITS_BASER_ADDR) !=
		     MASK_GET(baser_regs[i], GITS_BASER_ADDR)) ||
		    (MASK_GET(actual, GITS_BASER_VALID) == 0U)) {
			goto disable_tables;
		}
	}

	return 0;

disable_tables:
	for (i = 0; i < GITS_BASER_NR_REGS; ++i) {
		if (baser_regs[i] != 0U) {
			sys_write64(baser_regs[i] &
					    ~(MASK(GITS_BASER_ADDR) | MASK(GITS_BASER_VALID)),
				    data->base + GITS_BASER(i));
		}
	}
	return -ENOTSUP;
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

static struct its_cmd_block *its_allocate_entry(struct gicv3_its_data *data)
{
	struct its_cmd_block *cmd;
	unsigned int count = 1000000;   /* 1s! */

	while (its_queue_full(data)) {
		count--;
		if (!count) {
			LOG_ERR("ITS queue not draining");
			return NULL;
		}
		k_busy_wait(1);
	}

	cmd = data->cmd_write++;

	/* Handle queue wrapping */
	if (data->cmd_write == (data->cmd_base + ITS_CMD_QUEUE_NR_ENTRIES)) {
		data->cmd_write = data->cmd_base;
	}

	/* Clear command  */
	cmd->raw_cmd[0] = 0;
	cmd->raw_cmd[1] = 0;
	cmd->raw_cmd[2] = 0;
	cmd->raw_cmd[3] = 0;

	return cmd;
}

/* Caller holds data->lock. Each post samples CREADR, so it cannot advance a full
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

static int its_post_command(struct gicv3_its_data *data, struct its_cmd_block *cmd)
{
	struct its_cmd_block *cmd_entry;
	uint64_t wr_idx, rd_idx, idx, ticket;
	k_spinlock_key_t key;
	unsigned int count = 1000000;   /* 1s! */
	bool done;

	key = k_spin_lock(&data->lock);

	cmd_entry = its_allocate_entry(data);
	if (!cmd_entry) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	memcpy(cmd_entry, cmd, sizeof(*cmd_entry));
#ifdef CONFIG_GIC_V3_ITS_DMA_NONCOHERENT
	arch_dcache_flush_and_invd_range(cmd_entry, sizeof(*cmd_entry));
#endif

	wr_idx = (data->cmd_write - data->cmd_base) * sizeof(*cmd_entry);
	rd_idx = its_update_cmd_completed(data);
	ticket = ++data->cmd_posted;

	barrier_dsync_fence_full();

	sys_write32(wr_idx, data->base + GITS_CWRITER);

	k_spin_unlock(&data->lock, key);

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
	struct its_cmd_block cmd;

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_SYNC, GITS_CMD_ID);
	cmd.raw_cmd[2] = MASK_SET(rd_addr, GITS_CMD_RDBASE);

	return its_post_command(data, &cmd);
}

static int its_send_mapc_cmd(struct gicv3_its_data *data, uint32_t icid,
			     uintptr_t rd_addr, bool valid)
{
	struct its_cmd_block cmd;

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_MAPC, GITS_CMD_ID);
	cmd.raw_cmd[2] = MASK_SET(icid, GITS_CMD_ICID) | MASK_SET(rd_addr, GITS_CMD_RDBASE) |
			  MASK_SET(valid ? 1 : 0, GITS_CMD_VALID);

	return its_post_command(data, &cmd);
}

static int its_send_mapd_cmd(struct gicv3_its_data *data, uint32_t device_id,
			     uint32_t size, uintptr_t itt_addr, bool valid)
{
	struct its_cmd_block cmd;

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
	struct its_cmd_block cmd;

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
	struct its_cmd_block cmd;

	cmd.raw_cmd[0] = MASK_SET(GITS_CMD_ID_INT, GITS_CMD_ID) |
			  MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd.raw_cmd[1] = MASK_SET(event_id, GITS_CMD_EVENTID);

	return its_post_command(data, &cmd);
}

static int its_send_invall_cmd(struct gicv3_its_data *data, uint32_t icid)
{
	struct its_cmd_block cmd;

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

static int its_check_event(struct gicv3_its_data *data, uint32_t device_id, uint32_t event_id)
{
	struct its_device_slot *slot;
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
	} else if (event_id >= slot->nites) {
		ret = -EINVAL;
	} else {
		ret = 0;
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int gicv3_its_send_int(const struct device *dev, uint32_t device_id, uint32_t event_id)
{
	struct gicv3_its_data *data = dev->data;
	int ret = its_check_event(data, device_id, event_id);

	if (ret != 0) {
		return ret;
	}

	return its_send_int_cmd(data, device_id, event_id);
}

static void its_setup_cmd_queue(const struct device *dev)
{
	const struct gicv3_its_config *cfg = dev->config;
	struct gicv3_its_data *data = dev->data;
	uint64_t reg = 0, tmp;

	/* Zero out cmd table */
	memset(cfg->cmd_queue, 0, cfg->cmd_queue_size);

	reg |= MASK_SET((cfg->cmd_queue_size / SIZE_4K) - 1, GITS_CBASER_SIZE);
	reg |= MASK_SET(GIC_BASER_SHARE_INNER, GITS_CBASER_SHAREABILITY);
	reg |= MASK_SET((uintptr_t)cfg->cmd_queue >> GITS_CBASER_ADDR_SHIFT, GITS_CBASER_ADDR);
	reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_CBASER_OUTER_CACHE);
	reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_CBASER_INNER_CACHE);
	reg |= MASK_SET(1, GITS_CBASER_VALID);

	sys_write64(reg, data->base + GITS_CBASER);

#ifdef CONFIG_GIC_V3_ITS_DMA_NONCOHERENT
	reg &= ~(MASK(GITS_BASER_SHAREABILITY));
#endif
	/* Check whether hardware supports sharable */
	tmp = sys_read64(data->base + GITS_CBASER);
	if (!(tmp & MASK(GITS_BASER_SHAREABILITY))) {
		reg &= ~(MASK(GITS_BASER_SHAREABILITY) | MASK(GITS_BASER_INNER_CACHE));
		reg |= MASK_SET(GIC_BASER_CACHE_NCACHEABLE, GITS_CBASER_INNER_CACHE);
		sys_write64(reg, data->base + GITS_CBASER);
	}

	data->cmd_base = (struct its_cmd_block *)cfg->cmd_queue;
	data->cmd_write = data->cmd_base;
	data->cmd_read = 0U;
	data->cmd_posted = 0U;
	data->cmd_completed = 0U;

	LOG_INF("Allocated %ld entries for command table", ITS_CMD_QUEUE_NR_ENTRIES);

	sys_write64(0, data->base + GITS_CWRITER);
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

static int gicv3_its_map_intid_internal(const struct device *dev, uint32_t device_id,
					uint32_t event_id, unsigned int intid,
					bool *mapping_may_exist);

static int gicv3_its_map_intid(const struct device *dev, uint32_t device_id, uint32_t event_id,
			       unsigned int intid)
{
	bool mapping_may_exist;

	return gicv3_its_map_intid_internal(dev, device_id, event_id, intid, &mapping_may_exist);
}

static int gicv3_its_map_intid_internal(const struct device *dev, uint32_t device_id,
					uint32_t event_id, unsigned int intid,
					bool *mapping_may_exist)
{
	struct gicv3_its_data *data = dev->data;
	int ret;

	*mapping_may_exist = false;
	if ((intid < 8192U) || (intid >= MIN(BIT(data->lpi_id_bits), CONFIG_NUM_IRQS))) {
		return -EINVAL;
	}
	ret = its_check_event(data, device_id, event_id);
	if (ret != 0) {
		return ret;
	}

	/* A timed-out MAPTI may still have reached the ITS. */
	*mapping_may_exist = true;
	ret = its_send_mapti_cmd(data, device_id, event_id, intid, arch_curr_cpu()->id);
	if (ret != 0) {
		LOG_ERR("Failed to map eventid %d to intid %d for deviceid %x",
			event_id, intid, device_id);
		return ret;
	}

	return its_send_sync_cmd(data, gicv3_rdist_get_rdbase(dev, arch_curr_cpu()->id));
}

static int gicv3_its_init_device_id(const struct device *dev, uint32_t device_id,
				    unsigned int nites)
{
	const struct gicv3_its_config *cfg = dev->config;
	struct gicv3_its_data *data = dev->data;
	struct its_device_slot *slot;
	size_t entry_size, alloc_size, offset = 0U;
	size_t slot_index = ARRAY_SIZE(data->device_slots);
	size_t page_index = cfg->indirect_dev_page_count;
	uint64_t typer;
	unsigned int mapd_size;
	k_spinlock_key_t key;
	int ret;

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
	alloc_size = ROUND_UP(CONFIG_GIC_V3_ITS_MAX_VECTORS * entry_size, SIZE_256);
	if (alloc_size > ITS_ITT_SLOT_SIZE) {
		return -ENOMEM;
	}

	if (data->dev_table_is_indirect) {
		offset = device_id >> data->indirect_dev_lvl2_width;
		if (offset >= BIT(data->indirect_dev_lvl1_width)) {
			return -EINVAL;
		}
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
		slot_index = slot - data->device_slots;
		slot->status = -EINPROGRESS;
		k_spin_unlock(&data->lock, key);
		goto mapd;
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

	if (data->dev_table_is_indirect && (data->indirect_dev_lvl1_table[offset] == 0U)) {
		for (size_t i = 0U; i < cfg->indirect_dev_page_count; i++) {
			if (!data->indirect_pages_used[i]) {
				page_index = i;
				break;
			}
		}
		if (page_index == cfg->indirect_dev_page_count) {
			ret = -ENOMEM;
			goto unlock;
		}
		data->indirect_pages_used[page_index] = true;
		data->indirect_dev_lvl1_table[offset] =
			(uintptr_t)cfg->indirect_dev_pages[page_index] |
			MASK_SET(1, GITS_BASER_VALID);
#ifdef CONFIG_GIC_V3_ITS_DMA_NONCOHERENT
		arch_dcache_flush_and_invd_range(data->indirect_dev_lvl1_table + offset,
						 GITS_LVL1_ENTRY_SIZE);
#endif
		barrier_dsync_fence_full();
	}
	slot = &data->device_slots[slot_index];
	slot->device_id = device_id;
	slot->status = -EINPROGRESS;
	slot->used = true;
	k_spin_unlock(&data->lock, key);

mapd:
	LOG_INF("Using ITT for DeviceID %x and %u vectors (%zu bytes per entry)", device_id,
		CONFIG_GIC_V3_ITS_MAX_VECTORS, entry_size);

	ret = its_send_mapd_cmd(data, device_id, mapd_size, (uintptr_t)cfg->itts[slot_index], true);
	if (ret != 0) {
		LOG_ERR("Failed to map device id %x ITT table", device_id);
	}
	key = k_spin_lock(&data->lock);
	if (ret == 0) {
		slot->nites = nites;
	}
	slot->status = ret;
	k_spin_unlock(&data->lock, key);
	return ret;

unlock:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static unsigned int gicv3_its_alloc_intids(const struct device *dev, unsigned int count)
{
	struct gicv3_its_data *data = dev->data;
	unsigned int limit = MIN(BIT(data->lpi_id_bits), CONFIG_NUM_IRQS);
	size_t offset;
	int ret;

	if ((limit <= 8192U) || (count == 0U) || (count > limit - 8192U)) {
		return 0U;
	}

	ret = sys_bitarray_alloc(&its_lpi_intids, count, &offset);
	if ((ret != 0) || (offset + count > limit - 8192U)) {
		if (ret == 0) {
			(void)sys_bitarray_free(&its_lpi_intids, count, offset);
		}
		return 0U;
	}

	return 8192U + offset;
}

static unsigned int gicv3_its_alloc_map_intids(const struct device *dev, uint32_t device_id,
					       unsigned int count)
{
	struct gicv3_its_data *data = dev->data;
	size_t offset;
	unsigned int first_intid, mapped = 0U;
	bool mapping_may_exist = false;
	int ret, cleanup_ret = 0;

	if ((count == 0U) || (count > CONFIG_GIC_V3_ITS_MAX_VECTORS)) {
		return 0U;
	}

	ret = its_check_event(data, device_id, count - 1U);
	if (ret != 0) {
		return 0U;
	}

	first_intid = gicv3_its_alloc_intids(dev, count);
	if (first_intid == 0U) {
		return 0U;
	}
	offset = first_intid - 8192U;

	for (unsigned int i = 0U; i < count; i++) {
		mapping_may_exist = false;
		ret = gicv3_its_map_intid_internal(dev, device_id, i, first_intid + i,
						   &mapping_may_exist);
		if (ret != 0) {
			break;
		}
		mapped++;
	}

	if (mapped == count) {
		return first_intid;
	}

	for (unsigned int i = 0U; i < mapped; i++) {
		cleanup_ret = its_send_discard_cmd(data, device_id, i);
		if (cleanup_ret != 0) {
			break;
		}
	}
	if ((cleanup_ret == 0) && (mapped > 0U)) {
		cleanup_ret =
			its_send_sync_cmd(data, gicv3_rdist_get_rdbase(dev, arch_curr_cpu()->id));
	}
	if (cleanup_ret != 0) {
		LOG_ERR("Failed to discard mapped MSI EventIDs for deviceid %x: %d", device_id,
			cleanup_ret);
		return 0U;
	}

	if ((mapped > 0U) && (sys_bitarray_free(&its_lpi_intids, mapped, offset) != 0)) {
		LOG_ERR("Failed to release mapped MSI INTIDs for deviceid %x", device_id);
		return 0U;
	}

	/* Retain an INTID if its MAPTI command may have reached the ITS. */
	offset += mapped + (mapping_may_exist ? 1U : 0U);
	count -= mapped + (mapping_may_exist ? 1U : 0U);
	if ((count > 0U) && (sys_bitarray_free(&its_lpi_intids, count, offset) != 0)) {
		LOG_ERR("Failed to release unused MSI INTIDs for deviceid %x", device_id);
	}

	return 0U;
}

static unsigned int gicv3_its_alloc_intid(const struct device *dev)
{
	return gicv3_its_alloc_intids(dev, 1U);
}

static uint32_t gicv3_its_get_msi_addr(const struct device *dev)
{
	const struct gicv3_its_config *cfg = (const struct gicv3_its_config *)dev->config;

	return cfg->base_addr + GITS_TRANSLATER;
}

#define ITS_RDIST_MAP(n)									  \
	{											  \
		const struct device *const dev = DEVICE_DT_INST_GET(n);				  \
		struct gicv3_its_data *data;							  \
		int ret;									  \
												  \
		if (dev) {									  \
			data = (struct gicv3_its_data *) dev->data;				  \
			ret = its_send_mapc_cmd(data, arch_curr_cpu()->id,			  \
						gicv3_rdist_get_rdbase(dev, arch_curr_cpu()->id), \
						true);						  \
			if (ret) {								  \
				LOG_ERR("Failed to map CPU%d redistributor",			  \
					arch_curr_cpu()->id);					  \
			}									  \
		}										  \
	}

void its_rdist_map(void)
{
	DT_INST_FOREACH_STATUS_OKAY(ITS_RDIST_MAP)
}

#define ITS_RDIST_INVALL(n)									\
	{											\
		const struct device *const dev = DEVICE_DT_INST_GET(n);				\
		struct gicv3_its_data *data;							\
		int ret;									\
												\
		if (dev) {									\
			data = (struct gicv3_its_data *) dev->data;				\
			ret = its_send_invall_cmd(data, arch_curr_cpu()->id);			\
			if (ret) {								\
				LOG_ERR("Failed to sync RDIST LPI cache for CPU%d",		\
					arch_curr_cpu()->id);					\
			}									\
												\
			its_send_sync_cmd(data,							\
					  gicv3_rdist_get_rdbase(dev, arch_curr_cpu()->id));	\
		}										\
	}

void its_rdist_invall(void)
{
	DT_INST_FOREACH_STATUS_OKAY(ITS_RDIST_INVALL)
}

static int gicv3_its_init(const struct device *dev)
{
	const struct gicv3_its_config *cfg = dev->config;
	struct gicv3_its_data *data = dev->data;
	uint32_t reg;
	int ret;

	device_map(&data->base, cfg->base_addr, cfg->base_size, K_MEM_CACHE_NONE);
	data->lpi_id_bits = MIN(GICD_TYPER_IDBITS(sys_read32(GICD_TYPER)), ITS_LPI_ID_BITS);
	if (data->lpi_id_bits < 14U) {
		return -ENOTSUP;
	}

	ret = its_force_quiescent(data);
	if (ret) {
		LOG_ERR("Failed to quiesce, giving up");
		return ret;
	}

	ret = its_setup_tables(data, cfg);
	if (ret) {
		LOG_ERR("Failed to set up tables: %d", ret);
		return ret;
	}

#ifdef CONFIG_GIC_V3_ITS_DMA_NONCOHERENT
	arch_dcache_flush_and_invd_range(cfg->indirect_dev_pages,
					 cfg->indirect_dev_page_count * SIZE_64K);
	arch_dcache_flush_and_invd_range(cfg->itts,
					 CONFIG_GIC_V3_ITS_MAX_DEVICES * ITS_ITT_SLOT_SIZE);
#endif

	its_setup_cmd_queue(dev);

	reg = sys_read32(data->base + GITS_CTLR);
	reg |= MASK_SET(1, GITS_CTLR_ENABLED);
	sys_write32(reg, data->base + GITS_CTLR);

	/* Map the boot CPU id to the CPU redistributor */
	ret = its_send_mapc_cmd(data, arch_curr_cpu()->id,
				gicv3_rdist_get_rdbase(dev, arch_curr_cpu()->id), true);
	if (ret) {
		LOG_ERR("Failed to map boot CPU redistributor");
		return ret;
	}

	return 0;
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

#define GICV3_ITS_INIT(n)						       \
	BUILD_ASSERT(DT_INST_PROP_OR(n, zephyr_device_id_bits, ITS_MAX_DEVICE_ID_BITS) > 0U && \
		     DT_INST_PROP_OR(n, zephyr_device_id_bits, ITS_MAX_DEVICE_ID_BITS) <= \
			ITS_MAX_DEVICE_ID_BITS); \
	static uint8_t gicv3_its_device_table##n[ITS_DEVICE_TABLE_SIZE(n)] \
		__aligned(SIZE_64K); \
	static uint8_t gicv3_its_collection_table##n[ITS_COLLECTION_TABLE_SIZE] \
		__aligned(SIZE_64K); \
	static uint8_t gicv3_its_indirect_pages##n[ITS_INDIRECT_PAGE_COUNT(n)][SIZE_64K] \
		__aligned(SIZE_64K); \
	static uint8_t gicv3_its_itts##n[CONFIG_GIC_V3_ITS_MAX_DEVICES][ITS_ITT_SLOT_SIZE] \
		__aligned(SIZE_256); \
	static struct its_cmd_block gicv3_its_cmd##n[ITS_CMD_QUEUE_NR_ENTRIES] \
	__aligned(ITS_CMD_QUEUE_SIZE);					       \
	static struct gicv3_its_data gicv3_its_data##n;			       \
	static const struct gicv3_its_config gicv3_its_config##n = {	       \
		.base_addr = DT_INST_REG_ADDR(n),			       \
		.base_size = DT_INST_REG_SIZE(n),			       \
		.cmd_queue = gicv3_its_cmd##n,				       \
		.cmd_queue_size = sizeof(gicv3_its_cmd##n),		       \
		.device_table = gicv3_its_device_table##n, \
		.device_table_size = sizeof(gicv3_its_device_table##n), \
		.collection_table = gicv3_its_collection_table##n, \
		.collection_table_size = sizeof(gicv3_its_collection_table##n), \
		.indirect_dev_pages = gicv3_its_indirect_pages##n, \
		.itts = gicv3_its_itts##n, \
		.device_id_bits = ITS_DEVICE_ID_BITS(n), \
		.indirect_dev_page_count = ITS_INDIRECT_PAGE_COUNT(n), \
	};								       \
	DEVICE_DT_INST_DEFINE(n, &gicv3_its_init, NULL,			       \
			      &gicv3_its_data##n,			       \
			      &gicv3_its_config##n,			       \
			      PRE_KERNEL_1,				       \
			      CONFIG_INTC_INIT_PRIORITY,		       \
			      &gicv3_its_api);

DT_INST_FOREACH_STATUS_OKAY(GICV3_ITS_INIT)
