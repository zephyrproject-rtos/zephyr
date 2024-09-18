/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

/* Constant offsets in extended header (TargetA is present in PDUs with AdvA) */
#define ADVA_OFFSET   0
#define TGTA_OFFSET   (ADVA_OFFSET + BDADDR_SIZE)

#define BYTES2US(bytes, phy) (((bytes)<<3)/BIT((phy&0x3)>>1))

/* Advertisement channel minimum payload size */
#define PDU_AC_PAYLOAD_SIZE_MIN 1
/* Advertisement channel maximum legacy payload size */
#define PDU_AC_LEG_PAYLOAD_SIZE_MAX 37
/* Advertisement channel maximum legacy advertising/scan data size */
#define PDU_AC_LEG_DATA_SIZE_MAX   31

#if defined(CONFIG_BT_CTLR_ADV_EXT)
/* Advertisement channel maximum extended payload size */
#define PDU_AC_EXT_PAYLOAD_SIZE_MAX 255

/* Extended Scan and Periodic Sync Rx PDU time reservation */
#if defined(CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN)
#define PDU_AC_EXT_PAYLOAD_RX_SIZE 0U
#else /* !CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN */
#define PDU_AC_EXT_PAYLOAD_RX_SIZE PDU_AC_EXT_PAYLOAD_SIZE_MAX
#endif /* !CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN */

#define PDU_AC_EXT_HEADER_SIZE_MIN  offsetof(struct pdu_adv_com_ext_adv, \
					     ext_hdr_adv_data)
#define PDU_AC_EXT_HEADER_SIZE_MAX  63
/* TODO: PDU_AC_EXT_PAYLOAD_OVERHEAD can be reduced based on supported
 *       features, like omitting support for periodic advertising will reduce
 *       18 octets in the Common Extended Advertising Payload Format.
 */
#define PDU_AC_EXT_PAYLOAD_OVERHEAD (PDU_AC_EXT_HEADER_SIZE_MIN + \
				     PDU_AC_EXT_HEADER_SIZE_MAX)
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)
#define PDU_AC_PAYLOAD_SIZE_MAX     MAX(MIN((PDU_AC_EXT_PAYLOAD_OVERHEAD + \
					     CONFIG_BT_CTLR_ADV_DATA_LEN_MAX), \
					    PDU_AC_EXT_PAYLOAD_SIZE_MAX), \
					PDU_AC_LEG_PAYLOAD_SIZE_MAX)
#define PDU_AC_EXT_AD_DATA_LEN_MAX  (PDU_AC_PAYLOAD_SIZE_MAX - \
				     PDU_AC_EXT_PAYLOAD_OVERHEAD)
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_BROADCASTER */
#define PDU_AC_PAYLOAD_SIZE_MAX     PDU_AC_LEG_PAYLOAD_SIZE_MAX
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_BROADCASTER */

/* Link Layer header size of Adv PDU. Assumes pdu_adv is packed */
#define PDU_AC_LL_HEADER_SIZE  (offsetof(struct pdu_adv, payload))

/* Link Layer Advertisement channel maximum PDU buffer size */
#define PDU_AC_LL_SIZE_MAX     (PDU_AC_LL_HEADER_SIZE + PDU_AC_PAYLOAD_SIZE_MAX)

/* Advertisement channel Access Address */
#define PDU_AC_ACCESS_ADDR     0x8e89bed6

/* Advertisement channel CRC init value */
#define PDU_AC_CRC_IV          0x555555

/* CRC polynomial */
#define PDU_CRC_POLYNOMIAL     ((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16))

/* Data channel minimum payload size and time in us */
#define PDU_DC_PAYLOAD_SIZE_MIN       27
#define PDU_DC_PAYLOAD_TIME_MIN       328
#define PDU_DC_PAYLOAD_TIME_MIN_CODED 2704

/* Data channel maximum payload size and time in us */
#define PDU_DC_PAYLOAD_SIZE_MAX       251
#define PDU_DC_PAYLOAD_TIME_MAX_CODED 17040

#if defined(CONFIG_BT_CTLR_DF)
#define PDU_DC_PAYLOAD_TIME_MAX       2128
#else /* !CONFIG_BT_CTLR_DF */
#define PDU_DC_PAYLOAD_TIME_MAX       2120
#endif /* !CONFIG_BT_CTLR_DF */

/* Data channel control PDU maximum payload size */
#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CTLR_CONN_ISO)
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PERIPHERAL)
/* Isochronous Central and Peripheral */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(cis_req)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(cis_req)
#elif defined(CONFIG_BT_CENTRAL)
/* Isochronous Central */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(cis_req)
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_param_req)
#elif defined(CONFIG_BT_CTLR_LE_ENC)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_rsp)
#else /* !CONFIG_BT_CTLR_LE_ENC */
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(cis_rsp)
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#elif defined(CONFIG_BT_PERIPHERAL)
/* Isochronous Peripheral */
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_param_req)
#elif defined(CONFIG_BT_CTLR_LE_ENC)
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_rsp)
#else /* !CONFIG_BT_CTLR_LE_ENC */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(cis_rsp)
#endif /* !CONFIG_BT_CTLR_LE_ENC */
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(cis_req)
#endif /* !CONFIG_BT_PERIPHERAL */

