/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sff/sff8472.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

/* Milli-degrees Celsius per temperature LSB numerator/denominator (1/256 C). */
#define SFF8472_TEMP_MDEGC_NUM 1000
#define SFF8472_TEMP_MDEGC_DEN 256

/* Engineering-unit scale factors applied to the raw DDM words. */
#define SFF8472_VCC_UV_PER_LSB     100U /* 100 uV/LSB */
#define SFF8472_TX_BIAS_UA_PER_LSB 2U   /* 2 uA/LSB */
#define SFF8472_POWER_NW_PER_LSB   100U /* 0.1 uW/LSB == 100 nW/LSB */

/* Copy a fixed-length, space-padded ASCII field and trim trailing blanks. */
static void sff8472_copy_ascii(char *dst, const uint8_t *src, size_t len)
{
	size_t trimmed = len;

	(void)memcpy(dst, src, len);
	dst[len] = '\0';

	while (trimmed > 0U) {
		char c = dst[trimmed - 1U];

		if ((c != ' ') && (c != '\0')) {
			break;
		}
		trimmed--;
	}

	dst[trimmed] = '\0';
}

/* Collapse one SFF-8472 status byte pair into the normalized flag bitmask. */
static uint16_t sff8472_decode_flags(uint8_t byte0, uint8_t byte1)
{
	uint16_t flags = 0U;

	if ((byte0 & BIT(7)) != 0U) {
		flags |= SFF8472_FLAG_TEMP_HIGH;
	}
	if ((byte0 & BIT(6)) != 0U) {
		flags |= SFF8472_FLAG_TEMP_LOW;
	}
	if ((byte0 & BIT(5)) != 0U) {
		flags |= SFF8472_FLAG_VCC_HIGH;
	}
	if ((byte0 & BIT(4)) != 0U) {
		flags |= SFF8472_FLAG_VCC_LOW;
	}
	if ((byte0 & BIT(3)) != 0U) {
		flags |= SFF8472_FLAG_TX_BIAS_HIGH;
	}
	if ((byte0 & BIT(2)) != 0U) {
		flags |= SFF8472_FLAG_TX_BIAS_LOW;
	}
	if ((byte0 & BIT(1)) != 0U) {
		flags |= SFF8472_FLAG_TX_POWER_HIGH;
	}
	if ((byte0 & BIT(0)) != 0U) {
		flags |= SFF8472_FLAG_TX_POWER_LOW;
	}
	if ((byte1 & BIT(7)) != 0U) {
		flags |= SFF8472_FLAG_RX_POWER_HIGH;
	}
	if ((byte1 & BIT(6)) != 0U) {
		flags |= SFF8472_FLAG_RX_POWER_LOW;
	}

	return flags;
}

int sff8472_parse_id(const uint8_t *page_a0, struct sff8472_id *id)
{
	uint8_t diag_mon;

	if ((page_a0 == NULL) || (id == NULL)) {
		return -EINVAL;
	}

	(void)memset(id, 0, sizeof(*id));

	id->identifier = page_a0[SFF8472_A0_IDENTIFIER];
	id->connector = page_a0[SFF8472_A0_CONNECTOR];
	id->br_nominal_mbd = (uint16_t)page_a0[SFF8472_A0_BR_NOMINAL] * 100U;
	id->wavelength_nm = sys_get_be16(&page_a0[SFF8472_A0_WAVELENGTH]);

	(void)memcpy(id->vendor_oui, &page_a0[SFF8472_A0_VENDOR_OUI], SFF8472_VENDOR_OUI_LEN);

	sff8472_copy_ascii(id->vendor_name, &page_a0[SFF8472_A0_VENDOR_NAME],
			   SFF8472_VENDOR_NAME_LEN);
	sff8472_copy_ascii(id->vendor_pn, &page_a0[SFF8472_A0_VENDOR_PN], SFF8472_VENDOR_PN_LEN);
	sff8472_copy_ascii(id->vendor_rev, &page_a0[SFF8472_A0_VENDOR_REV], SFF8472_VENDOR_REV_LEN);
	sff8472_copy_ascii(id->vendor_sn, &page_a0[SFF8472_A0_VENDOR_SN], SFF8472_VENDOR_SN_LEN);
	sff8472_copy_ascii(id->date_code, &page_a0[SFF8472_A0_DATE_CODE], SFF8472_DATE_CODE_LEN);

	diag_mon = page_a0[SFF8472_A0_DIAG_MON_TYPE];
	id->ddm_supported = (diag_mon & SFF8472_DIAG_MON_DDM_IMPLEMENTED) != 0U;
	id->ddm_internally_calibrated = (diag_mon & SFF8472_DIAG_MON_INT_CAL) != 0U;
	id->ddm_externally_calibrated = (diag_mon & SFF8472_DIAG_MON_EXT_CAL) != 0U;

	return 0;
}

