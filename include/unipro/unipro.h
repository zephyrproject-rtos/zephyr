/**
 * Copyright (c) 2014-2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author: Perry Hung
 */

#ifndef _UNIPRO_H_
#define _UNIPRO_H_

#include <stdlib.h>
#include <stdint.h>

#define CPORT_BUF_SIZE              (2048)

#define INFINITE_MAX_INFLIGHT_BUFCOUNT      0

enum unipro_event {
    UNIPRO_EVT_MAILBOX,
    UNIPRO_EVT_LUP_DONE,
};

typedef int (*unipro_send_completion_t)(int status, const void *buf,
                                        void *priv);
typedef void (*cport_reset_completion_cb_t)(unsigned int cportid, void *data);
typedef void (*unipro_event_handler_t)(enum unipro_event evt);

struct unipro_driver {
    const char name[32];
    int (*rx_handler)(unsigned int cportid,  // Called in irq context
                      void *data,
                      size_t len);
};

unsigned int unipro_cport_count(void);
void unipro_init(void);
void unipro_init_with_event_handler(unipro_event_handler_t evt_handler);
void unipro_deinit(void);
void unipro_set_event_handler(unipro_event_handler_t evt_handler);
void unipro_info(void);
int unipro_send(unsigned int cportid, const void *buf, size_t len);
int unipro_send_async(unsigned int cportid, const void *buf, size_t len,
                      unipro_send_completion_t callback, void *priv);
int unipro_reset_cport(unsigned int cportid, cport_reset_completion_cb_t cb,
                       void *priv);

int unipro_set_max_inflight_rxbuf_count(unsigned int cportid,
                                        size_t max_inflight_buf);
void *unipro_rxbuf_alloc(unsigned int cportid);
void unipro_rxbuf_free(unsigned int cportid, void *ptr);

/*
 * UniPro attributes
 */

/*
 * "Don't care" selector index
 */
#define UNIPRO_SELINDEX_NULL 0  // FIXME: put this back to 0xffff when we're
                                // out of demo land.
                                //
                                // It's causing SW-2431, apparently by
                                // exposing a bug elsewhere in the SVC
                                // usage of DME attributes.

/*
 * Result codes for UniPro DME accesses
 */
#define UNIPRO_CONFIGRESULT_SUCCESS           0 /* Access successful */
#define UNIPRO_CONFIGRESULT_INVALID_ATTR      1 /* Attribute is invalid,
                                                 * unimplemented, or
                                                 * STATIC and doesn't support
                                                 * setting. */
#define UNIPRO_CONFIGRESULT_INVALID_ATTR_VAL  2 /* Attribute is settable,
                                                 * but invalid value was
                                                 * written. */
#define UNIPRO_CONFIGRESULT_READ_ONLY_ATTR    3 /* Attribute is read-only. */
#define UNIPRO_CONFIGRESULT_WRITE_ONLY_ATTR   4 /* Attribute is write-only. */
#define UNIPRO_CONFIGRESULT_BAD_INDEX         5 /* CPort in selector index
                                                 * is out of range. */
#define UNIPRO_CONFIGRESULT_LOCKED_ATTR       6 /* CPort attribute is settable,
                                                 * but CPort is connected. */
#define UNIPRO_CONFIGRESULT_BAD_TEST_FEATURE  7 /* Test feature selector index
                                                 * is out of range. */
#define UNIPRO_CONFIGRESULT_PEER_COMM_FAILURE 8 /* Peer I/O error. */
#define UNIPRO_CONFIGRESULT_BUSY              9 /* Previous attr operation
                                                 * hasn't been completed. */
#define UNIPRO_CONFIGRESULT_DME_FAILURE       10 /* Layer addressed by DME */

/*
 * L1 attributes
 */
