/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the PCIe driver APIs.
 * @ingroup pcie_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_H_

/**
 * @brief Interfaces for PCIe devices.
 * @defgroup pcie_interface PCIe
 * @ingroup io_interfaces
 * @{
 * @}
 *
 * @brief Interfaces for PCIe Host.
 * @defgroup pcie_host_interface PCIe Host
 * @ingroup pcie_interface
 * @{
 */

#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pcie/pcie.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A PCI(e) bus, device, function tuple within one controller.
 *
 * Different controllers may expose the same tuple. The internal structure is documented
 * in include/dt-bindings/pcie/pcie.h: see PCIE_BDF() and friends, since
 * these tuples are referenced from devicetree.
 */
typedef uint32_t pcie_bdf_t;

/** Identity of an endpoint owned by a device-model PCIe controller. */
struct pcie_endpoint {
	/** Owning controller; NULL never selects the chosen controller. */
	const struct device *ctrl;
	/** Bus, device and function within the controller. */
	pcie_bdf_t bdf;
};

/**
 * @brief A unique PCI(e) identifier (vendor ID, device ID).
 *
 * The PCIE_CONF_ID register for each endpoint is a (vendor ID, device ID)
 * pair, which is meant to tell the system what the PCI(e) endpoint is. Again,
 * look to PCIE_ID_* macros in include/dt-bindings/pcie/pcie.h for more.
 */
typedef uint32_t pcie_id_t;

/* Helper macro to exclude invalid PCIe identifiers. We should really only
 * need to look for PCIE_ID_NONE, but because of some broken PCI host controllers
 * we have try cases where both VID & DID are zero or just one of them is
 * zero (0x0000) and the other is all ones (0xFFFF).
 */
#define PCIE_ID_IS_VALID(id) ((id != PCIE_ID_NONE) && \
			      (id != PCIE_ID(0x0000, 0x0000)) && \
			      (id != PCIE_ID(0xFFFF, 0x0000)) && \
			      (id != PCIE_ID(0x0000, 0xFFFF)))

struct pcie_dev {
	pcie_bdf_t bdf;
	pcie_id_t  id;
	uint32_t   class_rev;
	uint32_t   class_rev_mask;
	/** Expected controller, set before enumeration; unused on architecture backends. */
	const struct device *ctrl;
};

/**
 * @brief Construct an endpoint from a static PCIe declaration.
 * @param dev Non-NULL declaration whose controller and BDF are already assigned.
 * @return Endpoint value; no ownership is transferred.
 */
static inline struct pcie_endpoint pcie_dev_endpoint(const struct pcie_dev *dev)
{
	return (struct pcie_endpoint){.ctrl = dev->ctrl, .bdf = dev->bdf};
}

/**
 * @brief Get a node's PCIe controller.
 * @param node_id Endpoint node; indirect children require a pcie-controller phandle.
 * @return Controller node identifier.
 */
#define PCIE_DT_CONTROLLER(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, pcie_controller), \
		    (DT_PHANDLE(node_id, pcie_controller)), (DT_PARENT(node_id)))

#ifdef CONFIG_PCIE_CONTROLLER
#define Z_PCIE_DT_CTRL(node_id) DEVICE_DT_GET_OR_NULL(PCIE_DT_CONTROLLER(node_id))
#else
#define Z_PCIE_DT_CTRL(node_id) NULL
#endif

#define Z_DEVICE_PCIE_NAME(node_id) _CONCAT(pcie_dev_, DT_DEP_ORD(node_id))

/**
 * @brief Get the PCIe Vendor and Device ID for a node
 *
 * @param node_id DTS node identifier
 * @return The VID/DID combination as pcie_id_t
 */
#define PCIE_DT_ID(node_id) PCIE_ID(DT_PROP_OR(node_id, vendor_id, 0xffff), \
				    DT_PROP_OR(node_id, device_id, 0xffff))

