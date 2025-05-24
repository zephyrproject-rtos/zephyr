/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_UFS_UFS_H_
#define ZEPHYR_INCLUDE_UFS_UFS_H_

#include <zephyr/kernel.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/sys/sys_io.h>

/* MACROS */

/* Maximum size for UFS Host Controller Query Descriptor */
#define UFSHC_QRY_DESC_MAX_SIZE	(255U)

/* Maximum number of Physical Region Descriptor Table entries */
#define UFSHC_PRDT_ENTRIES	(128U)

/* Maximum Logical Unit Numbers supported by UFS controller */
#define UFSHC_MAX_LUNS		(32U)

/* Maximum length of Command Descriptor Block */
#define UFSHC_MAX_CDB_LEN	(16U)

/* Maximum number of link-startup retries */
#define UFSHC_DME_LINKSTARTUP_RETRIES	(3)

/* Timeout in microseconds used in UFS operations */
#define UFS_TIMEOUT_US	(1000000U)

/* Host Controller Enable Register (0x34h) */
#define UFSHC_HCE_MASK	(0x1U)

/* Host Controller Status Register (0x30h) */
#define UFSHC_HCS_DP_MASK	(0x1U)
#define UFSHC_HCS_UTRLRDY_MASK	(0x2U)
#define UFSHC_HCS_UCRDY_MASK	(0x8U)
#define UFSHC_HCS_UPMCRS_MASK	(0x700U)
#define UFSHC_HCS_CCS_MASK	(0x800U)

#define UFSHCD_STATUS_READY	(UFSHC_HCS_UTRLRDY_MASK | UFSHC_HCS_UCRDY_MASK)

/* Interrupt Status Register (0x20h) */
#define UFSHC_IS_UTRCS_MASK	(0x1U)
#define UFSHC_IS_UE_MASK	(0x4U)
#define UFSHC_IS_PWR_STS_MASK	(0x10U)
#define UFSHC_IS_ULSS_MASK	(0x100U)
#define UFSHC_IS_UCCS_MASK	(0x400U)

#define UFSHCD_UIC_MASK		(UFSHC_IS_UCCS_MASK | UFSHC_IS_PWR_STS_MASK)

#define UFSHCD_ERROR_MASK	(UFSHC_IS_UE_MASK)

#define UFSHCD_ENABLE_INTRS	(UFSHC_IS_UTRCS_MASK | UFSHCD_ERROR_MASK)

#define UFSHC_UCMDARG2_RESCODE_MASK	GENMASK(7, 0)

/* UTP Transfer Request Run-Stop Register (0x60h) */
#define UFSHC_UTRL_RUN	(0x1U)

#define UFSHC_256KB	(0x40000U)

/* Events for UFS interrupt */
#define UFS_UIC_CMD_COMPLETION_EVENT	BIT(0)
#define UFS_UPIU_COMPLETION_EVENT	BIT(1)
#define UFS_UIC_PWR_COMPLETION_EVENT	BIT(2)

/* UFS Host Controller Registers */
#define UFSHC_HOST_CTRL_CAP	0x00U
#define UFSHC_IS		0x20U
#define UFSHC_IE		0x24U
#define UFSHC_HCS		0x30U
#define UFSHC_HCE		0x34U
#define UFSHC_UTRLBA		0x50U
#define UFSHC_UTRLBAU		0x54U
#define UFSHC_UTRLDBR		0x58U
#define UFSHC_UTRLRSR		0x60U
#define UFSHC_UICCMD		0x90U
#define UFSHC_UCMDARG1		0x94U
#define UFSHC_UCMDARG2		0x98U
#define UFSHC_UCMDARG3		0x9CU

/* Controller capability masks */
#define UFSHC_TRANSFER_REQ_SLOT_MASK	GENMASK(4, 0)

/* DME Operations supported */
#define UFSHC_DME_GET_OPCODE		0x01U
#define UFSHC_DME_SET_OPCODE		0x02U
#define UFSHC_DME_LINKSTARTUP_OPCODE	0x16U

