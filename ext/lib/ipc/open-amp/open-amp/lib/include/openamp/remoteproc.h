/*
 * Remoteproc Framework
 *
 * Copyright(c) 2018 Xilinx Ltd.
 * Copyright(c) 2011 Texas Instruments, Inc.
 * Copyright(c) 2011 Google, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REMOTEPROC_H
#define REMOTEPROC_H

#include <metal/io.h>
#include <metal/mutex.h>
#include <openamp/compiler.h>

#if defined __cplusplus
extern "C" {
#endif

#define RSC_NOTIFY_ID_ANY 0xFFFFFFFFUL

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
 * @RSC_VENDOR_START: start of the vendor specific resource types range
 * @RSC_VENDOR_END  : end of the vendor specific resource types range
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
	RSC_VENDOR_START = 128,
	RSC_VENDOR_END = 512,
};

#define FW_RSC_ADDR_ANY (0xFFFFFFFFFFFFFFFF)
#define FW_RSC_U32_ADDR_ANY (0xFFFFFFFF)

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
 * struct fw_rsc_vendor - remote processor vendor specific resource
 * @len: length of the resource
 *
 * This resource entry tells the host the vendor specific resource
 * required by the remote.
 *
 * These request entries should precede other shared resource entries
 * such as vdevs, vrings.
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_vendor {
	uint32_t type;
	uint32_t len;
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

struct loader_ops;
struct image_store_ops;
struct remoteproc_ops;

/**
 * struct remoteproc_mem
 *
 * This structure presents the memory used by the remote processor
 *
 * @da: device memory
 * @pa: physical memory
 * @size: size of the memory
 * @io: pointer to the I/O region
 * @node: list node
 */
struct remoteproc_mem {
	metal_phys_addr_t da;
	metal_phys_addr_t pa;
	size_t size;
	char name[32];
	struct metal_io_region *io;
	struct metal_list node;
};

/**
 * struct remoteproc
 *
 * This structure is maintained by the remoteproc to represent the remote
 * processor instance. This structure acts as a prime parameter to use
 * the remoteproc APIs.
 *
 * @bootadd: boot address
 * @loader: executable loader
 * @lock: mutext lock
 * @ops: remoteproc operations
 * @rsc_table: pointer to resource table
 * @rsc_len: length of resource table
 * @rsc_io: metal I/O region of resource table
 * @mems: remoteproc memories
 * @vdevs: remoteproc virtio devices
 * @bitmap: bitmap for notify IDs for remoteproc subdevices
 * @state: remote processor state
 * @priv: private data
 */
struct remoteproc {
	metal_mutex_t lock;
	void *rsc_table;
	size_t rsc_len;
	struct metal_io_region *rsc_io;
	struct metal_list mems;
	struct metal_list vdevs;
	unsigned long bitmap;
	struct remoteproc_ops *ops;
	metal_phys_addr_t bootaddr;
	struct loader_ops *loader;
	unsigned int state;
	void *priv;
};

/**
 * struct remoteproc_ops
 *
 * remoteproc operations needs to be implemented by each remoteproc driver
 *
 * @init: initialize the remoteproc instance
 * @remove: remove the remoteproc instance
 * @mmap: memory mapped the mempory with physical address or destination
 *        address as input.
 * @handle_rsc: handle the vendor specific resource
 * @config: configure the remoteproc to make it ready to load and run
 *          executable
 * @start: kick the remoteproc to run application
 * @stop: stop the remoteproc from running application, the resource such as
 *        memory may not be off.
 * @shutdown: shutdown the remoteproc and release its resources.
 * @notify: notify the remote
 */
struct remoteproc_ops {
	struct remoteproc *(*init)(struct remoteproc *rproc,
				   struct remoteproc_ops *ops, void *arg);
	void (*remove)(struct remoteproc *rproc);
	void *(*mmap)(struct remoteproc *rproc,
		      metal_phys_addr_t *pa, metal_phys_addr_t *da,
		      size_t size, unsigned int attribute,
		      struct metal_io_region **io);
	int (*handle_rsc)(struct remoteproc *rproc, void *rsc, size_t len);
	int (*config)(struct remoteproc *rproc, void *data);
	int (*start)(struct remoteproc *rproc);
	int (*stop)(struct remoteproc *rproc);
	int (*shutdown)(struct remoteproc *rproc);
	int (*notify)(struct remoteproc *rproc, uint32_t id);
};

