/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <smtc_modem_hal.h>
#include <smtc_modem_hal_init.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>

#include <lora_lbm_transceiver.h>

/* for variadic args */
#include <stdarg.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smtc_modem_hal, CONFIG_LORA_BASICS_MODEM_LOG_LEVEL);

/* ------------ Local context ------------ */

/* A binary semaphore to notify the main LBM loop */
K_SEM_DEFINE(prv_main_event_sem, 0, 1);

/* transceiver device pointer */
static const struct device *prv_transceiver_dev;

/* External callbacks */
static struct smtc_modem_hal_cb *prv_hal_cb;

/* context and callback for modem_hal_timer */
static void *prv_smtc_modem_hal_timer_context;
static void (*prv_smtc_modem_hal_timer_callback)(void *context);

/* flag for enabling/disabling timer interrupt. This is set by the libraray during "critical"
 * sections
 */
static bool prv_modem_irq_enabled = true;
static bool prv_modem_irq_pending_while_disabled;
static bool prv_radio_irq_pending_while_disabled;

/* The timer and work used for the modem_hal_timer */
static void prv_smtc_modem_hal_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(prv_smtc_modem_hal_timer, prv_smtc_modem_hal_timer_handler, NULL);

/* context and callback for the event pin interrupt */
static void *prv_smtc_modem_hal_radio_irq_context;
static void (*prv_smtc_modem_hal_radio_irq_callback)(void *context);

/* ------------ Initialization ------------
 *
 * This function is defined in smtc_modem_hal_init.h
 * and is used to set everything up in here.
 */

void smtc_modem_hal_init(const struct device *transceiver)
{
	__ASSERT(transceiver, "transceiver must be provided");
	prv_transceiver_dev = transceiver;
}

void smtc_modem_hal_register_callbacks(struct smtc_modem_hal_cb *hal_cb)
{
	__ASSERT_NO_MSG(hal_cb);
	__ASSERT_NO_MSG(hal_cb->get_battery_level);
	__ASSERT_NO_MSG(hal_cb->get_temperature);
	__ASSERT_NO_MSG(hal_cb->get_voltage);

#ifdef CONFIG_LORA_BASICS_MODEM_FUOTA
	__ASSERT_NO_MSG(hal_cb->get_hw_version_for_fuota);
	__ASSERT_NO_MSG(hal_cb->get_fw_version_for_fuota);
	__ASSERT_NO_MSG(hal_cb->get_fw_status_available_for_fuota);
	__ASSERT_NO_MSG(hal_cb->get_next_fw_version_for_fuota);
	__ASSERT_NO_MSG(hal_cb->get_fw_delete_status_for_fuota);
#endif

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL
	__ASSERT_NO_MSG(hal_cb->context_store);
	__ASSERT_NO_MSG(hal_cb->context_restore);
#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */

	prv_hal_cb = hal_cb;
}

/* ------------ Reset management ------------ */

void smtc_modem_hal_reset_mcu(void)
{
	LOG_WRN("Resetting the MCU");
	log_panic(); /* To flush the logs */
	sys_reboot(SYS_REBOOT_COLD);
}

/* ------------ Watchdog management ------------ */

void smtc_modem_hal_reload_wdog(void)
{
	/* This is not called anywhere except smtc_modem_test.c from the smtc modem stack, so I am
	 * unsure why this is required
	 */
}

/* ------------ Time management ------------ */

uint32_t smtc_modem_hal_get_time_in_s(void)
{
	int64_t time_ms = k_uptime_get();

	return (uint32_t)(time_ms / 1000);
}

uint32_t smtc_modem_hal_get_time_in_ms(void)
{
	/* The wrapping every 49 days is expected by the modem lib */
	return k_uptime_get_32();
}

void smtc_modem_hal_set_offset_to_test_wrapping(const uint32_t offset_to_test_wrapping)
{
	/* We're using RTOS which handles RTC wrapping. */
}

void smtc_modem_hal_interruptible_msleep(k_timeout_t timeout)
{
	/* Notify the main loop if it's sleeping */
	k_sem_take(&prv_main_event_sem, timeout);
}