/* Transaction Codes */
#define UFSHC_NOP_UPIU_TRANS_CODE	0x00U
#define UFSHC_CMD_UPIU_TRANS_CODE	0x01U
#define UFSHC_TSK_UPIU_TRANS_CODE	0x04U
#define UFSHC_QRY_UPIU_TRANS_CODE	0x16U

/* UPIU Read/Write flags */
#define UFSHC_UPIU_FLAGS_WRITE	0x20U
#define UFSHC_UPIU_FLAGS_READ	0x40U

/* Query function */
#define UFSHC_QRY_READ	0x01U
#define UFSHC_QRY_WRITE	0x81U

/* Flag IDN for Query Requests*/
#define UFSHC_FDEVINIT_FLAG_IDN	0x01U

/* Attribute IDN for Query Requests */
#define UFSHC_BLUEN_ATTRID	0x00U

/* Descriptor IDN for Query Requests */
#define UFSHC_DEVICE_DESC_IDN	0x0U
#define UFSHC_CONFIG_DESC_IDN	0x1U
#define UFSHC_UNIT_DESC_IDN	0x2U
#define UFSHC_GEOMETRY_DESC_IDN	0x7U

/* Unit Descriptor Parameters Offsets (bytes) */
#define UFSHC_UD_PARAM_UNIT_INDEX	0x2U
#define UFSHC_UD_PARAM_LU_ENABLE	0x3U
#define UFSHC_UD_PARAM_LOGICAL_BLKSZ	0xAU
#define UFSHC_UD_PARAM_LOGICAL_BLKCNT	0xBU

/* Geometry Descriptor Parameters Offsets (bytes) */
#define UFSHC_GEO_DESC_PARAM_MAX_NUM_LUN	0xCU

/* UTP QUERY Transaction Specific Fields OpCode */
#define UFSHC_QRY_READ_DESC_CMD		0x1U
#define UFSHC_QRY_WRITE_DESC_CMD	0x2U
#define UFSHC_QRY_READ_ATTR_CMD		0x3U
#define UFSHC_QRY_WRITE_ATTR_CMD	0x4U
#define UFSHC_QRY_READ_FLAG_CMD		0x5U
#define UFSHC_QRY_SET_FLAG_CMD		0x6U
#define UFSHC_QRY_CLR_FLAG_CMD		0x7U

/* UTP Transfer Request Descriptor definitions */
#define UFSHC_CT_UFS_STORAGE_MASK	0x10000000U
#define UFSHC_INTERRUPT_CMD_MASK	0x01000000U
#define UFSHC_DD_DEV_TO_MEM_MASK	0x04000000U
#define UFSHC_DD_MEM_TO_DEV_MASK	0x02000000U

/* Speed Gears */

/* PWM */
#define UFSHC_PWM_G1	0x2201U
#define UFSHC_PWM_G2	0x2202U
#define UFSHC_PWM_G3	0x2203U
#define UFSHC_PWM_G4	0x2204U

/* HS RATE-A */
#define UFSHC_HS_G1	0x11101U
#define UFSHC_HS_G2	0x11102U
#define UFSHC_HS_G3	0x11103U
#define UFSHC_HS_G4	0x11104U

/* HS RATE-B */
#define UFSHC_HS_G1_B	0x21101U
#define UFSHC_HS_G2_B	0x21102U
#define UFSHC_HS_G3_B	0x21103U
#define UFSHC_HS_G4_B	0x21104U

#define UFSHC_TX_RX_FAST	0x11U
#define UFSHC_TX_RX_SLOW	0x22U

#define UFSHC_HSSERIES_A	1U
#define UFSHC_HSSERIES_B	2U

#define UFSHC_GEAR4		4U

