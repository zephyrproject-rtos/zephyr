/*
 * Remote remote_proc Framework
 *
 * Copyright(c) 2011 Texas Instruments, Inc.
 * Copyright(c) 2011 Google, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REMOTEPROC_H
#define REMOTEPROC_H

#include <openamp/rpmsg.h>
#include <openamp/firmware.h>

#if defined __cplusplus
extern "C" {
#endif

/**
 * struct resource_table - firmware resource table header
 * @ver: version number
 * @num: number of resource entries
 * @reserved: reserved (must be zero)
 * @offset: array of offsets pointing at the various resource entries
 *
 * A resource table is essentially a list of system resources required
 * by the remote remote_proc. It may also include configuration entries.
 * If needed, the remote remote_proc firmware should contain this table
 * as a dedicated ".resource_table" ELF section.
 *
 * Some resources entries are mere announcements, where the host is informed
 * of specific remoteproc configuration. Other entries require the host to
 * do something (e.g. allocate a system resource). Sometimes a negotiation
 * is expected, where the firmware requests a resource, and once allocated,
 * the host should provide back its details (e.g. address of an allocated
 * memory region).
 *
 * The header of the resource table, as expressed by this structure,
 * contains a version number (should we need to change this format in the
 * future), the number of available resource entries, and their offsets
 * in the table.
 *
 * Immediately following this header are the resource entries themselves,
 * each of which begins with a resource entry header (as described below).
 */
OPENAMP_PACKED_BEGIN
struct resource_table {
	uint32_t ver;
	uint32_t num;
	uint32_t reserved[2];
	uint32_t offset[0];
} OPENAMP_PACKED_END;

/**
 * struct fw_rsc_hdr - firmware resource entry header
 * @type: resource type
 * @data: resource data
 *
 * Every resource entry begins with a 'struct fw_rsc_hdr' header providing
 * its @type. The content of the entry itself will immediately follow
 * this header, and it should be parsed according to the resource type.
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_hdr {
	uint32_t type;
	uint8_t data[0];
} OPENAMP_PACKED_END;

/**
 * enum fw_resource_type - types of resource entries
 *
 * @RSC_CARVEOUT:   request for allocation of a physically contiguous
 *          memory region.
 * @RSC_DEVMEM:     request to iommu_map a memory-based peripheral.
 * @RSC_TRACE:      announces the availability of a trace buffer into which
 *          the remote remote_proc will be writing logs.
 * @RSC_VDEV:       declare support for a virtio device, and serve as its
 *          virtio header.
 * @RSC_LAST:       just keep this one at the end
 *
 * For more details regarding a specific resource type, please see its
 * dedicated structure below.
 *
 * Please note that these values are used as indices to the rproc_handle_rsc
 * lookup table, so please keep them sane. Moreover, @RSC_LAST is used to
 * check the validity of an index before the lookup table is accessed, so
 * please update it as needed.
 */
enum fw_resource_type {
	RSC_CARVEOUT = 0,
	RSC_DEVMEM = 1,
	RSC_TRACE = 2,
	RSC_VDEV = 3,
	RSC_RPROC_MEM = 4,
	RSC_FW_CHKSUM = 5,
	RSC_LAST = 6,
};

#define FW_RSC_ADDR_ANY (0xFFFFFFFFFFFFFFFF)

