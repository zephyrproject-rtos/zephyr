/*
 * Copyright (c) 2020 STMicroelectronics
 * Copyright (c) 2024 Maurer Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * In addition to the standard ELF segments, most remote processors would
 * also include a special section which we call "the resource table".
 *
 * The resource table contains system resources that the remote processor
 * requires before it should be powered on, such as allocation of physically
 * contiguous memory, or iommu mapping of certain on-chip peripherals.

 * In addition to system resources, the resource table may also contain
 * resource entries that publish the existence of supported features
 * or configurations by the remote processor, such as trace buffers and
 * supported virtio devices (and their configurations).

 * Dependencies:
 *   to be compliant with Linux kernel OS the resource table must be linked in a
 *   specific section named ".resource_table".

 * Related documentation:
 *   https://www.kernel.org/doc/Documentation/remoteproc.txt
 *   https://github.com/OpenAMP/open-amp/wiki/OpenAMP-Life-Cycle-Management
 */

#include <zephyr/drivers/remoteproc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/linker/devicetree_regions.h>

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>

LOG_MODULE_REGISTER(openamp_remoteproc, CONFIG_REMOTEPROC_LOG_LEVEL);

#define REMOTEPROC_NODE DT_PATH(remoteproc)

#define CARVEOUTS DT_CHILD(REMOTEPROC_NODE, carveouts)
#define VDEVS DT_CHILD(REMOTEPROC_NODE, devices)

#define NUM_CARVEOUTS DT_PROP_LEN(REMOTEPROC_NODE, carveouts)
#define NUM_VDEVS DT_CHILD_NUM(REMOTEPROC_NODE)

#define RPMSG_IPU_C0_FEATURES   1

#define VRING_RX_ADDRESS        -1  /* allocated by Master processor */
#define VRING_TX_ADDRESS        -1  /* allocated by Master processor */
#define VRING_BUFF_ADDRESS      -1  /* allocated by Master processor */
#define VRING_ALIGNMENT         16  /* fixed to match with Linux constraint */

#define RESOURCE_TABLE_ATTR	\
__attribute__((__section__(DT_PROP(VDEV_NODE, resource_table)))))


#ifdef CONFIG_RAM_CONSOLE_BUFFER_SECTION
#if DT_HAS_CHOSEN(zephyr_ram_console)

static const uint32_t ram_console_buf = DT_REG_ADDR(DT_CHOSEN(zephyr_ram_console));

#else
#error "Lack of chosen property zephyr,ram_console!"
#endif

#else
extern char ram_console_buf[];
#endif

#define __resource Z_GENERIC_SECTION(.resource_table)



#define CARVEOUT_OFFSET(node_id, prop, idx) offsetof(struct fw_resource_table, carveouts[idx]),
#define VDEV_OFFSET(node_id) offsetof(struct fw_resource_table, vdevs[DT_NODE_CHILD_IDX(node_id)]),

#define CARVEOUT_ENTRY(node_id, prop, idx) (struct fw_rsc_carveout) {  \
	.type = RSC_CARVEOUT,  \
	.da = DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, carveouts, idx)),  \
	.pa = DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, carveouts, idx)),  \
	.len = DT_REG_SIZE(DT_PHANDLE_BY_IDX(node_id, carveouts, idx)),  \
	.flags = 0,  \
	.name = DT_PROP_BY_PHANDLE_IDX(node_id, carveouts, idx, zephyr_memory_region) \
	},

#define  VDEV_ENTRY(node_id) (struct fw_rsc_vdev_vrings) {  \
	.vdev = {  \
		.type =	RSC_VDEV, .id = VIRTIO_ID_RPMSG, .notifyid = 0,  \
		.dfeatures = RPMSG_IPU_C0_FEATURES, .gfeatures = 0,  \
		.config_len = 0, .status = 0,  \
		.num_of_vrings = VRING_COUNT,  \
	},  \
	.vring0 = {  \
		.da = VRING_TX_ADDRESS, .align = VRING_ALIGNMENT,  \
		.num = DT_PROP(node_id, num_tx_buffers),  \
		.notifyid = VRING0_ID, 0  \
	},  \
	.vring1 = {  \
		.da = VRING_RX_ADDRESS, .align = VRING_ALIGNMENT,  \
		.num = DT_PROP(node_id, num_rx_buffers),  \
		.notifyid = VRING1_ID, 0  \
	}  \
},


#define FOREACH_CARVEOUT(fn) DT_FOREACH_PROP_ELEM(REMOTEPROC_NODE, carveouts, fn)
#define FOREACH_VDEV(fn) DT_FOREACH_CHILD(REMOTEPROC_NODE, fn)

#define CARVEOUT_ENTRIES FOREACH_CARVEOUT(CARVEOUT_ENTRY)

#define VDEV_ENTRIES FOREACH_VDEV(VDEV_ENTRY)


enum rsc_table_entries {
#if NUM_CARVEOUTS > 0
	RSC_TABLE_CARVEOUT_ENTRIES = NUM_CARVEOUTS - 1,
#endif
#if NUM_VDEVS > 0
	RSC_TABLE_VDEV_ENTRY = NUM_CARVEOUTS + NUM_VDEVS - 1,
#endif
#if defined(CONFIG_RAM_CONSOLE)
	RSC_TABLE_TRACE_ENTRY,
#endif
	RSC_TABLE_NUM_ENTRIES
};

struct fw_rsc_vdev_vrings {
	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;
} METAL_PACKED_END __aligned(8);

