/*
 * Copyright (c) 2021 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_EEPROM_H_
#define __SETTINGS_EEPROM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * In the EEPROM backend, a settings entry is stored as:
 * a. record length (uint16_t, including the crc16)
 * b. record id maximum index (id length - 1): uint8_t,
 * c. id,
 * d. data,
 * f. crc16 calculated over the id + data
 *
 * For now the id is equal to the name, but this might change if a more compact
 * storage is developed.
 */

#define EEPROM_SETTINGS_VERSION 1
#define EEPROM_SETTINGS_MAGIC 0x45455053 /* EEPS in hex */

/* Record info */
struct settings_eeprom_rec_info {
	/* id length */
	size_t idlen;
	/* start of data */
	off_t dataoffset;
	/* data length */
	size_t datalen;
};

/* Header stored at beginning of EEPROM to identify that settings are stored
 * in the EEPROM
 */
struct settings_eeprom_hdr {
	uint32_t magic;
	uint32_t ver;
} __packed;

struct settings_eeprom {
	struct settings_store cf_store;
	off_t start; /* start address in eeprom */
	size_t size; /* size of used eeprom area */
	off_t end; /* end of the in use eeprom area */
	const struct device *eeprom;
};

/* register EEPROM to be a source of settings */
int settings_eeprom_src(struct settings_eeprom *cf);

/* register EEPROM to be the destination of settings */
int settings_eeprom_dst(struct settings_eeprom *cf);

/* Initialize a EEPROM backend. */
int settings_eeprom_backend_init(struct settings_eeprom *cf);


#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_EEPROM_H_ */