#define TX_HSMODE_CAPABILITY                     0x0001
#define TX_HSGEAR_CAPABILITY                     0x0002
#define TX_PWMG0_CAPABILITY                      0x0003
#define TX_PWMGEAR_CAPABILITY                    0x0004
#define TX_AMPLITUDE_CAPABILITY                  0x0005
#define TX_EXTERNALSYNC_CAPABILITY               0x0006
#define TX_HS_UNTERMINATED_LINE_DRIVE_CAPABILITY 0x0007
#define TX_LS_TERMINATED_LINE_DRIVE_CAPABILITY   0x0008
#define TX_MIN_SLEEP_NOCONFIG_TIME_CAPABILITY    0x0009
#define TX_MIN_STALL_NOCONFIG_TIME_CAPABILITY    0x000a
#define TX_MIN_SAVE_CONFIG_TIME_CAPABILITY       0x000b
#define TX_REF_CLOCK_SHARED_CAPABILITY           0x000c
#define TX_PHY_MAJORMINOR_RELEASE_CAPABILITY     0x000d
#define TX_PHY_EDITORIAL_RELEASE_CAPABILITY      0x000e
#define TX_HIBERN8TIME_CAPABILITY                0x000f
#define TX_ADVANCED_GRANULARITY_CAPABILITY       0x0010
#define TX_ADVANCED_HIBERN8TIME_CAPABILITY       0x0011
#define TX_HS_EQUALIZER_SETTING_CAPABILITY       0x0012
#define TX_MODE                                  0x0021
#define TX_HSRATE_SERIES                         0x0022
#define TX_HSGEAR                                0x0023
#define TX_PWMGEAR                               0x0024
#define TX_AMPLITUDE                             0x0025
#define TX_HS_SLEWRATE                           0x0026
#define TX_SYNC_SOURCE                           0x0027
#define TX_HS_SYNC_LENGTH                        0x0028
#define TX_HS_PREPARE_LENGTH                     0x0029
#define TX_LS_PREPARE_LENGTH                     0x002a
#define TX_HIBERN8_CONTROL                       0x002b
#define TX_LCC_ENABLE                            0x002c
#define TX_PWM_BURST_CLOSURE_EXTENSION           0x002d
#define TX_BYPASS_8B10B_ENABLE                   0x002e
#define TX_DRIVER_POLARITY                       0x002f
#define TX_HS_UNTERMINATED_LINE_DRIVE_ENABLE     0x0030
#define TX_LS_TERMINATED_LINE_DRIVE_ENABLE       0x0031
#define TX_LCC_SEQUENCER                         0x0032
#define TX_MIN_ACTIVATETIME                      0x0033
#define TX_ADVANCED_GRANULARITY_SETTING          0x0035
#define TX_ADVANCED_GRANULARITY                  0x0036
#define TX_HS_EQUALIZER_SETTING                  0x0037
#define TX_FSM_STATE                             0x0041
#define MC_OUTPUT_AMPLITUDE                      0x0061
#define MC_HS_UNTERMINATED_ENABLE                0x0062
#define MC_LS_TERMINATED_ENABLE                  0x0063
#define MC_HS_UNTERMINATED_LINE_DRIVE_ENABLE     0x0064
#define MC_LS_TERMINATED_LINE_DRIVE_ENABLE       0x0065
#define RX_HSMODE_CAPABILITY                     0x0081
#define RX_HSGEAR_CAPABILITY                     0x0082
#define RX_PWMG0_CAPABILITY                      0x0083
#define RX_PWMGEAR_CAPABILITY                    0x0084
#define RX_HS_UNTERMINATED_CAPABILITY            0x0085
#define RX_LS_TERMINATED_CAPABILITY              0x0086
#define RX_MIN_SLEEP_NOCONFIG_TIME_CAPABILITY    0x0087
#define RX_MIN_STALL_NOCONFIG_TIME_CAPABILITY    0x0088
#define RX_MIN_SAVE_CONFIG_TIME_CAPABILITY       0x0089
#define RX_REF_CLOCK_SHARED_CAPABILITY           0x008a
#define RX_HS_G1_SYNC_LENGTH_CAPABILITY          0x008b
#define RX_HS_G1_PREPARE_LENGTH_CAPABILITY       0x008c
#define RX_LS_PREPARE_LENGTH_CAPABILITY          0x008d
#define RX_PWM_BURST_CLOSURE_LENGTH_CAPABILITY   0x008e
#define RX_MIN_ACTIVATETIME_CAPABILITY           0x008f
#define RX_PHY_MAJORMINOR_RELEASE_CAPABILITY     0x0090
#define RX_PHY_EDITORIAL_RELEASE_CAPABILITY      0x0091
#define RX_HIBERN8TIME_CAPABILITY                0x0092
#define RX_HS_G2_SYNC_LENGTH_CAPABILITY          0x0094
#define RX_HS_G3_SYNC_LENGTH_CAPABILITY          0x0095
#define RX_HS_G2_PREPARE_LENGTH_CAPABILITY       0x0096
#define RX_HS_G3_PREPARE_LENGTH_CAPABILITY       0x0097
#define RX_ADVANCED_GRANULARITY_CAPABILITY       0x0098
#define RX_ADVANCED_HIBERN8TIME_CAPABILITY       0x0099
#define RX_ADVANCED_MIN_ACTIVATETIME_CAPABILITY  0x009a
#define RX_MODE                                  0x00a1
#define RX_HSRATE_SERIES                         0x00a2
#define RX_HSGEAR                                0x00a3
#define RX_PWMGEAR                               0x00a4
#define RX_LS_TERMINATED_ENABLE                  0x00a5
#define RX_HS_UNTERMINATED_ENABLE                0x00a6
#define RX_ENTER_HIBERN8                         0x00a7
#define RX_BYPASS_8B10B_ENABLE                   0x00a8
#define RX_TERMINATION_FORCE_ENABLE              0x00a9
#define RX_FSM_STATE                             0x00c1
#define OMC_TYPE_CAPABILITY                      0x00d1
#define MC_HSMODE_CAPABILITY                     0x00d2
#define MC_HSGEAR_CAPABILITY                     0x00d3
#define MC_HS_START_TIME_VAR_CAPABILITY          0x00d4
#define MC_HS_START_TIME_RANGE_CAPABILITY        0x00d5
#define MC_RX_SA_CAPABILITY                      0x00d6
#define MC_RX_LA_CAPABILITY                      0x00d7
#define MC_LS_PREPARE_LENGTH                     0x00d8
#define MC_PWMG0_CAPABILITY                      0x00d9
#define MC_PWMGEAR_CAPABILITY                    0x00da
#define MC_LS_TERMINATED_CAPABILITY              0x00db
#define MC_HS_UNTERMINATED_CAPABILITY            0x00dc
#define MC_LS_TERMINATED_LINE_DRIVE_CAPABILITY   0x00dd
#define MC_HS_UNTERMINATED_LINE_DRIVE_CAPABILIT  0x00de
#define MC_MFG_ID_PART1                          0x00df
#define MC_MFG_ID_PART2                          0x00e0
#define MC_PHY_MAJORMINOR_RELEASE_CAPABILITY     0x00e1
#define MC_PHY_EDITORIAL_RELEASE_CAPABILITY      0x00e2
#define MC_VENDOR_INFO_PART1                     0x00e3
#define MC_VENDOR_INFO_PART2                     0x00e4
#define MC_VENDOR_INFO_PART3                     0x00e5
#define MC_VENDOR_INFO_PART4                     0x00e6

