/*
 * Copyright 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_gicv3_its, LOG_LEVEL_ERR);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>

#include "intc_gic_common_priv.h"
#include "intc_gicv3_priv.h"

#define DT_DRV_COMPAT   arm_gic_v3_its

/*
 * Current ITS implementation only handle GICv3 ITS physical interruption generation
 * Implementation is designed for the PCIe MSI/MSI-X use-case in mind.
 */

#define GITS_BASER_NR_REGS              8

/* convenient access to all redistributors base address */
extern mem_addr_t gic_rdists[CONFIG_MP_NUM_CPUS];

#define SIZE_256                        256
#define SIZE_4K                         KB(4)
#define SIZE_16K                        KB(16)
#define SIZE_64K                        KB(64)

struct its_cmd_block {
	uint64_t raw_cmd[4];
};

#define ITS_CMD_QUEUE_SIZE              SIZE_64K
#define ITS_CMD_QUEUE_NR_ENTRIES        (ITS_CMD_QUEUE_SIZE / sizeof(struct its_cmd_block))

struct gicv3_its_data {
	mm_reg_t base;
	struct its_cmd_block *cmd_base;
	struct its_cmd_block *cmd_write;
	bool dev_table_is_indirect;
	uint64_t *indirect_dev_lvl1_table;
	size_t indirect_dev_lvl1_width;
	size_t indirect_dev_lvl2_width;
	size_t indirect_dev_page_size;
};

struct gicv3_its_config {
	uintptr_t base_addr;
	size_t base_size;
	struct its_cmd_block *cmd_queue;
	size_t cmd_queue_size;
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

/* wait 500ms & wakeup every millisecond */
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

		k_msleep(1);
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

		reg &= ~MASK(GITS_BASER_PAGE_SIZE);
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

static int its_alloc_tables(struct gicv3_its_data *data)
{
	unsigned int device_ids = GITS_TYPER_DEVBITS_GET(sys_read64(data->base + GITS_TYPER)) + 1;
	int i;

	for (i = 0; i < GITS_BASER_NR_REGS; ++i) {
		uint64_t reg = sys_read64(data->base + GITS_BASER(i));
		unsigned int type = GITS_BASER_TYPE_GET(reg);
		size_t page_size, entry_size, page_cnt, lvl2_width = 0;
		bool indirect = false;
		void *alloc_addr;

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
			if (device_ids > 16) {
				/* Use the largest possible page size for indirect */
				page_size = its_probe_baser_page_size(data, i);

				/*
				 * lvl1 table size:
				 * subtract ID bits that sparse lvl2 table from 'ids'
				 * which is reported by ITS hardware times lvl1 table
				 * entry size.
				 */
				lvl2_width = fls_z(page_size / entry_size) - 1;
				device_ids -= lvl2_width + 1;

				/* The level 1 entry size is a 64bit pointer */
				entry_size = sizeof(uint64_t);

				indirect = true;
			}

			page_cnt = ROUND_UP(entry_size << device_ids, page_size) / page_size;
			break;
		case GITS_BASER_TYPE_COLLECTION:
			page_cnt = ROUND_UP(entry_size * CONFIG_MP_NUM_CPUS, page_size) / page_size;
			break;
		default:
			continue;
		}

		LOG_INF("Allocating %s table of %ldx%ldK pages (%ld bytes entry)",
			its_base_type_string[type], page_cnt, page_size / 1024, entry_size);

		alloc_addr = k_aligned_alloc(page_size, page_size * page_cnt);
		if (!alloc_addr) {
			return -ENOMEM;
		}

		memset(alloc_addr, 0, page_size * page_cnt);

		switch (page_size) {
		case SIZE_4K:
			reg = MASK_SET(GITS_BASER_PAGE_SIZE_4K, GITS_BASER_PAGE_SIZE);
			break;
		case SIZE_16K:
			reg = MASK_SET(GITS_BASER_PAGE_SIZE_16K, GITS_BASER_PAGE_SIZE);
			break;
		case SIZE_64K:
			reg = MASK_SET(GITS_BASER_PAGE_SIZE_64K, GITS_BASER_PAGE_SIZE);
			break;
		}

		reg |= MASK_SET(page_cnt - 1, GITS_BASER_SIZE);
		reg |= MASK_SET(GIC_BASER_SHARE_INNER, GITS_BASER_SHAREABILITY);
		reg |= MASK_SET((uintptr_t)alloc_addr >> GITS_BASER_ADDR_SHIFT, GITS_BASER_ADDR);
		reg |= MASK_SET(GIC_BASER_CACHE_INNERLIKE, GITS_BASER_OUTER_CACHE);
		reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_BASER_INNER_CACHE);
		reg |= MASK_SET(indirect ? 1 : 0, GITS_BASER_INDIRECT);
		reg |= MASK_SET(1, GITS_BASER_VALID);