int sff8472_parse_diagnostics(const uint8_t *page_a2, struct sff8472_diagnostics *diag)
{
	int16_t temp_raw;

	if ((page_a2 == NULL) || (diag == NULL)) {
		return -EINVAL;
	}

	temp_raw = (int16_t)sys_get_be16(&page_a2[SFF8472_A2_TEMP]);
	diag->temp_mdegc =
		((int32_t)temp_raw * SFF8472_TEMP_MDEGC_NUM) / SFF8472_TEMP_MDEGC_DEN;

	diag->vcc_uv = (uint32_t)sys_get_be16(&page_a2[SFF8472_A2_VCC]) * SFF8472_VCC_UV_PER_LSB;
	diag->tx_bias_ua =
		(uint32_t)sys_get_be16(&page_a2[SFF8472_A2_TX_BIAS]) * SFF8472_TX_BIAS_UA_PER_LSB;
	diag->tx_power_nw =
		(uint32_t)sys_get_be16(&page_a2[SFF8472_A2_TX_POWER]) * SFF8472_POWER_NW_PER_LSB;
	diag->rx_power_nw =
		(uint32_t)sys_get_be16(&page_a2[SFF8472_A2_RX_POWER]) * SFF8472_POWER_NW_PER_LSB;

	diag->alarm_flags = sff8472_decode_flags(page_a2[SFF8472_A2_ALARM_FLAGS],
						 page_a2[SFF8472_A2_ALARM_FLAGS + 1U]);
	diag->warning_flags = sff8472_decode_flags(page_a2[SFF8472_A2_WARNING_FLAGS],
						   page_a2[SFF8472_A2_WARNING_FLAGS + 1U]);

	return 0;
}

bool sff8472_base_checksum_valid(const uint8_t *page_a0)
{
	uint8_t sum = 0U;

	if (page_a0 == NULL) {
		return false;
	}

	for (size_t i = 0U; i < SFF8472_A0_CC_BASE; i++) {
		sum += page_a0[i];
	}

	return sum == page_a0[SFF8472_A0_CC_BASE];
}

bool sff8472_ext_checksum_valid(const uint8_t *page_a0)
{
	uint8_t sum = 0U;

	if (page_a0 == NULL) {
		return false;
	}

	for (size_t i = 64U; i < SFF8472_A0_CC_EXT; i++) {
		sum += page_a0[i];
	}

	return sum == page_a0[SFF8472_A0_CC_EXT];
}

const char *sff8472_identifier_name(uint8_t identifier)
{
	switch (identifier) {
	case SFF8472_ID_UNKNOWN:
		return "Unknown";
	case SFF8472_ID_GBIC:
		return "GBIC";
	case SFF8472_ID_SFF:
		return "Module soldered to motherboard";
	case SFF8472_ID_SFP:
		return "SFP/SFP+/SFP28";
	default:
		return "Reserved or vendor specific";
	}
}

const char *sff8472_connector_name(uint8_t connector)
{
	switch (connector) {
	case 0x00U:
		return "Unknown";
	case 0x01U:
		return "SC";
	case 0x07U:
		return "LC";
	case 0x0BU:
		return "Optical pigtail";
	case 0x0CU:
		return "MPO 1x12";
	case 0x0DU:
		return "MPO 2x16";
	case 0x21U:
		return "Copper pigtail";
	case 0x22U:
		return "RJ45";
	case 0x23U:
		return "No separable connector";
	default:
		return "Reserved or vendor specific";
	}
}
