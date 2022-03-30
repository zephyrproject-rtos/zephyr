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

#include <zephyr.h>
#include <resource_table.h>

extern char ram_console[];

#define __resource Z_GENERIC_SECTION(.resource_table)

static struct fw_resource_table __resource resource_table = {
	.ver = 1,
	.num = RSC_TABLE_NUM_ENTRY,
	.offset = {

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
		offsetof(struct fw_resource_table, vdev),
#endif

#if defined(CONFIG_RAM_CONSOLE)
		offsetof(struct fw_resource_table, cm_trace),
#endif
	},

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
	/* Virtio device entry */
	.vdev = {
		RSC_VDEV, VIRTIO_ID_RPMSG, 0, RPMSG_IPU_C0_FEATURES, 0, 0, 0,
		VRING_COUNT, {0, 0},
	},

	/* Vring rsc entry - part of vdev rsc entry */
	.vring0 = {VRING_TX_ADDRESS, VRING_ALIGNMENT,
		   CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF,
		   VRING0_ID, 0},
	.vring1 = {VRING_RX_ADDRESS, VRING_ALIGNMENT,
		   CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF,
		   VRING1_ID, 0},
#endif

#if defined(CONFIG_RAM_CONSOLE)
	.cm_trace = {
		RSC_TRACE,
		(uint32_t)ram_console, CONFIG_RAM_CONSOLE_BUFFER_SIZE + 1, 0,
		"Zephyr_log",
	},
#endif
};

#if DT_HAS_CHOSEN(zephyr_rsc_table)

#define RSC_TABLE_RELOC			DT_CHOSEN(zephyr_rsc_table)
#define RSC_TABLE_RELOC_START		DT_REG_ADDR(RSC_TABLE_RELOC)
#define RSC_TABLE_RELOC_SIZE		DT_REG_SIZE(RSC_TABLE_RELOC)

BUILD_ASSERT(sizeof(resource_table) <= RSC_TABLE_RELOC_SIZE,
	     "Resource table does not fit in the relocation area");

#endif

void rsc_table_get(void **table_ptr, int *length)
{
#if defined(RSC_TABLE_RELOC)
	const size_t cmp_size = offsetof(struct fw_resource_table, offset) + sizeof(((struct fw_resource_table *)0)->offset);

	*table_ptr = (void *)RSC_TABLE_RELOC_START;
	*length = sizeof(resource_table);

	/*
	 * Do not copy the resource table if it has already been installed from the remote host.
	 * Because the remote host may have already initialized some fields, only the guaranteed constant bytes can be compared.
	 */
	if(memcmp(&resource_table, *table_ptr, cmp_size))
		memcpy(*table_ptr, (void *)&resource_table, *length);
#else
	*table_ptr = (void *)&resource_table;
	*length = sizeof(resource_table);
#endif
}
