#ifndef _ETH_HEADER_H_
#define _ETH_HEADER_H_

#include <stddef.h>

typedef unsigned int uint;
typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef signed long long shuge;
typedef unsigned long long uhuge;

typedef schar s8;
typedef uchar u8;
typedef short s16;
typedef ushort u16;
typedef int s32;
typedef unsigned u32;
typedef shuge s64;
typedef uhuge u64;


/******************************************************************************/

#define LR_WR_MAC_LOW 0x0800
#define LR_WR_MAC_HI_IRQ 0x0808
#define LR_WR_TX_SIZE 0x0810
#define LR_WR_TFCS 0x0818
#define LR_WR_MDIO_CTRL 0x0820
#define LR_WR_LAST_BUFFER_PTR 0x0828
#define LR_WR_FIRST_BUFFER_PTR 0x0830

#define LR_RD_MAC_LOW 0x0800
#define LR_RD_MAC_HI_IRQ 0x0808
#define LR_RD_TX_STATUS 0x0810
#define LR_RD_TX_FRAME_CHECK 0x0818
#define LR_RD_MDIO_CTRL 0x0820
#define LR_RD_RX_FRAME_CHECK 0x0828
#define LR_RD_ISR_PTR_STATUS 0x0830

/*
 *  Register offsets (in bytes) for the LowRISC Core
 */
#define TXBUFF_OFFSET 0x1000   /* Transmit Buffer */
#define MACLO_OFFSET 0x0800    /* MAC address low 32-bits */
#define MACHI_OFFSET 0x0808    /* MAC address high 16-bits and MAC ctrl */
#define TPLR_OFFSET 0x0810     /* Tx packet length */
#define TFCS_OFFSET 0x0818     /* Tx frame check sequence register */
#define MDIOCTRL_OFFSET 0x0820 /* MDIO Control Register */
#define RFCS_OFFSET                                                           \
    0x0828 /* Rx frame check sequence register(read) and last register(write) \
            */
#define RSR_OFFSET 0x0830  /* Rx status and reset register */
#define RBAD_OFFSET 0x0838 /* Rx bad frame and bad fcs register arrays */
#define RPLR_OFFSET 0x0840 /* Rx packet length register array */

#define RXBUFF_OFFSET 0x4000 /* Receive Buffer */

/* MAC Ctrl Register (MACHI) Bit Masks */
#define MACHI_MACADDR_MASK 0x0000FFFF  /* MAC high 16-bits mask */
#define MACHI_COOKED_MASK 0x00010000   /* obsolete flag */
#define MACHI_LOOPBACK_MASK 0x00020000 /* Rx loopback packets */
#define MACHI_ALLPKTS_MASK 0x00400000  /* Rx all packets (promiscuous mode) */
#define MACHI_IRQ_EN 0x00800000        /* Rx packet interrupt enable */

/* MDIO Control Register Bit Masks */
#define MDIOCTRL_MDIOCLK_MASK 0x00000001 /* MDIO Clock Mask */
#define MDIOCTRL_MDIOOUT_MASK 0x00000002 /* MDIO Output Mask */
#define MDIOCTRL_MDIOOEN_MASK                                          \
    0x00000004 /* MDIO Output Enable Mask, 3-state enable, high=input, \
                  low=output */
#define MDIOCTRL_MDIORST_MASK 0x00000008 /* MDIO Input Mask */
#define MDIOCTRL_MDIOIN_MASK 0x00000008  /* MDIO Input Mask */

/* Transmit Status Register (TPLR) Bit Masks */
#define TPLR_FRAME_ADDR_MASK 0x0FFF0000 /* Tx frame address */
#define TPLR_PACKET_LEN_MASK 0x00000FFF /* Tx packet length */
#define TPLR_BUSY_MASK 0x80000000       /* Tx busy mask */

/* Receive Status Register (RSR) */
#define RSR_RECV_FIRST_MASK 0x0000000F /* first available buffer (static) */
#define RSR_RECV_NEXT_MASK 0x000000F0  /* current rx buffer (volatile) */
#define RSR_RECV_LAST_MASK 0x00000F00  /* last available rx buffer (static) */
#define RSR_RECV_DONE_MASK 0x00001000  /* Rx complete */
#define RSR_RECV_IRQ_MASK 0x00002000   /* Rx irq bit */

/* Receive Packet Length Register (RPLR) */
#define RPLR_LENGTH_MASK 0x00000FFF    /* Rx packet length */
#define RPLR_ERROR_MASK 0x40000000     /* Rx error mask */
#define RPLR_FCS_ERROR_MASK 0x80000000 /* Rx FCS error mask */

/* General Ethernet Definitions */
#define HEADER_OFFSET 12           /* Offset to length field */
#define HEADER_SHIFT 16            /* Shift value for length */
#define ARP_PACKET_SIZE 28         /* Max ARP packet size */
#define HEADER_IP_LENGTH_OFFSET 16 /* IP Length Offset */

/******************************************************************************/

#define MDIO_READ 2
#define MDIO_WRITE 1

#define MDIO_C45 (1 << 15)
#define MDIO_C45_ADDR (MDIO_C45 | 0)
#define MDIO_C45_READ (MDIO_C45 | 3)
#define MDIO_C45_WRITE (MDIO_C45 | 1)

#define MDIO_SETUP_TIME 10
#define MDIO_HOLD_TIME 10

/* Minimum MDC period is 400 ns, plus some margin for error.  MDIO_DELAY
 * is done twice per period.
 */
#define MDIO_DELAY 250

/* The PHY may take up to 300 ns to produce data, plus some margin
 * for error.
 */
#define MDIO_READ_DELAY 350
#define MII_ADDR_C45 (1 << 30)

/******************************************************************************/

#define mmiowb()

/* General Ethernet Definitions */
#define XEL_ARP_PACKET_SIZE 28         /* Max ARP packet size */
#define XEL_HEADER_IP_LENGTH_OFFSET 16 /* IP Length Offset */

// TODO: Is this right?
#define HZ 1000
#define __iomem

#define TX_TIMEOUT (60 * HZ) /* Tx timeout is 60 seconds. */

/******************************************************************************/

/*
 *  Private structure that represents one instance of an ethernet driver.
 */
struct net_local_lr
{
    void __iomem *ioaddr;
    u32 msg_enable;

//    struct phy_device *phy_dev;
    int last_duplex;
    int last_carrier;

    u32 last_mdio_gpio;
    u32 spurious; /* Count packets we took in but did not process */
    int irq;
    struct net_if *iface;
    uint8_t mac[6];
    uint8_t txb[NET_ETH_MTU];
    uint8_t rxb[NET_ETH_MTU];

};

static void inline eth_write(struct net_local_lr *priv, size_t addr, int data)
{
    volatile u64 *eth_base = (volatile u64 *)(priv->ioaddr);
    eth_base[addr >> 3] = data;
}

static volatile inline int eth_read(struct net_local_lr *priv, size_t addr)
{
    volatile u64 *eth_base = (volatile u64 *)(priv->ioaddr);
    return eth_base[addr >> 3];
}


#endif  // _ETH_HEADER_H_