#elif defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
/* Central and Peripheral with Connection Parameter Request */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_param_req)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_param_req)

#elif defined(CONFIG_BT_CTLR_LE_ENC)
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PERIPHERAL)
/* Central and Peripheral with encryption */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_req)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_req)
#elif defined(CONFIG_BT_CENTRAL)
/* Central with encryption */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_req)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_rsp)
#elif defined(CONFIG_BT_PERIPHERAL)
/* Peripheral with encryption */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_rsp)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(enc_req)
#endif /* !CONFIG_BT_PERIPHERAL */

#else /* !CONFIG_BT_CTLR_LE_ENC */
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PERIPHERAL)
/* Central and Peripheral without encryption */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_update_ind)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_update_ind)
#elif defined(CONFIG_BT_CENTRAL)
/* Central without encryption */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_update_ind)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(feature_rsp)
#elif defined(CONFIG_BT_PERIPHERAL)
/* Peripheral without encryption */
#define PDU_DC_CTRL_TX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(feature_rsp)
#define PDU_DC_CTRL_RX_SIZE_MAX       PDU_DATA_LLCTRL_LEN(conn_update_ind)
#endif /* !CONFIG_BT_PERIPHERAL */
#endif /* !CONFIG_BT_CTLR_LE_ENC */

#else /* !CONFIG_BT_CONN */
#define PDU_DC_CTRL_TX_SIZE_MAX       0U
#define PDU_DC_CTRL_RX_SIZE_MAX       0U
#endif /* !CONFIG_BT_CONN */

/* Link Layer header size of Data PDU. Assumes pdu_data is packed */
#define PDU_DC_LL_HEADER_SIZE  (offsetof(struct pdu_data, lldata))

/* Link Layer Max size of an empty PDU. TODO: Remove; only used in Nordic LLL */
#define PDU_EM_LL_SIZE_MAX     (PDU_DC_LL_HEADER_SIZE)

/* Link Layer header size of BIS PDU. Assumes pdu_bis is packed */
#define PDU_BIS_LL_HEADER_SIZE (offsetof(struct pdu_bis, payload))

/* Event Active Clock Jitter */
#define EVENT_CLOCK_JITTER_US   2
/* Event interframe timings */
#define EVENT_IFS_US            150
/* Standard allows 2 us timing uncertainty inside the event */
#define EVENT_IFS_MAX_US        (EVENT_IFS_US + EVENT_CLOCK_JITTER_US)
/* Specification defined Minimum AUX Frame Space (MAFS) */
#define EVENT_MAFS_US           300
/* Controller dependent MAFS minimum used to populate aux_offset */
#define EVENT_MAFS_MIN_US       MAX(EVENT_MAFS_US, PDU_ADV_AUX_OFFSET_MIN_US)
/* Standard allows 2 us timing uncertainty inside the event */
#define EVENT_MAFS_MAX_US       (EVENT_MAFS_US + EVENT_CLOCK_JITTER_US)
/* Controller defined back to back transmit MAFS for extended advertising */
#define EVENT_B2B_MAFS_US       (CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK_AFS)
/* Controller defined back to back transmit MAFS for periodic advertising */
#define EVENT_SYNC_B2B_MAFS_US  (CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK_AFS)
/* Minimum Subevent Space timings */
#define EVENT_MSS_US            150
/* Standard allows 2 us timing uncertainty inside the event */
#define EVENT_MSS_MAX_US        (EVENT_MSS_US + EVENT_CLOCK_JITTER_US)

/* Instant maximum value and maximum latency (or delta) past the instant */
#define EVENT_INSTANT_MAX         0xffff
#define EVENT_INSTANT_LATENCY_MAX 0x7fff

/* Channel Map Unused channels count minimum */
#define CHM_USED_COUNT_MIN     2U

/* Channel Map hop count minimum and maximum */
#define CHM_HOP_COUNT_MIN      5U
#define CHM_HOP_COUNT_MAX      16U

/* Offset Units field encoding */
#define OFFS_UNIT_BITS         13
#define OFFS_UNIT_30_US        30
#define OFFS_UNIT_300_US       300
#define OFFS_UNIT_VALUE_30_US  0
#define OFFS_UNIT_VALUE_300_US 1
/* Value specified in BT Spec. Vol 6, Part B, section 2.3.4.6 */
#define OFFS_ADJUST_US         2457600UL

