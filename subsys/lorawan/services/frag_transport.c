/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022 tado GmbH
 *
 * Parts of this implementation were inspired by LmhpFragmentation.c from the
 * LoRaMac-node firmware repository https://github.com/Lora-net/LoRaMac-node
 * written by Miguel Luis (Semtech).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "frag_flash.h"
#include "lorawan_services.h"

#include <LoRaMac.h>
#include <FragDecoder.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/rand32.h>

LOG_MODULE_REGISTER(lorawan_frag_transport, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

/* maximum length of frag_transport answers */
#define MAX_FRAG_TRANSPORT_ANS_LEN 5

enum frag_transport_commands {
	FRAG_TRANSPORT_CMD_PKG_VERSION         = 0x00,
	FRAG_TRANSPORT_CMD_FRAG_STATUS         = 0x01,
	FRAG_TRANSPORT_CMD_FRAG_SESSION_SETUP  = 0x02,
	FRAG_TRANSPORT_CMD_FRAG_SESSION_DELETE = 0x03,
#if CONFIG_LORAWAN_FRAG_TRANSPORT_VERSION >= 2
	FRAG_TRANSPORT_CMD_BLOCK_RECEIVED      = 0x04,
#endif /* CONFIG_LORAWAN_FRAG_TRANSPORT_VERSION */
	FRAG_TRANSPORT_CMD_DATA_FRAGMENT       = 0x08,
};

struct frag_transport_context {
	/** Stores if this session is active */
	bool is_active;
	union {
		uint8_t frag_session;
		struct {
			/** Multicast groups allowed to input to this frag session */
			uint8_t mc_group_bit_mask: 4;
			/** Identifies this session (equal to array index) */
			uint8_t frag_index: 2;
		};
	};
	/** Number of fragments of the data block for this session, max. 2^14-1 */
	uint16_t nb_frag;
	/** Size of each fragment in octets */
	uint8_t frag_size;
	union {
		uint8_t control;
		struct {
			/** Random delay for some responses between 0 and 2^(BlockAckDelay + 4) */
			uint8_t block_ack_delay: 3;
			/** Used fragmentation algorithm (0 for forward error correction) */
			uint8_t frag_algo: 3;
#if CONFIG_LORAWAN_FRAG_TRANSPORT_VERSION >= 2
			/** Specifies if full block reception should be ACKed */
			uint8_t ack_reception : 1;
#endif /* CONFIG_LORAWAN_FRAG_TRANSPORT_VERSION */
		};
	};
	/** Padding in the last fragment if total size is not a multiple of frag_size */
	uint8_t padding;
	/** Application-specific descriptor for the data block, e.g. firmware version */
	uint32_t descriptor;

	/* variables required for FragDecoder.h */
	FragDecoderStatus_t decoder_status;
	FragDecoderCallbacks_t decoder_callbacks;
	int32_t decoder_process_status;
};

static struct frag_transport_context ctx[LORAMAC_MAX_MC_CTX];

static struct k_work_q *workq;

/* Callback for notification of finished firmware transfer */
static void (*finished_cb)(void);

static void frag_transport_package_callback(uint8_t port, bool data_pending, int16_t rssi,
					    int8_t snr, uint8_t len, const uint8_t *rx_buf)
{
	uint8_t tx_buf[3 * MAX_FRAG_TRANSPORT_ANS_LEN];
	uint8_t tx_pos = 0;
	uint8_t rx_pos = 0;
	int ans_delay = 0;

	__ASSERT(port == LORAWAN_PORT_FRAG_TRANSPORT, "Wrong port %d", port);

	while (rx_pos < len) {
		uint8_t command_id = rx_buf[rx_pos++];

		if (sizeof(tx_buf) - tx_pos < MAX_FRAG_TRANSPORT_ANS_LEN) {
			LOG_ERR("insufficient tx_buf size, some requests discarded");
			break;
		}

		switch (command_id) {
		case FRAG_TRANSPORT_CMD_PKG_VERSION:
			/* ToDo: Don't process in case of multicast session */

			tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_PKG_VERSION;
			tx_buf[tx_pos++] = LORAWAN_PACKAGE_ID_FRAG_TRANSPORT_BLOCK;
			tx_buf[tx_pos++] = CONFIG_LORAWAN_FRAG_TRANSPORT_VERSION;
			break;
		case FRAG_TRANSPORT_CMD_FRAG_STATUS: {
			uint8_t frag_status = rx_buf[rx_pos++] & 0x07;
			uint8_t participants = frag_status & 0x01;
			uint8_t index = frag_status >> 1;

			LOG_DBG("FragSessionStatusReq index %d, participants: %u",
				index, participants);

			ctx[index].decoder_status = FragDecoderGetStatus();

			if (participants == 1 || ctx[index].decoder_status.FragNbLost > 0) {

				uint8_t missing_frag = CLAMP(ctx[index].nb_frag -
					ctx[index].decoder_status.FragNbRx, 0, 255);

				tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_FRAG_STATUS;
				tx_buf[tx_pos++] = ctx[index].decoder_status.FragNbRx & 0xFF;
				tx_buf[tx_pos++] = (index << 6) |
					((ctx[index].decoder_status.FragNbRx >> 8) & 0x3F);
				tx_buf[tx_pos++] = missing_frag;
				tx_buf[tx_pos++] = ctx[index].decoder_status.MatrixError & 0x01;

				ans_delay = sys_rand32_get() %
					(1U << (ctx[index].block_ack_delay + 4));

				LOG_DBG("FragSessionStatusAns index %d, FragNbRx: %u, "
					"FragNbLost: %u, MissingFrag: %u, status: %u, delay: %d",
					index, ctx[index].decoder_status.FragNbRx,
					ctx[index].decoder_status.FragNbLost, missing_frag,
					ctx[index].decoder_status.MatrixError, ans_delay);
			}
			break;
		}
		case FRAG_TRANSPORT_CMD_FRAG_SESSION_SETUP: {
			/* ToDo: Don't process in case of multicast session */

			uint8_t frag_session = rx_buf[rx_pos++] & 0x3F;
			uint8_t index = frag_session >> 4;
			uint8_t status = index << 6;

			ctx[index].frag_session = frag_session;

			ctx[index].nb_frag = rx_buf[rx_pos++];
			ctx[index].nb_frag |= rx_buf[rx_pos++] << 8;

			ctx[index].frag_size = rx_buf[rx_pos++];
			ctx[index].control = rx_buf[rx_pos++];
			ctx[index].padding = rx_buf[rx_pos++];

			ctx[index].descriptor = rx_buf[rx_pos++];
			ctx[index].descriptor += rx_buf[rx_pos++] << 8;
			ctx[index].descriptor += rx_buf[rx_pos++] << 16;
			ctx[index].descriptor += rx_buf[rx_pos++] << 24;

			LOG_DBG("FragSessionSetupReq index %d, nb_frag: %u, frag_size: %u, "
				"padding: %u, control: 0x%x, descriptor: 0x%.8x", index,
				ctx[index].nb_frag, ctx[index].frag_size, ctx[index].padding,
				ctx[index].control, ctx[index].descriptor);

			/* ToDo: Add new Spec v2 features
			 * - SessionCnt to prevent replay attacks
			 * - MIC for integrity check and authentication
			 */

			if (ctx[index].frag_algo > 0) {
				/* FragAlgo unsupported */
				status |= 1U << 0;
			}

			if (ctx[index].nb_frag > FRAG_MAX_NB ||
					ctx[index].frag_size > FRAG_MAX_SIZE ||
					ctx[index].nb_frag * ctx[index].frag_size >
					FragDecoderGetMaxFileSize()) {
				/* Not enough memory */
				status |= 1U << 1;
			}

			if (ctx[index].frag_index >= ARRAY_SIZE(ctx)) {
				/* FragIndex unsupported */
				status |= 1U << 2;
			}

			/* Descriptor not used: Ignore Wrong Descriptor error */

			if ((status & 0x1F) == 0) {
				/*
				 * Assign callbacks after initialization to prevent the FragDecoder
				 * from writing byte-wise 0xFF to the entire flash. Instead, erase
				 * flash properly with own implementation.
				 */
				ctx[index].decoder_callbacks.FragDecoderWrite = NULL;
				ctx[index].decoder_callbacks.FragDecoderRead = NULL;

				FragDecoderInit(ctx[index].nb_frag, ctx[index].frag_size,
						&ctx[index].decoder_callbacks);

				ctx[index].decoder_callbacks.FragDecoderWrite = frag_flash_write;
				ctx[index].decoder_callbacks.FragDecoderRead = frag_flash_read;
				ctx[index].is_active = true;

				frag_flash_init();
				ctx[index].decoder_process_status = FRAG_SESSION_ONGOING;
			}

			tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_FRAG_SESSION_SETUP;
			tx_buf[tx_pos++] = status;
			break;
		}
		case FRAG_TRANSPORT_CMD_FRAG_SESSION_DELETE: {
			/* ToDo: Don't process in case of multicast session */

			uint8_t index = rx_buf[rx_pos++] & 0x03;
			uint8_t status = 0x00;

			status |= index;
			if (index >= ARRAY_SIZE(ctx) || !ctx[index].is_active) {
				/* Session does not exist */
				status |= 1U << 3;
			} else {
				ctx[index].is_active = false;
			}

			tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_FRAG_SESSION_DELETE;
			tx_buf[tx_pos++] = status;
			break;
		}
#if CONFIG_LORAWAN_FRAG_TRANSPORT_VERSION >= 2
		case FRAG_TRANSPORT_CMD_BLOCK_RECEIVED:
			LOG_ERR("FragDataBlockReceivedAns not implemented");
			return;
#endif /* CONFIG_LORAWAN_FRAG_TRANSPORT_VERSION */
		case FRAG_TRANSPORT_CMD_DATA_FRAGMENT: {
			uint16_t frag_index_n;

			frag_index_n = rx_buf[rx_pos++];
			frag_index_n |= rx_buf[rx_pos++] << 8;

			uint16_t frag_counter = frag_index_n & 0x3FFF;
			uint8_t index = (frag_index_n >> 14) & 0x03;

			LOG_DBG("DataFragment %u of %u (session index: %u)",
				frag_counter, ctx[index].nb_frag, index);

			if (ctx[index].decoder_process_status == FRAG_SESSION_ONGOING) {
				ctx[index].decoder_process_status =
					FragDecoderProcess(frag_counter,
							   (uint8_t *)&rx_buf[rx_pos]);
				ctx[index].decoder_status = FragDecoderGetStatus();

				LOG_DBG("FragDecoder process status: %d",
					ctx[index].decoder_process_status);
			} else if (ctx[index].decoder_process_status >= 0) {
				/* Positive status corresponds to number of lost (but recovered)
				 * fragments. Value >= 0 means the upgrade is done.
				 */
				frag_flash_finish();

				if (finished_cb != NULL) {
					finished_cb();
				}
			}

			rx_pos += ctx[index].frag_size;
			break;
		}
		default:
			return;
		}
	}

	if (tx_pos > 0) {
		lorawan_services_schedule_uplink(LORAWAN_PORT_FRAG_TRANSPORT, tx_buf, tx_pos,
						 K_SECONDS(ans_delay));
	}
}

static struct lorawan_downlink_cb downlink_cb = {
	.port = (uint8_t)LORAWAN_PORT_FRAG_TRANSPORT,
	.cb = frag_transport_package_callback
};

int lorawan_frag_transport_run(void (*transport_finished_cb)(void))
{
	workq = lorawan_services_get_work_queue();
	finished_cb = transport_finished_cb;

	/* initialize non-zero static variables */
	for (int i = 0; i < ARRAY_SIZE(ctx); i++) {
		ctx[i].decoder_process_status = FRAG_SESSION_NOT_STARTED;
	}

	lorawan_register_downlink_callback(&downlink_cb);

	return 0;
}
