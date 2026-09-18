/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_HWINFO
#include <zephyr/drivers/hwinfo.h>
#endif
#ifdef CONFIG_INPUT
#include <zephyr/input/input.h>
#endif
#include <zephyr/knx/knx_pkt.h>
#include "object_device.h"
#include "object_interface.h"
#include "object_memory.h"

LOG_MODULE_REGISTER(knx_device, CONFIG_KNX_STACK_LOG_LEVEL);

static knx_u16_be_t _object_type = KNX_U16_BE(OT_DEVICE);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _object_type);
static uint8_t _serial_number[6] = {0x00, 0xFA, 0x12, 0x34, 0x56, 0x78};
KNX_PROP_ASSERT_SIZE(PDT_GENERIC_06, _serial_number);
static uint8_t _manufacturer_id[2] = {0x00, 0xFA};
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _manufacturer_id);
static uint8_t _hardware_type[6] = {0, 0, 0, 0, 0, 0};
KNX_PROP_ASSERT_SIZE(PDT_GENERIC_06, _hardware_type);
static uint8_t _version[2] = {0, 0};
KNX_PROP_ASSERT_SIZE(PDT_VERSION, _version);
static uint8_t _prog_mode;
KNX_PROP_ASSERT_SIZE(PDT_BITSET8, _prog_mode);
/*
 * PID_MAX_APDU_LENGTH — 2 octets, KNX wire (big-endian) order.
 *
 * Derived from KNX_MAX_APDU_OCTETS so the published value cannot drift away
 * from what the transmit path can actually carry.  It used to be hard-coded to
 * 0x00FE (254), which was a promise the driver could not keep: knx_l1_send_frame()
 * refuses anything past the NCN5130's first 64-octet block, so a client taking
 * 254 at face value would send frames this device silently drops.
 */
static uint8_t _max_apdu_length[2] = {
	(uint8_t)((KNX_MAX_APDU_OCTETS >> 8) & 0xFFu),
	(uint8_t)(KNX_MAX_APDU_OCTETS & 0xFFu),
};
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _max_apdu_length);
static knx_u16_be_t _device_descriptor = KNX_U16_BE(0x07B0);
KNX_PROP_ASSERT_SIZE(PDT_GENERIC_02, _device_descriptor);
static uint8_t _product_id[10] = {0x00, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
KNX_PROP_ASSERT_SIZE(PDT_GENERIC_10, _product_id);
static uint8_t _order_info[10] = {0x00, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
KNX_PROP_ASSERT_SIZE(PDT_GENERIC_10, _order_info);
/* PID_PEI_TYPE — 0 = no Physical External Interface (ETS product: PeiType="0"). */
static uint8_t _pei_type;
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_CHAR, _pei_type);
/* PID_MAX_RETRY_COUNT — 0 bbb 0 nnn: busy_retry = 3, nak_retry = 3. */
static uint8_t _max_retry_count = 0x33;
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_CHAR, _max_retry_count);
/*
 * PID_ERROR_FLAGS (53) — 03_05_01 §4.23.2, Volume 6 §A.2.3.2 (p.142):
 * mandatory, read-only bitset of device-level fault categories. Was missing
 * entirely from the Device Object. No bit is currently ever set — this
 * device's only tracked faults today are per-table Load State Machine
 * errors, which live in each table object's own PID_ERROR_CODE instead
 * (see set_error_code() in object_interface.c) — but the property must
 * exist and read as "no fault" (all bits 0) for a client that checks it.
 */
static uint8_t _error_flags;
KNX_PROP_ASSERT_SIZE(PDT_BITSET8, _error_flags);
static char _name[13] = "ObjectDevice";
/*
 * PID_IO_LIST element count, host order (interface_read_property() reads
 * nr_of_elem natively and re-emits it big-endian itself).
 *
 * NUMBER_OF_INTERFACE, not +1: object_interfaces_types[] has
 * N+1 slots with the count mirrored at index 0 and the types at 1..N, so
 * advertising N+1 elements made a client read index N+1 — one past the array.
 */
static uint16_t _nb_interface = NUMBER_OF_INTERFACE;

static void (*s_addr_changed_cb)(uint16_t);

#if DT_NODE_HAS_PROP(DT_CHOSEN(knx_prog_led), gpios)
static const struct gpio_dt_spec s_prog_led = GPIO_DT_SPEC_GET(DT_CHOSEN(knx_prog_led), gpios);
#define HAS_PROG_LED 1
#endif

/*
 * __memory fields backing writable properties below are checked against
 * their PDT here, next to the properties array that declares them, since
 * their declaration itself lives in struct Memory (object_memory.h).
 */
KNX_PROP_ASSERT_SIZE(PDT_BITSET8, __memory.device_control);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_CHAR, __memory.routing_count);

