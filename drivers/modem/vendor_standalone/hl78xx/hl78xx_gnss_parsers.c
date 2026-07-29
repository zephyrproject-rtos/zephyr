/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hl78xx_gnss_parsers.c
 *
 * @brief Supplementary NMEA0183 parsers for HL78xx GNSS driver
 *
 * This file contains parsers for supplementary NMEA0183 sentences that are not
 * part of the standard Zephyr GNSS subsystem. These include:
 * - GSA: GNSS DOP and Active Satellites
 * - GST: GNSS Pseudorange Error Statistics
 * - EPU: Sierra proprietary EPU (Estimated Position Error)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hl78xx_gnss_parsers.h"
#include "hl78xx_gnss.h"
#include "hl78xx_gnssloc.h"
#include "../../../gnss/gnss_parse.h"
#include "hl78xx_gnss_aux_parse.h"

LOG_MODULE_DECLARE(hl78xx_gnss);

enum hl78xx_gnss_aux_sentence {
	HL78XX_GNSS_AUX_SENTENCE_GSA,
	HL78XX_GNSS_AUX_SENTENCE_GST,
	HL78XX_GNSS_AUX_SENTENCE_EPU,
};

static K_SEM_DEFINE(semlock, 1, 1);

/* -------------------------------------------------------------------------
 * NMEA Sentence Parser Callbacks
 * -------------------------------------------------------------------------
 */

#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER

static bool hl78xx_gnss_aux_should_publish(enum hl78xx_gnss_aux_sentence sentence)
{
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_PEPU)) {
		return sentence == HL78XX_GNSS_AUX_SENTENCE_EPU;
	}

	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GST)) {
		return sentence == HL78XX_GNSS_AUX_SENTENCE_GST;
	}

	return sentence == HL78XX_GNSS_AUX_SENTENCE_GSA;
}

void gnss_nmea0183_match_gsa_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;
	int ret;

	ARG_UNUSED(chat);

	if (data == NULL) {
		return;
	}
	/* Parse fix type (1=no fix, 2=2D, 3=3D) */
	ret = hl78xx_gnss_parse_gsa(argv, argc, &data->aux_data);
	if (ret < 0) {
		LOG_WRN("GSA: failed to parse GSA data");
		return;
	}

	/* Note: DOP values could be stored in user_data if extended
	 * gnss_data structure is needed
	 */
	if (hl78xx_gnss_aux_should_publish(HL78XX_GNSS_AUX_SENTENCE_GSA)) {
		gnss_publish_aux_data((const struct device *)data->dev, &data->aux_data,
				      sizeof(data->aux_data));
	}
}

void gnss_nmea0183_match_gst_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;
	int ret;

	ARG_UNUSED(chat);

	if (data == NULL) {
		return;
	}

	ret = hl78xx_gnss_parse_gst(argv, argc, &data->aux_data);
	if (ret < 0) {
		return;
	}

	if (hl78xx_gnss_aux_should_publish(HL78XX_GNSS_AUX_SENTENCE_GST)) {
		gnss_publish_aux_data((const struct device *)data->dev, &data->aux_data,
				      sizeof(data->aux_data));
	}
}


void gnss_nmea0183_match_epu_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;
	int ret;

	ARG_UNUSED(chat);

	if (data == NULL) {
		return;
	}

	ret = hl78xx_gnss_parse_epu(argv, argc, &data->aux_data);
	if (ret < 0) {
		return;
	}

	if (hl78xx_gnss_aux_should_publish(HL78XX_GNSS_AUX_SENTENCE_EPU)) {
		gnss_publish_aux_data((const struct device *)data->dev, &data->aux_data,
				      sizeof(data->aux_data));
	}
}

void gnss_publish_aux_data(const struct device *dev,
			   const struct hl78xx_gnss_nmea_aux_data *aux_data, uint16_t size)
{
	(void)k_sem_take(&semlock, K_FOREVER);

	STRUCT_SECTION_FOREACH(hl78xx_gnss_aux_data_callback, callback) {
		if (callback->dev == NULL || callback->dev == dev) {
			callback->callback(dev, aux_data, size);
		}
	}

	k_sem_give(&semlock);
}
#endif /* CONFIG_HL78XX_GNSS_AUX_DATA_PARSER */