/**
 * @brief Get the PCIe Vendor and Device ID for a node
 *
 * This is equivalent to
 * <tt>PCIE_DT_ID(DT_DRV_INST(inst))</tt>
 *
 * @param inst Devicetree instance number
 * @return The VID/DID combination as pcie_id_t
 */
#define PCIE_DT_INST_ID(inst) PCIE_DT_ID(DT_DRV_INST(inst))

/**
 * @brief Declare an endpoint-aware PCIe context variable for a DTS node
 *
 * The driver must use controller-qualified endpoint operations. This must be
 * done before using DEVICE_PCIE_INIT() for the same node.
 *
 * @param node_id DTS node identifier
 */
#define DEVICE_PCIE_ENDPOINT_DECLARE(node_id)                               \
	STRUCT_SECTION_ITERABLE(pcie_dev, Z_DEVICE_PCIE_NAME(node_id)) = {  \
		.bdf = PCIE_BDF_NONE,                                       \
		.id = PCIE_DT_ID(node_id),                                  \
		.class_rev = DT_PROP_OR(node_id, class_rev, 0),             \
		.class_rev_mask = DT_PROP_OR(node_id, class_rev_mask, 0),   \
		.ctrl = Z_PCIE_DT_CTRL(node_id),                           \
	}

/**
 * @brief Declare an endpoint-aware PCIe context for a driver instance.
 *
 * This is equivalent to
 * <tt>DEVICE_PCIE_ENDPOINT_DECLARE(DT_DRV_INST(inst))</tt>
 *
 * @param inst Devicetree instance number
 */
#define DEVICE_PCIE_ENDPOINT_INST_DECLARE(inst) DEVICE_PCIE_ENDPOINT_DECLARE(DT_DRV_INST(inst))

#ifdef CONFIG_PCIE_CONTROLLER
#define Z_PCIE_ASSERT_LEGACY(node_id) \
	BUILD_ASSERT(DT_SAME_NODE(PCIE_DT_CONTROLLER(node_id), DT_CHOSEN(zephyr_pcie_controller)), \
		     "BDF-only PCIe driver requires the chosen controller")
#else
#define Z_PCIE_ASSERT_LEGACY(node_id)
#endif

/**
 * @brief Declare a PCIe context for a driver using BDF-only host operations.
 *
 * Reject non-chosen controller placements at build time. Drivers using endpoint
 * operations instead use DEVICE_PCIE_ENDPOINT_DECLARE().
 *
 * @param node_id DTS node identifier
 */
#define DEVICE_PCIE_DECLARE(node_id) \
	Z_PCIE_ASSERT_LEGACY(node_id); \
	DEVICE_PCIE_ENDPOINT_DECLARE(node_id)

/**
 * @brief Declare a legacy PCIe context for a driver instance.
 *
 * Equivalent to DEVICE_PCIE_DECLARE(DT_DRV_INST(inst)).
 *
 * @param inst Devicetree instance number
 */
#define DEVICE_PCIE_INST_DECLARE(inst) DEVICE_PCIE_DECLARE(DT_DRV_INST(inst))

/**
 * @brief Initialize a named struct member to point at a PCIe context
 *
 * Initialize PCIe-related information within a specific instance of
 * a device config struct, using information from DTS. Using the macro
 * requires having first created PCIe context struct using the
 * DEVICE_PCIE_DECLARE() macro.
 *
 * Example for an instance of a driver belonging to the "foo" subsystem
 *
 * struct foo_config {
 *	struct pcie_dev *pcie;
 *	...
 *};
 *
 * DEVICE_PCIE_ID_DECLARE(DT_DRV_INST(...));
 * struct foo_config my_config = {
	DEVICE_PCIE_INIT(pcie, DT_DRV_INST(...)),
 *	...
 * };
 *
 * @param node_id DTS node identifier
 * @param name Member name within config for the MMIO region
 */
#define DEVICE_PCIE_INIT(node_id, name) .name = &Z_DEVICE_PCIE_NAME(node_id)