struct property object_device_properties[] = {
	KNX_PROP_SCALAR(PID_OBJECT_TYPE, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_object_type),
	KNX_PROP_CHARS(PID_OBJECT_NAME, false, ReadLv3 | WriteLv0, _name),
	KNX_PROP_SCALAR(PID_SERIAL_NUMBER, PDT_GENERIC_06, false, ReadLv3 | WriteLv0,
			&_serial_number),
	KNX_PROP_SCALAR(PID_PRODUCT_ID, PDT_GENERIC_10, false, ReadLv3 | WriteLv0, &_product_id),
	KNX_PROP_SCALAR(PID_MANUFACTURER_ID, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_manufacturer_id),
	/*
	 * PID_DEVICE_CONTROL — Volume 6 §A.2.3 gives 3/3 for mask 07B0h:
	 * mandatory AND writable at level 3.  ETS writes it to turn on Verify
	 * Mode (bit 2), which a product whose load procedure carries
	 * Verify="true" depends on — with write_enable = false and §1.7 now
	 * enforcing it, that download would be refused.
	 */
	KNX_PROP_SCALAR(PID_DEVICE_CONTROL, PDT_BITSET8, true, ReadLv3 | WriteLv3,
			&__memory.device_control),
	/* PID_ORDER_INFO — 10-octet manufacturer order info, 3/x (mandatory,
	 * read-only).  Has its own storage: it used to alias _product_id, so the
	 * two properties could never differ.
	 */
	KNX_PROP_SCALAR(PID_ORDER_INFO, PDT_GENERIC_10, false, ReadLv3 | WriteLv0, &_order_info),
	/*
	 * PID_PEI_TYPE — Volume 6 §A.2.3 gives 3/x for mask 07B0h: MANDATORY and
	 * read-only, and it was missing entirely from the Device Object (only the
	 * Application Program object had one).  0 = no Physical External
	 * Interface, matching PeiType="0" in the ETS product description.
	 */
	KNX_PROP_SCALAR(PID_PEI_TYPE, PDT_UNSIGNED_CHAR, false, ReadLv3 | WriteLv0, &_pei_type),
	/*
	 * PID_MAX_RETRY_COUNT — Volume 6 §A.2.3 gives 3/3: MANDATORY and
	 * writable, and it was missing.  One octet, 0 bbb 0 nnn: bits[6:4] =
	 * busy_retry, bits[2:0] = nak_retry (KNX spec 3/5/1).  Conveniently the
	 * same layout as the NCN5130's U_SetRepetition.req data byte.
	 *
	 * 3/3 describes the effective behaviour today: L2 retries a frame up to
	 * six times, i.e. nak_retry + busy_retry + 1 = 7 transmissions, which is
	 * exactly what 3/3 means (KNX spec 3/2/2 §2.4.2).  Actually feeding this
	 * value to the transceiver and splitting the NAK and BUSY cases apart is
	 * not yet implemented.
	 */
	KNX_PROP_SCALAR(PID_MAX_RETRY_COUNT, PDT_UNSIGNED_CHAR, true, ReadLv3 | WriteLv3,
			&_max_retry_count),
	KNX_PROP_SCALAR(PID_ERROR_FLAGS, PDT_BITSET8, false, ReadLv3 | WriteLv0, &_error_flags),
	KNX_PROP_SCALAR(PID_HARDWARE_TYPE, PDT_GENERIC_06, false, ReadLv3 | WriteLv0,
			&_hardware_type),
	KNX_PROP_SCALAR(PID_VERSION, PDT_VERSION, false, ReadLv3 | WriteLv0, &_version),
	/* PID_ROUTING_COUNT — Volume 6 §A.2.3 gives 3/3 for mask 07B0h:
	 * mandatory and writable, so ETS can set the hop count.
	 */
	KNX_PROP_SCALAR(PID_ROUTING_COUNT, PDT_UNSIGNED_CHAR, true, ReadLv3 | WriteLv3,
			&__memory.routing_count),
	KNX_PROP_SCALAR(PID_PROGMODE, PDT_BITSET8, true, ReadLv3 | WriteLv3, &_prog_mode),
	KNX_PROP_SCALAR(PID_MAX_APDU_LENGTH, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_max_apdu_length),
	KNX_PROP_ARRAY(PID_IO_LIST, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
		       NUMBER_OF_INTERFACE + 1, &_nb_interface, &object_interfaces_types, false),
	KNX_PROP_SCALAR(PID_DEVICE_DESCRIPTOR, PDT_GENERIC_02, false, ReadLv3 | WriteLv0,
			&_device_descriptor),
	{.description = {.property_id = PID_LAST}}};

void object_device_properties_written(PropertyID id)
{
	switch (id) {
	case PID_PROGMODE:
		device_prog_mode_set(_prog_mode != 0);
		break;
	case PID_ROUTING_COUNT:
		/* Lives in __memory, so a write has to reach flash. PID_DEVICE_CONTROL
		 * deliberately does NOT: memory_read() clears it on every start-up,
		 * because the spec requires those bits to reset on power-up.
		 */
		memory_modified();
		break;
	default:
		break;
	}
}

uint16_t device_individual_address(void)
{
	return ((uint16_t)__memory.subnet_addr << 8) | __memory.device_addr;
}

void device_individual_address_set(uint16_t value)
{
	__memory.subnet_addr = (value >> 8) & 0xFF;
	__memory.device_addr = (value >> 0) & 0xFF;
	if (s_addr_changed_cb != NULL) {
		s_addr_changed_cb(value);
	}
}

