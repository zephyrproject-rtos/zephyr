/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_BRCMFMAC_PRIV_H_
#define ZEPHYR_DRIVERS_WIFI_BRCMFMAC_PRIV_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdio.h>

/* === SBSDIO function-1 misc-register layout =================================
 *
 * F1 exposes a 32 KiB sliding window onto the chip's AXI/SB backplane;
 * the SBADDR* registers set the window base, then accesses to func1
 * offset [0..0x7FFF] map to backplane[base + offset]. Setting bit 15
 * in the offset (SB_ACCESS_2_4B_FLAG) makes a 4-byte access at the
 * chip side, not four single-byte accesses.
 */
#define SBSDIO_FUNC1_SBADDRLOW          0x1000A
#define SBSDIO_FUNC1_SBADDRMID          0x1000B
#define SBSDIO_FUNC1_SBADDRHIGH         0x1000C
#define SBSDIO_FUNC1_CHIPCLKCSR         0x1000E
#define SBSDIO_FUNC1_SDIOPULLUP         0x1000F
#define SBSDIO_SB_OFT_ADDR_MASK         0x07FFF
#define SBSDIO_SB_ACCESS_2_4B_FLAG      0x08000
#define SBSDIO_SBWINDOW_MASK            0xFFFF8000
#define SBSDIO_SB_OFT_ADDR_LIMIT        0x8000

/* CHIPCLKCSR bits. */
#define SBSDIO_FORCE_ALP                0x01
#define SBSDIO_ALP_AVAIL_REQ            0x08
#define SBSDIO_HT_AVAIL_REQ             0x10
#define SBSDIO_FORCE_HW_CLKREQ_OFF      0x20
#define SBSDIO_ALP_AVAIL                0x40
#define SBSDIO_HT_AVAIL                 0x80
#define SBSDIO_AVBITS                   (SBSDIO_ALP_AVAIL | SBSDIO_HT_AVAIL)
#define BRCMF_INIT_CLKCTL1              (SBSDIO_FORCE_HW_CLKREQ_OFF | \
					 SBSDIO_ALP_AVAIL_REQ)

/* Chipcommon enumeration base + chipid register layout. */
#define BRCMF_SI_ENUM_BASE              0x18000000
#define CID_ID_MASK                     0x0000FFFF
#define CID_REV_MASK                    0x000F0000
#define CID_REV_SHIFT                   16
#define CID_TYPE_MASK                   0xF0000000
#define CID_TYPE_SHIFT                  28

/* Broadcom/Cypress chipcommon[0] chip-ID values. The chip reports these
 * as the low 16 bits of CHIPID; brcmfmac uses them to select chip-specific
 * code paths (firmware blob, RAM layout, SOCRAM bank-3 quirk). Mirrors
 * Linux brcm_hw_ids.h.
 */
#define BRCM_CC_43430_CHIP_ID           0xa9a6   /* BCM43430 / BCM43436 */
#define BRCM_CC_43439_CHIP_ID           0xa9af   /* CYW43439 */
#define BRCM_CC_4345_CHIP_ID            0x4345   /* BCM43458F */

/* BCMA core IDs (subset; full list in Linux include/linux/bcma/bcma.h). */
#define BCMA_CORE_INTERNAL_MEM          0x80E    /* SOCRAM */
#define BCMA_CORE_80211                 0x812    /* D11 MAC */
#define BCMA_CORE_PMU                   0x827
#define BCMA_CORE_SDIO_DEV              0x829
#define BCMA_CORE_ARM_CM3               0x82A
#define BCMA_CORE_ARM_CR4               0x83E
#define BCMA_CORE_GCI                   0x840

/* SDIO core register offset within data base. */
#define SDPCMD_INTSTATUS                0x20     /* W1C ack-all = 0xFFFFFFFF */
#define SDPCMD_HOSTINTMASK              0x24     /* which intstatus bits raise DAT1 (host-IRQ) */