/**
 * @brief Initialize a named struct member to point at a PCIe context
 *
 * This is equivalent to
 * <tt>DEVICE_PCIE_INIT(DT_DRV_INST(inst), name)</tt>
 *
 * @param inst Devicetree instance number
 * @param name Name of the struct member (of type struct pcie_dev *)
 */
#define DEVICE_PCIE_INST_INIT(inst, name) \
	DEVICE_PCIE_INIT(DT_DRV_INST(inst), name)

struct pcie_bar {
	uintptr_t phys_addr;
	size_t size;
};

/**
 * @brief Read a configuration word through the endpoint's controller.
 * @param ep Endpoint with a non-NULL controller.
 * @param reg Configuration word index, less than 1024.
 * @param value Non-NULL output word. All-ones data is passed through unchanged.
 * @retval 0 Word read.
 * @retval -EINVAL Invalid endpoint, BDF encoding, index or output pointer.
 * @retval -ENOTSUP Configuration reads are unsupported.
 * @return Negative backend errno from a status-bearing callback on failure.
 */
int pcie_endpoint_conf_read(const struct pcie_endpoint *ep, unsigned int reg, uint32_t *value);

/**
 * @brief Write a configuration word through the endpoint's controller.
 * @param ep Endpoint with a non-NULL controller.
 * @param reg Configuration word index, less than 1024.
 * @param value Word to write.
 * @retval 0 Word written.
 * @retval -EINVAL Invalid endpoint, BDF encoding or index.
 * @retval -ENOTSUP Configuration writes are unsupported.
 * @return Negative backend errno from a status-bearing callback on failure.
 */
int pcie_endpoint_conf_write(const struct pcie_endpoint *ep, unsigned int reg, uint32_t value);

/**
 * @brief Find a conventional capability through the endpoint's controller.
 * @param ep Endpoint with a non-NULL controller.
 * @param cap_id Capability ID to find.
 * @param cap Non-NULL output word index; zero when absent.
 * @retval 0 Query completed, including an absent capability.
 * @retval -EINVAL Invalid endpoint, output pointer or capability chain.
 * @retval -ENOTSUP Configuration reads are unsupported.
 * @return Negative backend errno on failure.
 */
int pcie_endpoint_get_cap(const struct pcie_endpoint *ep, uint32_t cap_id, uint32_t *cap);

/**
 * @brief Find an extended capability through the endpoint's controller.
 * @param ep Endpoint with a non-NULL controller.
 * @param cap_id Extended capability ID to find.
 * @param cap Non-NULL output word index; zero when absent.
 * @retval 0 Query completed, including an absent capability.
 * @retval -EINVAL Invalid endpoint, output pointer or capability chain.
 * @retval -ENOTSUP Configuration reads are unsupported.
 * @return Negative backend errno on failure.
 */
int pcie_endpoint_get_ext_cap(const struct pcie_endpoint *ep, uint32_t cap_id, uint32_t *cap);

/**
 * @brief Probe a memory BAR through the endpoint's controller.
 *
 * Configuration access and bus-to-CPU translation use the same controller.
 * The caller must serialize BAR sizing against accesses to the device.
 * Type 0 headers have six BAR registers; Type 1 headers have two. A 64-bit BAR
 * must fit entirely within those registers. Other header types are unsupported.
 *
 * @param ep Endpoint with a non-NULL controller.
 * @param bar_index BAR register index, from 0 to 5.
 * @param bar Non-NULL output BAR descriptor.
 * @retval 0 Memory BAR found and translated.
 * @retval -EINVAL Invalid endpoint, BDF encoding, index above 5 or output pointer.
 * @retval -ENOENT BAR is absent, unassigned, an I/O BAR or a 64-bit BAR's upper word.
 *                 Also returned when the BAR index or a 64-bit BAR extends beyond
 *                 the header's BAR registers.
 * @retval -ENOTSUP Header type, access, translation or the BAR address/size is unsupported.
 * @return Negative backend errno on failure.
 */
