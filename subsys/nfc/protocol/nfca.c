/*
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#include "common/poller.h"
#include "protocol/iso14443a.h"

LOG_MODULE_REGISTER(nfc_nfca, CONFIG_NFC_LOG_LEVEL);

int z_nfca_transceive(struct nfc_poller *poller, const struct nfc_target *target, const uint8_t *tx,
		      uint16_t tx_len, uint8_t *rx, uint16_t *rx_len,
		      const struct z_nfca_xfer *xfer)
{
	uint8_t frame[32];
	uint16_t len = tx_len;
	struct nfc_property props[] = {
		{.type = NFC_PROP_HW_TX_CRC, .hw_tx_crc = xfer->tx_crc},
		{.type = NFC_PROP_HW_RX_CRC, .hw_rx_crc = xfer->rx_crc},
		{.type = NFC_PROP_TIMEOUT, .timeout_us = xfer->timeout_us},
	};
	bool sw_rx_crc;
	int ret;

	if ((uint32_t)tx_len + 2U > sizeof(frame)) {
		return -EINVAL;
	}

	if (z_nfc_poller_backend(poller) == Z_NFC_BACKEND_OFFLOAD) {
		/* The controller frames the command and handles the CRC itself. */
		return nfc_offload_exchange(poller->dev, target, tx, tx_len, rx, rx_len,
					    DIV_ROUND_UP(xfer->timeout_us, USEC_PER_MSEC));
	}

	memcpy(frame, tx, tx_len);

	(void)nfc_set_properties(poller->dev, props, ARRAY_SIZE(props));

	if (xfer->tx_crc && props[0].status == -ENOTSUP) {
		nfc_iso14443a_crc_append(frame, len);
		len += 2U;
	}
	sw_rx_crc = xfer->rx_crc && (props[1].status == -ENOTSUP);

	ret = nfc_initiator_transceive(poller->dev, frame, len, 8U, rx, rx_len);
	if (ret == 0 && sw_rx_crc) {
		if (*rx_len < 2U || nfc_iso14443a_crc(rx, *rx_len) != 0U) {
			ret = -EBADMSG;
		} else {
			*rx_len -= 2U;
		}
	}

	return ret;
}

static int nfca_request(struct nfc_poller *poller, uint8_t *atqa)
{
	struct nfc_property props[] = {
		{.type = NFC_PROP_MFC_CRYPTO, .mfc_crypto_on = false},
		{.type = NFC_PROP_HW_TX_CRC, .hw_tx_crc = false},
		{.type = NFC_PROP_HW_RX_CRC, .hw_rx_crc = false},
		{.type = NFC_PROP_TIMEOUT, .timeout_us = 85U + 60U},
	};
	uint8_t cmd = NFC_ISO14443A_CMD_SENS_REQ;
	uint16_t rx_len = 2U;
	int ret;

	ret = nfc_set_properties(poller->dev, props, ARRAY_SIZE(props));
	if (ret < 0) {
		return ret;
	}

	ret = nfc_initiator_transceive(poller->dev, &cmd, 1U, 7U, atqa, &rx_len);
	if (ret < 0) {
		return ret;
	}

	if (rx_len != 2U) {
		return -EPROTO;
	}

	return 0;
}

static int nfca_sdd_cmd(uint8_t level, uint8_t *cmd)
{
	switch (level) {
	case 0:
		*cmd = NFC_ISO14443A_CMD_SDD_SEL_CL1;
		return 0;
	case 1:
		*cmd = NFC_ISO14443A_CMD_SDD_SEL_CL2;
		return 0;
	case 2:
		*cmd = NFC_ISO14443A_CMD_SDD_SEL_CL3;
		return 0;
	default:
		return -EINVAL;
	}
}

