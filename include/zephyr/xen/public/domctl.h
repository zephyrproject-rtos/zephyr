/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * domctl.h
 *
 * Domain management operations. For use by node control stack.
 *
 * Copyright (c) 2002-2003, B Dragovic
 * Copyright (c) 2002-2006, K Fraser
 */

#ifndef __XEN_PUBLIC_DOMCTL_H__
#define __XEN_PUBLIC_DOMCTL_H__

#ifndef CONFIG_XEN_DOM0
#error "domctl operations are intended for use by node control tools only"
#endif

#include "xen.h"
#include "event_channel.h"
#include "grant_table.h"
#include "memory.h"

#define XEN_DOMCTL_INTERFACE_VERSION 0x00000015

/*
 * NB. xen_domctl.domain is an IN/OUT parameter for this operation.
 * If it is specified as an invalid value (0 or >= DOMID_FIRST_RESERVED),
 * an id is auto-allocated and returned.
 */
/* XEN_DOMCTL_createdomain */
struct xen_domctl_createdomain {
	/* IN parameters */
	uint32_t ssidref;
	xen_domain_handle_t handle;
/* Is this an HVM guest (as opposed to a PV guest)? */
#define _XEN_DOMCTL_CDF_hvm		0
#define XEN_DOMCTL_CDF_hvm		(1U << _XEN_DOMCTL_CDF_hvm)
/* Use hardware-assisted paging if available? */
#define _XEN_DOMCTL_CDF_hap		1
#define XEN_DOMCTL_CDF_hap		(1U << _XEN_DOMCTL_CDF_hap)
/* Should domain memory integrity be verifed by tboot during Sx? */
#define _XEN_DOMCTL_CDF_s3_integrity	2
#define XEN_DOMCTL_CDF_s3_integrity	(1U << _XEN_DOMCTL_CDF_s3_integrity)
/* Disable out-of-sync shadow page tables? */
#define _XEN_DOMCTL_CDF_oos_off		3
#define XEN_DOMCTL_CDF_oos_off		(1U << _XEN_DOMCTL_CDF_oos_off)
/* Is this a xenstore domain? */
#define _XEN_DOMCTL_CDF_xs_domain	4
#define XEN_DOMCTL_CDF_xs_domain	(1U << _XEN_DOMCTL_CDF_xs_domain)
/* Should this domain be permitted to use the IOMMU? */
#define _XEN_DOMCTL_CDF_iommu		5
#define XEN_DOMCTL_CDF_iommu		(1U << _XEN_DOMCTL_CDF_iommu)
#define _XEN_DOMCTL_CDF_nested_virt	6
#define XEN_DOMCTL_CDF_nested_virt	(1U << _XEN_DOMCTL_CDF_nested_virt)
/* Should we expose the vPMU to the guest? */
#define XEN_DOMCTL_CDF_vpmu		(1U << 7)

/* Max XEN_DOMCTL_CDF_* constant.  Used for ABI checking. */
#define XEN_DOMCTL_CDF_MAX		XEN_DOMCTL_CDF_vpmu

	uint32_t flags;

#define _XEN_DOMCTL_IOMMU_no_sharept	0
#define XEN_DOMCTL_IOMMU_no_sharep	(1U << _XEN_DOMCTL_IOMMU_no_sharept)

/* Max XEN_DOMCTL_IOMMU_* constant. Used for ABI checking. */
#define XEN_DOMCTL_IOMMU_MAX		XEN_DOMCTL_IOMMU_no_sharept

	uint32_t iommu_opts;

	/*
	 * Various domain limits, which impact the quantity of resources
	 * (global mapping space, xenheap, etc) a guest may consume.  For
	 * max_grant_frames and max_maptrack_frames, < 0 means "use the
	 * default maximum value in the hypervisor".
	 */
	uint32_t max_vcpus;
	uint32_t max_evtchn_port;
	int32_t max_grant_frames;
	int32_t max_maptrack_frames;

/* Grant version, use low 4 bits. */
#define XEN_DOMCTL_GRANT_version_mask	0xf
#define XEN_DOMCTL_GRANT_version(v)	((v) & XEN_DOMCTL_GRANT_version_mask)

