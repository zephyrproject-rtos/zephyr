/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Xilinx, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       zynqmp_remoteproc_r5.c
 *
 * DESCRIPTION
 *
 *       This file is the Implementation of IPC hardware layer interface
 *       for Xilinx Zynq UltraScale+ MPSoC system.
 *
 **************************************************************************/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <metal/io.h>
#include <metal/device.h>
#include <metal/utilities.h>
#include <metal/atomic.h>
#include <metal/irq.h>
#include <metal/cpu.h>
#include <metal/alloc.h>
#include <metal/assert.h>
#include <metal/shmem.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <openamp/hil.h>
#include <openamp/virtqueue.h>

#define MAX_VRING_MEM_SIZE 0x20000
#define _rproc_wait() metal_cpu_yield()

#define UNIX_PREFIX "unix:"
#define UNIXS_PREFIX "unixs:"

struct vring_ipi_info {
	/* Socket file path */
	char *path;
	int fd;
	struct metal_io_region *vring_io;
	atomic_int sync;
};

/*--------------------------- Declare Functions ------------------------ */
static int _ipi_handler(int vect_id, void *data);
static int _enable_interrupt(struct proc_intr *intr);
static void _notify(struct hil_proc *proc, struct proc_intr *intr_info);
static int _boot_cpu(struct hil_proc *proc, unsigned int load_addr);
static void _shutdown_cpu(struct hil_proc *proc);
static int _poll(struct hil_proc *proc, int nonblock);
static int _initialize(struct hil_proc *proc);
static void _release(struct hil_proc *proc);
static struct metal_io_region* _alloc_shm(struct hil_proc *proc,
			metal_phys_addr_t pa,
			size_t size,
			struct metal_device **dev);
static void _release_shm(struct hil_proc *proc,
			struct metal_device *dev,
			struct metal_io_region *io);


/*--------------------------- Globals ---------------------------------- */
struct hil_platform_ops linux_proc_ops = {
	.enable_interrupt     = _enable_interrupt,
	.notify               = _notify,
	.boot_cpu             = _boot_cpu,
	.shutdown_cpu         = _shutdown_cpu,
	.poll                 = _poll,
	.alloc_shm = _alloc_shm,
	.release_shm = _release_shm,
	.initialize    = _initialize,
	.release    = _release,
};

static int sk_unix_client(const char *descr)
{
	struct sockaddr_un addr;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);

	memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, descr + strlen(UNIX_PREFIX),
		sizeof addr.sun_path);
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) >= 0) {
		printf("connected to %s\n", descr + strlen(UNIX_PREFIX));
		return fd;
	}

	close(fd);
	return -1;
}

static int sk_unix_server(const char *descr)
{
	struct sockaddr_un addr;
	int fd, nfd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, descr + strlen(UNIXS_PREFIX),
		sizeof addr.sun_path);
	unlink(addr.sun_path);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		goto fail;
	}

	listen(fd, 5);
	printf("Waiting for connection on %s\n", addr.sun_path);
	nfd = accept(fd, NULL, NULL);
	close(fd);
	return nfd;
fail:
	close(fd);
	return -1;
}

static int event_open(const char *descr)
{
	int fd = -1;
	int i;

	if (descr == NULL) {
		return fd;
	}

	if (memcmp(UNIX_PREFIX, descr, strlen(UNIX_PREFIX)) == 0) {
		/* UNIX.  Retry to connect a few times to give the peer a
		* chance to setup.  */
		for (i = 0; i < 100 && fd == -1; i++) {
			fd = sk_unix_client(descr);
			if (fd == -1)
				usleep(i * 10 * 1000);
		}
	}
	if (memcmp(UNIXS_PREFIX, descr, strlen(UNIXS_PREFIX)) == 0) {
		/* UNIX.  */
		fd = sk_unix_server(descr);
	}
	printf("Open IPI: %s\n", descr);
	return fd;
}

static int _ipi_handler(int vect_id, void *data)
{
	char dummy_buf[32];
	struct proc_intr *intr = data;
	struct vring_ipi_info *ipi = intr->data;

	(void) vect_id;

	read(vect_id, dummy_buf, sizeof(dummy_buf));
	atomic_flag_clear(&ipi->sync);
	return 0;
}

static int _enable_interrupt(struct proc_intr *intr)
{
	struct vring_ipi_info *ipi = intr->data;

	ipi->fd = event_open(ipi->path);
	if (ipi->fd < 0) {
		fprintf(stderr, "ERROR: Failed to open sock %s for IPI.\n",
			ipi->path);
		return -1;
	}

	intr->vect_id = ipi->fd;

	/* Register ISR */
	metal_irq_register(ipi->fd, _ipi_handler,
				NULL, intr);
	return 0;
}

