#include "rsc_table.hpp"
#include <openamp/virtio.h>
#include <stddef.h>
#include <device.h>

#define RSC_VDEV_FEATURE_NS (1) /* Support name service announcement */
#define RSC_VDEV_NUM_VRINGS (2)

#define VDEV0_VRING_BASE (DT_REG_ADDR(DT_NODELABEL(vdev0vring0)))
#define VDEV1_VRING_BASE (DT_REG_ADDR(DT_NODELABEL(vdev0vring1)))

#define VRING_ALIGN (0x1000U)
#define RL_BUFFER_COUNT (256U)
#define RESOURCE_TABLE_OFFSET (0xFF000)

Z_GENERIC_SECTION(.resource_table)
remote_resource_table resources = {
    .version = 1,
    .num = RSC_TABLE_NUM_ENTRY,
    .offset = { offsetof(remote_resource_table, user_vdev), },

    /* SRTM virtio device entry */
    .user_vdev = {
        .type = RSC_VDEV,
        .id = VIRTIO_ID_RPMSG,
        .notifyid = 0,
        .dfeatures = RSC_VDEV_FEATURE_NS,
        .gfeatures = 0,
        .config_len = 0,
        .status = 0,
        .num_of_vrings = RSC_VDEV_NUM_VRINGS,
    },

    /* Vring rsc entry - part of vdev rsc entry */
    .user_vring0 = {
        .da = VDEV0_VRING_BASE,
        .align = VRING_ALIGN,
        .num = RL_BUFFER_COUNT,
        .notifyid = 0,
    },

    .user_vring1 = {
        .da = VDEV1_VRING_BASE,
        .align = VRING_ALIGN,
        .num = RL_BUFFER_COUNT,
        .notifyid = 1
    },
};

void copyResourceTable(void)
{
    /*
     * Resource table should be copied to VDEV0_VRING_BASE + RESOURCE_TABLE_OFFSET.
     * VDEV0_VRING_BASE is temperorily kept for backward compatibility, will be
     * removed in future release
     */
    memcpy((void *)VDEV0_VRING_BASE, &resources, sizeof(resources));
    memcpy((void *)(VDEV0_VRING_BASE + RESOURCE_TABLE_OFFSET), &resources, sizeof(resources));
}

remote_resource_table* getResourceTable(void) {
    return &resources;
}
