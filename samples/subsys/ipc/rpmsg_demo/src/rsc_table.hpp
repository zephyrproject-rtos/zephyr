#pragma once

#include <stdint.h>
#include <openamp/remoteproc.h>

#if defined __cplusplus
extern "C" {
#endif

// RPMSG MU channel index
// As Linux suggests, use MU->Data Channel 1 as communication channel
// see RPMSG-Lite rpmsg_platform.c -> platform_notify
#define RPMSG_MU_CHANNEL (1)

enum resource_table_entries {
	RSC_TABLE_VDEV_ENTRY,
	RSC_TABLE_NUM_ENTRY
};

/* Resource table for the given remote */
METAL_PACKED_BEGIN
struct remote_resource_table
{
    uint32_t version;
    uint32_t num;
    uint32_t reserved[2];
    uint32_t offset[RSC_TABLE_NUM_ENTRY];

    /* rpmsg vdev entry for user app communication */
    fw_rsc_vdev user_vdev;
    fw_rsc_vdev_vring user_vring0;
    fw_rsc_vdev_vring user_vring1;
} METAL_PACKED_END;

/*
 * Copy resource table to shared memory base for early M4 boot case.
 * In M4 early boot case, Linux kernel need to get resource table before file system gets loaded.
 */
void copyResourceTable(void);
remote_resource_table* getResourceTable(void);

#if defined __cplusplus
}
#endif
