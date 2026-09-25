/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register and descriptor definitions follow the Cadence GEM (Gigabit Ethernet
 * MAC) as documented in the Xilinx Zynq-7000 TRM (UG585, chapter 16) and the
 * Linux macb driver. Names use the Linux MACB_/GEM_ prefixes so the MACB
 * (10/100) variant can be added later without renaming.
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_CDNS_MACB_ETH_CDNS_MACB_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_CDNS_MACB_ETH_CDNS_MACB_PRIV_H_

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

/*
 * Common checks
 */

/*
 * The DMA writes whole receive buffers, so a fragment must own every byte of
 * its data area. CONFIG_NET_BUF_VARIABLE_DATA_SIZE places the reference count
 * inside the data area, which the DMA would overwrite.
 */
#ifdef CONFIG_NET_BUF_VARIABLE_DATA_SIZE
#error "CONFIG_NET_BUF_VARIABLE_DATA_SIZE=y is not supported"
#endif

#if CONFIG_DCACHE
#if (CONFIG_DCACHE_LINE_SIZE + 0 == 0)
#error "CONFIG_DCACHE_LINE_SIZE must be configured to a non-zero value"
#endif
#endif

/* The GEM receive buffer size is programmed in units of 64 bytes */
#define CDNS_MACB_RX_BUFFER_MULTIPLE 64U
#define CDNS_MACB_RX_BUFFER_MAX      16320U

BUILD_ASSERT((CONFIG_NET_BUF_DATA_SIZE % CDNS_MACB_RX_BUFFER_MULTIPLE) == 0,
	     "CONFIG_NET_BUF_DATA_SIZE must be a multiple of 64");
BUILD_ASSERT(CONFIG_NET_BUF_DATA_SIZE <= CDNS_MACB_RX_BUFFER_MAX,
	     "CONFIG_NET_BUF_DATA_SIZE must not exceed 16320");

/*
 * The DMA imposes certain constraints on the buffers:
 * 1. Receive buffer addresses must be 4-byte aligned, the two low bits of the
 *    address word are the used and wrap bits. The DMA accesses memory with the
 *    data bus width, so buffers are also required to be aligned to that width
 *    and their size to be a multiple of it.
 * 2. If a data cache is enabled, buffers must also be aligned to the cache line
 *    size and their size must be a multiple of the cache line size, as cache
 *    lines are invalidated for received data.
 */
#define CDNS_MACB_DCACHE_LINE_SIZE COND_CODE_1(CONFIG_DCACHE, (CONFIG_DCACHE_LINE_SIZE), (0))

#define CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(bus_width)                                               \
	BUILD_ASSERT(((CONFIG_NET_BUF_DATA_SIZE) %                                                 \
		      MAX(((bus_width) / 8), CDNS_MACB_DCACHE_LINE_SIZE)) == 0,                    \
		     "CONFIG_NET_BUF_DATA_SIZE must be a multiple of the data bus width or "       \
		     "cache line size");                                                           \
	BUILD_ASSERT(COND_CODE_0(CONFIG_NET_BUF_ALIGNMENT, (sizeof(void *)),                       \
				 (CONFIG_NET_BUF_ALIGNMENT)) >=                                    \
			     MAX(((bus_width) / 8), CDNS_MACB_DCACHE_LINE_SIZE),                   \
		     "CONFIG_NET_BUF_ALIGNMENT must be at least the data bus width or cache "      \
		     "line size");                                                                 \
	IF_ENABLED(CONFIG_DCACHE,                                                                  \
		   (BUILD_ASSERT((CONFIG_NET_BUF_ALIGNMENT) % (CONFIG_DCACHE_LINE_SIZE) == 0,      \
				 "CONFIG_NET_BUF_ALIGNMENT must be a multiple of the data cache "  \
				 "line size")));

/*
 * Global driver parameters
 */

#define CDNS_MACB_NB_TX_DESCS CONFIG_ETH_CDNS_MACB_NB_TX_DESCS
#define CDNS_MACB_NB_RX_DESCS CONFIG_ETH_CDNS_MACB_NB_RX_DESCS