static int nfca_sdd(struct nfc_poller *poller, struct nfc_target_a *a, uint8_t cmd,
		    uint8_t *sdd_res)
{
	uint8_t tx_data[2] = {cmd, 0x20U};
	uint16_t rx_len = 5U;
	struct nfc_property hw_tx_crc = {.type = NFC_PROP_HW_TX_CRC, .hw_tx_crc = false};
	struct nfc_property hw_rx_crc = {.type = NFC_PROP_HW_RX_CRC, .hw_rx_crc = false};
	struct nfc_property timeout = {.type = NFC_PROP_TIMEOUT, .timeout_us = 85U + 60U};
	int ret;

	(void)nfc_set_properties(poller->dev, &timeout, 1U);
	(void)nfc_set_properties(poller->dev, &hw_tx_crc, 1U);
	(void)nfc_set_properties(poller->dev, &hw_rx_crc, 1U);

	ret = nfc_initiator_transceive(poller->dev, tx_data, sizeof(tx_data), 8U, sdd_res, &rx_len);
	if (ret < 0) {
		return ret;
	}

	if (rx_len != 5U || nfc_iso14443a_bcc(sdd_res, 5U) != 0U) {
		return -EBADMSG;
	}

	if (sdd_res[0] == NFC_ISO14443A_CASCADE_TAG) {
		memcpy(&a->uid[a->uid_len], &sdd_res[1], 3U);
		a->uid_len += 3U;
	} else {
		memcpy(&a->uid[a->uid_len], &sdd_res[0], 4U);
		a->uid_len += 4U;
	}

	return 0;
}

static int nfca_select(struct nfc_poller *poller, struct nfc_target_a *a, uint8_t cmd,
		       const uint8_t *sdd_res)
{
	uint8_t tx_data[9] = {cmd, 0x70U};
	uint16_t tx_len = 7U;
	uint8_t rx_data[5];
	uint16_t rx_len = sizeof(rx_data);
	struct nfc_property hw_tx_crc = {.type = NFC_PROP_HW_TX_CRC, .hw_tx_crc = true};
	struct nfc_property hw_rx_crc = {.type = NFC_PROP_HW_RX_CRC, .hw_rx_crc = true};
	int ret;

	memcpy(&tx_data[2], sdd_res, 5U);

	(void)nfc_set_properties(poller->dev, &hw_tx_crc, 1U);
	(void)nfc_set_properties(poller->dev, &hw_rx_crc, 1U);
	if (hw_tx_crc.status == -ENOTSUP) {
		nfc_iso14443a_crc_append(tx_data, tx_len);
		tx_len += 2U;
	}

	ret = nfc_initiator_transceive(poller->dev, tx_data, tx_len, 8U, rx_data, &rx_len);
	if (ret < 0) {
		return ret;
	}

	if (hw_rx_crc.status == -ENOTSUP) {
		if (rx_len != 3U || nfc_iso14443a_crc(rx_data, 3U) != 0U) {
			return -EBADMSG;
		}
	} else {
		if (rx_len != 1U) {
			return -EBADMSG;
		}
	}

	a->sak = rx_data[0];

	return 0;
}

static int nfca_cascade(struct nfc_poller *poller, struct nfc_target_a *a, uint8_t level)
{
	uint8_t sdd_res[5];
	uint8_t cmd;
	int ret;

	ret = nfca_sdd_cmd(level, &cmd);
	if (ret < 0) {
		return ret;
	}

	ret = nfca_sdd(poller, a, cmd, sdd_res);
	if (ret < 0) {
		return ret;
	}

	return nfca_select(poller, a, cmd, sdd_res);
}

static int nfca_discover_frontend(struct nfc_poller *poller, struct nfc_target *out)
{
	struct nfc_target_a *a = &out->a;
	int ret;

	ret = nfca_request(poller, a->atqa);
	if (ret < 0) {
		return ret;
	}

	for (uint8_t lvl = 0U; lvl < 3U; ++lvl) {
		ret = nfca_cascade(poller, a, lvl);
		if (ret < 0 || (a->sak & NFC_ISO14443A_SAK_CASCADE) == 0U) {
			break;
		}
	}

	return ret;
}

struct nfca_offload_ctx {
	struct k_sem sem;
	struct nfc_target hit;
	bool got;
};

static void nfca_offload_cb(const struct device *dev, const struct nfc_target *target,
			    void *user_data)
{
	struct nfca_offload_ctx *ctx = user_data;

	ARG_UNUSED(dev);

	if (!ctx->got) {
		ctx->hit = *target;
		ctx->got = true;
		k_sem_give(&ctx->sem);
	}
}