void smtc_modem_hal_wake_up(void)
{
	k_sem_give(&prv_main_event_sem);
}

/* ------------ Timer management ------------ */

/**
 * @brief Called when the prv_smtc_modem_hal_timer expires.
 *
 * Submits the prv_smtc_modem_hal_timer_work to be handle the callback.
 */
static void prv_smtc_modem_hal_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (prv_modem_irq_enabled) {
		prv_smtc_modem_hal_timer_callback(prv_smtc_modem_hal_timer_context);
		smtc_modem_hal_wake_up();
	} else {
		prv_modem_irq_pending_while_disabled = true;
	}
};

void smtc_modem_hal_start_timer(const uint32_t milliseconds, void (*callback)(void *context),
				void *context)
{
	prv_smtc_modem_hal_timer_callback = callback;
	prv_smtc_modem_hal_timer_context = context;

	/* start one-shot timer */
	k_timer_start(&prv_smtc_modem_hal_timer, K_MSEC(milliseconds), K_NO_WAIT);
}

void smtc_modem_hal_stop_timer(void)
{
	k_timer_stop(&prv_smtc_modem_hal_timer);
}

/* ------------ IRQ management ------------ */

void smtc_modem_hal_disable_modem_irq(void)
{
	prv_modem_irq_enabled = false;
}

void smtc_modem_hal_enable_modem_irq(void)
{
	prv_modem_irq_enabled = true;
	lora_transceiver_board_enable_interrupt(prv_transceiver_dev);
	/* FIXME: pending timer IRQ should be called at reenable time? */
	if (prv_radio_irq_pending_while_disabled) {
		prv_radio_irq_pending_while_disabled = false;
		prv_smtc_modem_hal_radio_irq_callback(prv_smtc_modem_hal_radio_irq_context);
	}
	if (prv_modem_irq_pending_while_disabled) {
		prv_modem_irq_pending_while_disabled = false;
		prv_smtc_modem_hal_timer_callback(prv_smtc_modem_hal_timer_context);
	}
}

/* ------------ Context saving management ------------ */

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL

void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset,
				    uint8_t *buffer, const uint32_t size)
{
	prv_hal_cb->context_restore(ctx_type, offset, buffer, size);
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset,
				  const uint8_t *buffer, const uint32_t size)
{
	prv_hal_cb->context_store(ctx_type, offset, buffer, size);
}

#define CRASH_LOG_ID        0xFE
#define CRASH_LOG_STATUS_ID (CRASH_LOG_ID + 1)

void smtc_modem_hal_crashlog_store(const uint8_t *crashlog, uint8_t crash_string_length)
{
	/* We use 0xFF as the ID so we do not overwrite any of the valid contexts */
	prv_hal_cb->context_store(CRASH_LOG_ID, crashlog, crash_string_length);
}

void smtc_modem_hal_crashlog_restore(uint8_t *crashlog, uint8_t *crash_string_length)
{
	prv_hal_cb->context_restore(CRASH_LOG_ID, crashlog, crash_string_length);
}

void smtc_modem_hal_crashlog_set_status(bool available)
{
	prv_hal_cb->context_store(CRASH_LOG_STATUS_ID, (uint8_t *)&available, sizeof(available));
}

bool smtc_modem_hal_crashlog_get_status(void)
{
	bool available;

	prv_hal_cb->context_restore(CRASH_LOG_STATUS_ID, (uint8_t *)&available, sizeof(available));
	return available;
}

#else

/* FIXME: That whole storage bit should be revamped to something more generic,
 * that would remove the whole read-erase-write logics and leverage the zephyr flash stack.
 * Maybe split the flash in half, one with contexts and one with store-and-forward.
 * Maybe remove the store-and-forward circularfs backend and just use NVS.
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
	int rc;
	uint32_t real_offset;

	flash_init();
	real_offset = priv_hal_context_address(ctx_type, offset);
	rc = flash_area_read(context_flash_area, real_offset, buffer, size);
}

uint8_t page_buffer[4096];

/* We assume (FIXME:) that stores are only on one sector.
 * FIXME: we assume page size = 4096B like in nrf
 */