/* Macros for getting offset/phy from pdu_adv_aux_ptr */
#define PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) ((aux_ptr)->offs_phy_packed[0] | \
					     (((aux_ptr)->offs_phy_packed[1] & 0x1F) << 8))
#define PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) (((aux_ptr)->offs_phy_packed[1] >> 5) & 0x07)

/* Macros for getting/setting offset/offset_units/offset_adjust from pdu_adv_sync_info */
#define PDU_ADV_SYNC_INFO_OFFSET_GET(si) ((si)->offs_packed[0] | \
					 (((si)->offs_packed[1] & 0x1F) << 8))
#define PDU_ADV_SYNC_INFO_OFFS_UNITS_GET(si) (((si)->offs_packed[1] >> 5) & 0x01)
#define PDU_ADV_SYNC_INFO_OFFS_ADJUST_GET(si) (((si)->offs_packed[1] >> 6) & 0x01)
#define PDU_ADV_SYNC_INFO_OFFS_SET(si, offs, offs_units, offs_adjust) \
	do { \
		(si)->offs_packed[0] = (offs) & 0xFF; \
		(si)->offs_packed[1] = (((offs) >> 8) & 0x1F) + \
				       (((offs_units) << 5) & 0x20) + \
				       (((offs_adjust) << 6) & 0x40); \
	} while (0)

/* Advertiser's Sleep Clock Accuracy Value */
#define SCA_500_PPM       500 /* 51 ppm to 500 ppm */
#define SCA_50_PPM        50  /* 0 ppm to 50 ppm */
#define SCA_VALUE_500_PPM 0   /* 51 ppm to 500 ppm */
#define SCA_VALUE_50_PPM  1   /* 0 ppm to 50 ppm */

/* Sleep Clock Accuracy, calculate drift in microseconds */
#define SCA_DRIFT_50_PPM_US(t)  (((t) * 50UL) / 1000000UL)
#define SCA_DRIFT_500_PPM_US(t) (((t) * 500UL) / 1000000UL)

/* transmitWindowDelay times (us) */
#define WIN_DELAY_LEGACY     1250
#define WIN_DELAY_UNCODED    2500
#define WIN_DELAY_CODED      3750

/* Channel Map Size */
#define PDU_CHANNEL_MAP_SIZE 5

/* Advertising Data */
#define PDU_ADV_DATA_HEADER_SIZE        2U
#define PDU_ADV_DATA_HEADER_LEN_SIZE    1U
#define PDU_ADV_DATA_HEADER_TYPE_SIZE   1U
#define PDU_ADV_DATA_HEADER_LEN_OFFSET  0U
#define PDU_ADV_DATA_HEADER_TYPE_OFFSET 1U
#define PDU_ADV_DATA_HEADER_DATA_OFFSET 2U

/* Advertising Data Types in ACAD */
#define PDU_ADV_DATA_TYPE_CHANNEL_MAP_UPDATE_IND 0x28

/*
 * Macros to return correct Data Channel PDU time
 * Note: formula is valid for 1M, 2M and Coded S8
 * see BT spec Version 5.1 Vol 6. Part B, chapters 2.1 and 2.2
 * for packet formats and thus lengths
 */

#define PHY_LEGACY   0
#define PHY_1M       BIT(0)
#define PHY_2M       BIT(1)
#define PHY_CODED    BIT(2)
#define PHY_FLAGS_S2 0
#define PHY_FLAGS_S8 BIT(0)

/* Macros for getting/setting did/sid from pdu_adv_adi */
#define PDU_ADV_ADI_DID_GET(adi) ((adi)->did_sid_packed[0] | \
					     (((adi)->did_sid_packed[1] & 0x0F) << 8))
#define PDU_ADV_ADI_SID_GET(adi) (((adi)->did_sid_packed[1] >> 4) & 0x0F)
#define PDU_ADV_ADI_SID_SET(adi, sid) (adi)->did_sid_packed[1] = (((sid) << 4) + \
								 ((adi)->did_sid_packed[1] & 0x0F))
#define PDU_ADV_ADI_DID_SID_SET(adi, did, sid) \
	do { \
		(adi)->did_sid_packed[0] = (did) & 0xFF; \
		(adi)->did_sid_packed[1] = (((did) >> 8) & 0x0F) + ((sid) << 4); \
	} while (0)

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define CODED_PHY_PREAMBLE_TIME_US       80
#define CODED_PHY_ACCESS_ADDRESS_TIME_US 256
#define CODED_PHY_CI_TIME_US             16
#define CODED_PHY_TERM1_TIME_US          24
#define CODED_PHY_CRC_SIZE               24
#define CODED_PHY_TERM2_SIZE             3

#define FEC_BLOCK1_US                    ((CODED_PHY_ACCESS_ADDRESS_TIME_US) + \
					  (CODED_PHY_CI_TIME_US) + \
					  (CODED_PHY_TERM1_TIME_US))