/* Chip-side host-int mask (mirrors Linux HOSTINTMASK in brcmfmac/sdio.c):
 *   I_HMB_SW_MASK = 0x000000f0  -- mailbox SW interrupts 0..3 (incl. FRAME_IND)
 *   I_CHIPACTIVE  = 0x20000000  -- chip transitioned from doze to active
 * Without this, the chip processes events but never asserts DAT1, so the
 * SDHC CARD_INT path stays silent and the brcmfmac RX thread sleeps forever.
 */
#define BRCMFMAC_HOSTINTMASK            (0x000000F0u | (1u << 29))

/* Individual SDPCMD intstatus bits (subset of I_HMB_SW_MASK).
 * Linux names from sdio.c: I_HMB_SW0..SW3 map to FC_STATE, FC_CHANGE,
 * FRAME_IND, HOST_INT respectively.
 */
#define BRCMFMAC_I_HMB_FC_STATE     (1u << 4)    /* HMB_SW0 -- chip xoff'd  */
#define BRCMFMAC_I_HMB_FC_CHANGE    (1u << 5)    /* HMB_SW1 -- fc transition*/
#define BRCMFMAC_I_HMB_FRAME_IND    (1u << 6)    /* HMB_SW2 -- frame avail  */
#define BRCMFMAC_I_HMB_HOST_INT     (1u << 7)    /* HMB_SW3 -- hostmail data*/

/* BCMA wrapper-register offsets (within wrapbase). */
#define BCMA_IOCTL                      0x0408
#define BCMA_IOCTL_CLK                  0x0001
#define BCMA_IOCTL_FGC                  0x0002
#define BCMA_RESET_CTL                  0x0800
#define BCMA_RESET_CTL_RESET            0x0001

/* D11-specific IOCTL bits. */
#define D11_BCMA_IOCTL_PHYCLOCKEN       0x0004
#define D11_BCMA_IOCTL_PHYRESET         0x0008

/* CR4-specific IOCTL bits. */
#define ARMCR4_BCMA_IOCTL_CPUHALT       0x0020

/* SOCRAM register offsets (within SOCRAM core base). */
#define SOCRAM_BANKIDX_OFFSET           0x10
#define SOCRAM_BANKPDA_OFFSET           0x44

/* Per-CMD53 block-mode cap. 512-block CMD53 wedges this chip's SDIO
 * state machine (an early errata in the initialization debug): 511 * 64 = 32704.
 */
#define BRCMFMAC_MAX_CMD53_BLOCK_BYTES  (511 * 64)

#define BRCMFMAC_MAX_CORES              12

/* === SDPCM + BCDC protocol ================================================
 *
 * Wire format for an SDIO control message:
 *   [4 B  SDPCM frame:  u16 len, u16 ~len]
 *   [8 B  SDPCM sw hdr: seq, chan, nextlen, hdrlen, flow, credit, rsv x2]
 *   [16 B BCDC/CDC hdr: cmd, outlen, inlen, flags, status]
 *   [payload (variable)]
 *   [padded to 4-byte alignment]
 */
#define BRCMFMAC_F2_FIFO_ADDR           0x8000     /* SB_ACCESS_2_4B_FLAG bit */
#define BRCMFMAC_F2_BLOCK_SIZE          512

/* Selected WLC command IDs (Linux brcmfmac/fwil.h). */
#define BRCMFMAC_WLC_UP                 2
#define BRCMFMAC_WLC_DOWN               3
#define BRCMFMAC_WLC_SET_INFRA          20
#define BRCMFMAC_WLC_SET_AUTH           22
#define BRCMFMAC_WLC_GET_BSSID          23
#define BRCMFMAC_WLC_SET_SSID           26
#define BRCMFMAC_WLC_DISASSOC           52
#define BRCMFMAC_WLC_SET_PM             86  /* power mgmt: 0=OFF 1=MAX 2=FAST */
#define BRCMFMAC_WLC_GET_RSSI           127
#define BRCMFMAC_WLC_SET_WSEC           134
#define BRCMFMAC_WLC_GET_UP             162
#define BRCMFMAC_WLC_SET_WPA_AUTH       165
#define BRCMFMAC_WLC_GET_VAR            262
#define BRCMFMAC_WLC_SET_VAR            263
#define BRCMFMAC_WLC_SET_WSEC_PMK       268