struct fw_resource_table {
	struct resource_table hdr;
	uint32_t offset[RSC_TABLE_NUM_ENTRIES];
#if NUM_CARVEOUTS > 0
	struct fw_rsc_carveout carveouts[NUM_CARVEOUTS];
#endif
#if NUM_VDEVS > 0
	struct fw_rsc_vdev_vrings vdevs[NUM_VDEVS];
#endif

#if defined(CONFIG_RAM_CONSOLE)
	/* rpmsg trace entry */
	struct fw_rsc_trace cm_trace;
#endif
} METAL_PACKED_END __aligned(8);


struct fw_resource_table __resource resource_table = {
	.hdr = {
		.ver = 1,
		.num = RSC_TABLE_NUM_ENTRIES,
	},

	.offset = {
#if NUM_CARVEOUTS > 0
		FOREACH_CARVEOUT(CARVEOUT_OFFSET)
#endif

#if NUM_VDEVS > 0
		FOREACH_VDEV(VDEV_OFFSET)
#endif

#if defined(CONFIG_RAM_CONSOLE)
		offsetof(struct fw_resource_table, cm_trace),
#endif
	},

#if NUM_CARVEOUTS > 0
	.carveouts = { CARVEOUT_ENTRIES },
#endif
#if NUM_VDEVS > 0
	.vdevs = { VDEV_ENTRIES },
#endif

#if defined(CONFIG_RAM_CONSOLE)
	.cm_trace = {
		.type =	RSC_TRACE, .da = (uint32_t)ram_console_buf,
		.len = CONFIG_RAM_CONSOLE_BUFFER_SIZE,
		.name = "Zephyr_log",
	},
#endif
};

#define CARVEOUT_LOOKUP_ENTRY(node_id, prop, idx) { .carveout = &resource_table.carveouts[idx] },

static struct metal_io_region resource_table_region;

#if NUM_CARVEOUTS > 0
static struct  {
	const struct fw_rsc_carveout *carveout;
	struct metal_io_region region;
} carveout_lut[NUM_CARVEOUTS] = {
	FOREACH_CARVEOUT(CARVEOUT_LOOKUP_ENTRY)
};
#endif


struct fw_resource_table *remoteproc_get_rsc_table(void)
{
	return &resource_table;
}

size_t rsc_table_get_rsc_table_size(void)
{
	return sizeof(resource_table);
}

unsigned int remoteproc_get_num_carveouts(void)
{
	return NUM_CARVEOUTS;
}

struct fw_rsc_carveout *remoteproc_get_carveout_by_name(const char *name)
{
#if NUM_CARVEOUTS > 0
	struct fw_resource_table *rsc_table = &resource_table;

	for (int i = 0; i < NUM_CARVEOUTS; i++) {
		if (strcmp(rsc_table->carveouts[i].name, name) == 0) {
			return &rsc_table->carveouts[i];
		}
	}
#endif
	return NULL;
}

struct fw_rsc_carveout *remoteproc_get_carveout_by_idx(unsigned int idx)
{
#if NUM_CARVEOUTS > 0
	struct fw_resource_table *rsc_table = &resource_table;

	if (idx >= NUM_CARVEOUTS) {
		return NULL;
	}

	return &rsc_table->carveouts[idx];
#else
	return NULL;
#endif
}

struct fw_rsc_vdev *remoteproc_get_vdev(unsigned int vdev_idx)
{
#if NUM_VDEVS > 0
	struct fw_resource_table *rsc_table = &resource_table;

	/* multiple vdev instances not yet supported */
	if (vdev_idx >= NUM_VDEVS) {
		return NULL;
	}

	return &rsc_table->vdevs[vdev_idx].vdev;
#else
	return NULL;
#endif
}

struct fw_rsc_vdev_vring *remoteproc_get_vring(unsigned int vdev_idx, unsigned int vring_idx)
{
#if NUM_VDEVS > 0
	struct fw_rsc_vdev_vrings *rsc_vdev;
	struct fw_resource_table *rsc_table = &resource_table;

	if (vdev_idx >= NUM_VDEVS) {
		return NULL;
	}

	if (vring_idx >= 2) {
		return NULL;
	}

	rsc_vdev = &rsc_table->vdevs[vdev_idx];

	return vring_idx == 0 ? &rsc_vdev->vring0 : &rsc_vdev->vring1;
#else
	return NULL;
#endif
}

static int metal_init_once(void)
{
	static int status = -1;

	if (status) {
		const struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

		status = metal_init(&metal_params);

		if (status) {
			LOG_ERR("metal_init: failed: %d\n", status);
			return status;
		}

		metal_io_init(&resource_table_region, &resource_table,
					  (metal_phys_addr_t *)&resource_table,
					  sizeof(resource_table), -1, 0, NULL);

#if NUM_CARVEOUTS > 0
		for (int i = 0; i < NUM_CARVEOUTS; i++) {
			const struct fw_rsc_carveout *carveout = carveout_lut[i].carveout;
			struct metal_io_region *region = &carveout_lut[i].region;

			metal_io_init(region, (void *)carveout->da,
						  (metal_phys_addr_t *)&carveout->pa,
						  carveout->len, -1, 0, NULL);

			LOG_INF("metal_init: name=%s: %x\n", carveout->name, carveout->da);
		}
#endif

	}
	return status;
}

struct metal_io_region *remoteproc_get_io_region(void)
{
	int status = metal_init_once();

	if (status) {
		return NULL;
	}

	return &resource_table_region;
}

struct metal_io_region *remoteproc_get_carveout_io_region(const struct fw_rsc_carveout *carveout)
{
	struct metal_io_region *region = NULL;

	int status = metal_init_once();

	if (status)
		return NULL;

#if NUM_CARVEOUTS > 0
	for (int i = 0; i < NUM_CARVEOUTS; i++) {
		if (carveout_lut[i].carveout == carveout) {
			region = &carveout_lut[i].region;
			break;
		}
	}
#endif

	return region;
}