int pcie_endpoint_get_mbar(const struct pcie_endpoint *ep, unsigned int bar_index,
			   struct pcie_bar *bar);

/*
 * These functions are arch-, board-, or SoC-specific.
 */

/**
 * @brief Read a 32-bit word from an endpoint's configuration space.
 *
 * This function is exported by the arch/SoC/board code.
 *
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @return the word read (0xFFFFFFFFU if nonexistent endpoint or word)
 */
extern uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg);

/**
 * @brief Write a 32-bit word to an endpoint's configuration space.
 *
 * This function is exported by the arch/SoC/board code.
 *
 * @param bdf PCI(e) endpoint
 * @param reg the configuration word index (not address)
 * @param data the value to write
 */
extern void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data);

/** Callback type used for scanning for PCI endpoints
 *
 * @param bdf      BDF value for a found endpoint.
 * @param id       Vendor & Device ID for the found endpoint.
 * @param cb_data  Custom, use case specific data.
 *
 * @return true to continue scanning, false to stop scanning.
 */
typedef bool (*pcie_scan_cb_t)(pcie_bdf_t bdf, pcie_id_t id, void *cb_data);

/**
 * @brief Visit an endpoint during a controller-specific scan.
 * @param ep Endpoint identity, valid only during the callback; copy to retain it.
 * @param id Vendor/device ID.
 * @param cb_data Caller data.
 * @return True to continue, false to stop.
 */
typedef bool (*pcie_endpoint_scan_cb_t)(const struct pcie_endpoint *ep, pcie_id_t id,
				      void *cb_data);

enum {
	/** Follow bridges and sub-buses within the selected controller. */
	PCIE_SCAN_RECURSIVE = BIT(0),
	/** Do the callback for all endpoint types, including bridges */
	PCIE_SCAN_CB_ALL = BIT(1),
};

/** Options for performing a scan for PCI devices */
struct pcie_scan_opt {
	/** Initial bus number to scan */
	uint8_t bus;

	/** Function to call for each found endpoint */
	pcie_scan_cb_t cb;

	/** Custom data to pass to the scan callback */
	void *cb_data;

	/** Scan flags */
	uint32_t flags;

	/** Function to call to determine if a BDF is masked */
	bool (*scan_masking_fn)(pcie_bdf_t bdf);
};

/** Options for scanning a device-model PCIe controller. */
struct pcie_scan_ctrl_opt {
	/** Initial bus number to scan. */
	uint8_t bus;
	/** Callback receiving the owning controller and BDF. */
	pcie_endpoint_scan_cb_t cb;
	/** Caller data passed to both callbacks. */
	void *cb_data;
	/** PCIE_SCAN_RECURSIVE and/or PCIE_SCAN_CB_ALL flags. */
	uint32_t flags;

	/** Return true to skip this endpoint and its sub-buses. */
	bool (*scan_masking_fn)(const struct pcie_endpoint *ep, void *cb_data);
};

/** Scan for PCIe devices.
 *
 * Scan the chosen controller on device-model platforms, or the architecture's PCI bus.
 * This function does not scan other device-model controllers.
 *
 * @param opt Options determining how to perform the scan.
 * @return 0 on success, negative POSIX error number on failure.
 */
int pcie_scan(const struct pcie_scan_opt *opt);

/**
 * @brief Scan one controller and, optionally, its sub-buses.
 * @param ctrl Non-NULL controller. The caller must initialize it before scanning.
 * @param opt Non-NULL controller-specific options with cb set.
 * @retval 0 Scan completed or stopped by the callback.
 * @retval -EINVAL Invalid controller, options or callback.
 * @retval -ENOTSUP Controller configuration reads are unsupported.
 * @return Negative backend errno on failure.
 */
int pcie_scan_ctrl(const struct device *ctrl, const struct pcie_scan_ctrl_opt *opt);

/**
 * @brief Get the MBAR at a specific BAR index
 * @param bdf the PCI(e) endpoint
 * @param bar_index 0-based BAR index
 * @param mbar Pointer to struct pcie_bar
 * @return true if the mbar was found and is valid, false otherwise
 */