/* Power Mode change request status */
#define UFSHC_PWR_OK		0U
#define UFSHC_PWR_LOCAL		1U
#define UFSHC_PWR_ERR_CAP	4U
#define UFSHC_PWR_FATAL_ERR	5U

#define UFSHC_PWR_MODE_VAL	0x100U

/* ENUMS */
enum ChangeStage {
	PRE_CHANGE,
	POST_CHANGE,
};

/* Overall command status values */
enum CommandStatus {
	OCS_SUCCESS                 = 0x0,
	OCS_INVALID_CMD_TABLE_ATTR  = 0x1,
	OCS_INVALID_PRDT_ATTR       = 0x2,
	OCS_MISMATCH_DATA_BUF_SIZE  = 0x3,
	OCS_MISMATCH_RESP_UPIU_SIZE = 0x4,
	OCS_PEER_COMM_FAILURE       = 0x5,
	OCS_ABORTED                 = 0x6,
	OCS_FATAL_ERROR             = 0x7,
	OCS_DEVICE_FATAL_ERROR      = 0x8,
	OCS_INVALID_CRYPTO_CONFIG   = 0x9,
	OCS_GENERAL_CRYPTO_ERROR    = 0xA,
	OCS_INVALID_COMMAND_STATUS  = 0x0F,
};

/* STRUCTURES */

/**
 * @brief UIC Command Structure
 *
 * This structure represents the UIC (Unified Host Controller Interface) command
 * that is used for various operations in the UFS Host Controller.
 */
struct ufshc_uic_cmd {
	uint8_t command;        /**< Command identifier. */
	uint8_t attr_set_type;  /**< Attribute set type. */
	uint8_t reset_level;    /**< Reset level. */
	uint8_t result_code;    /**< Result of the command execution. */
	uint32_t mib_value;     /**< MIB (Management Information Base) value. */
	uint32_t gen_sel_index; /**< Generic selector index. */
	uint16_t mib_attribute; /**< MIB attribute. */
} __packed;

/**
 * @brief Physical Region Descriptor Table
 *
 * This structure describes a physical region descriptor used in data transfer
 * operations. It contains the address and byte count of a data block being
 * transferred, aligned to an 8-byte boundary.
 */
struct ufshc_xfer_prdt {
	uint32_t bufaddr_lower;  /**< Lower 32 bits of the physical address of the data block. */
	uint32_t bufaddr_upper;  /**< Upper 32 bits of the physical address of the data block. */
	uint32_t reserved;       /**< Reserved for future use. */
	uint32_t data_bytecount; /**< Byte count of the data block. */
} __packed;

/**
 * @brief UTP Transfer Request Descriptor Structure
 */
struct ufshc_xfer_req_desc {
	uint32_t dw0_config; /**< Configuration bits for UTP transfer. */
	uint32_t dw1_dunl; /**< Lower 32 bits of Data Unit Number (DUN). */
	uint32_t dw2_ocs; /**< Overall Command Status (OCS). */
	uint32_t dw3_dunu; /**< Upper 32 bits of Data Unit Number (DUN). */
	uint32_t dw4_utp_cmd_desc_base_addr_lo; /**< Lower 32 bits of the UTP Command Desc base. */
	uint32_t dw5_utp_cmd_desc_base_addr_up; /**< Upper 32 bits of the UTP Command Desc base. */
	uint32_t dw6_resp_upiu_info; /**< [31:16] - Resp UPIU Offset, [15:0] - Resp UPIU Length */
	uint32_t dw7_prdt_info; /**< [31:16] - PRDT Offset, [15:0] - PRDT Length */
} __packed;

/**
 * @brief UPIU header structure
 */