/* Event-mask length (Linux BRCMF_EVENTING_MASK_LEN = roundup(160,8)/8). */
#define BRCMFMAC_EVENTING_MASK_LEN      20

/* WPA2-PSK assoc constants (Linux brcmu_wifi.h). */
#define WSEC_AES_ENABLED                0x0004
#define WPA_AUTH_PSK                    0x0004
#define WPA2_AUTH_PSK                   0x0080
#define WPA_WPA2_AUTH_PSK_MIXED         (WPA_AUTH_PSK | WPA2_AUTH_PSK)

/* wsec_pmk flags (Linux brcmfmac/fwil_types.h). */
#define BRCMF_WSEC_PASSPHRASE           0x0001  /* key is passphrase, chip derives PMK */
#define BRCMF_WSEC_MAX_PSK_LEN          32      /* PMK length when flags=0 */
#define BRCMF_WSEC_MAX_PASSPHRASE_LEN   64      /* generous upper bound */

struct brcmf_wsec_pmk_le {
	uint16_t key_len;
	uint16_t flags;
	uint8_t  key[64];   /* BRCMF_WSEC_MAX_PASSPHRASE_LEN */
};

/* Event reason codes (Linux brcmfmac/fweh.h subset). */
#define BRCMFMAC_E_REASON_LINK_BSSCFG_DIS  4
#define BRCMFMAC_E_REASON_INITIAL_ASSOC    0
#define BRCMFMAC_E_REASON_LOW_RSSI         1
#define BRCMFMAC_E_REASON_DISASSOC         2
#define BRCMFMAC_E_REASON_DEAUTH           3

/* Internal connect state (driven by chan=1 event arrival). */
enum brcmfmac_link_state {
	BRCMFMAC_LINK_DOWN = 0,
	BRCMFMAC_LINK_AUTHING,
	BRCMFMAC_LINK_ASSOCING,
	BRCMFMAC_LINK_UP,
};

#define BCDC_FLAG_ERROR                 0x01
#define BCDC_FLAG_SET                   0x02
#define BCDC_REQ_ID_SHIFT               16

#define SDPCM_CHAN_CTRL                 0
#define SDPCM_CHAN_EVENT                1
#define SDPCM_CHAN_DATA                 2

struct sdpcm_frame_hdr {
	uint16_t len;
	uint16_t notlen;
} __packed;

struct sdpcm_sw_hdr {
	uint8_t seq;
	uint8_t chan;
	uint8_t nextlen;
	uint8_t hdrlen;
	uint8_t flow;
	uint8_t credit;
	uint8_t reserved[2];
} __packed;

struct cdc_hdr {
	uint32_t cmd;
	uint16_t outlen;
	uint16_t inlen;
	uint32_t flags;
	int32_t status;
} __packed;

/* BDC (Broadcom Data Codec) header sits between SDPCM and the L2 frame
 * on chan=1 (event) + chan=2 (data). Distinct from the 16-B CDC IOCTL
 * header used on chan=0.
 */
#define BDC_HEADER_LEN          4
#define BDC_PROTO_VER           2
#define BDC_PROTO_VER_SHIFT     4

struct bdc_hdr {
	uint8_t flags;          /* [7:4]=ver, [3]=SUM_NEEDED, [2]=SUM_GOOD */
	uint8_t priority;
	uint8_t flags2;         /* [3:0]=if_idx */
	uint8_t data_offset;    /* additional bytes between BDC end and L2 frame, in 4-B units */
} __packed;