/*
 * L1.5 attributes
 */
#define PA_PHYTYPE                     0x1500
#define PA_AVAILTXDATALANES            0x1520
#define PA_AVAILRXDATALANES            0x1540
#define PA_MINRXTRAILINGCLOCKS         0x1543
#define PA_TXHSG1SYNCLENGTH            0x1552
#define PA_TXHSG1PREPARELENGTH         0x1553
#define PA_TXHSG2SYNCLENGTH            0x1554
#define PA_TXHSG2PREPARELENGTH         0x1555
#define PA_TXHSG3SYNCLENGTH            0x1556
#define PA_TXHSG3PREPARELENGTH         0x1557
#define PA_TXMK2EXTENSION              0x155a
#define PA_PEERSCRAMBLING              0x155b
#define PA_TXSKIP                      0x155c
#define PA_SAVECONFIGEXTENSIONENABLE   0x155d
#define PA_LOCALTXLCCENABLE            0x155e
#define PA_PEERTXLCCENABLE             0x155f
#define PA_ACTIVETXDATALANES           0x1560
#define PA_CONNECTEDTXDATALANES        0x1561
#define PA_TXTRAILINGCLOCKS            0x1564
#define PA_TXPWRSTATUS                 0x1567
#define PA_TXGEAR                      0x1568
#define PA_TXTERMINATION               0x1569
#define PA_HSSERIES                    0x156a
#define PA_PWRMODE                     0x1571
#define PA_ACTIVERXDATALANES           0x1580
#define PA_CONNECTEDRXDATALANES        0x1581
#define PA_RXPWRSTATUS                 0x1582
#define PA_RXGEAR                      0x1583
#define PA_RXTERMINATION               0x1584
#define PA_SCRAMBLING                  0x1585
#define PA_MAXRXPWMGEAR                0x1586
#define PA_MAXRXHSGEAR                 0x1587
#define PA_PACPREQTIMEOUT              0x1590
#define PA_PACPREQEOBTIMEOUT           0x1591
#define PA_REMOTEVERINFO               0x15a0
#define PA_LOGICALLANEMAP              0x15a1
#define PA_SLEEPNOCONFIGTIME           0x15a2
#define PA_STALLNOCONFIGTIME           0x15a3
#define PA_SAVECONFIGTIME              0x15a4
#define PA_RXHSUNTERMINATIONCAPABILITY 0x15a5
#define PA_RXLSTERMINATIONCAPABILITY   0x15a6
#define PA_HIBERN8TIME                 0x15a7
#define PA_TACTIVATE                   0x15a8
#define PA_LOCALVERINFO                0x15a9
#define PA_GRANULARITY                 0x15aa
#define PA_MK2EXTENSIONGUARDBAND       0x15ab
#define PA_PWRMODEUSERDATA0            0x15b0
#define PA_PWRMODEUSERDATA1            0x15b1
#define PA_PWRMODEUSERDATA2            0x15b2
#define PA_PWRMODEUSERDATA3            0x15b3
#define PA_PWRMODEUSERDATA4            0x15b4
#define PA_PWRMODEUSERDATA5            0x15b5
#define PA_PWRMODEUSERDATA6            0x15b6
#define PA_PWRMODEUSERDATA7            0x15b7
#define PA_PWRMODEUSERDATA8            0x15b8
#define PA_PWRMODEUSERDATA9            0x15b9
#define PA_PWRMODEUSERDATA10           0x15ba
#define PA_PWRMODEUSERDATA11           0x15bb
#define PA_PACPFRAMECOUNT              0x15c0
#define PA_PACPERRORCOUNT              0x15c1
#define PA_PHYTESTCONTROL              0x15c2