struct ufshc_upiu_header {
	uint8_t transaction_type;   /**< Type of the transaction (e.g., command, response) */
	uint8_t flags;              /**< Various flags (e.g., task management, response expected) */
	uint8_t lun;                /**< Logical Unit Number (LUN) */
	uint8_t task_tag;           /**< Task tag associated with the UPIU */
	uint8_t iid_cmd_type;       /**< Initiator Command Type */
	uint8_t query_task_mang_fn; /**< Task Management Function (for query UPIUs) */
	uint8_t response;           /**< Response code for the transaction */
	uint8_t status;             /**< Transaction status (e.g., success, failure) */
	uint8_t total_ehs_len;      /**< Length of the Extended Header Segment (EHS) */
	uint8_t device_info;        /**< Device-specific information */
	uint16_t data_segment_len;  /**< Length of the data segment (in Big Endian format) */
} __packed;

/**
 * @brief Command UPIU Structure
 */
struct ufshc_cmd_upiu {
	uint32_t exp_data_xfer_len;          /**< Expected data transfer length for the command */
	uint8_t scsi_cdb[UFSHC_MAX_CDB_LEN]; /**< SCSI Command Descriptor Block (CDB) */
} __packed;

/**
 * @brief Transaction Specific Fields Structure
 */
struct ufshc_trans_spec_flds {
	uint8_t opcode;     /**< Operation code for the transaction */
	uint8_t desc_id;    /**< Descriptor ID */
	uint8_t index;      /**< Index for the specific transaction */
	uint8_t selector;   /**< Selector value */
	uint16_t reserved0; /**< Reserved field for padding or future use */
	uint16_t length;    /**< Length of the transaction (Big Endian format) */
	uint32_t value;     /**< Value associated with the transaction (Big Endian format) */
	uint32_t reserved1; /**< Reserved field for padding or future use */
} __packed;

/**
 * @brief Query UPIU Structure
 */
struct ufshc_query_upiu {
	struct ufshc_trans_spec_flds tsf; /**< Transaction specific fields for the query */
	uint32_t reserved;                /**< Reserved field for future use */
	uint8_t data[256];                /**< Query-specific data */
} __packed;

/**
 * @brief NOP IN/OUT UPIU Structure
 */
struct ufshc_nop_upiu {
	uint8_t reserved[20]; /**< Reserved for future use */
} __packed;

/**
 * @brief UTP Transfer Request UPIU Structure
 */
struct ufshc_xfer_req_upiu {
	struct ufshc_upiu_header upiu_header;           /**< UPIU header structure */
	union {
		struct ufshc_cmd_upiu	cmd_upiu;       /**< Command UPIU structure */
		struct ufshc_query_upiu	query_req_upiu; /**< Query UPIU structure */
		struct ufshc_nop_upiu	nop_out_upiu;   /**< NOP OUT UPIU structure */
	};
} __packed;

/**
 * @brief Response UPIU Structure
 */
struct ufshc_resp_upiu {
	uint32_t residual_trans_count; /**< Residual transfer count (Big Endian format) */
	uint32_t reserved[4];          /**< Reserved fields */
	uint16_t sense_data_len;       /**< Sense data length (Big Endian format) */
	uint8_t sense_data[20];        /**< Sense data (if any) */
} __packed;

/**
 * @brief UTP Transfer Response UPIU Structure
 */
struct ufshc_xfer_resp_upiu {
	struct ufshc_upiu_header upiu_header;            /**< UPIU header structure */
	union {
		struct ufshc_resp_upiu	resp_upiu;       /**< Response UPIU structure */
		struct ufshc_query_upiu	query_resp_upiu; /**< Query response UPIU structure */
		struct ufshc_nop_upiu	nop_in_upiu;     /**< NOP IN UPIU structure */
	};
} __packed;

/**
 * @brief UTP Command Request Descriptor Structure
 */
struct ufshc_xfer_cmd_desc {
	struct ufshc_xfer_req_upiu req_upiu;
	/**< UTP Transfer Request UPIU */
	struct ufshc_xfer_resp_upiu resp_upiu __aligned(8);
	/**< UTP Transfer Response UPIU (aligned to 8 bytes) */
	struct ufshc_xfer_prdt prdt[UFSHC_PRDT_ENTRIES] __aligned(8);
	/**< PRDT (Physical Region Descriptor Table) (aligned to 8 bytes) */
} __packed;