	uint32_t grant_opts;

	/* Per-vCPU buffer size in bytes.  0 to disable. */
	uint32_t vmtrace_size;

	/* CPU pool to use; specify 0 or a specific existing pool */
	uint32_t cpupool_id;

	struct xen_arch_domainconfig arch;
};

/* XEN_DOMCTL_getdomaininfo */
struct xen_domctl_getdomaininfo {
	/* OUT variables. */
	domid_t  domain;	/* Also echoed in domctl.domain */
	uint16_t pad1;
/* Domain is scheduled to die. */
#define _XEN_DOMINF_dying		0
#define XEN_DOMINF_dying		(1U << _XEN_DOMINF_dying)
/* Domain is an HVM guest (as opposed to a PV guest). */
#define _XEN_DOMINF_hvm_guest		1
#define XEN_DOMINF_hvm_guest		(1U << _XEN_DOMINF_hvm_guest)
/* The guest OS has shut down. */
#define _XEN_DOMINF_shutdown		2
#define XEN_DOMINF_shutdown		(1U << _XEN_DOMINF_shutdown)
/* Currently paused by control software. */
#define _XEN_DOMINF_paused		3
#define XEN_DOMINF_paused		(1U << _XEN_DOMINF_paused)
/* Currently blocked pending an event. */
#define _XEN_DOMINF_blocked		4
#define XEN_DOMINF_blocked		(1U << _XEN_DOMINF_blocked)
/* Domain is currently running. */
#define _XEN_DOMINF_running		5
#define XEN_DOMINF_running		(1U << _XEN_DOMINF_running)
/* Being debugged.  */
#define _XEN_DOMINF_debugged		6
#define XEN_DOMINF_debugged		(1U << _XEN_DOMINF_debugged)
/* domain is a xenstore domain */
#define _XEN_DOMINF_xs_domain		7
#define XEN_DOMINF_xs_domain		(1U << _XEN_DOMINF_xs_domain)
/* domain has hardware assisted paging */
#define _XEN_DOMINF_hap			8
#define XEN_DOMINF_hap			(1U << _XEN_DOMINF_hap)
/* XEN_DOMINF_shutdown guest-supplied code.  */
#define XEN_DOMINF_shutdownmask		255
#define XEN_DOMINF_shutdownshift	16
	uint32_t flags; /* XEN_DOMINF_* */
	uint64_aligned_t tot_pages;
	uint64_aligned_t max_pages;
	uint64_aligned_t outstanding_pages;
	uint64_aligned_t shr_pages;
	uint64_aligned_t paged_pages;
	uint64_aligned_t shared_info_frame; /* GMFN of shared_info struct */
	uint64_aligned_t cpu_time;
	uint32_t nr_online_vcpus; /* Number of VCPUs currently online. */
#define XEN_INVALID_MAX_VCPU_ID (~0U) /* Domain has no vcpus? */
	uint32_t max_vcpu_id; /* Maximum VCPUID in use by this domain. */
	uint32_t ssidref;
	xen_domain_handle_t handle;
	uint32_t cpupool;
	uint8_t gpaddr_bits; /* Guest physical address space size. */
	uint8_t pad2[7];
	struct xen_arch_domainconfig arch_config;
};
typedef struct xen_domctl_getdomaininfo xen_domctl_getdomaininfo_t;
DEFINE_XEN_GUEST_HANDLE(xen_domctl_getdomaininfo_t);

/*
 * Control shadow pagetables operation
 */
/* XEN_DOMCTL_shadow_op */

/* Memory allocation accessors. */
#define XEN_DOMCTL_SHADOW_OP_GET_ALLOCATION	30
#define XEN_DOMCTL_SHADOW_OP_SET_ALLOCATION	31

