/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdmmc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/logging/log.h>

#include "sd_utils.h"

LOG_MODULE_DECLARE(sd, CONFIG_SD_LOG_LEVEL);

/*
 * Send SDIO OCR using CMD5
 */
int sdio_send_ocr(struct sd_card *card, uint32_t ocr)
{
	struct sdhc_command cmd = {0};
	int ret;
	int retries;

	cmd.opcode = SDIO_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.response_type = (SD_RSP_TYPE_R4 | SD_SPI_RSP_TYPE_R4);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	/* Send OCR5 to initialize card */
	for (retries = 0; retries < CONFIG_SD_OCR_RETRY_COUNT; retries++) {
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		if (ret) {
			if (ocr == 0) {
				/* Just probing card, likely not SDIO */
				return SD_NOT_SDIO;
			}
			return ret;
		}
		if (ocr == 0) {
			/* We are probing card, check number of IO functions */
			card->num_io = (cmd.response[0] & SDIO_OCR_IO_NUMBER)
				>> SDIO_OCR_IO_NUMBER_SHIFT;
			if ((card->num_io == 0) ||
				((cmd.response[0] & SDIO_IO_OCR_MASK) == 0)) {
				if (cmd.response[0] & SDIO_OCR_MEM_PRESENT_FLAG) {
					/* Card is not an SDIO card */
					return SD_NOT_SDIO;
				}
				/* Card is not a supported SD device */
				return -ENOTSUP;
			}
			/* Card has IO present, return zero to
			 * indicate SDIO card
			 */
			return 0;
		}
	}
}

/*
 * Initialize an SDIO card for use with subsystem
 */
int sdio_card_init(struct sd_card *card)
{
	int ret;

	/* Probe card with SDIO OCR CM5 */
	ret = sdio_send_ocr(card, 0);
	if (ret) {
		return ret;
	}
	/* Card responded to ACMD41, type is SDIO */
	card->type = CARD_SDIO;
	/* No support for SDIO */
	return -ENOTSUP;
}
