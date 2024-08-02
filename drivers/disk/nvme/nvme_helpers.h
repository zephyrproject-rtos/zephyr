/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2022 Intel Corp.
 */

#ifndef ZEPHYR_DRIVERS_DISK_NVME_NHME_HELPERS_H_
#define ZEPHYR_DRIVERS_DISK_NVME_NHME_HELPERS_H_

#define NVME_GONE		0xfffffffful

/*
 * Macros to deal with NVME revisions, as defined VS register
 */
#define NVME_REV(x, y)			(((x) << 16) | ((y) << 8))
#define NVME_MAJOR(r)			(((r) >> 16) & 0xffff)
#define NVME_MINOR(r)			(((r) >> 8) & 0xff)

/*
 * Use to mark a command to apply to all namespaces, or to retrieve global
 * log pages.
 */
#define NVME_GLOBAL_NAMESPACE_TAG	((uint32_t)0xFFFFFFFF)

/* Many items are expressed in terms of power of two times MPS */
#define NVME_MPS_SHIFT			12

/* Register field definitions */
#define NVME_CAP_LO_REG_MQES_SHIFT	(0)
#define NVME_CAP_LO_REG_MQES_MASK	(0xFFFF)
#define NVME_CAP_LO_REG_CQR_SHIFT	(16)
#define NVME_CAP_LO_REG_CQR_MASK	(0x1)
#define NVME_CAP_LO_REG_AMS_SHIFT	(17)
#define NVME_CAP_LO_REG_AMS_MASK	(0x3)
#define NVME_CAP_LO_REG_TO_SHIFT	(24)
#define NVME_CAP_LO_REG_TO_MASK		(0xFF)
#define NVME_CAP_LO_MQES(x)						\
	(((x) >> NVME_CAP_LO_REG_MQES_SHIFT) & NVME_CAP_LO_REG_MQES_MASK)
#define NVME_CAP_LO_CQR(x)						\
	(((x) >> NVME_CAP_LO_REG_CQR_SHIFT) & NVME_CAP_LO_REG_CQR_MASK)
#define NVME_CAP_LO_AMS(x)						\
	(((x) >> NVME_CAP_LO_REG_AMS_SHIFT) & NVME_CAP_LO_REG_AMS_MASK)
#define NVME_CAP_LO_TO(x)						\
	(((x) >> NVME_CAP_LO_REG_TO_SHIFT) & NVME_CAP_LO_REG_TO_MASK)

#define NVME_CAP_HI_REG_DSTRD_SHIFT	(0)
#define NVME_CAP_HI_REG_DSTRD_MASK	(0xF)
#define NVME_CAP_HI_REG_NSSRS_SHIFT	(4)
#define NVME_CAP_HI_REG_NSSRS_MASK	(0x1)
#define NVME_CAP_HI_REG_CSS_SHIFT	(5)
#define NVME_CAP_HI_REG_CSS_MASK	(0xff)
#define NVME_CAP_HI_REG_CSS_NVM_SHIFT	(5)
#define NVME_CAP_HI_REG_CSS_NVM_MASK	(0x1)
#define NVME_CAP_HI_REG_BPS_SHIFT	(13)
#define NVME_CAP_HI_REG_BPS_MASK	(0x1)
#define NVME_CAP_HI_REG_MPSMIN_SHIFT	(16)
#define NVME_CAP_HI_REG_MPSMIN_MASK	(0xF)
#define NVME_CAP_HI_REG_MPSMAX_SHIFT	(20)
#define NVME_CAP_HI_REG_MPSMAX_MASK	(0xF)
#define NVME_CAP_HI_REG_PMRS_SHIFT	(24)
#define NVME_CAP_HI_REG_PMRS_MASK	(0x1)
#define NVME_CAP_HI_REG_CMBS_SHIFT	(25)
#define NVME_CAP_HI_REG_CMBS_MASK	(0x1)
#define NVME_CAP_HI_DSTRD(x)						\
	(((x) >> NVME_CAP_HI_REG_DSTRD_SHIFT) & NVME_CAP_HI_REG_DSTRD_MASK)
#define NVME_CAP_HI_NSSRS(x)						\
	(((x) >> NVME_CAP_HI_REG_NSSRS_SHIFT) & NVME_CAP_HI_REG_NSSRS_MASK)
