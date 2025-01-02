/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "factory_data_parser.h"

#include <zephyr/kernel.h>
#include <ctype.h>
#include <string.h>

#define MAX_FACTORY_DATA_NESTING_LEVEL 3

static inline bool uint16_decode(zcbor_state_t *states, uint16_t *value)
{
	uint32_t u32;

	if (zcbor_uint32_decode(states, &u32)) {
		*value = (uint16_t)u32;
		return true;
	}

	return false;
}

bool parse_factory_data(const uint8_t *buffer, uint16_t bufferSize, struct FactoryData *factoryData)
{
	memset(factoryData, 0, sizeof(*factoryData));
	ZCBOR_STATE_D(states, MAX_FACTORY_DATA_NESTING_LEVEL, buffer, bufferSize, 1, NULL);

	bool res = zcbor_map_start_decode(states);
	struct zcbor_string currentString;

	while (res) {
		res = zcbor_tstr_decode(states, &currentString);

		if (!res) {
			res = true;
			break;
		}

		if (currentString.len == strlen("version") &&
		    memcmp("version", (const char *)currentString.value, currentString.len) == 0) {
			res = res && uint16_decode(states, &factoryData->version);
		} else if (strncmp("hw_ver", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && uint16_decode(states, &factoryData->hw_ver);
			factoryData->hwVerPresent = res;
		} else if (strncmp("spake2_it", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && zcbor_uint32_decode(states, &factoryData->spake2_it);
		} else if (strncmp("vendor_id", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && uint16_decode(states, &factoryData->vendor_id);
			factoryData->vendorIdPresent = res;
		} else if (strncmp("product_id", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && uint16_decode(states, &factoryData->product_id);
			factoryData->productIdPresent = res;
		} else if (strncmp("discriminator", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && uint16_decode(states, &factoryData->discriminator);
			factoryData->discriminatorPresent = res;
		} else if (strncmp("passcode", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && zcbor_uint32_decode(states, &factoryData->passcode);
		} else if (strncmp("sn", (const char *)currentString.value, currentString.len) ==
			   0) {
			res = res &&
			      zcbor_bstr_decode(states, (struct zcbor_string *)&factoryData->sn);
		} else if (strncmp("date", (const char *)currentString.value, currentString.len) ==
			   0) {
			/*
			 * Date format is YYYY-MM-DD, so format needs to be validated and string
			 * parse to integer parts.
			 */
			struct zcbor_string date;

			res = res && zcbor_bstr_decode(states, &date);
			if (date.len == 10 && isdigit(date.value[0]) && isdigit(date.value[1]) &&
			    isdigit(date.value[2]) && isdigit(date.value[3]) &&
			    date.value[4] == '-' && isdigit(date.value[5]) &&
			    isdigit(date.value[6]) && date.value[7] == '-' &&
			    isdigit(date.value[8]) && isdigit(date.value[9])) {
				factoryData->date_year =
					1000 * (date.value[0] - '0') + 100 * (date.value[1] - '0') +
					10 * (date.value[2] - '0') + date.value[3] - '0';
				factoryData->date_month =
					10 * (date.value[5] - '0') + date.value[6] - '0';
				factoryData->date_day =
					10 * (date.value[8] - '0') + date.value[9] - '0';
			} else {
				printk("[E] Parsing error - wrong date format\n");
			}
		} else if (strncmp("hw_ver_str", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->hw_ver_str);
		} else if (strncmp("rd_uid", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && zcbor_bstr_decode(states,
						       (struct zcbor_string *)&factoryData->rd_uid);
		} else if (strncmp("dac_cert", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && zcbor_bstr_decode(
					     states, (struct zcbor_string *)&factoryData->dac_cert);
		} else if (strncmp("dac_key", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->dac_priv_key);
		} else if (strncmp("pai_cert", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res && zcbor_bstr_decode(
					     states, (struct zcbor_string *)&factoryData->pai_cert);
		} else if (strncmp("spake2_salt", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->spake2_salt);
		} else if (strncmp("spake2_verifier", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(
				      states, (struct zcbor_string *)&factoryData->spake2_verifier);
		} else if (strncmp("vendor_name", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->vendor_name);
		} else if (strncmp("product_name", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->product_name);
		} else if (strncmp("part_number", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->part_number);
		} else if (strncmp("product_url", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->product_url);
		} else if (strncmp("product_label", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->product_label);
		} else if (strncmp("enable_key", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(states,
						(struct zcbor_string *)&factoryData->enable_key);
		} else if (strncmp("user", (const char *)currentString.value, currentString.len) ==
			   0) {
			res = res &&
			      zcbor_bstr_decode(states, (struct zcbor_string *)&factoryData->user);
		} else if (strncmp("cert_dclrn", (const char *)currentString.value,
				   currentString.len) == 0) {
			res = res &&
			      zcbor_bstr_decode(
				      states,
				      (struct zcbor_string *)&factoryData->certificate_declaration);
		} else {
			res = res && zcbor_any_skip(states, NULL);
		}
	}

	return res && zcbor_list_map_end_force_decode(states);
}

static void print_binary_data(const char *log_message, const void *data, size_t len)
{
	if (data == NULL || len == 0) {
		printk("%sNot available\n", log_message);
		return;
	}

	const uint8_t *bytes = (const uint8_t *)data;

	if (log_message) {
		printk("%s: ", log_message);
	}

	for (size_t i = 0; i < len; i++) {
		printk("%02X", bytes[i]);
	}

	printk("\n");
}

void print_factory_data(const struct FactoryData *factoryData)
{
	printk("\nFactory Data:\n");
	printk("Version: %u\n", factoryData->version);

	if (factoryData->sn.value && factoryData->sn.len > 0) {
		printk("Serial Number: %.*s\n", (int)factoryData->sn.len,
		       (char *)factoryData->sn.value);
	} else {
		printk("Serial Number: Not available\n");
	}

	printk("Date: %04u-%02u-%02u\n", factoryData->date_year, factoryData->date_month,
	       factoryData->date_day);

	if (factoryData->vendorIdPresent) {
		printk("Vendor ID: %u\n", factoryData->vendor_id);
	} else {
		printk("Vendor ID: Not available\n");
	}

	if (factoryData->productIdPresent) {
		printk("Product ID: %u\n", factoryData->product_id);
	} else {
		printk("Product ID: Not available\n");
	}

	if (factoryData->vendor_name.value && factoryData->vendor_name.len > 0) {
		printk("Vendor Name: %.*s\n", (int)factoryData->vendor_name.len,
		       (char *)factoryData->vendor_name.value);
	} else {
		printk("Vendor Name: Not available\n");
	}

	if (factoryData->product_name.value && factoryData->product_name.len > 0) {
		printk("Product Name: %.*s\n", (int)factoryData->product_name.len,
		       (char *)factoryData->product_name.value);
	} else {
		printk("Product Name: Not available\n");
	}

	if (factoryData->part_number.value && factoryData->part_number.len > 0) {
		printk("Part Number: %.*s\n", (int)factoryData->part_number.len,
		       (char *)factoryData->part_number.value);
	} else {
		printk("Part Number: Not available\n");
	}

	if (factoryData->product_url.value && factoryData->product_url.len > 0) {
		printk("Product URL: %.*s\n", (int)factoryData->product_url.len,
		       (char *)factoryData->product_url.value);
	} else {
		printk("Product URL: Not available\n");
	}

	if (factoryData->hwVerPresent) {
		printk("Hardware Version: %u\n", factoryData->hw_ver);
	} else {
		printk("Hardware Version: Not available\n");
	}

	if (factoryData->hw_ver_str.value && factoryData->hw_ver_str.len > 0) {
		printk("Hardware Version String: %.*s\n", (int)factoryData->hw_ver_str.len,
		       (char *)factoryData->hw_ver_str.value);
	} else {
		printk("Hardware Version String: Not available\n");
	}

	print_binary_data("RD UID (Hex): ", factoryData->rd_uid.value, factoryData->rd_uid.len);

	print_binary_data("DAC Certificate (Hex): ", factoryData->dac_cert.value,
				factoryData->dac_cert.len);

	print_binary_data("DAC Private Key (Hex): ", factoryData->dac_priv_key.value,
				factoryData->dac_priv_key.len);

	print_binary_data("Spake2 Salt (Hex): ", factoryData->spake2_salt.value,
				factoryData->spake2_salt.len);

	print_binary_data("Spake2 Verifier (Hex): ", factoryData->spake2_verifier.value,
				factoryData->spake2_verifier.len);

	printk("Spake2 Iterations: %u\n", factoryData->spake2_it);

	print_binary_data("Enable Key (Hex): ", factoryData->enable_key.value,
				factoryData->enable_key.len);

	if (factoryData->user.value && factoryData->user.len > 0) {
		printk("User: %.*s\n", (int)factoryData->user.len, (char *)factoryData->user.value);
	} else {
		printk("User: Not available\n");
	}

	print_binary_data("Certificate Declaration (Hex): ",
				factoryData->certificate_declaration.value,
				factoryData->certificate_declaration.len);

	if (factoryData->discriminatorPresent) {
		printk("Discriminator: %u\n", factoryData->discriminator);
	} else {
		printk("Discriminator: Not available\n");
	}

	printk("Passcode: %u\n", factoryData->passcode);
}