/* Remoteproc error codes */
#define RPROC_EBASE	0
#define RPROC_ENOMEM	(RPROC_EBASE + 1)
#define RPROC_EINVAL	(RPROC_EBASE + 2)
#define RPROC_ENODEV	(RPROC_EBASE + 3)
#define RPROC_EAGAIN	(RPROC_EBASE + 4)
#define RPROC_ERR_RSC_TAB_TRUNC (RPROC_EBASE + 5)
#define RPROC_ERR_RSC_TAB_VER   (RPROC_EBASE + 6)
#define RPROC_ERR_RSC_TAB_RSVD  (RPROC_EBASE + 7)
#define RPROC_ERR_RSC_TAB_VDEV_NRINGS (RPROC_EBASE + 9)
#define RPROC_ERR_RSC_TAB_NP          (RPROC_EBASE + 10)
#define RPROC_ERR_RSC_TAB_NS          (RPROC_EBASE + 11)
#define RPROC_ERR_LOADER_STATE (RPROC_EBASE + 12)
#define RPROC_EMAX	(RPROC_EBASE + 16)
#define RPROC_EPTR	(void *)(-1)
#define RPROC_EOF	(void *)(-1)

static inline long RPROC_PTR_ERR(const void *ptr)
{
	return (long)ptr;
}

static inline int RPROC_IS_ERR(const void *ptr)
{
	if ((unsigned long)ptr >= (unsigned long)(-RPROC_EMAX))
		return 1;
	else
		return 0;
}

static inline void *RPROC_ERR_PTR(long error)
{
	return (void *)error;
}

/**
 * enum rproc_state - remote processor states
 * @RPROC_OFFLINE:	remote is offline
 * @RPROC_READY:	remote is ready to start
 * @RPROC_RUNNING:	remote is up and running
 * @RPROC_SUSPENDED:	remote is suspended
 * @RPROC_ERROR:	remote has error; need to recover
 * @RPROC_STOPPED:	remote is stopped
 * @RPROC_LAST:		just keep this one at the end
 */
enum remoteproc_state {
	RPROC_OFFLINE		= 0,
	RPROC_CONFIGURED	= 1,
	RPROC_READY		= 2,
	RPROC_RUNNING		= 3,
	RPROC_SUSPENDED		= 4,
	RPROC_ERROR		= 5,
	RPROC_STOPPED		= 6,
	RPROC_LAST		= 7,
};

/**
 * remoteproc_init
 *
 * Initializes remoteproc resource.
 *
 * @rproc - pointer to remoteproc instance
 * @ops - pointer to remoteproc operations
 * @priv - pointer to private data
 *
 * @returns created remoteproc pointer
 */
struct remoteproc *remoteproc_init(struct remoteproc *rproc,
				   struct remoteproc_ops *ops, void *priv);

/**
 * remoteproc_remove
 *
 * Remove remoteproc resource
 *
 * @rproc - pointer to remoteproc instance
 *
 * returns 0 for success, negative value for failure
 */
int remoteproc_remove(struct remoteproc *rproc);

/**
 * remoteproc_init_mem
 *
 * Initialize remoteproc memory
 *
 * @mem - pointer to remoteproc memory
 * @char - memory name
 * @pa - physcial address
 * @da - device address
 * @size - memory size
 * @io - pointer to the I/O region
 */
static inline void
remoteproc_init_mem(struct remoteproc_mem *mem, const char *name,
		    metal_phys_addr_t pa, metal_phys_addr_t da,
		    size_t size, struct metal_io_region *io)
{
	if (!mem)
		return;
	if (name)
		strncpy(mem->name, name, sizeof(mem->name));
	else
		mem->name[0] = 0;
	mem->pa = pa;
	mem->da = da;
	mem->io = io;
	mem->size = size;
}

/**
 * remoteproc_add_mem
 *
 * Add remoteproc memory
 *
 * @rproc - pointer to remoteproc
 * @mem - pointer to remoteproc memory
 */
static inline void
remoteproc_add_mem(struct remoteproc *rproc, struct remoteproc_mem *mem)
{
	if (!rproc || !mem)
		return;
	metal_list_add_tail(&rproc->mems, &mem->node);
}