#define NVME_CAP_HI_CSS(x)						\
	(((x) >> NVME_CAP_HI_REG_CSS_SHIFT) & NVME_CAP_HI_REG_CSS_MASK)
#define NVME_CAP_HI_CSS_NVM(x)						\
	(((x) >> NVME_CAP_HI_REG_CSS_NVM_SHIFT) & NVME_CAP_HI_REG_CSS_NVM_MASK)
#define NVME_CAP_HI_BPS(x)						\
	(((x) >> NVME_CAP_HI_REG_BPS_SHIFT) & NVME_CAP_HI_REG_BPS_MASK)
#define NVME_CAP_HI_MPSMIN(x)						\
	(((x) >> NVME_CAP_HI_REG_MPSMIN_SHIFT) & NVME_CAP_HI_REG_MPSMIN_MASK)
#define NVME_CAP_HI_MPSMAX(x)						\
	(((x) >> NVME_CAP_HI_REG_MPSMAX_SHIFT) & NVME_CAP_HI_REG_MPSMAX_MASK)
#define NVME_CAP_HI_PMRS(x)						\
	(((x) >> NVME_CAP_HI_REG_PMRS_SHIFT) & NVME_CAP_HI_REG_PMRS_MASK)
#define NVME_CAP_HI_CMBS(x)						\
	(((x) >> NVME_CAP_HI_REG_CMBS_SHIFT) & NVME_CAP_HI_REG_CMBS_MASK)

#define NVME_CC_REG_EN_SHIFT		(0)
#define NVME_CC_REG_EN_MASK		(0x1)
#define NVME_CC_REG_CSS_SHIFT		(4)
#define NVME_CC_REG_CSS_MASK		(0x7)
#define NVME_CC_REG_MPS_SHIFT		(7)
#define NVME_CC_REG_MPS_MASK		(0xF)
#define NVME_CC_REG_AMS_SHIFT		(11)
#define NVME_CC_REG_AMS_MASK		(0x7)
#define NVME_CC_REG_SHN_SHIFT		(14)
#define NVME_CC_REG_SHN_MASK		(0x3)
#define NVME_CC_REG_IOSQES_SHIFT	(16)
#define NVME_CC_REG_IOSQES_MASK		(0xF)
#define NVME_CC_REG_IOCQES_SHIFT	(20)
#define NVME_CC_REG_IOCQES_MASK		(0xF)

#define NVME_CSTS_REG_RDY_SHIFT		(0)
#define NVME_CSTS_REG_RDY_MASK		(0x1)
#define NVME_CSTS_REG_CFS_SHIFT		(1)
#define NVME_CSTS_REG_CFS_MASK		(0x1)
#define NVME_CSTS_REG_SHST_SHIFT	(2)
#define NVME_CSTS_REG_SHST_MASK		(0x3)
#define NVME_CSTS_REG_NVSRO_SHIFT	(4)
#define NVME_CSTS_REG_NVSRO_MASK	(0x1)
#define NVME_CSTS_REG_PP_SHIFT		(5)
#define NVME_CSTS_REG_PP_MASK		(0x1)

#define NVME_CSTS_GET_SHST(csts)					\
	(((csts) >> NVME_CSTS_REG_SHST_SHIFT) & NVME_CSTS_REG_SHST_MASK)

#define NVME_AQA_REG_ASQS_SHIFT		(0)
#define NVME_AQA_REG_ASQS_MASK		(0xFFF)
#define NVME_AQA_REG_ACQS_SHIFT		(16)
#define NVME_AQA_REG_ACQS_MASK		(0xFFF)

#define NVME_PMRCAP_REG_RDS_SHIFT	(3)
#define NVME_PMRCAP_REG_RDS_MASK	(0x1)
#define NVME_PMRCAP_REG_WDS_SHIFT	(4)
#define NVME_PMRCAP_REG_WDS_MASK	(0x1)
#define NVME_PMRCAP_REG_BIR_SHIFT	(5)
#define NVME_PMRCAP_REG_BIR_MASK	(0x7)
#define NVME_PMRCAP_REG_PMRTU_SHIFT	(8)
#define NVME_PMRCAP_REG_PMRTU_MASK	(0x3)
#define NVME_PMRCAP_REG_PMRWBM_SHIFT	(10)
#define NVME_PMRCAP_REG_PMRWBM_MASK	(0xf)
#define NVME_PMRCAP_REG_PMRTO_SHIFT	(16)
#define NVME_PMRCAP_REG_PMRTO_MASK	(0xff)
#define NVME_PMRCAP_REG_CMSS_SHIFT	(24)
#define NVME_PMRCAP_REG_CMSS_MASK	(0x1)

