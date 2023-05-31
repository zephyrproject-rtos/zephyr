/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * sysctl.h
 *
 * System management operations. For use by node control stack.
 *
 * Copyright (c) 2002-2006, K Fraser
 * Copyright (c) 2023 EPAM Systems
 *
 */

#ifndef __XEN_PUBLIC_SYSCTL_H__
#define __XEN_PUBLIC_SYSCTL_H__

#if !defined(CONFIG_XEN_DOM0)
#error "sysctl operations are intended for use by node control tools only"
#endif

#include <zephyr/sys/util_macro.h>
#include "xen.h"
#include "domctl.h"

#define XEN_SYSCTL_INTERFACE_VERSION 0x00000015

/*
 * Get physical information about the host machine
 */
/* XEN_SYSCTL_physinfo */
 /* The platform supports HVM guests. */
#define _XEN_SYSCTL_PHYSCAP_hvm          0
#define XEN_SYSCTL_PHYSCAP_hvm           BIT(_XEN_SYSCTL_PHYSCAP_hvm)
 /* The platform supports PV guests. */
#define _XEN_SYSCTL_PHYSCAP_pv           1
#define XEN_SYSCTL_PHYSCAP_pv            BIT(_XEN_SYSCTL_PHYSCAP_pv)
 /* The platform supports direct access to I/O devices with IOMMU. */
#define _XEN_SYSCTL_PHYSCAP_directio     2
#define XEN_SYSCTL_PHYSCAP_directio      BIT(_XEN_SYSCTL_PHYSCAP_directio)
/* The platform supports Hardware Assisted Paging. */
#define _XEN_SYSCTL_PHYSCAP_hap          3
#define XEN_SYSCTL_PHYSCAP_hap           BIT(_XEN_SYSCTL_PHYSCAP_hap)
/* The platform supports software paging. */
#define _XEN_SYSCTL_PHYSCAP_shadow       4
#define XEN_SYSCTL_PHYSCAP_shadow        BIT(_XEN_SYSCTL_PHYSCAP_shadow)
/* The platform supports sharing of HAP page tables with the IOMMU. */
#define _XEN_SYSCTL_PHYSCAP_iommu_hap_pt_share 5
#define XEN_SYSCTL_PHYSCAP_iommu_hap_pt_share  BIT(_XEN_SYSCTL_PHYSCAP_iommu_hap_pt_share)
#define XEN_SYSCTL_PHYSCAP_vmtrace       BIT(6)
/* The platform supports vPMU. */
#define XEN_SYSCTL_PHYSCAP_vpmu          BIT(7)

/* Xen supports the Grant v1 and/or v2 ABIs. */
#define XEN_SYSCTL_PHYSCAP_gnttab_v1     BIT(8)
#define XEN_SYSCTL_PHYSCAP_gnttab_v2     BIT(9)

/* Max XEN_SYSCTL_PHYSCAP_* constant.  Used for ABI checking. */
#define XEN_SYSCTL_PHYSCAP_MAX XEN_SYSCTL_PHYSCAP_gnttab_v2

struct xen_sysctl_physinfo {
	uint32_t threads_per_core;
	uint32_t cores_per_socket;
	uint32_t nr_cpus;     /* # CPUs currently online */
	uint32_t max_cpu_id;  /* Largest possible CPU ID on this host */
	uint32_t nr_nodes;    /* # nodes currently online */
	uint32_t max_node_id; /* Largest possible node ID on this host */
	uint32_t cpu_khz;
	uint32_t capabilities;/* XEN_SYSCTL_PHYSCAP_??? */
	uint32_t arch_capabilities;/* XEN_SYSCTL_PHYSCAP_{X86,ARM,...}_??? */
	uint32_t pad;
	uint64_aligned_t total_pages;
	uint64_aligned_t free_pages;
	uint64_aligned_t scrub_pages;
	uint64_aligned_t outstanding_pages;
	uint64_aligned_t max_mfn; /* Largest possible MFN on this host */
	uint32_t hw_cap[8];
};

/* XEN_SYSCTL_getdomaininfolist */
struct xen_sysctl_getdomaininfolist {
	/* IN variables. */
	domid_t               first_domain;
	uint32_t              max_domains;

	XEN_GUEST_HANDLE_64(xen_domctl_getdomaininfo_t) buffer;
	/* OUT variables. */
	uint32_t              num_domains;
};

/* Get physical CPU information. */
/* XEN_SYSCTL_getcpuinfo */
struct xen_sysctl_cpuinfo {
	uint64_aligned_t idletime;
};

typedef struct xen_sysctl_cpuinfo xen_sysctl_cpuinfo_t;
DEFINE_XEN_GUEST_HANDLE(xen_sysctl_cpuinfo_t);

struct xen_sysctl_getcpuinfo {
	/* IN variables. */
	uint32_t max_cpus;

	XEN_GUEST_HANDLE_64(xen_sysctl_cpuinfo_t) info;
	/* OUT variables. */
	uint32_t nr_cpus;
};

struct xen_sysctl {
	uint32_t cmd;
#define XEN_SYSCTL_readconsole                    1
#define XEN_SYSCTL_tbuf_op                        2
#define XEN_SYSCTL_physinfo                       3
#define XEN_SYSCTL_sched_id                       4
#define XEN_SYSCTL_perfc_op                       5
#define XEN_SYSCTL_getdomaininfolist              6
#define XEN_SYSCTL_debug_keys                     7
#define XEN_SYSCTL_getcpuinfo                     8
#define XEN_SYSCTL_availheap                      9
#define XEN_SYSCTL_get_pmstat                    10
#define XEN_SYSCTL_cpu_hotplug                   11
#define XEN_SYSCTL_pm_op                         12
#define XEN_SYSCTL_page_offline_op               14
#define XEN_SYSCTL_lockprof_op                   15
#define XEN_SYSCTL_cputopoinfo                   16
#define XEN_SYSCTL_numainfo                      17
#define XEN_SYSCTL_cpupool_op                    18
#define XEN_SYSCTL_scheduler_op                  19
#define XEN_SYSCTL_coverage_op                   20
#define XEN_SYSCTL_psr_cmt_op                    21
#define XEN_SYSCTL_pcitopoinfo                   22
#define XEN_SYSCTL_psr_alloc                     23
/* #define XEN_SYSCTL_tmem_op                       24 */
#define XEN_SYSCTL_get_cpu_levelling_caps        25
#define XEN_SYSCTL_get_cpu_featureset            26
#define XEN_SYSCTL_livepatch_op                  27
/* #define XEN_SYSCTL_set_parameter              28 */
#define XEN_SYSCTL_get_cpu_policy                29
	uint32_t interface_version; /* XEN_SYSCTL_INTERFACE_VERSION */
	union {
		struct xen_sysctl_physinfo          physinfo;
		struct xen_sysctl_getdomaininfolist getdomaininfolist;
		struct xen_sysctl_getcpuinfo        getcpuinfo;
		uint8_t                             pad[128];
	} u;
};

typedef struct xen_sysctl xen_sysctl_t;
DEFINE_XEN_GUEST_HANDLE(xen_sysctl_t);

#endif /* __XEN_PUBLIC_SYSCTL_H__ */