/* cs = 0, S2 coding scheme, use PHY_FLAGS_S2
 * cs = 1, S8 coding scheme, use PHY_FLAGS_S8
 *
 * Not using the term CI, Coding Indicator, where in the Spec its defined value
 * of 0 is S8 encoding, 1 is S2 encoding.
 */
#define FEC_BLOCK2_US(octets, mic, cs)   (((((PDU_HEADER_SIZE) + \
					     (octets) + \
					     (mic)) << 3) + \
					   (CODED_PHY_CRC_SIZE) + \
					   (CODED_PHY_TERM2_SIZE)) << \
					  (1 + (((cs) & 0x01) << 1)))

#define PDU_US(octets, mic, phy, cs)     (((phy) & PHY_CODED) ? \
					  ((CODED_PHY_PREAMBLE_TIME_US) + \
					   (FEC_BLOCK1_US) + \
					   FEC_BLOCK2_US((octets), (mic), \
							 (cs))) : \
					  (((PDU_PREAMBLE_SIZE(phy) + \
					     (PDU_ACCESS_ADDR_SIZE) + \
					     (PDU_HEADER_SIZE) + \
					     (octets) + \
					     (mic) + \
					     (PDU_CRC_SIZE)) << 3) / \
					   BIT(((phy) & 0x03) >> 1)))

#define PDU_MAX_US(octets, mic, phy)     PDU_US((octets), (mic), (phy), \
						PHY_FLAGS_S8)

#else /* !CONFIG_BT_CTLR_PHY_CODED */
#define PDU_US(octets, mic, phy, cs)     (((PDU_PREAMBLE_SIZE(phy) + \
					    (PDU_ACCESS_ADDR_SIZE) + \
					    (PDU_HEADER_SIZE) + \
					    (octets) + \
					    (mic) + \
					    (PDU_CRC_SIZE)) << 3) / \
					  BIT(((phy) & 0x03) >> 1))

#define PDU_MAX_US(octets, mic, phy)     PDU_US((octets), (mic), (phy), 0)
#endif /* !CONFIG_BT_CTLR_PHY_CODED */

#define PDU_DC_MAX_US(octets, phy)   PDU_MAX_US((octets), (PDU_MIC_SIZE), (phy))

#define PDU_DC_US(octets, mic, phy, cs)  PDU_US((octets), (mic), (phy), (cs))

#define PDU_AC_MAX_US(octets, phy)   PDU_MAX_US((octets), 0, (phy))

#define PDU_AC_US(octets, phy, cs)   PDU_US((octets), 0, (phy), (cs))

#define PDU_BIS_MAX_US(octets, enc, phy) PDU_MAX_US((octets), \
						    ((enc) ? \
						     (PDU_MIC_SIZE) : 0), \
						    (phy))

#define PDU_BIS_US(octets, enc, phy, s8) PDU_US((octets), \
						((enc) ? (PDU_MIC_SIZE) : 0), \
						(phy), (s8))

#define PDU_CIS_MAX_US(octets, enc, phy) PDU_MAX_US((octets), \
						    ((enc) ? \
						     (PDU_MIC_SIZE) : 0), \
						    (phy))
#define PDU_CIS_OFFSET_MIN_US 500U

struct pdu_adv_adv_ind {
	uint8_t addr[BDADDR_SIZE];
	uint8_t data[PDU_AC_LEG_DATA_SIZE_MAX];
} __packed;

struct pdu_adv_direct_ind {
	uint8_t adv_addr[BDADDR_SIZE];
	uint8_t tgt_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_scan_rsp {
	uint8_t addr[BDADDR_SIZE];
	uint8_t data[PDU_AC_LEG_DATA_SIZE_MAX];
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
		uint8_t  chan_map[PDU_CHANNEL_MAP_SIZE];
#ifdef CONFIG_LITTLE_ENDIAN
		uint8_t  hop:5;
		uint8_t  sca:3;
#else
		uint8_t  sca:3;
		uint8_t  hop:5;
#endif /* CONFIG_LITTLE_ENDIAN */

	} __packed;
} __packed;

struct pdu_adv_ext_hdr {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t adv_addr:1;
	uint8_t tgt_addr:1;
	uint8_t cte_info:1;
	uint8_t adi:1;
	uint8_t aux_ptr:1;
	uint8_t sync_info:1;
	uint8_t tx_pwr:1;
	uint8_t rfu:1;
#else
	uint8_t rfu:1;
	uint8_t tx_pwr:1;
	uint8_t sync_info:1;
	uint8_t aux_ptr:1;
	uint8_t adi:1;
	uint8_t cte_info:1;
	uint8_t tgt_addr:1;
	uint8_t adv_addr:1;
#endif /* CONFIG_LITTLE_ENDIAN */
	uint8_t data[0];
} __packed;