#define NVME_PMRCAP_RDS(x)						\
	(((x) >> NVME_PMRCAP_REG_RDS_SHIFT) & NVME_PMRCAP_REG_RDS_MASK)
#define NVME_PMRCAP_WDS(x)						\
	(((x) >> NVME_PMRCAP_REG_WDS_SHIFT) & NVME_PMRCAP_REG_WDS_MASK)
#define NVME_PMRCAP_BIR(x)						\
	(((x) >> NVME_PMRCAP_REG_BIR_SHIFT) & NVME_PMRCAP_REG_BIR_MASK)
#define NVME_PMRCAP_PMRTU(x)						\
	(((x) >> NVME_PMRCAP_REG_PMRTU_SHIFT) & NVME_PMRCAP_REG_PMRTU_MASK)
#define NVME_PMRCAP_PMRWBM(x)						\
	(((x) >> NVME_PMRCAP_REG_PMRWBM_SHIFT) & NVME_PMRCAP_REG_PMRWBM_MASK)
#define NVME_PMRCAP_PMRTO(x)						\
	(((x) >> NVME_PMRCAP_REG_PMRTO_SHIFT) & NVME_PMRCAP_REG_PMRTO_MASK)
#define NVME_PMRCAP_CMSS(x)						\
	(((x) >> NVME_PMRCAP_REG_CMSS_SHIFT) & NVME_PMRCAP_REG_CMSS_MASK)

/* Command field definitions */

#define NVME_CMD_FUSE_SHIFT		(8)
#define NVME_CMD_FUSE_MASK		(0x3)

#define NVME_STATUS_P_SHIFT		(0)
#define NVME_STATUS_P_MASK		(0x1)
#define NVME_STATUS_SC_SHIFT		(1)
#define NVME_STATUS_SC_MASK		(0xFF)
#define NVME_STATUS_SCT_SHIFT		(9)
#define NVME_STATUS_SCT_MASK		(0x7)
#define NVME_STATUS_CRD_SHIFT		(12)
#define NVME_STATUS_CRD_MASK		(0x3)
#define NVME_STATUS_M_SHIFT		(14)
#define NVME_STATUS_M_MASK		(0x1)
#define NVME_STATUS_DNR_SHIFT		(15)
#define NVME_STATUS_DNR_MASK		(0x1)

#define NVME_STATUS_GET_P(st)					\
	(((st) >> NVME_STATUS_P_SHIFT) & NVME_STATUS_P_MASK)
#define NVME_STATUS_GET_SC(st)					\
	(((st) >> NVME_STATUS_SC_SHIFT) & NVME_STATUS_SC_MASK)
#define NVME_STATUS_GET_SCT(st)					\
	(((st) >> NVME_STATUS_SCT_SHIFT) & NVME_STATUS_SCT_MASK)
#define NVME_STATUS_GET_CRD(st)					\
	(((st) >> NVME_STATUS_CRD_SHIFT) & NVME_STATUS_CRD_MASK)
#define NVME_STATUS_GET_M(st)					\
	(((st) >> NVME_STATUS_M_SHIFT) & NVME_STATUS_M_MASK)
#define NVME_STATUS_GET_DNR(st)					\
	(((st) >> NVME_STATUS_DNR_SHIFT) & NVME_STATUS_DNR_MASK)

/** Controller Multi-path I/O and Namespace Sharing Capabilities */
/* More then one port */
#define NVME_CTRLR_DATA_MIC_MPORTS_SHIFT		(0)
#define NVME_CTRLR_DATA_MIC_MPORTS_MASK			(0x1)
/* More then one controller */
#define NVME_CTRLR_DATA_MIC_MCTRLRS_SHIFT		(1)
#define NVME_CTRLR_DATA_MIC_MCTRLRS_MASK		(0x1)
/* SR-IOV Virtual Function */
#define NVME_CTRLR_DATA_MIC_SRIOVVF_SHIFT		(2)
#define NVME_CTRLR_DATA_MIC_SRIOVVF_MASK		(0x1)
/* Asymmetric Namespace Access Reporting */
#define NVME_CTRLR_DATA_MIC_ANAR_SHIFT			(3)
#define NVME_CTRLR_DATA_MIC_ANAR_MASK			(0x1)