void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset,
				  const uint8_t *buffer, const uint32_t size)
{
	int rc;
	uint32_t real_offset;

	flash_init();
	real_offset = priv_hal_context_address(ctx_type, offset);

	/* read-erase-write */
	if (real_offset < ADDR_STORE_AND_FORWARD_CONTEXT_OFFSET) {
		memset(page_buffer, 0, 4096);
		flash_area_read(context_flash_area, 0, page_buffer, 4096);
		memset(page_buffer + real_offset, 0, size);
		memcpy(page_buffer + real_offset, buffer, size);
		flash_area_erase(context_flash_area, 0, 4096);
		rc = flash_area_write(context_flash_area, 0, page_buffer, 4096);
	} else {
		/* LOG_INF("%s: offset %d, real_offset=%d", __FUNCTION__, offset, real_offset); */
		rc = flash_area_write(context_flash_area, real_offset, buffer, size);
	}
}

/* We assume (FIXME:) that erases are aligned on sectors */
void smtc_modem_hal_context_flash_pages_erase(const modem_context_type_t ctx_type, uint32_t offset,
					      uint8_t nb_page)
{
	int rc;
	uint32_t real_offset;

	flash_init();
	real_offset = priv_hal_context_address(ctx_type, offset);
	rc = flash_area_erase(context_flash_area, real_offset,
			      smtc_modem_hal_flash_get_page_size() * nb_page);
}

#define NUM_PAGES 6

uint16_t smtc_modem_hal_store_and_forward_get_number_of_pages(void)
{
	return NUM_PAGES;
	/* return ADDR_STORE_AND_FORWARD_CONTEXT_OFFSET smtc_modem_hal_flash_get_page_size(); */
}

uint16_t smtc_modem_hal_flash_get_page_size(void)
{
	flash_init();
	return flash_area_align(context_flash_area);
}

void smtc_modem_hal_crashlog_store(const uint8_t *crashlog, uint8_t crash_string_length)
{
	flash_init();

	memset(page_buffer, 0, 4096);
	page_buffer[0] = 1;
	page_buffer[1] = crash_string_length;
	memcpy(page_buffer + 2, crashlog, crash_string_length);

	flash_area_erase(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, 4096);
	flash_area_write(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, page_buffer, 4096);

	/* prv_store("smtc_modem_hal/crashlog", crashlog, crash_string_length); */
}

void smtc_modem_hal_crashlog_restore(uint8_t *crashlog, uint8_t *crash_string_length)
{
	flash_init();
	flash_area_read(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, page_buffer, 4096);
	int available = page_buffer[0];
	int length = page_buffer[1];

	crashlog[0] = 0;
	*crash_string_length = length;

	if (available != 0) {
		memcpy(crashlog, page_buffer + 2, length);
	}
}

void smtc_modem_hal_crashlog_set_status(bool available)
{
	flash_init();
	if (!available) {
		flash_area_erase(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET,
				 smtc_modem_hal_flash_get_page_size());
	}
	/* prv_store("smtc_modem_hal/crashlog_status", (uint8_t *)&available, sizeof(available)); */
}

bool smtc_modem_hal_crashlog_get_status(void)
{
	flash_init();
	flash_area_read(context_flash_area, ADDR_CRASHLOG_CONTEXT_OFFSET, page_buffer, 1);

	/* Any other state might mean uninitialized flash area */
	return (page_buffer[0] == 1);
}

#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */

/* ------------ Panic management ------------ */