struct xen_domctl_shadow_op_stats {
	uint32_t fault_count;
	uint32_t dirty_count;
};

struct xen_domctl_shadow_op {
	/* IN variables. */
	uint32_t op; /* XEN_DOMCTL_SHADOW_OP_* */

	/* OP_ENABLE: XEN_DOMCTL_SHADOW_ENABLE_* */
	/* OP_PEAK / OP_CLEAN: XEN_DOMCTL_SHADOW_LOGDIRTY_* */
	uint32_t mode;

	/* OP_GET_ALLOCATION / OP_SET_ALLOCATION */
	uint32_t mb; /* Shadow memory allocation in MB */

	/* OP_PEEK / OP_CLEAN */
	XEN_GUEST_HANDLE_64(uint8_t) dirty_bitmap;
	uint64_aligned_t pages; /* Size of buffer. Updated with actual size. */
	struct xen_domctl_shadow_op_stats stats;
};

/* XEN_DOMCTL_max_mem */
struct xen_domctl_max_mem {
	/* IN variables. */
	uint64_aligned_t max_memkb;
};

/* XEN_DOMCTL_setvcpucontext */
/* XEN_DOMCTL_getvcpucontext */
struct xen_domctl_vcpucontext {
	uint32_t vcpu; /* IN */

	XEN_GUEST_HANDLE_64(vcpu_guest_context_t) ctxt; /* IN/OUT */
};

/*
 * XEN_DOMCTL_max_vcpus:
 *
 * The parameter passed to XEN_DOMCTL_max_vcpus must match the value passed to
 * XEN_DOMCTL_createdomain.  This hypercall is in the process of being removed
 * (once the failure paths in domain_create() have been improved), but is
 * still required in the short term to allocate the vcpus themselves.
 */
struct xen_domctl_max_vcpus {
	uint32_t max;	   /* maximum number of vcpus */
};

/* XEN_DOMCTL_scheduler_op */
/* Scheduler types. */
/* #define XEN_SCHEDULER_SEDF  4 (Removed) */
#define XEN_SCHEDULER_CREDIT	5
#define XEN_SCHEDULER_CREDIT2	6
#define XEN_SCHEDULER_ARINC653	7
#define XEN_SCHEDULER_RTDS	8
#define XEN_SCHEDULER_NULL	9

struct xen_domctl_sched_credit {
	uint16_t weight;
	uint16_t cap;
};

struct xen_domctl_sched_credit2 {
	uint16_t weight;
	uint16_t cap;
};

struct xen_domctl_sched_rtds {
	uint32_t period;
	uint32_t budget;
/* Can this vCPU execute beyond its reserved amount of time? */
#define _XEN_DOMCTL_SCHEDRT_extra	0
#define XEN_DOMCTL_SCHEDRT_extra	(1U<<_XEN_DOMCTL_SCHEDRT_extra)
	uint32_t flags;
};

typedef struct xen_domctl_schedparam_vcpu {
	union {
		struct xen_domctl_sched_credit credit;
		struct xen_domctl_sched_credit2 credit2;
		struct xen_domctl_sched_rtds rtds;
	} u;
	uint32_t vcpuid;
} xen_domctl_schedparam_vcpu_t;
DEFINE_XEN_GUEST_HANDLE(xen_domctl_schedparam_vcpu_t);

/*
 * Set or get info?
 * For schedulers supporting per-vcpu settings (e.g., RTDS):
 *  XEN_DOMCTL_SCHEDOP_putinfo sets params for all vcpus;
 *  XEN_DOMCTL_SCHEDOP_getinfo gets default params;
 *  XEN_DOMCTL_SCHEDOP_put(get)vcpuinfo sets (gets) params of vcpus;
 *
 * For schedulers not supporting per-vcpu settings:
 *  XEN_DOMCTL_SCHEDOP_putinfo sets params for all vcpus;
 *  XEN_DOMCTL_SCHEDOP_getinfo gets domain-wise params;
 *  XEN_DOMCTL_SCHEDOP_put(get)vcpuinfo returns error;
 */
