/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define IP101A      '1'
#define DP83848     '2'
/* DP83848 and IP101A phys are supported. */
#define EMAC_PHY    DP83848

/*!< ETH_HEADER + ETH_EXTRA + ETH_VLAN_TAG + ETH_MAX_ETH_PAYLOAD + ETH_CRC */
#define ETH_MAX_PACKET_SIZE    ((uint32_t)1524U)

/* DP83848: 0x01,   IP101A: 0x01 */
#define PHY_ADDRESS                 (0x01)
#define PHY_BCR                     (0x0000)
#define PHY_BSR                     (0x0001)
#define PHY_ID1                     (0x0002)
#define PHY_ID2                     (0x0003)
#define PHY_ADV                     (0x0004)
#define PHY_LPA                     (0x0005)
/* PHY: DP83848 */
#define PHY_STS                     (0x0010)
/* PHY: IP101A */
#define PHY_SMR                     (0x0012)
/* PHY: IP101A */
#define PHY_PAD                     (0x001A)
/* PHY: DP83848 */
#define PHY_EDCR                    (0x001D)

/*!< PHY Reset */
#define PHY_RESET                       ((uint16_t)0x8000)
/*!< Select loop-back mode */
#define PHY_LOOPBACK                    ((uint16_t)0x4000)
/*!< Set the full-duplex mode at 100 Mb/s */
#define PHY_FULLDUPLEX_100M             ((uint16_t)0x2100)
/*!< Set the half-duplex mode at 100 Mb/s */
#define PHY_HALFDUPLEX_100M             ((uint16_t)0x2000)
/*!< Set the full-duplex mode at 10 Mb/s  */
#define PHY_FULLDUPLEX_10M              ((uint16_t)0x0100)
/*!< Set the half-duplex mode at 10 Mb/s  */
#define PHY_HALFDUPLEX_10M              ((uint16_t)0x0000)
/*!< Enable auto-negotiation function     */
#define PHY_AUTONEGOTIATION             ((uint16_t)0x1000)
/*!< Restart auto-negotiation function    */
#define PHY_RESTART_AUTONEGOTIATION     ((uint16_t)0x0200)
/*!< Select the power down mode           */
#define PHY_POWERDOWN                   ((uint16_t)0x0800)
/*!< Isolate PHY from MII                 */
#define PHY_ISOLATE                     ((uint16_t)0x0400)

/*!< Auto-Negotiation process completed   */
#define PHY_AUTONEGO_COMPLETE           ((uint16_t)0x0020)
/*!< Valid link established               */
#define PHY_LINKED_STATUS               ((uint16_t)0x0004)
/*!< Jabber condition detected*/
#define PHY_JABBER_DETECTION            ((uint16_t)0x0002)

/* BCR (Register 0) */
/*!< PHY Reset                            */
#define PHY_BCR_RESET                       ((uint16_t)0x8000)
/*!< Select loop-back mode                */
#define PHY_BCR_LOOPBACK                    ((uint16_t)0x4000)
/*!< Set the full-duplex mode at 100 Mb/s */
#define PHY_BCR_FULLDUPLEX_100M             ((uint16_t)0x2100)
/*!< Set the half-duplex mode at 100 Mb/s */
#define PHY_BCR_HALFDUPLEX_100M             ((uint16_t)0x2000)
/*!< Set the full-duplex mode at 10 Mb/s  */
#define PHY_BCR_FULLDUPLEX_10M              ((uint16_t)0x0100)
/*!< Set the half-duplex mode at 10 Mb/s  */
#define PHY_BCR_HALFDUPLEX_10M              ((uint16_t)0x0000)
/*!< Enable auto-negotiation function     */
#define PHY_BCR_AUTONEGOTIATION             ((uint16_t)0x1000)
/*!< Select the power down mode           */
#define PHY_BCR_POWERDOWN                   ((uint16_t)0x0800)
/*!< Isolate PHY from MII                 */
#define PHY_BCR_ISOLATE                     ((uint16_t)0x0400)
/*!< Restart auto-negotiation function    */
#define PHY_BCR_RESTART_AUTONEGOTIATION     ((uint16_t)0x0200)

/* BSR (Register 1) */
/*!< Auto-Negotiation process completed   */
#define PHY_BSR_AUTONEGO_COMPLETE           ((uint16_t)0x0020)
/*!< Valid link established               */
#define PHY_BSR_LINKED_STATUS               ((uint16_t)0x0004)
/*!< Jabber condition detected            */
#define PHY_BSR_JABBER_DETECTION            ((uint16_t)0x0002)

/* SMR (Register 18, PHY: IP101A) */
/*!< The speed selection after auto-negotiation     */
#define PHY_SMR_SPEED                       ((uint16_t)0x4000)
/*!< The duplex selection after auto-negotiation    */
#define PHY_SMR_DUPLEX                      ((uint16_t)0x2000)