/*
 * L2 attributes
 */
#define DL_TXPREEMPTIONCAP         0x2000
#define DL_TC0TXMAXSDUSIZE         0x2001
#define DL_TC0RXINITCREDITVAL      0x2002
#define DL_TC1TXMAXSDUSIZE         0x2003
#define DL_TC1RXINITCREDITVAL      0x2004
#define DL_TC0TXBUFFERSIZE         0x2005
#define DL_TC1TXBUFFERSIZE         0x2006
#define DL_TC0TXFCTHRESHOLD        0x2040
#define DL_FC0PROTECTIONTIMEOUTVAL 0x2041
#define DL_TC0REPLAYTIMEOUTVAL     0x2042
#define DL_AFC0REQTIMEOUTVAL       0x2043
#define DL_AFC0CREDITTHRESHOLD     0x2044
#define DL_TC0OUTACKTHRESHOLD      0x2045
#define DL_PEERTC0PRESENT          0x2046
#define DL_PEERTC0RXINITCREDITVAL  0x2047
#define DL_TC1TXFCTHRESHOLD        0x2060
#define DL_FC1PROTECTIONTIMEOUTVAL 0x2061
#define DL_TC1REPLAYTIMEOUTVAL     0x2062
#define DL_AFC1REQTIMEOUTVAL       0x2063
#define DL_AFC1CREDITTHRESHOLD     0x2064
#define DL_TC1OUTACKTHRESHOLD      0x2065
#define DL_PEERTC1PRESENT          0x2066
#define DL_PEERTC1RXINITCREDITVAL  0x2067