#define XEN_DOMCTL_SCHEDOP_putinfo	0
#define XEN_DOMCTL_SCHEDOP_getinfo	1
#define XEN_DOMCTL_SCHEDOP_putvcpuinfo	2
#define XEN_DOMCTL_SCHEDOP_getvcpuinfo	3
struct xen_domctl_scheduler_op {
	uint32_t sched_id; /* XEN_SCHEDULER_* */
	uint32_t cmd; /* XEN_DOMCTL_SCHEDOP_* */
	/* IN/OUT */
	union {
		struct xen_domctl_sched_credit credit;
		struct xen_domctl_sched_credit2 credit2;
		struct xen_domctl_sched_rtds rtds;
		struct {
			XEN_GUEST_HANDLE_64(xen_domctl_schedparam_vcpu_t) vcpus;
			/*
			 * IN: Number of elements in vcpus array.
			 * OUT: Number of processed elements of vcpus array.
			 */
			uint32_t nr_vcpus;
			uint32_t padding;
		} v;
	} u;
};

/* XEN_DOMCTL_iomem_permission */
struct xen_domctl_iomem_permission {
	uint64_aligned_t first_mfn;/* first page (physical page number) in range */
	uint64_aligned_t nr_mfns;  /* number of pages in range (>0) */
	uint8_t  allow_access;     /* allow (!0) or deny (0) access to range? */
};

/* XEN_DOMCTL_set_address_size */
/* XEN_DOMCTL_get_address_size */
struct xen_domctl_address_size {
	uint32_t size;
};

/* Assign a device to a guest. Sets up IOMMU structures. */
/* XEN_DOMCTL_assign_device */
/*
 * XEN_DOMCTL_test_assign_device: Pass DOMID_INVALID to find out whether the
 * given device is assigned to any DomU at all. Pass a specific domain ID to
 * find out whether the given device can be assigned to that domain.
 */
/*
 * XEN_DOMCTL_deassign_device: The behavior of this DOMCTL differs
 * between the different type of device:
 *  - PCI device (XEN_DOMCTL_DEV_PCI) will be reassigned to DOM0
 *  - DT device (XEN_DOMCTL_DEV_DT) will left unassigned. DOM0
 *  will have to call XEN_DOMCTL_assign_device in order to use the
 *  device.
 */
#define XEN_DOMCTL_DEV_PCI	0
#define XEN_DOMCTL_DEV_DT	1
struct xen_domctl_assign_device {
	/* IN */
	uint32_t dev; /* XEN_DOMCTL_DEV_* */
	uint32_t flags;
#define XEN_DOMCTL_DEV_RDM_RELAXED	1 /* assign only */
	union {
		struct {
			uint32_t machine_sbdf; /* machine PCI ID of assigned device */
		} pci;
		struct {
			uint32_t size; /* Length of the path */

			XEN_GUEST_HANDLE_64(char) path; /* path to the device tree node */
		} dt;
	} u;
};

/* Pass-through interrupts: bind real irq -> hvm devfn. */
/* XEN_DOMCTL_bind_pt_irq */
/* XEN_DOMCTL_unbind_pt_irq */
enum pt_irq_type {
	PT_IRQ_TYPE_PCI,
	PT_IRQ_TYPE_ISA,
	PT_IRQ_TYPE_MSI,
	PT_IRQ_TYPE_MSI_TRANSLATE,
	PT_IRQ_TYPE_SPI, /* ARM: valid range 32-1019 */
};
struct xen_domctl_bind_pt_irq {
	uint32_t machine_irq;
	uint32_t irq_type; /* enum pt_irq_type */