/*
 * Cores with descriptor read prefetch (DCFG10) read up to 2 << (n - 1)
 * descriptors past the current one, also past the wrap descriptor. Pad the
 * rings so those reads stay inside the ring object.
 */
#define CDNS_MACB_DESC_PREFETCH_PAD 16

/* Alignment of the descriptor ring object */
#define CDNS_MACB_RINGS_ALIGN MAX(64, CDNS_MACB_DCACHE_LINE_SIZE)

/* Stack size for the RX refill thread */
#define CDNS_MACB_RX_REFILL_STACK_SIZE 1024

/*
 * Platform capabilities, set by the glue code from the SoC integration.
 * They mirror the MACB_CAPS_* flags of the Linux driver.
 */

/* The DMA can stop under heavy load after a used bit read, toggle RE to recover */
#define CDNS_MACB_CAPS_NEEDS_RSTONUBR       BIT(0)
/* The DMA prefetches descriptors past the current one (DCFG10) */
#define CDNS_MACB_CAPS_BD_RD_PREFETCH       BIT(1)
/* The MAC can run at 1000 Mbit/s */
#define CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE BIT(2)
/* USRIO bit 0 selects the PHY interface */
#define CDNS_MACB_CAPS_USRIO_HAS_MII        BIT(3)
/* USRIO bit 0 clear means MII/GMII, set means RMII/RGMII */
#define CDNS_MACB_CAPS_USRIO_DEFAULT_IS_MII_GMII BIT(4)
/* USRIO bit 1 enables the transceiver clock */
#define CDNS_MACB_CAPS_USRIO_HAS_CLKEN      BIT(5)

/* PHY interface, reduced from the phy-connection-type property */
enum cdns_macb_phy_iface {
	CDNS_MACB_PHY_IFACE_MII,
	CDNS_MACB_PHY_IFACE_RMII,
	CDNS_MACB_PHY_IFACE_GMII,
	CDNS_MACB_PHY_IFACE_RGMII,
	CDNS_MACB_PHY_IFACE_SGMII,
};

#define CDNS_MACB_DT_INST_IS_RGMII(n)                                                              \
	(DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii) ||                                  \
	 DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii_id) ||                               \
	 DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii_txid) ||                             \
	 DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii_rxid))

/* The PHY interface of an instance, MII when the property is absent */
#define CDNS_MACB_DT_INST_PHY_IFACE(n)                                                             \
	(DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, sgmii)  ? CDNS_MACB_PHY_IFACE_SGMII        \
	 : CDNS_MACB_DT_INST_IS_RGMII(n)                        ? CDNS_MACB_PHY_IFACE_RGMII        \
	 : DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rmii) ? CDNS_MACB_PHY_IFACE_RMII         \
	 : DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii) ? CDNS_MACB_PHY_IFACE_GMII         \
								: CDNS_MACB_PHY_IFACE_MII)

/*
 * Common structure definitions
 */

/*
 * Hardware descriptor. With CONFIG_ETH_CDNS_MACB_DMA_64BIT the descriptor
 * grows to 16 bytes and carries the upper address bits (GEM_DMACFG.ADDR64).
 */
struct cdns_macb_dma_desc {
	uint32_t addr;
	uint32_t ctrl;
#ifdef CONFIG_ETH_CDNS_MACB_DMA_64BIT
	uint32_t addrh;
	uint32_t resvd;
#endif
};

/*
 * The descriptor rings of one instance. The glue code places one of these in
 * uncached memory per instance. The tie-off descriptors terminate the unused
 * priority queues of multi-queue cores.
 */
struct cdns_macb_rings {
	struct cdns_macb_dma_desc tx[CDNS_MACB_NB_TX_DESCS + CDNS_MACB_DESC_PREFETCH_PAD];
	struct cdns_macb_dma_desc rx[CDNS_MACB_NB_RX_DESCS + CDNS_MACB_DESC_PREFETCH_PAD];
	struct cdns_macb_dma_desc tx_tieoff;
	struct cdns_macb_dma_desc rx_tieoff;
};

