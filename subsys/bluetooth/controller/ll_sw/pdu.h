/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

/*
 * PDU fields sizes
 */

#define PDU_PREAMBLE_SIZE(phy) (phy&0x3)
#define PDU_ACCESS_ADDR_SIZE   4
#define PDU_HEADER_SIZE        2
#define PDU_MIC_SIZE           4
#define PDU_CRC_SIZE           3
#define PDU_OVERHEAD_SIZE(phy) (PDU_PREAMBLE_SIZE(phy) + \
				PDU_ACCESS_ADDR_SIZE + \
				PDU_HEADER_SIZE + \
				PDU_CRC_SIZE)

#define BDADDR_SIZE   6
#define ADVA_SIZE     BDADDR_SIZE
#define SCANA_SIZE    BDADDR_SIZE
#define INITA_SIZE    BDADDR_SIZE
#define TARGETA_SIZE  BDADDR_SIZE
#define LLDATA_SIZE   22
#define CTE_INFO_SIZE 1
#define TX_PWR_SIZE   1
#define ACAD_SIZE     0

#define BYTES2US(bytes, phy) (((bytes)<<3)/BIT((phy&0x3)>>1))

/* Advertisement channel maximum legacy payload size */
#define PDU_AC_PAYLOAD_SIZE_MAX 37
/* Advertisement channel maximum extended payload size */
#define PDU_AC_EXT_PAYLOAD_SIZE_MAX 251
/* Advertisement channel minimum extended payload size */
#define PDU_AC_EXT_PAYLOAD_SIZE_MIN (offsetof(pdu_adv_com_ext_adv, \
					      ext_hdr_adi_adv_data) + \
				     ADVA_SIZE + \
				     TARGETA_SIZE + \
				     CTE_INFO_SIZE + \
				     sizeof(struct pdu_adv_adi) + \
				     sizeof(struct pdu_adv_aux_ptr) + \
				     sizeof(struct pdu_adv_sync_info) + \
				     TX_PWR_SIZE + \
				     ACAD_SIZE)

/* Link Layer header size of Adv PDU. Assumes pdu_adv is packed */
#define PDU_AC_LL_HEADER_SIZE  (offsetof(struct pdu_adv, payload))

/* Link Layer Advertisement channel maximum PDU buffer size */
#define PDU_AC_LL_SIZE_MAX     (PDU_AC_LL_HEADER_SIZE + PDU_AC_PAYLOAD_SIZE_MAX)

/* Advertisement channel Access Address */
#define PDU_AC_ACCESS_ADDR     0x8e89bed6

/* Data channel minimum payload size and time */
#define PDU_DC_PAYLOAD_SIZE_MIN 27
#define PDU_DC_PAYLOAD_TIME_MIN 328

/* Link Layer header size of Data PDU. Assumes pdu_data is packed */
#define PDU_DC_LL_HEADER_SIZE  (offsetof(struct pdu_data, lldata))

/* Link Layer Max size of an empty PDU. TODO: Remove; only used in Nordic LLL */
#define PDU_EM_LL_SIZE_MAX     (PDU_DC_LL_HEADER_SIZE)

/* Event interframe timings */
#define EVENT_IFS_US            150
/* Standard allows 2 us timing uncertainty inside the event */
#define EVENT_IFS_MAX_US        (EVENT_IFS_US + 2)
/* Controller will layout extended adv with minimum separation */
#define EVENT_MAFS_US           300
/* Standard allows 2 us timing uncertainty inside the event */
#define EVENT_MAFS_MAX_US       (EVENT_MAFS_US + 2)

/* SyncInfo field Sync Packet Offset Units field encoding */
#define SYNC_PKT_OFFS_UNIT_30_US  30
#define SYNC_PKT_OFFS_UNIT_300_US 300

/*
 * Macros to return correct Data Channel PDU time
 * Note: formula is valid for 1M, 2M and Coded S8
 * see BT spec Version 5.1 Vol 6. Part B, chapters 2.1 and 2.2
 * for packet formats and thus lengths
 */

#define PHY_LEGACY 0
#define PHY_1M     BIT(0)
#define PHY_2M     BIT(1)
#define PHY_CODED  BIT(2)

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define CODED_PHY_PREAMBLE_TIME_US       80
#define CODED_PHY_ACCESS_ADDRESS_TIME_US 256
#define CODED_PHY_CI_TIME_US             16
#define CODED_PHY_TERM1_TIME_US          24
#define CODED_PHY_CRC_SIZE               24
#define CODED_PHY_TERM2_SIZE             3