/**
 * struct fw_rsc_carveout - physically contiguous memory request
 * @da: device address
 * @pa: physical address
 * @len: length (in bytes)
 * @flags: iommu protection flags
 * @reserved: reserved (must be zero)
 * @name: human-readable name of the requested memory region
 *
 * This resource entry requests the host to allocate a physically contiguous
 * memory region.
 *
 * These request entries should precede other firmware resource entries,
 * as other entries might request placing other data objects inside
 * these memory regions (e.g. data/code segments, trace resource entries, ...).
 *
 * Allocating memory this way helps utilizing the reserved physical memory
 * (e.g. CMA) more efficiently, and also minimizes the number of TLB entries
 * needed to map it (in case @rproc is using an IOMMU). Reducing the TLB
 * pressure is important; it may have a substantial impact on performance.
 *
 * If the firmware is compiled with static addresses, then @da should specify
 * the expected device address of this memory region. If @da is set to
 * FW_RSC_ADDR_ANY, then the host will dynamically allocate it, and then
 * overwrite @da with the dynamically allocated address.
 *
 * We will always use @da to negotiate the device addresses, even if it
 * isn't using an iommu. In that case, though, it will obviously contain
 * physical addresses.
 *
 * Some remote remote_procs needs to know the allocated physical address
 * even if they do use an iommu. This is needed, e.g., if they control
 * hardware accelerators which access the physical memory directly (this
 * is the case with OMAP4 for instance). In that case, the host will
 * overwrite @pa with the dynamically allocated physical address.
 * Generally we don't want to expose physical addresses if we don't have to
 * (remote remote_procs are generally _not_ trusted), so we might want to
 * change this to happen _only_ when explicitly required by the hardware.
 *
 * @flags is used to provide IOMMU protection flags, and @name should
 * (optionally) contain a human readable name of this carveout region
 * (mainly for debugging purposes).
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_carveout {
	uint32_t type;
	uint32_t da;
	uint32_t pa;
	uint32_t len;
	uint32_t flags;
	uint32_t reserved;
	uint8_t name[32];
} OPENAMP_PACKED_END;

/**
 * struct fw_rsc_devmem - iommu mapping request
 * @da: device address
 * @pa: physical address
 * @len: length (in bytes)
 * @flags: iommu protection flags
 * @reserved: reserved (must be zero)
 * @name: human-readable name of the requested region to be mapped
 *
 * This resource entry requests the host to iommu map a physically contiguous
 * memory region. This is needed in case the remote remote_proc requires
 * access to certain memory-based peripherals; _never_ use it to access
 * regular memory.
 *
 * This is obviously only needed if the remote remote_proc is accessing memory
 * via an iommu.
 *
 * @da should specify the required device address, @pa should specify
 * the physical address we want to map, @len should specify the size of
 * the mapping and @flags is the IOMMU protection flags. As always, @name may
 * (optionally) contain a human readable name of this mapping (mainly for
 * debugging purposes).
 *
 * Note: at this point we just "trust" those devmem entries to contain valid
 * physical addresses, but this isn't safe and will be changed: eventually we
 * want remoteproc implementations to provide us ranges of physical addresses
 * the firmware is allowed to request, and not allow firmwares to request
 * access to physical addresses that are outside those ranges.
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_devmem {
	uint32_t type;
	uint32_t da;
	uint32_t pa;
	uint32_t len;
	uint32_t flags;
	uint32_t reserved;
	uint8_t name[32];
} OPENAMP_PACKED_END;

/**
 * struct fw_rsc_trace - trace buffer declaration
 * @da: device address
 * @len: length (in bytes)
 * @reserved: reserved (must be zero)
 * @name: human-readable name of the trace buffer
 *
 * This resource entry provides the host information about a trace buffer
 * into which the remote remote_proc will write log messages.
 *
 * @da specifies the device address of the buffer, @len specifies
 * its size, and @name may contain a human readable name of the trace buffer.
 *
 * After booting the remote remote_proc, the trace buffers are exposed to the
 * user via debugfs entries (called trace0, trace1, etc..).
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_trace {
	uint32_t type;
	uint32_t da;
	uint32_t len;
	uint32_t reserved;
	uint8_t name[32];
} OPENAMP_PACKED_END;

/**
 * struct fw_rsc_vdev_vring - vring descriptor entry
 * @da: device address
 * @align: the alignment between the consumer and producer parts of the vring
 * @num: num of buffers supported by this vring (must be power of two)
 * @notifyid is a unique rproc-wide notify index for this vring. This notify
 * index is used when kicking a remote remote_proc, to let it know that this
 * vring is triggered.
 * @reserved: reserved (must be zero)
 *
 * This descriptor is not a resource entry by itself; it is part of the
 * vdev resource type (see below).
 *
 * Note that @da should either contain the device address where
 * the remote remote_proc is expecting the vring, or indicate that
 * dynamically allocation of the vring's device address is supported.
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_vdev_vring {
	uint32_t da;
	uint32_t align;
	uint32_t num;
	uint32_t notifyid;
	uint32_t reserved;
} OPENAMP_PACKED_END;

/**
 * struct fw_rsc_vdev - virtio device header
 * @id: virtio device id (as in virtio_ids.h)
 * @notifyid is a unique rproc-wide notify index for this vdev. This notify
 * index is used when kicking a remote remote_proc, to let it know that the
 * status/features of this vdev have changes.
 * @dfeatures specifies the virtio device features supported by the firmware
 * @gfeatures is a place holder used by the host to write back the
 * negotiated features that are supported by both sides.
 * @config_len is the size of the virtio config space of this vdev. The config
 * space lies in the resource table immediate after this vdev header.
 * @status is a place holder where the host will indicate its virtio progress.
 * @num_of_vrings indicates how many vrings are described in this vdev header
 * @reserved: reserved (must be zero)
 * @vring is an array of @num_of_vrings entries of 'struct fw_rsc_vdev_vring'.
 *
 * This resource is a virtio device header: it provides information about
 * the vdev, and is then used by the host and its peer remote remote_procs
 * to negotiate and share certain virtio properties.
 *
 * By providing this resource entry, the firmware essentially asks remoteproc
 * to statically allocate a vdev upon registration of the rproc (dynamic vdev
 * allocation is not yet supported).
 *
 * Note: unlike virtualization systems, the term 'host' here means
 * the Linux side which is running remoteproc to control the remote
 * remote_procs. We use the name 'gfeatures' to comply with virtio's terms,
 * though there isn't really any virtualized guest OS here: it's the host
 * which is responsible for negotiating the final features.
 * Yeah, it's a bit confusing.
 *
 * Note: immediately following this structure is the virtio config space for
 * this vdev (which is specific to the vdev; for more info, read the virtio
 * spec). the size of the config space is specified by @config_len.
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_vdev {
	uint32_t type;
	uint32_t id;
	uint32_t notifyid;
	uint32_t dfeatures;
	uint32_t gfeatures;
	uint32_t config_len;
	uint8_t status;
	uint8_t num_of_vrings;
	uint8_t reserved[2];
	struct fw_rsc_vdev_vring vring[0];
} OPENAMP_PACKED_END;

/**
 * struct fw_rsc_rproc_mem - remote processor memory
 * @da: device address
 * @pa: physical address
 * @len: length (in bytes)
 * @reserved: reserved (must be zero)
 *
 * This resource entry tells the host to the remote processor
 * memory that the host can be used as shared memory.
 *
 * These request entries should precede other shared resource entries
 * such as vdevs, vrings.
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_rproc_mem {
	uint32_t type;
	uint32_t da;
	uint32_t pa;
	uint32_t len;
	uint32_t reserved;
} OPENAMP_PACKED_END;

/*
 * struct fw_rsc_fw_chksum - firmware checksum
 * @algo: algorithm to generate the cheksum
 * @chksum: checksum of the firmware loadable sections.
 *
 * This resource entry provides checksum for the firmware loadable sections.
 * It is used to check if the remote already runs with the expected firmware to
 * decide if it needs to start the remote if the remote is already running.
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_fw_chksum {
	uint32_t type;
	uint8_t algo[16];
	uint8_t chksum[64];
} OPENAMP_PACKED_END;

/**
 * struct remote_proc
 *
 * This structure is maintained by the remoteproc to represent the remote
 * processor instance. This structure acts as a prime parameter to use
 * the remoteproc APIs.
 *
 * @proc                : hardware interface layer processor control
 * @rdev                : remote device , used by RPMSG "messaging" framework.
 * @loader              : pointer remoteproc loader
 * @channel_created     : create channel callback
 * @channel_destroyed   : delete channel callback
 * @default_cb          : default callback for channel
 * @role                : remote proc role , RPROC_MASTER/RPROC_REMOTE
 *
 */
