/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_TXRX_H__
#define __WIFI_TXRX_H__

#include "main.h"

#define SPRD_AP_CP_DRAM_MAP_BASE (0x40400000)

#define SPRD_AP_DRAM_BEGIN (0x00100000)
#define SPRD_AP_DRAM_END (0x001FFFFF)

#define SPRDWL_PHYS_LEN (5)

#define ADDR_LEN (5)

#define MAX_RX_ADDR_COUNT CONFIG_WIFI_UWP_MAX_RX_ADDR_COUNT

#define SPRD_CP_DRAM_BEGIN (SPRD_AP_DRAM_BEGIN + SPRD_AP_CP_DRAM_MAP_BASE)
#define SPRD_CP_DRAM_END (SPRD_AP_DRAM_END + SPRD_AP_CP_DRAM_MAP_BASE)

#define SPRD_AP_TO_CP_ADDR(addr)					      \
	do {								      \
		if (addr >= SPRD_AP_DRAM_BEGIN && addr <= SPRD_AP_DRAM_END) { \
			addr = addr + SPRD_AP_CP_DRAM_MAP_BASE; }	      \
	} while (0)

#define SPRD_CP_TO_AP_ADDR(addr)					      \
	do {								      \
		if (addr >= SPRD_CP_DRAM_BEGIN && addr <= SPRD_CP_DRAM_END) { \
			addr = addr - SPRD_AP_CP_DRAM_MAP_BASE; }	      \
	} while (0)

struct sprdwl_common_hdr {
	u8_t type : 3;
	u8_t direction_ind : 1;
	u8_t buffer_type : 1;
	u8_t interface : 3;
};

struct tx_ctrl {
	/* 1:need HW to do checksum */
	unsigned char checksum_offload : 1;
	/* 0:udp, 1:tcp */
	unsigned char checksum_type : 1;
	/* 1:use SW rate,no aggregation 0:normal */
	unsigned char sw_rate : 1;
	/* WDS frame */
	unsigned char wds : 1;
	/**
	 * 1:frame sent from SWQ to MH,
	 * 0:frame sent from TXQ to MH,
	 * default:0
	 */
	unsigned char swq_flag : 1;
	unsigned char rsvd : 2;
	unsigned char pcie_mh_readcomp : 1;
};