/* The part of the device configuration the core knows about, first in the glue's config */
struct cdns_macb_config {
	DEVICE_MMIO_ROM;
	const struct device *phy_dev;
	struct net_eth_mac_config mac_cfg;
	struct cdns_macb_rings *rings;
	uint32_t caps;
	/* GEM_DMACFG.FBLDO: 1, 4, 8 or 16 */
	uint8_t dma_burst_length;
	enum cdns_macb_phy_iface phy_iface;
};

/* The part of the device data the core knows about, first in the glue's data */
struct cdns_macb_priv {
	DEVICE_MMIO_RAM;
	struct net_if *iface;

	uint8_t mac_addr[6];

	/* Physical address of the ring object, filled in by the glue */
	uintptr_t rings_phys;

	/* Queues present in the hardware, bit 0 is queue 0 */
	uint32_t queue_mask;
	bool isr_clear_on_write;
	/* Both packet buffers are in SRAM mode, needed for checksum offload */
	bool pkt_buf_mode;

	struct k_sem free_tx_descs, free_rx_descs;
	unsigned int tx_head, tx_tail;
	unsigned int rx_head, rx_tail;

	struct net_buf *rx_frags[CDNS_MACB_NB_RX_DESCS]; /* index shared with rings->rx */
	struct net_pkt *tx_pkts[CDNS_MACB_NB_TX_DESCS];  /* index shared with rings->tx */

	struct net_pkt *rx_pkt;
	uint16_t rx_nfrags;

	/* The receiver stopped on an unfilled descriptor and RXUBR is masked */
	bool rx_stalled;

	/* Incremented by every TX ring reset, senders that observed an older value give up */
	uint32_t tx_epoch;
	bool tx_resetting;
	struct k_work tx_error_work;

	K_KERNEL_STACK_MEMBER(rx_refill_thread_stack, CDNS_MACB_RX_REFILL_STACK_SIZE);
	struct k_thread rx_refill_thread;

