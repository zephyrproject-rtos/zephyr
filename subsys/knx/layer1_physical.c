/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "layer2_data_link.h"
#include "layer1_physical.h"
#include "object_device.h"

LOG_MODULE_REGISTER(knx_l1, CONFIG_KNX_STACK_LOG_LEVEL);

/*
 * RX path:
 *   rx_frame[]    — raw wire-frame bytes accumulated by Ph_Data__ind (ISR).
 *   rx_frame_len  — number of valid bytes in rx_frame.
 */
static volatile uint8_t rx_frame[MAX_KNX_TELEGRAM_SIZE];
static volatile uint16_t rx_frame_len;
/*
 * Set when the driver reported Ind_parity_error / Ind_framing_error for an
 * octet of the frame currently being accumulated.  In 9-bit mode a parity
 * mismatch means the NCN5130 flagged an acceptance-window or pulse-duration
 * error on that octet, so the frame is not trustworthy and is dropped at EOF
 * instead of being handed to L2.
 */
static volatile bool rx_frame_corrupt;

/* #define VERBOSE_DEBUG_FRAME_CONTENT */

#ifdef CONFIG_LOG
static void LOG_FRAME(struct knx_pkt *pkt, char *buf, int max_len)
{
	int pos = 0;

	pos += snprintf(buf + pos, max_len - pos, "[");
	for (int i = 0; i < pkt->buf_len; i++) {
		pos += snprintf(buf + pos, max_len - pos, "%02X ", pkt->buf[i]);
	}
	pos += snprintf(buf + pos, max_len - pos, "]");
#if defined(VERBOSE_DEBUG_FRAME_CONTENT)
	/*
	 * Header decode is only needed by this block, so it lives inside the
	 * guard — otherwise every field below is set-but-unused and trips
	 * -Wunused-but-set-variable.
	 */
	bool std_frame = (pkt->buf[0] & 0x80u);
	/*
	 * CTRL bit 5 is the repeat flag and its polarity is INVERTED
	 * (KNX spec 3/2/2 §2.2.2): r = 0 means repeated, r = 1 means not
	 * repeated. The previous code tested bit 6 (0x40) and read the result
	 * the wrong way round, so it printed REPEAT on every ordinary frame.
	 */
	bool repeat = (pkt->buf[0] & 0x20u) == 0u;
	uint8_t lsdu_offset = std_frame ? 6u : 7u;
	knx_addr_type_t addr_type =
		std_frame ? ((pkt->buf[5] & 0x80u) >> 7) : ((pkt->buf[1] & 0x80u) >> 7);
	knx_addr_t src = std_frame ? (((uint16_t)pkt->buf[1] << 8) | pkt->buf[2])
				   : (((uint16_t)pkt->buf[2] << 8) | pkt->buf[3]);
	knx_addr_t dst = std_frame ? (((uint16_t)pkt->buf[3] << 8) | pkt->buf[4])
				   : (((uint16_t)pkt->buf[4] << 8) | pkt->buf[5]);
	knx_priority_t priority = (pkt->buf[0] & 0x0Cu) >> 2;
	uint8_t hop_count = std_frame ? ((pkt->buf[5] >> 4) & 0x07u) : ((pkt->buf[1] >> 4) & 0x07u);
	uint8_t lsdu_len = std_frame ? ((pkt->buf[5] & 0x0Fu) + 1u) : (pkt->buf[6] + 1u);
	uint8_t *lsdu = &pkt->buf[lsdu_offset];

	/* Padding */
	pos += snprintf(buf + pos, max_len - pos, " > ");

	if (addr_type == KNX_ADDR_TYPE_INDIVIDUAL) {
		pos += snprintf(buf + pos, max_len - pos, "[%u.%u.%u -> %i.%i.%i]",
				KNX_ADDR_VAL(src), KNX_ADDR_VAL(dst));
	} else {
		pos += snprintf(buf + pos, max_len - pos, "[%u.%u.%u -> %i/%i/%i]",
				KNX_ADDR_VAL(src), KNX_GROUP_VAL(dst));
	}

	if (repeat) {
		pos += snprintf(buf + pos, max_len - pos, " REPEAT ");
	}
	pos += snprintf(buf + pos, max_len - pos, " prio=%s", KNX_PRIORITY_VAL(priority));
	pos += snprintf(buf + pos, max_len - pos, " hop=%i", hop_count);

	if (addr_type == KNX_ADDR_TYPE_INDIVIDUAL) {
		pos += snprintf(buf + pos, max_len - pos, " INDIV TCF=%X ", lsdu[0]);
		if (lsdu[0] & 0x80) {
			if (lsdu[0] & 0x40) {
				if (lsdu[0] & 1) {
					pos += snprintf(buf + pos, max_len - pos, "T_Nack ");
				} else {
					pos += snprintf(buf + pos, max_len - pos, "T_Ack ");
				}
			} else {
				if (lsdu[0] & 1) {
					pos += snprintf(buf + pos, max_len - pos, "T_Disconnect ");
				} else {
					pos += snprintf(buf + pos, max_len - pos, "T_Connect ");
				}
			}
		} else {
			if (lsdu[0] & 0x40) {
				pos += snprintf(buf + pos, max_len - pos, "T_DataConnected ");
			} else {
				pos += snprintf(buf + pos, max_len - pos, "T_DataIndividual ");
			}
		}
	} else {
		if (dst == 0) {
			pos += snprintf(buf + pos, max_len - pos, " BROADCAST ");
		} else {
			pos += snprintf(buf + pos, max_len - pos, " GROUP ");
			uint16_t apci = ((uint16_t)lsdu[0] << 8) | lsdu[1];

			pos += snprintf(buf + pos, max_len - pos, " APCI=0x%X ", apci);
			switch (apci) {
			case 0x000:
				pos += snprintf(buf + pos, max_len - pos,
						" GroupValue_Read %i/%i/%i", KNX_ADDR_VAL(dst));
				break;
			case 0x040:
				pos += snprintf(buf + pos, max_len - pos,
						" GroupValue_Response %i/%i/%i", KNX_ADDR_VAL(dst));
				pos += snprintf(buf + pos, max_len - pos, " [");
				for (int i = 2; i < lsdu_len; i++) {
					pos += snprintf(buf + pos, max_len - pos, "%02X ", lsdu[i]);
				}
				pos += snprintf(buf + pos, max_len - pos, "]");
				break;
			case 0x080:
				pos += snprintf(buf + pos, max_len - pos,
						" GroupValue_Write %i/%i/%i", KNX_ADDR_VAL(dst));
				pos += snprintf(buf + pos, max_len - pos, " [");
				for (int i = 2; i < lsdu_len; i++) {
					pos += snprintf(buf + pos, max_len - pos, "%02X ", lsdu[i]);
				}
				pos += snprintf(buf + pos, max_len - pos, "]");
				break;
			default:
				pos += snprintf(buf + pos, max_len - pos, " GroupValue_Unknown");
				break;
			}
		}
	}
#endif
}

