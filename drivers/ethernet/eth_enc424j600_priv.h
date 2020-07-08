/* ENC424J600 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/gpio.h>

#ifndef _ENC424J600_
#define _ENC424J600_

/* Bank 0 Registers */
#define ENC424J600_SFR0_ETXSTL			0x00
#define ENC424J600_SFR0_ETXSTH			0x01
#define ENC424J600_SFR0_ETXLENL			0x02
#define ENC424J600_SFR0_ETXLENH			0x03
#define ENC424J600_SFR0_ERXSTL			0x04
#define ENC424J600_SFR0_ERXSTH			0x05
#define ENC424J600_SFR0_ERXTAILL		0x06
#define ENC424J600_SFR0_ERXTAILH		0x07
#define ENC424J600_SFR0_ERXHEADL		0x08
#define ENC424J600_SFR0_ERXHEADH		0x09
#define ENC424J600_SFR0_EDMASTL			0x0A
#define ENC424J600_SFR0_EDMASTH			0x0B
#define ENC424J600_SFR0_EDMALENL		0x0C
#define ENC424J600_SFR0_EDMALENH		0x0D
#define ENC424J600_SFR0_EDMADSTL		0x0E
#define ENC424J600_SFR0_EDMADSTH		0x0F
#define ENC424J600_SFR0_EDMACSL			0x10
#define ENC424J600_SFR0_EDMACSH			0x11
#define ENC424J600_SFR0_ETXSTATL		0x12
#define ENC424J600_SFR0_ETXSTATH		0x13
#define ENC424J600_SFR0_ETXWIREL		0x14
#define ENC424J600_SFR0_ETXWIREH		0x15
/* Common Registers */
#define ENC424J600_SFRX_EUDASTL			0x16
#define ENC424J600_SFRX_EUDASTH			0x17
#define ENC424J600_SFRX_EUDANDL			0x18
#define ENC424J600_SFRX_EUDANDH			0x19
#define ENC424J600_SFRX_ESTATL			0x1A
#define ENC424J600_SFRX_ESTATH			0x1B
#define ENC424J600_SFRX_EIRL			0x1C
#define ENC424J600_SFRX_EIRH			0x1D
#define ENC424J600_SFRX_ECON1L			0x1E
#define ENC424J600_SFRX_ECON1H			0x1F

/* Bank 1 Registers */
#define ENC424J600_SFR1_EHT1L			0x20
#define ENC424J600_SFR1_EHT1H			0x21
#define ENC424J600_SFR1_EHT2L			0x22
#define ENC424J600_SFR1_EHT2H			0x23
#define ENC424J600_SFR1_EHT3L			0x24
#define ENC424J600_SFR1_EHT3H			0x25
#define ENC424J600_SFR1_EHT4L			0x26
#define ENC424J600_SFR1_EHT4H			0x27
#define ENC424J600_SFR1_EPMM1L			0x28
#define ENC424J600_SFR1_EPMM1H			0x29
#define ENC424J600_SFR1_EPMM2L			0x2A
#define ENC424J600_SFR1_EPMM2H			0x2B
#define ENC424J600_SFR1_EPMM3L			0x2C
#define ENC424J600_SFR1_EPMM3H			0x2D
#define ENC424J600_SFR1_EPMM4L			0x2E
#define ENC424J600_SFR1_EPMM4H			0x2F
#define ENC424J600_SFR1_EPMCSL			0x30
#define ENC424J600_SFR1_EPMCSH			0x31
#define ENC424J600_SFR1_EPMOL			0x32
#define ENC424J600_SFR1_EPMOH			0x33
#define ENC424J600_SFR1_ERXFCONL		0x34
#define ENC424J600_SFR1_ERXFCONH		0x35