/** OAES - Optional Asynchronous Events Supported */
/* supports Namespace Attribute Notices event */
#define NVME_CTRLR_DATA_OAES_NS_ATTR_SHIFT		(8)
#define NVME_CTRLR_DATA_OAES_NS_ATTR_MASK		(0x1)
/* supports Firmware Activation Notices event */
#define NVME_CTRLR_DATA_OAES_FW_ACTIVATE_SHIFT		(9)
#define NVME_CTRLR_DATA_OAES_FW_ACTIVATE_MASK		(0x1)
/* supports Asymmetric Namespace Access Change Notices event */
#define NVME_CTRLR_DATA_OAES_ASYM_NS_CHANGE_SHIFT	(11)
#define NVME_CTRLR_DATA_OAES_ASYM_NS_CHANGE_MASK	(0x1)
/* supports Predictable Latency Event Aggregate Log Change Notices event */
#define NVME_CTRLR_DATA_OAES_PREDICT_LATENCY_SHIFT	(12)
#define NVME_CTRLR_DATA_OAES_PREDICT_LATENCY_MASK	(0x1)
/* supports LBA Status Information Notices event */
#define NVME_CTRLR_DATA_OAES_LBA_STATUS_SHIFT		(13)
#define NVME_CTRLR_DATA_OAES_LBA_STATUS_MASK		(0x1)
/* supports Endurance Group Event Aggregate Log Page Changes Notices event */
#define NVME_CTRLR_DATA_OAES_ENDURANCE_GROUP_SHIFT	(14)
#define NVME_CTRLR_DATA_OAES_ENDURANCE_GROUP_MASK	(0x1)
/* supports Normal NVM Subsystem Shutdown event */
#define NVME_CTRLR_DATA_OAES_NORMAL_SHUTDOWN_SHIFT	(15)
#define NVME_CTRLR_DATA_OAES_NORMAL_SHUTDOWN_MASK	(0x1)
/* supports Zone Descriptor Changed Notices event */
#define NVME_CTRLR_DATA_OAES_ZONE_DESC_CHANGE_SHIFT	(27)
#define NVME_CTRLR_DATA_OAES_ZONE_DESC_CHANGE_MASK	(0x1)
/* supports Discovery Log Page Change Notification event */
#define NVME_CTRLR_DATA_OAES_LOG_PAGE_CHANGE_SHIFT	(31)
#define NVME_CTRLR_DATA_OAES_LOG_PAGE_CHANGE_MASK	(0x1)

/** OACS - optional admin command support */
/* supports security send/receive commands */
#define NVME_CTRLR_DATA_OACS_SECURITY_SHIFT		(0)
#define NVME_CTRLR_DATA_OACS_SECURITY_MASK		(0x1)
/* supports format nvm command */
#define NVME_CTRLR_DATA_OACS_FORMAT_SHIFT		(1)
#define NVME_CTRLR_DATA_OACS_FORMAT_MASK		(0x1)
/* supports firmware activate/download commands */
#define NVME_CTRLR_DATA_OACS_FIRMWARE_SHIFT		(2)
#define NVME_CTRLR_DATA_OACS_FIRMWARE_MASK		(0x1)
/* supports namespace management commands */
#define NVME_CTRLR_DATA_OACS_NSMGMT_SHIFT		(3)
#define NVME_CTRLR_DATA_OACS_NSMGMT_MASK		(0x1)
/* supports Device Self-test command */
#define NVME_CTRLR_DATA_OACS_SELFTEST_SHIFT		(4)
#define NVME_CTRLR_DATA_OACS_SELFTEST_MASK		(0x1)
/* supports Directives */
#define NVME_CTRLR_DATA_OACS_DIRECTIVES_SHIFT		(5)
#define NVME_CTRLR_DATA_OACS_DIRECTIVES_MASK		(0x1)
/* supports NVMe-MI Send/Receive */
#define NVME_CTRLR_DATA_OACS_NVMEMI_SHIFT		(6)
#define NVME_CTRLR_DATA_OACS_NVMEMI_MASK		(0x1)
/* supports Virtualization Management */
#define NVME_CTRLR_DATA_OACS_VM_SHIFT			(7)
#define NVME_CTRLR_DATA_OACS_VM_MASK			(0x1)
/* supports Doorbell Buffer Config */
#define NVME_CTRLR_DATA_OACS_DBBUFFER_SHIFT		(8)
#define NVME_CTRLR_DATA_OACS_DBBUFFER_MASK		(0x1)
/* supports Get LBA Status */
#define NVME_CTRLR_DATA_OACS_GETLBA_SHIFT		(9)
#define NVME_CTRLR_DATA_OACS_GETLBA_MASK		(0x1)

