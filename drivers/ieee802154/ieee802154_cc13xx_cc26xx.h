/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * References are to the IEEE 802.15.4-2020 standard.
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC13XX_CC26XX_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC13XX_CC26XX_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_radio.h>

#include <ti/drivers/rf/RF.h>

#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_ieee_cmd.h>
#include <driverlib/rf_mailbox.h>

/* For O-QPSK the physical and MAC timing symbol rates are the same, see section 12.3.3. */
#define IEEE802154_2450MHZ_OQPSK_SYMBOLS_PER_SECOND                                                \
	IEEE802154_PHY_SYMBOLS_PER_SECOND(IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS)

/* PHY PIB attribute phyCcaMode - CCA Mode 3: Carrier sense with energy above threshold, see
 * section 11.3, table 11-2 and section 10.2.8
 */
#define IEEE802154_PHY_CCA_MODE 3

#define IEEE802154_PHY_SHR_DURATION 10 /* in symbols, 8 preamble and 2 SFD, see section 12.1.2 */

#define IEEE802154_PHY_SYMBOLS_PER_OCTET 2 /* see section 12.2.1 */

/* ACK is 2 bytes for PHY header + 2 bytes MAC header + 2 bytes MAC footer */
#define IEEE802154_ACK_FRAME_OCTETS 6

/* IEEE 802.15.4-2006 MAC PIB attributes (7.4.2)
 *
 * The macAckWaitDuration attribute does not include aUnitBackoffPeriod for
 * non-beacon enabled PANs (See IEEE 802.15.4-2006 7.5.6.4.2)
 */
#define IEEE802154_MAC_ACK_WAIT_DURATION                                                           \
	(IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT + IEEE802154_PHY_SHR_DURATION +                  \
	 IEEE802154_ACK_FRAME_OCTETS * IEEE802154_PHY_SYMBOLS_PER_OCTET)

#define CC13XX_CC26XX_RAT_CYCLES_PER_SECOND 4000000

#define CC13XX_CC26XX_NUM_RX_BUF 2

/* Three additional bytes for length, RSSI and correlation values from CPE. */
#define CC13XX_CC26XX_RX_BUF_SIZE (IEEE802154_MAX_PHY_PACKET_SIZE + 3)

#define CC13XX_CC26XX_CPE0_IRQ (INT_RFC_CPE_0 - 16)
#define CC13XX_CC26XX_CPE1_IRQ (INT_RFC_CPE_1 - 16)

#define CC13XX_CC26XX_RECEIVER_SENSITIVITY -100
#define CC13XX_CC26XX_INVALID_RSSI INT8_MIN

struct ieee802154_cc13xx_cc26xx_data {
	RF_Handle rf_handle;
	RF_Object rf_object;

	struct net_if *iface;

	uint8_t mac[8]; /* in big endian */

	struct k_mutex tx_mutex;

	dataQueue_t rx_queue;
	rfc_dataEntryPointer_t rx_entry[CC13XX_CC26XX_NUM_RX_BUF];
	uint8_t rx_data[CC13XX_CC26XX_NUM_RX_BUF]
		    [CC13XX_CC26XX_RX_BUF_SIZE] __aligned(4);

	volatile rfc_CMD_FS_t cmd_fs;
	volatile rfc_CMD_IEEE_CCA_REQ_t cmd_ieee_cca_req;
	volatile rfc_CMD_IEEE_RX_t cmd_ieee_rx;
	volatile rfc_CMD_IEEE_CSMA_t cmd_ieee_csma;
	volatile rfc_CMD_IEEE_TX_t cmd_ieee_tx;
	volatile rfc_CMD_IEEE_RX_ACK_t cmd_ieee_rx_ack;
#if defined(CONFIG_SOC_CC1352R) || defined(CONFIG_SOC_CC2652R) || \
	defined(CONFIG_SOC_CC1352R7) || defined(CONFIG_SOC_CC2652R7)
	volatile rfc_CMD_RADIO_SETUP_t cmd_radio_setup;
#elif defined(CONFIG_SOC_CC1352P) || defined(CONFIG_SOC_CC2652P) || \
	defined(CONFIG_SOC_CC1352P7) || defined(CONFIG_SOC_CC2652P7)
	volatile rfc_CMD_RADIO_SETUP_PA_t cmd_radio_setup;
#else
	BUILD_ASSERT(false, "unknown model");
#endif /* CONFIG_SOC_CCxx52x */

	volatile int16_t saved_cmdhandle;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC13XX_CC26XX_H_ */
