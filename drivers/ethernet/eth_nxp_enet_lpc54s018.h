/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

/*
 * LPC54S018 support for NXP ENET Ethernet driver
 * 
 * This header adds LPC54S018-specific support to the existing
 * eth_nxp_enet.c driver.
 */

#ifndef ETH_NXP_ENET_LPC54S018_H_
#define ETH_NXP_ENET_LPC54S018_H_

#ifdef CONFIG_SOC_LPC54S018

/* LPC54S018 unique ID for MAC address generation */
/* Using OTP memory or other unique identifier */
#define ETH_NXP_ENET_UNIQUE_ID  (*(uint32_t *)0x40015000)

/* Additional LPC54S018-specific definitions if needed */

#endif /* CONFIG_SOC_LPC54S018 */

#endif /* ETH_NXP_ENET_LPC54S018_H_ */