struct remote_proc {
	struct hil_proc *proc;
	struct remote_device *rdev;
	struct remoteproc_loader *loader;
	rpmsg_chnl_cb_t channel_created;
	rpmsg_chnl_cb_t channel_destroyed;
	rpmsg_rx_cb_t default_cb;
	int role;
};

/**
 * struct resc_table_info
 *
 * This structure is maintained by the remoteproc to allow applications
 * to pass resource table info during remote initialization.
 *
 * @rsc_tab : pointer to resource table control block
 * @size    : size of resource table.
 *
 */
struct rsc_table_info {
	struct resource_table *rsc_tab;
	int size;
};

/* Definitions for device types , null pointer, etc.*/
#define RPROC_SUCCESS                           0
#define RPROC_NULL                              (void *)0
#define RPROC_TRUE                              1
#define RPROC_FALSE                             0
#define RPROC_MASTER                            1
#define RPROC_REMOTE                            0
/* Number of msecs to wait for remote context to come up */
#define RPROC_BOOT_DELAY                        500

/* Remoteproc error codes */
#define RPROC_ERR_BASE                          -4000
#define RPROC_ERR_CPU_INIT                      (RPROC_ERR_BASE -1)
#define RPROC_ERR_NO_RSC_TABLE                  (RPROC_ERR_BASE -2)
#define RPROC_ERR_NO_MEM                        (RPROC_ERR_BASE -3)
#define RPROC_ERR_RSC_TAB_TRUNC                 (RPROC_ERR_BASE -4)
#define RPROC_ERR_RSC_TAB_VER                   (RPROC_ERR_BASE -5)
#define RPROC_ERR_RSC_TAB_RSVD                  (RPROC_ERR_BASE -6)
#define RPROC_ERR_RSC_TAB_VDEV_NRINGS           (RPROC_ERR_BASE -7)
#define RPROC_ERR_RSC_TAB_NP                    (RPROC_ERR_BASE -8)
#define RPROC_ERR_RSC_TAB_NS                    (RPROC_ERR_BASE -9)
#define RPROC_ERR_INVLD_FW                      (RPROC_ERR_BASE -10)
#define RPROC_ERR_LOADER                        (RPROC_ERR_BASE -11)
#define RPROC_ERR_PARAM                         (RPROC_ERR_BASE -12)
#define RPROC_ERR_PTR                           (void*)0xDEADBEAF