/* === Broadcom event frame (ethertype 0x886C over chan=1) ================== */
#define BRCM_ETHERTYPE_EVENT    0x886C

struct brcm_ethhdr {
	uint16_t subtype;      /* BE */
	uint16_t length;       /* BE */
	uint8_t  version;
	uint8_t  oui[3];
	uint16_t usr_subtype;  /* BE */
} __packed;

struct brcmf_event_msg_be {
	uint16_t version;
	uint16_t flags;
	uint32_t event_type;
	uint32_t status;
	uint32_t reason;
	uint32_t auth_type;
	uint32_t datalen;
	uint8_t  addr[6];
	char     ifname[16];
	uint8_t  ifidx;
	uint8_t  bsscfgidx;
} __packed;

/* WLC event type codes we care about (Linux fweh.h). */
#define WLC_E_SET_SSID           0
#define WLC_E_AUTH               3
#define WLC_E_ASSOC              7
#define WLC_E_DISASSOC_IND      12
#define WLC_E_LINK              16
#define WLC_E_DEAUTH_IND        33
#define WLC_E_DEAUTH            34
#define WLC_E_AUTH_FAIL         42
#define WLC_E_PSK_SUP           46
#define WLC_E_ESCAN_RESULT      69

/* Event status codes. */
#define BRCMF_E_STATUS_SUCCESS   0
#define BRCMF_E_STATUS_FAIL      1
#define BRCMF_E_STATUS_TIMEOUT   2
#define BRCMF_E_STATUS_NO_NETWORKS 3
#define BRCMF_E_STATUS_PARTIAL   8

/* Flags for struct brcmf_dload_data_le (Linux brcmfmac/fwil_types.h
 * enum brcmf_dload_data_flag). The chip expects DL_BEGIN on the first
 * chunk and DL_END on the last; bit 12 (CRC-supplied) is set unconditionally
 * since the header carries a (currently zero) CRC field.
 */
#define BRCMF_DL_BEGIN          (1u << 1)
#define BRCMF_DL_END            (1u << 2)
#define BRCMF_DL_CRC_IN_HDR     (1u << 12)

/* dload_type values (Linux brcmfmac/fwil_types.h enum brcmf_dload_type). */
#define BRCMF_DL_TYPE_CLM       2

struct brcmf_dload_data_le {
	uint16_t flag;
	uint16_t dload_type;
	uint32_t len;
	uint32_t crc;
	uint8_t data[];
} __packed;

/* === Escan IOVAR =========================================================
 *
 * iovar name "escan", value = struct brcmf_escan_params_le. Action=1 starts
 * an escan; results stream back as chan=1 events of type WLC_E_ESCAN_RESULT
 * (each event carries one or more bss_info_le records via brcmf_escan_result_le).
 */
#define BRCMF_SCAN_PARAMS_VERSION   1
#define WL_ESCAN_ACTION_START       1
#define BRCMF_SSID_MAX_LEN          32
#define BRCMF_MCSSET_LEN            16

struct brcmf_ssid_le {
	uint32_t SSID_len;
	uint8_t  SSID[BRCMF_SSID_MAX_LEN];
} __packed;

struct brcmf_scan_params_le {
	struct brcmf_ssid_le ssid_le;
	uint8_t  bssid[6];
	int8_t   bss_type;
	uint8_t  scan_type;
	uint32_t nprobes;
	uint32_t active_time;
	uint32_t passive_time;
	uint32_t home_time;
	uint32_t channel_num;
	uint16_t channel_list[]; /* flexible: zero or more channels */
} __packed;

struct brcmf_escan_params_le {
	uint32_t version;
	uint16_t action;
	uint16_t sync_id;
	struct brcmf_scan_params_le params_le;
} __packed;

/* Linux declares brcmf_bss_info_le WITHOUT __packed -- natural alignment
 * inserts a pad byte before rateset.count, before RSSI, and before
 * nbss_cap. Mirror that exactly or chanspec/RSSI read the wrong bytes.
 */