extern bool pcie_get_mbar(pcie_bdf_t bdf,
			  unsigned int bar_index,
			  struct pcie_bar *mbar);

/**
 * @brief Probe the nth MMIO address assigned to an endpoint.
 * @param bdf the PCI(e) endpoint
 * @param index (0-based) index
 * @param mbar Pointer to struct pcie_bar
 * @return true if the mbar was found and is valid, false otherwise
 *
 * A PCI(e) endpoint has 0 or more memory-mapped regions. This function
 * allows the caller to enumerate them by calling with index=0..n.
 * Value of n has to be below 6, as there is a maximum of 6 BARs. The indices
 * are order-preserving with respect to the endpoint BARs: e.g., index 0
 * will return the lowest-numbered memory BAR on the endpoint.
 */
extern bool pcie_probe_mbar(pcie_bdf_t bdf,
			    unsigned int index,
			    struct pcie_bar *mbar);

/**
 * @brief Get the I/O BAR at a specific BAR index
 * @param bdf the PCI(e) endpoint
 * @param bar_index 0-based BAR index
 * @param iobar Pointer to struct pcie_bar
 * @return true if the I/O BAR was found and is valid, false otherwise
 */
extern bool pcie_get_iobar(pcie_bdf_t bdf,
			   unsigned int bar_index,
			   struct pcie_bar *iobar);

/**
 * @brief Probe the nth I/O BAR address assigned to an endpoint.
 * @param bdf the PCI(e) endpoint
 * @param index (0-based) index
 * @param iobar Pointer to struct pcie_bar
 * @return true if the I/O BAR was found and is valid, false otherwise
 *
 * A PCI(e) endpoint has 0 or more I/O regions. This function
 * allows the caller to enumerate them by calling with index=0..n.
 * Value of n has to be below 6, as there is a maximum of 6 BARs. The indices
 * are order-preserving with respect to the endpoint BARs: e.g., index 0
 * will return the lowest-numbered I/O BAR on the endpoint.
 */
extern bool pcie_probe_iobar(pcie_bdf_t bdf,
			     unsigned int index,
			     struct pcie_bar *iobar);

/**
 * @brief Set or reset bits in the endpoint command/status register.
 *
 * @param bdf the PCI(e) endpoint
 * @param bits the powerset of bits of interest
 * @param on use true to set bits, false to reset them
 */
extern void pcie_set_cmd(pcie_bdf_t bdf, uint32_t bits, bool on);

#ifndef CONFIG_PCIE_CONTROLLER
/**
 * @brief Allocate an IRQ for an endpoint.
 *
 * This function first checks the IRQ register and if it contains a valid
 * value this is returned. If the register does not contain a valid value
 * allocation of a new one is attempted.
 * Such function is only exposed if CONFIG_PCIE_CONTROLLER is unset.
 * It is thus available where architecture tied dynamic IRQ allocation for
 * PCIe device makes sense.
 *
 * @param bdf the PCI(e) endpoint
 * @return the IRQ number, or PCIE_CONF_INTR_IRQ_NONE if allocation failed.
 */
extern unsigned int pcie_alloc_irq(pcie_bdf_t bdf);
#endif /* CONFIG_PCIE_CONTROLLER */

/**
 * @brief Return the IRQ assigned by the firmware/board to an endpoint.
 *
 * @param bdf the PCI(e) endpoint
 * @return the IRQ number, or PCIE_CONF_INTR_IRQ_NONE if unknown.
 */
extern unsigned int pcie_get_irq(pcie_bdf_t bdf);

/**
 * @brief Enable the PCI(e) endpoint to generate the specified IRQ.
 *
 * @param bdf the PCI(e) endpoint
 * @param irq the IRQ to generate
 *
 * If MSI is enabled and the endpoint supports it, the endpoint will
 * be configured to generate the specified IRQ via MSI. Otherwise, it
 * is assumed that the IRQ has been routed by the boot firmware
 * to the specified IRQ, and the IRQ is enabled (at the I/O APIC, or
 * wherever appropriate).
 */
