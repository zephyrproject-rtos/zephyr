#ifndef _MDIO_BB_H_
#define _MDIO_BB_H_

#include "eth_lowRISC.h"

/*
 *  Write the specified network device's MDIO register `reg` for the phy
 *  specified with the value `val`.
 */
int mdiobb_write(struct net_local_lr *priv, int phy, int reg, u16 val);

/*
 *  Read the specified network device's MDIO register `reg` for the phy
 *  specified.
 */
int mdiobb_read(struct net_local_lr *priv, int phy, int reg);

#endif  // _MDIO_BB_H_
