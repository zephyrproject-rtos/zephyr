/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>

#include <CANopen.h>
#include <storage/CO_storage.h>
#include <canopennode.h>

#define LOG_LEVEL CONFIG_CANOPEN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(canopen_storage);

/* Variables for reporting errors through CANopen once the stack is up */
static int canopen_storage_rom_error;
static int canopen_storage_eeprom_error;

static CO_storage_t storage;


static ODR_t storage_store(CO_storage_entry_t *entry,
			   CO_CANmodule_t *canmodule)
{
	int ret;

	switch (entry->type) {
	default:
		break;
	case CANOPEN_STORAGE_ROM:
		ret = settings_save_one("canopen/rom",
					entry->addr, entry->len);
		if (ret != 0) {
			canopen_storage_rom_error = true;
			return ODR_HW;
		}
		canopen_storage_rom_error = false;
		break;
	case CANOPEN_STORAGE_EEPROM:
		ret = settings_save_one("canopen/eeprom",
					entry->addr, entry->len);
		if (ret != 0) {
			canopen_storage_eeprom_error = true;
			return ODR_HW;
		}
		canopen_storage_eeprom_error = false;
		break;
	}

	return ODR_OK;
}

static ODR_t storage_restore(CO_storage_entry_t *entry,
			     CO_CANmodule_t *canmodule)
{
	int ret;

	switch (entry->type) {
	default:
		break;
	case CANOPEN_STORAGE_ROM:
		ret = settings_delete("canopen/rom");
		if (ret != 0) {
			canopen_storage_rom_error = true;
			return ODR_HW;
		}
		canopen_storage_rom_error = false;
		break;
	case CANOPEN_STORAGE_EEPROM:
		ret = settings_delete("canopen/eeprom");
		if (ret != 0) {
			canopen_storage_eeprom_error = true;
			return ODR_HW;
		}
		canopen_storage_eeprom_error = false;
		break;
	}

	return ODR_OK;
}

void canopen_storage_attach(struct canopen_context *ctx,
			    OD_entry_t *OD_1010_entry, /* store */
			    OD_entry_t *OD_1011_entry, /* restore */
			    CO_storage_entry_t *storage_entries,
			    OD_size_t storage_entries_count)
{
	CO_ReturnError_t err;
	CO_EM_t *em;

	em = ctx->co->em;

	err = CO_storage_init(&storage, ctx->co->CANmodule,
			      OD_1010_entry, OD_1011_entry,
			      storage_store, storage_restore,
			      storage_entries, storage_entries_count);
	if (err != CO_ERROR_NO) {
		LOG_ERR("CO_storage_init failed (err=%d)", err);
		return;
	}

	if (canopen_storage_eeprom_error) {
		CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE,
			       canopen_storage_eeprom_error);
	}

	if (canopen_storage_rom_error) {
		CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE,
			       canopen_storage_rom_error);
	}

	return;
}
