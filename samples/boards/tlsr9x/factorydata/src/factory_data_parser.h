/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zcbor_decode.h>

struct FactoryData {
	uint16_t version;
	struct zcbor_string sn;
	uint16_t date_year;
	uint8_t date_month;
	uint8_t date_day;
	uint16_t vendor_id;
	uint16_t product_id;
	struct zcbor_string vendor_name;
	struct zcbor_string product_name;
	struct zcbor_string part_number;
	struct zcbor_string product_url;
	struct zcbor_string product_label;
	uint16_t hw_ver;
	struct zcbor_string hw_ver_str;
	struct zcbor_string rd_uid;
	struct zcbor_string dac_cert;
	struct zcbor_string dac_priv_key;
	struct zcbor_string pai_cert;
	uint32_t spake2_it;
	struct zcbor_string spake2_salt;
	struct zcbor_string spake2_verifier;
	uint16_t discriminator;
	uint32_t passcode;
	struct zcbor_string enable_key;
	struct zcbor_string user;
	struct zcbor_string certificate_declaration;

	bool vendorIdPresent;
	bool productIdPresent;
	bool hwVerPresent;
	bool discriminatorPresent;
};

/**
 * @brief Parses raw factory data into the factory data structure.
 *
 * This function decodes raw binary data stored in a buffer and populates
 * the fields of the provided FactoryData structure with the extracted information.
 *
 * @param[in] buffer Pointer to the buffer containing raw factory data.
 * @param[in] bufferSize Size of the factory data in the buffer, in bytes.
 * @param[out] factoryData Pointer to the FactoryData structure to be filled.
 *
 * @returns true if parsing is successful, false otherwise.
 */
bool parse_factory_data(const uint8_t *buffer, uint16_t bufferSize,
			struct FactoryData *factoryData);

/**
 * @brief Prints all fields of the FactoryData structure in a human-readable format.
 *
 * This function iterates over the fields of a FactoryData structure, formatting
 * and printing their values to standard output.
 *
 * @param[in] factoryData Pointer to the FactoryData structure to print.
 */
void print_factory_data(const struct FactoryData *factoryData);
