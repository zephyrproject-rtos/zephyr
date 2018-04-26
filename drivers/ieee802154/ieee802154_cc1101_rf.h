/*
 * Copyright (c) 2018 Matthias Boesl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_CC1101_RF_H__
#define __IEEE802154_CC1101_RF_H__

#include <ieee802154/cc1101.h>

/* About PKTCFGn from Kconfig:
 * CONFIG_IEEE802154_CC1101_PKTCFG0 LENGTH_VAR_1
 * CONFIG_IEEE802154_CC1101_PKTCFG1 (APPEND_STATUS | CRC_FFFF | ADDR_NO_CHK)
 * We do not enable 802.15.4g mode yet:
 * CONFIG_IEEE802154_CC1101_PKTCFG2 (PKT_FORMAT_NORMAL_MODE | CCA_ALWAYS_CLEAR)
 **/

/* 802.15.4g compliant settings */

/* PA ramping = false */
/* Data format = Normal mode */
/* Channel spacing = 199.951172 */
/* Manchester enable = false */
/* Packet length = 255 */
/* Packet length mode = Variable packet length mode. Packet length configured by the first byte after sync word */
/* Preamble count = 4 */
/* Whitening = false */
/* Modulation format = GFSK */
/* Data rate = 49.9878 */
/* CRC enable = true */
/* Channel number = 0 */
/* CRC autoflush = false */
/* Address config = No address check */
/* Carrier frequency = 863.125 */
/* Base frequency = 863.125 */
/* Deviation = 25.390625 */
/* Sync word qualifier mode = 30/32 sync word bits detected */
/* Modulated = true */
/* Device address = 0 */
/* TX power = 0 */

/*RX_FILTER_BW					135 */
/* CURRENT_OPTIMIZED				0 */

#define CC1101_SETTING_IOCFG2           0x04
#define CC1101_SETTING_IOCFG1           0x2E
#define CC1101_SETTING_IOCFG0           0x01
#define CC1101_SETTING_FIFOTHR          0x03
#define CC1101_SETTING_SYNC1            0x90
#define CC1101_SETTING_SYNC0            0x4E
#define CC1101_SETTING_PKTLEN           0x80
#define CC1101_SETTING_PKTCTRL1         0x04
#define CC1101_SETTING_PKTCTRL0         0x05
#define CC1101_SETTING_ADDR             0x00
#define CC1101_SETTING_CHANNR           0x00
#define CC1101_SETTING_FSCTRL1          0x06
#define CC1101_SETTING_FSCTRL0          0x00
#define CC1101_SETTING_FREQ2            0x21
#define CC1101_SETTING_FREQ1            0x32
#define CC1101_SETTING_FREQ0            0x76
#define CC1101_SETTING_MDMCFG4          0xAA
#define CC1101_SETTING_MDMCFG3          0xF8
#define CC1101_SETTING_MDMCFG2          0x13
#define CC1101_SETTING_MDMCFG1          0x22
#define CC1101_SETTING_MDMCFG0          0xF8
#define CC1101_SETTING_DEVIATN          0x40
#define CC1101_SETTING_MCSM2            0x07
#define CC1101_SETTING_MCSM1            0x3F
#define CC1101_SETTING_MCSM0            0x08
#define CC1101_SETTING_FOCCFG           0x16
#define CC1101_SETTING_BSCFG            0x6C
#define CC1101_SETTING_AGCCTRL2         0x43
#define CC1101_SETTING_AGCCTRL1         0x40
#define CC1101_SETTING_AGCCTRL0         0x91
#define CC1101_SETTING_WOREVT1          0x87
#define CC1101_SETTING_WOREVT0          0x6B
#define CC1101_SETTING_WORCTRL          0xFB
#define CC1101_SETTING_FREND1           0x56
#define CC1101_SETTING_FREND0           0x10
#define CC1101_SETTING_FSCAL3           0xE9
#define CC1101_SETTING_FSCAL2           0x2A
#define CC1101_SETTING_FSCAL1           0x00
#define CC1101_SETTING_FSCAL0           0x1F
#define CC1101_SETTING_RCCTRL1          0x41
#define CC1101_SETTING_RCCTRL0          0x00
#define CC1101_SETTING_FSTEST           0x59
#define CC1101_SETTING_PTEST            0x7F
#define CC1101_SETTING_AGCTEST          0x3F
#define CC1101_SETTING_TEST2            0x81
#define CC1101_SETTING_TEST1            0x35
#define CC1101_SETTING_TEST0            0x09


#define CC1101_SETTING_PARTNUM          0x00
#define CC1101_SETTING_VERSION          0x04
#define CC1101_SETTING_FREQEST          0x00
#define CC1101_SETTING_LQI              0x00
#define CC1101_SETTING_RSSI             0x00
#define CC1101_SETTING_MARCSTATE        0x00
#define CC1101_SETTING_WORTIME1         0x00
#define CC1101_SETTING_WORTIME0         0x00
#define CC1101_SETTING_PKTSTATUS        0x00
#define CC1101_SETTING_VCO_VC_DAC       0x00
#define CC1101_SETTING_TXBYTES          0x00
#define CC1101_SETTING_RXBYTES          0x00
#define CC1101_SETTING_RCCTRL1_STATUS   0x00
#define CC1101_SETTING_RCCTRL0_STATUS   0x00

#define CC1101_RF_REGS  44


const struct cc1101_rf_registers_set cc1101_rf_settings = {
	.channel_limit = 33,
	.registers = {
		CC1101_SETTING_FIFOTHR,
		CC1101_SETTING_SYNC1,
		CC1101_SETTING_SYNC0,
		CC1101_SETTING_PKTLEN,
		CC1101_SETTING_PKTCTRL1,
		CC1101_SETTING_PKTCTRL0,
		CC1101_SETTING_ADDR,
		CC1101_SETTING_CHANNR,
		CC1101_SETTING_FSCTRL1,
		CC1101_SETTING_FSCTRL0,
		CC1101_SETTING_FREQ2,
		CC1101_SETTING_FREQ1,
		CC1101_SETTING_FREQ0,
		CC1101_SETTING_MDMCFG4,
		CC1101_SETTING_MDMCFG3,
		CC1101_SETTING_MDMCFG2,
		CC1101_SETTING_MDMCFG1,
		CC1101_SETTING_MDMCFG0,
		CC1101_SETTING_DEVIATN,
		CC1101_SETTING_MCSM2,
		CC1101_SETTING_MCSM1,
		CC1101_SETTING_MCSM0,
		CC1101_SETTING_FOCCFG,
		CC1101_SETTING_BSCFG,
		CC1101_SETTING_AGCCTRL2,
		CC1101_SETTING_AGCCTRL1,
		CC1101_SETTING_AGCCTRL0,
		CC1101_SETTING_WOREVT1,
		CC1101_SETTING_WOREVT0,
		CC1101_SETTING_WORCTRL,
		CC1101_SETTING_FREND1,
		CC1101_SETTING_FREND0,
		CC1101_SETTING_FSCAL3,
		CC1101_SETTING_FSCAL2,
		CC1101_SETTING_FSCAL1,
		CC1101_SETTING_FSCAL0,
		CC1101_SETTING_RCCTRL1,
		CC1101_SETTING_RCCTRL0,
		CC1101_SETTING_FSTEST,
		CC1101_SETTING_PTEST,
		CC1101_SETTING_AGCTEST,
		CC1101_SETTING_TEST2,
		CC1101_SETTING_TEST1,
		CC1101_SETTING_TEST0
	}
};

#endif /* __IEEE802154_CC1101_RF_H__ */