/* PHYSTS (Register 16, PHY: DP83848) */
/*!< The duplex selection after auto-negotiation    */
#define PHY_STS_DUPLEX                      ((uint16_t)0x0004)
/*!< The speed selection after auto-negotiation     */
#define PHY_STS_SPEED                       ((uint16_t)0x0002)

/* buffer size for receive               */
#define ETH_RX_BUF_SIZE                ETH_MAX_PACKET_SIZE
/* buffer size for transmit              */
#define ETH_TX_BUF_SIZE                ETH_MAX_PACKET_SIZE
/* 4 Rx buffers of size ETH_RX_BUF_SIZE  */
#define ETH_RXBUFNB                    ((uint32_t)2)
/* 4 Tx buffers of size ETH_TX_BUF_SIZE  */
#define ETH_TXBUFNB                    ((uint32_t)2)


/*
 * @brief  Bit definition of TDES0 register: DMA Tx descriptor status register
 */
/*!< OWN bit: descriptor is owned by DMA engine */
#define ETH_DMATXDESC_OWN                     ((uint32_t)0x80000000U)
/*!< Interrupt on Completion */
#define ETH_DMATXDESC_IC                      ((uint32_t)0x40000000U)
/*!< Last Segment */
#define ETH_DMATXDESC_LS                      ((uint32_t)0x20000000U)
/*!< First Segment */
#define ETH_DMATXDESC_FS                      ((uint32_t)0x10000000U)
/*!< Disable CRC */
#define ETH_DMATXDESC_DC                      ((uint32_t)0x08000000U)
/*!< Disable Padding */
#define ETH_DMATXDESC_DP                      ((uint32_t)0x04000000U)
/*!< Transmit Time Stamp Enable */
#define ETH_DMATXDESC_TTSE                    ((uint32_t)0x02000000U)
/*!< Checksum Insertion Control: 4 cases */
#define ETH_DMATXDESC_CIC                     ((uint32_t)0x00C00000U)
/*!< Do Nothing: Checksum Engine is bypassed */
#define ETH_DMATXDESC_CIC_BYPASS              ((uint32_t)0x00000000U)
/*!< IPV4 header Checksum Insertion */
#define ETH_DMATXDESC_CIC_IPV4HEADER          ((uint32_t)0x00400000U)
/*!< TCP/UDP/ICMP Checksum Insertion calculated over segment only */
#define ETH_DMATXDESC_CIC_TCPUDPICMP_SEGMENT  ((uint32_t)0x00800000U)
/*!< TCP/UDP/ICMP Checksum Insertion fully calculated */
#define ETH_DMATXDESC_CIC_TCPUDPICMP_FULL     ((uint32_t)0x00C00000U)
/*!< Transmit End of Ring */
#define ETH_DMATXDESC_TER                     ((uint32_t)0x00200000U)
/*!< Second Address Chained */
#define ETH_DMATXDESC_TCH                     ((uint32_t)0x00100000U)
/*!< Tx Time Stamp Status */
#define ETH_DMATXDESC_TTSS                    ((uint32_t)0x00020000U)
/*!< IP Header Error */
#define ETH_DMATXDESC_IHE                     ((uint32_t)0x00010000U)
/*!< Error summary: OR of the following bits: UE || ED || EC || LCO || NC || LCA || FF || JT */
#define ETH_DMATXDESC_ES                      ((uint32_t)0x00008000U)
/*!< Jabber Timeout */
#define ETH_DMATXDESC_JT                      ((uint32_t)0x00004000U)
/*!< Frame Flushed: DMA/MTL flushed the frame due to SW flush */
#define ETH_DMATXDESC_FF                      ((uint32_t)0x00002000U)
/*!< Payload Checksum Error */
#define ETH_DMATXDESC_PCE                     ((uint32_t)0x00001000U)
/*!< Loss of Carrier: carrier lost during transmission */
#define ETH_DMATXDESC_LCA                     ((uint32_t)0x00000800U)
/*!< No Carrier: no carrier signal from the transceiver */
#define ETH_DMATXDESC_NC                      ((uint32_t)0x00000400U)
/*!< Late Collision: transmission aborted due to collision */
#define ETH_DMATXDESC_LCO                     ((uint32_t)0x00000200U)
/*!< Excessive Collision: transmission aborted after 16 collisions */
#define ETH_DMATXDESC_EC                      ((uint32_t)0x00000100U)
/*!< VLAN Frame */
#define ETH_DMATXDESC_VF                      ((uint32_t)0x00000080U)
/*!< Collision Count */
#define ETH_DMATXDESC_CC                      ((uint32_t)0x00000078U)
/*!< Excessive Deferral */
#define ETH_DMATXDESC_ED                      ((uint32_t)0x00000004U)
/*!< Underflow Error: late data arrival from the memory */
#define ETH_DMATXDESC_UF                      ((uint32_t)0x00000002U)
/*!< Deferred Bit */
#define ETH_DMATXDESC_DB                      ((uint32_t)0x00000001U)

