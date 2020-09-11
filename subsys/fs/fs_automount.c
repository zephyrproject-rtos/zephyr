/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <errno.h>
#include <init.h>
#include <fs/fs.h>
#include <sys/stat.h>
#include <fs_impl.h>

#define DT_DRV_COMPAT fixed_partitions
#include <devicetree.h>

#define LOG_LEVEL CONFIG_FS_LOG_LEVEL
#include <logging/log.h>

LOG_MODULE_REGISTER(fs_automount);

/* Macro generates fs_mount_t structure initialization block for given partition
 */
#define AUTOMOUNT_FROM_DTS(part)					\
	{								\
		.type_sz = DT_PROP_OR(part, mount_type, NULL),		\
		.mnt_point = DT_PROP_OR(part, mount_point, NULL),	\
		.storage_dev = (void *)DT_FIXED_PARTITION_ID(part),	\
	},


/* Filter used to only initialize structures that have both file system type
 * and mount point specified in dts
 */
#define AUTOMOUNT_FROM_DTS_FILTERED(part)				\
	COND_CODE_1(DT_NODE_HAS_PROP(part, mount_point),		\
		    (COND_CODE_1(DT_NODE_HAS_PROP(part, mount_type),	\
				 (AUTOMOUNT_FROM_DTS(part)),		\
				 ())),					\
		    ())


/* Macro iterating through all partition definitions of DT_DRV_COMPAT partition
 * table.
 */
#define FOREACH_DTS_PARTITION(n) \
	DT_FOREACH_CHILD(DT_DRV_INST(n), AUTOMOUNT_FROM_DTS_FILTERED)

struct fs_mount_t dts_part_table[] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_DTS_PARTITION)
};

/**
 * @brief Attempts to allocate files system structures and auto-mount the
 * systems.
 *
 * @param dev	unused
 *
 * @return 0
 */
static int fs_automount(const struct device *dev)
{
	ARG_UNUSED(dev);

	for (int i = 0; i < ARRAY_SIZE(dts_part_table); ++i) {
		struct fs_mount_t *m = &dts_part_table[i];
		const char *type_sz = m->type_sz;

		/* Translate file system type from string to int; strings
		 * are for internal usage only until initialized to in.
		 */
		m->type = fs_get_compatible(m->type_sz);

		if (m->type == FS_UNKNOWN) {
			continue;
		}

		const struct fs_file_system_t *api = fs_get_api(m->type);

		/* Due to data structures are file system characteristic, the
		 * file system will be asked to allocate and initialize them.
		 **/
		if (api != NULL && api->get_fs_data_storage != NULL) {
			m->fs_data = api->get_fs_data_storage();
		} else {
			m->fs_data = NULL;
		}

		int err = errno;

		if (err == ENOSPC) {
			LOG_ERR("No more fs storage available for %s",
				m->mnt_point);
			continue;
		} else if (err != 0) {
			LOG_ERR("Fs data storage for %s failed (errno = %d)",
				m->mnt_point, err);
		}

		LOG_INF("Attempting to auto-mount %s at %s", type_sz,
			 m->mnt_point);

		int ret = fs_mount(m);

		if (ret != 0) {
			LOG_ERR("Failed to auto-mount %s at %s with error %d",
				type_sz, m->mnt_point, ret);
		} else {
			LOG_INF("Auto-mounted %s successfully", m->mnt_point);
		}
	}

	return 0;
}

SYS_INIT(fs_automount, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