void LOG_RX(struct knx_pkt *pkt)
{
	char buf[512];

	LOG_FRAME(pkt, buf, sizeof(buf));
	LOG_DBG("%s", buf);
}

void LOG_TX(struct knx_pkt *pkt)
{
	char buf[512];

	LOG_FRAME(pkt, buf, sizeof(buf));
	LOG_DBG("%s", buf);
}
#endif

/* -------- Physical Layer callbacks (called from driver / ISR) -------- */

/* Forward IA changes to the NCN5130 auto-ACK register (ANALYZE.MD §2.6). */
static void ia_changed_cb(uint16_t new_addr)
{
	U_SetAddress__req((uint8_t)(new_addr & 0xFFu), (uint8_t)(new_addr >> 8));
	LOG_DBG("NCN5130 IA updated to " KNX_ADDR_FMT, KNX_ADDR_VAL(new_addr));
}

void Ph_Reset__con(P_Status status)
{
	if (status == p_ok) {
		uint16_t addr = device_individual_address();

		/* Register the callback so future A_IndividualAddress_Write or
		 * ETS IA programming updates the NCN5130 auto-ACK address.
		 */
		device_individual_address_changed_set_cb(ia_changed_cb);

		U_SetAddress__req((uint8_t)(addr & 0xFFu), (uint8_t)(addr >> 8));
		l_reset();
	}
}

/*
 * Ph_Data__ind — called from UART ISR for each received byte and for EOF.
 *
 * Ind_start_of_Frame : reset accumulator, store first byte.
 * Ind_inner_Frame_char: append byte to accumulator.
 * Ind_end_of_Frame   : allocate knx_pkt, copy raw frame into pkt->buf,
 *                      push to l_rx_pkt_msgq, wake RX thread.
 */