extern void pcie_irq_enable(pcie_bdf_t bdf, unsigned int irq);

/**
 * @brief Find a PCI(e) capability in an endpoint's configuration space.
 *
 * @param bdf the PCI endpoint to examine
 * @param cap_id the capability ID of interest
 * @return the index of the configuration word, or 0 if no capability.
 */
extern uint32_t pcie_get_cap(pcie_bdf_t bdf, uint32_t cap_id);

/**
 * @brief Find an Extended PCI(e) capability in an endpoint's configuration space.
 *
 * @param bdf the PCI endpoint to examine
 * @param cap_id the capability ID of interest
 * @return the index of the configuration word, or 0 if no capability.
 */
extern uint32_t pcie_get_ext_cap(pcie_bdf_t bdf, uint32_t cap_id);

/**
 * @brief Dynamically connect a PCIe endpoint IRQ to an ISR handler
 *
 * @param bdf the PCI endpoint to examine
 * @param irq the IRQ to connect (see pcie_alloc_irq())
 * @param priority priority of the IRQ
 * @param routine the ISR handler to connect to the IRQ
 * @param parameter the parameter to provide to the handler
 * @param flags IRQ connection flags
 * @return true if connected, false otherwise
 */
extern bool pcie_connect_dynamic_irq(pcie_bdf_t bdf,
				     unsigned int irq,
				     unsigned int priority,
				     void (*routine)(const void *parameter),
				     const void *parameter,
				     uint32_t flags);

/**
 * @brief Get the BDF for a given PCI host controller
 *
 * This macro is useful when the PCI host controller behind PCIE_BDF(0, 0, 0)
 * indicates a multifunction device. In such a case each function of this
 * endpoint is a potential host controller itself.
 *
 * @param n Bus number
 * @return BDF value of the given host controller
 */
#define PCIE_HOST_CONTROLLER(n) PCIE_BDF(0, 0, n)

/*
 * Configuration word 13 contains the head of the capabilities list.
 */

#define PCIE_CONF_CAPPTR	13U	/* capabilities pointer */
#define PCIE_CONF_CAPPTR_FIRST(w)	(((w) >> 2) & 0x3FU)

/*
 * The first word of every capability contains a capability identifier,
 * and a link to the next capability (or 0) in configuration space.
 */

#define PCIE_CONF_CAP_ID(w)		((w) & 0xFFU)
#define PCIE_CONF_CAP_NEXT(w)		(((w) >> 10) & 0x3FU)

/*
 * The extended PCI Express capabilities lie at the end of the PCI configuration space
 */

#define PCIE_CONF_EXT_CAPPTR	64U

/*
 * The first word of every capability contains an extended capability identifier,
 * and a link to the next capability (or 0) in the extended configuration space.
 */

#define PCIE_CONF_EXT_CAP_ID(w)		((w) & 0xFFFFU)
#define PCIE_CONF_EXT_CAP_VER(w)	(((w) >> 16) & 0xFU)
#define PCIE_CONF_EXT_CAP_NEXT(w)	(((w) >> 20) & 0xFFFU)

/*
 * Configuration word 0 aligns directly with pcie_id_t.
 */

#define PCIE_CONF_ID		0U

/*
 * Configuration word 1 contains command and status bits.
 */

#define PCIE_CONF_CMDSTAT	1U	/* command/status register */

#define PCIE_CONF_CMDSTAT_IO		0x00000001U  /* I/O access enable */
#define PCIE_CONF_CMDSTAT_MEM		0x00000002U  /* mem access enable */
#define PCIE_CONF_CMDSTAT_MASTER	0x00000004U  /* bus master enable */
#define PCIE_CONF_CMDSTAT_SERR		0x00000100U  /* SERR# enable */
#define PCIE_CONF_CMDSTAT_INTERRUPT	0x00080000U  /* interrupt status */
#define PCIE_CONF_CMDSTAT_CAPS		0x00100000U  /* capabilities list */

