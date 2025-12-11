/*
 * Copyright (c) 2022-2024 Libre Solar Technologies GmbH
 * Copyright (c) 2022-2024 tado GmbH
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
#ifdef CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_SEMTECH
#include <FragDecoder.h>
#elif defined(CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_LOWMEM)
#include "frag_decoder_lowmem.h"
#endif

#include <zephyr/lorawan/lorawan.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(lorawan_frag_transport, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

/**
 * Version of LoRaWAN Fragmented Data Block Transport Specification
 *
 * This implementation only supports TS004-1.0.0.
 */
#define FRAG_TRANSPORT_VERSION 1

/**
 * Maximum expected number of frag transport commands per packet
 *
 * The standard states "A message MAY carry more than one command". Even though this was not
 * observed during testing, space for up to 3 packages is reserved.
 */
#define FRAG_TRANSPORT_MAX_CMDS_PER_PACKAGE 3

/* maximum length of frag_transport answers */
#define FRAG_TRANSPORT_MAX_ANS_LEN 5

enum frag_transport_commands {
	FRAG_TRANSPORT_CMD_PKG_VERSION = 0x00,
	FRAG_TRANSPORT_CMD_FRAG_STATUS = 0x01,
	FRAG_TRANSPORT_CMD_FRAG_SESSION_SETUP = 0x02,
	FRAG_TRANSPORT_CMD_FRAG_SESSION_DELETE = 0x03,
	FRAG_TRANSPORT_CMD_DATA_FRAGMENT = 0x08,
};

struct frag_transport_context {
	/** Stores if a session is active */
	bool is_active;
	union {
		uint8_t frag_session;
		struct {
			/** Multicast groups allowed to input to this frag session */
			uint8_t mc_group_bit_mask: 4;
			/** Identifies this session */
			uint8_t frag_index: 2;
		};
	};
	/** Number of fragments of the data block for this session, max. 2^14-1 */
	uint16_t nb_frag;
	/** Number of fragments received in this session (including coded, uncoded and repeated) */
	uint16_t nb_frag_received;
	/** Size of each fragment in octets */
	uint8_t frag_size;
	union {
		uint8_t control;
		struct {
			/** Random delay for some responses between 0 and 2^(BlockAckDelay + 4) */
			uint8_t block_ack_delay: 3;
			/** Used fragmentation algorithm (0 for forward error correction) */
			uint8_t frag_algo: 3;
		};
	};
	/** Padding in the last fragment if total size is not a multiple of frag_size */
	uint8_t padding;
	/** Application-specific descriptor for the data block, e.g. firmware version */
	uint32_t descriptor;

#ifdef CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_SEMTECH
	/* variables required for FragDecoder.h */
	FragDecoderCallbacks_t decoder_callbacks;
#elif defined(CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_LOWMEM)
	struct frag_decoder decoder;
#endif
};

/*
 * The frag decoder is a singleton, so we can only have one ongoing frag session at a time, even
 * though the standard allows up to 4 sessions
 */
static struct frag_transport_context ctx;

/* Callback for notification of finished firmware transfer */
static void (*finished_cb)(void);

/* Callback to handle descriptor field */
static transport_descriptor_cb descriptor_cb;

