/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdmmc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "sd_utils.h"
#include "sd_init.h"


LOG_MODULE_REGISTER(sd, CONFIG_SD_LOG_LEVEL);

/* Idle all cards on bus. Can be used to clear errors on cards */
static inline int sd_idle(struct sd_card *card)
{
	struct sdhc_command cmd;

	/* Reset card with CMD0 */
	cmd.opcode = SD_GO_IDLE_STATE;
	cmd.arg = 0x0;
	cmd.response_type = (SD_RSP_TYPE_NONE | SD_SPI_RSP_TYPE_R1);
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	return sdhc_request(card->sdhc, &cmd, NULL);
}

/* Sends CMD8 during SD initialization */
static int sd_send_interface_condition(struct sd_card *card)
{
	struct sdhc_command cmd;
	int ret;
	uint32_t resp;

	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = SD_IF_COND_VHS_3V3 | SD_IF_COND_CHECK;
	cmd.response_type = (SD_RSP_TYPE_R7 | SD_SPI_RSP_TYPE_R7);
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("SD CMD8 failed with error %d", ret);
		/* Retry */
		return SD_RETRY;
	}
	if (card->host_props.is_spi) {
		resp = cmd.response[1];
	} else {
		resp = cmd.response[0];
	}
	if ((resp & 0xFF) != SD_IF_COND_CHECK) {
		LOG_INF("Legacy card detected, no CMD8 support");
		/* Retry probe */
		return SD_RETRY;
	}
	if ((resp & SD_IF_COND_VHS_MASK) != SD_IF_COND_VHS_3V3) {
		/* Card does not support 3.3V */
		return -ENOTSUP;
	}
	LOG_DBG("Found SDHC with CMD8 support");
	card->flags |= SD_SDHC_FLAG;
	return 0;
}

/* Sends CMD59 to enable CRC checking for SD card in SPI mode */
static int sd_enable_crc(struct sd_card *card)
{
	struct sdhc_command cmd;

	/* CMD59 for CRC mode is only valid for SPI hosts */
	__ASSERT_NO_MSG(card->host_props.is_spi);
	cmd.opcode = SD_SPI_CRC_ON_OFF;
	cmd.arg = 0x1; /* Enable CRC */
	cmd.response_type = SD_SPI_RSP_TYPE_R1;
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	return sdhc_request(card->sdhc, &cmd, NULL);
}

/*
 * Perform init required for both SD and SDIO cards.
 * This function performs the following portions of SD initialization
 * - CMD0 (SD reset)
 * - CMD8 (SD voltage check)
 */
static int sd_common_init(struct sd_card *card)
{
	int ret;

	/* Reset card with CMD0 */
	ret = sd_idle(card);
	if (ret) {
		LOG_ERR("Card error on CMD0");
		return ret;
	}
	/* Perform voltage check using SD CMD8 */
	ret = sd_retry(sd_send_interface_condition, card, CONFIG_SD_RETRY_COUNT);
	if (ret == -ETIMEDOUT) {
		LOG_INF("Card does not support CMD8, assuming legacy card");
		return sd_idle(card);
	} else if (ret) {
		LOG_ERR("Card error on CMD 8");
		return ret;
	}
	if (card->host_props.is_spi &&
		IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		/* Enable CRC for spi commands using CMD59 */
		ret = sd_enable_crc(card);
	}
	return ret;
}

static int sd_init_io(struct sd_card *card)
{
	struct sdhc_io *bus_io = &card->bus_io;
	struct sdhc_host_props *host_props = &card->host_props;
	int ret, voltage;

	/* SD clock should start gated */
	bus_io->clock = 0;
	/* SPI requires SDHC PUSH PULL, and open drain buses use more power */
	bus_io->bus_mode = SDHC_BUSMODE_PUSHPULL;
	bus_io->power_mode = SDHC_POWER_ON;
	bus_io->bus_width = SDHC_BUS_WIDTH1BIT;
	/* Cards start with legacy timing and Maximum voltage Host controller support */
	bus_io->timing = SDHC_TIMING_LEGACY;

	if (host_props->host_caps.vol_330_support) {
		LOG_DBG("Host controller support 3.3V max");
		voltage = SD_VOL_3_3_V;
	} else if (host_props->host_caps.vol_300_support) {
		LOG_DBG("Host controller support 3.0V max");
		voltage = SD_VOL_3_0_V;
	} else {
		LOG_DBG("Host controller support 1.8V max");
		voltage = SD_VOL_1_8_V;
	}

	/* Set to maximum voltage support by Host controller */
	bus_io->signal_voltage = voltage;

	/* Toggle power to card to reset it */
	LOG_DBG("Resetting power to card");
	bus_io->power_mode = SDHC_POWER_OFF;
	ret = sdhc_set_io(card->sdhc, bus_io);
	if (ret) {
		LOG_ERR("Could not disable card power via SDHC");
		return ret;
	}
	sd_delay(card->host_props.power_delay);
	bus_io->power_mode = SDHC_POWER_ON;
	ret = sdhc_set_io(card->sdhc, bus_io);
	if (ret) {
		LOG_ERR("Could not disable card power via SDHC");
		return ret;
	}
	/* After reset or init, card voltage should be max HC support */
	card->card_voltage = voltage;
	/* Reset card flags */
	card->flags = 0U;
	/* Delay so card can power up */
	sd_delay(card->host_props.power_delay);
	/* Start bus clock */
	bus_io->clock = SDMMC_CLOCK_400KHZ;
	ret = sdhc_set_io(card->sdhc, bus_io);
	if (ret) {
		LOG_ERR("Could not start bus clock");
		return ret;
	}
	return 0;
}