/* NOTE: MUST not modify, defined by HW */
/* It still change now */
struct rx_msdu_desc {
	/* WORD7 */
	u32_t host_type : 4; /* indicate data/event/rsp, host driver used */
	u32_t ctx_id : 4; /* indicate hw mac address index */
	u32_t msdu_offset : 8; /* 802.3 header offset from msdu_dscr_header */
	u32_t msdu_len : 16; /* len of 802.3 frame */
	/* WORD8
	 * base buffer addr of this msdu
	 * low 32 bit
	 */
	u32_t curr_buff_base_addr_l;
	/* WORD9 */
	union {
		/* base buffer addr of
		 * this msdu high 8 bit
		 */
		u8_t curr_buff_base_addr_h;
		/* sw use, used in short
		 * pkt process in SDIO mode
		 */
		u8_t short_pkt_num;
	};
	u8_t msdu_index_of_mpdu; /* msdu index of mpdu */
	/* Indicate whether this msdu is
	 * the first msdu in buffer.
	 */
	u16_t first_msdu_of_buff : 1;
	/* Indicate whether this msdu is
	 * the last msdu in buffer.
	 */
	u16_t last_msdu_of_buff : 1;
	u16_t rsvd1 : 2; /* reserved */
	/* Indicate whether this msdu is
	 * the first msdu of mpdu.
	 */
	u16_t first_msdu_of_mpdu : 1;
	/* Indicate whether this msdu is
	 * the last msdu of mpdu.
	 */
	u16_t last_msdu_of_mpdu : 1;
	u16_t null_frame_flag : 1; /* indicate this msdu is null */
	u16_t qos_null_frame_flag : 1; /* indicate this msdu is qos null */
	/* Indicate whether the buffer this msdu
	 * is the first buff of mpdu.
	 */
	u16_t first_buff_of_mpdu : 1;
	/* Indicate whether the buffer this msdu
	 * is the last buff of mpdu.
	 */
	u16_t last_buff_of_mpdu : 1;
	u16_t sta_lut_valid : 1; /* Indicate if find hw sta lut. */
	/* hw sta lut index, valid only
	 * when sta_lut_valid is true
	 */
	u16_t sta_lut_index : 5;
	/* WORD 10 */
	u32_t more_data_bit : 1; /* more data bit in mac header */
	u32_t eosp_bit : 1; /* eosp bit in mac header */
	u32_t pm_bit : 1; /* pm bit in mac header */
	u32_t bc_mc_w2w_flag : 1; /* bc/mc wlan2wlan flag */
	u32_t bc_mc_flag : 1; /* bc/mc flag */
	u32_t uc_w2w_flag : 1; /* uc wlan2wlan flag */
	u32_t eapol_flag : 1; /* eapol flag */
	u32_t vlan_type_flag : 1; /* vlan pkt */
	/* indicate if hw find snap header
	 * (0xAA 0xAA 0x03 0x00 0x00 0x00)
	 * (0xAA 0xAA 0x03 0x00 0x00 0xFB)
	 */
	u32_t snap_hdr_present : 1;
	u32_t snap_hdr_type : 1; /* snap header type: rfc1042/rfc896(802.1h) */
	/* Indicate if this msdu is
	 * received in rx ba session period.
	 */
	u32_t ba_session_flag : 1;
	u32_t ampdu_flag : 1; /* indicate if this msdu is in ampdu */
	u32_t amsdu_flag : 1; /* indicate if this msdu is in amsdu */
	u32_t qos_flag : 1; /* qos flag */
	u32_t rsvd2 : 2; /* reserved */
	u32_t tid : 4; /* TID */
	u32_t seq_num : 12; /* sequence number */
	/* WORD11 */
	u32_t pn_l; /* PN, low 4 bytes, hw has got real PN value */
	/* WORD12 */
	u32_t pn_h : 16; /* PN, high 2 bytes */
	u32_t frag_num : 4; /* fragment number in mac header */
	u32_t more_frag_bit : 1; /* more fragment bit in mac header */
	u32_t retry_bit : 1; /* retransmission bit in mac header */
	u32_t rsvd3 : 2; /* reserved */
	u32_t cipher_type : 4; /* cipher type */
	u32_t rsvd4 : 3; /* reserved */
	/* In PCIE mode, indicate if data has been
	 * transferred from HW to ap, host driver use.
	 */
	u32_t data_write_done : 1;
	/* WORD13 */
	u32_t rsvd5; /* reserved */
} __packed;

/* NOTE: MUST not modify, defined by HW */
struct rx_mh_desc {
	/* WORD0 */
	u32_t next_msdu_ptr_l; /* ptr to next msdu low 32 bit */
	/* WORD1 */
	u32_t next_msdu_ptr_h : 8; /* ptr to next msdu high 8 bit */
	u32_t transfer_len : 16; /* SDIO HW use */
	u32_t offset_for_sdio : 8; /* SDIO HW use, default:0 */
	/* WORD2 */
	u32_t tcp_checksum_offset : 12; /* HW use */
	u32_t tcp_checksum_len : 16; /* HW use */
	u32_t tcp_checksum_en : 1; /* HW use */
	u32_t rsvd1 : 3; /* reserved */
	/* WORD3 */
	u32_t tcp_hw_checksum : 16; /* MAC HW fill, host driver use */
	/* Indicate whether this msdu
	 * is the last procq msdu in buffer.
	 */
	u32_t last_procq_msdu_of_buff : 1;
	u32_t rsvd2 : 7; /* reserved */
	u32_t filter_status : 6; /* used in filter queue */
	/* Indicate if this msdu is received
	 * in STA ps state.
	 */
	u32_t msdu_sta_ps_flag : 1;
	/* indicate if this msdu is
	 * a filter msdu
	 */
	u32_t filter_flag : 1;
	/* WORD4 */
	u32_t data_rate : 8; /* data rate from PA RX DESCRIPTOR */
	u32_t rss1 : 8; /* RSS1 from PA RX DESCRIPTOR */
	u32_t rss2 : 8; /* RSS2 from PA RX DESCRIPTOR */
	u32_t snr1 : 8; /* SNR1 from PA RX DESCRIPTOR */
	/* WORD5 */
	u32_t snr2 : 8; /* SNR2 from PA RX DESCRIPTOR */
	u32_t snr_combo : 8; /* SNR-COMBO from PA RX DESCRIPTOR */
	u32_t snr_l : 8; /* SNR-L from PA RX DESCRIPTOR */
	u32_t rsvd3 : 8; /* reserved */
	/* WORD6 */
	u32_t phy_rx_mode; /* PHY RX MODE from PA RX DESCRIPTOR */
} __packed;

