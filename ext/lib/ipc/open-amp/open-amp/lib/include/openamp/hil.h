#ifndef _HIL_H_
#define _HIL_H_

/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       hil.h
 *
 * DESCRIPTION
 *
 *       This file defines interface layer to access hardware features. This
 *       interface is used by both RPMSG and remoteproc components.
 *
 ***************************************************************************/

#include <openamp/virtio.h>
#include <openamp/firmware.h>
#include <metal/list.h>
#include <metal/io.h>
#include <metal/device.h>
#include <metal/mutex.h>

#if defined __cplusplus
extern "C" {
#endif

/* Configurable parameters */
#define HIL_MAX_CORES                   2
#define HIL_MAX_NUM_VRINGS              2
#define HIL_MAX_NUM_CHANNELS            1
/* Reserved CPU id */
#define HIL_RSVD_CPU_ID                 0xffffffff

struct hil_proc;

typedef void (*hil_proc_vdev_rst_cb_t)(struct hil_proc *proc, int id);

/**
 * struct proc_shm
 *
 * This structure is maintained by hardware interface layer for
 * shared memory information. The shared memory provides buffers
 * for use by the vring to exchange messages between the cores.
 *
 */
struct proc_shm {
	/* Start address of shared memory used for buffers. */
	void *start_addr;
	/* Start physical address of shared memory used for buffers. */
	metal_phys_addr_t start_paddr;
	/* sharmed memory I/O region */
	struct metal_io_region *io;
	/* sharmed memory metal device */
	struct metal_device *dev;
	/* Size of shared memory. */
	unsigned long size;
};

/**
* struct proc_intr
*
* This structure is maintained by hardware interface layer for
* notification(interrupts) mechanism. The most common notification mechanism
* is Inter-Processor Interrupt(IPI). There can be other mechanism depending
* on SoC architecture.
*
*/
struct proc_intr {
	/* Interrupt number for vring - use for IPI */
	unsigned int vect_id;
	/* Interrupt priority */
	unsigned int priority;
	/* Interrupt trigger type */
	unsigned int trigger_type;
	/* IPI metal device */
	struct metal_device *dev;
	/* IPI device I/O */
	struct metal_io_region *io;
	/* Private data */
	void *data;
};

/**
* struct proc_vring
*
* This structure is maintained by hardware interface layer to keep
* vring physical memory and notification info.
*
*/
struct proc_vring {
	/* Pointer to virtqueue encapsulating the vring */
	struct virtqueue *vq;
	/* Vring logical address */
	void *vaddr;
	/* Vring metal device */
	struct metal_device *dev;
	/* Vring I/O region */
	struct metal_io_region *io;
	/* Number of vring descriptors */
	unsigned short num_descs;
	/* Vring alignment */
	unsigned long align;
	/* Vring interrupt control block */
	struct proc_intr intr_info;
};

/**
 * struct proc_vdev
 *
 * This structure represents a virtio HW device for remote processor.
 * Currently only one virtio device per processor is supported.
 *
 */
struct proc_vdev {
	/* Address for the vdev info */
	void *vdev_info;
	/* Vdev interrupt control block */
	struct proc_intr intr_info;
	/* Vdev reset callback */
	hil_proc_vdev_rst_cb_t rst_cb;
	/* Number of vrings */
	unsigned int num_vrings;
	/* Virtio device features */
	unsigned int dfeatures;
	/* Virtio gen features */
	unsigned int gfeatures;
	/* Vring info control blocks */
	struct proc_vring vring_info[HIL_MAX_NUM_VRINGS];
};

/**
 * struct proc_chnl
 *
 * This structure represents channel IDs that would be used by
 * the remote in the name service message. This will be extended
 * further to support static channel creation.
 *
 */
struct proc_chnl {
	/* Channel ID */
	char name[32];
};

/**
* struct hil_proc
*
* This structure represents a remote processor and encapsulates shared
* memory and notification info required for IPC.
*
*/
struct hil_proc {
	/* HIL CPU ID */
	unsigned long cpu_id;
	/* HIL platform ops table */
	struct hil_platform_ops *ops;
	/* Resource table metal device */
	struct metal_device *rsc_dev;
	/* Resource table I/O region */
	struct metal_io_region *rsc_io;
	/* Shared memory info */
	struct proc_shm sh_buff;
	/* Virtio device hardware info */
	struct proc_vdev vdev;
	/* Number of RPMSG channels */
	unsigned long num_chnls;
	/* RPMsg channels array */
	struct proc_chnl chnls[HIL_MAX_NUM_CHANNELS];
	/* Initialized status */
	int is_initialized;
	/* hil_proc lock */
	metal_mutex_t lock;
	/* private data */
	void *pdata;
	/* List node */
	struct metal_list node;
};

/**
 * hil_create_proc
 *
 * This function creates a HIL proc instance
 *
 * @param ops - hil proc platform operations
 * @param cpu_id - remote CPU ID.
 *                 E.g. the CPU ID of the remote processor in its
 *                 cluster.
 * @param pdata  - private data
 * @return - pointer to proc instance
 *
 */
struct hil_proc *hil_create_proc(struct hil_platform_ops *ops,
				unsigned long cpu_id, void *pdata);

/**
 * hil_delete_proc
 *
 * This function deletes the given proc instance and frees the
 * associated resources.
 *
 * @param proc - pointer to HIL proc instance
 *
 */
void hil_delete_proc(struct hil_proc *proc);

/**
 * hil_init_proc
 *
 * This function initialize a HIL proc instance with the given platform data
 * @param proc  - pointer to the hil_proc to initialize
 *
 * @return - 0 succeeded, non-0 for failure
 *
 */
int hil_init_proc(struct hil_proc *proc);

/**
 * hil_notified()
 *
 * This function is called when notification is received.
 * This function gets the corresponding virtqueue and generates
 * call back for it.
 *
 * @param proc   - pointer to hil_proc
 * @param notifyid - notifyid
 *
 */
void hil_notified(struct hil_proc *proc, uint32_t notifyid);

/**
 * hil_get_vdev_info
 *
 * This function return virtio device for remote core.
 *
 * @param proc - pointer to remote proc
 *
 * @return - pointer to virtio HW device.
 *
 */
struct proc_vdev *hil_get_vdev_info(struct hil_proc *proc);

/**
 * hil_get_chnl_info
 *
 * This function returns channels info for given proc.
 *
 * @param proc - pointer to proc info struct
 * @param num_chnls - pointer to integer variable to hold
 *                    number of available channels
 *
 * @return - pointer to channel info control block
 *
 */
struct proc_chnl *hil_get_chnl_info(struct hil_proc *proc, int *num_chnls);

/**
 * hil_get_vring_info
 *
 * This function returns vring_info_table. The caller will use
 * this table to get the vring HW info which will be subsequently
 * used to create virtqueues.
 *
 * @param vdev - pointer to virtio HW device
 * @param num_vrings - pointer to hold number of vrings
 *
 * @return - pointer to vring hardware info table
 */
struct proc_vring *hil_get_vring_info(struct proc_vdev *vdev, int *num_vrings);

/**
 * hil_get_shm_info
 *
 * This function returns shared memory info control block. The caller
 * will use this information to create and manage memory buffers for
 * vring descriptor table.
 *
 * @param proc - pointer to proc instance
 *
 * @return - pointer to shared memory region used for buffers
 *
 */
struct proc_shm *hil_get_shm_info(struct hil_proc *proc);

/**
 * hil_free_virtqueues
 *
 * This function remove virt queues of the vdev.

 * @param vdev - pointer to the vdev which needs to remove vqs
 */
void hil_free_vqs(struct virtio_device *vdev);

/**
 * hil_enable_vdev_notification()
 *
 * This function enable handler for vdev notification.
 *
 * @param proc - pointer to hil_proc
 * @param id   - vdev index
 *
 * @return - execution status
 */
int hil_enable_vdev_notification(struct hil_proc *proc, int id);

/**
 * hil_enable_vring_notifications()
 *
 * This function is called after successful creation of virtqueues.
 * This function saves queue handle in the vring_info_table which
 * will be used during interrupt handling .This function setups
 * interrupt handlers.
 *
 * @param vring_index - index to vring HW table
 * @param vq          - pointer to virtqueue to save in vring HW table
 *
 * @return            - execution status
 */
int hil_enable_vring_notifications(int vring_index, struct virtqueue *vq);

/**
 * hil_vdev_notify()
 *
 * This function generates IPI to let the other side know that there is
 * change to virtio device configs.
 *
 * @param vdev - pointer to virtio device
 *
 */
void hil_vdev_notify(struct virtio_device *vdev);

/**
 * hil_vring_notify()
 *
 * This function generates IPI to let the other side know that there is
 * job available for it. The required information to achieve this, like interrupt
 * vector, CPU id etc is be obtained from the proc_vring table.
 *
 * @param vq - pointer to virtqueue
 *
 */
void hil_vring_notify(struct virtqueue *vq);

/**
 * hil_get_status
 *
 * This function is used to check if the given core is up and running.
 * This call will return after it is confirmed that remote core has
 * started.
 *
 * @param proc - pointer to proc instance
 *
 * @return - execution status
 */
int hil_get_status(struct hil_proc *proc);

/**
 * hil_set_status
 *
 * This function is used to update the status
 * of the given core i.e it is ready for IPC.
 *
 * @param proc - pointer to remote proc
 *
 * @return - execution status
 */

int hil_set_status(struct hil_proc *proc);

/** hil_create_generic_mem_dev
 *
 * This function creates generic memory device.
 * This is a helper function.
 *
 * @param pa - physical base address
 * @param size - size of the memory
 * @param flags - flags of the memory region
 *
 * @return - pointer to the memory device
 */
struct metal_device *hil_create_generic_mem_dev( metal_phys_addr_t pa,
		size_t size, unsigned int flags);

/** hil_close_generic_mem_dev
 *
 * This function closes the generic memory device.
 *
 * @param dev - pointer to the memory device.
 */
void hil_close_generic_mem_dev(struct metal_device *dev);

/**
 * hil_boot_cpu
 *
 * This function starts remote processor at given address.
 *
 * @param proc      - pointer to remote proc
 * @param load_addr - load address of remote firmware
 *
 * @return - execution status
 */
int hil_boot_cpu(struct hil_proc *proc, unsigned int load_addr);

/**
 * hil_shutdown_cpu
 *
 *  This function shutdowns the remote processor
 *
 * @param proc - pointer to remote proc
 *
 */
void hil_shutdown_cpu(struct hil_proc *proc);

/**
 * hil_get_firmware
 *
 * This function returns address and size of given firmware name passed as
 * parameter.
 *
 * @param fw_name    - name of the firmware
 * @param start_addr - pointer t hold start address of firmware
 * @param size       - pointer to hold size of firmware
 *
 * returns -  status of function execution
 *
 */
int hil_get_firmware(char *fw_name, uintptr_t *start_addr,
		     unsigned int *size);

/**
 * hil_poll
 *
 * This function polls the remote processor.
 * If it is blocking mode, it will not return until the remoteproc
 * is signaled. If it is non-blocking mode, it will return 0
 * if the remoteproc has pending signals, it will return non 0
 * otherwise.
 *
 * @param proc     - hil_proc to poll
 * @param nonblock - 0 for blocking, non-0 for non-blocking.
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_poll (struct hil_proc *proc, int nonblock);

/**
 * hil_set_shm
 *
 * This function set HIL proc shared memory
 *
 * @param proc     - hil_proc to set
 * @param bus_name - bus name of the shared memory device
 * @param name     - name of the shared memory, or platform device
 *                   mandatory for Linux system.
 * @param paddr    - physical address of the memory
 * @param size     - size of the shared memory
 *
 * If name argument exists, it will open the specified libmetal
 * shared memory or the specified libmetal device if bus_name
 * is specified to get the I/O region of the shared memory.
 * If memory name doesn't exist, it will create a metal device
 * for teh shared memory.
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_set_shm (struct hil_proc *proc,
		 const char *bus_name, const char *name,
		 metal_phys_addr_t paddr, size_t size);

/**
 * hil_set_rsc
 *
 * This function set HIL proc RSC I/O
 *
 * @param proc     - hil_proc to set vdev io regsion
 * @param bus_name - bus name of the vdev device
 * @param name     - name of the shared memory, or platform device
 *                   mandatory for Linux system.
 * @param paddr    - physical address of the memory
 * @param size     - size of the shared memory
 *
 * If name argument exists, it will open the specified libmetal
 * shared memory or the specified libmetal device if bus_name
 * is specified to get the I/O region of the shared memory.
 * If memory name doesn't exist, it will create a metal device
 * for teh shared memory.
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_set_rsc (struct hil_proc *proc,
		const char *bus_name, const char *name,
		metal_phys_addr_t paddr, size_t size);
/**
 * hil_set_vring
 *
 * This function set HIL proc vring
 *
 * @param proc     - hil_proc to set
 * @param index    - vring index
 * @param bus_name - bus name of the vring device
 * @param name     - name of the shared memory, or platform device
 *                   mandatory for Linux system.
 * @param paddr    - physical address of the memory
 * @param size     - size of the shared memory
 *
 * If name argument exists, it will open the specified libmetal
 * shared memory or the specified libmetal device if bus_name
 * is specified to get the I/O region of the shared memory.
 * If memory name doesn't exist, it will create a metal device
 * for teh shared memory.
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_set_vring (struct hil_proc *proc, int index,
		   const char *bus_name, const char *name,
		metal_phys_addr_t paddr, size_t size);

/**
 * hil_set_vdev_ipi
 *
 * This function set HIL proc vdev IPI
 *
 * @param proc     - hil_proc to set
 * @param index    - vring index for the IPI
 * @param irq      - IPI irq vector ID
 * @param data     - IPI data
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_set_vdev_ipi (struct hil_proc *proc, int index,
		 unsigned int irq, void *data);

/**
 * hil_set_vring_ipi
 *
 * This function set HIL proc vring IPI
 *
 * @param proc     - hil_proc to set
 * @param index    - vring index for the IPI
 * @param irq      - IPI irq vector ID
 * @param data     - IPI data
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_set_vring_ipi (struct hil_proc *proc, int index,
		 unsigned int irq, void *data);

/**
 * hil_set_rpmsg_channel
 *
 * This function set HIL proc rpmsg_channel
 *
 * @param proc     - hil_proc to set
 * @param index    - vring index for the rpmsg_channel
 * @param name     - RPMsg channel name
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_set_rpmsg_channel (struct hil_proc *proc, int index,
		char *name);

/**
 * hil_set_vdev_rst_cb
 *
 * This function set HIL proc vdev reset callback
 *
 * @param proc     - hil_proc to set
 * @param index    - vdev index
 * @param cb       - reset callback
 *
 * @return - 0 for no errors, non-0 for errors.
 */
int hil_set_vdev_rst_cb (struct hil_proc *proc, int index,
		hil_proc_vdev_rst_cb_t cb);

/**
 *
 * This structure is an interface between HIL and platform porting
 * component. It is required for the user to provide definitions of
 * these functions when framework is ported to new hardware platform.
 *
 */
struct hil_platform_ops {
	/**
	 * enable_interrupt()
	 *
	 * This function enables interrupt(IPI)
	 *
	 * @param intr - pointer to intr information
	 *
	 * @return  - execution status
	 */
	int (*enable_interrupt) (struct proc_intr *intr);

	/**
	 * notify()
	 *
	 * This function generates IPI to let the other side know that there is
	 * job available for it.
	 *
	 * @param proc - pointer to the hil_proc
	 * @param intr_info - pointer to interrupt info control block
	 */
	void (*notify) (struct hil_proc *proc, struct proc_intr * intr_info);

	/**
	 * boot_cpu
	 *
	 * This unction boots the remote processor.
	 *
	 * @param proc - pointer to the hil_proc
	 * @param start_addr - start address of remote cpu
	 *
	 * @return - execution status
	 */
	int (*boot_cpu) (struct hil_proc *proc, unsigned int start_addr);

	/**
	 * shutdown_cpu
	 *
	 *  This function shutdowns the remote processor.
	 *
	 * @param proc - pointer to the hil_proc
	 *
	 */
	void (*shutdown_cpu) (struct hil_proc *proc);

	/**
	 * poll
	 *
	 * This function polls the remote processor.
	 *
	 * @param proc	 - hil_proc to poll
	 * @param nonblock - 0 for blocking, non-0 for non-blocking.
	 *
	 * @return - 0 for no errors, non-0 for errors.
	 */
	int (*poll) (struct hil_proc *proc, int nonblock);

	/**
	 * alloc_shm
	 *
	 *  This function is to allocate shared memory
	 *
	 * @param[in] proc - pointer to the remote processor
	 * @param[in] pa - physical address
	 * @param[in] size - size of the shared memory
	 * @param[out] dev - pointer to the mem dev pointer
	 *
	 * @return - NULL, pointer to the I/O region
	 *
	 */
	struct metal_io_region *(*alloc_shm) (struct hil_proc *proc,
				metal_phys_addr_t pa,
				size_t size,
				struct metal_device **dev);

	/**
	 * release_shm
	 *
	 *  This function is to release shared memory
	 *
	 * @param[in] proc - pointer to the remote processor
	 * @param[in] dev - pointer to the mem dev
	 * @param[in] io - pointer to the I/O region
	 *
	 */
	void (*release_shm) (struct hil_proc *proc,
				struct metal_device *dev,
				struct metal_io_region *io);

	/**
	 * initialize
	 *
	 *  This function initialize remote processor with platform data.
	 *
	 * @param proc	 - hil_proc to poll
	 *
	 * @return NULL on failure, hil_proc pointer otherwise
	 *
	 */
	int (*initialize) (struct hil_proc *proc);

	/**
	 * release
	 *
	 *  This function is to release remote processor resource
	 *
	 * @param[in] proc - pointer to the remote processor
	 *
	 */
	void (*release) (struct hil_proc *proc);
};

/* Utility macros for register read/write */
#define         HIL_MEM_READ8(addr)         *(volatile unsigned char *)(addr)
#define         HIL_MEM_READ16(addr)        *(volatile unsigned short *)(addr)
#define         HIL_MEM_READ32(addr)        *(volatile unsigned long *)(addr)
#define         HIL_MEM_WRITE8(addr,data)   *(volatile unsigned char *)(addr) = (unsigned char)(data)
#define         HIL_MEM_WRITE16(addr,data)  *(volatile unsigned short *)(addr) = (unsigned short)(data)
#define         HIL_MEM_WRITE32(addr,data)  *(volatile unsigned long *)(addr) = (unsigned long)(data)

#if defined __cplusplus
}
#endif

#endif				/* _HIL_H_ */