struct brcmf_bss_info_le {
	uint32_t version;
	uint32_t length;
	uint8_t  BSSID[6];
	uint16_t beacon_period;
	uint16_t capability;
	uint8_t  SSID_len;
	uint8_t  SSID[32];
	struct {
		uint32_t count;
		uint8_t  rates[16];
	} rateset;
	uint16_t chanspec;
	uint16_t atim_window;
	uint8_t  dtim_period;
	uint16_t RSSI;
	int8_t   phy_noise;
	uint8_t  n_cap;
	uint32_t nbss_cap;
	uint8_t  ctl_ch;
	uint32_t reserved32;
	uint8_t  flags;
	uint8_t  reserved[3];
	uint8_t  basic_mcs[BRCMF_MCSSET_LEN];
	uint16_t ie_offset;
	uint32_t ie_length;
	uint16_t SNR;
};

struct brcmf_escan_result_le {
	uint32_t buflen;
	uint32_t version;
	uint16_t sync_id;
	uint16_t bss_count;
	struct brcmf_bss_info_le bss_info_le;
};

struct bcm_core {
	uint16_t id;
	uint32_t base;
	uint32_t wrapbase;
};

struct brcmfmac_config {
	const struct device *sdhc;
	struct gpio_dt_spec reg_on;
	const char *firmware_name;
};

struct brcmfmac_data;
struct net_pkt;

/* Pending IOCTL waiter: filled by query_dcmd, completed by RX thread. */
struct brcmfmac_pending_ioctl {
	bool active;
	uint16_t reqid;
	uint8_t *out_buf;
	uint16_t out_capacity;
	uint16_t out_copied;
	int status;
	struct k_sem done;
};

/* TX glomming: queue net frames in a fixed-size ring, drain them in
 * batches via one CMD53 per burst. Mirrors Linux brcmfmac's brcmf_sdio_txpkt
 * pattern. Chip's `bus:txglom=7` confirms it accepts glommed SDPCM frames.
 *
 * Sizing: 16 slots x 1600 B = 25.6 KB ring; 12 KB single glom buffer.
 * Up to 8 frames packed per CMD53 (gated by available SDPCM tx-credits
 * at flush time -- partial batches go out promptly if credits are tight).
 */
#define BRCMFMAC_TX_RING_SLOTS       16
#define BRCMFMAC_TX_SLOT_SIZE        1600
/* TODO: enable multi-frame glomming. Linux's wire format (sdio.c
 * ::brcmf_sdio_hdpack:1502-27) inserts an 8-byte SDPCM_HWEXT header
 * between the 4-byte fh and the 8-byte sw header on every frame in
 * glom mode, with (frame_len - 4) | (lastfrm << 24) in word 0 and
 * (tail_pad << 16) in word 1. The chip also needs `bus:txglom=1`
 * set via iovar at init time -- the default of 7 we read is the
 * capability bitmask, not the enabled mode. Without both halves, the
 * chip silently drops every multi-frame CMD53 and txseq stalls.
 * Single-frame (MAX=1) is the working path; mid-it gave
 * +16% throughput / 40x lower loss over the old direct-CMD53 path.
 */
#define BRCMFMAC_TX_GLOM_MAX_FRAMES  1
#define BRCMFMAC_TX_GLOM_BUF_SIZE    2048

struct brcmfmac_tx_slot {
	uint16_t len;   /* 0 = empty */
	uint8_t  data[BRCMFMAC_TX_SLOT_SIZE] __aligned(4);
};

struct brcmfmac_data {
	struct sd_card card;
	struct sdio_func backplane;     /* F1 */
	struct sdio_func radio;         /* F2 */

	/* Chip topology discovered by EROM scan. */
	struct bcm_core cores[BRCMFMAC_MAX_CORES];
	unsigned int num_cores;

	/* Chip identity (chipcommon[0]). */
	uint32_t chipid_reg;
	uint16_t chip_id;
	uint8_t  chip_rev;
	uint8_t  chip_type;