	union {
		struct {
			uint8_t isa_irq;
		} isa;
		struct {
			uint8_t bus;
			uint8_t device;
			uint8_t intx;
		} pci;
		struct {
			uint8_t gvec;
			uint32_t gflags;
#define XEN_DOMCTL_VMSI_X86_DEST_ID_MASK	0x0000ff
#define XEN_DOMCTL_VMSI_X86_RH_MASK		0x000100
#define XEN_DOMCTL_VMSI_X86_DM_MASK		0x000200
#define XEN_DOMCTL_VMSI_X86_DELIV_MASK		0x007000
#define XEN_DOMCTL_VMSI_X86_TRIG_MASK		0x008000
#define XEN_DOMCTL_VMSI_X86_UNMASKED		0x010000

			uint64_aligned_t gtable;
		} msi;
		struct {
			uint16_t spi;
		} spi;
	} u;
};


/* Bind machine I/O address range -> HVM address range. */
/* XEN_DOMCTL_memory_mapping */
/* Returns
 * - zero     success, everything done
 * - -E2BIG   passed in nr_mfns value too large for the implementation
 * - positive partial success for the first <result> page frames (with
 *	    <result> less than nr_mfns), requiring re-invocation by the
 *	    caller after updating inputs
 * - negative error; other than -E2BIG
 */
#define DPCI_ADD_MAPPING	1
#define DPCI_REMOVE_MAPPING	0
struct xen_domctl_memory_mapping {
	uint64_aligned_t first_gfn; /* first page (hvm guest phys page) in range */
	uint64_aligned_t first_mfn; /* first page (machine page) in range */
	uint64_aligned_t nr_mfns;   /* number of pages in range (>0) */
	uint32_t add_mapping;       /* add or remove mapping */
	uint32_t padding;           /* padding for 64-bit aligned structure */
};

/*
 * ARM: Clean and invalidate caches associated with given region of
 * guest memory.
 */
struct xen_domctl_cacheflush {
	/* IN: page range to flush. */
	xen_pfn_t start_pfn, nr_pfns;
};

/*
 * XEN_DOMCTL_get_paging_mempool_size / XEN_DOMCTL_set_paging_mempool_size.
 *
 * Get or set the paging memory pool size.  The size is in bytes.
 *
 * This is a dedicated pool of memory for Xen to use while managing the guest,
 * typically containing pagetables.  As such, there is an implementation
 * specific minimum granularity.
 *
 * The set operation can fail mid-way through the request (e.g. Xen running
 * out of memory, no free memory to reclaim from the pool, etc.).
 */
struct xen_domctl_paging_mempool {
	uint64_aligned_t size; /* Size in bytes. */
};