/** firmware updates */
/* first slot is read-only */
#define NVME_CTRLR_DATA_FRMW_SLOT1_RO_SHIFT		(0)
#define NVME_CTRLR_DATA_FRMW_SLOT1_RO_MASK		(0x1)
/* number of firmware slots */
#define NVME_CTRLR_DATA_FRMW_NUM_SLOTS_SHIFT		(1)
#define NVME_CTRLR_DATA_FRMW_NUM_SLOTS_MASK		(0x7)
/* firmware activation without reset */
#define NVME_CTRLR_DATA_FRMW_ACT_WO_RESET_SHIFT		(4)
#define NVME_CTRLR_DATA_FRMW_ACT_WO_RESET_MASK		(0x1)

/** log page attributes */
/* per namespace smart/health log page */
#define NVME_CTRLR_DATA_LPA_NS_SMART_SHIFT		(0)
#define NVME_CTRLR_DATA_LPA_NS_SMART_MASK		(0x1)

/** AVSCC - admin vendor specific command configuration */
/* admin vendor specific commands use spec format */
#define NVME_CTRLR_DATA_AVSCC_SPEC_FORMAT_SHIFT		(0)
#define NVME_CTRLR_DATA_AVSCC_SPEC_FORMAT_MASK		(0x1)

/** Autonomous Power State Transition Attributes */
/* Autonomous Power State Transitions supported */
#define NVME_CTRLR_DATA_APSTA_APST_SUPP_SHIFT		(0)
#define NVME_CTRLR_DATA_APSTA_APST_SUPP_MASK		(0x1)

/** Sanitize Capabilities */
/* Crypto Erase Support  */
#define NVME_CTRLR_DATA_SANICAP_CES_SHIFT		(0)
#define NVME_CTRLR_DATA_SANICAP_CES_MASK		(0x1)
/* Block Erase Support */
#define NVME_CTRLR_DATA_SANICAP_BES_SHIFT		(1)
#define NVME_CTRLR_DATA_SANICAP_BES_MASK		(0x1)
/* Overwrite Support */
#define NVME_CTRLR_DATA_SANICAP_OWS_SHIFT		(2)
#define NVME_CTRLR_DATA_SANICAP_OWS_MASK		(0x1)
/* No-Deallocate Inhibited  */
#define NVME_CTRLR_DATA_SANICAP_NDI_SHIFT		(29)
#define NVME_CTRLR_DATA_SANICAP_NDI_MASK		(0x1)
/* No-Deallocate Modifies Media After Sanitize */
#define NVME_CTRLR_DATA_SANICAP_NODMMAS_SHIFT		(30)
#define NVME_CTRLR_DATA_SANICAP_NODMMAS_MASK		(0x3)
#define NVME_CTRLR_DATA_SANICAP_NODMMAS_UNDEF		(0)
#define NVME_CTRLR_DATA_SANICAP_NODMMAS_NO		(1)
#define NVME_CTRLR_DATA_SANICAP_NODMMAS_YES		(2)

/** submission queue entry size */
#define NVME_CTRLR_DATA_SQES_MIN_SHIFT			(0)
#define NVME_CTRLR_DATA_SQES_MIN_MASK			(0xF)
#define NVME_CTRLR_DATA_SQES_MAX_SHIFT			(4)
#define NVME_CTRLR_DATA_SQES_MAX_MASK			(0xF)

/** completion queue entry size */
#define NVME_CTRLR_DATA_CQES_MIN_SHIFT			(0)
#define NVME_CTRLR_DATA_CQES_MIN_MASK			(0xF)
#define NVME_CTRLR_DATA_CQES_MAX_SHIFT			(4)
#define NVME_CTRLR_DATA_CQES_MAX_MASK			(0xF)

