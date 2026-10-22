/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_CAP_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_CAP_H_

/**
 * @file
 * @brief PCIe Capabilities
 * @defgroup pcie_capabilities PCIe Capabilities
 * @ingroup pcie_host_interface
 * @{
 */

/**
 * @name PCI & PCI Express Capabilities
 *
 * From PCI Code and ID Assignment Specification Revision 1.11
 * @{
 */
#define PCI_CAP_ID_NULL		0x00U	/**< Null Capability */
#define PCI_CAP_ID_PM		0x01U	/**< Power Management */
#define PCI_CAP_ID_AGP		0x02U	/**< Accelerated Graphics Port */
#define PCI_CAP_ID_VPD		0x03U	/**< Vital Product Data */
#define PCI_CAP_ID_SLOTID	0x04U	/**< Slot Identification */
#define PCI_CAP_ID_MSI		0x05U	/**< Message Signalled Interrupts */
#define PCI_CAP_ID_CHSWP	0x06U	/**< CompactPCI HotSwap */
#define PCI_CAP_ID_PCIX		0x07U	/**< PCI-X */
#define PCI_CAP_ID_HT		0x08U	/**< HyperTransport */
#define PCI_CAP_ID_VNDR		0x09U	/**< Vendor-Specific */
#define PCI_CAP_ID_DBG		0x0AU	/**< Debug port */
#define PCI_CAP_ID_CCRC		0x0BU	/**< CompactPCI Central Resource Control */
#define PCI_CAP_ID_SHPC		0x0CU	/**< PCI Standard Hot-Plug Controller */
#define PCI_CAP_ID_SSVID	0x0DU	/**< Bridge subsystem vendor/device ID */
#define PCI_CAP_ID_AGP3		0x0EU	/**< AGP 8x */
#define PCI_CAP_ID_SECDEV	0x0FU	/**< Secure Device */
#define PCI_CAP_ID_EXP		0x10U	/**< PCI Express */
#define PCI_CAP_ID_MSIX		0x11U	/**< MSI-X */
#define PCI_CAP_ID_SATA		0x12U	/**< Serial ATA Data/Index Configuration */
#define PCI_CAP_ID_AF		0x13U	/**< PCI Advanced Features */
#define PCI_CAP_ID_EA		0x14U	/**< PCI Enhanced Allocation */
#define PCI_CAP_ID_FPB		0x14U	/**< Flattening Portal Bridge */
/**
 * @}
 */

/**
 * @name PCI Express Extended Capabilities
 * @{
 */

#define PCIE_EXT_CAP_ID_NULL	0x0000U	/**< Null Capability */
#define PCIE_EXT_CAP_ID_ERR	0x0001U	/**< Advanced Error Reporting */
#define PCIE_EXT_CAP_ID_VC	0x0002U	/**< Virtual Channel when no MFVC */
#define PCIE_EXT_CAP_ID_DSN	0x0003U	/**< Device Serial Number */
#define PCIE_EXT_CAP_ID_PWR	0x0004U	/**< Power Budgeting */
#define PCIE_EXT_CAP_ID_RCLD	0x0005U	/**< Root Complex Link Declaration */
#define PCIE_EXT_CAP_ID_RCILC	0x0006U	/**< Root Complex Internal Link Control */
#define PCIE_EXT_CAP_ID_RCEC	0x0007U	/**< Root Complex Event Collector Endpoint Association */
#define PCIE_EXT_CAP_ID_MFVC	0x0008U	/**< Multi-Function VC Capability */
#define PCIE_EXT_CAP_ID_MFVC_VC	0x0009U	/**< Virtual Channel used with MFVC */
#define PCIE_EXT_CAP_ID_RCRB	0x000AU	/**< Root Complex Register Block */
#define PCIE_EXT_CAP_ID_VNDR	0x000BU	/**< Vendor-Specific Extended Capability */
#define PCIE_EXT_CAP_ID_CAC	0x000CU	/**< Config Access Correlation - obsolete */
#define PCIE_EXT_CAP_ID_ACS	0x000DU	/**< Access Control Services */
#define PCIE_EXT_CAP_ID_ARI	0x000EU	/**< Alternate Routing-ID Interpretation */
#define PCIE_EXT_CAP_ID_ATS	0x000FU	/**< Address Translation Services */
#define PCIE_EXT_CAP_ID_SRIOV	0x0010U	/**< Single Root I/O Virtualization */
#define PCIE_EXT_CAP_ID_MRIOV	0x0011U	/**< Multi Root I/O Virtualization */
#define PCIE_EXT_CAP_ID_MCAST	0x0012U	/**< Multicast */
#define PCIE_EXT_CAP_ID_PRI	0x0013U	/**< Page Request Interface */
#define PCIE_EXT_CAP_ID_AMD_XXX	0x0014U	/**< Reserved for AMD */
#define PCIE_EXT_CAP_ID_REBAR	0x0015U	/**< Resizable BAR */
#define PCIE_EXT_CAP_ID_DPA	0x0016U	/**< Dynamic Power Allocation */
#define PCIE_EXT_CAP_ID_TPH	0x0017U	/**< TPH Requester */
#define PCIE_EXT_CAP_ID_LTR	0x0018U	/**< Latency Tolerance Reporting */
#define PCIE_EXT_CAP_ID_SECPCI	0x0019U	/**< Secondary PCIe Capability */
#define PCIE_EXT_CAP_ID_PMUX	0x001AU	/**< Protocol Multiplexing */
#define PCIE_EXT_CAP_ID_PASID	0x001BU	/**< Process Address Space ID */
#define PCIE_EXT_CAP_ID_DPC	0x001DU	/**< DPC: Downstream Port Containment */
#define PCIE_EXT_CAP_ID_L1SS	0x001EU	/**< L1 PM Substates */
#define PCIE_EXT_CAP_ID_PTM	0x001FU	/**< Precision Time Measurement */
#define PCIE_EXT_CAP_ID_DVSEC	0x0023U	/**< Designated Vendor-Specific Extended Capability */
#define PCIE_EXT_CAP_ID_DLF	0x0025U	/**< Data Link Feature */
#define PCIE_EXT_CAP_ID_PL_16GT	0x0026U	/**< Physical Layer 16.0 GT/s */
#define PCIE_EXT_CAP_ID_LMR	0x0027U	/**< Lane Margining at the Receiver */
#define PCIE_EXT_CAP_ID_HID	0x0028U	/**< Hierarchy ID */
#define PCIE_EXT_CAP_ID_NPEM	0x0029U	/**< Native PCIe Enclosure Management */
#define PCIE_EXT_CAP_ID_PL_32GT	0x002AU	/**< Physical Layer 32.0 GT/s */
#define PCIE_EXT_CAP_ID_AP	0x002BU	/**< Alternate Protocol */
#define PCIE_EXT_CAP_ID_SFI	0x002CU	/**< System Firmware Intermediary */
/**
 * @}
 */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_CAP_H_ */