static void _notify(struct hil_proc *proc, struct proc_intr *intr_info)
{

	(void)proc;
	struct vring_ipi_info *ipi = (struct vring_ipi_info *)(intr_info->data);
	if (ipi == NULL)
		return;

	char dummy = 1;
	send(ipi->fd, &dummy, 1, MSG_NOSIGNAL);
}

static int _boot_cpu(struct hil_proc *proc, unsigned int load_addr)
{
	(void)proc;
	(void)load_addr;
	return -1;
}

static void _shutdown_cpu(struct hil_proc *proc)
{
	(void)proc;
	return;
}

static struct metal_io_region* _alloc_shm(struct hil_proc *proc,
			metal_phys_addr_t pa,
			size_t size,
			struct metal_device **dev)
{
	(void)proc;
	(void)pa;
	(void)size;

	*dev = NULL;
	return NULL;

}

static void _release_shm(struct hil_proc *proc,
			struct metal_device *dev,
			struct metal_io_region *io)
{
	(void)proc;
	(void)io;
	hil_close_generic_mem_dev(dev);
}

static int _poll(struct hil_proc *proc, int nonblock)
{
	(void) nonblock;
	struct proc_vring *vring;
	struct vring_ipi_info *ipi;
	unsigned int flags;

	int num_vrings = proc->vdev.num_vrings;
	int ret = 0;
	int notified;
	int i;

	metal_assert(proc);

	notified = 0;
	while (1) {
		for (i = 0; i < num_vrings; i++) {
			vring = &proc->vdev.vring_info[i];
			ipi = (struct vring_ipi_info *)(vring->intr_info.data);
			flags = metal_irq_save_disable();
			if (!(atomic_flag_test_and_set(&ipi->sync))) {
				metal_irq_restore_enable(flags);
				virtqueue_notification(vring->vq);
				notified = 1;
			} else {
				metal_irq_restore_enable(flags);
			}
		}
		if (notified)
			return 0;
		if (nonblock)
			return -EAGAIN;
		_rproc_wait();
	}
	return ret;
}

/**
 * @brief   _adjust_vring_io - Adjust the vring I/O region to map to the
 *                             specified start device address.
 * @param[in] io - vring I/O region
 * @param[in] start_phy - start device address of the vring, this is
 *                            not the actual physical address.
 * @return adjusted I/O region
 */
static struct metal_io_region *_create_vring_io(struct metal_io_region *in_io,
						int start_phy)
{
	struct metal_io_region *io = 0;
	metal_phys_addr_t *phys;
	io = metal_allocate_memory(sizeof(struct metal_io_region));
	if (!io) {
		fprintf(stderr, "ERROR: Failed to allocation I/O for vring.\n");
		return NULL;
	}
	phys = metal_allocate_memory(sizeof(metal_phys_addr_t));
	if (!phys) {
		fprintf(stderr, "ERROR: Failed to allocation phys for vring.\n");
		metal_free_memory(io);
		return NULL;
	}
	*phys = (metal_phys_addr_t)start_phy;
	metal_io_init(io, in_io->virt, phys, in_io->size,
		      sizeof(metal_phys_addr_t)*8 - 1, 0, NULL);
	return io;
}

static int _initialize(struct hil_proc *proc)
{
	struct proc_vring *vring;
	struct vring_ipi_info *ipi;
	struct metal_io_region *io;
	int i;
	if (proc) {
		for (i = 0; i < 2; i++) {
			vring = &proc->vdev.vring_info[i];
			ipi = (struct vring_ipi_info *)vring->intr_info.data;
			if (ipi && !ipi->vring_io && vring->io) {
				io = _create_vring_io(vring->io, 0);
				if (!io)
					return -1;
				ipi->vring_io = vring->io;
				vring->io = io;
				atomic_store(&ipi->sync, 1);
			}
		}
	}
	return 0;
}

static void _release(struct hil_proc *proc)
{
	struct proc_vring *vring;
	struct vring_ipi_info *ipi;
	int i;
	if (proc) {
		for (i = 0; i < 2; i++) {
			vring = &proc->vdev.vring_info[i];
			ipi = (struct vring_ipi_info *)vring->intr_info.data;
			if (ipi) {
				if (ipi->fd >= 0) {
					metal_irq_unregister(ipi->fd, 0, NULL,
						vring);
					close(ipi->fd);
				}
				if (ipi->vring_io) {
					metal_free_memory(vring->io->physmap);
					metal_free_memory(vring->io);
					vring->io = NULL;
					if (ipi->vring_io->ops.close)
						ipi->vring_io->ops.close(ipi->vring_io);
					ipi->vring_io = NULL;
				}
			}
		}
	}
}

