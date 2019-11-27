/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

#define BDADDR_SIZE 6

/*
 * PDU Sizes
 */
/* Advertisement channel maximum payload size */
#define PDU_AC_PAYLOAD_SIZE_MAX 37
/* Link Layer header size of Adv PDU. Assumes pdu_adv is packed */
#define PDU_AC_LL_HEADER_SIZE  (offsetof(struct pdu_adv, payload))
/* Advertisement channel maximum PDU size */
#define PDU_AC_SIZE_MAX        (PDU_AC_LL_HEADER_SIZE + PDU_AC_PAYLOAD_SIZE_MAX)

#define ACCESS_ADDR_SIZE        4
#define ADVA_SIZE               6
#define SCANA_SIZE              6
#define INITA_SIZE              6
#define TARGETA_SIZE            6
#define LLDATA_SIZE             22
#define CRC_SIZE                3
#define PREAMBLE_SIZE(phy)      (phy&0x3)
#define LL_HEADER_SIZE(phy)     (PREAMBLE_SIZE(phy) + PDU_AC_LL_HEADER_SIZE \
				  + ACCESS_ADDR_SIZE + CRC_SIZE)
#define BYTES2US(bytes, phy)    (((bytes)<<3)/BIT((phy&0x3)>>1))

/* Data channel minimum payload */
#define PDU_DC_PAYLOAD_SIZE_MIN 27
/* Link Layer header size of Data PDU. Assumes pdu_data is packed */
#define PDU_DC_LL_HEADER_SIZE  (offsetof(struct pdu_data, lldata))

/* Max size of an empty PDU. TODO: Remove; only used in Nordic LLL */
#define PDU_EM_SIZE_MAX        (PDU_DC_LL_HEADER_SIZE)

/* Event interframe timings */
#define EVENT_IFS_US            150
/* Standard allows 2 us timing uncertainty inside the event */
#define EVENT_IFS_MAX_US        (EVENT_IFS_US + 2)
/* Controller will layout extended adv with minimum separation */
#define EVENT_MAFS_US           300
/* Standard allows 2 us timing uncertainty inside the event */
#define EVENT_MAFS_MAX_US       (EVENT_MAFS_US + 2)

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

#define PDU_AC_SIZE_EXTRA (PDU_AC_SIZE_RSSI + \
			   PDU_AC_SIZE_PRIV + \
			   PDU_AC_SIZE_SCFP + \
			   PDU_AC_SIZE_MESH)

struct pdu_adv_adv_ind {
	u8_t addr[BDADDR_SIZE];
	u8_t data[31];
} __packed;

struct pdu_adv_direct_ind {
	u8_t adv_addr[BDADDR_SIZE];
	u8_t tgt_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_scan_rsp {
	u8_t addr[BDADDR_SIZE];
	u8_t data[31];
} __packed;

struct pdu_adv_scan_req {
	u8_t scan_addr[BDADDR_SIZE];
	u8_t adv_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_connect_ind {
	u8_t init_addr[BDADDR_SIZE];
	u8_t adv_addr[BDADDR_SIZE];
	struct {
		u8_t  access_addr[4];
		u8_t  crc_init[3];
		u8_t  win_size;
		u16_t win_offset;
		u16_t interval;
		u16_t latency;
		u16_t timeout;
		u8_t  chan_map[5];
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		u8_t  hop:5;
		u8_t  sca:3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		u8_t  sca:3;
		u8_t  hop:5;
#else
#error "Unsupported endianness"
#endif

	} __packed;
} __packed;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
struct pdu_adv_com_ext_adv {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u8_t ext_hdr_len:6;
	u8_t adv_mode:2;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u8_t adv_mode:2;
	u8_t ext_hdr_len:6;
#else
#error "Unsupported endianness"
#endif
	u8_t ext_hdr_adi_adv_data[254];
} __packed;

enum ext_adv_mode {
	EXT_ADV_MODE_NON_CONN_NON_SCAN = 0x00,
	EXT_ADV_MODE_CONN_NON_SCAN = 0x01,
	EXT_ADV_MODE_NON_CONN_SCAN = 0x02,
};

struct ext_adv_hdr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u8_t adv_addr:1;
	u8_t tgt_addr:1;
	u8_t rfu0:1;
	u8_t adi:1;
	u8_t aux_ptr:1;
	u8_t sync_info:1;
	u8_t tx_pwr:1;
	u8_t rfu1:1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u8_t rfu1:1;
	u8_t tx_pwr:1;
	u8_t sync_info:1;
	u8_t aux_ptr:1;
	u8_t adi:1;
	u8_t rfu0:1;
	u8_t tgt_addr:1;
	u8_t adv_addr:1;
#else
#error "Unsupported endianness"
#endif
} __packed;

struct ext_adv_adi {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u16_t did:12;
	u16_t sid:4;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u16_t sid:4;
	u16_t did:12;
#else
#error "Unsupported endianness"
#endif
} __packed;

struct ext_adv_aux_ptr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u8_t  chan_idx:6;
	u8_t  ca:1;
	u8_t  offs_units:1;
	u16_t offs:13;
	u16_t phy:3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u8_t  offs_units:1;
	u8_t  ca:1;
	u8_t  chan_idx:6;
	u16_t phy:3;
	u16_t offs:13;
#else
#error "Unsupported endianness"
#endif
} __packed;