#define FEC_BLOCK1_TIME_US               ((CODED_PHY_ACCESS_ADDRESS_TIME_US) + \
					  (CODED_PHY_CI_TIME_US) + \
					  (CODED_PHY_TERM1_TIME_US))
#define FEC_BLOCK2_TIME_US(octets, mic)  (((((PDU_HEADER_SIZE) + \
					     (octets) + \
					     (mic))<<3) + \
					    (CODED_PHY_CRC_SIZE) + \
					    (CODED_PHY_TERM2_SIZE))<<3)

#define PKT_DC_US(octets, mic, phy) (((phy) & PHY_CODED) ? \
				     ((CODED_PHY_PREAMBLE_TIME_US) + \
				      (FEC_BLOCK1_TIME_US) + \
				      FEC_BLOCK2_TIME_US((octets), (mic))) : \
				     (((PDU_PREAMBLE_SIZE(phy) + \
					(PDU_ACCESS_ADDR_SIZE) + \
					(PDU_HEADER_SIZE) + \
					(octets) + \
					(mic) + \
					(PDU_CRC_SIZE))<<3) / \
				      BIT(((phy) & 0x03) >> 1)))

#else /* !CONFIG_BT_CTLR_PHY_CODED */
#define PKT_DC_US(octets, mic, phy) (((PDU_PREAMBLE_SIZE(phy) + \
				       (PDU_ACCESS_ADDR_SIZE) + \
				       (PDU_HEADER_SIZE) + \
				       (octets) + \
				       (mic) + \
				       (PDU_CRC_SIZE))<<3) / \
				     BIT(((phy) & 0x03) >> 1))
#endif /* !CONFIG_BT_CTLR_PHY_CODED */

#define PKT_US(octets, phy) PKT_DC_US((octets), (PDU_MIC_SIZE), (phy))

#define PKT_AC_US(octets, mic, phy) PKT_DC_US((octets), (mic), (phy))

/* Extra bytes for enqueued node_rx metadata: rssi (always), resolving
 * index, directed adv report, and mesh channel and instant.
 */
#define PDU_AC_SIZE_RSSI 1
#if defined(CONFIG_BT_CTLR_PRIVACY)
#define PDU_AC_SIZE_PRIV 1
#else
#define PDU_AC_SIZE_PRIV 0
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
#define PDU_AC_SIZE_SCFP 1
#else
#define PDU_AC_SIZE_SCFP 0
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */
#if defined(CONFIG_BT_HCI_MESH_EXT)
#define PDU_AC_SIZE_MESH 5
#else
#define PDU_AC_SIZE_MESH 0
#endif /* CONFIG_BT_HCI_MESH_EXT */

#define PDU_AC_LL_SIZE_EXTRA (PDU_AC_SIZE_RSSI + \
			      PDU_AC_SIZE_PRIV + \
			      PDU_AC_SIZE_SCFP + \
			      PDU_AC_SIZE_MESH)

struct pdu_adv_adv_ind {
	uint8_t addr[BDADDR_SIZE];
	uint8_t data[31];
} __packed;

