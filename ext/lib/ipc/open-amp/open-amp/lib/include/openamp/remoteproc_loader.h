/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       remoteproc_loader.h
 *
 * COMPONENT
 *
 *         OpenAMP stack.
 *
 * DESCRIPTION
 *
 *       This file provides definitions for remoteproc loader
 *
 *
 **************************************************************************/
#ifndef REMOTEPROC_LOADER_H_
#define REMOTEPROC_LOADER_H_

#include <openamp/remoteproc.h>

#if defined __cplusplus
extern "C" {
#endif

/**
 * enum loader_type - dynamic name service announcement flags
 *
 * @ELF_LOADER: an ELF loader
 * @FIT_LOADER: a loader for Flattened Image Trees
 */
enum loader_type {
	ELF_LOADER = 0, FIT_LOADER = 1, LAST_LOADER = 2,
};

/* Loader structure definition. */

struct remoteproc_loader {
	enum loader_type type;
	void *remote_firmware;
	/* Pointer to firmware decoded info control block */
	void *fw_decode_info;

	/* Loader callbacks. */
	void *(*retrieve_entry) (struct remoteproc_loader * loader);
	void *(*retrieve_rsc) (struct remoteproc_loader * loader,
			       unsigned int *size);
	int (*load_firmware) (struct remoteproc_loader * loader);
	int (*attach_firmware) (struct remoteproc_loader * loader,
				void *firmware);
	int (*detach_firmware) (struct remoteproc_loader * loader);
	void *(*retrieve_load_addr) (struct remoteproc_loader * loader);

};

/* RemoteProc Loader functions. */
struct remoteproc_loader *remoteproc_loader_init(enum loader_type type);
int remoteproc_loader_delete(struct remoteproc_loader *loader);
int remoteproc_loader_attach_firmware(struct remoteproc_loader *loader,
				      void *firmware_image);
void *remoteproc_loader_retrieve_entry_point(struct remoteproc_loader *loader);
void *remoteproc_loader_retrieve_resource_section(struct remoteproc_loader
						  *loader, unsigned int *size);
int remoteproc_loader_load_remote_firmware(struct remoteproc_loader *loader);
void *remoteproc_get_load_address(struct remoteproc_loader *loader);

/* Supported loaders */
extern int elf_loader_init(struct remoteproc_loader *loader);

#if defined __cplusplus
}
#endif

#endif				/* REMOTEPROC_LOADER_H_ */