enum ext_adv_aux_ptr_ca {
	EXT_ADV_AUX_PTR_CA_500_PPM = 0x00,
	EXT_ADV_AUX_PTR_CA_50_PPM  = 0x01,
};

enum ext_adv_offs_units {
	EXT_ADV_AUX_PTR_OFFS_UNITS_30  = 0x00,
	EXT_ADV_AUX_PTR_OFFS_UNITS_300 = 0x01,
};

enum ext_adv_aux_phy {
	EXT_ADV_AUX_PHY_LE_1M  = 0x00,
	EXT_ADV_AUX_PHY_LE_2M  = 0x01,
	EXT_ADV_AUX_PHY_LE_COD = 0x02,
};

struct ext_adv_sync_info {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u16_t sync_pkt_offs:13;
	u16_t offs_units:1;
	u16_t rfu:2;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u16_t rfu:2;
	u16_t offs_units:1;
	u16_t sync_pkt_offs:13;
#else
#error "Unsupported endianness"
#endif
	u16_t interval;
	u8_t  sca_chm[5];
	u32_t aa;
	u8_t  crc_init[3];
	u16_t evt_cntr;
} __packed;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

enum pdu_adv_type {
	PDU_ADV_TYPE_ADV_IND = 0x00,
	PDU_ADV_TYPE_DIRECT_IND = 0x01,
	PDU_ADV_TYPE_NONCONN_IND = 0x02,
	PDU_ADV_TYPE_SCAN_REQ = 0x03,
	PDU_ADV_TYPE_AUX_SCAN_REQ = PDU_ADV_TYPE_SCAN_REQ,
	PDU_ADV_TYPE_SCAN_RSP = 0x04,
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
	u8_t type:4;
	u8_t rfu:1;
	u8_t chan_sel:1;
	u8_t tx_addr:1;
	u8_t rx_addr:1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u8_t rx_addr:1;
	u8_t tx_addr:1;
	u8_t chan_sel:1;
	u8_t rfu:1;
	u8_t type:4;
#else
#error "Unsupported endianness"
#endif

	u8_t len;