/* Bank 2 Registers */
#define ENC424J600_SFR2_MACON1L			0x40
#define ENC424J600_SFR2_MACON1H			0x41
#define ENC424J600_SFR2_MACON2L			0x42
#define ENC424J600_SFR2_MACON2H			0x43
#define ENC424J600_SFR2_MABBIPGL		0x44
#define ENC424J600_SFR2_MABBIPGH		0x45
#define ENC424J600_SFR2_MAIPGL			0x46
#define ENC424J600_SFR2_MAIPGH			0x47
#define ENC424J600_SFR2_MACLCONL		0x48
#define ENC424J600_SFR2_MACLCONH		0x49
#define ENC424J600_SFR2_MAMXFLL			0x4A
#define ENC424J600_SFR2_MAMXFLH			0x4B
#define ENC424J600_SFR2_MICMDL			0x52
#define ENC424J600_SFR2_MICMDH			0x53
#define ENC424J600_SFR2_MIREGADRL		0x54
#define ENC424J600_SFR2_MIREGADRH		0x55


/* Bank 3 Registers */
#define ENC424J600_SFR3_MAADR3L			0x60
#define ENC424J600_SFR3_MAADR3H			0x61
#define ENC424J600_SFR3_MAADR2L			0x62
#define ENC424J600_SFR3_MAADR2H			0x63
#define ENC424J600_SFR3_MAADR1L			0x64
#define ENC424J600_SFR3_MAADR1H			0x65
#define ENC424J600_SFR3_MIWRL			0x66
#define ENC424J600_SFR3_MIWRH			0x67
#define ENC424J600_SFR3_MIRDL			0x68
#define ENC424J600_SFR3_MIRDH			0x69
#define ENC424J600_SFR3_MISTATL			0x6A
#define ENC424J600_SFR3_MISTATH			0x6B
#define ENC424J600_SFR3_EPAUSL			0x6C
#define ENC424J600_SFR3_EPAUSH			0x6D
#define ENC424J600_SFR3_ECON2L			0x6E
#define ENC424J600_SFR3_ECON2H			0x6F
#define ENC424J600_SFR3_ERXWML			0x70
#define ENC424J600_SFR3_ERXWMH			0x71
#define ENC424J600_SFR3_EIEL			0x72
#define ENC424J600_SFR3_EIEH			0x73
#define ENC424J600_SFR3_EIDLEDL			0x74
#define ENC424J600_SFR3_EIDLEDH			0x75

/* Unbanked SFRs */
#define ENC424J600_SFR4_EGPDATA			0x80
#define ENC424J600_SFR4_ERXDATA			0x82
#define ENC424J600_SFR4_EUDADATA		0x84
#define ENC424J600_SFR4_EGPRDPTL		0x86
#define ENC424J600_SFR4_EGPRDPTH		0x87
#define ENC424J600_SFR4_EGPWRPTL		0x88
#define ENC424J600_SFR4_EGPWRPTH		0x89
#define ENC424J600_SFR4_ERXRDPTL		0x8A
#define ENC424J600_SFR4_ERXRDPTH		0x8B
#define ENC424J600_SFR4_ERXWRPTL		0x8C
#define ENC424J600_SFR4_ERXWRPTH		0x8D
#define ENC424J600_SFR4_EUDARDPTL		0x8E
#define ENC424J600_SFR4_EUDARDPTH		0x8F
#define ENC424J600_SFR4_EUDAWRPTL		0x90
#define ENC424J600_SFR4_EUDAWRPTH		0x91

/* PHY Registers */
#define ENC424J600_PSFR_PHCON1			(BIT(8) | 0x00)
#define ENC424J600_PSFR_PHSTAT1			(BIT(8) | 0x01)
#define ENC424J600_PSFR_PHANA			(BIT(8) | 0x04)
#define ENC424J600_PSFR_PHANLPA			(BIT(8) | 0x05)
#define ENC424J600_PSFR_PHANE			(BIT(8) | 0x06)
#define ENC424J600_PSFR_PHCON2			(BIT(8) | 0x11)
#define ENC424J600_PSFR_PHSTAT2			(BIT(8) | 0x1B)
#define ENC424J600_PSFR_PHSTAT3			(BIT(8) | 0x1F)