struct pdu_adv_com_ext_adv {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t ext_hdr_len:6;
	uint8_t adv_mode:2;
#else
	uint8_t adv_mode:2;
	uint8_t ext_hdr_len:6;
#endif /* CONFIG_LITTLE_ENDIAN */
	union {
		struct pdu_adv_ext_hdr ext_hdr;
		uint8_t ext_hdr_adv_data[0];
	};
} __packed;

enum pdu_adv_mode {
	EXT_ADV_MODE_NON_CONN_NON_SCAN = 0x00,
	EXT_ADV_MODE_CONN_NON_SCAN = 0x01,
	EXT_ADV_MODE_NON_CONN_SCAN = 0x02,
};

#define PDU_ADV_SID_COUNT 16

struct pdu_adv_adi {
	/* did:12
	 * sid:4
	 * NOTE: This layout as bitfields is not portable for BE using
	 * endianness conversion macros.
	 */
	uint8_t did_sid_packed[2];
} __packed;

struct pdu_adv_aux_ptr {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t  chan_idx:6;
	uint8_t  ca:1;
	uint8_t  offs_units:1;
#else
	uint8_t  offs_units:1;
	uint8_t  ca:1;
	uint8_t  chan_idx:6;
#endif /* CONFIG_LITTLE_ENDIAN */
	/* offs:13
	 * phy:3
	 * NOTE: This layout as bitfields is not portable for BE using
	 * endianness conversion macros.
	 */
	uint8_t  offs_phy_packed[2];
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
	EXT_ADV_AUX_PHY_LE_1M    = 0x00,
	EXT_ADV_AUX_PHY_LE_2M    = 0x01,
	EXT_ADV_AUX_PHY_LE_CODED = 0x02,
};

struct pdu_adv_sync_info {
	/* offs:13
	 * offs_units:1
	 * offs_adjust:1
	 * rfu:1
	 * NOTE: This layout as bitfields is not portable for BE using
	 * endianness conversion macros.
	 */
	uint8_t  offs_packed[2];
	uint16_t interval;
	uint8_t  sca_chm[PDU_CHANNEL_MAP_SIZE];
	uint8_t  aa[4];
	uint8_t  crc_init[3];
	uint16_t evt_cntr;
} __packed;

#define PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET 4
#define PDU_SYNC_INFO_SCA_CHM_SCA_BIT_POS     5
#define PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK    \
		(0x07 << (PDU_SYNC_INFO_SCA_CHM_SCA_BIT_POS))

struct pdu_adv_sync_chm_upd_ind {
	uint8_t  chm[PDU_CHANNEL_MAP_SIZE];
	uint16_t instant;
} __packed;

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
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t type:4;
	uint8_t rfu:1;
	uint8_t chan_sel:1;
	uint8_t tx_addr:1;
	uint8_t rx_addr:1;
#else
	uint8_t rx_addr:1;
	uint8_t tx_addr:1;
	uint8_t chan_sel:1;
	uint8_t rfu:1;
	uint8_t type:4;
#endif /* CONFIG_LITTLE_ENDIAN */

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
	PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG = 0x0E,
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
	PDU_DATA_LLCTRL_TYPE_CTE_REQ = 0x1A,
	PDU_DATA_LLCTRL_TYPE_CTE_RSP = 0x1B,
	PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_REQ = 0x1D,
	PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_RSP = 0x1E,
	PDU_DATA_LLCTRL_TYPE_CIS_REQ = 0x1F,
	PDU_DATA_LLCTRL_TYPE_CIS_RSP = 0x20,
	PDU_DATA_LLCTRL_TYPE_CIS_IND = 0x21,
	PDU_DATA_LLCTRL_TYPE_CIS_TERMINATE_IND = 0x22,
	PDU_DATA_LLCTRL_TYPE_UNUSED = 0xFF
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
	uint8_t  chm[PDU_CHANNEL_MAP_SIZE];
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