struct xen_domctl {
	uint32_t cmd;
#define XEN_DOMCTL_createdomain			1
#define XEN_DOMCTL_destroydomain		2
#define XEN_DOMCTL_pausedomain			3
#define XEN_DOMCTL_unpausedomain		4
#define XEN_DOMCTL_getdomaininfo		5
#define XEN_DOMCTL_setvcpuaffinity		9
#define XEN_DOMCTL_shadow_op			10
#define XEN_DOMCTL_max_mem			11
#define XEN_DOMCTL_setvcpucontext		12
#define XEN_DOMCTL_getvcpucontext		13
#define XEN_DOMCTL_getvcpuinfo			14
#define XEN_DOMCTL_max_vcpus			15
#define XEN_DOMCTL_scheduler_op			16
#define XEN_DOMCTL_setdomainhandle		17
#define XEN_DOMCTL_setdebugging			18
#define XEN_DOMCTL_irq_permission		19
#define XEN_DOMCTL_iomem_permission		20
#define XEN_DOMCTL_ioport_permission		21
#define XEN_DOMCTL_hypercall_init		22
#define XEN_DOMCTL_settimeoffset		24
#define XEN_DOMCTL_getvcpuaffinity		25
#define XEN_DOMCTL_real_mode_area		26 /* Obsolete PPC only */
#define XEN_DOMCTL_resumedomain			27
#define XEN_DOMCTL_sendtrigger			28
#define XEN_DOMCTL_subscribe			29
#define XEN_DOMCTL_gethvmcontext		33
#define XEN_DOMCTL_sethvmcontext		34
#define XEN_DOMCTL_set_address_size		35
#define XEN_DOMCTL_get_address_size		36
#define XEN_DOMCTL_assign_device		37
#define XEN_DOMCTL_bind_pt_irq			38
#define XEN_DOMCTL_memory_mapping		39
#define XEN_DOMCTL_ioport_mapping		40
#define XEN_DOMCTL_set_ext_vcpucontext		42
#define XEN_DOMCTL_get_ext_vcpucontext		43
#define XEN_DOMCTL_set_opt_feature		44 /* Obsolete IA64 only */
#define XEN_DOMCTL_test_assign_device		45
#define XEN_DOMCTL_set_target			46
#define XEN_DOMCTL_deassign_device		47
#define XEN_DOMCTL_unbind_pt_irq		48
#define XEN_DOMCTL_get_device_group		50
#define XEN_DOMCTL_debug_op			54
#define XEN_DOMCTL_gethvmcontext_partial	55
#define XEN_DOMCTL_vm_event_op			56
#define XEN_DOMCTL_mem_sharing_op		57
#define XEN_DOMCTL_gettscinfo			59
#define XEN_DOMCTL_settscinfo			60
#define XEN_DOMCTL_getpageframeinfo3		61
#define XEN_DOMCTL_setvcpuextstate		62
#define XEN_DOMCTL_getvcpuextstate		63
#define XEN_DOMCTL_set_access_required		64
#define XEN_DOMCTL_audit_p2m			65
#define XEN_DOMCTL_set_virq_handler		66
#define XEN_DOMCTL_set_broken_page_p2m		67
#define XEN_DOMCTL_setnodeaffinity		68
#define XEN_DOMCTL_getnodeaffinity		69
#define XEN_DOMCTL_cacheflush			71
#define XEN_DOMCTL_get_vcpu_msrs		72
#define XEN_DOMCTL_set_vcpu_msrs		73
#define XEN_DOMCTL_setvnumainfo			74
#define XEN_DOMCTL_psr_cmt_op			75
#define XEN_DOMCTL_monitor_op			77
#define XEN_DOMCTL_psr_alloc			78
#define XEN_DOMCTL_soft_reset			79
#define XEN_DOMCTL_vuart_op			81
#define XEN_DOMCTL_get_cpu_policy		82
#define XEN_DOMCTL_set_cpu_policy		83
#define XEN_DOMCTL_vmtrace_op			84
#define XEN_DOMCTL_get_paging_mempool_size	85
#define XEN_DOMCTL_set_paging_mempool_size	86
#define XEN_DOMCTL_gdbsx_guestmemio		1000
#define XEN_DOMCTL_gdbsx_pausevcpu		1001
#define XEN_DOMCTL_gdbsx_unpausevcpu		1002
#define XEN_DOMCTL_gdbsx_domstatus		1003
	uint32_t interface_version; /* XEN_DOMCTL_INTERFACE_VERSION */
	domid_t  domain;
	uint16_t _pad[3];
	union {
		struct xen_domctl_createdomain createdomain;
		struct xen_domctl_getdomaininfo getdomaininfo;
		struct xen_domctl_max_mem max_mem;
		struct xen_domctl_vcpucontext vcpucontext;
		struct xen_domctl_max_vcpus max_vcpus;
		struct xen_domctl_scheduler_op scheduler_op;
		struct xen_domctl_iomem_permission iomem_permission;
		struct xen_domctl_address_size address_size;
		struct xen_domctl_assign_device assign_device;
		struct xen_domctl_bind_pt_irq bind_pt_irq;
		struct xen_domctl_memory_mapping memory_mapping;
		struct xen_domctl_cacheflush cacheflush;
		struct xen_domctl_paging_mempool paging_mempool;
		uint8_t pad[128];
	} u;
};
typedef struct xen_domctl xen_domctl_t;
DEFINE_XEN_GUEST_HANDLE(xen_domctl_t);

#endif /* __XEN_PUBLIC_DOMCTL_H__ */