/** optional nvm command support */
#define NVME_CTRLR_DATA_ONCS_COMPARE_SHIFT		(0)
#define NVME_CTRLR_DATA_ONCS_COMPARE_MASK		(0x1)
#define NVME_CTRLR_DATA_ONCS_WRITE_UNC_SHIFT		(1)
#define NVME_CTRLR_DATA_ONCS_WRITE_UNC_MASK		(0x1)
#define NVME_CTRLR_DATA_ONCS_DSM_SHIFT			(2)
#define NVME_CTRLR_DATA_ONCS_DSM_MASK			(0x1)
#define NVME_CTRLR_DATA_ONCS_WRZERO_SHIFT		(3)
#define NVME_CTRLR_DATA_ONCS_WRZERO_MASK		(0x1)
#define NVME_CTRLR_DATA_ONCS_SAVEFEAT_SHIFT		(4)
#define NVME_CTRLR_DATA_ONCS_SAVEFEAT_MASK		(0x1)
#define NVME_CTRLR_DATA_ONCS_RESERV_SHIFT		(5)
#define NVME_CTRLR_DATA_ONCS_RESERV_MASK		(0x1)
#define NVME_CTRLR_DATA_ONCS_TIMESTAMP_SHIFT		(6)
#define NVME_CTRLR_DATA_ONCS_TIMESTAMP_MASK		(0x1)
#define NVME_CTRLR_DATA_ONCS_VERIFY_SHIFT		(7)
#define NVME_CTRLR_DATA_ONCS_VERIFY_MASK		(0x1)

/** Fused Operation Support */
#define NVME_CTRLR_DATA_FUSES_CNW_SHIFT		(0)
#define NVME_CTRLR_DATA_FUSES_CNW_MASK		(0x1)

/** Format NVM Attributes */
#define NVME_CTRLR_DATA_FNA_FORMAT_ALL_SHIFT		(0)
#define NVME_CTRLR_DATA_FNA_FORMAT_ALL_MASK		(0x1)
#define NVME_CTRLR_DATA_FNA_ERASE_ALL_SHIFT		(1)
#define NVME_CTRLR_DATA_FNA_ERASE_ALL_MASK		(0x1)
#define NVME_CTRLR_DATA_FNA_CRYPTO_ERASE_SHIFT		(2)
#define NVME_CTRLR_DATA_FNA_CRYPTO_ERASE_MASK		(0x1)

/** volatile write cache */
/* volatile write cache present */
#define NVME_CTRLR_DATA_VWC_PRESENT_SHIFT		(0)
#define NVME_CTRLR_DATA_VWC_PRESENT_MASK		(0x1)
/* flush all namespaces supported */
#define NVME_CTRLR_DATA_VWC_ALL_SHIFT			(1)
#define NVME_CTRLR_DATA_VWC_ALL_MASK			(0x3)
#define NVME_CTRLR_DATA_VWC_ALL_UNKNOWN			(0)
#define NVME_CTRLR_DATA_VWC_ALL_NO			(2)
#define NVME_CTRLR_DATA_VWC_ALL_YES			(3)

/** namespace features */
/* thin provisioning */
#define NVME_NS_DATA_NSFEAT_THIN_PROV_SHIFT		(0)
#define NVME_NS_DATA_NSFEAT_THIN_PROV_MASK		(0x1)
/* NAWUN, NAWUPF, and NACWU fields are valid */
#define NVME_NS_DATA_NSFEAT_NA_FIELDS_SHIFT		(1)
#define NVME_NS_DATA_NSFEAT_NA_FIELDS_MASK		(0x1)
/* Deallocated or Unwritten Logical Block errors supported */
#define NVME_NS_DATA_NSFEAT_DEALLOC_SHIFT		(2)
#define NVME_NS_DATA_NSFEAT_DEALLOC_MASK		(0x1)
/* NGUID and EUI64 fields are not reusable */
#define NVME_NS_DATA_NSFEAT_NO_ID_REUSE_SHIFT		(3)
#define NVME_NS_DATA_NSFEAT_NO_ID_REUSE_MASK		(0x1)
/* NPWG, NPWA, NPDG, NPDA, and NOWS are valid */
#define NVME_NS_DATA_NSFEAT_NPVALID_SHIFT		(4)
#define NVME_NS_DATA_NSFEAT_NPVALID_MASK		(0x1)