/*!< Transmit Buffer2 Size */
#define ETH_DMATXDESC_TBS2  ((uint32_t)0x1FFF0000U)
/*!< Transmit Buffer1 Size */
#define ETH_DMATXDESC_TBS1  ((uint32_t)0x00001FFFU)

/*
 * @brief  Bit definition of RDES0 register: DMA Rx descriptor status register
 */
/*!< OWN bit: descriptor is owned by DMA engine  */
#define ETH_DMARXDESC_OWN         ((uint32_t)0x80000000U)
/*!< DA Filter Fail for the rx frame  */
#define ETH_DMARXDESC_AFM         ((uint32_t)0x40000000U)
/*!< Receive descriptor frame length  */
#define ETH_DMARXDESC_FL          ((uint32_t)0x3FFF0000U)
/*!< Error summary: OR of the following bits: DE || OE || IPC || LC || RWT || RE || CE */
#define ETH_DMARXDESC_ES          ((uint32_t)0x00008000U)
/*!< Descriptor error: no more descriptors for receive frame  */
#define ETH_DMARXDESC_DE          ((uint32_t)0x00004000U)
/*!< SA Filter Fail for the received frame */
#define ETH_DMARXDESC_SAF         ((uint32_t)0x00002000U)
/*!< Frame size not matching with length field */
#define ETH_DMARXDESC_LE          ((uint32_t)0x00001000U)
/*!< Overflow Error: Frame was damaged due to buffer overflow */
#define ETH_DMARXDESC_OE          ((uint32_t)0x00000800U)
/*!< VLAN Tag: received frame is a VLAN frame */
#define ETH_DMARXDESC_VLAN        ((uint32_t)0x00000400U)
/*!< First descriptor of the frame  */
#define ETH_DMARXDESC_FS          ((uint32_t)0x00000200U)
/*!< Last descriptor of the frame  */
#define ETH_DMARXDESC_LS          ((uint32_t)0x00000100U)
/*!< IPC Checksum Error: Rx Ipv4 header checksum error   */
#define ETH_DMARXDESC_IPV4HCE     ((uint32_t)0x00000080U)
/*!< Late collision occurred during reception   */
#define ETH_DMARXDESC_LC          ((uint32_t)0x00000040U)
/*!< Frame type - Ethernet, otherwise 802.3    */
#define ETH_DMARXDESC_FT          ((uint32_t)0x00000020U)
/*!< Receive Watchdog Timeout: watchdog timer expired during reception    */
#define ETH_DMARXDESC_RWT         ((uint32_t)0x00000010U)
/*!< Receive error: error reported by MII interface  */
#define ETH_DMARXDESC_RE          ((uint32_t)0x00000008U)
/*!< Dribble bit error: frame contains non int multiple of 8 bits  */
#define ETH_DMARXDESC_DBE         ((uint32_t)0x00000004U)
/*!< CRC error */
#define ETH_DMARXDESC_CE          ((uint32_t)0x00000002U)
/*!< Rx MAC Address/Payload Checksum Error: Rx MAC address matched/ Rx Payload Checksum Error */
#define ETH_DMARXDESC_MAMPCE      ((uint32_t)0x00000001U)

/*
 * @brief  Bit definition of RDES1 register
 */
/*!< Disable Interrupt on Completion */
#define ETH_DMARXDESC_DIC   ((uint32_t)0x80000000U)
/*!< Receive Buffer2 Size */
#define ETH_DMARXDESC_RBS2  ((uint32_t)0x1FFF0000U)
/*!< Receive End of Ring */
#define ETH_DMARXDESC_RER   ((uint32_t)0x00008000U)
/*!< Second Address Chained */
#define ETH_DMARXDESC_RCH   ((uint32_t)0x00004000U)
/*!< Receive Buffer1 Size */
#define ETH_DMARXDESC_RBS1  ((uint32_t)0x00001FFFU)

enum  ETH_RET_STATUS {
	ETH_RET_OK       = 0x00,
	ETH_RET_ERROR    = 0x01,
	ETH_RET_BUSY     = 0x02,
	ETH_RET_TIMEOUT  = 0x03
};

enum  ETH_LOCK {
	ETH_LOCK_UNLOCKED = 0x00,
	ETH_LOCK_LOCKED   = 0x01
};