/*
 * L3 attributes
 */
#define N_DEVICEID        0x3000
#define N_DEVICEID_VALID  0x3001
#define N_TC0TXMAXSDUSIZE 0x3020
#define N_TC1TXMAXSDUSIZE 0x3021

/* Max device ID value */
#define N_MAXDEVICEID     127

/*
 * L4 attributes
 */
#define T_NUMCPORTS                  0x4000
#define T_NUMTESTFEATURES            0x4001
#define T_TC0TXMAXSDUSIZE            0x4060
#define T_TC1TXMAXSDUSIZE            0x4061
#define T_TSTCPORTID                 0x4080
#define T_TSTSRCON                   0x4081
#define T_TSTSRCPATTERN              0x4082
    #define TSTSRCPATTERN_SAWTOOTH  (0x0)
#define T_TSTSRCINCREMENT            0x4083
#define T_TSTSRCMESSAGESIZE          0x4084
#define T_TSTSRCMESSAGECOUNT         0x4085
#define T_TSTSRCINTERMESSAGEGAP      0x4086
#define T_TSTDSTON                   0x40a1
#define T_TSTDSTERRORDETECTIONENABLE 0x40a2
#define T_TSTDSTPATTERN              0x40a3
#define T_TSTDSTINCREMENT            0x40a4
#define T_TSTDSTMESSAGECOUNT         0x40a5
#define T_TSTDSTMESSAGEOFFSET        0x40a6
#define T_TSTDSTMESSAGESIZE          0x40a7
#define T_TSTDSTFCCREDITS            0x40a8
#define T_TSTDSTINTERFCTOKENGAP      0x40a9
#define T_TSTDSTINITIALFCCREDITS     0x40aa
#define T_TSTDSTERRORCODE            0x40ab
#define T_PEERDEVICEID               0x4021
#define T_PEERCPORTID                0x4022
#define T_CONNECTIONSTATE            0x4020
#define T_TRAFFICCLASS               0x4023
    #define CPORT_TC0               (0x0)
    #define CPORT_TC1               (0x1)
#define T_PROTOCOLID                 0x4024
#define T_CPORTFLAGS                 0x4025
    #define CPORT_FLAGS_E2EFC       (1)
    #define CPORT_FLAGS_CSD_N       (2)
    #define CPORT_FLAGS_CSV_N       (4)
#define T_TXTOKENVALUE               0x4026
#define T_RXTOKENVALUE               0x4027
#define T_LOCALBUFFERSPACE           0x4028
#define T_PEERBUFFERSPACE            0x4029
#define T_CREDITSTOSEND              0x402a
#define T_CPORTMODE                  0x402b
    #define CPORT_MODE_APPLICATION  (1)
    #define CPORT_MODE_UNDER_TEST   (2)

/*
 * DME attributes
 */
#define DME_DDBL1_REVISION          0x5000
#define DME_DDBL1_LEVEL             0x5001
#define DME_DDBL1_DEVICECLASS       0x5002
#define DME_DDBL1_MANUFACTURERID    0x5003
#define DME_DDBL1_PRODUCTID         0x5004
#define DME_DDBL1_LENGTH            0x5005
#define DME_FC0PROTECTIONTIMEOUTVAL 0xd041
#define DME_TC0REPLAYTIMEOUTVAL     0xd042
#define DME_AFC0REQTIMEOUTVAL       0xd043
#define DME_FC1PROTECTIONTIMEOUTVAL 0xd044
#define DME_TC1REPLAYTIMEOUTVAL     0xd045
#define DME_AFC1REQTIMEOUTVAL       0xd046

/*
 * Values for DME_DDBL1_MANUFACTURERID and DME_DDBL1_PRODUCTID, used to
 * determine the type and revision of bridges
 */