/* SPI Instructions */
#define ENC424J600_1BC_B0SEL			0xC0
#define ENC424J600_1BC_B1SEL			0xC2
#define ENC424J600_1BC_B2SEL			0xC4
#define ENC424J600_1BC_B3SEL			0xC6
#define ENC424J600_1BC_SETETHRST		0xCA
#define ENC424J600_1BC_FCDISABLE		0xE0
#define ENC424J600_1BC_FCSINGLE			0xE2
#define ENC424J600_1BC_FCMULTIPLE		0xE4
#define ENC424J600_1BC_FCCLEAR			0xE6
#define ENC424J600_1BC_SETPKTDEC		0xCC
#define ENC424J600_1BC_DMASTOP			0xD2
#define ENC424J600_1BC_DMACKSUM			0xD8
#define ENC424J600_1BC_DMACKSUMS		0xDA
#define ENC424J600_1BC_DMACOPY			0xDC
#define ENC424J600_1BC_DMACOPYS			0xDE
#define ENC424J600_1BC_SETTXRTS			0xD4
#define ENC424J600_1BC_ENABLERX			0xE8
#define ENC424J600_1BC_DISABLERX		0xEA
#define ENC424J600_1BC_SETEIE			0xEC
#define ENC424J600_1BC_CLREIE			0xEE
#define ENC424J600_2BC_RBSEL			0xC8
#define ENC424J600_3BC_WGPRDPT			0x60
#define ENC424J600_3BC_RGPRDPT			0x62
#define ENC424J600_3BC_WRXRDPT			0x64
#define ENC424J600_3BC_RRXRDPT			0x66
#define ENC424J600_3BC_WUDARDPT			0x68
#define ENC424J600_3BC_RUDARDPT			0x6A
#define ENC424J600_3BC_WGPWRPT			0x6C
#define ENC424J600_3BC_RGPWRPT			0x6E
#define ENC424J600_3BC_WRXWRPT			0x70
#define ENC424J600_3BC_RRXWRPT			0x72
#define ENC424J600_3BC_WUDAWRPT			0x74
#define ENC424J600_3BC_RUDAWRPT			0x76
#define ENC424J600_NBC_RCR			0x00
#define ENC424J600_NBC_WCR			0x40
#define ENC424J600_NBC_RCRU			0x20
#define ENC424J600_NBC_WCRU			0x22
#define ENC424J600_NBC_BFS			0x80
#define ENC424J600_NBC_BFC			0xA0
#define ENC424J600_NBC_BFSU			0x24
#define ENC424J600_NBC_BFCU			0x26
#define ENC424J600_NBC_RGPDATA			0x28
#define ENC424J600_NBC_WGPDATA			0x2A
#define ENC424J600_NBC_RRXDATA			0x2C
#define ENC424J600_NBC_WRXDATA			0x2E
#define ENC424J600_NBC_RUDADATA			0x30
#define ENC424J600_NBC_WUDADATA			0x32

/* Significant bits */
#define ENC424J600_MICMD_MIIRD			BIT(0)

#define ENC424J600_MISTAT_BUSY			BIT(0)

#define ENC424J600_ESTAT_RXBUSY			BIT(13)
#define ENC424J600_ESTAT_CLKRDY			BIT(12)
#define ENC424J600_ESTAT_PHYLNK			BIT(8)

#define ENC424J600_MACON2_FULDPX		BIT(0)

#define ENC424J600_ERXFCON_CRCEN		BIT(6)
#define ENC424J600_ERXFCON_RUNTEEN		BIT(5)
#define ENC424J600_ERXFCON_RUNTEN		BIT(4)
#define ENC424J600_ERXFCON_UCEN			BIT(3)
#define ENC424J600_ERXFCON_NOTMEEN		BIT(2)
#define ENC424J600_ERXFCON_MCEN			BIT(1)
#define ENC424J600_ERXFCON_BCEN			BIT(0)

#define ENC424J600_PHANA_ADNP			BIT(15)
#define ENC424J600_PHANA_ADFAULT		BIT(13)
#define ENC424J600_PHANA_ADPAUS_SYMMETRIC_ONLY	BIT(10)
#define ENC424J600_PHANA_AD100FD		BIT(8)
#define ENC424J600_PHANA_AD100			BIT(7)
#define ENC424J600_PHANA_AD10FD			BIT(6)
#define ENC424J600_PHANA_AD10			BIT(5)
#define ENC424J600_PHANA_ADIEEE_DEFAULT		BIT(0)