void smtc_modem_hal_on_panic(uint8_t *func, uint32_t line, const char *fmt, ...)
{
	uint8_t out_buff[255] = {0};
	uint8_t out_len = snprintf((char *)out_buff, sizeof(out_buff), "%s:%u ", func, line);
	va_list args;

	va_start(args, fmt);

	out_len += vsprintf((char *)&out_buff[out_len], fmt, args);
	va_end(args);

	/* NOTE: uint8_t *func parameter is actually __func__ casted to uint8_t*,
	 * so it can be safely printed with %s
	 */
	smtc_modem_hal_crashlog_store((uint8_t *)func, strlen(func));
	/* TODO: set_status to true is done by crashlog_store for simplicity of flash usage */
	/* smtc_modem_hal_crashlog_set_status(true); */
	LOG_ERR("Assert triggered. Crash log: %s:%u", func, line);

	/* calling assert here will halt execution if asserts are enabled and we are debugging.
	 * Otherwise, a reset of the mcu is performed
	 */
	/* __ASSERT(false, "smtc_modem_hal_assert triggered."); */

	smtc_modem_hal_reset_mcu();
}

/* ------------ Random management ------------ */

uint32_t smtc_modem_hal_get_random_nb_in_range(const uint32_t val_1, const uint32_t val_2)
{
	/* Implementation copied from the porting guide section 5.21 */
	uint32_t min = MIN(val_1, val_2);
	uint32_t max = MAX(val_1, val_2);
	uint32_t range = (max - min + 1);

	/* Fix cases when val1=0, val2=UINT32_MAX */
	range = range ? range : UINT32_MAX;
	return (uint32_t)((sys_rand32_get() % range) + min);
}

/* ------------ Radio env management ------------ */

/**
 * @brief Called when the transceiver event pin interrupt is triggered.
 *
 * If CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_GLOBAL_THREAD=y,
 * this is called in the system workq.
 * If CONFIGLORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD=y,
 * this is called in the transceiver event thread.
 *
 * @param[in] dev The transceiver device.
 */
void prv_transceiver_event_cb(const struct device *dev)
{
	if (prv_modem_irq_enabled) {
		/* Due to the way the transceiver driver is implemented,
		 * this is called from the system workq.
		 */
		prv_smtc_modem_hal_radio_irq_callback(prv_smtc_modem_hal_radio_irq_context);
		smtc_modem_hal_wake_up();
	} else {
		prv_radio_irq_pending_while_disabled = true;
	}
}

void smtc_modem_hal_irq_config_radio_irq(void (*callback)(void *context), void *context)
{
	/* save callback function and context */
	prv_smtc_modem_hal_radio_irq_context = context;
	prv_smtc_modem_hal_radio_irq_callback = callback;

	/* enable callback via transceiver driver */
	lora_transceiver_board_attach_interrupt(prv_transceiver_dev, prv_transceiver_event_cb);
	lora_transceiver_board_enable_interrupt(prv_transceiver_dev);
}

void smtc_modem_hal_irq_reset_radio_irq(void)
{
	lora_transceiver_board_disable_interrupt(prv_transceiver_dev);
	lora_transceiver_board_attach_interrupt(prv_transceiver_dev, prv_transceiver_event_cb);
	lora_transceiver_board_enable_interrupt(prv_transceiver_dev);
}

void smtc_modem_hal_radio_irq_clear_pending(void)
{
	/* Unimplemented as this is a corner case without consequences
	 * and Zephyr does not provide such API.
	 */
}

void smtc_modem_hal_start_radio_tcxo(void)
{
	/* TODO: We only support TCXO's that are wired to the transceiver.
	 * In such cases, this function must be empty. See 5.25 of the porting guide.
	 */
}

void smtc_modem_hal_stop_radio_tcxo(void)
{
	/* TODO: We only support TCXO's that are wired to the transceiver.
	 * In such cases, this function must be empty. See 5.26 of the porting guide.
	 */
}

uint32_t smtc_modem_hal_get_radio_tcxo_startup_delay_ms(void)
{
	/* From the porting guide:
	 * If the TCXO is configured by the RAL BSP to start up automatically, then the value used
	 * here should be the same as the startup delay used in the RAL BSP.
	 */
	return lora_transceiver_get_tcxo_startup_delay_ms(prv_transceiver_dev);
}