void Ph_Data__ind(Ph_Data_Ind_Class p_class, uint8_t p_data)
{
	switch (p_class) {
	case Ind_start_of_Frame:
		rx_frame[0] = p_data;
		rx_frame_len = 1;
		rx_frame_corrupt = false;
		break;

	case Ind_inner_Frame_char:
		if (rx_frame_len < MAX_KNX_TELEGRAM_SIZE) {
			rx_frame[rx_frame_len++] = p_data;
		}
		break;

	/*
	 * Per-octet error reported by the driver: the 9th bit disagreed
	 * with even parity, so the NCN5130 flagged this octet (or the host
	 * link corrupted it).  Store it anyway so the EOF length
	 * bookkeeping stays consistent, and remember the frame is damaged
	 * so it is dropped rather than parsed.
	 */
	case Ind_parity_error:
		if (rx_frame_len < MAX_KNX_TELEGRAM_SIZE) {
			rx_frame[rx_frame_len++] = p_data;
		}
		rx_frame_corrupt = true;
		break;

	/*
	 * Mark the frame damaged WITHOUT storing an octet.  The driver uses
	 * this when the octet has already been accumulated by another
	 * indication — specifically a flagged CTRL octet, which is reported
	 * as Ind_start_of_Frame (to keep frame synchronisation) followed by
	 * Ind_bit_error (to condemn the frame).
	 */
	case Ind_bit_error:
		rx_frame_corrupt = true;
		break;

	case Ind_framing_error:
		/* The receiver was reset mid-frame — abandon what we have. */
		rx_frame_len = 0;
		rx_frame_corrupt = false;
		break;

	case Ind_end_of_Frame: {
		uint16_t len = rx_frame_len;
		bool corrupt = rx_frame_corrupt;

		rx_frame_len = 0;
		rx_frame_corrupt = false;

		if (len < 4) {
			break;
		}

		if (corrupt) {
			/*
			 * KNX spec 3/2/2 §2.4.2: a frame that is not correct
			 * must not be passed to the user.  The NCN5130's
			 * auto-acknowledge has already NAKed it in hardware.
			 */
			LOG_WRN("RX: %u-byte frame had flagged octet(s) — discarding", len);
			break;
		}

		struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

		if (pkt == NULL) {
			LOG_ERR("RX: no pkt buffer");
			break;
		}

		memcpy(pkt->buf, (const void *)rx_frame, len);

		pkt->buf_len = len;

#if defined(CONFIG_KNX_DEBUG_VERBOSE_L1) || defined(CONFIG_KNX_DEBUG_VERBOSE_SYSTEM)
#if !defined(CONFIG_KNX_DEBUG_VERBOSE_L1)
		if (((pkt->buf[0] & 0x0Cu) >> 2) == KNX_PRIORITY_SYSTEM) {
#endif
			LOG_RX(pkt);
#if !defined(CONFIG_KNX_DEBUG_VERBOSE_L1)
		}
#endif
#endif
		l_frame_rx(pkt);

		break;
	}

	default:
		break;
	}
}

/*
 * Ph_Data__txdebug — dump an outgoing frame when tracing is enabled.
 *
 * The conditional compilation is deliberately kept INSIDE the function body:
 * an earlier version let the `#if` span the closing brace, so building with
 * every KNX_DEBUG_VERBOSE_* option off (or CONFIG_LOG=n) removed the `}` and
 * broke the build. Only application/prj.conf setting
 * CONFIG_KNX_DEBUG_VERBOSE_SYSTEM=y hid the problem.
 */
void Ph_Data__txdebug(struct knx_pkt *pkt)
{
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L1) || defined(CONFIG_KNX_DEBUG_VERBOSE_SYSTEM)
#if !defined(CONFIG_KNX_DEBUG_VERBOSE_L1)
	/* System-priority frames only unless L1 tracing is explicitly on. */
	if (((pkt->buf[0] & 0x0Cu) >> 2) == KNX_PRIORITY_SYSTEM) {
		LOG_TX(pkt);
	}
#else
	LOG_TX(pkt);
#endif
#else
	ARG_UNUSED(pkt);
#endif
}
