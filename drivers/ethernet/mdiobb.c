#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <assert.h>

#include "eth_lowRISC.h"
#include "mdiobb.h"

/******************************************************************************/

static void mdio_dir(struct net_local_lr *priv, int dir)
{
    if (dir)
    {
        priv->last_mdio_gpio &= ~MDIOCTRL_MDIOOEN_MASK;  // output driving
    }
    else
    {
        priv->last_mdio_gpio |= MDIOCTRL_MDIOOEN_MASK;  // input receiving
    }

    eth_write(priv, MDIOCTRL_OFFSET, priv->last_mdio_gpio);
}

static int mdio_get(struct net_local_lr *priv)
{
    u32 rslt = eth_read(priv, MDIOCTRL_OFFSET) & MDIOCTRL_MDIOIN_MASK ? 1 : 0;
    return rslt;
}

static void mdio_set(struct net_local_lr *priv, int what)
{
    if (what)
    {
        priv->last_mdio_gpio |= MDIOCTRL_MDIOOUT_MASK;
    }
    else
    {
        priv->last_mdio_gpio &= ~MDIOCTRL_MDIOOUT_MASK;
    }

    eth_write(priv, MDIOCTRL_OFFSET, priv->last_mdio_gpio);
}

static void mdc_set(struct net_local_lr *priv, int what)
{
    if (what)
    {
        priv->last_mdio_gpio |= MDIOCTRL_MDIOCLK_MASK;
    }
    else
    {
        priv->last_mdio_gpio &= ~MDIOCTRL_MDIOCLK_MASK;
    }

    eth_write(priv, MDIOCTRL_OFFSET, priv->last_mdio_gpio);
}

/*
 *  MDIO must already be configured as output.
 */
static void mdiobb_send_bit(struct net_local_lr *priv, int val)
{
    mdio_set(priv, val);

    k_msleep(1);
    mdc_set(priv, 1);

    k_msleep(1);
    mdc_set(priv, 0);
}

/*
 *  MDIO must already be configured as input.
 */
static int mdiobb_get_bit(struct net_local_lr *priv)
{
    k_msleep(1);
    mdc_set(priv, 1);

    k_msleep(1);
    mdc_set(priv, 0);

    return mdio_get(priv);
}

/*
 *  MDIO must already be configured as output.
 */
static void mdiobb_send_num(struct net_local_lr *priv, u16 val, int bits)
{
    for (int i = bits - 1; i >= 0; i--)
    {
        mdiobb_send_bit(priv, (val >> i) & 1);
    }
}


/*
 *  Utility to send the preamble, address, and register (common to read and
 *  write).
 */
static void mdiobb_cmd(struct net_local_lr *priv, int op, u8 phy, u8 reg)
{
    mdio_dir(priv, 1);

    /*
     *  Send a 32 bit preamble ('1's) with an extra '1' bit for good measure
     *  measure. The IEEE spec says this is a PHY optional requirement. The
     *  AMD 79C874 requires one after power up and one after a MII
     *  communications error. This means that we are doing more preambles than
     *  we need, but it is safer and will be much more robust.
     */
    for (int i = 0; i < 32; i++)
    {
        mdiobb_send_bit(priv, 1);
    }

    /*
     *  Send the start bit (01) and the read opcode (10) or write (01). Clause
     *  45 operation uses 00 for the start and 11, 10 for read/write.
     */
    mdiobb_send_bit(priv, 0);
    if (op & MDIO_C45)
    {
        mdiobb_send_bit(priv, 0);
    }
    else
    {
        mdiobb_send_bit(priv, 1);
    }

    mdiobb_send_bit(priv, (op >> 1) & 1);
    mdiobb_send_bit(priv, (op >> 0) & 1);
    mdiobb_send_num(priv, phy, 5);
    mdiobb_send_num(priv, reg, 5);
}

/*
 *  In clause 45 mode all commands are prefixed by MDIO_ADDR to specify the
 *  lower 16 bits of the 21 bit address. This transfer is done identically to a
 *  MDIO_WRITE except for a different code. To enable clause 45 mode or
 *  MII_ADDR_C45 into the address. Theoretically clause 45 and normal devices
 *  can exist on the same bus. Normal devices should ignore the MDIO_ADDR phase.
 */
static int mdiobb_cmd_addr(struct net_local_lr *priv, int phy, u32 addr)
{
    unsigned int dev_addr = (addr >> 16) & 0x1F;
    unsigned int reg = addr & 0xFFFF;
    mdiobb_cmd(priv, MDIO_C45_ADDR, phy, dev_addr);

    /*
     *  Send the turnaround (10).
     */
    mdiobb_send_bit(priv, 1);
    mdiobb_send_bit(priv, 0);

    mdiobb_send_num(priv, reg, 16);

    mdio_dir(priv, 0);
    mdiobb_get_bit(priv);

    return dev_addr;
}

/******************************************************************************/

/*
 *  Write the specified network device's MDIO register `reg` for the phy
 *  specified with the value `val`.
 */
int mdiobb_write(struct net_local_lr *priv, int phy, int reg, u16 val)
{
    if (reg & MII_ADDR_C45)
    {
        reg = mdiobb_cmd_addr(priv, phy, reg);
        mdiobb_cmd(priv, MDIO_C45_WRITE, phy, reg);
    }
    else
    {
        mdiobb_cmd(priv, MDIO_WRITE, phy, reg);
    }

    /* send the turnaround (10) */
    mdiobb_send_bit(priv, 1);
    mdiobb_send_bit(priv, 0);

    mdiobb_send_num(priv, val, 16);

    mdio_dir(priv, 0);
    mdiobb_get_bit(priv);
    return 0;
}

/*
 *  Read the specified network device's MDIO register `reg` for the phy
 *  specified.
 */
int mdiobb_read(struct net_local_lr *priv, int phy, int reg)
{
    if (reg & MII_ADDR_C45)
    {
        reg = mdiobb_cmd_addr(priv, phy, reg);
        mdiobb_cmd(priv, MDIO_C45_READ, phy, reg);
    }
    else
    {
        mdiobb_cmd(priv, MDIO_READ, phy, reg);
    }

    mdio_dir(priv, 0);

    /*
     *  Check the turnaround bit: the PHY should be driving it to zero, if this
     *  PHY is listed in phy_ignore_ta_mask as having broken TA, skip that.
     */
    if (mdiobb_get_bit(priv) != 0)
    {
        /* PHY didn't drive TA low -- flush any bits it
         * may be trying to send.
         */
        for (int i = 0; i < 32; i++)
        {
            mdiobb_get_bit(priv);
        }
        return 0xffff;
    }

    return 0;
}

/******************************************************************************/
