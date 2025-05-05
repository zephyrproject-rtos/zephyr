/*
 * Copyright (c) 2020 STMicroelectronics
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
#include <resource_table.h>

#ifdef CONFIG_OPENAMP_COPY_RSC_TABLE
#define RSC_TABLE_ADDR	DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_rsc_table))
#define RSC_TABLE_SIZE	DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_rsc_table))
BUILD_ASSERT(sizeof(struct fw_resource_table) <= RSC_TABLE_SIZE);
#endif

extern char ram_console_buf[];

#define __resource Z_GENERIC_SECTION(.resource_table)

static struct fw_resource_table __resource resource_table = RESOURCE_TABLE_INIT;

void rsc_table_get(struct fw_resource_table **table_ptr, int *length)
{
	*length = sizeof(resource_table);
#ifdef CONFIG_OPENAMP_COPY_RSC_TABLE
	*table_ptr = (struct fw_resource_table *)RSC_TABLE_ADDR;
	memcpy(*table_ptr, &resource_table, *length);
#else
	*table_ptr = &resource_table;
#endif
}