		sys_write64(reg, data->base + GITS_BASER(i));

		/* TOFIX: check page size & SHAREABILITY validity after write */

		if (type == GITS_BASER_TYPE_DEVICE && indirect) {
			data->dev_table_is_indirect = indirect;
			data->indirect_dev_lvl1_table = alloc_addr;
			data->indirect_dev_lvl1_width = device_ids;
			data->indirect_dev_lvl2_width = lvl2_width;
			data->indirect_dev_page_size = page_size;
			LOG_DBG("%s table Indirection enabled", its_base_type_string[type]);
		}
	}

	return 0;
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
		k_usleep(1);
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

static int its_post_command(struct gicv3_its_data *data, struct its_cmd_block *cmd)
{
	uint64_t wr_idx, rd_idx, idx;
	unsigned int count = 1000000;   /* 1s! */

	wr_idx = (data->cmd_write - data->cmd_base) * sizeof(*cmd);
	rd_idx = sys_read32(data->base + GITS_CREADR);

	dsb();

	sys_write32(wr_idx, data->base + GITS_CWRITER);

	while (1) {
		idx = sys_read32(data->base + GITS_CREADR);

		if (idx == wr_idx) {
			break;
		}

		count--;
		if (!count) {
			LOG_ERR("ITS queue timeout (rd %lld => %lld => wr %lld)",
				rd_idx, idx, wr_idx);
			return -ETIMEDOUT;
		}
		k_usleep(1);
	}

	return 0;
}

static int its_send_sync_cmd(struct gicv3_its_data *data, uintptr_t rd_addr)
{
	struct its_cmd_block *cmd = its_allocate_entry(data);

	if (!cmd) {
		return -EBUSY;
	}

	cmd->raw_cmd[0] = MASK_SET(GITS_CMD_ID_SYNC, GITS_CMD_ID);
	cmd->raw_cmd[2] = MASK_SET(rd_addr >> GITS_CMD_RDBASE_ALIGN, GITS_CMD_RDBASE);

	return its_post_command(data, cmd);
}

static int its_send_mapc_cmd(struct gicv3_its_data *data, uint32_t icid,
			     uintptr_t rd_addr, bool valid)
{
	struct its_cmd_block *cmd = its_allocate_entry(data);

	if (!cmd) {
		return -EBUSY;
	}

	cmd->raw_cmd[0] = MASK_SET(GITS_CMD_ID_MAPC, GITS_CMD_ID);
	cmd->raw_cmd[2] = MASK_SET(icid, GITS_CMD_ICID) |
			  MASK_SET(rd_addr >> GITS_CMD_RDBASE_ALIGN, GITS_CMD_RDBASE) |
			  MASK_SET(valid ? 1 : 0, GITS_CMD_VALID);

	return its_post_command(data, cmd);
}

static int its_send_mapd_cmd(struct gicv3_its_data *data, uint32_t device_id,
			     uint32_t size, uintptr_t itt_addr, bool valid)
{
	struct its_cmd_block *cmd = its_allocate_entry(data);

	if (!cmd) {
		return -EBUSY;
	}

	cmd->raw_cmd[0] = MASK_SET(GITS_CMD_ID_MAPD, GITS_CMD_ID) |
			  MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd->raw_cmd[1] = MASK_SET(size, GITS_CMD_SIZE);
	cmd->raw_cmd[2] = MASK_SET(itt_addr >> GITS_CMD_ITTADDR_ALIGN, GITS_CMD_ITTADDR) |
			  MASK_SET(valid ? 1 : 0, GITS_CMD_VALID);

	return its_post_command(data, cmd);
}

