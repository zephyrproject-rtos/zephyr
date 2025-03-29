/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>

#include <smtc_modem_hal.h>
#include <zephyr/lorawan_lbm/lorawan_hal_init.h>

LOG_MODULE_DECLARE(lorawan_hal, CONFIG_LORA_BASICS_MODEM_LOG_LEVEL);

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL
static struct lorawan_user_storage_cb *user_storage_cb;
#endif

#ifdef CONFIG_LORA_BASICS_MODEM_PROVIDED_STORAGE_IMPL

/* NOTE: That whole storage implementation uses direct flash access on the storage_partition
 * from Zephyr instead of leaving it for NVS.
 * Users are expected to provide `chosen/lora-basics-modem-context-partition` to prevent that.
 *
 * A second, more generic implementation via user-provided callbacks was started, with the goal
 * to use the Zephyr NVS APIs, but the store-and-forward internal code from LBM prevents
 * a simple use of this API, it is thus not complete nor working.
 */

#if DT_HAS_CHOSEN(lora_basics_modem_context_partition)
#define CONTEXT_PARTITION DT_FIXED_PARTITION_ID(DT_CHOSEN(lora_basics_modem_context_partition))
#else
#define CONTEXT_PARTITION FIXED_PARTITION_ID(storage_partition)
#endif

const struct flash_area *context_flash_area;

/* in case of multistack the size of the lorawan context shall be extended */
#define ADDR_LORAWAN_CONTEXT_OFFSET           0
#define ADDR_MODEM_KEY_CONTEXT_OFFSET         256
#define ADDR_MODEM_CONTEXT_OFFSET             512
#define ADDR_SECURE_ELEMENT_CONTEXT_OFFSET    768
#define ADDR_CRASHLOG_CONTEXT_OFFSET          4096
#define ADDR_STORE_AND_FORWARD_CONTEXT_OFFSET 8192

static void flash_init(void)
{
	if (context_flash_area) {
		return;
	}
	int err = flash_area_open(CONTEXT_PARTITION, &context_flash_area);

	if (err != 0) {
		LOG_ERR("Could not open flash area for context (%d)", err);
	}
	LOG_INF("Opened flash area of size %d", context_flash_area->fa_size);
}

static uint32_t priv_hal_context_address(const modem_context_type_t ctx_type, uint32_t offset)
{
	switch (ctx_type) {
	case CONTEXT_MODEM:
		return ADDR_MODEM_CONTEXT_OFFSET + offset;
	case CONTEXT_KEY_MODEM:
		return ADDR_MODEM_KEY_CONTEXT_OFFSET + offset;
	case CONTEXT_LORAWAN_STACK:
		return ADDR_LORAWAN_CONTEXT_OFFSET + offset;
	case CONTEXT_FUOTA:
		/* no fuota example on stm32l0 */
		return 0;
	case CONTEXT_STORE_AND_FORWARD:
		return ADDR_STORE_AND_FORWARD_CONTEXT_OFFSET + offset;
	case CONTEXT_SECURE_ELEMENT:
		return ADDR_SECURE_ELEMENT_CONTEXT_OFFSET + offset;
	}
	k_oops();
	CODE_UNREACHABLE;
}

void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset,
				    uint8_t *buffer, const uint32_t size)
{
	const uint32_t real_offset = priv_hal_context_address(ctx_type, offset);

	flash_init();
	flash_area_read(context_flash_area, real_offset, buffer, size);
}

/* NOTE: We take here the maximum erase size out there to do read-erase-write */
#define PAGE_BUFFER_SIZE 4096

static uint8_t page_buffer[PAGE_BUFFER_SIZE];

/* NOTE: We assume that stores are only on one sector.
 */
void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset,
				  const uint8_t *buffer, const uint32_t size)
{
	const uint32_t real_offset = priv_hal_context_address(ctx_type, offset);

	flash_init();

	/* read-erase-write */
	if (real_offset < ADDR_STORE_AND_FORWARD_CONTEXT_OFFSET) {
		memset(page_buffer, 0, PAGE_BUFFER_SIZE);
		flash_area_read(context_flash_area, 0, page_buffer, PAGE_BUFFER_SIZE);
		memset(page_buffer + real_offset, 0, size);
		memcpy(page_buffer + real_offset, buffer, size);
		flash_area_erase(context_flash_area, 0, PAGE_BUFFER_SIZE);
		flash_area_write(context_flash_area, 0, page_buffer, PAGE_BUFFER_SIZE);
	} else {
		/* Workaround because some 4-bytes writes will come while flash supports only 8 */
		const uint32_t real_size = size + 8 - (size % 8);

		memset(page_buffer, 0, real_size);
		/* yes, size, not real_size */
		memcpy(page_buffer, buffer, size);
		flash_area_write(context_flash_area, real_offset, page_buffer, real_size);
	}
}

/* NOTE: We assume that erases are aligned on sectors */
void smtc_modem_hal_context_flash_pages_erase(const modem_context_type_t ctx_type, uint32_t offset,
					      uint8_t nb_page)
{
	const uint32_t real_offset = priv_hal_context_address(ctx_type, offset);

	flash_init();
	flash_area_erase(context_flash_area, real_offset,
			      smtc_modem_hal_flash_get_page_size() * nb_page);
}