/**
 * @brief UFS Device Information Structure
 */
struct ufs_dev_info {
	uint8_t max_lu_supported;            /**< Maximum number of Logical Units supported */
	struct lun_info	lun[UFSHC_MAX_LUNS]; /**< Array of LUN information structures */
};

/**
 * @brief UFS host controller private structure
 */
struct ufs_host_controller {
	struct device *dev; /**< Pointer to the driver device handle. */
	mem_addr_t mmio_base; /**< Base address for the UFSHCI memory-mapped I/O registers. */
	bool is_initialized; /**< Flag indicating if the host controller has been initialized. */
	uint32_t irq; /**< IRQ number for the UFS host controller. */
	uint8_t is_cache_coherent; /**< UFS controller supports cache coherency. */
	struct ufshc_xfer_cmd_desc *ucdl_base_addr; /**< UFS Command Desc base address */
	struct ufshc_xfer_req_desc *utrdl_base_addr; /**< UTP Transfer Request Desc base address */
	uint32_t xfer_req_depth; /**< Maximum depth of the Transfer Request Queue supported */
	uint32_t outstanding_xfer_reqs; /**< Outstanding transfer requests. */
	struct k_event irq_event; /**< Event object used to signal interrupt handling. */
	struct k_mutex ufs_lock; /**< Mutex to synchronize access to UFS driver resources. */
	struct ufs_dev_info dev_info; /**< Structure holding information about the UFS device. */
	struct scsi_host_info *host; /**< SCSI host associated with the UFS controller. */
};

/* INLINE FUNCTIONS */

/**
 * @brief Write a 32-bit value to a register in UFS host controller
 *
 * This function writes a 32-bit data to a specific register in the UFS host controller's
 * memory-mapped I/O space.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param reg_offset Offset from the base address of the register to write to
 * @param data Data to be written to the register
 */
static inline void ufshc_write_reg(const struct ufs_host_controller *ufshc,
				   uintptr_t reg_offset,
				   uint32_t data)
{
	sys_write32((uint32_t)(data), ((uintptr_t)((ufshc)->mmio_base) + (uintptr_t)(reg_offset)));
}

/**
 * @brief Read a 32-bit value from a register in UFS host controller
 *
 * This function reads a 32-bit value from a specific register in the UFS host controller's
 * memory-mapped I/O space.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param reg_offset Offset from the base address of the register to read from
 *
 * @return 32-bit value read from the specified register in the UFS host controller
 */
static inline uint32_t ufshc_read_reg(const struct ufs_host_controller *ufshc,
				      uintptr_t reg_offset)
{
	return sys_read32(((uintptr_t)((ufshc)->mmio_base) + (uintptr_t)(reg_offset)));
}

/* PUBLIC FUNCTIONS */

/**
 * @brief UFS Interface APIs
 * @defgroup ufs_interface UFS Subsystem Interface
 * @ingroup os_services
 * @{
 */

/**
 * @brief Initialize the UFS host controller
 *
 * This function is responsible for initializing the UFS driver by linking the UFS
 * device structure to its corresponding host controller instance and performing
 * the necessary initialization steps.
 *
 * @param ufshc_dev Pointer to the device structure for the UFS host controller
 * @param ufshc Pointer to the UFS host controller structure that will be initialized
 *
 * @return 0 on success, negative error code on failure.
 */
int32_t ufs_init(const struct device *ufshc_dev, struct ufs_host_controller **ufshc);

/**
 * @brief Fill UIC command structure
 *
 * This function fills the provided UIC command structure with the values
 * for the command, MIB attribute, generation selection, attribute set type,
 * and MIB value, based on the function arguments.
 *
 * @param uic_cmd_ptr Pointer to the UIC command structure to be filled
 * @param mib_attr_gen_sel MIB attribute and generation selection value
 *                         (upper 16-bits for attribute, lower 16-bits for selection)
 * @param attr_set_type Attribute set type for the UIC command
 * @param mib_val MIB value for the UIC command
 * @param cmd UIC command value
 */