/*
 * Performs init flow described in section 3.6 of SD specification.
 */
static int sd_command_init(struct sd_card *card)
{
	int ret;

	/*
	 * We must wait 74 clock cycles, per SD spec, to use card after power
	 * on. At 400000KHz, this is a  185us delay. Wait 1ms to be safe.
	 */
	sd_delay(1);


	/*
	 * Start card initialization and identification
	 * flow described in section 3.6 of SD specification
	 * Common to SDIO and SDMMC. Some eMMC chips break the
	 * specification and expect something like this too.
	 */
	ret = sd_common_init(card);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_SDIO_STACK
	/* Attempt to initialize SDIO card */
	if (!sdio_card_init(card)) {
		return 0;
	}
#endif /* CONFIG_SDIO_STACK */
#ifdef CONFIG_SDMMC_STACK
	/* Attempt to initialize SDMMC card */
	if (!sdmmc_card_init(card)) {
		return 0;
	}
#endif /* CONFIG_SDIO_STACK */
#ifdef CONFIG_MMC_STACK
	ret = sd_idle(card);
	if (ret) {
		LOG_ERR("Card error on CMD0");
		return ret;
	}
	if (!mmc_card_init(card)) {
		return 0;
	}
#endif /* CONFIG_MMC_STACK */
	/* Unknown card type */
	return -ENOTSUP;
}

/* Initializes SD/SDIO card */
int sd_init(const struct device *sdhc_dev, struct sd_card *card)
{
	int ret;

	if (!sdhc_dev) {
		return -ENODEV;
	}
	card->sdhc = sdhc_dev;
	ret = sdhc_get_host_props(card->sdhc, &card->host_props);
	if (ret) {
		LOG_ERR("SD host controller returned invalid properties");
		return ret;
	}

	/* init and lock card mutex */
	ret = k_mutex_init(&card->lock);
	if (ret) {
		LOG_DBG("Could not init card mutex");
		return ret;
	}
	ret = k_mutex_lock(&card->lock, K_MSEC(CONFIG_SD_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Timeout while trying to acquire card mutex");
		return ret;
	}

	/* Initialize SDHC IO with defaults */
	ret = sd_init_io(card);
	if (ret) {
		k_mutex_unlock(&card->lock);
		return ret;
	}

	/*
	 * SD protocol is stateful, so we must account for the possibility
	 * that the card is in a bad state. The return code SD_RESTART
	 * indicates that the initialization left the card in a bad state.
	 * In this case the subsystem takes the following steps:
	 * - set card status to error
	 * - re init host I/O (will also toggle power to the SD card)
	 * - retry initialization once more
	 * If initialization then fails, the sd_init routine will assume the
	 * card is inaccessible
	 */
	ret = sd_command_init(card);
	if (ret == SD_RESTART) {
		/* Reset I/O, and retry sd initialization once more */
		card->status = CARD_ERROR;
		/* Reset I/O to default */
		ret = sd_init_io(card);
		if (ret) {
			LOG_ERR("Failed to reset SDHC I/O");
			k_mutex_unlock(&card->lock);
			return ret;
		}
		ret = sd_command_init(card);
		if (ret) {
			LOG_ERR("Failed to init SD card after I/O reset");
			k_mutex_unlock(&card->lock);
			return ret;
		}
	} else if (ret != 0) {
		/* Initialization failed */
		k_mutex_unlock(&card->lock);
		card->status = CARD_ERROR;
		return ret;
	}
	/* Card initialization succeeded. */
	card->status = CARD_INITIALIZED;
	/* Unlock card mutex */
	ret = k_mutex_unlock(&card->lock);
	if (ret) {
		LOG_DBG("Could not unlock card mutex");
		return ret;
	}
	return ret;
}

/* Return true if card is present, false otherwise */
bool sd_is_card_present(const struct device *sdhc_dev)
{
	if (!sdhc_dev) {
		return false;
	}
	return sdhc_card_present(sdhc_dev) == 1;
}