static void frag_transport_package_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr,
					    uint8_t len, const uint8_t *rx_buf)
{
	uint8_t tx_buf[FRAG_TRANSPORT_MAX_CMDS_PER_PACKAGE * FRAG_TRANSPORT_MAX_ANS_LEN];
	uint8_t tx_pos = 0;
	uint8_t rx_pos = 0;
	int ans_delay = 0;
	int decoder_process_status;

	__ASSERT(port == LORAWAN_PORT_FRAG_TRANSPORT, "Wrong port %d", port);

	while (rx_pos < len) {
		uint8_t command_id = rx_buf[rx_pos++];

		if (sizeof(tx_buf) - tx_pos < FRAG_TRANSPORT_MAX_ANS_LEN) {
			LOG_ERR("insufficient tx_buf size, some requests discarded");
			break;
		}

		switch (command_id) {
		case FRAG_TRANSPORT_CMD_PKG_VERSION:
			tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_PKG_VERSION;
			tx_buf[tx_pos++] = LORAWAN_PACKAGE_ID_FRAG_TRANSPORT_BLOCK;
			tx_buf[tx_pos++] = FRAG_TRANSPORT_VERSION;
			break;
		case FRAG_TRANSPORT_CMD_FRAG_STATUS: {
			uint8_t frag_status = rx_buf[rx_pos++] & 0x07;
			uint8_t participants = frag_status & 0x01;
			uint8_t index = frag_status >> 1;

			LOG_DBG("FragSessionStatusReq index %d, participants: %u", index,
				participants);

			uint8_t missing_frag = CLAMP(ctx.nb_frag - ctx.nb_frag_received, 0, 255);

			uint8_t memory_error = 0;
#ifdef CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_SEMTECH
			FragDecoderStatus_t decoder_status = FragDecoderGetStatus();
			memory_error = decoder_status.MatrixError;
#endif

			if (participants == 1 || missing_frag > 0) {
				tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_FRAG_STATUS;
				tx_buf[tx_pos++] = ctx.nb_frag_received & 0xFF;
				tx_buf[tx_pos++] =
					(index << 6) | ((ctx.nb_frag_received >> 8) & 0x3F);
				tx_buf[tx_pos++] = missing_frag;
				tx_buf[tx_pos++] = memory_error & 0x01;

				ans_delay = sys_rand32_get() % (1U << (ctx.block_ack_delay + 4));

				LOG_DBG("FragSessionStatusAns index %d, NbFragReceived: %u, "
					"MissingFrag: %u, MemoryError: %u, delay: %d",
					index, ctx.nb_frag_received, missing_frag, memory_error,
					ans_delay);
			}
			break;
		}
		case FRAG_TRANSPORT_CMD_FRAG_SESSION_SETUP: {
			uint8_t frag_session = rx_buf[rx_pos++] & 0x3F;
			uint8_t index = frag_session >> 4;
			uint8_t status = index << 6;

			if (!ctx.is_active || ctx.frag_index == index) {
				ctx.frag_session = frag_session;
				ctx.nb_frag_received = 0;

				ctx.nb_frag = sys_get_le16(rx_buf + rx_pos);
				rx_pos += sizeof(uint16_t);

				ctx.frag_size = rx_buf[rx_pos++];
				ctx.control = rx_buf[rx_pos++];
				ctx.padding = rx_buf[rx_pos++];

				ctx.descriptor = sys_get_le32(rx_buf + rx_pos);
				rx_pos += sizeof(uint32_t);

				LOG_INF("FragSessionSetupReq index %d, nb_frag: %u, frag_size: %u, "
					"padding: %u, control: 0x%x, descriptor: 0x%.8x",
					index, ctx.nb_frag, ctx.frag_size, ctx.padding, ctx.control,
					ctx.descriptor);
			} else {
				/* FragIndex unsupported */
				status |= BIT(2);

				LOG_WRN("FragSessionSetupReq failed. Session %u still active",
					ctx.frag_index);
			}

			if (ctx.frag_algo > 0) {
				/* FragAlgo unsupported */
				status |= BIT(0);
			}

			if (ctx.nb_frag > FRAG_MAX_NB || ctx.frag_size > FRAG_MAX_SIZE) {
				/* Not enough memory */
				status |= BIT(1);
			}

#ifdef CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_SEMTECH
			if (ctx.nb_frag * ctx.frag_size > FragDecoderGetMaxFileSize()) {
				/* Not enough memory */
				status |= BIT(1);
			}
#endif

			if (descriptor_cb != NULL) {
				int rc = descriptor_cb(ctx.descriptor);

				if (rc < 0) {
					/* Wrong Descriptor */
					status |= BIT(3);
				}
			}

			if ((status & 0x1F) == 0) {
#ifdef CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_SEMTECH
				/*
				 * Assign callbacks after initialization to prevent the FragDecoder
				 * from writing byte-wise 0xFF to the entire flash. Instead, erase
				 * flash properly with own implementation.
				 */
				ctx.decoder_callbacks.FragDecoderWrite = NULL;
				ctx.decoder_callbacks.FragDecoderRead = NULL;

				FragDecoderInit(ctx.nb_frag, ctx.frag_size, &ctx.decoder_callbacks);

				ctx.decoder_callbacks.FragDecoderWrite = frag_flash_write;
				ctx.decoder_callbacks.FragDecoderRead = frag_flash_read;
#elif defined(CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_LOWMEM)
				frag_dec_init(&ctx.decoder, ctx.nb_frag, ctx.frag_size);
#endif
				frag_flash_init(ctx.frag_size);
				ctx.is_active = true;
			}

			tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_FRAG_SESSION_SETUP;
			tx_buf[tx_pos++] = status;
			break;
		}
		case FRAG_TRANSPORT_CMD_FRAG_SESSION_DELETE: {
			uint8_t index = rx_buf[rx_pos++] & 0x03;
			uint8_t status = 0x00;

			status |= index;
			if (!ctx.is_active || ctx.frag_index != index) {
				/* Session does not exist */
				status |= BIT(2);
			} else {
				ctx.is_active = false;
			}

			tx_buf[tx_pos++] = FRAG_TRANSPORT_CMD_FRAG_SESSION_DELETE;
			tx_buf[tx_pos++] = status;
			break;
		}
		case FRAG_TRANSPORT_CMD_DATA_FRAGMENT: {
			ctx.nb_frag_received++;

			uint16_t frag_index_n = sys_get_le16(rx_buf + rx_pos);

			rx_pos += 2;

			uint16_t frag_counter = frag_index_n & 0x3FFF;
			uint8_t index = (frag_index_n >> 14) & 0x03;

			if (!ctx.is_active || index != ctx.frag_index) {
				LOG_DBG("DataFragment received for inactive session %u", index);
				break;
			}

			if (frag_counter > ctx.nb_frag) {
				/* Additional fragments have to be cached in RAM for recovery
				 * algorithm.
				 */
				frag_flash_use_cache();
			}

#ifdef CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_SEMTECH
			decoder_process_status = FragDecoderProcess(
				frag_counter, (uint8_t *)&rx_buf[rx_pos]);
#elif defined(CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_LOWMEM)
			decoder_process_status = frag_dec(
				&ctx.decoder, frag_counter, &rx_buf[rx_pos], ctx.frag_size);
#endif

			LOG_INF("DataFragment %u of %u (%u lost), session: %u, decoder result: %d",
				frag_counter, ctx.nb_frag, frag_counter - ctx.nb_frag_received,
				index, decoder_process_status);

			if (decoder_process_status >= 0) {
				/* Positive status corresponds to number of lost (but recovered)
				 * fragments. Value >= 0 means the transport is done.
				 */
				frag_flash_finish();

				LOG_INF("Frag Transport finished successfully");

				if (finished_cb != NULL) {
					finished_cb();
				}

				/* avoid processing further fragments */
				ctx.is_active = false;
			}

			rx_pos += ctx.frag_size;
			break;
		}
		default:
			return;
		}
	}

	if (tx_pos > 0) {
		lorawan_services_schedule_uplink(LORAWAN_PORT_FRAG_TRANSPORT, tx_buf, tx_pos,
						 ans_delay);
	}
}

void lorawan_frag_transport_register_descriptor_callback(transport_descriptor_cb cb)
{
	descriptor_cb = cb;
}

static struct lorawan_downlink_cb downlink_cb = {
	.port = (uint8_t)LORAWAN_PORT_FRAG_TRANSPORT,
	.cb = frag_transport_package_callback,
};

int lorawan_frag_transport_run(void (*transport_finished_cb)(void))
{
	finished_cb = transport_finished_cb;

	lorawan_register_downlink_callback(&downlink_cb);

	return 0;
}
