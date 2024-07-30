/*
 * Copyright Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/coredump.h>
#include <zephyr/drivers/coredump.h>

#define DT_DRV_COMPAT zephyr_coredump

enum COREDUMP_TYPE {
	COREDUMP_TYPE_MEMCPY = 0,
	COREDUMP_TYPE_CALLBACK = 1,
};

struct coredump_config {
	/* Type of coredump device */
	enum COREDUMP_TYPE type;

	/* Length of memory_regions array */
	int length;

	/* Memory regions specified in device tree */
	size_t memory_regions[];
};

struct coredump_data {
	/* Memory regions registered at run time */
	sys_slist_t region_list;

	/* Callback to be invoked at time of dump */
	coredump_dump_callback_t dump_callback;
};

static void coredump_impl_dump(const struct device *dev)
{
	const struct coredump_config *config = dev->config;
	struct coredump_data *data = dev->data;

	if (config->type == COREDUMP_TYPE_CALLBACK) {
		if (data->dump_callback) {
			uintptr_t start_address = config->memory_regions[0];
			size_t size = config->memory_regions[1];

			/* Invoke callback to allow consumer to fill array with desired data */
			data->dump_callback(start_address, size);
			coredump_memory_dump(start_address, start_address + size);
		}
	} else { /* COREDUMP_TYPE_MEMCPY */
		/*
		 * Add each memory region specified in device tree to the core dump,
		 * the memory_regions array should contain two entries per region
		 * containing the start address and size.
		 */
		if ((config->length > 0) && ((config->length % 2) == 0)) {
			for (int i = 0; i < config->length; i += 2) {
				uintptr_t start_address = config->memory_regions[i];
				size_t size = config->memory_regions[i+1];

				coredump_memory_dump(start_address, start_address + size);
			}
		}

		sys_snode_t *node;

		/* Add each memory region registered at runtime to the core dump */
		SYS_SLIST_FOR_EACH_NODE(&data->region_list, node) {
			struct coredump_mem_region_node *region;

			region = CONTAINER_OF(node, struct coredump_mem_region_node, node);
			coredump_memory_dump(region->start, region->start + region->size);
		}
	}
}

static bool coredump_impl_register_memory(const struct device *dev,
	struct coredump_mem_region_node *region)
{
	const struct coredump_config *config = dev->config;

	if (config->type == COREDUMP_TYPE_CALLBACK) {
		return false;
	}

	struct coredump_data *data = dev->data;

	sys_slist_append(&data->region_list, &region->node);
	return true;
}

static bool coredump_impl_unregister_memory(const struct device *dev,
	struct coredump_mem_region_node *region)
{
	const struct coredump_config *config = dev->config;

	if (config->type == COREDUMP_TYPE_CALLBACK) {
		return false;
	}

	struct coredump_data *data = dev->data;

	return sys_slist_find_and_remove(&data->region_list, &region->node);
}

static bool coredump_impl_register_callback(const struct device *dev,
	coredump_dump_callback_t callback)
{
	const struct coredump_config *config = dev->config;

	if (config->type == COREDUMP_TYPE_MEMCPY) {
		return false;
	}

	struct coredump_data *data = dev->data;

	data->dump_callback = callback;
	return true;
}

static int coredump_init(const struct device *dev)
{
	struct coredump_data *data = dev->data;

	sys_slist_init(&data->region_list);
	return 0;
}

static const struct coredump_driver_api coredump_api = {
	.dump = coredump_impl_dump,
	.register_memory = coredump_impl_register_memory,
	.unregister_memory = coredump_impl_unregister_memory,
	.register_callback = coredump_impl_register_callback,
};

#define INIT_REGION(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),
#define DT_INST_COREDUMP_IF_TYPE_CALLBACK(n, a, b) \
	COND_CODE_1(DT_INST_ENUM_IDX(n, coredump_type), a, b)

#define CREATE_COREDUMP_DEVICE(n)                                                \
	/* Statially allocate desired memory for the callback type device */         \
	DT_INST_COREDUMP_IF_TYPE_CALLBACK(n,                                         \
	(                                                                            \
		BUILD_ASSERT(DT_INST_PROP_LEN(n, memory_regions) == 2,                   \
			"Allow exactly one entry (address and size) in memory_regions");     \
		BUILD_ASSERT(DT_INST_PROP_BY_IDX(n, memory_regions, 0) == 0,             \
			"Verify address is set to 0");                                       \
		static uint8_t coredump_bytes[DT_INST_PROP_BY_IDX(n, memory_regions, 1)] \
			__aligned(4);                                                        \
	), ())                                                                       \
	static struct coredump_data coredump_data_##n;                               \
	static const struct coredump_config coredump_config##n = {                   \
		.type = DT_INST_STRING_TOKEN_OR(n, coredump_type, COREDUMP_TYPE_MEMCPY), \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, memory_regions),                    \
		(                                                                        \
			.length = DT_INST_PROP_LEN(n, memory_regions),                       \
			DT_INST_COREDUMP_IF_TYPE_CALLBACK(n,                                 \
			(                                                                    \
			/* Callback type device has one entry in memory_regions array */     \
			.memory_regions = {                                                  \
				(size_t)&coredump_bytes[0],                                      \
				DT_INST_PROP_BY_IDX(n, memory_regions, 1),                       \
			},                                                                   \
			),                                                                   \
			(                                                                    \
			.memory_regions = {                                                  \
				DT_INST_FOREACH_PROP_ELEM(n, memory_regions, INIT_REGION)        \
			},                                                                   \
			))                                                                   \
		),                                                                       \
		(                                                                        \
			.length = 0,                                                         \
		))                                                                       \
	};                                                                           \
	DEVICE_DT_INST_DEFINE(n,                                    \
						  coredump_init,                        \
						  NULL,                                 \
						  &coredump_data_##n,                   \
						  &coredump_config##n,                  \
						  PRE_KERNEL_1,                         \
						  CONFIG_COREDUMP_DEVICE_INIT_PRIORITY, \
						  &coredump_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_COREDUMP_DEVICE)