	uint32_t ram_base;
	uint32_t ram_size;

	/* BCDC protocol state. */
	bool f2_ready;
	uint8_t sdpcm_txseq;     /* next outgoing SDPCM seq (8-bit wrap) */
	uint8_t sdpcm_tx_max;    /* chip-reported tx-window upper bound */
	struct k_sem tx_credit_sem;  /* signaled on every RX SDPCM hdr parse */
	struct k_sem rx_irq_sem;     /* signaled by SDHC SDIO_INT callback */
	uint16_t bcdc_reqid;
	struct k_mutex bcdc_mutex;
	struct brcmfmac_pending_ioctl pending;

	/* Net + scan state. */
	struct net_if *iface;
	scan_result_cb_t scan_cb;

	/* Link / assoc state. Updated from the RX thread when WLC_E_LINK /
	 * WLC_E_DISASSOC_IND arrive.
	 */
	enum brcmfmac_link_state link_state;

	/* Last connect()'s SSID + security, retained so iface_status can
	 * report something meaningful. Cleared on disconnect.
	 */
	uint8_t  connected_ssid[32];
	uint8_t  connected_ssid_len;
	uint8_t  connected_security;  /* enum wifi_security_type */

	/* MAC from chip OTP, read via cur_etheraddr IOCTL. */
	uint8_t chip_mac[6];

	bool probed;

	/* TX glom ring: SPSC (iface_send produces, tx-thread consumes).
	 * head/tail are atomics so producer/consumer don't need a lock.
	 * Slot 0 .. head-1 are filled (mod N); tail .. head-1 are pending.
	 */
	struct brcmfmac_tx_slot tx_ring[BRCMFMAC_TX_RING_SLOTS];
	atomic_t tx_ring_head;
	atomic_t tx_ring_tail;
	struct k_sem tx_pending_sem;

	/* tx-thread-private glom assembly buffer. Holds the next CMD53's
	 * worth of chained SDPCM frames before issuing the F2 write.
	 */
	uint8_t tx_glom_buf[BRCMFMAC_TX_GLOM_BUF_SIZE] __aligned(4);
};

/* === SDIO backplane primitives (brcmfmac_sdio.c) === */
int brcmfmac_sdio_set_backplane_window(struct brcmfmac_data *data, uint32_t addr);
int brcmfmac_sdio_backplane_read32(struct brcmfmac_data *data, uint32_t addr,
				   uint32_t *out);
int brcmfmac_sdio_backplane_write32(struct brcmfmac_data *data, uint32_t addr,
				    uint32_t val);
int brcmfmac_sdio_backplane_read_bytes(struct brcmfmac_data *data, uint32_t addr,
				       uint8_t *buf, uint32_t len);
int brcmfmac_sdio_backplane_write_bytes(struct brcmfmac_data *data, uint32_t addr,
					const uint8_t *buf, uint32_t len);
int brcmfmac_sdio_ramrw(struct brcmfmac_data *data, bool write,
			uint32_t chip_addr, uint8_t *buf, uint32_t size);
int brcmfmac_sdio_fw_upload(struct brcmfmac_data *data);
int brcmfmac_sdio_nvram_upload(struct brcmfmac_data *data);

/* === Chip initialization (brcmfmac_chip.c) === */
int brcmfmac_chip_read_id(struct brcmfmac_data *data);
int brcmfmac_chip_ram_data(struct brcmfmac_data *data);
int brcmfmac_chip_pmu_setup(struct brcmfmac_data *data);
int brcmfmac_chip_erom_scan(struct brcmfmac_data *data);
const struct bcm_core *brcmfmac_chip_core_find(const struct brcmfmac_data *data,
					       uint16_t id);
int brcmfmac_chip_set_passive(struct brcmfmac_data *data);
int brcmfmac_chip_set_active(struct brcmfmac_data *data);