struct pdu_adv_direct_ind {
	uint8_t adv_addr[BDADDR_SIZE];
	uint8_t tgt_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_scan_rsp {
	uint8_t addr[BDADDR_SIZE];
	uint8_t data[31];
} __packed;

struct pdu_adv_scan_req {
	uint8_t scan_addr[BDADDR_SIZE];
	uint8_t adv_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_connect_ind {
	uint8_t init_addr[BDADDR_SIZE];
	uint8_t adv_addr[BDADDR_SIZE];
	struct {
		uint8_t  access_addr[4];
		uint8_t  crc_init[3];
		uint8_t  win_size;
		uint16_t win_offset;
		uint16_t interval;
		uint16_t latency;
		uint16_t timeout;
		uint8_t  chan_map[5];
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		uint8_t  hop:5;
		uint8_t  sca:3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		uint8_t  sca:3;
		uint8_t  hop:5;
#else
#error "Unsupported endianness"
#endif

	} __packed;
} __packed;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
struct pdu_adv_com_ext_adv {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t ext_hdr_len:6;
	uint8_t adv_mode:2;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t adv_mode:2;
	uint8_t ext_hdr_len:6;
#else
#error "Unsupported endianness"
#endif
	uint8_t ext_hdr_adi_adv_data[254];
} __packed;

enum pdu_adv_mode {
	EXT_ADV_MODE_NON_CONN_NON_SCAN = 0x00,
	EXT_ADV_MODE_CONN_NON_SCAN = 0x01,
	EXT_ADV_MODE_NON_CONN_SCAN = 0x02,
};

struct pdu_adv_hdr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t adv_addr:1;
	uint8_t tgt_addr:1;
	uint8_t rfu0:1;
	uint8_t adi:1;
	uint8_t aux_ptr:1;
	uint8_t sync_info:1;
	uint8_t tx_pwr:1;
	uint8_t rfu1:1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t rfu1:1;
	uint8_t tx_pwr:1;
	uint8_t sync_info:1;
	uint8_t aux_ptr:1;
	uint8_t adi:1;
	uint8_t rfu0:1;
	uint8_t tgt_addr:1;
	uint8_t adv_addr:1;
#else
#error "Unsupported endianness"
#endif
} __packed;

struct pdu_adv_adi {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint16_t did:12;
	uint16_t sid:4;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint16_t sid:4;
	uint16_t did:12;
#else
#error "Unsupported endianness"
#endif
} __packed;

struct pdu_adv_aux_ptr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t  chan_idx:6;
	uint8_t  ca:1;
	uint8_t  offs_units:1;
	uint16_t offs:13;
	uint16_t phy:3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t  offs_units:1;
	uint8_t  ca:1;
	uint8_t  chan_idx:6;
	uint16_t phy:3;
	uint16_t offs:13;
#else
#error "Unsupported endianness"
#endif
} __packed;

enum pdu_adv_aux_ptr_ca {
	EXT_ADV_AUX_PTR_CA_500_PPM = 0x00,
	EXT_ADV_AUX_PTR_CA_50_PPM  = 0x01,
};

enum pdu_adv_offs_units {
	EXT_ADV_AUX_PTR_OFFS_UNITS_30  = 0x00,
	EXT_ADV_AUX_PTR_OFFS_UNITS_300 = 0x01,
};

enum pdu_adv_aux_phy {
	EXT_ADV_AUX_PHY_LE_1M  = 0x00,
	EXT_ADV_AUX_PHY_LE_2M  = 0x01,
	EXT_ADV_AUX_PHY_LE_COD = 0x02,
};

struct pdu_adv_sync_info {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint16_t offs:13;
	uint16_t offs_units:1;
	uint16_t rfu:2;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint16_t rfu:2;
	uint16_t offs_units:1;
	uint16_t offs:13;
#else
#error "Unsupported endianness"
#endif
	uint16_t interval;
	uint8_t  sca_chm[5];
	uint32_t aa;
	uint8_t  crc_init[3];
	uint16_t evt_cntr;
} __packed;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

enum pdu_adv_type {
	PDU_ADV_TYPE_ADV_IND = 0x00,
	PDU_ADV_TYPE_DIRECT_IND = 0x01,
	PDU_ADV_TYPE_NONCONN_IND = 0x02,
	PDU_ADV_TYPE_SCAN_REQ = 0x03,
	PDU_ADV_TYPE_AUX_SCAN_REQ = PDU_ADV_TYPE_SCAN_REQ,
	PDU_ADV_TYPE_SCAN_RSP = 0x04,
	PDU_ADV_TYPE_ADV_IND_SCAN_RSP = 0x05,
	PDU_ADV_TYPE_CONNECT_IND = 0x05,
	PDU_ADV_TYPE_AUX_CONNECT_REQ = PDU_ADV_TYPE_CONNECT_IND,
	PDU_ADV_TYPE_SCAN_IND = 0x06,
	PDU_ADV_TYPE_EXT_IND = 0x07,
	PDU_ADV_TYPE_AUX_ADV_IND = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_SCAN_RSP = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_SYNC_IND = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_CHAIN_IND = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_CONNECT_RSP = 0x08,
} __packed;

struct pdu_adv {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t type:4;
	uint8_t rfu:1;
	uint8_t chan_sel:1;
	uint8_t tx_addr:1;
	uint8_t rx_addr:1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t rx_addr:1;
	uint8_t tx_addr:1;
	uint8_t chan_sel:1;
	uint8_t rfu:1;
	uint8_t type:4;
#else
#error "Unsupported endianness"
#endif

	uint8_t len;