/*
 * Configuration word 2 has additional function identification that
 * we only care about for debug output (PCIe shell commands).
 */

#define PCIE_CONF_CLASSREV	2U	/* class/revision register */

#define PCIE_CONF_CLASSREV_CLASS(w)	(((w) >> 24) & 0xFFU)
#define PCIE_CONF_CLASSREV_SUBCLASS(w)  (((w) >> 16) & 0xFFU)
#define PCIE_CONF_CLASSREV_PROGIF(w)	(((w) >> 8) & 0xFFU)
#define PCIE_CONF_CLASSREV_REV(w)	((w) & 0xFFU)

/*
 * The only part of configuration word 3 that is of interest to us is
 * the header type, as we use it to distinguish functional endpoints
 * from bridges (which are, for our purposes, transparent).
 */

#define PCIE_CONF_TYPE		3U

#define PCIE_CONF_MULTIFUNCTION(w)	(((w) & 0x00800000U) != 0U)
#define PCIE_CONF_TYPE_BRIDGE(w)	(((w) & 0x007F0000U) != 0U)
#define PCIE_CONF_TYPE_GET(w)		(((w) >> 16) & 0x7F)

#define PCIE_CONF_TYPE_STANDARD         0x0U
#define PCIE_CONF_TYPE_PCI_BRIDGE       0x1U
#define PCIE_CONF_TYPE_CARDBUS_BRIDGE   0x2U

/*
 * Words 4-9 are BARs are I/O or memory decoders. Memory decoders may
 * be 64-bit decoders, in which case the next configuration word holds
 * the high-order bits (and is, thus, not a BAR itself).
 */

#define PCIE_CONF_BAR0		4U
#define PCIE_CONF_BAR1		5U
#define PCIE_CONF_BAR2		6U
#define PCIE_CONF_BAR3		7U
#define PCIE_CONF_BAR4		8U
#define PCIE_CONF_BAR5		9U

#define PCIE_CONF_BAR_IO(w)		(((w) & 0x00000001U) == 0x00000001U)
#define PCIE_CONF_BAR_MEM(w)		(((w) & 0x00000001U) != 0x00000001U)
#define PCIE_CONF_BAR_64(w)		(((w) & 0x00000006U) == 0x00000004U)
#define PCIE_CONF_BAR_ADDR(w)		((w) & ~0xfUL)
#define PCIE_CONF_BAR_IO_ADDR(w)	((w) & ~0x3UL)
#define PCIE_CONF_BAR_FLAGS(w)		((w) & 0xfUL)
#define PCIE_CONF_BAR_NONE		0U

#define PCIE_CONF_BAR_INVAL		0xFFFFFFF0U
#define PCIE_CONF_BAR_INVAL64		0xFFFFFFFFFFFFFFF0UL

#define PCIE_CONF_BAR_INVAL_FLAGS(w)			\
	((((w) & 0x00000006U) == 0x00000006U) ||	\
	 (((w) & 0x00000006U) == 0x00000002U))

/*
 * Type 1 Header has files related to bus management
 */
#define PCIE_BUS_NUMBER         6U

#define PCIE_BUS_PRIMARY_NUMBER(w)      ((w) & 0xffUL)
#define PCIE_BUS_SECONDARY_NUMBER(w)    (((w) >> 8) & 0xffUL)
#define PCIE_BUS_SUBORDINATE_NUMBER(w)  (((w) >> 16) & 0xffUL)
#define PCIE_SECONDARY_LATENCY_TIMER(w) (((w) >> 24) & 0xffUL)

#define PCIE_BUS_NUMBER_VAL(prim, sec, sub, lat) \
	(((prim) & 0xffUL) |			 \
	 (((sec) & 0xffUL) << 8) |		 \
	 (((sub) & 0xffUL) << 16) |		 \
	 (((lat) & 0xffUL) << 24))