/** formatted lba size */
#define NVME_NS_DATA_FLBAS_FORMAT_SHIFT			(0)
#define NVME_NS_DATA_FLBAS_FORMAT_MASK			(0xF)
#define NVME_NS_DATA_FLBAS_EXTENDED_SHIFT		(4)
#define NVME_NS_DATA_FLBAS_EXTENDED_MASK		(0x1)

/** metadata capabilities */
/* metadata can be transferred as part of data prp list */
#define NVME_NS_DATA_MC_EXTENDED_SHIFT			(0)
#define NVME_NS_DATA_MC_EXTENDED_MASK			(0x1)
/* metadata can be transferred with separate metadata pointer */
#define NVME_NS_DATA_MC_POINTER_SHIFT			(1)
#define NVME_NS_DATA_MC_POINTER_MASK			(0x1)

/** end-to-end data protection capabilities */
/* protection information type 1 */
#define NVME_NS_DATA_DPC_PIT1_SHIFT			(0)
#define NVME_NS_DATA_DPC_PIT1_MASK			(0x1)
/* protection information type 2 */
#define NVME_NS_DATA_DPC_PIT2_SHIFT			(1)
#define NVME_NS_DATA_DPC_PIT2_MASK			(0x1)
/* protection information type 3 */
#define NVME_NS_DATA_DPC_PIT3_SHIFT			(2)
#define NVME_NS_DATA_DPC_PIT3_MASK			(0x1)
/* first eight bytes of metadata */
#define NVME_NS_DATA_DPC_MD_START_SHIFT			(3)
#define NVME_NS_DATA_DPC_MD_START_MASK			(0x1)
/* last eight bytes of metadata */
#define NVME_NS_DATA_DPC_MD_END_SHIFT			(4)
#define NVME_NS_DATA_DPC_MD_END_MASK			(0x1)

/** end-to-end data protection type settings */
/* protection information type */
#define NVME_NS_DATA_DPS_PIT_SHIFT			(0)
#define NVME_NS_DATA_DPS_PIT_MASK			(0x7)
/* 1 == protection info transferred at start of metadata */
/* 0 == protection info transferred at end of metadata */
#define NVME_NS_DATA_DPS_MD_START_SHIFT			(3)
#define NVME_NS_DATA_DPS_MD_START_MASK			(0x1)

/** Namespace Multi-path I/O and Namespace Sharing Capabilities */
/* the namespace may be attached to two or more controllers */
#define NVME_NS_DATA_NMIC_MAY_BE_SHARED_SHIFT		(0)
#define NVME_NS_DATA_NMIC_MAY_BE_SHARED_MASK		(0x1)

/** Reservation Capabilities */
/* Persist Through Power Loss */
#define NVME_NS_DATA_RESCAP_PTPL_SHIFT		(0)
#define NVME_NS_DATA_RESCAP_PTPL_MASK		(0x1)
/* supports the Write Exclusive */
#define NVME_NS_DATA_RESCAP_WR_EX_SHIFT		(1)
#define NVME_NS_DATA_RESCAP_WR_EX_MASK		(0x1)
/* supports the Exclusive Access */
#define NVME_NS_DATA_RESCAP_EX_AC_SHIFT		(2)
#define NVME_NS_DATA_RESCAP_EX_AC_MASK		(0x1)
/* supports the Write Exclusive – Registrants Only */
#define NVME_NS_DATA_RESCAP_WR_EX_RO_SHIFT	(3)
#define NVME_NS_DATA_RESCAP_WR_EX_RO_MASK	(0x1)
/* supports the Exclusive Access - Registrants Only */
#define NVME_NS_DATA_RESCAP_EX_AC_RO_SHIFT	(4)
#define NVME_NS_DATA_RESCAP_EX_AC_RO_MASK	(0x1)
/* supports the Write Exclusive – All Registrants */
#define NVME_NS_DATA_RESCAP_WR_EX_AR_SHIFT	(5)
#define NVME_NS_DATA_RESCAP_WR_EX_AR_MASK	(0x1)
/* supports the Exclusive Access - All Registrants */
#define NVME_NS_DATA_RESCAP_EX_AC_AR_SHIFT	(6)
#define NVME_NS_DATA_RESCAP_EX_AC_AR_MASK	(0x1)
/* Ignore Existing Key is used as defined in revision 1.3 or later */
#define NVME_NS_DATA_RESCAP_IEKEY13_SHIFT	(7)
#define NVME_NS_DATA_RESCAP_IEKEY13_MASK	(0x1)

