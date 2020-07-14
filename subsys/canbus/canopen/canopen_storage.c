/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <settings/settings.h>

#include <CANopen.h>
#include <CO_Emergency.h>
#include <CO_SDO.h>

#include <canbus/canopen.h>

#define LOG_LEVEL CONFIG_CANOPEN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(canopen_storage);

/* 's', 'a', 'v', 'e' from LSB to MSB */
#define STORE_PARAM_MAGIC   0x65766173UL

/* 'l', 'o', 'a', 'd' from LSB to MSB */
#define RESTORE_PARAM_MAGIC 0x64616F6CUL

/* Variables for reporing errors through CANopen once the stack is up */
static int canopen_storage_rom_error;
static int canopen_storage_eeprom_error;

static CO_SDO_abortCode_t canopen_odf_1010(CO_ODF_arg_t *odf_arg)
{
	CO_EM_t *em = odf_arg->object;
	uint32_t value;
	int err;

	value = CO_getUint32(odf_arg->data);

	if (odf_arg->reading) {
		return CO_SDO_AB_NONE;
	}

	/* Preserve old value */
	memcpy(odf_arg->data, odf_arg->ODdataStorage, sizeof(uint32_t));

	if (odf_arg->subIndex != 1U) {
		return CO_SDO_AB_NONE;
	}

	if (value != STORE_PARAM_MAGIC) {
		return CO_SDO_AB_DATA_TRANSF;
	}

	err = canopen_storage_save(CANOPEN_STORAGE_ROM);
	if (err) {
		LOG_ERR("failed to save object dictionary ROM entries (err %d)",
			err);
		CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE,
			       err);
		return CO_SDO_AB_HW;
	} else {
		LOG_DBG("saved object dictionary ROM entries");
	}

	return CO_SDO_AB_NONE;
}

static CO_SDO_abortCode_t canopen_odf_1011(CO_ODF_arg_t *odf_arg)
{
	CO_EM_t *em = odf_arg->object;
	bool failed = false;
	uint32_t value;
	int err;

	value = CO_getUint32(odf_arg->data);

	if (odf_arg->reading) {
		return CO_SDO_AB_NONE;
	}

	/* Preserve old value */
	memcpy(odf_arg->data, odf_arg->ODdataStorage, sizeof(uint32_t));

	if (odf_arg->subIndex < 1U) {
		return CO_SDO_AB_NONE;
	}

	if (value != RESTORE_PARAM_MAGIC) {
		return CO_SDO_AB_DATA_TRANSF;
	}

	err = canopen_storage_erase(CANOPEN_STORAGE_ROM);
	if (err == -ENOENT) {
		LOG_DBG("no object dictionary ROM entries to delete");
	} else if (err) {
		LOG_ERR("failed to delete object dictionary ROM entries"
			" (err %d)", err);
		CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE,
			       err);
		failed = true;
	} else {
		LOG_DBG("deleted object dictionary ROM entries");
	}

#ifdef CONFIG_CANOPEN_STORAGE_HANDLER_ERASES_EEPROM
	err = canopen_storage_erase(CANOPEN_STORAGE_EEPROM);
	if (err == -ENOENT) {
		LOG_DBG("no object dictionary EEPROM entries to delete");
	} else if (err) {
		LOG_ERR("failed to delete object dictionary EEPROM entries"
			" (err %d)", err);
		CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE,
			       err);
		failed = true;
	} else {
		LOG_DBG("deleted object dictionary EEPROM entries");
	}
#endif

	if (failed) {
		return CO_SDO_AB_HW;
	}

	return CO_SDO_AB_NONE;
}

static int canopen_settings_set(const char *key, size_t len_rd,
				settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int nlen;
	ssize_t len;

	nlen = settings_name_next(key, &next);

	if (!strncmp(key, "eeprom", nlen)) {
		struct sCO_OD_EEPROM eeprom;

		len = read_cb(cb_arg, &eeprom, sizeof(eeprom));
		if (len < 0) {
			LOG_ERR("failed to restore object dictionary EEPROM"
				" entries (err %d)", len);
			canopen_storage_eeprom_error = len;
		} else {
			if ((eeprom.FirstWord == CO_OD_FIRST_LAST_WORD) &&
			    (eeprom.LastWord == CO_OD_FIRST_LAST_WORD)) {
				memcpy(&CO_OD_EEPROM, &eeprom,
				       sizeof(CO_OD_EEPROM));
				LOG_DBG("restored object dictionary EEPROM"
					" entries");
			} else {
				LOG_WRN("object dictionary EEPROM entries"
					" signature mismatch, skipping"
					" restore");
			}
		}

		return 0;
	} else if (!strncmp(key, "rom", nlen)) {
		struct sCO_OD_ROM rom;

		len = read_cb(cb_arg, &rom, sizeof(rom));
		if (len < 0) {
			LOG_ERR("failed to restore object dictionary ROM"
				" entries (err %d)", len);
			canopen_storage_rom_error = len;
		} else {
			if ((rom.FirstWord == CO_OD_FIRST_LAST_WORD) &&
			    (rom.LastWord == CO_OD_FIRST_LAST_WORD)) {
				memcpy(&CO_OD_ROM, &rom, sizeof(CO_OD_ROM));
				LOG_DBG("restored object dictionary ROM"
					" entries");
			} else {
				LOG_WRN("object dictionary ROM entries"
					" signature mismatch, skipping"
					" restore");
			}
		}

		return 0;
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(canopen, "canopen", NULL,
			       canopen_settings_set, NULL, NULL);

void canopen_storage_attach(CO_SDO_t *sdo, CO_EM_t *em)
{
	CO_OD_configure(sdo, OD_H1010_STORE_PARAM_FUNC, canopen_odf_1010,
			em, 0U, 0U);
	CO_OD_configure(sdo, OD_H1011_REST_PARAM_FUNC, canopen_odf_1011,
			em, 0U, 0U);

	if (canopen_storage_eeprom_error) {
		CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE,
			       canopen_storage_eeprom_error);
	}

	if (canopen_storage_rom_error) {
		CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE,
			       canopen_storage_rom_error);
	}
}

int canopen_storage_save(enum canopen_storage storage)
{
	int ret = 0;

	if (storage == CANOPEN_STORAGE_ROM) {
		ret = settings_save_one("canopen/rom", &CO_OD_ROM,
					sizeof(CO_OD_ROM));
	} else if (storage == CANOPEN_STORAGE_EEPROM) {
		ret = settings_save_one("canopen/eeprom", &CO_OD_EEPROM,
					sizeof(CO_OD_EEPROM));
	}

	return ret;
}

int canopen_storage_erase(enum canopen_storage storage)
{
	int ret = 0;

	if (storage == CANOPEN_STORAGE_ROM) {
		ret = settings_delete("canopen/rom");
	} else if (storage == CANOPEN_STORAGE_EEPROM) {
		ret = settings_delete("canopen/eeprom");
	}

	return ret;
}