/**
 * remoteproc_get_io_with_name
 *
 * get remoteproc memory I/O region with name
 *
 * @rproc - pointer to the remote processor
 * @name - name of the shared memory
 * @io - pointer to the pointer of the I/O region
 *
 * returns metal I/O region pointer, NULL for failure
 */
struct metal_io_region *
remoteproc_get_io_with_name(struct remoteproc *rproc,
			    const char *name);

/**
 * remoteproc_get_io_with_pa
 *
 * get remoteproc memory I/O region with physical address
 *
 * @rproc - pointer to the remote processor
 * @pa - physical address
 *
 * returns metal I/O region pointer, NULL for failure
 */
struct metal_io_region *
remoteproc_get_io_with_pa(struct remoteproc *rproc,
			  metal_phys_addr_t pa);

/**
 * remoteproc_get_io_with_da
 *
 * get remoteproc memory I/O region with device address
 *
 * @rproc - pointer to the remote processor
 * @da - device address
 * @offset - I/O region offset of the device address
 *
 * returns metal I/O region pointer, NULL for failure
 */
struct metal_io_region *
remoteproc_get_io_with_da(struct remoteproc *rproc,
			  metal_phys_addr_t da,
			  unsigned long *offset);

/**
 * remoteproc_get_io_with_va
 *
 * get remoteproc memory I/O region with virtual address
 *
 * @rproc - pointer to the remote processor
 * @va - virtual address
 *
 * returns metal I/O region pointer, NULL for failure
 */
struct metal_io_region *
remoteproc_get_io_with_va(struct remoteproc *rproc,
			  void *va);

/**
 * remoteproc_mmap
 *
 * remoteproc mmap memory
 *
 * @rproc - pointer to the remote processor
 * @pa - physical address pointer
 * @da - device address pointer
 * @size - size of the memory
 * @attribute - memory attribute
 * @io - pointer to the I/O region
 *
 * returns pointer to the memory
 */
void *remoteproc_mmap(struct remoteproc *rproc,
		      metal_phys_addr_t *pa, metal_phys_addr_t *da,
		      size_t size, unsigned int attribute,
		      struct metal_io_region **io);

/**
 * remoteproc_parse_rsc_table
 *
 * Parse resource table of remoteproc
 *
 * @rproc - pointer to remoteproc instance
 * @rsc_table - pointer to resource table
 * @rsc_size - resource table size
 *
 * returns 0 for success and negative value for errors
 */
int remoteproc_parse_rsc_table(struct remoteproc *rproc,
			       struct resource_table *rsc_table,
			       size_t rsc_size);

/**
 * remoteproc_set_rsc_table
 *
 * Parse and set resource table of remoteproc
 *
 * @rproc - pointer to remoteproc instance
 * @rsc_table - pointer to resource table
 * @rsc_size - resource table size
 *
 * returns 0 for success and negative value for errors
 */
int remoteproc_set_rsc_table(struct remoteproc *rproc,
			     struct resource_table *rsc_table,
			     size_t rsc_size);

/**
 * remoteproc_config
 *
 * This function configures the remote processor to get it
 * ready to load and run executable.
 *
 * @rproc - pointer to remoteproc instance to start
 * @data - configuration data
 *
 * returns 0 for success and negative value for errors
 */
int remoteproc_config(struct remoteproc *rproc, void *data);

/**
 * remoteproc_start
 *
 * This function starts the remote processor.
 * It assumes the firmware is already loaded,
 *
 * @rproc - pointer to remoteproc instance to start
 *
 * returns 0 for success and negative value for errors
 */
int remoteproc_start(struct remoteproc *rproc);

/**
 * remoteproc_stop
 *
 * This function stops the remote processor but it
 * will not release its resource.
 *
 * @rproc - pointer to remoteproc instance
 *
 * returns 0 for success and negative value for errors
 */
int remoteproc_stop(struct remoteproc *rproc);

/**
 * remoteproc_shutdown
 *
 * This function shutdown the remote processor and
 * release its resources.
 *
 * @rproc - pointer to remoteproc instance
 *
 * returns 0 for success and negative value for errors
 */
int remoteproc_shutdown(struct remoteproc *rproc);

/**
 * remoteproc_load
 *
 * load executable, it expects the user application defines how to
 * open the executable file and how to get data from the executable file
 * and how to load data to the target memory.
 *
 * @rproc: pointer to the remoteproc instance
 * @path: optional path to the image file
 * @store: pointer to user defined image store argument
 * @store_ops: pointer to image store operations
 * @image_info: pointer to memory which stores image information used
 *              by remoteproc loader
 *
 * return 0 for success and negative value for failure
 */