#define ENC424J600_EIE_INTIE			BIT(15)
#define ENC424J600_EIE_MODEXIE			BIT(14)
#define ENC424J600_EIE_HASHIE			BIT(13)
#define ENC424J600_EIE_AESIE			BIT(12)
#define ENC424J600_EIE_LINKIE			BIT(11)
#define ENC424J600_EIE_PKTIE			BIT(6)
#define ENC424J600_EIE_DMAIE			BIT(5)
#define ENC424J600_EIE_TXIE			BIT(3)
#define ENC424J600_EIE_TXABTIE			BIT(2)
#define ENC424J600_EIE_RXABTIE			BIT(1)
#define ENC424J600_EIE_PCFULIE			BIT(0)

#define ENC424J600_ECON1_PKTDEC			BIT(8)
#define ENC424J600_ECON1_TXRTS			BIT(1)
#define ENC424J600_ECON1_RXEN			BIT(0)

#define ENC424J600_ECON2_ETHEN			BIT(15)
#define ENC424J600_ECON2_STRCH			BIT(14)

#define ENC424J600_EIR_LINKIF			BIT(11)
#define ENC424J600_EIR_PKTIF			BIT(6)
#define ENC424J600_EIR_TXIF			BIT(3)
#define ENC424J600_EIR_TXABTIF			BIT(2)
#define ENC424J600_EIR_RXABTIF			BIT(1)
#define ENC424J600_EIR_PCFULIF			BIT(0)

#define ENC424J600_PHCON1_PSLEEP		BIT(11)
#define ENC424J600_PHCON1_RENEG			BIT(9)
#define ENC424J600_PHSTAT3_SPDDPX_FD		BIT(4)
#define ENC424J600_PHSTAT3_SPDDPX_100		BIT(3)
#define ENC424J600_PHSTAT3_SPDDPX_10		BIT(2)

/* Buffer Configuration */
#define ENC424J600_TXSTART			0x0000U
#define ENC424J600_TXEND			0x2FFFU
#define ENC424J600_RXSTART			(ENC424J600_TXEND + 1)
#define ENC424J600_RXEND			0x5FFFU
#define ENC424J600_EUDAST_DEFAULT		0x6000U
#define ENC424J600_EUDAND_DEFAULT		(ENC424J600_EUDAST + 1)

/* Status vectors array size */
#define ENC424J600_RSV_SIZE			6U
#define ENC424J600_PTR_NXP_PKT_SIZE		2U

/* Full-Duplex mode Inter-Packet Gap default value */
#define ENC424J600_MABBIPG_DEFAULT		0x15U

#define ENC424J600_DEFAULT_NUMOF_RETRIES	3U

/* Delay for PHY write/read operations (25.6 us) */
#define ENC424J600_PHY_ACCESS_DELAY		26U

#define ENC424J600_PHY_READY_DELAY		260U

struct enc424j600_config {
	const char *gpio_port;
	uint8_t gpio_pin;
	gpio_dt_flags_t gpio_flags;
	const char *spi_port;
	gpio_pin_t spi_cs_pin;
	gpio_dt_flags_t spi_cs_dt_flags;
	const char *spi_cs_port;
	uint32_t spi_freq;
	uint8_t spi_slave;
	uint8_t full_duplex;
	int32_t timeout;
};

struct enc424j600_runtime {
	struct net_if *iface;
	const struct device *dev;

	K_KERNEL_STACK_MEMBER(thread_stack,
			      CONFIG_ETH_ENC424J600_RX_THREAD_STACK_SIZE);

	struct k_thread thread;
	uint8_t mac_address[6];
	const struct device *gpio;
	const struct device *spi;
	struct spi_cs_control spi_cs;
	struct spi_config spi_cfg;
	struct gpio_callback gpio_cb;
	struct k_sem tx_rx_sem;
	struct k_sem int_sem;
	uint16_t next_pkt_ptr;
	bool suspended : 1;
	bool iface_initialized : 1;
};

#endif /*_ENC424J600_*/