	union {
		uint8_t   payload[0];
		struct pdu_adv_adv_ind adv_ind;
		struct pdu_adv_direct_ind direct_ind;
		struct pdu_adv_scan_req scan_req;
		struct pdu_adv_scan_rsp scan_rsp;
		struct pdu_adv_connect_ind connect_ind;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		struct pdu_adv_com_ext_adv adv_ext_ind;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} __packed;
} __packed;

enum pdu_data_llid {
	PDU_DATA_LLID_RESV = 0x00,
	PDU_DATA_LLID_DATA_CONTINUE = 0x01,
	PDU_DATA_LLID_DATA_START = 0x02,
	PDU_DATA_LLID_CTRL = 0x03,
};

enum pdu_data_llctrl_type {
	PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND = 0x00,
	PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND = 0x01,
	PDU_DATA_LLCTRL_TYPE_TERMINATE_IND = 0x02,
	PDU_DATA_LLCTRL_TYPE_ENC_REQ = 0x03,
	PDU_DATA_LLCTRL_TYPE_ENC_RSP = 0x04,
	PDU_DATA_LLCTRL_TYPE_START_ENC_REQ = 0x05,
	PDU_DATA_LLCTRL_TYPE_START_ENC_RSP = 0x06,
	PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP = 0x07,
	PDU_DATA_LLCTRL_TYPE_FEATURE_REQ = 0x08,
	PDU_DATA_LLCTRL_TYPE_FEATURE_RSP = 0x09,
	PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ = 0x0A,
	PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP = 0x0B,
	PDU_DATA_LLCTRL_TYPE_VERSION_IND = 0x0C,
	PDU_DATA_LLCTRL_TYPE_REJECT_IND = 0x0D,
	PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ = 0x0E,
	PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ = 0x0F,
	PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP = 0x10,
	PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND = 0x11,
	PDU_DATA_LLCTRL_TYPE_PING_REQ = 0x12,
	PDU_DATA_LLCTRL_TYPE_PING_RSP = 0x13,
	PDU_DATA_LLCTRL_TYPE_LENGTH_REQ = 0x14,
	PDU_DATA_LLCTRL_TYPE_LENGTH_RSP = 0x15,
	PDU_DATA_LLCTRL_TYPE_PHY_REQ = 0x16,
	PDU_DATA_LLCTRL_TYPE_PHY_RSP = 0x17,
	PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND = 0x18,
	PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND = 0x19,
};

struct pdu_data_llctrl_conn_update_ind {
	uint8_t  win_size;
	uint16_t win_offset;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
	uint16_t instant;
} __packed;

struct pdu_data_llctrl_chan_map_ind {
	uint8_t  chm[5];
	uint16_t instant;
} __packed;

struct pdu_data_llctrl_terminate_ind {
	uint8_t error_code;
} __packed;

struct pdu_data_llctrl_enc_req {
	uint8_t rand[8];
	uint8_t ediv[2];
	uint8_t skdm[8];
	uint8_t ivm[4];
} __packed;

struct pdu_data_llctrl_enc_rsp {
	uint8_t skds[8];
	uint8_t ivs[4];
} __packed;

struct pdu_data_llctrl_start_enc_req {
	/* no members */
} __packed;

struct pdu_data_llctrl_start_enc_rsp {
	/* no members */
} __packed;

struct pdu_data_llctrl_unknown_rsp {
	uint8_t type;
} __packed;

struct pdu_data_llctrl_feature_req {
	uint8_t features[8];
} __packed;

struct pdu_data_llctrl_feature_rsp {
	uint8_t features[8];
} __packed;

struct pdu_data_llctrl_pause_enc_req {
	/* no members */
} __packed;

struct pdu_data_llctrl_pause_enc_rsp {
	/* no members */
} __packed;

struct pdu_data_llctrl_version_ind {
	uint8_t  version_number;
	uint16_t company_id;
	uint16_t sub_version_number;
} __packed;

struct pdu_data_llctrl_reject_ind {
	uint8_t error_code;
} __packed;

struct pdu_data_llctrl_slave_feature_req {
	uint8_t features[8];
} __packed;

struct pdu_data_llctrl_conn_param_req {
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint8_t  preferred_periodicity;
	uint16_t reference_conn_event_count;
	uint16_t offset0;
	uint16_t offset1;
	uint16_t offset2;
	uint16_t offset3;
	uint16_t offset4;
	uint16_t offset5;
} __packed;