	union {
		u8_t   payload[0];
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
	u8_t  win_size;
	u16_t win_offset;
	u16_t interval;
	u16_t latency;
	u16_t timeout;
	u16_t instant;
} __packed;

struct pdu_data_llctrl_chan_map_ind {
	u8_t  chm[5];
	u16_t instant;
} __packed;

struct pdu_data_llctrl_terminate_ind {
	u8_t error_code;
} __packed;

struct pdu_data_llctrl_enc_req {
	u8_t rand[8];
	u8_t ediv[2];
	u8_t skdm[8];
	u8_t ivm[4];
} __packed;

struct pdu_data_llctrl_enc_rsp {
	u8_t skds[8];
	u8_t ivs[4];
} __packed;

struct pdu_data_llctrl_start_enc_req {
	/* no members */
} __packed;

struct pdu_data_llctrl_start_enc_rsp {
	/* no members */
} __packed;

struct pdu_data_llctrl_unknown_rsp {
	u8_t type;
} __packed;

struct pdu_data_llctrl_feature_req {
	u8_t features[8];
} __packed;

struct pdu_data_llctrl_feature_rsp {
	u8_t features[8];
} __packed;

struct pdu_data_llctrl_pause_enc_req {
	/* no members */
} __packed;

struct pdu_data_llctrl_pause_enc_rsp {
	/* no members */
} __packed;

struct pdu_data_llctrl_version_ind {
	u8_t  version_number;
	u16_t company_id;
	u16_t sub_version_number;
} __packed;

struct pdu_data_llctrl_reject_ind {
	u8_t error_code;
} __packed;

struct pdu_data_llctrl_slave_feature_req {
	u8_t features[8];
} __packed;

struct pdu_data_llctrl_conn_param_req {
	u16_t interval_min;
	u16_t interval_max;
	u16_t latency;
	u16_t timeout;
	u8_t  preferred_periodicity;
	u16_t reference_conn_event_count;
	u16_t offset0;
	u16_t offset1;
	u16_t offset2;
	u16_t offset3;
	u16_t offset4;
	u16_t offset5;
} __packed;

struct pdu_data_llctrl_conn_param_rsp {
	u16_t interval_min;
	u16_t interval_max;
	u16_t latency;
	u16_t timeout;
	u8_t  preferred_periodicity;
	u16_t reference_conn_event_count;
	u16_t offset0;
	u16_t offset1;
	u16_t offset2;
	u16_t offset3;
	u16_t offset4;
	u16_t offset5;
} __packed;

struct pdu_data_llctrl_reject_ext_ind {
	u8_t reject_opcode;
	u8_t error_code;
} __packed;

struct pdu_data_llctrl_ping_req {
	/* no members */
} __packed;

struct pdu_data_llctrl_ping_rsp {
	/* no members */
} __packed;

struct pdu_data_llctrl_length_req {
	u16_t max_rx_octets;
	u16_t max_rx_time;
	u16_t max_tx_octets;
	u16_t max_tx_time;
} __packed;

struct pdu_data_llctrl_length_rsp {
	u16_t max_rx_octets;
	u16_t max_rx_time;
	u16_t max_tx_octets;
	u16_t max_tx_time;
} __packed;

struct pdu_data_llctrl_phy_req {
	u8_t tx_phys;
	u8_t rx_phys;
} __packed;

struct pdu_data_llctrl_phy_rsp {
	u8_t tx_phys;
	u8_t rx_phys;
} __packed;

struct pdu_data_llctrl_phy_upd_ind {
	u8_t  m_to_s_phy;
	u8_t  s_to_m_phy;
	u16_t instant;
} __packed;

struct pdu_data_llctrl_min_used_chans_ind {
	u8_t phys;
	u8_t min_used_chans;
} __packed;

struct pdu_data_llctrl {
	u8_t opcode;
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
	u8_t lcur;
	u8_t lmin;
	u8_t lmax;
	u8_t cur;
	u8_t min;
	u8_t max;
} __packed;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

struct pdu_data {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u8_t ll_id:2;
	u8_t nesn:1;
	u8_t sn:1;
	u8_t md:1;
	u8_t rfu:3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u8_t rfu:3;
	u8_t md:1;
	u8_t sn:1;
	u8_t nesn:1;
	u8_t ll_id:2;
#else
#error "Unsupported endianness"
#endif

	u8_t len;

#if !defined(CONFIG_SOC_OPENISA_RV32M1_RISCV32)
#if !defined(CONFIG_BT_CTLR_DATA_LENGTH_CLEAR)
	u8_t resv:8; /* TODO: remove nRF specific code */
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH_CLEAR */
#endif /* !CONFIG_SOC_OPENISA_RV32M1_RISCV32 */

	union {
		struct pdu_data_llctrl llctrl;
		u8_t                   lldata[0];

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		u8_t                   rssi;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		struct profile         profile;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */
	} __packed;
} __packed;