/* === BCDC protocol (brcmfmac_bcdc.c) ===
 *
 * F2 enable + polled IOCTL round-trip. Single in-flight,
 * caller blocks on the response. splits the polling out
 * into a dedicated RX thread.
 */
int brcmfmac_bcdc_init(struct brcmfmac_data *data);

/* Run a dcmd. tx_payload + tx_len go into the request; chip's response
 * payload (up to rx_capacity bytes) is copied to rx_buf. Returns bytes
 * copied or negative errno.
 */
int brcmfmac_bcdc_query_dcmd(struct brcmfmac_data *data, uint32_t cmd,
			     const uint8_t *tx_payload, uint16_t tx_len,
			     uint8_t *rx_buf, uint16_t rx_capacity);

/* IOVAR helper: WLC_GET_VAR with `name` as the var key. */
int brcmfmac_bcdc_iovar_get(struct brcmfmac_data *data, const char *name,
			    uint8_t *buf, uint16_t len);

/* SET dcmd variant: send a command with no response payload (chip echoes
 * status back). For WLC_SET_VAR + others.
 */
int brcmfmac_bcdc_set_dcmd(struct brcmfmac_data *data, uint32_t cmd,
			   const uint8_t *tx_payload, uint16_t tx_len);

/* IOVAR helper: WLC_SET_VAR with `name` as the var key + value as data. */
int brcmfmac_bcdc_iovar_set(struct brcmfmac_data *data, const char *name,
			    const uint8_t *value, uint16_t value_len);

/* bsscfg-prefixed iovar SET. The chip recognizes some iovars (sup_wpa,
 * sup_wpa_tmo, etc.) only when sent in the bsscfg-namespaced form even
 * for bsscfgidx=0. Wire format:
 *   WLC_SET_VAR cmd, payload = "bsscfg:NAME\0" + LE32(bsscfgidx) + value
 */
int brcmfmac_bcdc_bsscfg_iovar_set_int(struct brcmfmac_data *data,
				       const char *name, uint32_t bsscfgidx,
				       int32_t value);

/* Build SDPCM headers in-place at start of `frame`, then F2 TX the
 * (4-byte-padded) frame. Caller must hold bcdc_mutex. Used by both
 * the IOCTL path (chan=0) and the data path (chan=2).
 */
int brcmfmac_bcdc_tx_frame(struct brcmfmac_data *data, uint8_t chan,
			   uint8_t *frame, uint16_t total);

/* === net_if + wifi_mgmt glue (brcmfmac_net.c) === */
void brcmfmac_iface_init(struct net_if *iface);
int  brcmfmac_iface_send(const struct device *dev, struct net_pkt *pkt);
int  brcmfmac_mgmt_scan(const struct device *dev, struct net_if *iface,
			struct wifi_scan_params *params, scan_result_cb_t cb);
int  brcmfmac_mgmt_connect(const struct device *dev, struct net_if *iface,
			   struct wifi_connect_req_params *params);
int  brcmfmac_mgmt_disconnect(const struct device *dev, struct net_if *iface);
int  brcmfmac_mgmt_iface_status(const struct device *dev, struct net_if *iface,
				struct wifi_iface_status *status);

/* RX-thread dispatchers (called from bcdc.c's RX thread). */
void brcmfmac_net_rx_data(struct brcmfmac_data *data,
			  const uint8_t *frame, uint16_t len);
void brcmfmac_net_rx_event(struct brcmfmac_data *data,
			   const uint8_t *frame, uint16_t len);

/* === Embedded firmware blobs (firmware/) === */
extern const unsigned char brcmfmac_fw[];
extern const unsigned int  brcmfmac_fw_len;
extern const unsigned char brcmfmac_nvram[];
extern const unsigned int  brcmfmac_nvram_len;
extern const unsigned char brcmfmac_clm_blob[];
extern const unsigned int  brcmfmac_clm_blob_len;

#endif /* ZEPHYR_DRIVERS_WIFI_BRCMFMAC_PRIV_H_ */
