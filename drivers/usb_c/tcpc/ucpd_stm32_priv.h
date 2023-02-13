/*
 * Copyright (c) 2022  The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USBC_DEVICE_UCPD_STM32_PRIV_H_
#define ZEPHYR_DRIVERS_USBC_DEVICE_UCPD_STM32_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>
#include <zephyr/drivers/pinctrl.h>
#include <stm32_ll_ucpd.h>

/**
 * @brief The packet type(SOP*) consists of 2-bytes
 */
#define PACKET_TYPE_SIZE 2

/**
 * @brief The message header consists of 2-bytes
 */
#define MSG_HEADER_SIZE 2

/**
 * @brief USB PD message buffer length.
 */
#define UCPD_BUF_LEN (PD_MAX_EXTENDED_MSG_LEN +	\
		      PACKET_TYPE_SIZE +	\
		      MSG_HEADER_SIZE)

/**
 * @brief UCPD alert mask used for enabling alerts
 *        used to receive a message
 */
#define UCPD_IMR_RX_INT_MASK (UCPD_IMR_RXNEIE |	     \
			      UCPD_IMR_RXORDDETIE |  \
			      UCPD_IMR_RXHRSTDETIE | \
			      UCPD_IMR_RXOVRIE |     \
			      UCPD_IMR_RXMSGENDIE)
/**
 * @brief UCPD alert mask used for clearing alerts
 *        used to receive a message
 */
#define UCPD_ICR_RX_INT_MASK (UCPD_ICR_RXORDDETCF |  \
			      UCPD_ICR_RXHRSTDETCF | \
			      UCPD_ICR_RXOVRCF |     \
			      UCPD_ICR_RXMSGENDCF)

/**
 * @brief UCPD alert mask used for enabling alerts
 *        used to transmit a message
 */
#define UCPD_IMR_TX_INT_MASK (UCPD_IMR_TXISIE |	     \
			      UCPD_IMR_TXMSGDISCIE | \
			      UCPD_IMR_TXMSGSENTIE | \
			      UCPD_IMR_TXMSGABTIE |  \
			      UCPD_IMR_TXUNDIE)

/**
 * @brief UCPD alert mask used for clearing alerts
 *        used to transmit a message
 */
#define UCPD_ICR_TX_INT_MASK (UCPD_ICR_TXMSGDISCCF | \
			      UCPD_ICR_TXMSGSENTCF | \
			      UCPD_ICR_TXMSGABTCF |  \
			      UCPD_ICR_TXUNDCF)

/**
 * @brief UCPD alert mask for all alerts
 */
#define UCPD_ICR_ALL_INT_MASK (UCPD_ICR_FRSEVTCF |    \
			       UCPD_ICR_TYPECEVT2CF | \
			       UCPD_ICR_TYPECEVT1CF | \
			       UCPD_ICR_RXMSGENDCF |  \
			       UCPD_ICR_RXOVRCF |     \
			       UCPD_ICR_RXHRSTDETCF | \
			       UCPD_ICR_RXORDDETCF |  \
			       UCPD_ICR_TXUNDCF |     \
			       UCPD_ICR_HRSTSENTCF |  \
			       UCPD_ICR_HRSTDISCCF |  \
			       UCPD_ICR_TXMSGABTCF |  \
			       UCPD_ICR_TXMSGSENTCF | \
			       UCPD_ICR_TXMSGDISCCF)

/**
 * @brief For STM32G0X devices, this macro enables
 *	  Dead Battery functionality
 */
#define UCPD_CR_DBATTEN BIT(15)

/**
 * @brief Map UCPD ANASUB value to TCPC RP value
 *
 * @param r UCPD ANASUB value
 */
#define UCPD_ANASUB_TO_RP(r) ((r - 1) & 0x3)

/**
 * @brief Map TCPC RP value to UCPD ANASUB value
 *
 * @param r TCPC RP value
 */
#define UCPD_RP_TO_ANASUB(r) ((r + 1) & 0x3)

/**
 * @brief Create value for writing to UCPD CR ANASUBMOD
 */
#define STM32_UCPD_CR_ANASUBMODE_VAL(x) ((x) << UCPD_CR_ANASUBMODE_Pos)

/**
 * @brief UCPD VSTATE CCx open value when in source mode
 */
#define STM32_UCPD_SR_VSTATE_OPEN 3

/**
 * @brief UCPD VSTATE CCx RA value when in source mode
 */
#define STM32_UCPD_SR_VSTATE_RA 0

/**
 * @brief PD message send retry count for Rev 2.0
 */
#define UCPD_N_RETRY_COUNT_REV20 3

/**
 * @brief PD message send retry count for Rev 3.0
 */
#define UCPD_N_RETRY_COUNT_REV30 2

/**
 * @brief Events for ucpd_alert_handler
 */
enum {
	/* Request to send a goodCRC message */
	UCPD_EVT_GOOD_CRC_REQ,
	/* Request to send a power delivery message */
	UCPD_EVT_TCPM_MSG_REQ,
	/* Request to send a Hard Reset message */
	UCPD_EVT_HR_REQ,
	/* Transmission of power delivery message failed */
	UCPD_EVT_TX_MSG_FAIL,
	/* Transmission of power delivery message was discarded */
	UCPD_EVT_TX_MSG_DISC,
	/* Transmission of power delivery message was successful */
	UCPD_EVT_TX_MSG_SUCCESS,
	/* Transmission of Hard Reset message was successful */
	UCPD_EVT_HR_DONE,
	/* Transmission of Hard Reset message failed */
	UCPD_EVT_HR_FAIL,
	/* A goodCRC message was received */
	UCPD_EVT_RX_GOOD_CRC,
	/* A power delivery message was received */
	UCPD_EVT_RX_MSG,
	/* A CC event occurred */
	UCPD_EVT_EVENT_CC,
	/* A Hard Reset message was received */
	UCPD_EVT_HARD_RESET_RECEIVED,
};