#define MANUFACTURER_TOSHIBA        0x0126
#define PRODUCT_ES2_TSB_BRIDGE      0x1000
#define PRODUCT_ES3_TSB_APBRIDGE    0x1001
#define PRODUCT_ES3_TSB_GPBRIDGE    0x1002

/*
 * Mailbox ACK attribute, ES specific.
 * Used to handshake the SVC and the bridges.
 *
 * The Switch is using both ES2 and ES3 attributes for compatibility
 * with modules.
 *
 * For ES2 an unrelated attribute is repurposed.
 * Warning: this attribute is used by the Unipro traffic generation on the
 * switch, cf. 'svc t' command
 */
#define ES2_MBOX_ACK_ATTR           T_TSTSRCINTERMESSAGEGAP
#define ES3_SYSTEM_STATUS_15        0x610f
#define ES3_MBOX_ACK_ATTR           ES3_SYSTEM_STATUS_15
/* The bridges define only one value depending on the ES revision */
#if defined(CONFIG_TSB_CHIP_REV_ES2)
    #define MBOX_ACK_ATTR           ES2_MBOX_ACK_ATTR
#elif defined(CONFIG_TSB_CHIP_REV_ES3)
    #define MBOX_ACK_ATTR           ES3_MBOX_ACK_ATTR
#endif

/*
 * Lower level attribute read/write.
 *
 * Please use one of the unipro_{local,peer}_attr_{read,write}()
 * wrappers below if it is possible instead. They are more readable.
 */
int unipro_attr_read(uint16_t attr,
                     uint32_t *val,
                     uint16_t selector,
                     int peer);
int unipro_attr_write(uint16_t attr,
                      uint32_t val,
                      uint16_t selector,
                      int peer);
int unipro_attr_access(uint16_t attr,
                       uint32_t *val,
                       uint16_t selector,
                       int peer,
                       int write);

int unipro_disable_fct_tx_flow(unsigned int cport);
int unipro_enable_fct_tx_flow(unsigned int cport);

int unipro_driver_register(struct unipro_driver *drv, unsigned int cportid);
int unipro_driver_unregister(unsigned int cportid);
void unipro_if_rx(unsigned int, void *, size_t);

static inline int unipro_attr_local_read(uint16_t attr,
                                         uint32_t *val,
                                         uint16_t selector)
{
    return unipro_attr_access(attr, val, selector, 0, 0);
}

static inline int unipro_attr_peer_read(uint16_t attr,
                                        uint32_t *val,
                                        uint16_t selector)
{
    return unipro_attr_access(attr, val, selector, 1, 0);
}

static inline int unipro_attr_local_write(uint16_t attr,
                                          uint32_t val,
                                          uint16_t selector)
{
    return unipro_attr_access(attr, &val, selector, 0, 1);
}

static inline int unipro_attr_peer_write(uint16_t attr,
                                         uint32_t val,
                                         uint16_t selector)
{
    return unipro_attr_access(attr, &val, selector, 1, 1);
}

/*
 * Link configuration
 */

/**
 * UniPro power mode.
 *
 * The TX and RX directions of a UniPro link can have different power
 * modes.
 */
enum unipro_pwr_mode {
    /*
     * These four values go into DME attributes. Don't change them.
     */

    /** Permanently in FAST_STATE; i.e. a high speed (HS) M-PHY gear. */
    UNIPRO_FAST_MODE = 1,
    /** Permanently in SLOW_STATE; i.e. a PWM M-PHY gear. */
    UNIPRO_SLOW_MODE = 2,
    /** Alternating automatically between FAST_STATE (HS) and SLEEP_STATE. */
    UNIPRO_FASTAUTO_MODE = 4,
    /** Alternating automatically between SLOW_STATE (PWM) and SLEEP_STATE. */
    UNIPRO_SLOWAUTO_MODE = 5,
    /**
     * Special value to use when you don't want to change one link
     * direction's power mode */
    UNIPRO_MODE_UNCHANGED = 7,

    /*
     * These values are random and can be changed if needd.
     */

    /** Hibernate mode */
    UNIPRO_HIBERNATE_MODE = 100,
    /** Powered off */
    UNIPRO_OFF_MODE = 101,
};