int remoteproc_load(struct remoteproc *rproc, const char *path,
		    void *store, struct image_store_ops *store_ops,
		    void **img_info);

/**
 * remoteproc_load_noblock
 *
 * load executable, it expects the caller has loaded image data to local
 * memory and passed to the this function. If the function needs more
 * image data it will return the next expected image data offset and
 * the next expected image data length. If the function requires the
 * caller to download image data to the target memory, it will also
 * return the target physical address besides the offset and length.
 * This function can be used to load firmware in stream mode. In this
 * mode, you cannot do seek to the executable file. If the executable
 * is ELF, it cannot get the resource table section before it loads
 * the full ELF file. Furthermore, application usually don't store
 * the data which is loaded to local memory in streaming mode, and
 * thus, in this mode, it will load the binrary to the target memory
 * before it gets the resource table. And thus, when calling this funciton
 * don't put the target exectuable memory in the resource table, as
 * this function will parse the resource table after it loads the binary
 * to target memory.
 *
 * @rproc: pointer to the remoteproc instance
 * @img_data: pointer to image data for remoteproc loader to parse
 * @offset: image data offset to the beginning of the image file
 * @len: image data length
 * @image_info: pointer to memory which stores image information used
 *              by remoteproc loader
 * @pa: pointer to the target memory physical address. If the next expected
 *      data doesn't need to load to the target memory, the function will
 *      set it to ANY.
 * @io: pointer to the target memory physical address. If the next expected
 *      data doesn't need to load to the target memory, the function will
 *      set it to ANY.
 * @noffset: pointer to the next image data offset to the beginning of
 *           the image file needs to load to local or to the target
 *           memory.
 * @nlen: pointer to the next image data length needs to load to local
 *        or to the target memory.
 * @nmlen: pointer to the memory size. It is only used when the next
 *         expected data is going to be loaded to the target memory. E.g.
 *         in ELF, it is possible that loadable segment in memory is
 *         larger that the segment data in the ELF file. In this case,
 *         application will need to pad the rest of the memory with
 *         padding.
 * @padding: pointer to the padding value. It is only used when the next
 *           expected data is going to be loaded to the target memory.
 *           and the target memory size is larger than the segment data in
 *           the executable file.
 *
 * return 0 for success and negative value for failure
 */
int remoteproc_load_noblock(struct remoteproc *rproc,
			    const void *img_data, size_t offset, size_t len,
			    void **img_info,
			    metal_phys_addr_t *pa, struct metal_io_region **io,
			    size_t *noffset, size_t *nlen,
			    size_t *nmlen, unsigned char *padding);

/**
 * remoteproc_allocate_id
 *
 * allocate notifyid for resource
 *
 * @rproc - pointer to the remoteproc instance
 * @start - start of the id range
 * @end - end of the id range
 *
 * return allocated notify id
 */
unsigned int remoteproc_allocate_id(struct remoteproc *rproc,
				    unsigned int start,
				    unsigned int end);

/* remoteproc_create_virtio
 *
 * create virtio device, it returns pointer to the created virtio device.
 *
 * @rproc: pointer to the remoteproc instance
 * @vdev_id: virtio device ID
 * @role: virtio device role
 * @rst_cb: virtio device reset callback
 *
 * return pointer to the created virtio device, NULL for failure.
 */
struct virtio_device *
remoteproc_create_virtio(struct remoteproc *rproc,
			 int vdev_id, unsigned int role,
			 void (*rst_cb)(struct virtio_device *vdev));

/* remoteproc_remove_virtio
 *
 * Remove virtio device
 *
 * @rproc: pointer to the remoteproc instance
 * @vdev: pointer to the virtio device
 *
 */
void remoteproc_remove_virtio(struct remoteproc *rproc,
			      struct virtio_device *vdev);

/* remoteproc_get_notification
 *
 * remoteproc is got notified, it will check its subdevices
 * for the notification
 *
 * @rproc -  pointer to the remoteproc instance
 * @notifyid - notificatin id
 *
 * return 0 for succeed, negative value for failure
 */
int remoteproc_get_notification(struct remoteproc *rproc,
				uint32_t notifyid);
#if defined __cplusplus
}
#endif

#endif /* REMOTEPROC_H_ */