/** Format Progress Indicator */
/* percentage of the Format NVM command that remains to be completed */
#define NVME_NS_DATA_FPI_PERC_SHIFT		(0)
#define NVME_NS_DATA_FPI_PERC_MASK		(0x7f)
/* namespace supports the Format Progress Indicator */
#define NVME_NS_DATA_FPI_SUPP_SHIFT		(7)
#define NVME_NS_DATA_FPI_SUPP_MASK		(0x1)

/** Deallocate Logical Block Features */
/* deallocated logical block read behavior */
#define NVME_NS_DATA_DLFEAT_READ_SHIFT		(0)
#define NVME_NS_DATA_DLFEAT_READ_MASK		(0x07)
#define NVME_NS_DATA_DLFEAT_READ_NR		(0x00)
#define NVME_NS_DATA_DLFEAT_READ_00		(0x01)
#define NVME_NS_DATA_DLFEAT_READ_FF		(0x02)
/* supports the Deallocate bit in the Write Zeroes */
#define NVME_NS_DATA_DLFEAT_DWZ_SHIFT		(3)
#define NVME_NS_DATA_DLFEAT_DWZ_MASK		(0x01)
/* Guard field for deallocated logical blocks is set to the CRC  */
#define NVME_NS_DATA_DLFEAT_GCRC_SHIFT		(4)
#define NVME_NS_DATA_DLFEAT_GCRC_MASK		(0x01)

/** lba format support */
/* metadata size */
#define NVME_NS_DATA_LBAF_MS_SHIFT		(0)
#define NVME_NS_DATA_LBAF_MS_MASK		(0xFFFF)
/* lba data size */
#define NVME_NS_DATA_LBAF_LBADS_SHIFT		(16)
#define NVME_NS_DATA_LBAF_LBADS_MASK		(0xFF)
/* relative performance */
#define NVME_NS_DATA_LBAF_RP_SHIFT		(24)
#define NVME_NS_DATA_LBAF_RP_MASK		(0x3)

enum nvme_critical_warning_state {
	NVME_CRIT_WARN_ST_AVAILABLE_SPARE		= 0x1,
	NVME_CRIT_WARN_ST_TEMPERATURE			= 0x2,
	NVME_CRIT_WARN_ST_DEVICE_RELIABILITY		= 0x4,
	NVME_CRIT_WARN_ST_READ_ONLY			= 0x8,
	NVME_CRIT_WARN_ST_VOLATILE_MEMORY_BACKUP	= 0x10,
};
#define NVME_CRIT_WARN_ST_RESERVED_MASK		(0xE0)
#define	NVME_ASYNC_EVENT_NS_ATTRIBUTE		(0x100)
#define	NVME_ASYNC_EVENT_FW_ACTIVATE		(0x200)


/* Helper macro to combine *_MASK and *_SHIFT defines */
#define NVMEB(name)	(name##_MASK << name##_SHIFT)

/* CC register SHN field values */
enum shn_value {
	NVME_SHN_NORMAL		= 0x1,
	NVME_SHN_ABRUPT		= 0x2,
};

/* CSTS register SHST field values */
enum shst_value {
	NVME_SHST_NORMAL	= 0x0,
	NVME_SHST_OCCURRING	= 0x1,
	NVME_SHST_COMPLETE	= 0x2,
};

#define nvme_mmio_offsetof(reg)						\
	offsetof(struct nvme_registers, reg)

#define nvme_mmio_read_4(b_a, reg)					\
	sys_read32((mm_reg_t)b_a +  nvme_mmio_offsetof(reg))

#define nvme_mmio_write_4(b_a, reg, val)				\
	sys_write32(val, (mm_reg_t)b_a + nvme_mmio_offsetof(reg))

#define nvme_mmio_write_8(b_a, reg, val)				\
	do {								\
		sys_write32(val & 0xFFFFFFFF,				\
			    (mm_reg_t)b_a + nvme_mmio_offsetof(reg));	\
		sys_write32((val & 0xFFFFFFFF00000000ULL) >> 32,	\
			    (mm_reg_t)b_a + nvme_mmio_offsetof(reg) + 4); \
	} while (0)

#define NVME_IS_BUFFER_DWORD_ALIGNED(_buf_addr) (!((uintptr_t)_buf_addr & 0x3))

#endif /* ZEPHYR_DRIVERS_DISK_NVME_NHME_HELPERS_H_ */