struct tx_msdu_dscr {
	unsigned int next_buf_addr_low;
	unsigned char next_buf_addr_high;
	struct sprdwl_common_hdr common;
	unsigned char offset;
	struct {
		/* 1:need HW to do checksum. */
		unsigned char checksum_offload : 1;
		/* 0:udp, 1:tcp. */
		unsigned char checksum_type : 1;
		/* 1:use SW rate,no aggregation 0:normal. */
		unsigned char sw_rate : 1;
		/* WDS frame. */
		unsigned char wds : 1;
		/* 1:frame sent from SWQ to MH,
		 * 0:frame sent from TXQ to MH,
		   default:0.
		 */
		unsigned char swq_flag : 1;
		unsigned char rsvd : 2;
		/* Used by PCIe address buffer, need set default: 1. */
		unsigned char pcie_mh_readcomp : 1;
	} tx_ctrl;
	unsigned short pkt_len;
	struct {
		unsigned char msdu_tid : 4;
		unsigned char mac_data_offset : 4;
		unsigned char sta_lut_idx : 5;
		unsigned char rsvd : 1;
		unsigned char encrypt_bypass : 1;
		unsigned char ap_buf_flag : 1;
	} buffer_info;
	unsigned char color_bit : 2;
	unsigned short rsvd : 14;
	unsigned short tcp_udp_header_offset;
} __packed;

struct txc_addr_buff {
	unsigned char offset;
	struct  tx_ctrl tx_ctrl;
	unsigned short number;
	unsigned short rsvd;
	unsigned char data[0];
} __packed;

struct txc {
	struct sprdwl_common_hdr common;
	struct txc_addr_buff txc_addr;
} __packed;

struct rx_empty_buff {
	struct sprdwl_common_hdr common;
	unsigned char type;
	unsigned char num;
	unsigned char addr[MAX_RX_ADDR_COUNT][ADDR_LEN];
} __packed;

/* 0 for cmd, 1 for event, 2 for data, 3 for mh data. */
enum sprdwl_head_type {
	SPRDWL_TYPE_CMD = 0,
	SPRDWL_TYPE_EVENT,
	SPRDWL_TYPE_DATA,
	SPRDWL_TYPE_DATA_HIGH_SPEED,
	SPRDWL_TYPE_DATA_SPECIAL,
	SPRDWL_TYPE_PKT_LOG,
};

struct hw_addr_buff_t {
	struct sprdwl_common_hdr common;
	unsigned char offset;
	struct  tx_ctrl tx_ctrl;
	unsigned short number;
	unsigned short rsvd;
	unsigned char pcie_addr[ADDR_LEN];
} __packed;


struct rxc_ddr_addr_trans_t {
	unsigned int timestamp;
	unsigned short seq_num;
	unsigned char tlv_num;
	unsigned char type;
	unsigned char num;
	unsigned char addr_addr[0][ADDR_LEN];
} __packed;

struct rxc {
	struct sprdwl_common_hdr common;
	struct rxc_ddr_addr_trans_t rxc_addr;
} __packed;

enum {
	TRANS_FOR_RX_PATH = 1,
	TRANS_FOR_TX_PATH = 0,
};

enum {
	EMPTY_DDR_BUFF = 0,
	DDR_BUFF_ERROR = 1,
};


int wifi_tx_cmd(void *data, int len);
int wifi_txrx_init(struct wifi_priv *priv);
int wifi_alloc_rx_buf(int num);
int wifi_tx_data(void *data, int len);
int wifi_release_rx_buf(void);

#endif /* __WIFI_TXRX_H__ */
