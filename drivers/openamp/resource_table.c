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

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/linker/devicetree_regions.h>

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>

LOG_MODULE_REGISTER(openamp_driver, CONFIG_OPENAMP_DRIVER_LOG_LEVEL);

extern char ram_console_buf[];

#define VRING_ALIGNMENT       16 /* fixed to match with Linux constraint */
#define RPMSG_IPU_C0_FEATURES 1

#define RESOURCE_TABLE_ATTR                                                                        \
__attribute__((__section__(DT_PROP(VDEV_NODE, resource_table)))))
#define __resource Z_GENERIC_SECTION(.resource_table)

#define RESOURCE_TABLE_NODE DT_CHOSEN(openamp_resource_table)

#define NUM_CARVEOUTS DT_PROP_LEN(RESOURCE_TABLE_NODE, carveouts)
#define NUM_VDEVS     DT_PROP_LEN(RESOURCE_TABLE_NODE, vdevs)

#define VDEV_INDEX(idx) vdev_##idx

#define CARVEOUT_OFFSET(node_id, prop, idx) offsetof(struct fw_resource_table, carveouts[idx]),

#define CARVEOUT_ENTRY(node_id, prop, idx)                                                         \
	(struct fw_rsc_carveout){                                                                  \
		.type = RSC_CARVEOUT,                                                              \
		.da = DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                          \
		.pa = DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                          \
		.len = DT_REG_SIZE(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                         \
		.flags = 0,                                                                        \
		.name = DT_PROP_BY_PHANDLE_IDX(node_id, carveouts, idx, zephyr_memory_region)},

#define VRING_ENTRY(node_id, prop, idx, ...)                                                       \
	.vrings[idx] = (struct fw_rsc_vdev_vring){                                                 \
		.da = DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, device_address),                  \
		.align = VRING_ALIGNMENT,                                                          \
		.num = DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, num_buffers),                    \
		.notifyid = DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, notify_id),                 \
	},

#define VDEV_ENTRY(node_id, prop, idx)                                                             \
	.VDEV_INDEX(idx) = {                                                                       \
		.vdev =                                                                            \
			(struct fw_rsc_vdev){                                                      \
				.type = RSC_VDEV,                                                  \
				.id = DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, device_id),       \
				.notifyid = 0,                                                     \
				.dfeatures = RPMSG_IPU_C0_FEATURES,                                \
				.gfeatures = 0,                                                    \
				.config_len = 0,                                                   \
				.status = 0,                                                       \
				.num_of_vrings = DT_PROP_LEN(                                      \
					DT_PHANDLE_BY_IDX(node_id, prop, idx), vrings),            \
			},                                                                         \
		DT_FOREACH_PROP_ELEM_VARGS(DT_PHANDLE_BY_IDX(node_id, prop, idx), vrings,          \
					   VRING_ENTRY)},

#define VDEV_FIELD(node_id, prop, idx)                                                             \
	struct {                                                                                   \
		struct fw_rsc_vdev vdev;                                                           \
		struct fw_rsc_vdev_vring                                                           \
			vrings[DT_PROP_LEN(DT_PHANDLE_BY_IDX(node_id, prop, idx), vrings)];        \
	} vdev_##idx;

#define FOREACH_CARVEOUT(fn)
#define CARVEOUT_ENTRIES FOREACH_CARVEOUT(CARVEOUT_ENTRY)

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

#define VRING_FIELD(node_id, prop, idx) vdev_##idx;

#define VDEV_OFFSET(node_id, prop, idx)     offsetof(struct fw_resource_table, vdev_##idx),
#define CARVEOUT_OFFSET(node_id, prop, idx) offsetof(struct fw_resource_table, carveouts[idx]),

struct fw_resource_table {
	struct resource_table hdr;
	uint32_t offset[RSC_TABLE_NUM_ENTRIES];
#if NUM_CARVEOUTS > 0
	struct fw_rsc_carveout carveouts[NUM_CARVEOUTS];
#endif
	DT_FOREACH_PROP_ELEM(RESOURCE_TABLE_NODE, vdevs, VDEV_FIELD)
#if defined(CONFIG_RAM_CONSOLE)
	struct fw_rsc_trace cm_trace;
#endif
} METAL_PACKED_END __aligned(8) resource_table __resource = {
	.hdr = {
			.ver = 1,
			.num = RSC_TABLE_NUM_ENTRIES,
		},

	.offset = {
			DT_FOREACH_PROP_ELEM(RESOURCE_TABLE_NODE, carveouts, CARVEOUT_OFFSET)
				DT_FOREACH_PROP_ELEM(RESOURCE_TABLE_NODE, vdevs, VDEV_OFFSET)

#if defined(CONFIG_RAM_CONSOLE)
					offsetof(struct fw_resource_table, cm_trace),
#endif
		},

#if NUM_CARVEOUTS > 0
	.carveouts = {DT_FOREACH_PROP_ELEM(RESOURCE_TABLE_NODE, carveouts, CARVEOUT_ENTRY)},
#endif
	DT_FOREACH_PROP_ELEM(RESOURCE_TABLE_NODE, vdevs, VDEV_ENTRY)

#if defined(CONFIG_RAM_CONSOLE)
		.cm_trace = {
			.type = RSC_TRACE,
			.da = (uint32_t)ram_console_buf,
			.len = CONFIG_RAM_CONSOLE_BUFFER_SIZE,
			.name = "Zephyr_log",
		},
#endif
};

#if NUM_VDEVS > 0
#define VDEV_LUT_ENTRY(node_id, prop, idx)                                                         \
	{.index = DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                              \
	 &resource_table.VDEV_INDEX(idx).vdev},

static struct {
	unsigned int index;
	struct fw_rsc_vdev *vdev;
} vdev_lut[] = {DT_FOREACH_PROP_ELEM(RESOURCE_TABLE_NODE, vdevs, VDEV_LUT_ENTRY)};
#endif

void *openamp_get_rsc_table(void)
{
	return &resource_table;
}

size_t openamp_get_rsc_table_size(void)
{
	return sizeof(resource_table);
}

struct fw_rsc_carveout *openamp_get_carveout_by_name(const char *name)
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

struct fw_rsc_carveout *openamp_get_carveout_by_index(unsigned int idx)
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

struct fw_rsc_vdev *openamp_get_vdev(unsigned int idx)
{
#if NUM_VDEVS > 0
	for (int i = 0; i < NUM_VDEVS; i++) {
		if (vdev_lut[i].index == idx) {
			return vdev_lut[i].vdev;
		}
	}
#endif
	return NULL;
}