void device_individual_address_changed_set_cb(void (*callback)(uint16_t newvalue))
{
	s_addr_changed_cb = callback;
}

bool device_prog_mode(void)
{
	return _prog_mode != 0;
}

void device_prog_mode_set(bool value)
{
	_prog_mode = value ? 1 : 0;
#ifdef HAS_PROG_LED
	gpio_pin_set_dt(&s_prog_led, _prog_mode);
#endif
	LOG_INF("KNX prog mode: %s", value ? "ON" : "OFF");
}

void device_set_ia_duplication(void)
{
	/* PID_DEVICE_CONTROL (PID=14) bit 1: IA duplication detected.
	 * Spec §14.9: set when own IA is seen as SA in a received frame.
	 */
	__memory.device_control |= 0x02u;
	memory_modified();
	LOG_WRN("IA duplication detected — own address seen on bus");
}

void device_clear_verify_mode(void)
{
	/* PID_DEVICE_CONTROL (PID=14) bit 2: Verify Mode. Only active during
	 * an open TL connection — auto-cleared the instant it closes.
	 */
	if (__memory.device_control & 0x04u) {
		__memory.device_control &= ~0x04u;
		memory_modified();
		LOG_DBG("Verify Mode cleared on TL disconnect");
	}
}

/*
 * device_factory_format — DEBUG ONLY, not called anywhere in the stack.
 *
 * Wipes the individual address and every downloaded table (Address,
 * Association, Group Object, Application Program) back to their
 * unloaded/default state, as if the device had just come out of the
 * factory. Call it manually — e.g. temporarily from main() — to get a
 * clean slate for a new ETS download without a full flash erase.
 *
 * Unloading each table via load_state_machine_event() (rather than just
 * zeroing __memory) also resets PID_LOAD_STATE_CONTROL to LS_UNLOADED and
 * clears PID_TABLE_REFERENCE, matching the state ETS expects at the start
 * of a "Download all" sequence.
 *
 * memory_reset() must run FIRST: Load State now lives inside
 * __memory, so calling it after the unload events would immediately
 * overwrite them with the CONFIG_KNX_AUTO_LOAD_TABLES default — this
 * function's whole point is to force LS_UNLOADED regardless of that
 * default, so the explicit unload events must be the last word.
 */
void device_factory_format(void)
{
	uint8_t unload[1] = {LE_UNLOAD};

	LOG_WRN("%s: wiping individual address and all tables", __func__);

	memory_reset();

	load_state_machine_event(OBJ_IDX_ADDRESS_TABLE, unload, sizeof(unload));
	load_state_machine_event(OBJ_IDX_ASSOCIATION_TABLE, unload, sizeof(unload));
	load_state_machine_event(OBJ_IDX_GROUP_OBJ_TABLE, unload, sizeof(unload));
	load_state_machine_event(OBJ_IDX_APPLICATION_PROG, unload, sizeof(unload));
	load_state_machine_event(OBJ_IDX_INTERFACE_PROG, unload, sizeof(unload));

	memory_write();

	LOG_WRN("%s: done — individual address is now 0xFFFF", __func__);
}

/* Stored in wire order for the property path; A_DeviceDescriptor_Read wants a
 * host-order value, which it then serialises itself.
 */
uint16_t device_descriptor(void)
{
	return knx_u16_be_get(_device_descriptor);
}
uint16_t device_manufacturer_id(void)
{
	return ((uint16_t)_manufacturer_id[0] << 8) | _manufacturer_id[1];
}

uint8_t device_serial_number(int i)
{
	if (i >= 6) {
		return 0;
	}
	return _serial_number[i];
}

uint8_t device_hardware_type(int i)
{
	if (i >= 6) {
		return 0;
	}
	return _hardware_type[i];
}

uint16_t device_version(void)
{
	return ((uint16_t)_version[0] << 8) | _version[1];
}
uint8_t device_routing_count(void)
{
	return __memory.routing_count;
}

#ifdef CONFIG_INPUT
static void prog_btn_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);
	if (evt->type == INPUT_EV_KEY && evt->code == INPUT_KEY_0 && evt->value == 1) {
		device_prog_mode_set(!device_prog_mode());
	}
}
INPUT_CALLBACK_DEFINE(NULL, prog_btn_cb, NULL);
#endif

static int device_obj_init(void)
{
	LOG_DBG("INIT");
#ifdef HAS_PROG_LED
	if (!gpio_is_ready_dt(&s_prog_led)) {
		LOG_ERR("KNX prog LED not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&s_prog_led, GPIO_OUTPUT_INACTIVE);
#endif
#ifdef CONFIG_HWINFO
	uint8_t uid[12]; /* 3 × 32-bit words = 96 bits */
	ssize_t len = hwinfo_get_device_id(uid, sizeof(uid));

	if (len > 6) {
		for (int i = 0; i < 4; i++) {
			_serial_number[i + 2] = uid[8 + i];
			LOG_DBG("HW SER");
		}
	}
#endif
	return 0;
}
SYS_INIT(device_obj_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