struct pdu_data_llctrl_per_init_feat_xchg {
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

/*
 * According to Spec Core v5.3, section 2.4.2.17
 * LL_CONNECTION_PARAM_RSP and LL_CONNECTION_PARAM_REQ are identical
 * This is utilized in pdu encode/decode, and for this is needed a common struct
 */
struct pdu_data_llctrl_conn_param_req_rsp_common {
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

/*
 * According to Spec Core v5.3, section 2.4.2.21
 * LL_LENGTH_REQ and LL_LENGTH_RSP are identical
 * This is utilized in pdu encode/decode, and for this is needed a common struct
 */
struct pdu_data_llctrl_length_req_rsp_common {
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
	uint8_t  c_to_p_phy;
	uint8_t  p_to_c_phy;
	uint16_t instant;
} __packed;

struct pdu_data_llctrl_min_used_chans_ind {
	uint8_t phys;
	uint8_t min_used_chans;
} __packed;

struct pdu_data_llctrl_cte_req {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t min_cte_len_req : 5;
	uint8_t rfu : 1;
	uint8_t cte_type_req : 2;
#else
	uint8_t cte_type_req : 2;
	uint8_t rfu : 1;
	uint8_t min_cte_len_req : 5;
#endif /* CONFIG_LITTLE_ENDIAN */
} __packed;

struct pdu_data_llctrl_cte_rsp {
	/* no members */
} __packed;

struct pdu_data_llctrl_clock_accuracy_req {
	uint8_t sca;
} __packed;

struct pdu_data_llctrl_clock_accuracy_rsp {
	uint8_t sca;
} __packed;

struct pdu_data_llctrl_cis_req {
	uint8_t  cig_id;
	uint8_t  cis_id;
	uint8_t  c_phy;
	uint8_t  p_phy;
	/* c_max_sdu:12
	 * rfu:3
	 * framed:1
	 * NOTE: This layout as bitfields is not portable for BE using
	 * endianness conversion macros.
	 */
	uint8_t  c_max_sdu_packed[2];
	/* p_max_sdu:12
	 * rfu:4
	 * NOTE: This layout as bitfields is not portable for BE using
	 * endianness conversion macros.
	 */
	uint8_t  p_max_sdu[2];
	uint8_t  c_sdu_interval[3];
	uint8_t  p_sdu_interval[3];
	uint16_t c_max_pdu;
	uint16_t p_max_pdu;
	uint8_t  nse;
	uint8_t  sub_interval[3];
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t  c_bn:4;
	uint8_t  p_bn:4;
#else
	uint8_t  p_bn:4;
	uint8_t  c_bn:4;
#endif /* CONFIG_LITTLE_ENDIAN */
	uint8_t  c_ft;
	uint8_t  p_ft;
	uint16_t iso_interval;
	uint8_t  cis_offset_min[3];
	uint8_t  cis_offset_max[3];
	uint16_t conn_event_count;
} __packed;

struct pdu_data_llctrl_cis_rsp {
	uint8_t  cis_offset_min[3];
	uint8_t  cis_offset_max[3];
	uint16_t conn_event_count;
} __packed;

struct pdu_data_llctrl_cis_ind {
	uint8_t  aa[4];
	uint8_t  cis_offset[3];
	uint8_t  cig_sync_delay[3];
	uint8_t  cis_sync_delay[3];
	uint16_t conn_event_count;
} __packed;

struct pdu_data_llctrl_cis_terminate_ind {
	uint8_t  cig_id;
	uint8_t  cis_id;
	uint8_t  error_code;
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
		struct pdu_data_llctrl_per_init_feat_xchg per_init_feat_xchg;
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
		struct pdu_data_llctrl_cte_req cte_req;
		struct pdu_data_llctrl_cte_rsp cte_rsp;
		struct pdu_data_llctrl_clock_accuracy_req clock_accuracy_req;
		struct pdu_data_llctrl_clock_accuracy_rsp clock_accuracy_rsp;
		struct pdu_data_llctrl_cis_req cis_req;
		struct pdu_data_llctrl_cis_rsp cis_rsp;
		struct pdu_data_llctrl_cis_ind cis_ind;
		struct pdu_data_llctrl_cis_terminate_ind cis_terminate_ind;
	} __packed;
} __packed;

#define PDU_DATA_LLCTRL_LEN(type) (offsetof(struct pdu_data_llctrl, type) + \
				   sizeof(struct pdu_data_llctrl_ ## type))

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
struct profile {
	uint16_t lcur;
	uint16_t lmin;
	uint16_t lmax;
	uint16_t cur;
	uint16_t min;
	uint16_t max;
	uint16_t radio;
	uint16_t lll;
	uint16_t ull_high;
	uint16_t ull_low;
	uint8_t  radio_ticks;
	uint8_t  lll_ticks;
	uint8_t  ull_high_ticks;
	uint8_t  ull_low_ticks;
} __packed;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

struct pdu_data {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t ll_id:2;
	uint8_t nesn:1;
	uint8_t sn:1;
	uint8_t md:1;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	uint8_t cp:1;
	uint8_t rfu:2;
#else
	uint8_t rfu:3;
#endif
#else
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	uint8_t rfu:2;
	uint8_t cp:1;
#else
	uint8_t rfu:3;
#endif
	uint8_t md:1;
	uint8_t sn:1;
	uint8_t nesn:1;
	uint8_t ll_id:2;
#endif /* CONFIG_LITTLE_ENDIAN */

	uint8_t len;

	struct pdu_data_vnd_octet3 octet3;

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

/* Generic ISO pdu, could be CIS or BIS
 * To be used when referring to component without knowing CIS or BIS type
 */
struct pdu_iso {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t ll_id:2;
	uint8_t hdr_other:6;
#else
	uint8_t hdr_other:6;
	uint8_t ll_id:2;
#endif /* CONFIG_LITTLE_ENDIAN */

	uint8_t len;

	struct pdu_iso_vnd_octet3 octet3;

	uint8_t payload[0];
} __packed;

/* ISO SDU segmentation header */
#define PDU_ISO_SEG_HDR_SIZE 2
#define PDU_ISO_SEG_TIMEOFFSET_SIZE 3

struct pdu_iso_sdu_sh {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t sc:1;
	uint8_t cmplt:1;
	uint8_t rfu:6;

	uint8_t len;
	/* Note, timeoffset only available in first segment of sdu */
	uint32_t timeoffset:24;
	uint32_t payload:8;
#else
	uint8_t rfu:6;
	uint8_t cmplt:1;
	uint8_t sc:1;

	uint8_t len;

	/* Note, timeoffset only available in first segment of sdu */
	uint32_t timeoffset:24;
	uint32_t payload:8;
#endif /* CONFIG_LITTLE_ENDIAN */
} __packed;

enum pdu_cis_llid {
	/** Unframed complete or end fragment */
	PDU_CIS_LLID_COMPLETE_END = 0x00,
	/** Unframed start or continuation fragment */
	PDU_CIS_LLID_START_CONTINUE = 0x01,
	/** Framed; one or more segments of a SDU */
	PDU_CIS_LLID_FRAMED = 0x02
};

struct pdu_cis {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t ll_id:2;
	uint8_t nesn:1;
	uint8_t sn:1;
	uint8_t cie:1;
	uint8_t rfu0:1;
	uint8_t npi:1;
	uint8_t rfu1:1;
#else
	uint8_t rfu1:1;
	uint8_t npi:1;
	uint8_t rfu0:1;
	uint8_t cie:1;
	uint8_t sn:1;
	uint8_t nesn:1;
	uint8_t ll_id:2;
#endif /* CONFIG_LITTLE_ENDIAN */

	uint8_t len;

	struct pdu_cis_vnd_octet3 octet3;

	uint8_t payload[0];
} __packed;

enum pdu_big_ctrl_type {
	PDU_BIG_CTRL_TYPE_CHAN_MAP_IND = 0x00,
	PDU_BIG_CTRL_TYPE_TERM_IND = 0x01,
};

struct pdu_big_ctrl_chan_map_ind {
	uint8_t  chm[PDU_CHANNEL_MAP_SIZE];
	uint16_t instant;
} __packed;

struct pdu_big_ctrl_term_ind {
	uint8_t  reason;
	uint16_t instant;
} __packed;


struct pdu_big_ctrl {
	uint8_t opcode;
	union {
		uint8_t ctrl_data[0];
		struct pdu_big_ctrl_chan_map_ind chan_map_ind;
		struct pdu_big_ctrl_term_ind term_ind;
	} __packed;
} __packed;

enum pdu_bis_llid {
	/** Unframed complete or end fragment */
	PDU_BIS_LLID_COMPLETE_END = 0x00,
	/** Unframed start or continuation fragment */
	PDU_BIS_LLID_START_CONTINUE = 0x01,
	/** Framed; one or more segments of a SDU */
	PDU_BIS_LLID_FRAMED = 0x02,
	/** BIG Control PDU */
	PDU_BIS_LLID_CTRL = 0x03,
};

struct pdu_bis {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t ll_id:2;
	uint8_t cssn:3;
	uint8_t cstf:1;
	uint8_t rfu:2;
#else
	uint8_t rfu:2;
	uint8_t cstf:1;
	uint8_t cssn:3;
	uint8_t ll_id:2;
#endif /* CONFIG_LITTLE_ENDIAN */

	uint8_t len;

	struct pdu_bis_vnd_octet3 octet3;

	union {
		uint8_t payload[0];
		struct pdu_big_ctrl ctrl;
	} __packed;
} __packed;

struct pdu_big_info {
	/* offs:14          [0].0 - [1].5
	 * offs_units:1     [1].6
	 * iso_interval:12  [1].7 - [3].2
	 * num_bis:5        [3].3 - [3].7
	 */
	uint8_t bi_packed_0_3[4];

	/* nse:5            [0].0 - [0].4
	 * bn:3             [0].5 - [0].7
	 * sub_interval:20  [1].0 - [3].3
	 * pto:4            [3].4 - [3].7
	 */
	uint8_t bi_packed_4_7[4];

	/* spacing:20       [0].0 - [2].3
	 * irc:4            [2].4 - [2].7
	 */
	uint8_t bi_packed_8_11[3];

	uint8_t max_pdu;

	uint8_t  rfu;

	uint32_t seed_access_addr;

	/* sdu_interval:20  [0].0 - [2].3
	 * max_sdu:12;      [2].4 - [3].7
	 */
	uint8_t  sdu_packed[4];

	uint16_t base_crc_init;

	uint8_t chm_phy[PDU_CHANNEL_MAP_SIZE]; /* 37 bit chm; 3 bit phy */
	uint8_t payload_count_framing[5]; /* 39 bit count; 1 bit framing */

	uint8_t giv[8]; /* encryption required */
	uint8_t gskd[16]; /* encryption required */
} __packed;
#define PDU_BIG_INFO_CLEARTEXT_SIZE offsetof(struct pdu_big_info, giv)
#define PDU_BIG_INFO_ENCRYPTED_SIZE sizeof(struct pdu_big_info)
#define PDU_BIG_BN_MAX              0x07
#define PDU_BIG_PAYLOAD_COUNT_MAX   28

#define PDU_BIG_INFO_OFFS_GET(bi) \
	util_get_bits(&(bi)->bi_packed_0_3[0], 0, 14)
#define PDU_BIG_INFO_OFFS_UNITS_GET(bi) \
	util_get_bits(&(bi)->bi_packed_0_3[1], 6, 1)
#define PDU_BIG_INFO_ISO_INTERVAL_GET(bi) \
	util_get_bits(&(bi)->bi_packed_0_3[1], 7, 12)
#define PDU_BIG_INFO_NUM_BIS_GET(bi) \
	util_get_bits(&(bi)->bi_packed_0_3[3], 3, 5)
#define PDU_BIG_INFO_NSE_GET(bi) \
	util_get_bits(&(bi)->bi_packed_4_7[0], 0, 5)
#define PDU_BIG_INFO_BN_GET(bi) \
	util_get_bits(&(bi)->bi_packed_4_7[0], 5, 3)
#define PDU_BIG_INFO_SUB_INTERVAL_GET(bi) \
	util_get_bits(&(bi)->bi_packed_4_7[1], 0, 20)
#define PDU_BIG_INFO_PTO_GET(bi) \
	util_get_bits(&(bi)->bi_packed_4_7[3], 4, 4)
#define PDU_BIG_INFO_SPACING_GET(bi) \
	util_get_bits(&(bi)->bi_packed_8_11[0], 0, 20)
#define PDU_BIG_INFO_IRC_GET(bi) \
	util_get_bits(&(bi)->bi_packed_8_11[2], 4, 4)
#define PDU_BIG_INFO_SDU_INTERVAL_GET(bi) \
	util_get_bits(&(bi)->sdu_packed[0], 0, 20)
#define PDU_BIG_INFO_MAX_SDU_GET(bi) \
	util_get_bits(&(bi)->sdu_packed[2], 4, 12)

#define PDU_BIG_INFO_OFFS_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_0_3[0], 0, 14, val)
#define PDU_BIG_INFO_OFFS_UNITS_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_0_3[1], 6, 1, val)
#define PDU_BIG_INFO_ISO_INTERVAL_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_0_3[1], 7, 12, val)
#define PDU_BIG_INFO_NUM_BIS_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_0_3[3], 3, 5, val)
#define PDU_BIG_INFO_NSE_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_4_7[0], 0, 5, val)
#define PDU_BIG_INFO_BN_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_4_7[0], 5, 3, val)
#define PDU_BIG_INFO_SUB_INTERVAL_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_4_7[1], 0, 20, val)
#define PDU_BIG_INFO_PTO_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_4_7[3], 4, 4, val)
#define PDU_BIG_INFO_SPACING_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_8_11[0], 0, 20, val)
#define PDU_BIG_INFO_IRC_SET(bi, val) \
	util_set_bits(&(bi)->bi_packed_8_11[2], 4, 4, val)
#define PDU_BIG_INFO_SDU_INTERVAL_SET(bi, val) \
	util_set_bits(&(bi)->sdu_packed[0], 0, 20, val)
#define PDU_BIG_INFO_MAX_SDU_SET(bi, val) \
	util_set_bits(&(bi)->sdu_packed[2], 4, 12, val)

struct pdu_dtm {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t type:4;
	uint8_t rfu0:1;
#if defined(CONFIG_BT_CTLR_DF_CTE_TX)
	uint8_t cp:1;
	uint8_t rfu1:2;
#else
	uint8_t rfu1:3;
#endif
#else
#if defined(CONFIG_BT_CTLR_DF_CTE_TX)
	uint8_t rfu1:2;
	uint8_t cp:1;
#else
	uint8_t rfu1:3;
#endif
	uint8_t rfu0:1;
	uint8_t type:4;
#endif /* CONFIG_LITTLE_ENDIAN */

	uint8_t len;

	struct pdu_data_vnd_octet3 octet3;

	uint8_t payload[0];
} __packed;

/* Direct Test Mode maximum payload size */
#define PDU_DTM_PAYLOAD_SIZE_MAX 255