	struct k_spinlock lock;

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

/*
 * Handy register accessors
 */

#define CDNS_MACB_REG_READ(r)     sys_read32(DEVICE_MMIO_GET(dev) + (r))
#define CDNS_MACB_REG_WRITE(r, v) sys_write32((v), DEVICE_MMIO_GET(dev) + (r))

/* The register base of a MAC, for the MDIO child device */
static inline mm_reg_t cdns_macb_reg_base(const struct device *mac_dev)
{
	return DEVICE_MMIO_GET(mac_dev);
}

/*
 * Shared declarations between core and platform glue code
 */

int cdns_macb_probe(const struct device *dev);
void cdns_macb_isr(const struct device *dev);
/*
 * Called by the core from cdns_macb_probe() once the MAC is configured. The
 * glue fills in rings_phys and connects the interrupt.
 */
int cdns_macb_platform_init(const struct device *dev);
/*
 * Called by the core whenever the PHY reports a link at a new speed, after
 * NCFGR has been updated. Platforms feeding the MAC from a speed-dependent
 * clock, as RGMII ones typically do, override this to retune that clock. The
 * default implementation does nothing.
 */
void cdns_macb_platform_link_speed_changed(const struct device *dev, enum phy_link_speed speed);
extern const struct ethernet_api cdns_macb_api;

/*
 * Register offsets
 */

#define MACB_NCR   0x0000 /* Network Control */
#define MACB_NCFGR 0x0004 /* Network Config */
#define MACB_NSR   0x0008 /* Network Status */
#define GEM_USRIO  0x000c /* User IO */
#define GEM_DMACFG 0x0010 /* DMA Configuration */
#define MACB_TSR   0x0014 /* Transmit Status */
#define MACB_RBQP  0x0018 /* RX Q Base Address */
#define MACB_TBQP  0x001c /* TX Q Base Address */
#define MACB_RSR   0x0020 /* Receive Status */
#define MACB_ISR   0x0024 /* Interrupt Status */
#define MACB_IER   0x0028 /* Interrupt Enable */
#define MACB_IDR   0x002c /* Interrupt Disable */
#define MACB_IMR   0x0030 /* Interrupt Mask */
#define MACB_MAN   0x0034 /* PHY Maintenance */
#define GEM_HRB    0x0080 /* Hash Bottom */
#define GEM_HRT    0x0084 /* Hash Top */
#define GEM_SAB(n) (0x0088 + (n) * 8) /* Specific address n bottom, n = 0..3 */
#define GEM_SAT(n) (0x008c + (n) * 8) /* Specific address n top, n = 0..3 */
#define GEM_SA_COUNT 4
#define MACB_MID   0x00fc /* Module ID */
#define GEM_DCFG1  0x0280 /* Design Config 1 */
#define GEM_DCFG2  0x0284 /* Design Config 2 */
#define GEM_DCFG6  0x0294 /* Design Config 6 */
#define GEM_DCFG10 0x02a4 /* Design Config 10 */

/* Registers of the priority queues q >= 1, queue 0 uses the legacy registers */
#define GEM_ISR_Q(q)  (0x0400 + ((q) - 1) * 4)
#define GEM_TBQP_Q(q) (0x0440 + ((q) - 1) * 4)
#define GEM_RBQP_Q(q) (0x0480 + ((q) - 1) * 4)
#define GEM_RBQS_Q(q) (0x04a0 + ((q) - 1) * 4)
#define GEM_IER_Q(q)  (0x0600 + ((q) - 1) * 4)
#define GEM_IDR_Q(q)  (0x0620 + ((q) - 1) * 4)
#define GEM_IMR_Q(q)  (0x0640 + ((q) - 1) * 4)

/* Upper 32 address bits of the descriptor rings, shared by all queues */
#define MACB_TBQPH 0x04c8
#define MACB_RBQPH 0x04d4

/*
 * Register fields
 */

/* NCR */
#define MACB_NCR_LLB     BIT(1)  /* Local loopback */
#define MACB_NCR_RE      BIT(2)  /* Receive enable */
#define MACB_NCR_TE      BIT(3)  /* Transmit enable */
#define MACB_NCR_MPE     BIT(4)  /* Management port enable */
#define MACB_NCR_CLRSTAT BIT(5)  /* Clear statistics registers */
#define MACB_NCR_TSTART  BIT(9)  /* Start transmission */
#define MACB_NCR_THALT   BIT(10) /* Transmit halt */

/* NCFGR */
#define MACB_NCFGR_SPD     BIT(0)         /* 100 Mbit/s */
#define MACB_NCFGR_FD      BIT(1)         /* Full duplex */
#define MACB_NCFGR_JFRAME  BIT(3)         /* Jumbo frames */
#define MACB_NCFGR_CAF     BIT(4)         /* Copy all frames */
#define MACB_NCFGR_NBC     BIT(5)         /* No broadcast */
#define MACB_NCFGR_MTI     BIT(6)         /* Multicast hash enable */
#define MACB_NCFGR_UNI     BIT(7)         /* Unicast hash enable */
#define MACB_NCFGR_BIG     BIT(8)         /* Receive 1536 byte frames */
#define GEM_NCFGR_GBE      BIT(10)        /* Gigabit mode enable */
#define GEM_NCFGR_PCSSEL   BIT(11)        /* PCS select */
#define MACB_NCFGR_PAE     BIT(13)        /* Pause enable */
#define MACB_NCFGR_RBOF    GENMASK(15, 14) /* Receive buffer offset */
#define MACB_NCFGR_DRFCS   BIT(17)        /* Discard receive FCS */
#define GEM_NCFGR_CLK      GENMASK(20, 18) /* MDC clock division */
#define GEM_NCFGR_DBW      GENMASK(22, 21) /* Data bus width */
#define GEM_NCFGR_RXCOEN   BIT(24)        /* RX checksum offload enable */
#define GEM_NCFGR_SGMIIEN  BIT(27)        /* SGMII mode enable */

/* GEM_NCFGR_CLK values */
#define GEM_CLK_DIV8   0
#define GEM_CLK_DIV16  1
#define GEM_CLK_DIV32  2
#define GEM_CLK_DIV48  3
#define GEM_CLK_DIV64  4
#define GEM_CLK_DIV96  5
#define GEM_CLK_DIV128 6
#define GEM_CLK_DIV224 7

/* GEM_NCFGR_DBW values */
#define GEM_DBW32  0
#define GEM_DBW64  1
#define GEM_DBW128 2

/* NSR */
#define MACB_NSR_IDLE BIT(2) /* PHY management logic idle */

/* TSR */
#define MACB_TSR_UBR  BIT(0) /* Used bit read */
#define MACB_TSR_COL  BIT(1) /* Collision occurred */
#define MACB_TSR_RLE  BIT(2) /* Retry limit exceeded */
#define MACB_TSR_TGO  BIT(3) /* Transmit go */
#define MACB_TSR_BEX  BIT(4) /* TX frame corruption due to AHB error */
#define MACB_TSR_COMP BIT(5) /* Transmit complete */
#define MACB_TSR_UND  BIT(6) /* Transmit under run */

/* RSR */
#define MACB_RSR_BNA BIT(0) /* Buffer not available */
#define MACB_RSR_REC BIT(1) /* Frame received */
#define MACB_RSR_OVR BIT(2) /* Receive overrun */

/* ISR/IER/IDR/IMR */
#define MACB_INT_MFD   BIT(0)  /* Management frame done */
#define MACB_INT_RCOMP BIT(1)  /* Receive complete */
#define MACB_INT_RXUBR BIT(2)  /* RX used bit read */
#define MACB_INT_TXUBR BIT(3)  /* TX used bit read */
#define MACB_INT_TUND  BIT(4)  /* TX buffer under run */
#define MACB_INT_RLE   BIT(5)  /* Retry limit exceeded or late collision */
#define MACB_INT_TXERR BIT(6)  /* TX frame corruption from bus error */
#define MACB_INT_TCOMP BIT(7)  /* Transmit complete */
#define MACB_INT_LINK  BIT(9)  /* Link change */
#define MACB_INT_ROVR  BIT(10) /* Receive overrun */
#define MACB_INT_HRESP BIT(11) /* HRESP not OK */

#define MACB_TX_ERR_FLAGS (MACB_INT_TUND | MACB_INT_RLE | MACB_INT_TXERR)
#define MACB_TX_INT_FLAGS (MACB_TX_ERR_FLAGS | MACB_INT_TCOMP | MACB_INT_TXUBR)
#define MACB_RX_INT_FLAGS (MACB_INT_RCOMP | MACB_INT_RXUBR | MACB_INT_ROVR)

/* MAN */
#define MACB_MAN_DATA GENMASK(15, 0)
#define MACB_MAN_CODE GENMASK(17, 16) /* Must be written as 2 */
#define MACB_MAN_REGA GENMASK(22, 18) /* Register address */
#define MACB_MAN_PHYA GENMASK(27, 23) /* PHY address */
#define MACB_MAN_RW   GENMASK(29, 28) /* Operation */
#define MACB_MAN_SOF  GENMASK(31, 30) /* 1 for Clause 22, 0 for Clause 45 */

#define MACB_MAN_CODE_VALUE 2
#define MACB_MAN_C22_SOF    1
#define MACB_MAN_C45_SOF    0

/* USRIO */
#define MACB_USRIO_MII   BIT(0) /* MII instead of RMII, or RGMII instead of GMII */
#define MACB_USRIO_CLKEN BIT(1) /* Transceiver clock enable */

/* GEM_DMACFG */
#define GEM_DMACFG_FBLDO      GENMASK(4, 0)   /* Fixed burst length for DMA */
#define GEM_DMACFG_ENDIA_DESC BIT(6)          /* Endian swap for descriptor access */
#define GEM_DMACFG_ENDIA_PKT  BIT(7)          /* Endian swap for packet data access */
#define GEM_DMACFG_RXBMS      GENMASK(9, 8)   /* RX packet buffer memory size */
#define GEM_DMACFG_TXPBMS     BIT(10)         /* TX packet buffer memory size */
#define GEM_DMACFG_TXCOEN     BIT(11)         /* TX checksum offload enable */
#define GEM_DMACFG_RXBS       GENMASK(23, 16) /* DMA receive buffer size in 64 byte units */
#define GEM_DMACFG_DDRP       BIT(24)         /* Discard when no AHB resource */
#define GEM_DMACFG_ADDR64     BIT(30)         /* 64-bit descriptor addresses */

/* MID */
#define MACB_MID_IDNUM GENMASK(27, 16)
#define MACB_MID_REV   GENMASK(15, 0)
/* IDNUM values below this belong to the MACB (10/100) core */
#define MACB_MID_IDNUM_GEM 2

/* GEM_DCFG1 */
#define GEM_DCFG1_NO_PCS BIT(0)
#define GEM_DCFG1_USERIO BIT(9)          /* USRIO register present */
#define GEM_DCFG1_IRQCOR BIT(23)         /* ISR is clear-on-read */
#define GEM_DCFG1_DBWDEF GENMASK(27, 25) /* Data bus width: 1 = 32, 2 = 64, 4 = 128 */

/* GEM_DCFG2 */
#define GEM_DCFG2_RX_PKT_BUFF BIT(20) /* RX packet buffer in SRAM mode */
#define GEM_DCFG2_TX_PKT_BUFF BIT(21) /* TX packet buffer in SRAM mode */

/* GEM_DCFG6 */
#define GEM_DCFG6_QUEUE_MASK GENMASK(7, 0) /* Priority queues 1..7 present */
#define GEM_DCFG6_DAW64      BIT(23)       /* 64-bit DMA addressing available */

/* GEM_DCFG10 */
#define GEM_DCFG10_RXBD_RDBUFF GENMASK(11, 8)
#define GEM_DCFG10_TXBD_RDBUFF GENMASK(15, 12)

/*
 * Descriptor fields
 */

/* RX address word */
#define MACB_RX_USED BIT(0) /* Set by hardware once the buffer is filled */
#define MACB_RX_WRAP BIT(1) /* Last descriptor of the ring */
#define MACB_RX_ADDR GENMASK(31, 2)

/* RX control word */
#define MACB_RX_FRMLEN      GENMASK(12, 0) /* Frame length, GEM without jumbo frames */
#define MACB_RX_SOF         BIT(14)        /* Start of frame */
#define MACB_RX_EOF         BIT(15)        /* End of frame */
#define MACB_RX_CFI         BIT(16)
#define MACB_RX_VLAN_PRI    GENMASK(19, 17)
#define MACB_RX_PRI_TAG     BIT(20)
#define MACB_RX_VLAN_TAG    BIT(21)
#define GEM_RX_CSUM         GENMASK(23, 22) /* With RXCOEN: checksum status */
#define MACB_RX_SA_MATCH    GENMASK(26, 23)
#define MACB_RX_EXT_MATCH   BIT(28)
#define MACB_RX_UHASH_MATCH BIT(29)
#define MACB_RX_MHASH_MATCH BIT(30)
#define MACB_RX_BROADCAST   BIT(31)

/* TX control word */
#define GEM_TX_FRMLEN         GENMASK(13, 0) /* Buffer length */
#define MACB_TX_LAST          BIT(15)        /* Last buffer of the frame */
#define MACB_TX_NOCRC         BIT(16)        /* Do not append the FCS */
#define MACB_TX_LATE_COLL     BIT(26)        /* Late collision, Zynq */
#define MACB_TX_BUF_EXHAUSTED BIT(27)        /* Buffers exhausted mid frame */
#define MACB_TX_UNDERRUN      BIT(28)        /* Transmit under run */
#define MACB_TX_ERROR         BIT(29)        /* Retry limit exceeded */
#define MACB_TX_WRAP          BIT(30)        /* Last descriptor of the ring */
#define MACB_TX_USED          BIT(31)        /* Set by hardware once the frame is sent */

#define MACB_TX_ERR_MASK                                                                           \
	(MACB_TX_LATE_COLL | MACB_TX_BUF_EXHAUSTED | MACB_TX_UNDERRUN | MACB_TX_ERROR)

#endif /* ZEPHYR_DRIVERS_ETHERNET_CDNS_MACB_ETH_CDNS_MACB_PRIV_H_ */