static int nfca_discover_offload(struct nfc_poller *poller, k_timeout_t timeout,
				 struct nfc_target *out)
{
	struct nfca_offload_ctx ctx = {.got = false};
	int ret;

	k_sem_init(&ctx.sem, 0, 1);

	ret = nfc_offload_poll_start(poller->dev, NFC_PROTO_ISO14443A, NULL, nfca_offload_cb, &ctx);
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_take(&ctx.sem, timeout);
	(void)nfc_offload_poll_stop(poller->dev);
	if (ret < 0) {
		return -EAGAIN;
	}

	*out = ctx.hit;
	return 0;
}

/* Silence or a garbled answer: another cycle may still find a target. */
static bool nfca_poll_again(int ret)
{
	return ret == -ETIMEDOUT || ret == -EBADMSG || ret == -EPROTO;
}

/*
 * NFC Forum Digital poll cycle. The field is removed briefly so that a target
 * which answered an earlier cycle, or was halted, powers down and can be found
 * again; it is then restored for the rest of the cycle, which a secure element
 * needs to finish starting up.
 */
static void nfca_field_cycle(struct nfc_poller *poller)
{
	struct nfc_property field = {.type = NFC_PROP_RF_FIELD};

	z_nfc_poller_lock(poller);
	field.rf_on = false;
	(void)nfc_set_properties(poller->dev, &field, 1U);
	z_nfc_poller_unlock(poller);

	k_sleep(K_MSEC(CONFIG_NFC_NFCA_FIELD_OFF_MS));

	z_nfc_poller_lock(poller);
	field.rf_on = true;
	(void)nfc_set_properties(poller->dev, &field, 1U);
	z_nfc_poller_unlock(poller);

	k_sleep(K_USEC(MAX(CONFIG_NFC_NFCA_POLL_INTERVAL_MS * USEC_PER_MSEC,
			   NFC_ISO14443A_GUARD_TIME_US)));
}

int z_nfca_discover(struct nfc_poller *poller, k_timeout_t timeout, struct nfc_target *out)
{
	enum z_nfc_backend_kind backend;
	k_timepoint_t deadline;
	int ret;

	backend = z_nfc_poller_backend(poller);
	deadline = sys_timepoint_calc(timeout);

	for (;;) {
		memset(out, 0, sizeof(*out));
		out->tech = NFC_TECH_A;
		out->proto = NFC_PROTO_ISO14443A;

		z_nfc_poller_lock(poller);

		switch (backend) {
		case Z_NFC_BACKEND_OFFLOAD:
			/* The firmware runs the poll cycles until the deadline. */
			ret = nfca_discover_offload(poller, timeout, out);
			break;
		case Z_NFC_BACKEND_FRONTEND_INITIATOR:
			ret = nfca_discover_frontend(poller, out);
			break;
		default:
			ret = -ENOTSUP;
			break;
		}

		z_nfc_poller_unlock(poller);

		if (backend != Z_NFC_BACKEND_FRONTEND_INITIATOR || !nfca_poll_again(ret)) {
			return ret;
		}
		if (sys_timepoint_expired(deadline)) {
			return -EAGAIN;
		}

		/* No target is selected between cycles, so the lock is dropped. */
		nfca_field_cycle(poller);
	}
}

int z_nfca_halt(struct nfc_poller *poller, const struct nfc_target *target, k_timeout_t timeout)
{
	uint8_t cmd[2] = {NFC_ISO14443A_CMD_HALT, 0U};
	uint8_t rx[3];
	uint16_t rx_len = 0U;
	int ret;

	const struct z_nfca_xfer xfer = {
		.tx_crc = true,
		.rx_crc = false,
		.timeout_us = z_nfc_timeout_us_cap(sys_timepoint_calc(timeout), 1100U + 60U),
	};

	ret = z_nfca_transceive(poller, target, cmd, sizeof(cmd), rx, &rx_len, &xfer);

	/* ISO/IEC 14443-3: a target that accepted HALT stays silent. */
	return (ret == -ETIMEDOUT) ? 0 : ret;
}
