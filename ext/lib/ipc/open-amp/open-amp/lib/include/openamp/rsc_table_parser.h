/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RSC_TABLE_PARSER_H
#define RSC_TABLE_PARSER_H

#include <openamp/remoteproc.h>
#include <openamp/hil.h>

#if defined __cplusplus
extern "C" {
#endif

#define RSC_TAB_SUPPORTED_VERSION           1
#define RSC_TAB_HEADER_SIZE                 12
#define RSC_TAB_MAX_VRINGS                  2

/* Standard control request handling. */
typedef int (*rsc_handler) (struct remote_proc * rproc, void *rsc);

/* Function prototypes */
int handle_rsc_table(struct remote_proc *rproc,
		     struct resource_table *rsc_table, int len);
int handle_carve_out_rsc(struct remote_proc *rproc, void *rsc);
int handle_trace_rsc(struct remote_proc *rproc, void *rsc);
int handle_dev_mem_rsc(struct remote_proc *rproc, void *rsc);
int handle_vdev_rsc(struct remote_proc *rproc, void *rsc);
int handle_rproc_mem_rsc(struct remote_proc *rproc, void *rsc);
int handle_fw_chksum_rsc(struct remote_proc *rproc, void *rsc);
int handle_mmu_rsc(struct remote_proc *rproc, void *rsc);

#if defined __cplusplus
}
#endif

#endif				/* RSC_TABLE_PARSER_H */