static int its_send_mapti_cmd(struct gicv3_its_data *data, uint32_t device_id,
			      uint32_t event_id, uint32_t intid, uint32_t icid)
{
	struct its_cmd_block *cmd = its_allocate_entry(data);

	if (!cmd) {
		return -EBUSY;
	}

	cmd->raw_cmd[0] = MASK_SET(GITS_CMD_ID_MAPTI, GITS_CMD_ID) |
			  MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd->raw_cmd[1] = MASK_SET(event_id, GITS_CMD_EVENTID) |
			  MASK_SET(intid, GITS_CMD_PINTID);
	cmd->raw_cmd[2] = MASK_SET(icid, GITS_CMD_ICID);

	return its_post_command(data, cmd);
}

static int its_send_int_cmd(struct gicv3_its_data *data, uint32_t device_id,
			    uint32_t event_id)
{
	struct its_cmd_block *cmd = its_allocate_entry(data);

	if (!cmd) {
		return -EBUSY;
	}

	cmd->raw_cmd[0] = MASK_SET(GITS_CMD_ID_INT, GITS_CMD_ID) |
			  MASK_SET(device_id, GITS_CMD_DEVICEID);
	cmd->raw_cmd[1] = MASK_SET(event_id, GITS_CMD_EVENTID);

	return its_post_command(data, cmd);
}

static int its_send_invall_cmd(struct gicv3_its_data *data, uint32_t icid)
{
	struct its_cmd_block *cmd = its_allocate_entry(data);

	if (!cmd) {
		return -EBUSY;
	}

	cmd->raw_cmd[0] = MASK_SET(GITS_CMD_ID_INVALL, GITS_CMD_ID);
	cmd->raw_cmd[2] = MASK_SET(icid, GITS_CMD_ICID);

	return its_post_command(data, cmd);
}

static int gicv3_its_send_int(const struct device *dev, uint32_t device_id, uint32_t event_id)
{
	struct gicv3_its_data *data = dev->data;

	/* TOFIX check device_id & event_id bounds */

	return its_send_int_cmd(data, device_id, event_id);
}

static void its_setup_cmd_queue(const struct device *dev)
{
	const struct gicv3_its_config *cfg = dev->config;
	struct gicv3_its_data *data = dev->data;
	uint64_t reg = 0;

	/* Zero out cmd table */
	memset(cfg->cmd_queue, 0, cfg->cmd_queue_size);

	reg |= MASK_SET(cfg->cmd_queue_size / SIZE_4K, GITS_CBASER_SIZE);
	reg |= MASK_SET(GIC_BASER_SHARE_INNER, GITS_CBASER_SHAREABILITY);
	reg |= MASK_SET((uintptr_t)cfg->cmd_queue >> GITS_CBASER_ADDR_SHIFT, GITS_CBASER_ADDR);
	reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_CBASER_OUTER_CACHE);
	reg |= MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_CBASER_INNER_CACHE);
	reg |= MASK_SET(1, GITS_CBASER_VALID);

	sys_write64(reg, data->base + GITS_CBASER);

	data->cmd_base = (struct its_cmd_block *)cfg->cmd_queue;
	data->cmd_write = data->cmd_base;

	LOG_INF("Allocated %ld entries for command table", ITS_CMD_QUEUE_NR_ENTRIES);

	sys_write64(0, data->base + GITS_CWRITER);
}

static uintptr_t gicv3_rdist_get_rdbase(const struct device *dev, unsigned int cpuid)
{
	struct gicv3_its_data *data = dev->data;
	uint64_t typer = sys_read64(data->base + GITS_TYPER);

	if (GITS_TYPER_PTA_GET(typer)) {
		return gic_rdists[cpuid];
	} else {
		return GICR_TYPER_PROCESSOR_NUMBER_GET(sys_read64(gic_rdists[cpuid] + GICR_TYPER));
	}
}

static int gicv3_its_map_intid(const struct device *dev, uint32_t device_id, uint32_t event_id,
			       unsigned int intid)
{
	struct gicv3_its_data *data = dev->data;
	int ret;

	/* TOFIX check device_id, event_id & intid bounds */

	if (intid < 8192) {
		return -EINVAL;
	}

	/* The CPU id directly maps as ICID for the current CPU redistributor */
	ret = its_send_mapti_cmd(data, device_id, event_id, intid, arch_curr_cpu()->id);
	if (ret) {
		LOG_ERR("Failed to map eventid %d to intid %d for deviceid %x",
			event_id, intid, device_id);
		return ret;
	}

	return its_send_sync_cmd(data, gicv3_rdist_get_rdbase(dev, arch_curr_cpu()->id));
}