enum ETH_STATE {
	/*!< Peripheral not yet Initialized or disabled         */
	ETH_STATE_RESET             = 0x00,
	/*!< Peripheral Initialized and ready for use           */
	ETH_STATE_READY             = 0x01,
	/*!< an internal process is ongoing                     */
	ETH_STATE_BUSY              = 0x02,
	/*!< Data Transmission process is ongoing               */
	ETH_STATE_BUSY_TX           = 0x12,
	/*!< Data Reception process is ongoing                  */
	ETH_STATE_BUSY_RX           = 0x22,
	/*!< Data Transmission and Reception process is ongoing */
	ETH_STATE_BUSY_TX_RX        = 0x32,
	/*!< Write process is ongoing                           */
	ETH_STATE_BUSY_WR           = 0x42,
	/*!< Read process is ongoing                            */
	ETH_STATE_BUSY_RD           = 0x82,
	/*!< Timeout state                                      */
	ETH_STATE_TIMEOUT           = 0x03,
	/*!< Reception process is ongoing                       */
	ETH_STATE_ERROR             = 0x04
};

enum ETH_SPEED {
	ETH_SPEED_10M               = 0x00,
	ETH_SPEED_100M              = 0x01,
};

enum ETH_DUPLEX {
	ETH_MODE_HALFDUPLEX         = 0x00,
	ETH_MODE_FULLDUPLEX         = 0x01,
};

enum ETH_RX_MODE {
	ETH_RXPOLLING_MODE          = 0x00,
	ETH_RXINTERRUPT_MODE        = 0x01,
};

enum ETH_CHKSUM_MODE {
	ETH_CHECKSUM_BY_HARDWARE     = 0x00,
	ETH_CHECKSUM_BY_SOFTWARE     = 0x01,
};

struct ETH_INIT_PARM {
	/*!< Selects or not the AutoNegotiation mode for the external PHY. */
	uint32_t        AutoNegotiation;
	/*!< Sets the Ethernet speed: 10/100 Mbps. */
	enum ETH_SPEED       Speed;
	/*!< Selects the MAC duplex mode: Half-Duplex or Full-Duplex mode. */
	enum ETH_DUPLEX      DuplexMode;
	/*!< Ethernet PHY address. (This parameter must be a number between Min_Data = 0 and
	 * Max_Data = 32.)
	 */
	uint16_t        PhyAddress;
	/*!< MAC Address of used Hardware: must be pointer on an array of 6 bytes. */
	uint8_t         *MACAddr;
	/*!< Selects the Ethernet Rx mode: Polling mode, Interrupt mode. */
	enum ETH_RX_MODE     RxMode;
	/*!< Selects if the checksum is check by hardware or by software. */
	enum ETH_CHKSUM_MODE ChecksumMode;
	/*!< Selects or not the Loopback mode for the external PHY. */
	uint32_t        PhyLoopback;
};

struct ETH_DMA_DESCRIPTOR {
	/*!< Status */
	uint32_t   Status;
	/*!< Control and Buffer1, Buffer2 lengths */
	uint32_t   ControlBufferSize;
	/*!< Buffer1 address pointer */
	uint32_t   Buffer1Addr;
	/*!< Buffer2 or next descriptor address pointer */
	uint32_t   Buffer2NextDescAddr;

	/*!< Enhanced Ethernet DMA PTP Descriptors */
	/*!< Extended status for PTP receive descriptor */
	uint32_t   ExtendedStatus;
	/*!< Reserved */
	uint32_t   Reserved1;
	/*!< Time Stamp Low value for transmit and receive */
	uint32_t   TimeStampLow;
	/*!< Time Stamp High value for transmit and receive */
	uint32_t   TimeStampHigh;
};

struct ETH_DMA_RX_INFO {
	/*!< First Segment Rx Desc */
	struct ETH_DMA_DESCRIPTOR   *FSRxDesc;
	/*!< Last Segment Rx Desc */
	struct ETH_DMA_DESCRIPTOR   *LSRxDesc;
	/*!< Segment count */
	uint32_t            SegCount;
	/*!< Frame length */
	uint32_t            length;
	/*!< Frame buffer */
	uint32_t            buffer;
};

struct ETH_HANDLE_TYPE {
	/*!< Ethernet init parm configuration */
	struct ETH_INIT_PARM       InitParm;
	/*!< Ethernet link status        */
	uint32_t            LinkStatus;
	/*!< Rx descriptor to Get        */
	struct ETH_DMA_DESCRIPTOR   *RxDesc;
	/*!< Tx descriptor to Set        */
	struct ETH_DMA_DESCRIPTOR   *TxDesc;
	/*!< last Rx frame infos         */
	struct ETH_DMA_RX_INFO     RxFrameInfos;
	/*!< ETH communication state     */
	enum ETH_STATE           State;
	/*!< ETH Lock                    */
	uint32_t            Lock;
};