/** @brief UniPro frequency series in high speed mode. */
enum unipro_hs_series {
    UNIPRO_HS_SERIES_UNCHANGED = 0,
    UNIPRO_HS_SERIES_A = 1,
    UNIPRO_HS_SERIES_B = 2,
};

/**
 * @brief User-data for peer DME during power mode reconfiguration.
 *
 * These are used to set remote timeout values during the link
 * configuration procedure.
 */
struct unipro_pwr_user_data {
    /* Do not try to set any peer L2 Timeout Values */
#   define UPRO_PWRF_NONE (0U)
    /* Try to set peer DL_FC0PROTECTIONTIMEOUTVAL */
#   define UPRO_PWRF_FC0  (1U << 0)
    /* Try to set peer DL_TC0REPLAYTIMEOUTVAL */
#   define UPRO_PWRF_TC0  (1U << 1)
    /* Try to set peer DL_AFC0REQTIMEOUTVAL */
#   define UPRO_PWRF_AFC0 (1U << 2)
    /* Try to set peer DL_FC1PROTECTIONTIMEOUTVAL */
#   define UPRO_PWRF_FC1  (1U << 3)
    /* Try to set peer DL_TC1REPLAYTIMEOUTVAL */
#   define UPRO_PWRF_TC1  (1U << 4)
    /* Try to set peer DL_AFC1REQTIMEOUTVAL */
#   define UPRO_PWRF_AFC1 (1U << 5)
    uint32_t flags;
    uint16_t upro_pwr_fc0_protection_timeout;
    uint16_t upro_pwr_tc0_replay_timeout;
    uint16_t upro_pwr_afc0_req_timeout;
    uint16_t upro_pwr_fc1_protection_timeout;
    uint16_t upro_pwr_tc1_replay_timeout;
    uint16_t upro_pwr_afc1_req_timeout;
    const uint16_t reserved_tc2[3];
    const uint16_t reserved_tc3[3];
} __attribute__((__packed__));

/**
 * @brief UniPro link per-direction configuration.
 *
 * This is used to configure a unipro link in one direction.
 */
struct unipro_pwr_cfg {
    enum unipro_pwr_mode  upro_mode;   /**< Power mode to set. */
    uint8_t               upro_gear;   /**< M-PHY gear to use. */
    uint8_t               upro_nlanes; /**< Number of active data lanes. */
} __attribute__((__packed__));

/**
 * @brief UniPro link configuration.
 */
struct unipro_link_cfg {
    enum unipro_hs_series upro_hs_ser; /**< Frequency series in HS mode. */
    struct unipro_pwr_cfg upro_tx_cfg; /**< Configuration for TX direction. */
    struct unipro_pwr_cfg upro_rx_cfg; /**< Configuration for RX direction. */

    /** User-data (e.g. L2 timers) */
    struct unipro_pwr_user_data upro_user;

#   define UPRO_LINKF_TX_TERMINATION (1U << 0) /**< TX termination is on. */
#   define UPRO_LINKF_RX_TERMINATION (1U << 1) /**< RX termination is on. */
#   define UPRO_LINKF_SCRAMBLING     (1U << 2) /**< Scrambling request. */
    uint32_t flags;
} __attribute__((__packed__));

#define UNIPRO_PWR_CFG(_mode, gear, nlanes)                             \
    {                                                                   \
        .upro_mode = (_mode),                                           \
        .upro_gear = (gear),                                            \
        .upro_nlanes = (nlanes),                                        \
    }

#define UNIPRO_FAST_PWR_CFG(auto_, gear, nlanes)                        \
    UNIPRO_PWR_CFG((auto_) ? UNIPRO_FASTAUTO_MODE : UNIPRO_FAST_MODE,   \
                   (gear),                                              \
                   (nlanes))

#define UNIPRO_SLOW_PWR_CFG(auto_, gear, nlanes)                        \
    UNIPRO_PWR_CFG((auto_) ? UNIPRO_SLOWAUTO_MODE : UNIPRO_SLOW_MODE,   \
                   (gear),                                              \
                   (nlanes))

#endif