void ufshc_fill_uic_cmd(struct ufshc_uic_cmd *uic_cmd_ptr,
			uint32_t mib_attr_gen_sel, uint32_t mib_val,
			uint32_t attr_set_type, uint32_t cmd);

/**
 * @brief Send a UIC command to the UFS host controller
 *
 * This function sends a UIC (UFS Interface command) to the UFS host controller
 * and waits for its completion.
 *
 * @param ufshc Pointer to the UFS host controller instance.
 * @param uic_cmd Pointer to the UIC command structure to be sent.
 *
 * @return 0 on success, negative error code on failure
 */
int32_t ufshc_send_uic_cmd(struct ufs_host_controller *ufshc, struct ufshc_uic_cmd *uic_cmd);

/**
 * @brief Send raw UPIU commands to the UFS host controller
 *
 * This function executes raw UPIU commands such as NOP, Query, and SCSI commands
 * by using the provided `msgcode`. The caller must fill the UPIU request content
 * correctly, as no further validation is performed. The function copies the response
 * to the `rsp` parameter if it's not NULL.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param msgcode  Message code, one of UPIU Transaction Codes Initiator to Target
 * @param req Pointer to the request data (raw UPIU command)
 * @param rsp Pointer to the response data (raw UPIU reply)
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_exec_raw_upiu_cmd(struct ufs_host_controller *ufshc,
				uint32_t msgcode, void *req, void *rsp);

/**
 * @brief Read or write descriptors in the UFS host controller
 *
 * This function reads or writes the specified UFS descriptor based on the operation mode.
 * If writing, the contents of `param_buff` are written to the descriptor. If reading,
 * the descriptor contents are copied into `param_buff` starting at `param_offset`.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param write Boolean indicating whether to write (true) or read (false) the descriptors
 * @param idn Descriptor identifier
 * @param index Descriptor index
 * @param param_offset Offset for the parameters in the descriptor
 * @param param_buff Pointer to the buffer holding the parameters to read/write
 * @param param_size Size of the parameter buffer
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_rw_descriptors(struct ufs_host_controller *ufshc, bool write,
			     uint8_t idn, uint8_t index, uint8_t param_offset,
			     void *param_buff, uint8_t param_size);

/**
 * @brief Send attribute query requests to the UFS host controller
 *
 * This function performs a read or write operation for a specific UFS attribute
 * identified by `idn`.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param write Boolean indicating whether to write (true) or read (false) the attribute
 * @param idn Attribute identifier
 * @param data Pointer to the attribute value to read/write
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_rw_attributes(struct ufs_host_controller *ufshc, bool write, uint8_t idn, void *data);

/**
 * @brief Read or write flags in the UFS host controller
 *
 * This function performs a read or write operation for a UFS flag specified by
 * `idn` and `index`. Supports setting, clearing, or reading the flag value.
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param write Boolean indicating whether to write (true) or read (false) the flags
 * @param idn Flag identifier
 * @param index Flag index
 * @param data  Pointer to the data buffer for reading/writing the flag
 *
 * @return 0 on success, a negative error code on failure.
 */
int32_t ufshc_rw_flags(struct ufs_host_controller *ufshc, bool write,
		       uint8_t idn, uint8_t index, void *data);

/**
 * @brief Configure the UFS speed gear setting
 *
 * This function sets the UFS host controller's speed gear by configuring the tx and rx
 * attributes (such as termination capabilities)
 *
 * @param ufshc Pointer to the UFS host controller structure
 * @param speed_gear Desired speed gear setting
 *
 * @return 0 on success, negative error code on failure
 */
int32_t ufshc_configure_speedgear(struct ufs_host_controller *ufshc, uint32_t speed_gear);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_UFS_UFS_H_ */