void smtc_modem_hal_set_ant_switch(bool is_tx_on)
{
	/* From the porting guide:
	 * If no antenna switch is used then implement an empty command.
	 */
}

/* ------------ Environment management ------------ */

/* This is called when LoRa Cloud sends a device status request */
uint8_t smtc_modem_hal_get_battery_level(void)
{
	uint32_t battery;
	int ret = prv_hal_cb->get_battery_level(&battery);

	if (ret) {
		return 0;
	}

	if (battery > 1000) {
		return 255;
	}

	/* scale 0-1000 to 0-255 */
	return (uint8_t)((double)battery * 0.255);
}

/* This is called when DM info required is */
int8_t smtc_modem_hal_get_temperature(void)
{
	int32_t temperature;
	int ret = prv_hal_cb->get_temperature(&temperature);

	if (ret) {
		return -128;
	}

	if (temperature < -128) {
		return -128;
	} else if (temperature > 127) {
		return 127;
	} else {
		return (int8_t)temperature;
	}
}

/* This is called when DM info required is */
uint16_t smtc_modem_hal_get_voltage_mv(void)
{
	uint32_t voltage;
	int ret = prv_hal_cb->get_voltage(&voltage);

	if (ret) {
		return 0;
	}

	/* Step used in Semtech's hal is 1/50V == 20 mV */
	/* FIXME: uint32_t converted = voltage / 20; */

	if (voltage > 65535) {
		return 65535;
	} else {
		return (uint16_t)voltage;
	}
}

/* ------------ Misc ------------ */

int8_t smtc_modem_hal_get_board_delay_ms(void)
{
	/* the wakeup time is probably closer to 0ms then 1ms,
	 * but just to be safe:
	 */
	return 1;
}

/*
 * NOTE: smtc_modem_hal_print_trace() is implemented in ./logging/smtc_modem_hal_additional_prints.c
 */

/* ------------ FUOTA ------------ */
/* https://resources.lora-alliance.org/technical-specifications/ts006-1-0-0-firmware-management-protocol
 */
#if defined(CONFIG_LORA_BASICS_MODEM_FUOTA)

uint32_t smtc_modem_hal_get_hw_version_for_fuota(void)
{
	uint32_t hw_version;
	int ret = prv_hal_cb->get_hw_version_for_fuota(&hw_version);

	if (ret) {
		LOG_ERR("Failed to get hardware version");
		return 0;
	}
	return hw_version;
}

uint32_t smtc_modem_hal_get_fw_version_for_fuota(void)
{
	uint32_t fw_version;
	int ret = prv_hal_cb->get_fw_version_for_fuota(&fw_version);

	if (ret) {
		LOG_ERR("Failed to get firmware version");
		return 0;
	}
	return fw_version;
}

uint8_t smtc_modem_hal_get_fw_status_available_for_fuota(void)
{
	uint8_t fw_status;
	int ret = prv_hal_cb->get_fw_status_available_for_fuota(&fw_status);

	if (ret) {
		LOG_ERR("Failed to get firmware status");
		return 0;
	}
	return fw_status;
}

uint32_t smtc_modem_hal_get_next_fw_version_for_fuota(void)
{
	uint32_t next_fw_version;
	int ret = prv_hal_cb->get_next_fw_version_for_fuota(&next_fw_version);

	if (ret) {
		LOG_ERR("Failed to get next firmware version");
		return 0;
	}
	return next_fw_version;
}

uint8_t smtc_modem_hal_get_fw_delete_status_for_fuota(uint32_t fw_to_delete_version)
{
	if (fw_to_delete_version != smtc_modem_hal_get_next_fw_version_for_fuota()) {
		return 2;
	}

	int ret = prv_hal_cb->get_fw_delete_status_for_fuota(fw_to_delete_version);

	if (ret) {
		LOG_ERR("Failed to delete firmware");
	}
	return ret;
}
#endif /* USE_FUOTA */

/* ------------ For Real Time OS compatibility  ------------*/

void smtc_modem_hal_user_lbm_irq(void)
{
	/* Do nothing in case implementation is bare metal */
}