static int gicv3_its_init_device_id(const struct device *dev, uint32_t device_id,
				    unsigned int nites)
{
	struct gicv3_its_data *data = dev->data;
	size_t entry_size, alloc_size;
	int nr_ites;
	void *itt;
	int ret;

	/* TOFIX check device_id & nites bounds */

	entry_size = GITS_TYPER_ITT_ENTRY_SIZE_GET(sys_read64(data->base + GITS_TYPER)) + 1;

	if (data->dev_table_is_indirect) {
		size_t offset = device_id >> data->indirect_dev_lvl2_width;

		/* Check if DeviceID can fit in the Level 1 table */
		if (offset > (1 << data->indirect_dev_lvl1_width)) {
			return -EINVAL;
		}

		/* Check if a Level 2 table has already been allocated for the DeviceID */
		if (!data->indirect_dev_lvl1_table[offset]) {
			void *alloc_addr;

			LOG_INF("Allocating Level 2 Device %ldK table",
				data->indirect_dev_page_size / 1024);

			alloc_addr = k_aligned_alloc(data->indirect_dev_page_size,
						     data->indirect_dev_page_size);
			if (!alloc_addr) {
				return -ENOMEM;
			}

			memset(alloc_addr, 0, data->indirect_dev_page_size);

			data->indirect_dev_lvl1_table[offset] = (uintptr_t)alloc_addr |
								MASK_SET(1, GITS_BASER_VALID);

			dsb();
		}
	}

	/* ITT must be of power of 2 */
	nr_ites = MAX(2, nites);
	alloc_size = ROUND_UP(nr_ites * entry_size, 256);

	LOG_INF("Allocating ITT for DeviceID %x and %d vectors (%ld bytes entry)",
		device_id, nr_ites, entry_size);

	itt = k_aligned_alloc(256, alloc_size);
	if (!itt) {
		return -ENOMEM;
	}

	/* size is log2(ites) - 1, equivalent to (fls(ites) - 1) - 1 */
	ret = its_send_mapd_cmd(data, device_id, fls_z(nr_ites) - 2, (uintptr_t)itt, true);
	if (ret) {
		LOG_ERR("Failed to map device id %x ITT table", device_id);
		return ret;
	}

	return 0;
}

static unsigned int gicv3_its_alloc_intid(const struct device *dev)
{
	return atomic_inc(&nlpi_intid);
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

	ret = its_force_quiescent(data);
	if (ret) {
		LOG_ERR("Failed to quiesce, giving up");
		return ret;
	}

	ret = its_alloc_tables(data);
	if (ret) {
		LOG_ERR("Failed to allocate tables, giving up");
		return ret;
	}

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

struct its_driver_api gicv3_its_api = {
	.alloc_intid = gicv3_its_alloc_intid,
	.setup_deviceid = gicv3_its_init_device_id,
	.map_intid = gicv3_its_map_intid,
	.send_int = gicv3_its_send_int,
	.get_msi_addr = gicv3_its_get_msi_addr,
};

#define GICV3_ITS_INIT(n)						       \
	static struct its_cmd_block gicv3_its_cmd##n[ITS_CMD_QUEUE_NR_ENTRIES] \
	__aligned(ITS_CMD_QUEUE_SIZE);					       \
	static struct gicv3_its_data gicv3_its_data##n;			       \
	static const struct gicv3_its_config gicv3_its_config##n = {	       \
		.base_addr = DT_INST_REG_ADDR(n),			       \
		.base_size = DT_INST_REG_SIZE(n),			       \
		.cmd_queue = gicv3_its_cmd##n,				       \
		.cmd_queue_size = sizeof(gicv3_its_cmd##n),		       \
	};								       \
	DEVICE_DT_INST_DEFINE(n, &gicv3_its_init, NULL,			       \
			      &gicv3_its_data##n,			       \
			      &gicv3_its_config##n,			       \
			      POST_KERNEL,				       \
			      CONFIG_INTC_INIT_PRIORITY,		       \
			      &gicv3_its_api);

DT_INST_FOREACH_STATUS_OKAY(GICV3_ITS_INIT)