uint16_t smtc_modem_hal_flash_get_page_size(void)
{
	const struct device *flash_device;
	struct flash_pages_info info;

	flash_init();
	flash_device = flash_area_get_device(context_flash_area);
	flash_get_page_info_by_offs(flash_device, ADDR_STORE_AND_FORWARD_CONTEXT_OFFSET, &info);
	return info.size;
}

uint16_t smtc_modem_hal_store_and_forward_get_number_of_pages(void)
{
	uint16_t page_size;
	size_t flash_size;
	uint16_t pages_possible;

	page_size = smtc_modem_hal_flash_get_page_size();
	flash_size = context_flash_area->fa_size;

	/* 8192B are taken by contexts before store_and_forward */
	pages_possible = (flash_size - ADDR_STORE_AND_FORWARD_CONTEXT_OFFSET) / page_size;

	return pages_possible;
}

void smtc_modem_hal_crashlog_store(const uint8_t *crashlog, uint8_t crash_string_length)
{
	flash_init();

	memset(page_buffer, 0, PAGE_BUFFER_SIZE);
	page_buffer[0] = 1;
	page_buffer[1] = crash_string_length;
	memcpy(page_buffer + 2, crashlog, crash_string_length);

	flash_area_erase(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, PAGE_BUFFER_SIZE);
	flash_area_write(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, page_buffer, PAGE_BUFFER_SIZE);
}

void smtc_modem_hal_crashlog_restore(uint8_t *crashlog, uint8_t *crash_string_length)
{
	flash_init();
	flash_area_read(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, page_buffer, PAGE_BUFFER_SIZE);
	int available = page_buffer[0];
	int length = page_buffer[1];

	crashlog[0] = 0;
	*crash_string_length = length;

	if (available != 0) {
		memcpy(crashlog, page_buffer + 2, length);
	}
}

static int crashlog_status = -1;

void smtc_modem_hal_crashlog_set_status(bool available)
{
	flash_init();
	if (!available) {
		flash_area_erase(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET,
				 smtc_modem_hal_flash_get_page_size());
	}
	crashlog_status = available ? 1 : 0;
}

bool smtc_modem_hal_crashlog_get_status(void)
{
	if (crashlog_status == -1) {
		flash_init();
		flash_area_read(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, page_buffer, 1);
		/* Any other state might mean uninitialized flash area */
		bool available = (page_buffer[0] == 1);

		crashlog_status = available ? 1 : 0;
	}

	/* Any other state might mean uninitialized flash area */
	return crashlog_status == 1;
}

#endif /* CONFIG_LORA_BASICS_MODEM_PROVIDED_STORAGE_IMPL */

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL

/* As said in the comment at the beginning of the file, this implementation is neither
 * finished nor working. This is because the store-and-forward implementation expects
 * raw flash accesses, and "CONTEXT_*" have variable / unsure sizes.
 */

void lorawan_register_user_storage_callbacks(struct lorawan_user_storage_cb *cb)
{
	user_storage_cb = cb;
}

/*
 * CONTEXT_MODEM -> size = 16
 * CONTEXT_KEY_MODEM -> size = 36 bytes
 * CONTEXT_LORAWAN_STACK -> size = 32
 * CONTEXT_FUOTA -> variable size
 * CONTEXT_SECURE_ELEMENT -> size = 24 or 483 (!!)
 * CONTEXT_STORE_AND_FORWARD -> garbage
 */

void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset,
				    uint8_t *buffer, const uint32_t size)
{
	user_storage_cb->context_restore(ctx_type, offset, buffer, size);
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset,
				  const uint8_t *buffer, const uint32_t size)
{
	user_storage_cb->context_store(ctx_type, offset, buffer, size);
}

#define CRASH_LOG_ID        0xFE
#define CRASH_LOG_STATUS_ID (CRASH_LOG_ID + 1)

void smtc_modem_hal_crashlog_store(const uint8_t *crashlog, uint8_t crash_string_length)
{
	/* We use 0xFF as the ID so we do not overwrite any of the valid contexts */
	user_storage_cb->context_store(CRASH_LOG_ID, 0, crashlog, crash_string_length);
}

void smtc_modem_hal_crashlog_restore(uint8_t *crashlog, uint8_t *crash_string_length)
{
	user_storage_cb->context_restore(CRASH_LOG_ID, 0, crashlog, *crash_string_length);
}

void smtc_modem_hal_crashlog_set_status(bool available)
{
	user_storage_cb->context_store(CRASH_LOG_STATUS_ID, 0, (uint8_t *)&available,
				       sizeof(available));
}

bool smtc_modem_hal_crashlog_get_status(void)
{
	bool available;

	user_storage_cb->context_restore(CRASH_LOG_STATUS_ID, 0, (uint8_t *)&available,
					 sizeof(available));
	return available;
}

/* NOTE: only used in store-and-forward */
void smtc_modem_hal_context_flash_pages_erase(const modem_context_type_t ctx_type, uint32_t offset,
					      uint8_t nb_page)
{
}

uint16_t smtc_modem_hal_flash_get_page_size(void)
{
	return 0;
}

uint16_t smtc_modem_hal_store_and_forward_get_number_of_pages(void)
{
	return 0;
}

#endif