/*
 * Type 1 words 7 to 12 setups Bridge Memory base and limits
 */
#define PCIE_IO_SEC_STATUS      7U

#define PCIE_IO_BASE(w)         ((w) & 0xffUL)
#define PCIE_IO_LIMIT(w)        (((w) >> 8) & 0xffUL)
#define PCIE_SEC_STATUS(w)      (((w) >> 16) & 0xffffUL)

#define PCIE_IO_SEC_STATUS_VAL(iob, iol, sec_status) \
	(((iob) & 0xffUL) |			     \
	 (((iol) & 0xffUL) << 8) |		     \
	 (((sec_status) & 0xffffUL) << 16))

#define PCIE_MEM_BASE_LIMIT     8U

#define PCIE_MEM_BASE(w)        ((w) & 0xffffUL)
#define PCIE_MEM_LIMIT(w)       (((w) >> 16) & 0xffffUL)

#define PCIE_MEM_BASE_LIMIT_VAL(memb, meml) \
	(((memb) & 0xffffUL) |		    \
	 (((meml) & 0xffffUL) << 16))

#define PCIE_PREFETCH_BASE_LIMIT        9U

#define PCIE_PREFETCH_BASE(w)   ((w) & 0xffffUL)
#define PCIE_PREFETCH_LIMIT(w)  (((w) >> 16) & 0xffffUL)

#define PCIE_PREFETCH_BASE_LIMIT_VAL(pmemb, pmeml) \
	(((pmemb) & 0xffffUL) |			   \
	 (((pmeml) & 0xffffUL) << 16))

#define PCIE_PREFETCH_BASE_UPPER        10U

#define PCIE_PREFETCH_LIMIT_UPPER       11U

#define PCIE_IO_BASE_LIMIT_UPPER        12U

#define PCIE_IO_BASE_UPPER(w)   ((w) & 0xffffUL)
#define PCIE_IO_LIMIT_UPPER(w)  (((w) >> 16) & 0xffffUL)

#define PCIE_IO_BASE_LIMIT_UPPER_VAL(iobu, iolu) \
	(((iobu) & 0xffffUL) |			 \
	 (((iolu) & 0xffffUL) << 16))

/*
 * Word 15 contains information related to interrupts.
 *
 * We're only interested in the low byte, which is [supposed to be] set by
 * the firmware to indicate which wire IRQ the device interrupt is routed to.
 */

#define PCIE_CONF_INTR		15U

#define PCIE_CONF_INTR_IRQ(w)	((w) & 0xFFU)
#define PCIE_CONF_INTR_IRQ_NONE	0xFFU  /* no interrupt routed */

#define PCIE_MAX_BUS  (0xFFFFFFFFU & PCIE_BDF_BUS_MASK)
#define PCIE_MAX_DEV  (0xFFFFFFFFU & PCIE_BDF_DEV_MASK)
#define PCIE_MAX_FUNC (0xFFFFFFFFU & PCIE_BDF_FUNC_MASK)

/**
 * @brief Initialize an interrupt handler for a PCIe endpoint IRQ
 *
 * This routine is only meant to be used by drivers using PCIe bus and having
 * fixed or MSI based IRQ (so no runtime detection of the IRQ). In case
 * of runtime detection see pcie_connect_dynamic_irq()
 *
 * @param bdf_p PCIe endpoint BDF
 * @param irq_p IRQ line number.
 * @param priority_p Interrupt priority.
 * @param isr_p Address of interrupt service routine.
 * @param isr_param_p Parameter passed to interrupt service routine.
 * @param flags_p Architecture-specific IRQ configuration flags..
 */
#define PCIE_IRQ_CONNECT(bdf_p, irq_p, priority_p,			\
			 isr_p, isr_param_p, flags_p)			\
	ARCH_PCIE_IRQ_CONNECT(bdf_p, irq_p, priority_p,			\
			      isr_p, isr_param_p, flags_p)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_H_ */