/**
 * @brief GoodCRC message header roles
 */
struct msg_header_info {
	/* Power Role */
	enum tc_power_role pr;
	/* Data Role */
	enum tc_data_role dr;
};

/**
 * @brief States for managing TX messages
 */
enum ucpd_state {
	/* Idle state */
	STATE_IDLE,
	/* Transmitting a message state */
	STATE_ACTIVE_TCPM,
	/* Transmitting a goodCRC message state */
	STATE_ACTIVE_CRC,
	/* Transmitting a Hard Reset message state */
	STATE_HARD_RESET,
	/* Waiting for a goodCRC message state */
	STATE_WAIT_CRC_ACK
};

/**
 * @brief Tx messages are initiated either by the application or from
 *        the driver when a GoodCRC ack message needs to be sent.
 */
enum ucpd_tx_msg {
	/* Default value for not sending a message */
	TX_MSG_NONE     = -1,
	/* Message initiated from the application */
	TX_MSG_TCPM     = 0,
	/* Message initiated from the driver */
	TX_MSG_GOOD_CRC = 1,
	/* Total number sources that can initiate a message */
	TX_MSG_TOTAL    = 2
};

/**
 * @brief Message from application mask
 */
#define MSG_TCPM_MASK      BIT(TX_MSG_TCPM)

/**
 * @brief Message from driver mask
 */
#define MSG_GOOD_CRC_MASK  BIT(TX_MSG_GOOD_CRC)

/**
 * @brief Buffer for holding the received message
 */
union pd_buffer {
	/* Power Delivery message header */
	uint16_t header;
	/* Power Deliver message data including the message header */
	uint8_t msg[UCPD_BUF_LEN];
};

/**
 * @brief Struct used for transmitting a power delivery message
 */
struct ucpd_tx_desc {
	/* Type of the message */
	enum pd_packet_type type;
	/* Length of the message */
	int msg_len;
	/* Index of the current byte to transmit */
	int msg_index;
	/* Power Delivery message to transmit */
	union pd_buffer data;
};

/**
 * @brief Alert handler information
 */
struct alert_info {
	/* Runtime device structure */
	const struct device *dev;
	/* Application supplied data that's passed to the
	 * application's alert handler callback
	 **/
	void *data;
	/* Application's alert handler callback */
	tcpc_alert_handler_cb_t handler;
	/*
	 * Kernel worker used to call the application's
	 * alert handler callback
	 */
	struct k_work work;
	/* Event flags used in the kernel worker */
	atomic_t evt;
};

/**
 * @brief Driver config
 */
struct tcpc_config {
	/* STM32 UCPC CC pin control */
	const struct pinctrl_dev_config *ucpd_pcfg;
	/* STM32 UCPD port */
	UCPD_TypeDef *ucpd_port;
	/* STM32 UCPD parameters */
	LL_UCPD_InitTypeDef ucpd_params;
	/* STM32 UCPD dead battery support */
	bool ucpd_dead_battery;
};

/**
 * @brief Driver data
 */
struct tcpc_data {
	/* VCONN callback function */
	tcpc_vconn_control_cb_t vconn_cb;
	/* VCONN Discharge callback function */
	tcpc_vconn_discharge_cb_t vconn_discharge_cb;
	/* Alert information */
	struct alert_info alert_info;

	/* CC Rp value */
	enum tc_rp_value rp;

	/* PD Rx variables */

	/* Number of RX bytes received */
	int ucpd_rx_byte_count;
	/* Buffer to hold the received bytes */
	uint8_t ucpd_rx_buffer[UCPD_BUF_LEN];
	/* GoodCRC message ID */
	int ucpd_crc_id;
	/* Flag to receive or ignore SOP Prime messages */
	bool ucpd_rx_sop_prime_enabled;
	/* Flag set to true when receiving a message */
	bool ucpd_rx_msg_active;
	/* Flag set to true when in RX BIST Mode */
	bool ucpd_rx_bist_mode;

	/* Tx message variables */

	/* Buffers to hold messages ready for transmission */
	struct ucpd_tx_desc ucpd_tx_buffers[TX_MSG_TOTAL];
	/* Current buffer being transmitted */
	struct ucpd_tx_desc *ucpd_tx_active_buffer;
	/* Request to send a transmission message */
	int ucpd_tx_request;
	/* State of the TX state machine */
	enum ucpd_state ucpd_tx_state;
	/* Transmission message ID */
	int msg_id_match;
	/* Retry count on failure to transmit a message */
	int tx_retry_count;
	/* Max number of reties before giving up */
	int tx_retry_max;

	/* GoodCRC message Header */
	struct msg_header_info msg_header;
	/* Track VCONN on/off state */
	bool ucpd_vconn_enable;
	/* Track CC line that VCONN was active on */
	enum tc_cc_polarity ucpd_vconn_cc;

	/* Timer for amount of time to wait for receiving a GoodCRC */
	struct k_timer goodcrc_rx_timer;
};

#endif /* ZEPHYR_DRIVERS_USBC_DEVICE_UCPD_STM32_PRIV_H_ */