/**
 * remoteproc_resource_init
 *
 * Initializes resources for remoteproc remote configuration.Only
 * remoteproc remote applications are allowed to call this function.
 *
 * @param rsc_info          - pointer to resource table info control
 *                            block
 * @param proc              - pointer to the hil_proc
 * @param channel_created   - callback function for channel creation
 * @param channel_destroyed - callback function for channel deletion
 * @param default_cb        - default callback for channel I/O
 * @param rproc_handle      - pointer to new remoteproc instance
 * @param init_env          - 1 to initialize environment, 0 not to
 * @param rpmsg_role        - 1 for rpmsg master,  or 0 for rpmsg slave
 *
 * @param returns - status of execution
 *
 */
int remoteproc_resource_init(struct rsc_table_info *rsc_info,
			    struct hil_proc *proc,
			     rpmsg_chnl_cb_t channel_created,
			     rpmsg_chnl_cb_t channel_destroyed,
			     rpmsg_rx_cb_t default_cb,
			     struct remote_proc **rproc_handle,
			     int rpmsg_role);

/**
 * remoteproc_resource_deinit
 *
 * Uninitializes resources for remoteproc remote configuration.
 *
 * @param rproc - pointer to remoteproc instance
 *
 * @param returns - status of execution
 *
 */

int remoteproc_resource_deinit(struct remote_proc *rproc);

/**
 * remoteproc_init
 *
 * Initializes resources for remoteproc master configuration. Only
 * remoteproc master applications are allowed to call this function.
 *
 * @param fw_name           - name of firmware
 * @param proc              - pointer to hil_proc
 * @param channel_created   - callback function for channel creation
 * @param channel_destroyed - callback function for channel deletion
 * @param default_cb        - default callback for channel I/O
 * @param rproc_handle      - pointer to new remoteproc instance
 *
 * @param returns - status of function execution
 *
 */
int remoteproc_init(char *fw_name, struct hil_proc *proc,
		    rpmsg_chnl_cb_t channel_created,
		    rpmsg_chnl_cb_t channel_destroyed,
		    rpmsg_rx_cb_t default_cb,
		    struct remote_proc **rproc_handle);

/**
 * remoteproc_deinit
 *
 * Uninitializes resources for remoteproc "master" configuration.
 *
 * @param rproc - pointer to remoteproc instance
 *
 * @param returns - status of function execution
 *
 */
int remoteproc_deinit(struct remote_proc *rproc);

/**
 * remoteproc_boot
 *
 * This function loads the image on the remote processor and starts
 * its execution from image load address.
 *
 * @param rproc - pointer to remoteproc instance to boot
 *
 * @param returns - status of function execution
 */
int remoteproc_boot(struct remote_proc *rproc);

/**
 * remoteproc_shutdown
 *
 * This function shutdowns the remote execution context.
 *
 * @param rproc - pointer to remoteproc instance to shutdown
 *
 * @param returns - status of function execution
 */
int remoteproc_shutdown(struct remote_proc *rproc);

#if defined __cplusplus
}
#endif

#endif				/* REMOTEPROC_H_ */