struct pdu_data_llctrl_conn_param_rsp {
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint8_t  preferred_periodicity;
	uint16_t reference_conn_event_count;
	uint16_t offset0;
	uint16_t offset1;
	uint16_t offset2;
	uint16_t offset3;
	uint16_t offset4;
	uint16_t offset5;
} __packed;

struct pdu_data_llctrl_reject_ext_ind {
	uint8_t reject_opcode;
	uint8_t error_code;
} __packed;

struct pdu_data_llctrl_ping_req {
	/* no members */
} __packed;

struct pdu_data_llctrl_ping_rsp {
	/* no members */
} __packed;

struct pdu_data_llctrl_length_req {
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
} __packed;

struct pdu_data_llctrl_length_rsp {
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
} __packed;

struct pdu_data_llctrl_phy_req {
	uint8_t tx_phys;
	uint8_t rx_phys;
} __packed;

struct pdu_data_llctrl_phy_rsp {
	uint8_t tx_phys;
	uint8_t rx_phys;
} __packed;

struct pdu_data_llctrl_phy_upd_ind {
	uint8_t  m_to_s_phy;
	uint8_t  s_to_m_phy;
	uint16_t instant;
} __packed;

struct pdu_data_llctrl_min_used_chans_ind {
	uint8_t phys;
	uint8_t min_used_chans;
} __packed;

struct pdu_data_llctrl {
	uint8_t opcode;
	union {
		struct pdu_data_llctrl_conn_update_ind conn_update_ind;
		struct pdu_data_llctrl_chan_map_ind chan_map_ind;
		struct pdu_data_llctrl_terminate_ind terminate_ind;
		struct pdu_data_llctrl_enc_req enc_req;
		struct pdu_data_llctrl_enc_rsp enc_rsp;
		struct pdu_data_llctrl_start_enc_req start_enc_req;
		struct pdu_data_llctrl_start_enc_rsp start_enc_rsp;
		struct pdu_data_llctrl_unknown_rsp unknown_rsp;
		struct pdu_data_llctrl_feature_req feature_req;
		struct pdu_data_llctrl_feature_rsp feature_rsp;
		struct pdu_data_llctrl_pause_enc_req pause_enc_req;
		struct pdu_data_llctrl_pause_enc_rsp pause_enc_rsp;
		struct pdu_data_llctrl_version_ind version_ind;
		struct pdu_data_llctrl_reject_ind reject_ind;
		struct pdu_data_llctrl_slave_feature_req slave_feature_req;
		struct pdu_data_llctrl_conn_param_req conn_param_req;
		struct pdu_data_llctrl_conn_param_rsp conn_param_rsp;
		struct pdu_data_llctrl_reject_ext_ind reject_ext_ind;
		struct pdu_data_llctrl_ping_req ping_req;
		struct pdu_data_llctrl_ping_rsp ping_rsp;
		struct pdu_data_llctrl_length_req length_req;
		struct pdu_data_llctrl_length_rsp length_rsp;
		struct pdu_data_llctrl_phy_req phy_req;
		struct pdu_data_llctrl_phy_rsp phy_rsp;
		struct pdu_data_llctrl_phy_upd_ind phy_upd_ind;
		struct pdu_data_llctrl_min_used_chans_ind min_used_chans_ind;
	} __packed;
} __packed;

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
struct profile {
	uint8_t lcur;
	uint8_t lmin;
	uint8_t lmax;
	uint8_t cur;
	uint8_t min;
	uint8_t max;
} __packed;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

struct pdu_data {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t ll_id:2;
	uint8_t nesn:1;
	uint8_t sn:1;
	uint8_t md:1;
	uint8_t rfu:3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t rfu:3;
	uint8_t md:1;
	uint8_t sn:1;
	uint8_t nesn:1;
	uint8_t ll_id:2;
#else
#error "Unsupported endianness"
#endif

	uint8_t len;

#if !defined(CONFIG_SOC_OPENISA_RV32M1_RISCV32)
#if !defined(CONFIG_BT_CTLR_DATA_LENGTH_CLEAR)
	uint8_t resv:8; /* TODO: remove nRF specific code */
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH_CLEAR */
#endif /* !CONFIG_SOC_OPENISA_RV32M1_RISCV32 */

	union {
		struct pdu_data_llctrl llctrl;
		uint8_t                   lldata[0];

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		uint8_t                   rssi;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		struct profile         profile;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */
	} __packed;
} __packed;
